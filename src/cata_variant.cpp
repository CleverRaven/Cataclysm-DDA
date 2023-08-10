#include "cata_variant.h"
#include "debug_menu.h" // IWYU pragma: keep
#include "mutation.h" // IWYU pragma: keep

template<size_t I>
static bool is_valid_impl_2( const std::string &s )
{
    constexpr cata_variant_type T = static_cast<cata_variant_type>( I );
    return cata_variant_detail::convert<T>::is_valid( s );
}

template<size_t... I>
constexpr bool is_valid_impl( const cata_variant &v, std::index_sequence<I...> )
{
    constexpr size_t num_types = static_cast<size_t>( cata_variant_type::num_types );
    constexpr std::array<bool( * )( const std::string & ), num_types> is_valid_helpers = {{
            is_valid_impl_2<I>...
        }
    };
    // No match
    return is_valid_helpers[static_cast<size_t>( v.type() )]( v.get_string() );
}

bool cata_variant::is_valid() const
{
    constexpr size_t num_types = static_cast<size_t>( cata_variant_type::num_types );
    return is_valid_impl( *this, std::make_index_sequence<num_types> {} );
}

namespace io
{

template<>
std::string enum_to_string<cata_variant_type>( cata_variant_type type )
{
    switch( type ) {
        // *INDENT-OFF*
        case cata_variant_type::void_: return "void";
        case cata_variant_type::achievement_id: return "achievement_id";
        case cata_variant_type::activity_id: return "activity_id";
        case cata_variant_type::addiction_id: return "addiction_id";
        case cata_variant_type::bionic_id: return "bionic_id";
        case cata_variant_type::body_part: return "body_part";
        case cata_variant_type::bool_: return "bool";
        case cata_variant_type::character_id: return "character_id";
        case cata_variant_type::chrono_seconds: return "chrono_seconds";
        case cata_variant_type::debug_menu_index: return "debug_menu_index";
        case cata_variant_type::efftype_id: return "efftype_id";
        case cata_variant_type::faction_id: return "faction_id";
        case cata_variant_type::field_type_id: return "field_type_id";
        case cata_variant_type::field_type_str_id: return "field_type_str_id";
        case cata_variant_type::furn_id: return "furn_id";
        case cata_variant_type::furn_str_id: return "furn_str_id";
        case cata_variant_type::flag_id: return "flag_id";
        case cata_variant_type::int_: return "int";
        case cata_variant_type::item_group_id: return "item_group_id";
        case cata_variant_type::itype_id: return "itype_id";
        case cata_variant_type::matype_id: return "matype_id";
        case cata_variant_type::mongroup_id: return "mongroup_id";
        case cata_variant_type::mtype_id: return "mtype_id";
        case cata_variant_type::move_mode_id: return "character_movemode";
        case cata_variant_type::mutagen_technique: return "mutagen_technique";
        case cata_variant_type::mutation_category_id: return "mutation_category_id";
        case cata_variant_type::nested_mapgen_id: return "nested_mapgen_id";
        case cata_variant_type::npc_template_id: return "npc_template_id";
        case cata_variant_type::oter_id: return "oter_id";
        case cata_variant_type::oter_type_str_id: return "oter_type_str_id";
        case cata_variant_type::overmap_special_id: return "overmap_special_id";
        case cata_variant_type::palette_id: return "palette_id";
        case cata_variant_type::point: return "point";
        case cata_variant_type::profession_id: return "profession_id";
        case cata_variant_type::skill_id: return "skill_id";
        case cata_variant_type::species_id: return "species_id";
        case cata_variant_type::spell_id: return "spell_id";
        case cata_variant_type::string: return "string";
        case cata_variant_type::ter_id: return "ter_id";
        case cata_variant_type::ter_furn_transform_id: return "ter_furn_transform_id";
        case cata_variant_type::ter_str_id: return "ter_str_id";
        case cata_variant_type::trait_id: return "trait_id";
        case cata_variant_type::trap_id: return "trap_id";
        case cata_variant_type::trap_str_id: return "trap_str_id";
        case cata_variant_type::tripoint: return "tripoint";
        case cata_variant_type::vgroup_id: return "vgroup_id";
        case cata_variant_type::widget_id: return "widget_id";
        case cata_variant_type::zone_type_id: return "zone_type_id";
        // *INDENT-ON*
        case cata_variant_type::num_types:
            break;
    }
    debugmsg( "unknown cata_variant_type %d", static_cast<int>( type ) );
    return "";
}

} // namespace io
