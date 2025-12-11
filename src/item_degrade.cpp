/**
* For item damage, durability, degradation, age, rot
*/

#include "item.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "body_part_set.h"
#include "bodypart.h"
#include "cached_options.h"
#include "calendar.h"
#include "color.h"
#include "coordinates.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "fault.h"
#include "fire.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "item_category.h"
#include "item_components.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_pocket.h"
#include "item_transformation.h"
#include "itype.h"
#include "map.h"
#include "material.h"
#include "messages.h"
#include "mtype.h"
#include "options.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "visitable.h"
#include "weather.h"
#include "weather_gen.h"
#include "weighted_list.h"

static const item_category_id item_category_drugs( "drugs" );
static const item_category_id item_category_food( "food" );

class Character;
class JsonObject;

namespace item_internal
{
static bool goes_bad_temp_cache = false;
static const item *goes_bad_temp_cache_for = nullptr;
static bool goes_bad_cache_fetch()
{
    return goes_bad_temp_cache;
}
static void goes_bad_cache_set( const item *i )
{
    goes_bad_temp_cache = i->goes_bad();
    goes_bad_temp_cache_for = i;
}
static void goes_bad_cache_unset()
{
    goes_bad_temp_cache = false;
    goes_bad_temp_cache_for = nullptr;
}
static bool goes_bad_cache_is_for( const item *i )
{
    return goes_bad_temp_cache_for == i;
}

struct scoped_goes_bad_cache {
    explicit scoped_goes_bad_cache( item *i ) {
        goes_bad_cache_set( i );
    }
    ~scoped_goes_bad_cache() {
        goes_bad_cache_unset();
    }
};
} // namespace item_internal

static const item *get_most_rotten_component( const item &craft )
{
    const item *most_rotten = nullptr;
    for( const item_components::type_vector_pair &tvp : craft.components ) {
        if( !tvp.second.front().goes_bad() ) {
            // they're all the same type, so this should be the same for all
            continue;
        }
        for( const item &it : tvp.second ) {
            if( !most_rotten || it.get_relative_rot() > most_rotten->get_relative_rot() ) {
                most_rotten = &it;
            }
        }
    }
    return most_rotten;
}

static time_duration get_shortest_lifespan_from_components( const item &craft )
{
    const item *shortest_lifespan_component = nullptr;
    time_duration shortest_lifespan = 0_turns;
    for( const item_components::type_vector_pair &tvp : craft.components ) {
        if( !tvp.second.front().goes_bad() ) {
            // they're all the same type, so this should be the same for all
            continue;
        }
        for( const item &it : tvp.second ) {
            time_duration lifespan = it.get_shelf_life() - it.get_rot();
            if( !shortest_lifespan_component || ( lifespan < shortest_lifespan ) ) {
                shortest_lifespan_component = &it;
                shortest_lifespan = shortest_lifespan_component->get_shelf_life() -
                                    shortest_lifespan_component->get_rot();
            }
        }
    }
    return shortest_lifespan;
}

static bool shelf_life_less_than_each_component( const item &craft )
{
    for( const item_components::type_vector_pair &tvp : craft.components ) {
        if( !tvp.second.front().goes_bad() || !tvp.second.front().is_comestible() ) {
            // they're all the same type, so this should be the same for all
            continue;
        }
        for( const item &it : tvp.second ) {
            if( it.get_shelf_life() < craft.get_shelf_life() ) {
                return false;
            }
        }
    }
    return true;
}

// There are two ways rot is inherited:
// 1) Inherit the remaining lifespan of the component with the lowest remaining lifespan
// 2) Inherit the relative rot of the component with the highest relative rot
//
// Method 1 is used when the result of the recipe has a lower maximum shelf life than all of
// its component's maximum shelf lives. Relative rot is not good to inherit in this case because
// it can make an extremely short resultant remaining lifespan on the product.
//
// Method 2 is used when any component has a longer maximum shelf life than the result does.
// Inheriting the lowest remaining lifespan can not be used in this case because it would break
// food preservation recipes.
void item::inherit_rot_from_components( item &it )
{
    if( shelf_life_less_than_each_component( it ) ) {
        const time_duration shortest_lifespan = get_shortest_lifespan_from_components( it );
        if( shortest_lifespan > 0_turns && shortest_lifespan < it.get_shelf_life() ) {
            it.set_rot( it.get_shelf_life() - shortest_lifespan );
            return;
        }
        // Fallthrough: shortest_lifespan <= 0_turns (all components are rotten)
    }

    const item *most_rotten = get_most_rotten_component( it );
    if( most_rotten ) {
        it.set_relative_rot( most_rotten->get_relative_rot() );
    }
}

int item::damage() const
{
    return damage_;
}

int item::degradation() const
{
    int ret = degradation_;
    for( fault_id f : faults ) {
        ret += f.obj().degradation_mod();
    }
    return ret;
}

void item::rand_degradation()
{
    if( type->degrade_increments() == 0 ) {
        return; // likely count_by_charges
    }
    set_degradation( rng( 0, damage() ) * 50.0f / type->degrade_increments() );
}

int item::damage_level( bool precise ) const
{
    return precise ? damage_ : type->damage_level( damage_ );
}

bool item::activation_success() const
{
    // Should be itype::damage_max_ - 1, but for some reason it's private.
    return rng( 0, 3999 ) - damage_ >= 0;
}

