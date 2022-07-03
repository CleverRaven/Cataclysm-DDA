#include "json_loader.h"

#include <memory>
#include <unordered_map>

#include <ghc/fs_std_fwd.hpp>

#include "flexbuffer_cache.h"
#include "flexbuffer_json.h"
#include "path_info.h"

namespace
{

flexbuffer_cache &global_cache()
{
    static flexbuffer_cache cache{ fs::u8path( PATH_INFO::cache_dir() ), fs::u8path( PATH_INFO::base_path() ) };
    return cache;
}

// The file pointed to by source_file must exist.
cata::optional<JsonValue> from_path_at_offset_opt_impl( fs::path source_file, size_t offset )
{
    std::shared_ptr<parsed_flexbuffer> buffer = global_cache().parse_and_cache(
                std::move( source_file ), offset );
    if( !buffer ) {
        return cata::nullopt;
    }

    flexbuffers::Reference buffer_root = flexbuffer_root_from_storage( buffer->get_storage() );
    return JsonValue( std::move( buffer ), buffer_root, nullptr, 0 );
}

} // namespace

cata::optional<JsonValue> json_loader::from_path_at_offset_opt( fs::path source_file,
        size_t offset ) noexcept( false )
{
    if( !file_exist( source_file ) ) {
        return cata::nullopt;
    }
    return from_path_at_offset_opt_impl( std::move( source_file ), offset );
}

cata::optional<JsonValue> json_loader::from_path_opt( fs::path source_file ) noexcept( false )
{
    return from_path_at_offset_opt( std::move( source_file ), 0 );
}

JsonValue json_loader::from_path_at_offset( const fs::path &source_file,
        size_t offset ) noexcept( false )
{
    if( !file_exist( source_file ) ) {
        throw JsonError( source_file.generic_u8string() + " does not exist." );
    }
    auto obj = from_path_at_offset_opt_impl( source_file, offset );

    if( !obj ) {
        throw JsonError( "Json file " + source_file.generic_u8string() + " did not contain valid json" );
    }
    return std::move( *obj );
}

JsonValue json_loader::from_path( const fs::path &source_file ) noexcept( false )
{
    return from_path_at_offset( source_file, 0 );
}

JsonValue json_loader::from_string( std::string const &data ) noexcept( false )
{
    std::shared_ptr<parsed_flexbuffer> buffer = flexbuffer_cache::parse_buffer( data );
    if( !buffer ) {
        throw JsonError( "Failed to parse string into json" );
    }

    flexbuffers::Reference buffer_root = flexbuffer_root_from_storage( buffer->get_storage() );
    return JsonValue( std::move( buffer ), buffer_root, nullptr, 0 );
}
