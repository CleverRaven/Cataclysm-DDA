#include "json_loader.h"

#include <memory>
#include <unordered_map>

#include <ghc/fs_std_fwd.hpp>

#include "filesystem.h"
#include "flexbuffer_cache.h"
#include "flexbuffer_json.h"
#include "path_info.h"

namespace
{
flexbuffer_cache &base_cache()
{
    static flexbuffer_cache cache{ fs::u8path( PATH_INFO::base_path() ) / "cache", fs::u8path( PATH_INFO::base_path() ) };
    return cache;
}

flexbuffer_cache &config_cache()
{
    static flexbuffer_cache cache{ fs::u8path( PATH_INFO::config_dir() ) / "cache", fs::u8path( PATH_INFO::config_dir() ) };
    return cache;
}

flexbuffer_cache &data_cache()
{
    static flexbuffer_cache cache{ fs::u8path( PATH_INFO::datadir() ) / "cache", fs::u8path( PATH_INFO::datadir() ) };
    return cache;
}

flexbuffer_cache &memorial_cache()
{
    static flexbuffer_cache cache{ fs::u8path( PATH_INFO::memorialdir() ) / "cache", fs::u8path( PATH_INFO::memorialdir() ) };
    return cache;
}

flexbuffer_cache &user_cache()
{
    static flexbuffer_cache cache{ fs::u8path( PATH_INFO::user_dir() ) / "cache", fs::u8path( PATH_INFO::user_dir() ) };
    return cache;
}

std::unordered_map<std::string, std::unique_ptr<flexbuffer_cache>> save_caches;
std::unordered_map<std::string, std::unordered_map<std::string, std::unique_ptr<flexbuffer_cache>>>
world_to_character_caches;

flexbuffer_cache &cache_for_save( const cata_path &path )
{
    // Assume lexically normal path
    auto path_it = path.get_relative_path().begin();
    // First path element is the world name
    std::string worldname = path_it->u8string();
    ++path_it;
    // Next element is either a file, a character folder, or the maps folder
    std::string folder_or_file = path_it->u8string();
    ++path_it;

    auto it = save_caches.find( worldname );
    if( it == save_caches.end() ) {
        it = save_caches.emplace( worldname,
                                  std::make_unique<flexbuffer_cache>( fs::u8path( PATH_INFO::savedir() ) / worldname / "cache",
                                          fs::u8path( PATH_INFO::savedir() ) / worldname ) ).first;
    }

    if( folder_or_file == "maps" ) {
        // Generic per-save cache is fine.
        return *it->second;
    }

    // Either a global save file or a per-character file.
    fs::path test_path = fs::u8path( PATH_INFO::savedir() ) / worldname / folder_or_file;
    if( fs::is_directory( test_path ) ) {
        // Character file.
        const std::string &character_name = folder_or_file;
        std::unordered_map<std::string, std::unique_ptr<flexbuffer_cache>> &character_caches_for_world =
                    world_to_character_caches[worldname];
        it = character_caches_for_world.find( character_name );
        if( it == character_caches_for_world.end() ) {
            it = character_caches_for_world.emplace( character_name,
                    std::make_unique<flexbuffer_cache>( test_path / "cache", test_path ) ).first;
        }
    }
    return *it->second;
}

flexbuffer_cache &cache_for_lexically_normal_path( const cata_path &path )
{
    switch( path.get_logical_root() ) {
        case cata_path::root_path::base:
            return base_cache();
        case cata_path::root_path::config:
            return config_cache();
        case cata_path::root_path::data:
            return data_cache();
        case cata_path::root_path::memorial:
            return memorial_cache();
        case cata_path::root_path::save:
            // Saves we store data one per save.
            // But also... per character per save.
            return cache_for_save( path );
        case cata_path::root_path::user:
            return user_cache();
        case cata_path::root_path::unknown:
        default:
            cata_fatal( "Cannot create cache for unknown path %s",
                        path.generic_u8string() );
    }
}

// The file pointed to by source_file must exist.
cata::optional<JsonValue> from_path_at_offset_opt_impl( const cata_path &source_file,
        size_t offset )
{
    cata_path lexically_normal_path = source_file.lexically_normal();
    std::shared_ptr<parsed_flexbuffer> buffer;
    if( lexically_normal_path.get_logical_root() != cata_path::root_path::unknown ) {
        flexbuffer_cache &cache = cache_for_lexically_normal_path( lexically_normal_path );
        buffer = cache.parse_and_cache(
                     lexically_normal_path.get_unrelative_path(), offset );
    } else {
        buffer = flexbuffer_cache::parse( lexically_normal_path.get_unrelative_path(), offset );
    }
    if( !buffer ) {
        return cata::nullopt;
    }

    flexbuffers::Reference buffer_root = flexbuffer_root_from_storage( buffer->get_storage() );
    return JsonValue( std::move( buffer ), buffer_root, nullptr, 0 );
}

} // namespace

cata::optional<JsonValue> json_loader::from_path_at_offset_opt( const cata_path &source_file,
        size_t offset ) noexcept( false )
{
    if( !file_exist( source_file.get_unrelative_path() ) ) {
        return cata::nullopt;
    }
    return from_path_at_offset_opt_impl( source_file, offset );
}

cata::optional<JsonValue> json_loader::from_path_opt( const cata_path &source_file ) noexcept(
    false )
{
    return from_path_at_offset_opt( source_file, 0 );
}

JsonValue json_loader::from_path_at_offset( const cata_path &source_file,
        size_t offset ) noexcept( false )
{
    fs::path unrelative_path = source_file.get_unrelative_path();
    if( !file_exist( unrelative_path ) ) {
        throw JsonError( unrelative_path.generic_u8string() + " does not exist." );
    }
    auto obj = from_path_at_offset_opt_impl( source_file, offset );

    if( !obj ) {
        throw JsonError( "Json file " + unrelative_path.generic_u8string() +
                         " did not contain valid json" );
    }
    return std::move( *obj );
}

JsonValue json_loader::from_path( const cata_path &source_file ) noexcept( false )
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

cata::optional<JsonValue> json_loader::from_string_opt( std::string const &data ) noexcept( false )
{
    cata::optional<JsonValue> ret;
    try {
        ret = from_string( data );
    } catch( JsonError &e ) {
        ret = cata::nullopt;
    }
    return ret;
}