float item::damage_adjusted_melee_weapon_damage( float value, const damage_type_id &dt ) const
{
    if( type->count_by_charges() ) {
        return value; // count by charges items don't have partial damage
    }

    for( fault_id fault : faults ) {
        for( const std::tuple<int, float, damage_type_id> &damage_mod : fault.obj().melee_damage_mod() ) {
            const damage_type_id dmg_type_of_fault = std::get<2>( damage_mod );
            if( dt == dmg_type_of_fault ) {
                const int damage_added = std::get<0>( damage_mod );
                const float damage_mult = std::get<1>( damage_mod );
                value = ( value + damage_added ) * damage_mult;
            }
        }
    }

    return std::max( 0.0f, value * ( 1.0f - 0.1f * std::max( 0, damage_level() - 1 ) ) );
}

float item::damage_adjusted_gun_damage( float value ) const
{
    if( type->count_by_charges() ) {
        return value; // count by charges items don't have partial damage
    }
    return value - 2 * std::max( 0, damage_level() - 1 );
}

float item::damage_adjusted_armor_resist( float value, const damage_type_id &dmg_type ) const
{
    if( type->count_by_charges() ) {
        return value; // count by charges items don't have partial damage
    }

    for( fault_id fault : faults ) {
        for( const std::tuple<int, float, damage_type_id> &damage_mod : fault.obj().armor_mod() ) {
            const damage_type_id dmg_type_of_fault = std::get<2>( damage_mod );
            if( dmg_type == dmg_type_of_fault ) {
                const int damage_added = std::get<0>( damage_mod );
                const float damage_mult = std::get<1>( damage_mod );
                value = ( value + damage_added ) * damage_mult;
            }
        }
    }

    return std::max( 0.0f, value * ( 1.0f - std::max( 0, damage_level() - 1 ) * 0.125f ) );
}

void item::set_damage( int qty )
{
    const int target_damage = std::clamp( qty, degradation_, max_damage() );
    const int delta = target_damage - damage_;
    mod_damage( delta );
    return;
}

void item::force_set_damage( int qty )
{
    damage_ = std::clamp( qty, degradation_, max_damage() );
}

void item::set_degradation( int qty )
{
    if( type->degrade_increments() > 0 ) {
        degradation_ = std::clamp( qty, 0, max_damage() );
    } else {
        degradation_ = 0;
    }
    set_damage( damage_ );
}

std::string item::degradation_symbol() const
{
    const int inc = max_damage() / 5;
    const int dgr_lvl = degradation() / ( inc > 0 ? inc : 1 );
    std::string dgr_symbol;

    switch( dgr_lvl ) {
        case 0:
            dgr_symbol = colorize( "\u2588", c_light_green );
            break;
        case 1:
            dgr_symbol = colorize( "\u2587", c_yellow );
            break;
        case 2:
            dgr_symbol = colorize( "\u2585", c_magenta );
            break;
        case 3:
            dgr_symbol = colorize( "\u2583", c_light_red );
            break;
        default:
            dgr_symbol = colorize( "\u2581", c_red );
            break;
    }
    return type->degrade_increments() == 0 ? "" : dgr_symbol;
}

bool item::has_fault( const fault_id &fault ) const
{
    return faults.count( fault );
}

bool item::has_fault_of_type( const std::string &fault_type ) const
{
    for( const fault_id &f : faults ) {
        if( f.obj().type() == fault_type ) {
            return true;
        }
    }
    return false;
}

bool item::has_fault_flag( const std::string &searched_flag ) const
{
    for( const fault_id &fault : faults ) {
        if( fault->has_flag( searched_flag ) ) {
            return true;
        }
    }
    return false;
}

std::string item::get_fault_description( const fault_id &f_id ) const
{
    return string_format( f_id.obj().description(), type->nname( 1 ) );
}

bool item::can_have_fault( const fault_id &f_id )
{
    // f_id fault is not defined in itype
    if( type->faults.get_specific_weight( f_id ) == 0 ) {
        return false;
    }

    // f_id is blocked by some another fault
    for( const fault_id &faults_of_item : faults ) {
        for( const fault_id &blocked_fault : faults_of_item.obj().get_block_faults() ) {
            if( f_id == blocked_fault ) {
                return false;
            }
        }
    }

    return true;
}

bool item::set_fault( const fault_id &f_id, bool force, bool message )
{
    if( !force && !can_have_fault( f_id ) ) {
        return false;
    }

    // if f_id fault blocks fault A, we should remove fault A before applying fault f_id
    // can't have chipped blade if the blade is gone
    for( const fault_id &f_blocked : f_id.obj().get_block_faults() ) {
        remove_fault( f_blocked );
    }

    if( message ) {
        add_msg( m_bad, f_id.obj().message() );
    }

    faults.insert( f_id );
    return true;
}

void item::set_random_fault_of_type( const std::string &fault_type, bool force, bool message )
{
    if( force ) {
        set_fault( random_entry( faults::all_of_type( fault_type ) ), true, message );
        return;
    }

    weighted_int_list<fault_id> faults_by_type;
    for( const std::pair<fault_id, int> &f : type->faults ) {
        if( f.first.obj().type() == fault_type && can_have_fault( f.first ) ) {
            faults_by_type.add( f.first, f.second );
        }

    }

    if( !faults_by_type.empty() ) {
        set_fault( *faults_by_type.pick(), force, message );
    }

}

void item::remove_fault( const fault_id &fault_id )
{
    faults.erase( fault_id );
}

void item::remove_single_fault_of_type( const std::string &fault_type )
{
    for( const fault_id f : faults ) {
        if( f.obj().type() == fault_type ) {
            faults.erase( f );
            return;
        }
    }
}

bool item::goes_bad() const
{
    if( item_internal::goes_bad_cache_is_for( this ) ) {
        return item_internal::goes_bad_cache_fetch();
    }
    if( has_flag( flag_PROCESSING ) ) {
        return false;
    }
    if( is_corpse() ) {
        // Corpses rot only if they are made of rotting materials
        // They also only rot if they are not dormant
        if( corpse->has_flag( mon_flag_DORMANT ) ) {
            return false;
        }
        return made_of_any( materials::get_rotting() );
    }
    return is_comestible() && get_comestible()->spoils != 0_turns;
}

