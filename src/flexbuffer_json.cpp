#include "flexbuffer_json.h"

#include <cstring>
#include <istream>

#include "cata_unreachable.h"
#include "filesystem.h"
#include "json.h"
#include "optional.h"

const std::string &FlexJson::flexbuffer_type_to_string( flexbuffers::Type t )
{
    static const std::array<std::string, 36> type_map = { {
            "null",
            "int",
            "uint",
            "float",
            "key",
            "string",
            "indirect_int",
            "indirect_uint",
            "indirect_float",
            "object",
            "array",
            "int_array",
            "uint_array",
            "float_array",
            "key_array",
            "string_array_deprecated",
            "int_tuple_array",
            "uint_tuple_array",
            "float_tuple_array",
            "int_triple_array",
            "uint_triple_array",
            "float_triple_array",
            "int_quad_array",
            "uint_quad_array",
            "float_quad_array",
            "blob",
            "bool",
            "bool_array",
        }
    };
    return type_map[static_cast<unsigned int>( t )];
}

// Advances the given jsin according to the passed JsonPath
static void advance_jsin( JsonIn *jsin, flexbuffers::Reference root, const JsonPath &path )
{
    for( JsonPath::index_type idx : path ) {
        if( root.IsMap() ) {
            std::string member = root.AsMap().Keys()[ idx ].AsString().str();
            jsin->start_object();
            while( !jsin->end_object() ) {
                if( jsin->get_member_name() == member ) {
                    break;
                }
                jsin->skip_value();
            }
            root = root.AsVector()[ idx ];
        } else if( root.IsVector() ) {
            jsin->start_array();
            for( size_t i = 0; i < idx && !jsin->end_array(); ++i ) {
                jsin->skip_value();
            }
            root = root.AsVector()[ idx ];
        } else {
            cata_unreachable();
        }
    }
}

void FlexJson::throw_error( const JsonPath &path, int offset, const std::string &message ) const
{
    std::unique_ptr<std::istream> original_json = root_->get_source_stream();
    std::string source_path = [&] {
        if( root_->get_source_path().empty() )
        {
            return std::string{ "<unknown source file>" };
        }
        return root_->get_source_path().u8string();
    }();
    JsonIn jsin( *original_json, source_path );

    advance_jsin( &jsin, flexbuffer_root_from_storage( root_->get_storage() ), path );

    jsin.error( offset, message );
}

void FlexJson::throw_error_after( const JsonPath &path, const std::string &message ) const
{
    std::unique_ptr<std::istream> original_json = root_->get_source_stream();
    std::string source_path = [&] {
        if( root_->get_source_path().empty() )
        {
            return std::string{ "<unknown source file>" };
        }
        return root_->get_source_path().u8string();
    }();
    JsonIn jsin( *original_json, source_path );

    advance_jsin( &jsin, flexbuffer_root_from_storage( root_->get_storage() ), path );
    jsin.skip_value();

    jsin.error( message );
}

void FlexJson::string_error( const JsonPath &path, int offset, const std::string &message ) const
{
    std::unique_ptr<std::istream> original_json = root_->get_source_stream();
    std::string source_path = [&] {
        if( root_->get_source_path().empty() )
        {
            return std::string{ "<unknown source file>" };
        }
        return root_->get_source_path().u8string();
    }();
    JsonIn jsin( *original_json, source_path );

    advance_jsin( &jsin, flexbuffer_root_from_storage( root_->get_storage() ), path );

    jsin.string_error( offset, message );
}

std::string FlexJson::str() const
{
    std::string ret;
    json_.ToString( false, true, ret );
    return ret;
}

cata::optional<FlexJsonValue> FlexJsonValue::fromOpt( fs::path source_file,
        size_t offset ) noexcept( false )
{
    if( !file_exist( source_file ) ) {
        return cata::nullopt;
    }

    std::shared_ptr<parsed_flexbuffer> buffer = flexbuffer_cache::global_cache().parse_and_cache(
                source_file, offset );
    if( !buffer ) {
        return cata::nullopt;
    }

    flexbuffer buffer_root = flexbuffer_root_from_storage( buffer->get_storage() );
    return FlexJsonValue{ std::move( buffer ), buffer_root, nullptr, 0 };
}

FlexJsonValue FlexJsonValue::from( fs::path source_file, size_t offset ) noexcept( false )
{
    if( !file_exist( source_file ) ) {
        throw JsonError( source_file.generic_u8string() + " does not exist." );
    }
    cata::optional<FlexJsonValue> obj = fromOpt( source_file, offset );

    if( !obj ) {
        throw JsonError( "Json file " + source_file.generic_u8string() + " did not contain valid json" );
    }
    return std::move( *obj );
}

FlexJsonValue FlexJsonValue::fromString( const std::string &data ) noexcept( false )
{
    std::shared_ptr<parsed_flexbuffer> buffer = flexbuffer_cache::parse_buffer( data );
    if( !buffer ) {
        throw JsonError( "Failed to parse string into json" );
    }

    flexbuffer buffer_root = flexbuffer_root_from_storage( buffer->get_storage() );
    return FlexJsonValue{ std::move( buffer ), buffer_root, nullptr, 0 };
}

