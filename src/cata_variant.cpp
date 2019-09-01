#include "cata_variant.h"

#include <unordered_map>

namespace cata_variant_detail
{

std::string to_string( cata_variant_type type )
{
    switch( type ) {
        case cata_variant_type::add_type:
            return "add_type";
        case cata_variant_type::bionic_id:
            return "bionic_id";
        case cata_variant_type::body_part:
            return "body_part";
        case cata_variant_type::bool_:
            return "bool";
        case cata_variant_type::character_id:
            return "character_id";
        case cata_variant_type::efftype_id:
            return "efftype_id";
        case cata_variant_type::int_:
            return "int";
        case cata_variant_type::itype_id:
            return "itype_id";
        case cata_variant_type::matype_id:
            return "matype_id";
        case cata_variant_type::mtype_id:
            return "mtype_id";
        case cata_variant_type::mutagen_technique:
            return "mutagen_technique";
        case cata_variant_type::mutation_category_id:
            return "mutation_category_id";
        case cata_variant_type::oter_id:
            return "oter_id";
        case cata_variant_type::skill_id:
            return "skill_id";
        case cata_variant_type::string:
            return "string";
        case cata_variant_type::trait_id:
            return "trait_id";
        case cata_variant_type::trap_str_id:
            return "trap_str_id";
        case cata_variant_type::num_types:
            break;
    };
    debugmsg( "unknown cata_variant_type %d", static_cast<int>( type ) );
    return "";
}

static std::unordered_map<std::string, cata_variant_type> create_type_lookup_map()
{
    std::unordered_map<std::string, cata_variant_type> result;
    int num_types = static_cast<int>( cata_variant_type::num_types );
    for( int i = 0; i < num_types; ++i ) {
        cata_variant_type type = static_cast<cata_variant_type>( i );
        std::string type_as_string = to_string( type );
        bool inserted = result.insert( {type_as_string, type} ).second;
        if( !inserted ) {
            debugmsg( "Duplicate variant type name %s", type_as_string );
        }
    }
    return result;
}

cata_variant_type from_string( const std::string &s )
{
    static const std::unordered_map<std::string, cata_variant_type> type_lookup_map =
        create_type_lookup_map();
    auto it = type_lookup_map.find( s );
    if( it == type_lookup_map.end() ) {
        debugmsg( "Unexpected cata_variant_type name %s", s );
        assert( false );
    }
    return it->second;
}

} // namespace cata_variant_detail