time_duration item::get_shelf_life() const
{
    if( goes_bad() ) {
        if( is_comestible() ) {
            return get_comestible()->spoils;
        } else if( is_corpse() ) {
            return 24_hours;
        }
    }
    return 0_turns;
}

double item::get_relative_rot() const
{
    if( goes_bad() ) {
        return rot / get_shelf_life();
    }
    return 0;
}

void item::set_relative_rot( double val )
{
    if( goes_bad() ) {
        rot = get_shelf_life() * val;
        // calc_rot uses last_temp_check (when it's not turn_zero) instead of bday.
        // this makes sure the rotting starts from now, not from bday.
        // if this item is the result of smoking or milling don't do this, we want to start from bday.
        if( !has_flag( flag_PROCESSING_RESULT ) ) {
            last_temp_check = calendar::turn;
        }
    }
}

void item::set_rot( time_duration val )
{
    rot = val;
}

void item::randomize_rot()
{
    if( is_comestible() && get_comestible()->spoils > 0_turns ) {
        const double x_input = rng_float( 0.0, 1.0 );
        const double k_rot = ( 1.0 - x_input ) / ( 1.0 + 2.0 * x_input );
        set_rot( get_shelf_life() * k_rot );
    }

    for( item_pocket *pocket : contents.get_all_contained_pockets() ) {
        if( pocket->spoil_multiplier() > 0.0f ) {
            for( item *subitem : pocket->all_items_top() ) {
                subitem->randomize_rot();
            }
        }
    }
}

int item::spoilage_sort_order() const
{
    int bottom = std::numeric_limits<int>::max();

    bool any_goes_bad = false;
    time_duration min_spoil_time = calendar::INDEFINITELY_LONG_DURATION;
    visit_items( [&]( item * const node, item * const parent ) {
        if( node && node->goes_bad() ) {
            float spoil_multiplier = 1.0f;
            if( parent ) {
                const item_pocket *const parent_pocket = parent->contained_where( *node );
                if( parent_pocket ) {
                    spoil_multiplier = parent_pocket->spoil_multiplier();
                }
            }
            if( spoil_multiplier > 0.0f ) {
                time_duration remaining_shelf_life = node->get_shelf_life() - node->rot;
                if( !any_goes_bad || min_spoil_time * spoil_multiplier > remaining_shelf_life ) {
                    any_goes_bad = true;
                    min_spoil_time = remaining_shelf_life / spoil_multiplier;
                }
            }
        }
        return VisitResponse::NEXT;
    } );
    if( any_goes_bad ) {
        return to_turns<int>( min_spoil_time );
    }

    if( get_comestible() ) {
        if( get_category_shallow().get_id() == item_category_food ) {
            return bottom - 3;
        } else if( get_category_shallow().get_id() == item_category_drugs ) {
            return bottom - 2;
        } else {
            return bottom - 1;
        }
    }
    return bottom;
}

/**
 * Food decay calculation.
 * Calculate how much food rots per hour, based on 3600 rot/hour at 65 F (18.3 C).
 * Rate of rot doubles every 16 F (~8.8888 C) increase in temperature
 * Rate of rot halves every 16 F decrease in temperature
 * Rot maxes out at 105 F
 * Rot stops below 32 F (0C) and above 145 F (63 C)
 */
float item::calc_hourly_rotpoints_at_temp( const units::temperature &temp ) const
{
    const units::temperature dropoff = units::from_fahrenheit( 38 ); // F, ~3 C
    const float max_rot_temp = 105; // F, ~41 C, Maximum rotting rate is at this temperature

    if( temp <= temperatures::freezing ) {
        return 0.f;
    } else if( temp < dropoff ) {
        // ditch our fancy equation and do a linear approach to 0 rot from 38 F (3 C) -> 32 F (0 C)
        return 600.f * std::exp2( -27.f / 16.f ) * ( units::to_fahrenheit( temp ) - units::to_fahrenheit(
                    temperatures::freezing ) );
    } else if( temp < units::from_fahrenheit( max_rot_temp ) ) {
        // Exponential progress from 38 F (3 C) to 105 F (41 C)
        return 3600.f * std::exp2( ( units::to_fahrenheit( temp ) - 65.f ) / 16.f );
    } else {
        // Constant rot from 105 F (41 C) upwards
        // This is approximately 20364.67 rot/hour
        return 3600.f * std::exp2( ( max_rot_temp - 65.f ) / 16.f );
    }
}

void item::calc_rot( units::temperature temp, const float spoil_modifier,
                     const time_duration &time_delta )
{
    // Avoid needlessly calculating already rotten things.  Corpses should
    // always rot away and food rots away at twice the shelf life.  If the food
    // is in a sealed container they won't rot away, this avoids needlessly
    // calculating their rot in that case.
    if( !is_corpse() && get_relative_rot() > 2.0 ) {
        return;
    }

    if( has_own_flag( flag_FROZEN ) ) {
        return;
    }

    // rot modifier
    float factor = spoil_modifier;
    if( is_corpse() && has_flag( flag_FIELD_DRESS ) ) {
        factor *= 0.75;
    }
    if( has_own_flag( flag_MUSHY ) ) {
        factor *= 3.0;
    }
    // Food irradiation can quadruple the shelf life.
    // "Shelf life of ground beef patties treated by gamma radiation":
    // > Beef patties treated at 1.0 and 3.0 kGy [spoiled] by day 14 and 21, ...
    // > patties treated at 5.0 kGy did not spoil until 42 days.
    // > The nonirradiated control samples for both batches of ground beef spoiled within 7 days
    // We get 0.5, 0.33, and 0.167. 0.25 seems reasonable for irradiation
    if( has_own_flag( flag_IRRADIATED ) ) {
        factor *= 0.25;
    }

    if( has_own_flag( flag_COLD ) ) {
        temp = std::min( temperatures::fridge, temp );
    }

    rot += factor * time_delta / 1_seconds * calc_hourly_rotpoints_at_temp( temp ) * 1_turns /
           ( 1_hours / 1_seconds );
}