bool FlexJsonValue::read( bool &b, bool throw_on_error ) const
{
    if( !test_bool() ) {
        return error_or_false( throw_on_error, "Expected bool" );
    }
    b = get_bool();
    return true;
}
bool FlexJsonValue::read( char &c, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    c = get_int();
    return true;
}
bool FlexJsonValue::read( signed char &c, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    c = get_int();
    return true;
}
bool FlexJsonValue::read( unsigned char &c, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    c = get_int();
    return true;
}
bool FlexJsonValue::read( short unsigned int &s, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    s = get_int();
    return true;
}
bool FlexJsonValue::read( short int &s, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    // TODO: test for overflow
    s = get_int();
    return true;
}
bool FlexJsonValue::read( int &i, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    i = get_int();
    return true;
}
bool FlexJsonValue::read( int64_t &i, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    i = get_int64();
    return true;
}
bool FlexJsonValue::read( uint64_t &i, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    i = get_uint64();
    return true;
}
bool FlexJsonValue::read( unsigned int &u, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    u = get_uint();
    return true;
}
bool FlexJsonValue::read( float &f, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    f = get_float();
    return true;
}
bool FlexJsonValue::read( double &d, bool throw_on_error ) const
{
    if( !test_number() ) {
        return error_or_false( throw_on_error, "Expected number" );
    }
    d = get_float();
    return true;
}
bool FlexJsonValue::read( std::string &s, bool throw_on_error ) const
{
    if( !test_string() ) {
        return error_or_false( throw_on_error, "Expected string" );
    }
    s = get_string();
    return true;
}

void FlexJsonObject::report_unvisited() const
{
    if( !visited_fields_bitset_.all() ) {
        std::vector<size_t> skipped_members;
        skipped_members.reserve( visited_fields_bitset_.size() );
        tiny_bitset::block_t *bits = visited_fields_bitset_.bits();
        size_t block_idx = 0;
        for( size_t last_whole_block = visited_fields_bitset_.size() / tiny_bitset::kBitsPerBlock;
             block_idx < last_whole_block; ++block_idx ) {
            tiny_bitset::block_t block = bits[block_idx];
            tiny_bitset::block_t mask = tiny_bitset::kLowBit << ( tiny_bitset::kBitsPerBlock - 1 );
            for( size_t bit_idx = 0; bit_idx < tiny_bitset::kBitsPerBlock; ++bit_idx ) {
                if( block & mask ) {
                    skipped_members.emplace_back( block_idx * tiny_bitset::kBitsPerBlock + bit_idx );
                }
                mask >>= 1;
            }
        }

        tiny_bitset::block_t block = bits[block_idx];
        tiny_bitset::block_t mask = tiny_bitset::kLowBit << ( tiny_bitset::kBitsPerBlock - 1 );
        for( size_t bit_idx = 0, end = visited_fields_bitset_.size() % tiny_bitset::kBitsPerBlock;
             bit_idx < end; ++bit_idx ) {
            if( !( block & mask ) ) {
                skipped_members.emplace_back( block_idx * tiny_bitset::kBitsPerBlock + bit_idx );
            }
            mask >>= 1;
        }

        error_skipped_members( skipped_members );
    }
}

void FlexJsonObject::error_no_member( const std::string &member ) const
{
    std::unique_ptr<std::istream> original_json = root_->get_source_stream();
    std::string source_path = [&] {
        if( root_->get_source_path().empty() )
        {
            return std::string{ "<unknown source file>" };
        }
        return root_->get_source_path().u8string();
    }();
    JsonIn jsin( *original_json, source_path );

    advance_jsin( &jsin, flexbuffer_root_from_storage( root_->get_storage() ), path_ );

    JsonObject jo = jsin.get_object();
    jo.allow_omitted_members();
    jo.get_member( member );
    // Just to make sure the compiler understands we will error earlier.
    jo.throw_error( "Failed to report missing member " + member );
}

void FlexJsonObject::error_skipped_members( const std::vector<size_t> &skipped_members ) const
{
    std::unique_ptr<std::istream> original_json = root_->get_source_stream();
    std::string source_path = [&] {
        if( root_->get_source_path().empty() )
        {
            return std::string{ "<unknown source file>" };
        }
        return root_->get_source_path().u8string();
    }();
    JsonIn jsin( *original_json, source_path );

    advance_jsin( &jsin, flexbuffer_root_from_storage( root_->get_storage() ), path_ );

    JsonObject jo = jsin.get_object();
    jo.allow_omitted_members();
    for( size_t skipped_member_idx : skipped_members ) {
        flexbuffers::String name = keys_[skipped_member_idx].AsString();
        if( strncmp( "//", name.c_str(), 2 ) != 0 ) {
            try {
                jo.throw_error_at( name.c_str(),
                                   string_format( "Invalid or misplaced field name \"%s\" in JSON data",
                                                  name.c_str() ) );
            } catch( const JsonError &e ) {
                debugmsg( "(json-error)\n%s", e.what() );
            }
        }
        mark_visited( skipped_member_idx );
    }
}

void FlexJsonObject::throw_error( const std::string &err ) const
{
    FlexJson::throw_error( path_, 0, err );
}

void FlexJsonObject::throw_error_at( const std::string &member, const std::string &err ) const
{
    throw_error_at( member.c_str(), err );
}

void FlexJsonObject::throw_error_at( const char *member, const std::string &err ) const
{
    cata::optional<FlexJsonValue> member_opt = get_member_opt( member );
    if( member_opt.has_value() ) {
        ( *member_opt ).throw_error( err );
    } else {
        throw_error( err );
    }
}
