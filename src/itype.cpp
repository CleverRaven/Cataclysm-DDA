#include "itype.h"

#include <cstdlib>
#include <utility>

#include "character.h"
#include "debug.h"
#include "item.h"
#include "make_static.h"
#include "recipe.h"
#include "ret_val.h"
#include "translations.h"

struct tripoint;

std::string gunmod_location::name() const
{
    // Yes, currently the name is just the translated id.
    return _( _id );
}

std::string islot_book::recipe_with_description_t::name() const
{
    if( optional_name ) {
        return optional_name->translated();
    } else {
        return recipe->result_name( /*decorated=*/true );
    }
}

namespace io
{
template<>
std::string enum_to_string<condition_type>( condition_type data )
{
    switch( data ) {
        case condition_type::FLAG:
            return "FLAG";
        case condition_type::COMPONENT_ID:
            return "COMPONENT_ID";
        case condition_type::COMPONENT_ID_SUBSTRING:
            return "COMPONENT_ID_SUBSTRING";
        case condition_type::VAR:
            return "VAR";
        case condition_type::SNIPPET_ID:
            return "SNIPPET_ID";
        case condition_type::num_condition_types:
            break;
    }
    cata_fatal( "Invalid condition_type" );
}

template<>
std::string enum_to_string<itype_variant_kind>( itype_variant_kind data )
{
    switch( data ) {
        case itype_variant_kind::gun:
            return "gun";
        case itype_variant_kind::generic:
            return "generic";
        case itype_variant_kind::last:
            debugmsg( "Invalid variant type!" );
            return "";
    }
    return "";
}
} // namespace io

std::string itype::get_item_type_string() const
{
    if( tool ) {
        return "TOOL";
    } else if( comestible ) {
        return "FOOD";
    } else if( armor ) {
        return "ARMOR";
    } else if( book ) {
        return "BOOK";
    } else if( gun ) {
        return "GUN";
    } else if( bionic ) {
        return "BIONIC";
    } else if( ammo ) {
        return "AMMO";
    }
    return "misc";
}

std::string itype::nname( unsigned int quantity ) const
{
    // Always use singular form for liquids.
    // (Maybe gases too?  There are no gases at the moment)
    if( phase == phase_id::LIQUID ) {
        quantity = 1;
    }
    return name.translated( quantity );
}

int itype::charges_default() const
{
    if( tool ) {
        return tool->def_charges;
    } else if( comestible ) {
        return comestible->def_charges;
    } else if( ammo ) {
        return ammo->def_charges;
    }
    return count_by_charges() ? 1 : 0;
}

int itype::charges_to_use() const
{
    if( tool ) {
        return tool->charges_per_use;
    } else if( comestible ) {
        return 1;
    }
    return 0;
}

int itype::charges_per_volume( const units::volume &vol ) const
{
    if( volume == 0_ml ) {
        // TODO: items should not have 0 volume at all!
        return item::INFINITE_CHARGES;
    }
    return ( count_by_charges() ? stack_size : 1 ) * vol / volume;
}

// Members of iuse struct, which is slowly morphing into a class.
bool itype::has_use() const
{
    return !use_methods.empty();
}

bool itype::has_flag( const flag_id &flag ) const
{
    return item_tags.count( flag );
}

const itype::FlagsSetType &itype::get_flags() const
{
    return item_tags;
}

bool itype::can_use( const std::string &iuse_name ) const
{
    return get_use( iuse_name ) != nullptr;
}

const use_function *itype::get_use( const std::string &iuse_name ) const
{
    const auto iter = use_methods.find( iuse_name );
    return iter != use_methods.end() ? &iter->second : nullptr;
}

int itype::tick( Character &p, item &it, const tripoint &pos ) const
{
    // Note: can go higher than current charge count
    // Maybe should move charge decrementing here?
    int charges_to_use = 0;
    for( const auto &method : use_methods ) {
        const int val = method.second.call( p, it, true, pos ).value_or( 0 );
        if( charges_to_use < 0 || val < 0 ) {
            charges_to_use = -1;
        } else {
            charges_to_use += val;
        }
    }

    return charges_to_use;
}

cata::optional<int> itype::invoke( Character &p, item &it, const tripoint &pos ) const
{
    if( !has_use() ) {
        return 0;
    }
    if( use_methods.find( "transform" ) != use_methods.end() ) {
        return  invoke( p, it, pos, "transform" );
    } else {
        return invoke( p, it, pos, use_methods.begin()->first );
    }
}

cata::optional<int> itype::invoke( Character &p, item &it, const tripoint &pos,
                                   const std::string &iuse_name ) const
{
    const use_function *use = get_use( iuse_name );
    if( use == nullptr ) {
        debugmsg( "Tried to invoke %s on a %s, which doesn't have this use_function",
                  iuse_name, nname( 1 ) );
        return 0;
    }

    p.invalidate_weight_carried_cache();
    const auto ret = use->can_call( p, it, false, pos );

    if( !ret.success() ) {
        p.add_msg_if_player( m_info, ret.str() );
        return 0;
    }

    return use->call( p, it, false, pos );
}

std::string gun_type_type::name() const
{
    const itype_id gun_type_as_id( name_ );
    return gun_type_as_id.is_valid()
           ? gun_type_as_id->nname( 1 )
           : pgettext( "gun_type_type", name_.c_str() );
}

bool itype::can_have_charges() const
{
    if( count_by_charges() ) {
        return true;
    }
    if( tool && tool->max_charges > 0 ) {
        return true;
    }
    if( gun && gun->clip > 0 ) {
        return true;
    }
    if( has_flag( STATIC( flag_id( "CAN_HAVE_CHARGES" ) ) ) ) {
        return true;
    }
    return false;
}