void item::calc_rot_while_processing( time_duration processing_duration )
{
    if( !has_own_flag( flag_PROCESSING ) ) {
        debugmsg( "calc_rot_while_processing called on non smoking item: %s", tname() );
        return;
    }

    // Apply no rot or temperature while smoking
    last_temp_check += processing_duration;
}

bool item::process_decay_in_air( map &here, Character *carrier, const tripoint_bub_ms &pos,
                                 int max_air_exposure_hours,
                                 time_duration time_delta )
{
    if( !has_own_flag( flag_FROZEN ) ) {
        double environment_multiplier = here.is_outside( pos ) ? 2.0 : 1.0;
        time_duration new_air_exposure = time_duration::from_seconds( item_counter ) + time_delta *
                                         rng_normal( 0.9, 1.1 ) * environment_multiplier;
        if( new_air_exposure >= time_duration::from_hours( max_air_exposure_hours ) ) {
            type->transform_into.value().transform( carrier, *this, true );
            return true;
        }
        item_counter = to_seconds<int>( new_air_exposure );
    }
    return false;
}

#if defined(_MSC_VER)
// Deal with MSVC compiler bug (#17791, #17958)
#pragma optimize( "", off )
#endif

bool item::damage_type_can_damage_items( const damage_type_id &dmg_type ) const
{
    return dmg_type->physical;
}

template<typename bodypart_target>
float item::resist( const damage_type_id &dmg_type, const bool to_self,
                    const bodypart_target &bp,
                    const int resist_value ) const
{
    if( is_null() ) {
        return 0.0f;
    }

    if( dmg_type.is_null() ) {
        return 0.0f;
    }

    if( !dmg_type.is_valid() ) {
        debugmsg( "Invalid damage type: %d", dmg_type.c_str() );
        return 0.0f;
    }

    // Implicit item damage immunity for damage types listed prior to bash
    // Acid/fire immunity would be handled in _environmental_resist, but there are more
    // dmg types that do not affect items such as PURE/COLD etc..
    if( to_self && !damage_type_can_damage_items( dmg_type ) ) {
        return std::numeric_limits<float>::max();
    }

    const bool bp_null = bp == bodypart_target();
    const std::vector<const part_material *> &armor_mats = armor_made_of( bp );
    const float avg_thickness = bp_null ? get_thickness() : get_thickness( bp );
    return _resist( dmg_type, to_self, resist_value, bp_null, armor_mats, avg_thickness );
}
template float item::resist<bodypart_id>( const damage_type_id &dmg_type,
        const bool to_self, const bodypart_id &bp, const int resist_value ) const;
template float item::resist<sub_bodypart_id>( const damage_type_id &dmg_type,
        const bool to_self, const sub_bodypart_id &bp, const int resist_value ) const;

float item::_resist( const damage_type_id &dmg_type, bool to_self, int resist_value,
                     const bool bp_null,
                     const std::vector<const part_material *> &armor_mats,
                     const float avg_thickness ) const
{
    if( dmg_type->env ) {
        return _environmental_resist( dmg_type, to_self, resist_value, bp_null, armor_mats );
    }

    std::optional<std::pair<damage_type_id, float>> derived;
    if( !dmg_type->derived_from.first.is_null() ) {
        derived = dmg_type->derived_from;
    }

    float resist = 0.0f;
    float mod = get_clothing_mod_val_for_damage_type( dmg_type );

    const float damage_scale = damage_adjusted_armor_resist( 1.0f, dmg_type );

    if( !bp_null ) {
        // If we have armour portion materials for this body part, use that instead
        if( !armor_mats.empty() ) {
            for( const part_material *m : armor_mats ) {
                // only count the material if it's hit

                // if roll is -1 each material is rolled at this point individually
                int internal_roll;
                resist_value < 0 ? internal_roll = rng( 0, 99 ) : internal_roll = resist_value;
                if( internal_roll < m->cover ) {
                    float tmp_add = 0.f;
                    if( derived.has_value() && !m->id->has_dedicated_resist( dmg_type ) ) {
                        tmp_add = m->id->resist( derived->first ) * m->thickness * derived->second;
                    } else {
                        tmp_add = m->id->resist( dmg_type ) * m->thickness;
                    }
                    resist += tmp_add;
                }
            }
            return ( resist + mod ) * damage_scale;
        }
    }

    // base resistance
    const std::map<material_id, int> mats = made_of();
    if( !mats.empty() ) {
        const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
        for( const auto &m : mats ) {
            float tmp_add = 0.f;
            if( derived.has_value() && !m.first->has_dedicated_resist( dmg_type ) ) {
                tmp_add = m.first->resist( derived->first ) * m.second * derived->second;
            } else {
                tmp_add = m.first->resist( dmg_type ) * m.second;
            }
            resist += tmp_add;
        }
        // Average based portion of materials
        resist /= total;
    }

    return ( resist * avg_thickness + mod ) * damage_scale;
}

