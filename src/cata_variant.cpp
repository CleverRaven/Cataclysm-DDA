#include "cata_variant.h"

#include <unordered_map>

namespace io
{

template<>
std::string enum_to_string<cata_variant_type>( cata_variant_type type )
{
    switch( type ) {
        // *INDENT-OFF*
        case cata_variant_type::void_: return "void";
        case cata_variant_type::add_type: return "add_type";
        case cata_variant_type::bionic_id: return "bionic_id";
        case cata_variant_type::body_part: return "body_part";
        case cata_variant_type::bool_: return "bool";
        case cata_variant_type::character_id: return "character_id";
        case cata_variant_type::efftype_id: return "efftype_id";
        case cata_variant_type::hp_part: return "hp_part";
        case cata_variant_type::int_: return "int";
        case cata_variant_type::itype_id: return "itype_id";
        case cata_variant_type::matype_id: return "matype_id";
        case cata_variant_type::mtype_id: return "mtype_id";
        case cata_variant_type::mutagen_technique: return "mutagen_technique";
        case cata_variant_type::mutation_category_id: return "mutation_category_id";
        case cata_variant_type::oter_id: return "oter_id";
        case cata_variant_type::skill_id: return "skill_id";
        case cata_variant_type::string: return "string";
        case cata_variant_type::trait_id: return "trait_id";
        case cata_variant_type::trap_str_id: return "trap_str_id";
        // *INDENT-ON*
        case cata_variant_type::num_types:
            break;
    };
    debugmsg( "unknown cata_variant_type %d", static_cast<int>( type ) );
    return "";
}

} // namespace io