bool itype::is_basic_component() const
{
    for( const auto &mat : materials ) {
        if( mat.first->salvaged_into() && *mat.first->salvaged_into() == get_id() ) {
            return true;
        }
    }
    return false;
}

int islot_armor::avg_env_resist() const
{
    int acc = 0;
    for( const armor_portion_data &datum : data ) {
        acc += datum.env_resist;
    }
    if( data.empty() ) {
        return 0;
    }
    return acc / data.size();
}

int islot_armor::avg_env_resist_w_filter() const
{
    int acc = 0;
    for( const armor_portion_data &datum : data ) {
        acc += datum.env_resist_w_filter;
    }
    if( data.empty() ) {
        return 0;
    }
    return acc / data.size();
}

float islot_armor::avg_thickness() const
{
    float acc = 0;
    for( const armor_portion_data &datum : data ) {
        acc += datum.avg_thickness;
    }
    if( data.empty() ) {
        return 0;
    }
    return acc / data.size();
}

int armor_portion_data::max_coverage( bodypart_str_id bp ) const
{
    if( bp->sub_parts.empty() ) {
        // if the location doesn't have subparts then its always 100 coverage
        return 100;
    }

    int primary_max_coverage = 0;
    int secondary_max_coverage = 0;
    for( const sub_bodypart_str_id &sbp : sub_coverage ) {
        if( bp.id() == sbp->parent.id() && !sbp->secondary ) {
            // add all sublocations that share the same parent limb
            primary_max_coverage += sbp->max_coverage;
        }

        if( bp.id() == sbp->parent.id() && sbp->secondary ) {
            // add all sublocations that share the same parent limb
            secondary_max_coverage += sbp->max_coverage;
        }
    }

    // return the max of primary or hanging sublocations (this only matters for hanging items on chest)
    return std::max( primary_max_coverage, secondary_max_coverage );
}

bool armor_portion_data::should_consolidate( const armor_portion_data &l,
        const armor_portion_data &r )
{
    //check if the following are equal:
    return l.encumber == r.encumber &&
           l.max_encumber == r.max_encumber &&
           l.volume_encumber_modifier == r.volume_encumber_modifier &&
           l.coverage == r.coverage &&
           l.cover_melee == r.cover_melee &&
           l.cover_ranged == r.cover_ranged &&
           l.cover_vitals == r.cover_vitals &&
           l.env_resist == r.env_resist &&
           l.breathability == r.breathability &&
           l.rigid == r.rigid &&
           l.comfortable == r.comfortable &&
           l.rigid_layer_only == r.rigid_layer_only &&
           l.materials == r.materials &&
           l.layers == r.layers;
}

int armor_portion_data::calc_encumbrance( units::mass weight, bodypart_id bp ) const
{
    // this function takes some fixed points for mass to encumbrance and interpolates them to get results for head encumbrance
    // TODO: Generalize this for other body parts (either with a modifier or seperated point graphs)
    // TODO: Handle distributed weight

    int encumbrance = 0;

    std::map<units::mass, int> mass_to_encumbrance = bp->encumbrance_per_weight;

    std::map<units::mass, int>::iterator itt = mass_to_encumbrance.lower_bound( weight );

    if( itt == mass_to_encumbrance.begin() || itt == mass_to_encumbrance.end() ) {
        debugmsg( "Can't find a notable point to match this with" );
        return 100;
    }

    // get the bound bellow our given weight
    --itt;

    std::map<units::mass, int>::iterator next_itt = std::next( itt );

    // between itt and next_itt need to figure out how much and scale values
    float scale = static_cast<float>( weight.value() - itt->first.value() ) / static_cast<float>
                  ( next_itt->first.value() - itt->first.value() );

    // encumbrance is scaled by range between the two values
    encumbrance = itt->second + std::roundf( static_cast<float>( next_itt->second - itt->second ) *
                  scale );

    // then add some modifiers
    int multiplier = 100;
    int additional_encumbrance = 0;
    for( const encumbrance_modifier &em : encumber_modifiers ) {
        std::tuple<encumbrance_modifier_type, int> modifier = armor_portion_data::convert_descriptor_to_val(
                    em );
        if( std::get<0>( modifier ) == encumbrance_modifier_type::FLAT ) {
            additional_encumbrance += std::get<1>( modifier );
        } else if( std::get<0>( modifier ) == encumbrance_modifier_type::MULT ) {
            multiplier += std::get<1>( modifier );
        }
    }
    // modify by multiplier
    encumbrance = std::roundf( static_cast<float>( encumbrance ) * static_cast<float>
                               ( multiplier ) / 100.0f );
    // modify by flat
    encumbrance += additional_encumbrance;

    // cap encumbrance at at least 1
    return std::max( encumbrance, 1 );
}

std::tuple<encumbrance_modifier_type, int> armor_portion_data::convert_descriptor_to_val(
    encumbrance_modifier em )
{
    // this is where the values for each of these exist
    switch( em ) {
        case encumbrance_modifier::IMBALANCED:
        case encumbrance_modifier::RESTRICTS_NECK:
            return { encumbrance_modifier_type::FLAT, 10 };
        case encumbrance_modifier::WELL_SUPPORTED:
            return { encumbrance_modifier_type::MULT, -20 };
        case encumbrance_modifier::NONE:
            return { encumbrance_modifier_type::FLAT, 0 };
        case encumbrance_modifier::last:
            break;
    }
    return { encumbrance_modifier_type::FLAT, 0 };
}

std::map<itype_id, std::set<itype_id>> islot_magazine::compatible_guns;