float item::_environmental_resist( const damage_type_id &dmg_type, const bool to_self,
                                   int base_env_resist,
                                   const bool bp_null,
                                   const std::vector<const part_material *> &armor_mats ) const
{
    if( to_self ) {
        // Currently no items are damaged by acid, and fire is handled elsewhere
        return std::numeric_limits<float>::max();
    }

    std::optional<std::pair<damage_type_id, float>> derived;
    if( !dmg_type->derived_from.first.is_null() ) {
        derived = dmg_type->derived_from;
    }

    float resist = 0.0f;
    float mod = get_clothing_mod_val_for_damage_type( dmg_type );

    if( !bp_null ) {
        // If we have armour portion materials for this body part, use that instead
        if( !armor_mats.empty() ) {
            for( const part_material *m : armor_mats ) {
                float tmp_add = 0.f;
                if( derived.has_value() && !m->id->has_dedicated_resist( dmg_type ) ) {
                    tmp_add = m->id->resist( derived->first ) * m->cover * 0.01f * derived->second;
                } else {
                    tmp_add = m->id->resist( dmg_type ) * m->cover * 0.01f;
                }
                resist += tmp_add;
            }
            const int env = get_env_resist( base_env_resist );
            if( env < 10 ) {
                resist *= env / 10.0f;
            }
        }
        return resist + mod;
    }

    const std::map<material_id, int> mats = made_of();
    if( !mats.empty() ) {
        const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
        // Not sure why cut and bash get an armor thickness bonus but acid/fire doesn't,
        // but such is the way of the code.
        for( const auto &m : mats ) {
            float tmp_add = 0.f;
            if( derived.has_value() && !m.first->has_dedicated_resist( dmg_type ) ) {
                tmp_add = m.first->resist( derived->first ) * m.second * derived->second;
            } else {
                tmp_add = m.first->resist( dmg_type ) * m.second;
            }
            resist += tmp_add;
        }
        // Average based portion of materials
        resist /= total;
    }

    const int env = get_env_resist( base_env_resist );
    if( env < 10 ) {
        // Low env protection means it doesn't prevent acid seeping in.
        resist *= env / 10.0f;
    }

    return resist + mod;
}

#if defined(_MSC_VER)
#pragma optimize( "", on )
#endif

int item::max_damage() const
{
    return type->damage_max();
}

float item::get_relative_health() const
{
    const int max_dmg = max_damage();
    if( max_dmg == 0 ) { // count_by_charges items
        return 1.0f;
    }
    return 1.0f - static_cast<float>( damage() ) / max_dmg;
}

static int get_degrade_amount( const item &it, int dmgNow_, int dmgPrev )
{
    const int max_dmg = it.max_damage();
    const int degrade_increments = it.type->degrade_increments();
    if( max_dmg == 0 || degrade_increments == 0 ) {
        return 0; // count by charges
    }
    const int facNow_ = dmgNow_ == 0 ? -1 : ( dmgNow_ - 1 ) * 5 / max_dmg;
    const int facPrev = dmgPrev == 0 ? -1 : ( dmgPrev - 1 ) * 5 / max_dmg;

    return std::max( facNow_ - facPrev, 0 ) * max_dmg / degrade_increments;
}

bool item::mod_damage( int qty )
{
    if( has_flag( flag_UNBREAKABLE ) ) {
        return false;
    }
    if( count_by_charges() ) {
        charges -= std::min( type->stack_size * qty / itype::damage_scale, charges );
        return charges == 0; // return destroy = true if no charges
    } else {
        const int dmg_before = damage_;
        const bool destroy = ( damage_ + qty ) > max_damage();
        force_set_damage( damage_ + qty );

        if( qty > 0 && !destroy ) { // apply automatic degradation
            set_degradation( degradation_ + get_degrade_amount( *this, damage_, dmg_before ) );
        }

        // TODO: think about better way to telling the game what faults should be applied when
        if( qty > 0 ) {
            for( int i = 0; i <= qty; i += itype::damage_scale ) {
                set_random_fault_of_type( "mechanical_damage" );
            }
        }

        return destroy;
    }
}

bool item::inc_damage()
{
    return mod_damage( itype::damage_scale );
}

int item::repairable_levels() const
{
    const int levels = type->damage_level( damage_ ) - type->damage_level( degradation_ );

    return levels > 0
           ? levels                    // full integer levels of damage
           : damage() > degradation(); // partial level of damage can still be repaired
}

item::armor_status item::damage_armor_durability( damage_unit &du, damage_unit &premitigated,
        const bodypart_id &bp,
        double enchant_multiplier )
{
    //Energy shields aren't damaged by attacks but do get their health variable reduced.  They are also only
    //damaged by the damage types they actually protect against.
    if( has_var( "ENERGY_SHIELD_HP" ) && resist( du.type, false, bp ) > 0.0f ) {
        double shield_hp = get_var( "ENERGY_SHIELD_HP", 0.0 );
        shield_hp -= premitigated.amount;
        set_var( "ENERGY_SHIELD_HP", shield_hp );
        if( shield_hp > 0 ) {
            return armor_status::UNDAMAGED;
        } else {
            //Shields deliberately ignore the enchantment multiplier, as the health mechanic wouldn't make sense otherwise.
            mod_damage( itype::damage_scale * 6 );
            return armor_status::DESTROYED;
        }
    }

    if( has_flag( flag_UNBREAKABLE ) || enchant_multiplier <= 0.0f ) {
        return armor_status::UNDAMAGED;
    }

    // We want armor's own resistance to this type, not the resistance it grants
    double armors_own_resist = resist( du.type, true, bp );
    if( armors_own_resist > 1000.0 ) {
        // This is some weird type that doesn't damage armors
        return armor_status::UNDAMAGED;
    }
    /*
    * Armor damage chance is calculated using the logistics function.
    *  No matter the damage dealt, an armor piece has at most 80% chance of being damaged.
    *  Chance of damage is 40% when the premitigation damage is equal to armor*1.333
    *  Sturdy items will not take damage if premitigation damage isn't higher than armor*1.333.
    *
    *  Non fragile items are considered to always have at least 10 points of armor, this is to prevent
    *  regular clothes from exploding into ribbons whenever you get punched.
    *
    *  Fragile items have a smaller growth rate for damage chance (k) and are more likely to be damaged
    *  even at low damage absorbed.
    */
    armors_own_resist = has_flag( flag_FRAGILE ) ? armors_own_resist * 0.6666 : std::max( 10.0,
                        armors_own_resist * 1.3333 );
    double k = has_flag( flag_FRAGILE ) ? 0.1 : 1.0;
    double damage_chance = 0.8 / ( 1.0 + std::exp( -k * ( premitigated.amount - armors_own_resist ) ) )
                           * enchant_multiplier;

    // Scale chance of article taking damage based on the number of parts it covers.
    // This represents large articles being able to take more punishment
    // before becoming ineffective or being destroyed
    if( !has_flag( flag_FRAGILE ) ) {
        damage_chance = damage_chance / get_covered_body_parts().count();
    }
    add_msg_debug( debugmode::DF_CREATURE, "Chance to degrade armor %s: %.2f%%",
                   tname(), damage_chance * 100.0 );
    if( has_flag( flag_STURDY ) && premitigated.amount < armors_own_resist ) {
        return armor_status::UNDAMAGED;
    } else if( x_in_y( damage_chance, 1.0 ) ) {
        return mod_damage( itype::damage_scale * enchant_multiplier ) ? armor_status::DESTROYED :
               armor_status::DAMAGED;
    }
    return armor_status::UNDAMAGED;
}


item::armor_status item::damage_armor_transforms( damage_unit &du, double enchant_multiplier ) const
{
    if( enchant_multiplier <= 0.0f ) {
        return armor_status::UNDAMAGED;
    }

    // We want armor's own resistance to this type, not the resistance it grants
    const float armors_own_resist = resist( du.type, true );

    // to avoid dumb chip damage plates shouldn't ever transform if they take less than
    // 20% of their protection in damage
    if( du.amount / armors_own_resist < 0.2f ) {
        return armor_status::UNDAMAGED;
    }

    // plates are rated to survive 3 shots at the caliber they protect
    // linearly scale off the scale value to find the chance it breaks
    float break_chance = 33.3f * ( du.amount / armors_own_resist ) * enchant_multiplier;

    float roll_to_break = rng_float( 0.0, 100.0 );

    if( roll_to_break < break_chance ) {
        //the plate is broken
        return armor_status::TRANSFORMED;
    }
    return armor_status::UNDAMAGED;
}

std::string item::damage_indicator() const
{
    return cata::options::damage_indicators[damage_level()];
}

std::string item::durability_indicator( bool include_intact ) const
{
    const std::string item_health_option = get_option<std::string>( "ITEM_HEALTH" );
    const bool show_bars_only = item_health_option == "bars";
    const bool show_description_only = item_health_option == "descriptions";
    const bool show_both = item_health_option == "both";
    const bool show_bars = show_both || show_bars_only;
    const bool show_description = show_both || show_description_only;
    std::string bars;
    std::string description;
    if( show_bars ) {
        bars = damage_indicator() + degradation_symbol() + "\u00A0";
    }
    if( damage() < 0 ) {
        if( show_description ) {
            if( is_gun() ) {
                description = pgettext( "damage adjective", "accurized " );
            } else {
                description = pgettext( "damage adjective", "reinforced " );
            }
        }
        if( show_bars_only ) {
            return bars;
        }
        if( show_description_only ) {
            return description;
        }
        return bars + description;
    }
    if( has_flag( flag_CORPSE ) ) {
        if( damage() > 0 ) {
            switch( damage_level() ) {
                case 1:
                    return pgettext( "damage adjective", "bruised " );
                case 2:
                    return pgettext( "damage adjective", "damaged " );
                case 3:
                    return pgettext( "damage adjective", "mangled " );
                default:
                    return pgettext( "damage adjective", "pulped " );
            }
        }
    }
    if( show_bars_only ) {
        return bars;
    }
    description = string_format( "%s ", get_base_material().dmg_adj( damage_level() ) );
    if( description == " " ) {
        if( include_intact ) {
            description = _( "fully intact " );
        } else {
            description = "";
        }
    }
    if( show_description_only ) {
        return description;
    }
    return bars + description;
}

const std::set<itype_id> &item::repaired_with() const
{
    static std::set<itype_id> no_repair;
    static std::set<itype_id> tools;
    tools.clear();

    std::copy_if( type->repair.begin(), type->repair.end(), std::inserter( tools, tools.begin() ),
    []( const itype_id & i ) {
        return !item_is_blacklisted( i );
    } );

    return has_flag( flag_NO_REPAIR ) ? no_repair : tools;
}

bool item::can_repair_with( const material_id &mat_ident ) const
{
    auto mat = type->repairs_with.find( mat_ident );
    return mat != type->repairs_with.end();
}

bool item::is_broken() const
{
    return has_flag( flag_ITEM_BROKEN ) || has_fault_flag( std::string( "ITEM_BROKEN" ) );
}

bool item::is_broken_on_active() const
{
    return has_flag( flag_ITEM_BROKEN ) ||
           has_fault_flag( std::string( "ITEM_BROKEN" ) ) ||
           ( wetness && has_flag( flag_WATER_BREAK_ACTIVE ) );
}

std::set<fault_id> item::faults_potential() const
{
    std::set<fault_id> res;
    for( const std::pair<fault_id, int> &fault_pair : type->faults ) {
        res.insert( fault_pair.first );
    }
    return res;
}

std::set<fault_id> item::faults_potential_of_type( const std::string &fault_type ) const
{
    std::set<fault_id> res;
    for( const std::pair<fault_id, int> &some_fault : type->faults ) {
        if( some_fault.first->type() == fault_type ) {
            res.emplace( some_fault.first );
        }
    }
    return res;
}

float item::simulate_burn( fire_data &frd ) const
{
    const std::map<material_id, int> &mats = made_of();
    float smoke_added = 0.0f;
    float time_added = 0.0f;
    float burn_added = 0.0f;
    const units::volume vol = base_volume();
    const int effective_intensity = frd.contained ? 3 : frd.fire_intensity;
    for( const auto &m : mats ) {
        const mat_burn_data &bd = m.first->burn_data( effective_intensity );
        if( bd.immune ) {
            // Made to protect from fire
            return 0.0f;
        }

        // If fire is contained, burn rate is independent of volume
        if( frd.contained || bd.volume_per_turn == 0_ml ) {
            time_added += bd.fuel * m.second;
            smoke_added += bd.smoke * m.second;
            burn_added += bd.burn * m.second;
        } else {
            double volume_burn_rate = to_liter( bd.volume_per_turn ) / to_liter( vol );
            time_added += bd.fuel * volume_burn_rate * m.second;
            smoke_added += bd.smoke * volume_burn_rate * m.second;
            burn_added += bd.burn * volume_burn_rate * m.second;
        }
    }
    const int mat_total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;

    // Liquids that don't burn well smother fire well instead
    if( made_of( phase_id::LIQUID ) && time_added < 200 ) {
        time_added -= rng( 400.0 * to_liter( vol ), 1200.0 * to_liter( vol ) );
    } else if( mats.size() > 1 ) {
        // Average the materials
        time_added /= mat_total;
        smoke_added /= mat_total;
        burn_added /= mat_total;
    } else if( mats.empty() ) {
        // Non-liquid items with no specified materials will burn at moderate speed
        burn_added = 1;
    }

    // Fire will depend on amount of fuel if stackable
    if( count_by_charges() ) {
        int stack_burnt = std::max( 1, count() / type->stack_size );
        time_added *= stack_burnt;
        smoke_added *= stack_burnt;
        burn_added *= stack_burnt;
    }

    frd.fuel_produced += time_added;
    frd.smoke_produced += smoke_added;
    return burn_added;
}

bool item::burn( fire_data &frd )
{
    if( has_flag( flag_UNBREAKABLE ) ) {
        return false;
    }
    float burn_added = simulate_burn( frd );

    if( burn_added <= 0 ) {
        return false;
    }

    if( count_by_charges() ) {
        if( type->volume == 0_ml ) {
            charges = 0;
        } else {
            charges -= roll_remainder( burn_added * 250_ml * type->stack_size /
                                       ( 3.0 * type->volume ) );
        }

        return charges <= 0;
    }

    if( is_corpse() ) {
        const mtype *mt = get_mtype();
        if( active && mt != nullptr && burnt + burn_added > mt->hp &&
            !mt->burn_into.is_null() && mt->burn_into.is_valid() ) {
            corpse = &get_mtype()->burn_into.obj();
            // Delay rezing
            set_age( 0_turns );
            burnt = 0;
            return false;
        }
    } else if( has_temperature() ) {
        heat_up();
    }

    contents.heat_up();

    burnt += roll_remainder( burn_added );

    const int vol = base_volume() / 250_ml;
    return burnt >= vol * 3;
}

bool item::flammable( int threshold ) const
{
    const std::map<material_id, int> &mats = made_of();
    if( mats.empty() ) {
        // Don't know how to burn down something made of nothing.
        return false;
    }

    int flammability = 0;
    units::volume volume_per_turn = 0_ml;
    for( const std::pair<const material_id, int> &m : mats ) {
        const mat_burn_data &bd = m.first->burn_data( 1 );
        if( bd.immune ) {
            // Made to protect from fire
            return false;
        }

        flammability += bd.fuel * m.second;
        volume_per_turn += bd.volume_per_turn * m.second;
    }
    const int total = type->mat_portion_total == 0 ? 1 : type->mat_portion_total;
    flammability /= total;
    volume_per_turn /= total;

    if( threshold == 0 || flammability <= 0 ) {
        return flammability > 0;
    }

    units::volume vol = base_volume();
    if( volume_per_turn > 0_ml && volume_per_turn < vol ) {
        flammability = flammability * volume_per_turn / vol;
    } else {
        // If it burns well, it provides a bonus here
        flammability = flammability * vol / 250_ml;
    }

    return flammability > threshold;
}

bool item::will_explode_in_fire() const
{
    if( type->explode_in_fire ) {
        return true;
    }

    if( type->ammo && ( type->ammo->special_cookoff || type->ammo->cookoff ) ) {
        return true;
    }

    return false;
}

bool item::has_rotten_away() const
{
    if( is_corpse() && !can_revive() ) {
        return get_rot() > 10_days;
    } else {
        return get_relative_rot() > 2.0;
    }
}

void item::apply_freezerburn()
{
    if( !has_flag( flag_FREEZERBURN ) ) {
        return;
    }
    set_flag( flag_MUSHY );
}

bool item::process_temperature_rot( float insulation, const tripoint_bub_ms &pos, map &here,
                                    Character *carrier, const temperature_flag flag, float spoil_modifier, bool watertight_container )
{
    const time_point now = calendar::turn;

    // if player debug menu'd the time backward it breaks stuff, just reset the
    // last_temp_check in this case
    if( now - last_temp_check < 0_turns ) {
        reset_temp_check();
        return false;
    }

    // process temperature and rot at most once every 100_turns (10 min)
    // note we're also gated by item::processing_speed
    time_duration smallest_interval = 10_minutes;
    if( now - last_temp_check < smallest_interval && units::to_joule_per_gram( specific_energy ) > 0 ) {
        return false;
    }

    units::temperature temp = get_weather().get_temperature( pos );

    switch( flag ) {
        case temperature_flag::NORMAL:
            // Just use the temperature normally
            break;
        case temperature_flag::FRIDGE:
            temp = std::min( temp, temperatures::fridge );
            break;
        case temperature_flag::FREEZER:
            temp = std::min( temp, temperatures::freezer );
            break;
        case temperature_flag::HEATER:
            temp = std::max( temp, temperatures::normal );
            break;
        case temperature_flag::ROOT_CELLAR:
            temp = units::from_celsius( get_weather().get_cur_weather_gen().base_temperature );
            break;
        default:
            debugmsg( "Temperature flag enum not valid.  Using current temperature." );
    }

    bool carried = carrier != nullptr;
    // body heat increases inventory temperature by 5 F (2.77 K) and insulation by 50%
    if( carried ) {
        insulation *= 1.5;
        temp += units::from_fahrenheit_delta( 5 );
    }

    time_point time = last_temp_check;
    item_internal::scoped_goes_bad_cache _cache( this );
    const bool process_rot = goes_bad() && spoil_modifier != 0;
    const bool decays_in_air = !watertight_container && has_flag( flag_DECAYS_IN_AIR ) &&
                               type->transform_into;
    int64_t max_air_exposure_hours = decays_in_air ? get_property_int64_t( "max_air_exposure_hours" ) :
                                     0;

    if( now - time > 1_hours ) {
        // This code is for items that were left out of reality bubble for long time

        const weather_generator &wgen = get_weather().get_cur_weather_gen();
        const unsigned int seed = g->get_seed();

        units::temperature_delta temp_mod;
        // Toilets and vending machines will try to get the heat radiation and convection during mapgen and segfault.
        if( !g->new_game && !g->swapping_dimensions ) {
            temp_mod = get_heat_radiation( pos );
            temp_mod += get_convection_temperature( pos );
            temp_mod += here.get_temperature_mod( pos );
        } else {
            temp_mod = units::from_kelvin_delta( 0 );
        }

        if( carried ) {
            temp_mod += units::from_fahrenheit_delta( 5 ); // body heat increases inventory temperature
        }

        // Process the past of this item in 1h chunks until there is less than 1h left.
        time_duration time_delta = 1_hours;

        while( now - time > 1_hours ) {
            time += time_delta;

            // Get the environment temperature
            // Use weather if above ground, use map temp if below
            units::temperature env_temperature;
            if( pos.z() >= 0 && flag != temperature_flag::ROOT_CELLAR ) {
                env_temperature = wgen.get_weather_temperature( get_map().get_abs( pos ), time, seed );
            } else {
                env_temperature = units::from_celsius( get_weather().get_cur_weather_gen().base_temperature );
            }
            env_temperature += temp_mod;

            switch( flag ) {
                case temperature_flag::NORMAL:
                    // Just use the temperature normally
                    break;
                case temperature_flag::FRIDGE:
                    env_temperature = std::min( env_temperature, temperatures::fridge );
                    break;
                case temperature_flag::FREEZER:
                    env_temperature = std::min( env_temperature, temperatures::freezer );
                    break;
                case temperature_flag::HEATER:
                    env_temperature = std::max( env_temperature, temperatures::normal );
                    break;
                case temperature_flag::ROOT_CELLAR:
                    env_temperature =  units::from_celsius( get_weather().get_cur_weather_gen().base_temperature );
                    break;
                default:
                    debugmsg( "Temperature flag enum not valid.  Using normal temperature." );
            }

            // Calculate item temperature from environment temperature
            // If the time was more than 2 d ago we do not care about item temperature.
            if( now - time < 2_days ) {
                calc_temp( env_temperature, insulation, time_delta );
            }
            last_temp_check = time;

            if( decays_in_air &&
                process_decay_in_air( here, carrier, pos, max_air_exposure_hours, time_delta ) ) {
                return false;
            }

            // Calculate item rot
            if( process_rot ) {
                calc_rot( env_temperature, spoil_modifier, time_delta );

                if( has_rotten_away() && carrier == nullptr ) {
                    // No need to track item that will be gone
                    return true;
                }
            }
        }
    }

    // Remaining <1 h from above
    // and items that are held near the player
    if( now - time > smallest_interval ) {
        calc_temp( temp, insulation, now - time );
        last_temp_check = now;

        if( decays_in_air &&
            process_decay_in_air( here, carrier, pos, max_air_exposure_hours, now - time ) ) {
            return false;
        }

        if( process_rot ) {
            calc_rot( temp, spoil_modifier, now - time );
            return has_rotten_away() && carrier == nullptr;
        }
    }

    // Just now created items will get here.
    if( units::to_joule_per_gram( specific_energy ) < 0 ) {
        set_item_temperature( temp );
    }
    return false;
}

bool item::is_filthy() const
{
    return has_flag( flag_FILTHY );
}

time_duration item::age() const
{
    return calendar::turn - birthday();
}

void item::set_age( const time_duration &age )
{
    set_birthday( time_point( calendar::turn ) - age );
}

time_point item::birthday() const
{
    return bday;
}

void item::set_birthday( const time_point &bday )
{
    this->bday = std::max( calendar::turn_zero, bday );
}

void rot_spawn_data::deserialize( const JsonObject &jo )
{
    optional( jo, false, "monster", rot_spawn_monster, mtype_id::NULL_ID() );
    optional( jo, false, "group", rot_spawn_group, mongroup_id::NULL_ID() );
    optional( jo, false, "chance", rot_spawn_chance );
    optional( jo, false, "amount", rot_spawn_monster_amount, {1, 1} );
}
