#include "item.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <climits>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iterator>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "avatar.h"
#include "ammo.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "character_id.h"
#include "character_martial_arts.h"
#include "coordinates.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "effect_source.h"
#include "enums.h"
#include "explosion.h"
#include "faction.h"
#include "flag.h"
#include "game.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "gun_mode.h"
#include "inventory.h"
#include "item_category.h"
#include "item_contents.h"
#include "item_factory.h"
#include "item_location.h"
#include "item_pocket.h"
#include "item_transformation.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "magic_enchantment.h"
#include "map.h"
#include "map_scale_constants.h"
#include "material.h"
#include "math_defines.h"
#include "messages.h"
#include "monster.h"
#include "options.h"
#include "output.h"
#include "pimpl.h"
#include "pocket_type.h"
#include "point.h"
#include "proficiency.h"
#include "projectile.h"
#include "requirements.h"
#include "ret_val.h"
#include "rng.h"
#include "safe_reference.h"
#include "string_formatter.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

class JsonObject;

static const std::string GUN_MODE_VAR_NAME( "item::mode" );

static const ammotype ammo_battery( "battery" );
static const ammotype ammo_bolt( "bolt" );
static const ammotype ammo_plutonium( "plutonium" );

static const damage_type_id damage_bash( "bash" );

static const fault_id fault_overheat_safety( "fault_overheat_safety" );

static const gun_mode_id gun_mode_REACH( "REACH" );

static const item_category_id item_category_spare_parts( "spare_parts" );
static const item_category_id item_category_tools( "tools" );
static const item_category_id item_category_weapons( "weapons" );

static const itype_id itype_arredondo_chute( "arredondo_chute" );
static const itype_id itype_barrel_small( "barrel_small" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_brass_catcher( "brass_catcher" );
static const itype_id itype_bullet_crossbow( "bullet_crossbow" );
static const itype_id itype_hand_crossbow( "hand_crossbow" );
static const itype_id itype_power_cord( "power_cord" );
static const itype_id itype_stock_none( "stock_none" );
static const itype_id itype_tuned_mechanism( "tuned_mechanism" );
static const itype_id itype_waterproof_gunmod( "waterproof_gunmod" );

static const matec_id RAPID( "RAPID" );

static const mtype_id mon_zombie_smoker( "mon_zombie_smoker" );
static const mtype_id mon_zombie_soldier_no_weakpoints( "mon_zombie_soldier_no_weakpoints" );
static const mtype_id mon_zombie_survivor_no_weakpoints( "mon_zombie_survivor_no_weakpoints" );
static const mtype_id pseudo_debug_mon( "pseudo_debug_mon" );

static const skill_id skill_archery( "archery" );

static constexpr float MIN_LINK_EFFICIENCY = 0.001f;

item &null_item_reference()
{
    static item result{};
    // reset it, in case a previous caller has changed it
    result = item();
    return result;
}

units::energy item::mod_energy( const units::energy &qty )
{
    if( !is_vehicle_battery() ) {
        debugmsg( "Tried to set energy of non-battery item" );
        return 0_J;
    }

    units::energy val = energy_remaining( nullptr ) + qty;
    if( val < 0_J ) {
        return val;
    } else if( val > type->battery->max_capacity ) {
        energy = type->battery->max_capacity;
    } else {
        energy = val;
    }
    return 0_J;
}

item &item::ammo_set( const itype_id &ammo, int qty )
{
    if( !ammo->ammo ) {
        if( !has_flag( flag_USES_BIONIC_POWER ) ) {
            debugmsg( "can't set ammo %s in %s as it is not an ammo", ammo.c_str(), type_name() );
        }
        return *this;
    }
    const ammotype &ammo_type = ammo->ammo->type;
    if( qty < 0 ) {
        // completely fill an integral or existing magazine
        //if( magazine_current() ) then we need capacity of the magazine instead of the item maybe?
        if( magazine_integral() || magazine_current() ) {
            qty = ammo_capacity( ammo_type );

            // else try to add a magazine using default ammo count property if set
        } else if( !magazine_default( true ).is_null() ) {
            item mag( magazine_default( true ) );
            if( mag.type->magazine->count > 0 ) {
                qty = mag.type->magazine->count;
            } else {
                qty = mag.ammo_capacity( ammo_type );
            }
        }
    }

    if( qty <= 0 ) {
        ammo_unset();
        return *this;
    }

    // handle reloadable tools and guns with no specific ammo type as special case
    if( ammo.is_null() && ammo_types().empty() ) {
        if( magazine_integral() ) {
            if( is_tool() ) {
                charges = std::min( qty, ammo_capacity( ammo_type ) );
            } else if( is_gun() ) {
                const item temp_ammo( ammo_default(), calendar::turn, std::min( qty, ammo_capacity( ammo_type ) ) );
                put_in( temp_ammo, pocket_type::MAGAZINE );
            }
        }
        return *this;
    }

    // check ammo is valid for the item
    std::set<itype_id> mags = magazine_compatible();
    if( ammo_types().count( ammo_type ) == 0 && !( magazine_current() &&
            magazine_current()->ammo_types().count( ammo_type ) ) &&
    !std::any_of( mags.begin(), mags.end(), [&ammo_type]( const itype_id & mag ) {
    return mag->magazine->type.count( ammo_type );
    } ) ) {
        debugmsg( "Tried to set invalid ammo of %s for %s", ammo.c_str(), typeId().c_str() );
        return *this;
    }

    if( is_magazine() ) {
        ammo_unset();
        item set_ammo( ammo, calendar::turn, std::min( qty, ammo_capacity( ammo_type ) ) );
        if( has_flag( flag_NO_UNLOAD ) ) {
            set_ammo.set_flag( flag_NO_DROP );
            set_ammo.set_flag( flag_IRREMOVABLE );
        }
        put_in( set_ammo, pocket_type::MAGAZINE );

    } else {
        if( !magazine_current() ) {
            itype_id mag = magazine_default();
            if( !mag->magazine ) {
                debugmsg( "Tried to set ammo of %s without suitable magazine for %s",
                          ammo.c_str(), typeId().c_str() );
                return *this;
            }

            item mag_item( mag );
            // if default magazine too small fetch instead closest available match
            if( mag_item.ammo_capacity( ammo_type ) < qty ) {
                std::vector<item> opts;
                for( const itype_id &mag_type : mags ) {
                    if( mag_type->magazine->type.count( ammo_type ) ) {
                        opts.emplace_back( mag_type );
                    }
                }
                if( opts.empty() ) {
                    const std::string magazines_str = enumerate_as_string( mags,
                    []( const itype_id & mag ) {
                        const std::string ammotype_str = enumerate_as_string( mag->magazine->type,
                        []( const ammotype & a ) {
                            return a.str();
                        } );
                        return string_format( _( "%s (taking %s)" ), mag.str(), ammotype_str );
                    } );
                    debugmsg( "Cannot find magazine fitting %s with any capacity for ammo %s "
                              "(ammotype %s).  Magazines considered were %s",
                              typeId().str(), ammo.str(), ammo_type.str(), magazines_str );
                    return *this;
                }
                std::sort( opts.begin(), opts.end(), [&ammo_type]( const item & lhs, const item & rhs ) {
                    return lhs.ammo_capacity( ammo_type ) < rhs.ammo_capacity( ammo_type );
                } );
                auto iter = std::find_if( opts.begin(), opts.end(), [&qty, &ammo_type]( const item & mag ) {
                    return mag.ammo_capacity( ammo_type ) >= qty;
                } );
                if( iter != opts.end() ) {
                    mag = iter->typeId();
                } else {
                    mag = opts.back().typeId();
                }
            }
            put_in( item( mag ), pocket_type::MAGAZINE_WELL );
        }
        item *mag_cur = magazine_current();
        if( mag_cur != nullptr ) {
            mag_cur->ammo_set( ammo, qty );
        }
    }
    return *this;
}

item &item::ammo_unset()
{
    if( !is_tool() && !is_gun() && !is_magazine() ) {
        // do nothing
    } else if( is_magazine() ) {
        if( is_money() ) { // charges are set wrong on cash cards.
            charges = 0;
        }
        contents.clear_magazines();
    } else if( magazine_integral() ) {
        charges = 0;
        if( is_gun() ) {
            contents.clear_magazines();
        }
    } else if( magazine_current() ) {
        magazine_current()->ammo_unset();
    }

    return *this;
}

/*
 * 0 based lookup table of accuracy - monster defense converted into number of hits per 10000
 * attacks
 * data painstakingly looked up at http://onlinestatbook.com/2/calculators/normal_dist.html
 */
static constexpr std::array<double, 41> hits_by_accuracy = {
    0,    1,   2,   3,   7, // -20 to -16
    13,   26,  47,   82,  139, // -15 to -11
    228,   359,  548,  808, 1151, // -10 to -6
    1587, 2119, 2743, 3446, 4207, // -5 to -1
    5000,  // 0
    5793, 6554, 7257, 7881, 8413, // 1 to 5
    8849, 9192, 9452, 9641, 9772, // 6 to 10
    9861, 9918, 9953, 9974, 9987, // 11 to 15
    9993, 9997, 9998, 9999, 10000 // 16 to 20
};

double item::effective_dps( const Character &guy, Creature &mon ) const
{
    const float mon_dodge = mon.get_dodge();
    float base_hit = guy.get_dex() / 4.0f + guy.get_hit_weapon( *this );
    base_hit *= std::max( 0.25f, 1.0f - guy.avg_encumb_of_limb_type( bp_type::torso ) /
                          100.0f );
    float mon_defense = mon_dodge + mon.size_melee_penalty() / 5.0f;
    constexpr double hit_trials = 10000.0;
    const int rng_mean = std::max( std::min( static_cast<int>( base_hit - mon_defense ), 20 ),
                                   -20 ) + 20;
    double num_all_hits = hits_by_accuracy[ rng_mean ];
    /* critical hits have two chances to occur: triple critical hits happen much less frequently,
     * and double critical hits can only occur if a hit roll is more than 1.5 * monster dodge.
     * Not the hit roll used to determine the attack, another one.
     * the way the math works, some percentage of the total hits are eligible to be double
     * critical hits, and the rest are eligible to be triple critical hits, but in each case,
     * only some small percent of them actually become critical hits.
     */
    const int rng_high_mean = std::max( std::min( static_cast<int>( base_hit - 1.5 * mon_dodge ),
                                        20 ), -20 ) + 20;
    double num_high_hits = hits_by_accuracy[ rng_high_mean ] * num_all_hits / hit_trials;
    double double_crit_chance = guy.crit_chance( 4, 0, *this );
    double crit_chance = guy.crit_chance( 0, 0, *this );
    double num_low_hits = std::max( 0.0, num_all_hits - num_high_hits );

    double moves_per_attack = guy.attack_speed( *this );
    // attacks that miss do no damage but take time
    double total_moves = ( hit_trials - num_all_hits ) * moves_per_attack;
    double total_damage = 0.0;
    double num_crits = std::min( num_low_hits * crit_chance + num_high_hits * double_crit_chance,
                                 num_all_hits );
    // critical hits are counted separately
    double num_hits = num_all_hits - num_crits;
    // sum average damage past armor and return the number of moves required to achieve
    // that damage
    const auto calc_effective_damage = [ &, moves_per_attack]( const double num_strikes,
    const bool crit, const Character & guy, Creature & mon ) {
        bodypart_id bp = bodypart_id( "torso" );
        Creature *temp_mon = &mon;
        double subtotal_damage = 0;
        damage_instance base_damage;
        guy.roll_all_damage( crit, base_damage, true, *this, attack_vector_id::NULL_ID(),
                             sub_bodypart_str_id::NULL_ID(), &mon, bp );
        damage_instance dealt_damage = base_damage;
        dealt_damage = guy.modify_damage_dealt_with_enchantments( dealt_damage );
        // TODO: Modify DPS calculation to consider weakpoints.
        resistances r = resistances( *static_cast<monster *>( temp_mon ) );
        for( damage_unit &dmg_unit : dealt_damage.damage_units ) {
            dmg_unit.amount -= std::min( r.get_effective_resist( dmg_unit ), dmg_unit.amount );
        }

        dealt_damage_instance dealt_dams;
        for( const damage_unit &dmg_unit : dealt_damage.damage_units ) {
            int cur_damage = 0;
            int total_pain = 0;
            temp_mon->deal_damage_handle_type( effect_source::empty(), dmg_unit, bp,
                                               cur_damage, total_pain );
            if( cur_damage > 0 ) {
                dealt_dams.dealt_dams[dmg_unit.type] += cur_damage;
            }
        }
        double damage_per_hit = dealt_dams.total_damage();
        subtotal_damage = damage_per_hit * num_strikes;
        double subtotal_moves = moves_per_attack * num_strikes;

        if( has_technique( RAPID ) ) {
            Creature *temp_rs_mon = &mon;
            damage_instance rs_base_damage;
            guy.roll_all_damage( crit, rs_base_damage, true, *this, attack_vector_id::NULL_ID(),
                                 sub_bodypart_str_id::NULL_ID(), &mon, bp );
            damage_instance dealt_rs_damage = rs_base_damage;
            for( damage_unit &dmg_unit : dealt_rs_damage.damage_units ) {
                dmg_unit.damage_multiplier *= 0.66;
            }
            // TODO: Modify DPS calculation to consider weakpoints.
            resistances rs_r = resistances( *static_cast<monster *>( temp_rs_mon ) );
            for( damage_unit &dmg_unit : dealt_rs_damage.damage_units ) {
                dmg_unit.amount -= std::min( rs_r.get_effective_resist( dmg_unit ), dmg_unit.amount );
            }
            dealt_damage_instance rs_dealt_dams;
            for( const damage_unit &dmg_unit : dealt_rs_damage.damage_units ) {
                int cur_damage = 0;
                int total_pain = 0;
                temp_rs_mon->deal_damage_handle_type( effect_source::empty(), dmg_unit, bp,
                                                      cur_damage, total_pain );
                if( cur_damage > 0 ) {
                    rs_dealt_dams.dealt_dams[dmg_unit.type] += cur_damage;
                }
            }
            double rs_damage_per_hit = rs_dealt_dams.total_damage();
            subtotal_moves *= 0.5;
            subtotal_damage *= 0.5;
            subtotal_moves += moves_per_attack * num_strikes * 0.33;
            subtotal_damage += rs_damage_per_hit * num_strikes * 0.5;
        }
        return std::make_pair( subtotal_moves, subtotal_damage );
    };
    std::pair<double, double> crit_summary = calc_effective_damage( num_crits, true, guy, mon );
    total_moves += crit_summary.first;
    total_damage += crit_summary.second;
    std::pair<double, double> summary = calc_effective_damage( num_hits, false, guy, mon );
    total_moves += summary.first;
    total_damage += summary.second;
    return total_damage * to_moves<double>( 1_seconds ) / total_moves;
}

struct dps_comp_data {
    mtype_id mon_id;
    bool display;
    bool evaluate;
};

static const std::vector<std::pair<translation, dps_comp_data>> dps_comp_monsters = {
    { to_translation( "Best" ), { pseudo_debug_mon, true, false } },
    { to_translation( "Vs. Agile" ), { mon_zombie_smoker, true, true } },
    { to_translation( "Vs. Armored" ), { mon_zombie_soldier_no_weakpoints, true, true } },
    { to_translation( "Vs. Mixed" ), { mon_zombie_survivor_no_weakpoints, false, true } },
};

std::map<std::string, double> item::dps( const bool for_display, const bool for_calc,
        const Character &guy ) const
{
    std::map<std::string, double> results;
    for( const std::pair<translation, dps_comp_data> &comp_mon : dps_comp_monsters ) {
        if( ( comp_mon.second.display != for_display ) &&
            ( comp_mon.second.evaluate != for_calc ) ) {
            continue;
        }
        monster test_mon = monster( comp_mon.second.mon_id );
        results[ comp_mon.first.translated() ] = effective_dps( guy, test_mon );
    }
    return results;
}

std::map<std::string, double> item::dps( const bool for_display, const bool for_calc ) const
{
    return dps( for_display, for_calc, get_avatar() );
}

double item::average_dps( const Character &guy ) const
{
    double dmg_count = 0.0;
    const std::map<std::string, double> &dps_data = dps( false, true, guy );
    for( const std::pair<const std::string, double> &dps_entry : dps_data ) {
        dmg_count += dps_entry.second;
    }
    return dmg_count / dps_data.size();
}

std::map<gunmod_location, int> item::get_mod_locations() const
{
    std::map<gunmod_location, int> mod_locations = type->gun->valid_mod_locations;

    for( const item *mod : gunmods() ) {
        if( !mod->type->gunmod->add_mod.empty() ) {
            std::map<gunmod_location, int> add_locations = mod->type->gunmod->add_mod;

            for( const std::pair<const gunmod_location, int> &add_location : add_locations ) {
                mod_locations[add_location.first] += add_location.second;
            }
        }
    }

    return mod_locations;
}

int item::get_free_mod_locations( const gunmod_location &location ) const
{
    if( !is_gun() ) {
        return 0;
    }

    std::map<gunmod_location, int> mod_locations = get_mod_locations();

    const auto loc = mod_locations.find( location );
    if( loc == mod_locations.end() ) {
        return 0;
    }
    int result = loc->second;
    for( const item *elem : contents.all_items_top( pocket_type::MOD ) ) {
        const cata::value_ptr<islot_gunmod> &mod = elem->type->gunmod;
        if( mod && mod->location == location ) {
            result--;
        }
    }
    return result;
}

int item::on_wield_cost( const Character &you ) const
{
    int mv = 0;
    // weapons with bayonet/bipod or other generic "unhandiness"
    if( has_flag( flag_SLOW_WIELD ) && !is_gunmod() ) {
        float d = 32.0f; // arbitrary linear scaling factor
        if( is_gun() ) {
            d /= std::max( you.get_skill_level( gun_skill() ), 1.0f );
        } else if( is_melee() ) {
            d /= std::max( you.get_skill_level( melee_skill() ), 1.0f );
        }

        int penalty = get_var( "volume", volume() / 250_ml ) * d;
        // arbitrary no more than 7 second of penalty
        mv += std::min( penalty, 700 );
    }

    // firearms with a folding stock or tool/melee without collapse/retract iuse
    if( has_flag( flag_NEEDS_UNFOLD ) && !is_gunmod() ) {
        int penalty = 50; // 200-300 for guns, 50-150 for melee, 50 as fallback
        if( is_gun() ) {
            penalty = std::max( 0, 300 - static_cast<int>( you.get_skill_level( gun_skill() ) * 10 ) );
        } else if( is_melee() ) {
            penalty = std::max( 0, 150 - static_cast<int>( you.get_skill_level( melee_skill() ) * 10 ) );
        }

        mv += penalty;
    }
    return mv;
}

void item::on_wield( Character &you, bool combat )
{
    int wield_cost = on_wield_cost( you );
    you.mod_moves( -wield_cost );

    std::string msg;

    msg = _( "You wield your %s." );

    // if game is loaded - don't want ownership assigned during char creation
    if( get_player_character().getID().is_valid() ) {
        handle_pickup_ownership( you );
    }
    you.add_msg_if_player( m_neutral, msg, tname() );

    if( combat && !you.martial_arts_data->selected_is_none() ) {
        you.martial_arts_data->martialart_use_message( you );
    }

    // Update encumbrance and discomfort in case we were wearing it
    you.flag_encumbrance();
    you.calc_discomfort();
    you.on_item_acquire( *this );
}

std::string item::dirt_symbol() const
{
    // these symbols are unicode square characters of different heights, representing a rough
    // estimation of fouling in a gun. This appears instead of "faulty" since most guns will
    // have some level of fouling in them, and usually it is not a big deal.
    switch( static_cast<int>( get_var( "dirt", 0 ) / 2000 ) ) {
        // *INDENT-OFF*
        case 1:  return "<color_white>\u2581</color>";
        case 2:  return "<color_light_gray>\u2583</color>";
        case 3:  return "<color_light_gray>\u2585</color>";
        case 4:  return "<color_dark_gray>\u2587</color>";
        case 5:  return "<color_brown>\u2588</color>";
        default: return "";
        // *INDENT-ON*
    }
}

std::string item::overheat_symbol() const
{

    if( !is_gun() || type->gun->overheat_threshold <= 0.0 ) {
        return "";
    }
    double modifier = 0;
    float multiplier = 1.0f;
    for( const item *mod : gunmods() ) {
        modifier += mod->type->gunmod->overheat_threshold_modifier;
        multiplier *= mod->type->gunmod->overheat_threshold_multiplier;
    }
    if( has_fault( fault_overheat_safety ) ) {
        return string_format( _( "<color_light_green>\u2588VNT </color>" ) );
    }
    switch( std::min( 5, static_cast<int>( get_var( "gun_heat",
                                           0 ) / std::max( type->gun->overheat_threshold * multiplier + modifier, 5.0 ) * 5.0 ) ) ) {
        case 1:
            return "";
        case 2:
            return string_format( _( "<color_yellow>\u2583HEAT </color>" ) );
        case 3:
            return string_format( _( "<color_light_red>\u2585HEAT </color>" ) );
        case 4:
            return string_format( _( "<color_red>\u2587HEAT! </color>" ) ); // NOLINT(cata-text-style)
        case 5:
            return string_format( _( "<color_pink>\u2588HEAT!! </color>" ) ); // NOLINT(cata-text-style)
        default:
            return "";
    }
}

units::length item::sawn_off_reduction() const
{
    int barrel_percentage = type->gun->barrel_volume / ( type->volume / 100 );
    return type->longest_side * barrel_percentage / 100;
}

units::length item::barrel_length() const
{
    if( is_gun() ) {
        units::length l = type->gun->barrel_length;
        for( const item *mod : mods() ) {
            // if a gun has a barrel length specifying mod then use that length for sure
            if( !mod->is_gun() && mod->type->gunmod->barrel_length > 0_mm ) {
                l = mod->type->gunmod->barrel_length;
                // if we find an explicit barrel mod then use that and quit the loop
                break;
            }
        }
        // if we've sawn off the barrel reduce the damage
        if( gunmod_find( itype_barrel_small ) ) {
            l = l - sawn_off_reduction();
        }

        return l;
    } else {
        return 0_mm;
    }
}

int item::attack_time( const Character &you ) const
{
    int ret = 65 + ( volume() / 62.5_ml + weight() / 60_gram ) / count();
    ret = calculate_by_enchantment_wield( you, ret, enchant_vals::mod::ITEM_ATTACK_SPEED,
                                          true );
    return ret;
}

int item::damage_melee( const damage_type_id &dt ) const
{
    if( is_null() ) {
        return 0;
    }

    // effectiveness is reduced by 10% per damage level
    int res = 0;
    if( type->melee.damage_map.count( dt ) > 0 ) {
        res = type->melee.damage_map.at( dt );
    }
    res = damage_adjusted_melee_weapon_damage( res, dt );

    // apply type specific flags
    // FIXME: Hardcoded damage types
    if( dt == damage_bash && has_flag( flag_REDUCED_BASHING ) ) {
        res *= 0.5;
    }
    if( dt->edged && has_flag( flag_DIAMOND ) ) {
        res *= 1.3;
    }

    // consider any attached bayonets
    if( is_gun() ) {
        std::vector<int> opts = { res };
        for( const item *mod : gunmods() ) {
            if( mod->type->gunmod->is_bayonet ) {
                opts.push_back( mod->damage_melee( dt ) );
            }
        }
        return *std::max_element( opts.begin(), opts.end() );

    }

    return std::max( res, 0 );
}

damage_instance item::base_damage_melee() const
{
    // TODO: Caching
    damage_instance ret;
    for( const damage_type &dt : damage_type::get_all() ) {
        int dam = damage_melee( dt.id );
        if( dam > 0 ) {
            ret.add_damage( dt.id, dam );
        }
    }

    return ret;
}

damage_instance item::base_damage_thrown() const
{
    // TODO: Create a separate cache for individual items (for modifiers like diamond etc.)
    return type->thrown_damage;
}

int item::reach_range( const Character &guy ) const
{
    int res = 1;
    int reach_attack_add = has_flag( flag_REACH_ATTACK ) ? has_flag( flag_REACH3 ) ? 2 : 1 : 0;

    res += reach_attack_add;

    if( !is_gun() ) {
        res = std::max( 1, static_cast<int>( guy.calculate_by_enchantment( res,
                                             enchant_vals::mod::MELEE_RANGE_MODIFIER ) ) );
    }

    // for guns consider any attached gunmods
    if( is_gun() && !is_gunmod() ) {
        for( const std::pair<const gun_mode_id, gun_mode> &m : gun_all_modes() ) {
            if( guy.is_npc() && m.second.flags.count( "NPC_AVOID" ) ) {
                continue;
            }
            if( m.second.melee() ) {
                res = std::max( res, std::max( 1,
                                               static_cast<int>( guy.calculate_by_enchantment( m.second.qty + reach_attack_add,
                                                       enchant_vals::mod::MELEE_RANGE_MODIFIER ) ) ) );
            }
        }
    }

    return res;
}

int item::current_reach_range( const Character &guy ) const
{
    int res = 1;

    if( has_flag( flag_REACH_ATTACK ) ) {
        res = has_flag( flag_REACH3 ) ? 3 : 2;
    } else if( is_gun() && !is_gunmod() && gun_current_mode().melee() ) {
        res = gun_current_mode().target->gun_range();
    }

    if( !is_gun() || ( is_gun() && !is_gunmod() && gun_current_mode().melee() ) ) {
        res = std::max( 1, static_cast<int>( guy.calculate_by_enchantment( res,
                                             enchant_vals::mod::MELEE_RANGE_MODIFIER ) ) );
    }

    if( is_gun() && !is_gunmod() ) {
        gun_mode gun = gun_current_mode();
        if( !( guy.is_npc() && gun.flags.count( "NPC_AVOID" ) ) && gun.melee() ) {
            res = std::max( res, gun.qty );
        }
    }

    return res;
}

bool item::has_technique( const matec_id &tech ) const
{
    return type->techniques.count( tech ) > 0 || techniques.count( tech ) > 0;
}

void item::add_technique( const matec_id &tech )
{
    techniques.insert( tech );
}

std::vector<item *> item::toolmods()
{
    std::vector<item *> res;
    if( is_tool() ) {
        for( item *e : contents.all_items_top( pocket_type::MOD ) ) {
            if( e->is_toolmod() ) {
                res.push_back( e );
            }
        }
    }
    return res;
}

std::vector<const item *> item::toolmods() const
{
    std::vector<const item *> res;
    if( is_tool() ) {
        for( const item *e : contents.all_items_top( pocket_type::MOD ) ) {
            if( e->is_toolmod() ) {
                res.push_back( e );
            }
        }
    }
    return res;
}

std::set<matec_id> item::get_techniques() const
{
    std::set<matec_id> result = type->techniques;
    result.insert( techniques.begin(), techniques.end() );
    return result;
}

bool item::is_maybe_melee_weapon() const
{
    item_category_id my_cat_id = get_category_shallow().id;
    return my_cat_id == item_category_weapons || my_cat_id == item_category_spare_parts ||
           my_cat_id == item_category_tools;
}

bool item::has_edged_damage() const
{
    for( const damage_type &dt : damage_type::get_all() ) {
        if( dt.edged && is_melee( dt.id ) ) {
            return true;
        }
    }
    return false;
}

bool item::is_gun() const
{
    return !!type->gun;
}

bool item::is_firearm() const
{
    return is_gun() && !has_flag( flag_PRIMITIVE_RANGED_WEAPON );
}

int item::get_reload_time() const
{
    if( !is_gun() && !is_magazine() ) {
        return 0;
    }
    int reload_time = 0;
    if( is_gun() ) {
        reload_time = type->gun->reload_time;
    } else if( type->magazine ) {
        reload_time = type->magazine->reload_time;
    } else {
        reload_time = INVENTORY_HANDLING_PENALTY;
    }
    for( const item *mod : gunmods() ) {
        reload_time = static_cast<int>( reload_time * ( 100 + mod->type->gunmod->reload_modifier ) / 100 );
    }

    return reload_time;
}

bool item::is_silent() const
{
    return gun_noise().volume < 5;
}

bool item::is_gunmod() const
{
    return !!type->gunmod;
}

bool item::is_magazine() const
{
    return !!type->magazine || contents.has_pocket_type( pocket_type::MAGAZINE );
}

bool item::is_battery() const
{
    return is_magazine() && ammo_capacity( ammo_battery );
}

bool item::is_vehicle_battery() const
{
    return !!type->battery;
}

bool item::is_ammo_belt() const
{
    return is_magazine() && has_flag( flag_MAG_BELT );
}

bool item::is_ammo() const
{
    return !!type->ammo;
}

bool item::is_medical_tool() const
{
    return type->get_use( "heal" ) != nullptr;
}

bool item::is_ammo_container() const
{
    return contents.has_any_with(
    []( const item & it ) {
        return it.is_ammo();
    }, pocket_type::CONTAINER );
}

bool item::is_melee() const
{
    for( const damage_type &dt : damage_type::get_all() ) {
        if( is_melee( dt.id ) ) {
            return true;
        }
    }
    return false;
}

bool item::is_melee( const damage_type_id &dt ) const
{
    return damage_melee( dt ) > MELEE_STAT;
}

bool item::is_fuel() const
{
    // A fuel is made of only one material
    if( type->materials.size() != 1 ) {
        return false;
    }
    // and this material has to produce energy
    if( get_base_material().get_fuel_data().energy <= 0_J ) {
        return false;
    }
    // and it needs to be have consumable charges
    return count_by_charges();
}

bool item::is_toolmod() const
{
    return !is_gunmod() && type->mod;
}

units::energy item::fuel_energy() const
{
    // The odd units and division are to avoid integer rounding errors.
    return get_base_material().get_fuel_data().energy * units::to_milliliter( base_volume() ) / 1000;
}

std::string item::fuel_pump_terrain() const
{
    return get_base_material().get_fuel_data().pump_terrain;
}

bool item::has_explosion_data() const
{
    return !get_base_material().get_fuel_data().explosion_data.is_empty();
}

struct fuel_explosion_data item::get_explosion_data() const {
    return get_base_material().get_fuel_data().explosion_data;
}

bool item::is_magazine_full() const
{
    return contents.is_magazine_full();
}

bool item::allows_speedloader( const itype_id &speedloader_id ) const
{
    return contents.allows_speedloader( speedloader_id );
}

bool item::can_reload_with( const item &ammo, bool now ) const
{
    if( has_flag( flag_NO_RELOAD ) && !has_flag( flag_VEHICLE ) ) {
        return false; // turrets ignore NO_RELOAD flag
    }

    if( now && ammo.is_magazine() && !ammo.empty() ) {
        if( is_tool() ) {
            // Dirty hack because "ammo" on tools is actually completely separate thing from "ammo" on guns and "ammo_types()" works only for guns
            if( !type->tool->ammo_id.count( ammo.contents.first_ammo().ammo_type() ) ) {
                return false;
            }
        } else {
            if( !ammo_types().count( ammo.contents.first_ammo().ammo_type() ) ) {
                return false;
            }
        }
    }

    // Check if the item is in general compatible with any reloadable pocket.
    return contents.can_reload_with( ammo, now );
}

bool item::is_tool() const
{
    return !!type->tool;
}

skill_id item::gun_skill() const
{
    if( !is_gun() ) {
        return skill_id::NULL_ID();
    }
    return type->gun->skill_used;
}

gun_type_type item::gun_type() const
{
    static skill_id skill_archery( "archery" );

    if( !is_gun() ) {
        return gun_type_type( std::string() );
    }
    // TODO: move to JSON and remove extraction of this from "GUN" (via skill id)
    //and from "GUNMOD" (via "mod_targets") in lang/extract_json_strings.py
    if( gun_skill() == skill_archery ) {
        if( ammo_types().count( ammo_bolt ) || typeId() == itype_bullet_crossbow ) {
            return gun_type_type( translate_marker_context( "gun_type_type", "crossbow" ) );
        } else {
            return gun_type_type( translate_marker_context( "gun_type_type", "bow" ) );
        }
    }
    return gun_type_type( gun_skill().str() );
}

skill_id item::melee_skill() const
{
    if( !is_melee() ) {
        return skill_id::NULL_ID();
    }

    int hi = 0;
    skill_id res = skill_id::NULL_ID();

    for( const damage_type &dt : damage_type::get_all() ) {
        const int val = damage_melee( dt.id );
        if( val > hi && !dt.skill.is_null() ) {
            hi = val;
            res = dt.skill;
        }
    }

    return res;
}

int item::min_cycle_recoil() const
{
    if( !is_gun() ) {
        return 0;
    }
    int to_cycle = type->gun->min_cycle_recoil;
    // This should only be used for one mod or it'll mess things up
    // TODO: maybe generalize this so you can have mods for hot loads or whatever
    for( const item *mod : gunmods() ) {
        // this value defaults to -1
        if( mod->type->gunmod->overwrite_min_cycle_recoil > 0 ) {
            to_cycle = mod->type->gunmod->overwrite_min_cycle_recoil;
        }
    }
    return to_cycle;
}

int item::gun_dispersion( bool with_ammo, bool with_scaling ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int dispersion_sum = type->gun->dispersion;
    for( const item *mod : gunmods() ) {
        dispersion_sum += mod->type->gunmod->dispersion;
    }
    int dispPerDamage = get_option< int >( "DISPERSION_PER_GUN_DAMAGE" );
    dispersion_sum += damage_level() * dispPerDamage;
    dispersion_sum = std::max( dispersion_sum, 0 );
    if( with_ammo && has_ammo() ) {
        dispersion_sum += ammo_data()->ammo->dispersion_considering_length( barrel_length() );
    }
    if( !with_scaling ) {
        return dispersion_sum;
    }

    // Dividing dispersion by 15 temporarily as a gross adjustment,
    // will bake that adjustment into individual gun definitions in the future.
    // Absolute minimum gun dispersion is 1.
    double divider = get_option< float >( "GUN_DISPERSION_DIVIDER" );
    dispersion_sum = std::max( static_cast<int>( std::round( dispersion_sum / divider ) ), 1 );

    return dispersion_sum;
}

std::pair<int, int> item::sight_dispersion( const Character &character ) const
{
    if( !is_gun() ) {
        return std::make_pair( 0, 0 );
    }
    int act_disp = has_flag( flag_DISABLE_SIGHTS ) ? 300 : type->gun->sight_dispersion;
    int eff_disp = character.effective_dispersion( act_disp );

    for( const item *e : gunmods() ) {
        const islot_gunmod &mod = *e->type->gunmod;
        int e_act_disp = mod.sight_dispersion;
        if( mod.sight_dispersion < 0 || mod.field_of_view <= 0 ) {
            continue;
        }
        int e_eff_disp = character.effective_dispersion( e_act_disp, e->has_flag( flag_ZOOM ) );
        if( eff_disp > e_eff_disp ) {
            eff_disp = e_eff_disp;
            act_disp = e_act_disp;

        }
    }
    return std::make_pair( act_disp, eff_disp );
}

damage_instance item::gun_damage( bool with_ammo, bool shot ) const
{
    if( !is_gun() ) {
        return damage_instance();
    }

    units::length bl = barrel_length();
    damage_instance ret = type->gun->damage.di_considering_length( bl );

    for( const item *mod : gunmods() ) {
        ret.add( mod->type->gunmod->damage.di_considering_length( bl ) );
    }

    if( with_ammo && has_ammo() ) {
        if( shot ) {
            ret.add( ammo_data()->ammo->shot_damage.di_considering_length( bl ) );
        } else {
            ret.add( ammo_data()->ammo->damage.di_considering_length( bl ) );
        }
    }

    if( damage() > 0 ) {
        // TODO: This isn't a good solution for multi-damage guns/ammos
        for( damage_unit &du : ret ) {
            if( du.amount <= 1.0 ) {
                continue;
            }
            du.amount = std::max<float>( 1.0f, damage_adjusted_gun_damage( du.amount ) );
        }
    }

    return ret;
}

damage_instance item::gun_damage( itype_id ammo ) const
{
    if( !is_gun() ) {
        return damage_instance();
    }

    units::length bl = barrel_length();
    damage_instance ret = type->gun->damage.di_considering_length( bl );

    for( const item *mod : gunmods() ) {
        ret.add( mod->type->gunmod->damage.di_considering_length( bl ) );
    }

    ret.add( ammo->ammo->damage.di_considering_length( bl ) );

    if( damage() > 0 ) {
        // TODO: This isn't a good solution for multi-damage guns/ammos
        for( damage_unit &du : ret ) {
            if( du.amount <= 1.0 ) {
                continue;
            }
            du.amount = std::max<float>( 1.0f, damage_adjusted_gun_damage( du.amount ) );
        }
    }

    return ret;
}
units::mass item::gun_base_weight() const
{
    units::mass base_weight = type->weight;
    for( const item *mod : gunmods() ) {
        if( !mod->type->mod->ammo_modifier.empty() ) {
            base_weight += mod->type->integral_weight;
        }
    }
    return base_weight;

}
int item::gun_recoil( const Character &p, bool bipod, bool ideal_strength ) const
{
    if( !is_gun() || ( ammo_required() && !ammo_remaining( ) ) ) {
        return 0;
    }

    ///\ARM_STR improves the handling of heavier weapons
    // we consider only base weight to avoid exploits
    // now we need to add weight of receiver
    double wt = ideal_strength ? gun_base_weight() / 333.0_gram : std::min( gun_base_weight(),
                p.get_arm_str() * 333_gram ) / 333.0_gram;

    double handling = type->gun->handling;
    for( const item *mod : gunmods() ) {
        if( bipod || !mod->has_flag( flag_BIPOD ) ) {
            handling += mod->type->gunmod->handling;
        }
    }

    // rescale from JSON units which are intentionally specified as integral values
    handling /= 10;

    // algorithm is biased so heavier weapons benefit more from improved handling
    handling = std::pow( wt, 0.8 ) * std::pow( handling, 1.2 );

    int qty = type->gun->recoil;
    if( has_ammo() ) {
        qty += ammo_data()->ammo->recoil;
    }

    // handling could be either a bonus or penalty dependent upon installed mods
    if( handling > 1.0 ) {
        return qty / handling;
    } else {
        return qty * ( 1.0 + std::abs( handling ) );
    }
}

float item::gun_shot_spread_multiplier() const
{
    if( !is_gun() ) {
        return 0;
    }
    float ret = 1.0f;
    for( const item *mod : gunmods() ) {
        ret += mod->type->gunmod->shot_spread_multiplier_modifier;
    }
    return std::max( ret, 0.0f );
}

int item::gun_range( bool with_ammo ) const
{
    if( !is_gun() ) {
        return 0;
    }
    int ret = type->gun->range;
    float range_multiplier = 1.0;
    for( const item *mod : gunmods() ) {
        ret += mod->type->gunmod->range;
        range_multiplier *= mod->type->gunmod->range_multiplier;
    }
    if( with_ammo && has_ammo() ) {
        const itype *ammo_info = ammo_data();
        ret += ammo_info->ammo->range;
        range_multiplier *= ammo_info->ammo->range_multiplier;
    }
    ret *= range_multiplier;
    return std::min( std::max( 0, ret ), RANGE_HARD_CAP );
}

int item::gun_range( const Character *p ) const
{
    int ret = gun_range( true );
    if( p == nullptr ) {
        return ret;
    }
    if( !p->meets_requirements( *this ) ) {
        return 0;
    }

    // Reduce bow range until player has twice minimum required strength
    if( has_flag( flag_STR_DRAW ) ) {
        ret += std::max( 0.0, ( p->get_str() - get_min_str() ) * 0.5 );
    }

    ret = p->enchantment_cache->modify_value( enchant_vals::mod::RANGE, ret );

    return std::max( 0, ret );
}

int item::shots_remaining( const map &here, const Character *carrier ) const
{
    int ret = 1000; // Arbitrary large number for things that do not require ammo.
    if( ammo_required() ) {
        ret = std::min( ammo_remaining_linked( here,  carrier ) / ammo_required(), ret );
    }
    if( get_gun_energy_drain() > 0_kJ ) {
        ret = std::min( static_cast<int>( energy_remaining( carrier ) / get_gun_energy_drain() ), ret );
    }
    return ret;
}

int item::ammo_remaining( const map &here, const std::set<ammotype> &ammo, const Character *carrier,
                          const bool include_linked ) const
{
    const bool is_tool_with_carrier = carrier != nullptr && is_tool();

    if( is_tool_with_carrier && has_flag( flag_USES_NEARBY_AMMO ) && !ammo.empty() ) {
        const inventory &crafting_inventory = carrier->crafting_inventory();
        const ammotype &a = *ammo.begin();
        return crafting_inventory.charges_of( a->default_ammotype(), INT_MAX );
    }

    int ret = 0;

    // Magazine in the item
    const item *mag = magazine_current();
    if( mag ) {
        ret += mag->ammo_remaining( );
    }

    // Cable connections
    if( include_linked && link_length() >= 0 && link().efficiency >= MIN_LINK_EFFICIENCY ) {
        if( link().t_veh ) {
            ret += link().t_veh->connected_battery_power_level( here ).first;
        } else {
            const optional_vpart_position vp = here.veh_at( link().t_abs_pos );
            if( vp ) {
                ret += vp->vehicle().connected_battery_power_level( here ).first;
            }
        }
    }

    // Non ammo using item that uses charges
    if( ammo.empty() ) {
        ret += charges;
    }

    // Magazines and integral magazines on their own
    if( is_magazine() ) {
        for( const item *e : contents.all_items_top( pocket_type::MAGAZINE ) ) {
            if( e->is_ammo() ) {
                ret += e->charges;
            }
        }
    }

    // Handle non-magazines with ammo_restriction in a CONTAINER type pocket (like quivers)
    if( !( mag || is_magazine() || ammo.empty() ) ) {
        for( const item *e : contents.all_items_top( pocket_type::CONTAINER ) ) {
            if( e->is_ammo() && ammo.find( e->ammo_type() ) != ammo.end() ) {
                ret += e->charges;
            }
        }
    }

    // UPS/bionic power can replace ammo requirement.
    // Only for tools. Guns should always use energy_drain for electricity use.
    if( is_tool_with_carrier ) {
        if( has_flag( flag_USES_BIONIC_POWER ) ) {
            ret += units::to_kilojoule( carrier->get_power_level() );
        }
        if( has_flag( flag_USE_UPS ) ) {
            ret += units::to_kilojoule( carrier->available_ups() );
        }
    }

    return ret;
}

int item::ammo_remaining_linked( const map &here, const Character *carrier ) const
{
    std::set<ammotype> ammo = ammo_types();
    return ammo_remaining( here, ammo, carrier, true );
}
int item::ammo_remaining( const Character *carrier ) const
{
    std::set<ammotype> ammo = ammo_types();
    return ammo_remaining( get_map(), ammo, carrier, false ); // get_map() is a dummy parameter here.
}

int item::ammo_remaining_linked( const map &here ) const
{
    return ammo_remaining_linked( here, nullptr );
}

int item::ammo_remaining( ) const
{
    return ammo_remaining( nullptr );
}

bool item::uses_energy() const
{
    if( is_vehicle_battery() ) {
        return true;
    }
    const item *mag = magazine_current();
    if( mag && mag->uses_energy() ) {
        return true;
    }
    return has_flag( flag_USES_BIONIC_POWER ) ||
           has_flag( flag_USE_UPS ) ||
           ( is_magazine() && ammo_capacity( ammo_battery ) > 0 );
}

bool item::is_chargeable() const
{
    if( !uses_energy() ) {
        return false;
    }
    // bionic power using items have ammo_capacity = player bionic power storage.  Since the items themselves aren't chargeable, auto fail unless they also have a magazine.
    if( has_flag( flag_USES_BIONIC_POWER ) && !magazine_current() ) {
        return false;
    }
    if( ammo_remaining() < ammo_capacity( ammo_battery ) ) {
        return true;
    } else {
        return false;
    }
}

units::energy item::energy_remaining( const Character *carrier ) const
{
    return energy_remaining( carrier, false );
}

units::energy item::energy_remaining( const Character *carrier, bool ignoreExternalSources ) const
{
    units::energy ret = 0_kJ;

    // Magazine in the item
    const item *mag = magazine_current();
    if( mag ) {
        ret += mag->energy_remaining( carrier );
    }

    if( !ignoreExternalSources ) {

        // Future energy based batteries
        if( is_vehicle_battery() ) {
            ret += energy;
        }

        // Power from bionic
        if( carrier != nullptr && has_flag( flag_USES_BIONIC_POWER ) ) {
            ret += carrier->get_power_level();
        }

        // Extra power from UPS
        if( carrier != nullptr && has_flag( flag_USE_UPS ) ) {
            ret += carrier->available_ups();
        }
    };

    // Battery(ammo) contained within
    if( is_magazine() ) {
        ret += energy;
        for( const item *e : contents.all_items_top( pocket_type::MAGAZINE ) ) {
            if( e->ammo_type() == ammo_battery ) {
                ret += units::from_kilojoule( static_cast<std::int64_t>( e->charges ) );
            }
        }
    }

    return ret;
}

int item::remaining_ammo_capacity() const
{
    if( ammo_types().empty() ) {
        return 0;
    }

    const itype *loaded_ammo = ammo_data();
    if( loaded_ammo == nullptr ) {
        return ammo_capacity( item::find_type( ammo_default() )->ammo->type ) - ammo_remaining( );
    } else {
        return ammo_capacity( loaded_ammo->ammo->type ) - ammo_remaining( );
    }
}

int item::ammo_capacity( const ammotype &ammo, bool include_linked ) const
{
    map &here = get_map();

    const item *mag = magazine_current();
    if( include_linked && has_link_data() ) {
        return link().t_veh ? link().t_veh->connected_battery_power_level( here ).second : 0;
    } else if( mag ) {
        return mag->ammo_capacity( ammo );
    } else if( has_flag( flag_USES_BIONIC_POWER ) ) {
        return units::to_kilojoule( get_player_character().get_max_power_level() );
    }

    if( contents.has_pocket_type( pocket_type::MAGAZINE ) ) {
        return contents.ammo_capacity( ammo );
    }
    if( is_magazine() ) {
        return type->magazine->capacity;
    }
    return 0;
}

int item::ammo_required() const
{
    if( is_gun() ) {
        if( type->gun->ammo.empty() ) {
            return 0;
        } else {
            int modifier = 0;
            float multiplier = 1.0f;
            for( const item *mod : gunmods() ) {
                modifier += mod->type->gunmod->ammo_to_fire_modifier;
                multiplier *= mod->type->gunmod->ammo_to_fire_multiplier;
            }
            return ( type->gun->ammo_to_fire * multiplier ) + modifier;
        }
    }

    return type->charges_to_use();
}

item &item::first_ammo()
{
    return contents.first_ammo();
}

const item &item::first_ammo() const
{
    return contents.first_ammo();
}

bool item::ammo_sufficient( const Character *carrier, int qty ) const
{
    map &here = get_map();

    if( count_by_charges() ) {
        return ammo_remaining_linked( here, carrier ) >= qty;
    }

    if( is_comestible() ) {
        return true;
    }

    return shots_remaining( here, carrier ) >= qty;
}

bool item::ammo_sufficient( const Character *carrier, const std::string &method, int qty ) const
{
    auto iter = type->ammo_scale.find( method );
    if( iter != type->ammo_scale.end() ) {
        qty *= iter->second;
    }

    return ammo_sufficient( carrier, qty );
}

int item::ammo_consume( int qty, const tripoint_bub_ms &pos, Character *carrier )
{
    return item::ammo_consume( qty, get_map(), pos, carrier );
}

int item::ammo_consume( int qty, map &here, const tripoint_bub_ms &pos, Character *carrier )
{
    if( qty < 0 ) {
        debugmsg( "Cannot consume negative quantity of ammo for %s", tname() );
        return 0;
    }
    const int wanted_qty = qty;
    const bool is_tool_with_carrier = carrier != nullptr && is_tool();

    if( is_tool_with_carrier && has_flag( flag_USES_NEARBY_AMMO ) ) {
        const ammotype ammo = ammo_type();
        if( !ammo.is_null() ) {
            const inventory &carrier_inventory = carrier->crafting_inventory( &here );
            itype_id ammo_type = ammo->default_ammotype();
            const int charges_avalable = carrier_inventory.charges_of( ammo_type, INT_MAX );

            qty = std::min( wanted_qty, charges_avalable );

            std::vector<item_comp> components;
            components.emplace_back( ammo_type, qty );
            carrier->consume_items( components, 1 );
            return wanted_qty - qty;
        }
    }

    // Consume power from appliances/vehicles connected with cables
    if( has_link_data() ) {
        if( link().t_veh && link().efficiency >= MIN_LINK_EFFICIENCY ) {
            qty = link().t_veh->discharge_battery( here, qty, true );
        } else {
            const optional_vpart_position vp = here.veh_at( link().t_abs_pos );
            if( vp ) {
                qty = vp->vehicle().discharge_battery( here, qty, true );
            }
        }
    }

    // Consume charges loaded in the item or its magazines
    if( is_magazine() || uses_magazine() ) {
        qty -= contents.ammo_consume( qty, &here, pos );
        if( ammo_capacity( ammo_battery ) == 0 && carrier != nullptr ) {
            carrier->invalidate_weight_carried_cache();
        }
    }

    // Dirty fix: activating a container of meds leads here, and used to use up all of the charges.
    if( is_medication() && charges > 1 ) {
        int charg_used = std::min( charges, qty );
        charges -= charg_used;
        qty -= charg_used;
    }

    // Some weird internal non-item charges (used by grenades)
    if( is_tool() && type->tool->ammo_id.empty() ) {
        int charg_used = std::min( charges, qty );
        charges -= charg_used;
        qty -= charg_used;
    }

    bool is_off_grid_powered = has_flag( flag_USE_UPS ) || has_flag( flag_USES_BIONIC_POWER );

    // Modded tools can consume UPS/bionic energy instead of ammo.
    // Guns handle energy in energy_consume()
    if( is_tool_with_carrier && is_off_grid_powered ) {
        units::energy wanted_energy = units::from_kilojoule( static_cast<std::int64_t>( qty ) );

        if( has_flag( flag_USE_UPS ) ) {
            wanted_energy -= carrier->consume_ups( wanted_energy );
        }

        if( has_flag( flag_USES_BIONIC_POWER ) ) {
            units::energy bio_used = std::min( carrier->get_power_level(), wanted_energy );
            carrier->mod_power_level( -bio_used );
            wanted_energy -= bio_used;
        }

        // It is possible for this to cause rounding error due to different precision of energy
        // But that can happen only if there was not enough ammo and you shouldn't be able to use without sufficient ammo
        qty = units::to_kilojoule( wanted_energy );
    }
    return wanted_qty - qty;
}

units::energy item::energy_consume( units::energy qty, const tripoint_bub_ms &pos,
                                    Character *carrier,
                                    float fuel_efficiency )
{
    return item::energy_consume( qty, &get_map(), pos, carrier, fuel_efficiency );
}

units::energy item::energy_consume( units::energy qty, map *here, const tripoint_bub_ms &pos,
                                    Character *carrier,
                                    float fuel_efficiency )
{
    if( qty < 0_kJ ) {
        debugmsg( "Cannot consume negative quantity of energy for %s", tname() );
        return 0_kJ;
    }

    const units::energy wanted_energy = qty;

    // Consume battery(ammo) and other fuel (if allowed)
    if( is_battery() || fuel_efficiency >= 0 ) {
        int consumed_kj = contents.ammo_consume( units::to_kilojoule( qty ), here, pos, fuel_efficiency );
        qty -= units::from_kilojoule( static_cast<std::int64_t>( consumed_kj ) );
        // Either we're out of juice or truncating the value above means we didn't drain quite enough.
        // In the latter case at least this will bump up energy enough to satisfy the remainder,
        // if not it will drain the item all the way.
        // TODO: reconsider what happens with fuel burning, right now this stashes
        // the remainder of energy from burning the fuel in the item in question,
        // which potentially allows it to burn less fuel next time.
        // Do we want an implicit 1kJ battery in the generator to smooth things out?
        if( qty > energy ) {
            int64_t residual_drain = contents.ammo_consume( 1, here, pos, fuel_efficiency );
            energy += units::from_kilojoule( residual_drain );
        }
        if( qty > energy ) {
            qty -= energy;
            energy = 0_J;
        } else {
            energy -= qty;
            qty = 0_J;
        }
    }

    // Consume energy from contained magazine
    if( magazine_current() ) {
        qty -= magazine_current()->energy_consume( qty, here, pos, carrier );
    }

    // Consume UPS energy from various sources
    if( carrier != nullptr && has_flag( flag_USE_UPS ) ) {
        qty -= carrier->consume_ups( qty );
    }

    // Consume bio energy
    if( carrier != nullptr && has_flag( flag_USES_BIONIC_POWER ) ) {
        units::energy bio_used = std::min( carrier->get_power_level(), qty );
        carrier->mod_power_level( -bio_used );
        qty -= bio_used;
    }

    return wanted_energy - qty;
}

int item::activation_consume( int qty, const tripoint_bub_ms &pos, Character *carrier )
{
    return ammo_consume( qty * ammo_required(), pos, carrier );
}

bool item::has_ammo() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->has_ammo();
    }

    if( is_ammo() ) {
        return true;
    }

    if( is_magazine() ) {
        return !contents.empty() && contents.first_ammo().has_ammo();
    }

    auto mods = is_gun() ? gunmods() : toolmods();
    for( const item *e : mods ) {
        if( !e->type->mod->ammo_modifier.empty() && e->has_ammo() ) {
            return true;
        }
    }

    return false;
}

bool item::has_ammo_data() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->has_ammo_data();
    }

    if( is_ammo() ) {
        return true;
    }

    if( is_magazine() ) {
        return !contents.empty() && contents.first_ammo().has_ammo_data();
    }

    auto mods = is_gun() ? gunmods() : toolmods();
    for( const item *e : mods ) {
        if( !e->type->mod->ammo_modifier.empty() && e->has_ammo_data() ) {
            return true;
        }
    }

    return false;
}

const itype *item::ammo_data() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->ammo_data();
    }

    if( is_ammo() ) {
        return type;
    }

    if( is_magazine() ) {
        return !contents.empty() ? contents.first_ammo().ammo_data() : nullptr;
    }

    auto mods = is_gun() ? gunmods() : toolmods();
    for( const item *e : mods ) {
        if( !e->type->mod->ammo_modifier.empty() && !e->ammo_current().is_null() &&
            item_controller->has_template( e->ammo_current() ) ) {
            return item_controller->find_template( e->ammo_current() );
        }
    }

    return nullptr;
}

itype_id item::ammo_current() const
{
    const itype *ammo = ammo_data();
    if( ammo ) {
        return ammo->get_id();
    }
    if( is_tool() && has_flag( flag_USES_NEARBY_AMMO ) ) {
        return ammo_default();
    }

    return itype_id::NULL_ID();
}

const item &item::loaded_ammo() const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->loaded_ammo();
    }

    if( is_magazine() ) {
        return !contents.empty() ? contents.first_ammo() : null_item_reference();
    }

    auto mods = is_gun() ? gunmods() : toolmods();
    for( const item *e : mods ) {
        const item &mod_ammo = e->loaded_ammo();
        if( !mod_ammo.is_null() ) {
            return mod_ammo;
        }
    }

    if( is_gun() && ammo_remaining( ) != 0 ) {
        return contents.first_ammo();
    }
    return null_item_reference();
}

std::set<ammotype> item::ammo_types( bool conversion ) const
{
    if( conversion ) {
        const std::vector<const item *> &mods = is_gun() ? gunmods() : toolmods();
        for( const item *e : mods ) {
            if( !e->type->mod->ammo_modifier.empty() ) {
                return e->type->mod->ammo_modifier;
            }
        }
    }

    if( is_gun() ) {
        return type->gun->ammo;
    }

    if( is_tool() && has_flag( flag_USES_NEARBY_AMMO ) ) {
        return type->tool->ammo_id;
    }

    return contents.ammo_types();
}

ammotype item::ammo_type() const
{
    if( is_ammo() ) {
        return type->ammo->type;
    }

    if( is_tool() && has_flag( flag_USES_NEARBY_AMMO ) ) {
        const std::set<ammotype> ammo_type_choices = ammo_types();
        if( !ammo_type_choices.empty() ) {
            return *ammo_type_choices.begin();
        }
    }

    return ammotype::NULL_ID();
}

itype_id item::ammo_default( bool conversion ) const
{
    if( !ammo_types( conversion ).empty() ) {
        itype_id res = ammotype( *ammo_types( conversion ).begin() )->default_ammotype();
        if( !res.is_empty() ) {
            return res;
        }
    }

    return itype_id::NULL_ID();
}

std::string item::print_ammo( ammotype at, const item *gun ) const
{
    const item *mag = magazine_current();
    if( mag ) {
        return mag->print_ammo( at, this );
    } else if( has_flag( flag_USES_BIONIC_POWER ) ) {
        return _( "energy" );
    }

    // for magazines if we have a gun in mind display damage as well
    if( type->magazine ) {
        if( gun ) {
            return enumerate_as_string( type->magazine->cached_ammos[at],
            [gun]( const itype_id & id ) {
                return string_format( "<info>%s</info>(%d)", id->nname( 1 ),
                                      static_cast<int>( gun->gun_damage( id ).total_damage() ) );
            } );
        } else {
            return enumerate_as_string( type->magazine->cached_ammos[at],
            []( const itype_id & id ) {
                return string_format( "<info>%s</info>", id->nname( 1 ) );
            } );
        }
    }

    if( type->gun ) {
        return enumerate_as_string( type->gun->cached_ammos[at],
        [this]( const itype_id & id ) {
            return string_format( "<info>%s(%d)</info>", id->nname( 1 ),
                                  static_cast<int>( gun_damage( id ).total_damage() ) );
        } );
    }

    return "";
}

itype_id item::common_ammo_default( bool conversion ) const
{
    for( const ammotype &at : ammo_types( conversion ) ) {
        const item *mag = magazine_current();
        if( mag && mag->type->magazine->type.count( at ) ) {
            itype_id res = at->default_ammotype();
            if( !res.is_empty() ) {
                return res;
            }
        }
    }
    return itype_id::NULL_ID();
}

std::set<ammo_effect_str_id> item::ammo_effects( bool with_ammo ) const
{
    if( !is_gun() ) {
        return std::set<ammo_effect_str_id>();
    }

    std::set<ammo_effect_str_id> res = type->gun->ammo_effects;
    if( with_ammo && has_ammo() ) {
        res.insert( ammo_data()->ammo->ammo_effects.begin(), ammo_data()->ammo->ammo_effects.end() );
    }

    for( const item *mod : gunmods() ) {
        res.insert( mod->type->gunmod->ammo_effects.begin(), mod->type->gunmod->ammo_effects.end() );
    }

    return res;
}

std::string item::ammo_sort_name() const
{
    if( is_magazine() || is_gun() || is_tool() ) {
        const std::set<ammotype> &types = ammo_types();
        if( !types.empty() ) {
            return ammotype( *types.begin() )->name();
        }
    }
    if( is_ammo() ) {
        return ammo_type()->name();
    }
    return "";
}

bool item::magazine_integral() const
{
    return contents.has_pocket_type( pocket_type::MAGAZINE );
}

bool item::uses_magazine() const
{
    return contents.has_pocket_type( pocket_type::MAGAZINE_WELL );
}

itype_id item::magazine_default( bool conversion ) const
{
    // consider modded ammo types
    itype_id ammo;
    if( conversion && ( ammo = ammo_default(), !ammo.is_null() ) ) {
        for( const itype_id &mag : contents.magazine_compatible() ) {
            auto mag_types = mag->magazine->type;
            if( mag_types.find( ammo->ammo->type ) != mag_types.end() ) {
                return mag;
            }
        }
    }

    // otherwise return the default
    return contents.magazine_default();
}

std::set<itype_id> item::magazine_compatible() const
{
    return contents.magazine_compatible();
}

item *item::magazine_current()
{
    return contents.magazine_current();
}

const item *item::magazine_current() const
{
    return const_cast<item *>( this )->magazine_current();
}

std::vector<item *> item::gunmods()
{
    return contents.gunmods();
}

std::vector<const item *> item::gunmods() const
{
    return contents.gunmods();
}

std::vector<const item *> item::mods() const
{
    return contents.mods();
}

std::vector<const item *> item::cables() const
{
    return contents.cables();
}

item *item::gunmod_find( const itype_id &mod )
{
    std::vector<item *> mods = gunmods();
    auto it = std::find_if( mods.begin(), mods.end(), [&mod]( item * e ) {
        return e->typeId() == mod;
    } );
    return it != mods.end() ? *it : nullptr;
}

const item *item::gunmod_find( const itype_id &mod ) const
{
    return const_cast<item *>( this )->gunmod_find( mod );
}

item *item::gunmod_find_by_flag( const flag_id &flag )
{
    std::vector<item *> mods = gunmods();
    auto it = std::find_if( mods.begin(), mods.end(), [&flag]( item * e ) {
        return e->has_flag( flag );
    } );
    return it != mods.end() ? *it : nullptr;
}

ret_val<void> item::is_gunmod_compatible( const item &mod ) const
{
    if( !mod.is_gunmod() ) {
        debugmsg( "Tried checking compatibility of non-gunmod" );
        return ret_val<void>::make_failure();
    }
    static const gun_type_type pistol_gun_type( translate_marker_context( "gun_type_type", "pistol" ) );

    if( !is_gun() ) {
        return ret_val<void>::make_failure( _( "isn't a weapon" ) );

    } else if( is_gunmod() ) {
        return ret_val<void>::make_failure( _( "is a gunmod and cannot be modded" ) );

    } else if( gunmod_find( mod.typeId() ) ) {
        return ret_val<void>::make_failure( _( "already has a %s" ), mod.tname( 1 ) );

    } else if( !get_mod_locations().count( mod.type->gunmod->location ) ) {
        return ret_val<void>::make_failure( _( "doesn't have a slot for this mod" ) );

    } else if( get_free_mod_locations( mod.type->gunmod->location ) <= 0 ) {
        return ret_val<void>::make_failure( _( "doesn't have enough room for another %s mod" ),
                                            mod.type->gunmod->location.name() );

    } else if( !mod.type->gunmod->usable.count( gun_type() ) &&
               !mod.type->gunmod->usable.count( gun_type_type( typeId().str() ) ) ) {
        return ret_val<void>::make_failure( _( "cannot have a %s" ), mod.tname() );

    } else if( typeId() == itype_hand_crossbow &&
               !mod.type->gunmod->usable.count( pistol_gun_type ) ) {
        return ret_val<void>::make_failure( _( "isn't big enough to use that mod" ) );

    } else if( mod.type->gunmod->location.str() == "underbarrel" &&
               !mod.has_flag( flag_PUMP_RAIL_COMPATIBLE ) && has_flag( flag_PUMP_ACTION ) ) {
        return ret_val<void>::make_failure( _( "can only accept small mods on that slot" ) );

    } else if( mod.typeId() == itype_waterproof_gunmod && has_flag( flag_WATERPROOF_GUN ) ) {
        return ret_val<void>::make_failure( _( "is already waterproof" ) );

    } else if( mod.typeId() == itype_tuned_mechanism && has_flag( flag_NEVER_JAMS ) ) {
        return ret_val<void>::make_failure( _( "is already eminently reliable" ) );

    } else if( mod.typeId() == itype_brass_catcher && has_flag( flag_RELOAD_EJECT ) ) {
        return ret_val<void>::make_failure( _( "cannot have a brass catcher" ) );

    } else if( ( mod.type->gunmod->location.name() == "magazine" ||
                 mod.type->gunmod->location.name() == "mechanism" ||
                 mod.type->gunmod->location.name() == "loading port" ||
                 mod.type->gunmod->location.name() == "bore" ) &&
               ( ammo_remaining( ) > 0 || magazine_current() ) ) {
        return ret_val<void>::make_failure( _( "must be unloaded before installing this mod" ) );

    } else if( gunmod_find( itype_stock_none ) &&
               mod.type->gunmod->location.name() == "stock accessory" ) {
        return ret_val<void>::make_failure( _( "doesn't have a stock to attach this mod" ) );

    } else if( mod.typeId() == itype_arredondo_chute ) {
        return ret_val<void>::make_failure( _( "chute needs modification before attaching" ) );

        // Acceptable_ammo check is kinda weird now, if it is passed, checks after it will be ignored.
        // Moved it here as a workaround.
    } else if( !mod.type->mod->acceptable_ammo.empty() ) {
        bool compat_ammo = false;
        for( const ammotype &at : mod.type->mod->acceptable_ammo ) {
            if( ammo_types( false ).count( at ) ) {
                compat_ammo = true;
            }
        }
        if( !compat_ammo ) {
            return ret_val<void>::make_failure(
                       _( "%1$s cannot be used on item with no compatible ammo types" ), mod.tname( 1 ) );
        }
    }

    for( const gunmod_location &slot : mod.type->gunmod->blacklist_slot ) {
        if( get_mod_locations().count( slot ) ) {
            return ret_val<void>::make_failure( _( "cannot be installed on a weapon with a \"%s\"" ),
                                                slot.name() );
        }
    }

    for( const itype_id &testmod : mod.type->gunmod->blacklist_mod ) {
        if( gunmod_find( testmod ) ) {
            return ret_val<void>::make_failure( _( "cannot be installed on a weapon with a \"%s\"" ),
                                                ( testmod->nname( 1 ) ) );
        }
    }

    return ret_val<void>::make_success();
}

std::map<gun_mode_id, gun_mode> item::gun_all_modes() const
{
    std::map<gun_mode_id, gun_mode> res;

    if( !is_gun() && !is_gunmod() ) {
        return res;
    }

    std::vector<const item *> opts = gunmods();
    opts.push_back( this );

    for( const item *e : opts ) {

        // handle base item plus any auxiliary gunmods
        if( e->is_gun() ) {
            for( const std::pair<const gun_mode_id, gun_modifier_data> &m : e->type->gun->modes ) {
                // prefix attached gunmods, e.g. M203_DEFAULT to avoid index key collisions
                std::string prefix = e->is_gunmod() ? ( std::string( e->typeId() ) += "_" ) : "";
                std::transform( prefix.begin(), prefix.end(), prefix.begin(),
                                static_cast<int( * )( int )>( toupper ) );

                const int qty = m.second.qty();

                res.emplace( gun_mode_id( prefix + m.first.str() ), gun_mode( m.second.name(),
                             const_cast<item *>( e ),
                             qty, m.second.flags() ) );
            }

            // non-auxiliary gunmods may provide additional modes for the base item
        } else if( e->is_gunmod() ) {
            for( const std::pair<const gun_mode_id, gun_modifier_data> &m : e->type->gunmod->mode_modifier ) {
                //checks for melee gunmod, points to gunmod
                if( m.first == gun_mode_REACH ) {
                    res.emplace( m.first, gun_mode { m.second.name(), const_cast<item *>( e ),
                                                     m.second.qty(), m.second.flags() } );
                    //otherwise points to the parent gun, not the gunmod
                } else {
                    res.emplace( m.first, gun_mode { m.second.name(), const_cast<item *>( this ),
                                                     m.second.qty(), m.second.flags() } );
                }
            }
        }
    }

    return res;
}

gun_mode item::gun_get_mode( const gun_mode_id &mode ) const
{
    if( is_gun() ) {
        for( const std::pair<const gun_mode_id, gun_mode> &e : gun_all_modes() ) {
            if( e.first == mode ) {
                return e.second;
            }
        }
    }
    return gun_mode();
}

gun_mode item::gun_current_mode() const
{
    return gun_get_mode( gun_get_mode_id() );
}

gun_mode_id item::gun_get_mode_id() const
{
    if( !is_gun() || is_gunmod() ) {
        return gun_mode_id();
    }
    return gun_mode_id( get_var( GUN_MODE_VAR_NAME, "DEFAULT" ) );
}

bool item::gun_set_mode( const gun_mode_id &mode )
{
    if( !is_gun() || is_gunmod() || !gun_all_modes().count( mode ) ) {
        return false;
    }
    set_var( GUN_MODE_VAR_NAME, mode.str() );
    return true;
}

void item::gun_cycle_mode()
{
    if( !is_gun() || is_gunmod() ) {
        return;
    }

    const gun_mode_id cur = gun_get_mode_id();
    const std::map<gun_mode_id, gun_mode> modes = gun_all_modes();

    for( auto iter = modes.begin(); iter != modes.end(); ++iter ) {
        if( iter->first == cur ) {
            if( std::next( iter ) == modes.end() ) {
                break;
            }
            gun_set_mode( std::next( iter )->first );
            return;
        }
    }
    gun_set_mode( modes.begin()->first );
}

item::reload_option::reload_option( const reload_option & ) = default;

item::reload_option &item::reload_option::operator=( const reload_option & ) = default;

item::reload_option::reload_option( const Character *who, const item_location &target,
                                    const item_location &ammo ) :
    who( who ), target( target ), ammo( ammo )
{
    if( this->target->is_ammo_belt() && this->target->type->magazine->linkage ) {
        max_qty = this->who->charges_of( * this->target->type->magazine->linkage );
    }
    qty( max_qty );
}

int item::reload_option::moves() const
{
    int mv = ammo.obtain_cost( *who, qty() ) + who->item_reload_cost( *target, *ammo, qty() );
    if( target.has_parent() ) {
        item_location parent = target.parent_item();
        if( parent->is_gun() && !target->is_gunmod() ) {
            mv += parent->get_reload_time() * 1.5;
        } else if( parent->is_tool() ) {
            mv += 100;
        }
    }
    return mv;
}

void item::reload_option::qty( int val )
{
    bool ammo_in_container = ammo->is_ammo_container();
    bool ammo_in_liquid_container = ammo->is_watertight_container();
    item &ammo_obj = ( ammo_in_container || ammo_in_liquid_container ) ?
                     // this is probably not the right way to do this. there is no guarantee whatsoever that ammo_obj will be an ammo
                     *ammo->contents.all_items_top( pocket_type::CONTAINER ).front() : *ammo;

    if( ( ammo_in_container && !ammo_obj.is_ammo() ) ||
        ( ammo_in_liquid_container && !ammo_obj.made_of( phase_id::LIQUID ) ) ) {
        debugmsg( "Invalid reload option: %s", ammo_obj.tname() );
        return;
    }

    // Checking ammo capacity implicitly limits guns with removable magazines to capacity 0.
    // This gets rounded up to 1 later.
    int remaining_capacity = target->is_watertight_container() ?
                             target->get_remaining_capacity_for_liquid( ammo_obj, true ) :
                             target->remaining_ammo_capacity();
    if( target->has_flag( flag_RELOAD_ONE ) &&
        !( ammo->has_flag( flag_SPEEDLOADER ) || ammo->has_flag( flag_SPEEDLOADER_CLIP ) ) ) {
        remaining_capacity = 1;
    }
    if( ammo_obj.type->ammo ) {
        if( ammo_obj.ammo_type() == ammo_plutonium ) {
            remaining_capacity = remaining_capacity / PLUTONIUM_CHARGES +
                                 ( remaining_capacity % PLUTONIUM_CHARGES != 0 );
        }
    }

    bool ammo_by_charges = ammo_obj.is_ammo() || ammo_in_liquid_container;
    int available_ammo;
    if( ammo->has_flag( flag_SPEEDLOADER ) || ammo->has_flag( flag_SPEEDLOADER_CLIP ) ) {
        available_ammo = ammo_obj.ammo_remaining( );
    } else {
        available_ammo = ammo_by_charges ? ammo_obj.charges : ammo_obj.count();
    }
    // constrain by available ammo, target capacity and other external factors (max_qty)
    // @ref max_qty is currently set when reloading ammo belts and limits to available linkages
    qty_ = std::min( { val, available_ammo, remaining_capacity, max_qty } );

    // always expect to reload at least one charge
    qty_ = std::max( qty_, 1 );

}

int item::casings_count() const
{
    int res = 0;

    const_cast<item *>( this )->casings_handle( [&res]( item & ) {
        ++res;
        return false;
    } );

    return res;
}

void item::casings_handle( const std::function<bool( item & )> &func )
{
    if( !is_gun() && !is_tool() ) {
        return;
    }
    contents.casings_handle( func );
}

bool item::reload( Character &u, item_location ammo, int qty )
{
    if( qty <= 0 ) {
        debugmsg( "Tried to reload zero or less charges" );
        return false;
    }

    if( !ammo ) {
        debugmsg( "Tried to reload using non-existent ammo" );
        return false;
    }

    if( !can_reload_with( *ammo.get_item(), true ) ) {
        return false;
    }

    bool ammo_from_map = !ammo.held_by( u );
    if( ammo->has_flag( flag_SPEEDLOADER ) || ammo->has_flag( flag_SPEEDLOADER_CLIP ) ) {
        // if the thing passed in is a speed loader, we want the ammo
        ammo = item_location( ammo, &ammo->first_ammo() );
    }

    // limit quantity of ammo loaded to remaining capacity
    int limit = 0;
    if( is_watertight_container() && ammo->made_of_from_type( phase_id::LIQUID ) ) {
        limit = get_remaining_capacity_for_liquid( *ammo );
    } else if( ammo->is_ammo() ) {
        limit = ammo_capacity( ammo->ammo_type() ) - ammo_remaining( );
    }

    if( ammo->ammo_type() == ammo_plutonium ) {
        limit = limit / PLUTONIUM_CHARGES + ( limit % PLUTONIUM_CHARGES != 0 );
    }

    qty = std::min( qty, limit );

    casings_handle( [&u]( item & e ) {
        return u.i_add_or_drop( e );
    } );

    if( is_magazine() ) {
        qty = std::min( qty, ammo->charges );

        if( is_ammo_belt() && type->magazine->linkage ) {
            if( !u.use_charges_if_avail( *type->magazine->linkage, qty ) ) {
                debugmsg( "insufficient linkages available when reloading ammo belt" );
            }
        }

        item item_copy( *ammo );
        ammo->charges -= qty;

        if( ammo->ammo_type() == ammo_plutonium ) {
            // any excess is wasted rather than overfilling the item
            item_copy.charges = std::min( qty * PLUTONIUM_CHARGES, ammo_capacity( ammo_plutonium ) );
        } else {
            item_copy.charges = qty;
        }

        put_in( item_copy, pocket_type::MAGAZINE );

    } else if( is_watertight_container() && ammo->made_of_from_type( phase_id::LIQUID ) ) {
        item contents( *ammo );
        fill_with( contents, qty );
        if( ammo.has_parent() ) {
            ammo.parent_item()->contained_where( *ammo.get_item() )->on_contents_changed();
        }
        ammo->charges -= qty;
    } else {
        item magazine_removed;
        bool allow_wield = false;
        // if we already have a magazine loaded prompt to eject it
        if( magazine_current() ) {
            // we don't want to replace wielded `ammo` with ejected magazine
            // as it will invalidate `ammo`. Also keep wielding the item being reloaded.
            allow_wield = ( !u.is_wielding( *ammo ) && !u.is_wielding( *this ) );
            // Defer placing the magazine into inventory until new magazine is installed.
            magazine_removed = *magazine_current();
            remove_item( *magazine_current() );
        }

        put_in( *ammo, pocket_type::MAGAZINE_WELL );
        ammo.remove_item();
        if( ammo_from_map ) {
            u.invalidate_weight_carried_cache();
        }
        if( !magazine_removed.is_null() ) {
            u.i_add( magazine_removed, true, nullptr, nullptr, true, allow_wield );
        }
        return true;
    }

    if( ammo->charges == 0 ) {
        ammo.remove_item();
    }
    if( ammo_from_map ) {
        u.invalidate_weight_carried_cache();
    }
    return true;
}

bool item::detonate( const tripoint_bub_ms &p, std::vector<item> &drops )
{
    const Creature *source = get_player_character().get_faction()->id == owner
                             ? &get_player_character()
                             : nullptr;
    if( type->explosion.power >= 0 ) {
        explosion_handler::explosion( source, p, type->explosion );
        return true;
    } else if( type->ammo && ( type->ammo->special_cookoff || type->ammo->cookoff ) ) {
        int charges_remaining = charges;
        const int rounds_exploded = rng( 1, charges_remaining / 2 );
        if( type->ammo->special_cookoff ) {
            // If it has a special effect just trigger it.
            apply_ammo_effects( nullptr, p, type->ammo->ammo_effects, 1 );
        }
        if( type->ammo->cookoff ) {
            // If ammo type can burn, then create an explosion proportional to quantity.
            float power = 3.0f * std::pow( rounds_exploded / 25.0f, 0.25f );
            explosion_handler::explosion( nullptr, p, power, 0.0f, false, 0 );
        }
        charges_remaining -= rounds_exploded;
        if( charges_remaining > 0 ) {
            item temp_item = *this;
            temp_item.charges = charges_remaining;
            drops.push_back( temp_item );
        }

        return true;
    }

    return false;
}

item::link_data &item::link()
{
    if( link_ ) {
        return *link_;
    }
    if( !can_link_up() ) {
        debugmsg( "%s is creating link_data even though it doesn't have a link_up action!  can_link_up() should be checked before using link().",
                  tname() );
    }
    return *( link_ = cata::make_value<item::link_data>() );
}

const item::link_data &item::link() const
{
    if( link_ ) {
        return *link_;
    }
    if( !can_link_up() ) {
        debugmsg( "%s is creating link_data even though it doesn't have a link_up action!  can_link_up() should be checked before using link().",
                  tname() );
    }
    return *( link_ = cata::make_value<item::link_data>() );
}

bool item::has_link_data() const
{
    return !!link_;
}

bool item::can_link_up() const
{
    return has_link_data() || type->can_use( "link_up" );
}

bool item::link_has_state( link_state state ) const
{
    return !has_link_data() ? state == link_state::no_link :
           link_->source == state || link_->target == state;
}

bool item::link_has_states( link_state source, link_state target ) const
{
    return !has_link_data() ? source == link_state::no_link && target == link_state::no_link :
           ( link_->source == source || link_->source == link_state::automatic ) &&
           ( link_->target == target || link_->target == link_state::automatic );
}

bool item::has_no_links() const
{
    return !has_link_data() ? true :
           link_->source == link_state::no_link && link_->target == link_state::no_link;
}

std::string item::link_name() const
{
    return has_flag( flag_CABLE_SPOOL ) ? label( 1 ) : string_format( _( " %s's cable" ), label( 1 ) );
}

ret_val<void> item::link_to( const optional_vpart_position &linked_vp, link_state link_type )
{
    return linked_vp ? link_to( linked_vp->vehicle(), linked_vp->mount_pos(), link_type ) :
           ret_val<void>::make_failure();
}

ret_val<void> item::link_to( const optional_vpart_position &first_linked_vp,
                             const optional_vpart_position &second_linked_vp, link_state link_type )
{
    if( !first_linked_vp || !second_linked_vp ) {
        return ret_val<void>::make_failure();
    }
    // Link up the second vehicle first so, if it's a tow cable, the first vehicle will tow the second.
    ret_val<void> result = link_to( second_linked_vp->vehicle(), second_linked_vp->mount_pos(),
                                    link_type );
    if( !result.success() ) {
        return result;
    }
    return link_to( first_linked_vp->vehicle(), first_linked_vp->mount_pos(), link_type );
}

ret_val<void> item::link_to( vehicle &veh, const point_rel_ms &mount, link_state link_type )
{
    map &here = get_map();

    if( !can_link_up() ) {
        return ret_val<void>::make_failure( _( "The %s doesn't have a cable!" ), type_name() );
    }

    // First, check if the desired link is actually possible.

    if( link_type == link_state::vehicle_tow ) {
        if( veh.has_tow_attached() || veh.is_towed() || veh.is_towing() ) {
            return ret_val<void>::make_failure( _( "That vehicle already has a tow-line attached." ) );
        } else if( !veh.is_external_part( mount ) ) {
            return ret_val<void>::make_failure( _( "You can't attach a tow-line to an internal part." ) );
        } else {
            const int part_at = veh.part_at( veh.coord_translate( mount ) );
            if( part_at != -1 && !veh.part( part_at ).carried_stack.empty() ) {
                return ret_val<void>::make_failure( _( "You can't attach a tow-line to a racked part." ) );
            }
        }
    } else {
        const link_up_actor *it_actor = static_cast<const link_up_actor *>
                                        ( get_use( "link_up" )->get_actor_ptr() );
        bool can_link_port = ( link_type == link_state::vehicle_port ||
                               link_type == link_state::automatic ) &&
                             it_actor->targets.find( link_state::vehicle_port ) != it_actor->targets.end() &&
                             veh.avail_linkable_part( mount, true ) != -1;

        bool can_link_battery = ( link_type == link_state::vehicle_battery ||
                                  link_type == link_state::automatic ) &&
                                it_actor->targets.find( link_state::vehicle_battery ) != it_actor->targets.end() &&
                                veh.avail_linkable_part( mount, false ) != -1;

        if( !can_link_port && !can_link_battery ) {
            return ret_val<void>::make_failure( _( "The %s can't connect to that." ), type_name() );
        } else if( link_type == link_state::automatic ) {
            link_type = can_link_port ? link_state::vehicle_port : link_state::vehicle_battery;
        }
    }

    bool bio_link = link_has_states( link_state::no_link, link_state::bio_cable );
    if( bio_link || has_no_links() ) {

        // No link exists, so start a new link to a vehicle/appliance.

        link().source = bio_link ? link_state::bio_cable : link_state::no_link;
        link().target = link_type;
        link().t_veh = veh.get_safe_reference();
        link().t_abs_pos = link().t_veh->pos_abs();
        link().t_mount = mount;
        link().s_bub_pos = tripoint_bub_ms::min; // Forces the item to check the length during process_link.

        update_link_traits();
        return ret_val<void>::make_success();
    }

    // There's already a link, so connect the previous vehicle/appliance to the new one.

    if( !link_has_state( link_state::no_link ) ) {
        debugmsg( "Failed to connect the %s, both ends are already connected!", tname() );
        return ret_val<void>::make_failure();
    }
    if( !link().t_veh ) {
        vehicle *found_veh = vehicle::find_vehicle( here, link().t_abs_pos );
        if( found_veh ) {
            link().t_veh = found_veh->get_safe_reference();
        } else {
            debugmsg( "Failed to connect the %s, it lost its vehicle pointer!", tname() );
            return ret_val<void>::make_failure();
        }
    }
    vehicle *prev_veh = link().t_veh.get();
    if( prev_veh == &veh ) {
        return ret_val<void>::make_failure( _( "You cannot connect the %s to itself." ), prev_veh->name );
    }

    // Prepare target tripoints for the cable parts that'll be added to the selected/previous vehicles
    const std::pair<tripoint_abs_ms, tripoint_abs_ms> prev_part_target = std::make_pair(
                veh.pos_abs() + veh.coord_translate( mount ),
                veh.pos_abs() );
    const std::pair<tripoint_abs_ms, tripoint_abs_ms> sel_part_target = std::make_pair(
                link().t_abs_pos + prev_veh->coord_translate( link().t_mount ),
                link().t_abs_pos );

    for( const vpart_reference &vpr : prev_veh->get_any_parts( VPFLAG_POWER_TRANSFER ) ) {
        if( vpr.part().target.first == prev_part_target.first &&
            vpr.part().target.second == prev_part_target.second ) {
            return ret_val<void>::make_failure( _( "The %1$s and %2$s are already connected." ),
                                                veh.name, prev_veh->name );
        }
    }

    if( rl_dist( prev_part_target.first, sel_part_target.first ) > link().max_length ) {
        return ret_val<void>::make_failure( _( "The %s can't stretch that far!" ), type_name() );
    }

    const itype_id item_id = typeId();
    vpart_id vpid = vpart_id::NULL_ID();
    for( const vpart_info &e : vehicles::parts::get_all() ) {
        if( e.base_item == item_id ) {
            vpid = e.id;
            break;
        }
    }

    if( vpid.is_null() ) {
        debugmsg( "item %s is not base item of any vehicle part!", item_id.c_str() );
        return ret_val<void>::make_failure();
    }

    const point_rel_ms prev_mount = link().t_mount;

    const ret_val<void> can_mount1 = prev_veh->can_mount( prev_mount, *vpid );
    if( !can_mount1.success() ) {
        //~ %1$s - cable name, %2$s - the reason why it failed
        return ret_val<void>::make_failure( _( "You can't attach the %1$s: %2$s" ),
                                            type_name(), can_mount1.str() );
    }
    const ret_val<void> can_mount2 = veh.can_mount( mount, *vpid );
    if( !can_mount2.success() ) {
        //~ %1$s - cable name, %2$s - the reason why it failed
        return ret_val<void>::make_failure( _( "You can't attach the %1$s: %2$s" ),
                                            type_name(), can_mount2.str() );
    }

    vehicle_part prev_veh_part( vpid, item( *this ) );
    prev_veh_part.target.first = prev_part_target.first;
    prev_veh_part.target.second = prev_part_target.second;
    prev_veh->install_part( here, prev_mount, std::move( prev_veh_part ) );
    prev_veh->precalc_mounts( 1, prev_veh->pivot_rotation[1], prev_veh->pivot_anchor[1] );

    vehicle_part sel_veh_part( vpid, item( *this ) );
    sel_veh_part.target.first = sel_part_target.first;
    sel_veh_part.target.second = sel_part_target.second;
    veh.install_part( here, mount, std::move( sel_veh_part ) );
    veh.precalc_mounts( 1, veh.pivot_rotation[1], veh.pivot_anchor[1] );

    if( link_type == link_state::vehicle_tow ) {
        if( link().source == link_state::vehicle_tow ) {
            veh.tow_data.set_towing( prev_veh, &veh ); // Previous vehicle is towing.
        } else {
            veh.tow_data.set_towing( &veh, prev_veh ); // Previous vehicle is being towed.
        }
    }

    if( typeId() != itype_power_cord ) {
        // Remove linked_flag from attached parts - the just-added cable vehicle parts do the same thing.
        reset_link();
    }
    //~ %1$s - first vehicle name, %2$s - second vehicle name - %3$s - cable name,
    return ret_val<void>::make_success( _( "You connect the %1$s and %2$s with the %3$s." ),
                                        prev_veh->disp_name(), veh.disp_name(), type_name() );
}

void item::update_link_traits()
{
    if( !can_link_up() ) {
        return;
    }

    const link_up_actor *it_actor = static_cast<const link_up_actor *>
                                    ( get_use( "link_up" )->get_actor_ptr() );
    link().max_length = it_actor->cable_length == -1 ? type->maximum_charges() : it_actor->cable_length;
    link().efficiency = it_actor->efficiency < MIN_LINK_EFFICIENCY ? 0.0f : it_actor->efficiency;
    // Reset s_bub_pos to force the item to check the length during process_link.
    link().s_bub_pos = tripoint_bub_ms::min;
    link().last_processed = calendar::turn;

    for( const item *cable : cables() ) {
        if( !cable->can_link_up() ) {
            continue;
        }
        const link_up_actor *actor = static_cast<const link_up_actor *>
                                     ( cable->get_use( "link_up" )->get_actor_ptr() );
        link().max_length += actor->cable_length == -1 ? cable->type->maximum_charges() :
                             actor->cable_length;
        link().efficiency = link().efficiency < MIN_LINK_EFFICIENCY ? 0.0f :
                            link().efficiency * actor->efficiency;
    }

    if( link().efficiency >= MIN_LINK_EFFICIENCY ) {
        link().charge_rate = it_actor->charge_rate.value();
        // Convert charge_rate to how long it takes to charge 1 kW, the unit batteries use.
        // 0 means batteries won't be charged, but it can still provide power to devices unless efficiency is also 0.
        if( it_actor->charge_rate != 0_W ) {
            double charge_rate = std::floor( 1000000.0 / abs( it_actor->charge_rate.value() ) + 0.5 );
            link().charge_interval = std::max( 1, static_cast<int>( charge_rate ) );
        }
    }
}

int item::link_length() const
{
    return has_no_links() ? -2 :
           link_has_state( link_state::needs_reeling ) ? -1 : link().length;
}

int item::max_link_length() const
{
    return !has_link_data() ? -2 :
           link().max_length != -1 ? link().max_length : type->maximum_charges();
}

int item::link_sort_key() const
{
    const int length = link_length();
    int key = length >= 0 ? -1000000000 : length == -1 ? 0 : 1000000000;
    key += max_link_length() * 100000;
    return key - length;
}

bool item::process_link( map &here, Character *carrier, const tripoint_bub_ms &pos )
{
    if( link_length() < 0 ) {
        return false;
    }

    const bool is_cable_item = has_flag( flag_CABLE_SPOOL );

    // Handle links to items in the inventory.
    if( link().source == link_state::solarpack ) {
        if( carrier == nullptr || !carrier->worn_with_flag( flag_SOLARPACK_ON ) ) {
            add_msg_if_player_sees( pos, m_bad, _( "The %s has come loose from the solar pack." ),
                                    link_name() );
            reset_link( true, carrier );
            return false;
        }
    }
    const item_filter used_ups = [&]( const item & itm ) {
        return itm.get_var( "cable" ) == "plugged_in";
    };
    if( link().source == link_state::ups ) {
        if( carrier == nullptr || !carrier->cache_has_item_with( flag_IS_UPS, used_ups ) ) {
            add_msg_if_player_sees( pos, m_bad, _( "The %s has come loose from the UPS." ), link_name() );
            reset_link( true, carrier );
            return false;
        }
    }
    // Certain cable states should skip processing and also become inactive if dropped.
    if( ( link().target == link_state::no_link && link().source != link_state::vehicle_tow ) ||
        link().target == link_state::bio_cable ) {
        if( carrier == nullptr ) {
            return reset_link( true, nullptr, -1, true, pos );
        }
        return false;
    }

    const bool last_t_abs_pos_is_oob = !here.inbounds( link().t_abs_pos );

    // Lambda function for checking if a cable has been stretched too long, resetting it if so.
    // @return True if the cable is disconnected.
    const auto check_length = [this, carrier, pos, last_t_abs_pos_is_oob, is_cable_item]() {
        if( debug_mode ) {
            add_msg_debug( debugmode::DF_IUSE, "%s linked to %s%s, length %d/%d", type_name(),
                           link().t_abs_pos.to_string_writable(), last_t_abs_pos_is_oob ? " (OoB)" : "",
                           link().length, link().max_length );
        }
        if( link().length > link().max_length ) {
            std::string cable_name = is_cable_item ? string_format( _( "over-extended %s" ), label( 1 ) ) :
                                     string_format( _( "%s's over-extended cable" ), label( 1 ) );
            if( carrier != nullptr ) {
                carrier->add_msg_if_player( m_bad, _( "Your %s breaks loose!" ), cable_name );
            } else {
                add_msg_if_player_sees( pos, m_bad, _( "Your %s breaks loose!" ), cable_name );
            }
            return true;
        } else if( link().length + M_SQRT2 >= link().max_length + 1 && carrier != nullptr ) {
            carrier->add_msg_if_player( m_warning, _( "Your %s is stretched to its limit!" ), link_name() );
        }
        return false;
    };

    // Check if the item has moved positions this turn.
    bool length_check_needed = false;
    if( link().s_bub_pos != pos ) {
        link().s_bub_pos = pos;
        length_check_needed = true;
    }

    // Re-establish vehicle pointer if it got lost or if this item just got loaded.
    if( !link().t_veh ) {
        vehicle *found_veh = vehicle::find_vehicle( here, link().t_abs_pos );
        if( !found_veh ) {
            return reset_link( true, carrier, -2, true, pos );
        }
        if( debug_mode ) {
            add_msg_debug( debugmode::DF_IUSE, "Re-established link of %s to %s.", type_name(),
                           found_veh->disp_name() );
        }
        link().t_veh = found_veh->get_safe_reference();
    }

    // Regular pointers are faster, so make one now that we know the reference is valid.
    vehicle *t_veh = link().t_veh.get();

    // We should skip processing if the last saved target point is out of bounds, since vehicles give innacurate absolute coordinates when out of bounds.
    // However, if the linked vehicle is moving fast enough, we should always do processing to avoid erroneously skipping linked items riding inside of it.
    if( last_t_abs_pos_is_oob && t_veh->velocity < HALF_MAPSIZE_X * 400 ) {
        if( !length_check_needed ) {
            return false;
        }
        link().length = rl_dist( here.get_abs( pos ), link().t_abs_pos ) +
                        link().t_mount.abs().x() + link().t_mount.abs().y();
        if( check_length() ) {
            return reset_link( true, carrier );
        }
        return false;
    }

    // Find the vp_part index the cable is linked to.
    int link_vp_index = -1;
    if( link().target == link_state::vehicle_port ) {
        for( int idx : t_veh->cable_ports ) {
            if( t_veh->part( idx ).mount == link().t_mount ) {
                link_vp_index = idx;
                break;
            }
        }
    } else if( link().target == link_state::vehicle_battery ) {
        for( int idx : t_veh->batteries ) {
            if( t_veh->part( idx ).mount == link().t_mount ) {
                link_vp_index = idx;
                break;
            }
        }
        if( link_vp_index == -1 ) {
            // Check cable_ports, since that includes appliances
            for( int idx : t_veh->cable_ports ) {
                if( t_veh->part( idx ).mount == link().t_mount ) {
                    link_vp_index = idx;
                    break;
                }
            }
        }
    } else if( link().target == link_state::vehicle_tow || link().source == link_state::vehicle_tow ) {
        link_vp_index = t_veh->part_at( t_veh->coord_translate( link().t_mount ) );
    }
    if( link_vp_index == -1 ) {
        // The part with cable ports was lost, so disconnect the cable.
        return reset_link( true, carrier, -2, true, pos );
    }

    if( link().last_processed <= t_veh->part( link_vp_index ).last_disconnected ) {
        add_msg_if_player_sees( pos, m_warning, string_format( _( "You detached the %s." ),
                                type_name() ) );
        return reset_link( true, carrier, -2 );
    }
    t_veh->part( link_vp_index ).set_flag( vp_flag::linked_flag );

    int turns_elapsed = to_turns<int>( calendar::turn - link().last_processed );
    link().last_processed = calendar::turn;

    // Set the new absolute position to the vehicle's origin.
    tripoint_abs_ms new_t_abs_pos = t_veh->pos_abs();;
    tripoint_bub_ms t_veh_bub_pos = here.get_bub( new_t_abs_pos );;
    if( link().t_abs_pos != new_t_abs_pos ) {
        link().t_abs_pos = new_t_abs_pos;
        length_check_needed = true;
    }

    // If either of the link's connected sides moved, check the cable's length.
    if( length_check_needed ) {
        link().length = rl_dist( pos, t_veh_bub_pos + t_veh->part( link_vp_index ).precalc[0].raw() );
        if( check_length() ) {
            return reset_link( true, carrier, link_vp_index );
        }
    }

    // Extra behaviors for the cabled item.
    if( !is_cable_item ) {
        int power_draw = 0;

        if( link().efficiency < MIN_LINK_EFFICIENCY ) {
            return false;
        }
        // Recharge or charge linked batteries
        power_draw -= charge_linked_batteries( *t_veh, turns_elapsed );

        // Tool power draw display
        if( active && type->tool && type->tool->power_draw > 0_W ) {
            power_draw -= type->tool->power_draw.value();
        }

        // Send total power draw to the vehicle so it can be displayed.
        if( power_draw != 0 ) {
            t_veh->linked_item_epower_this_turn += units::from_milliwatt( power_draw );
        }
    } else if( has_flag( flag_NO_DROP ) ) {
        debugmsg( "%s shouldn't exist outside of an item or vehicle part.", tname() );
        return true;
    }

    return false;
}

int item::charge_linked_batteries( vehicle &linked_veh, int turns_elapsed )
{
    map &here = get_map();

    if( !has_link_data() || link().charge_rate == 0 || link().charge_interval < 1 ) {
        return 0;
    }

    if( !is_battery() ) {
        const item *parent_mag = magazine_current();
        if( !parent_mag || !parent_mag->has_flag( flag_RECHARGE ) ) {
            return 0;
        }
    }

    const bool power_in = link().charge_rate > 0;
    if( power_in ? ammo_remaining( ) >= ammo_capacity( ammo_battery ) :
        ammo_remaining( ) <= 0 ) {
        return 0;
    }

    // Normally efficiency is the chance to get a charge every charge_interval, but if we're catching up from
    // time spent ouside the reality bubble it should be applied as a percentage of the total instead.
    bool short_time_passed = turns_elapsed <= link().charge_interval;

    if( turns_elapsed < 1 || ( short_time_passed &&
                               !calendar::once_every( time_duration::from_turns( link().charge_interval ) ) ) ) {
        return link().charge_rate;
    }

    // If a long time passed, multiply the total by the efficiency rather than cancelling a charge.
    int transfer_total = short_time_passed ? 1 :
                         ( turns_elapsed * 1.0f / link().charge_interval ) * link().charge_interval;

    if( power_in ) {
        const int battery_deficit = linked_veh.discharge_battery( here, transfer_total, true );
        // Around 85% efficient by default; a few of the discharges don't actually recharge
        if( battery_deficit == 0 && ( !short_time_passed || rng_float( 0.0, 1.0 ) <= link().efficiency ) ) {
            ammo_set( itype_battery, ammo_remaining( ) + transfer_total );
        }
    } else {
        // Around 85% efficient by default; a few of the discharges don't actually charge
        if( !short_time_passed || rng_float( 0.0, 1.0 ) <= link().efficiency ) {
            const int battery_surplus = linked_veh.charge_battery( here, transfer_total, true );
            if( battery_surplus == 0 ) {
                ammo_set( itype_battery, ammo_remaining( ) - transfer_total );
            }
        } else {
            const std::pair<int, int> linked_levels = linked_veh.connected_battery_power_level( here );
            if( linked_levels.first < linked_levels.second ) {
                ammo_set( itype_battery, ammo_remaining( ) - transfer_total );
            }
        }
    }
    return link().charge_rate;
}

bool item::reset_link( bool unspool_if_too_long, Character *p, int vpart_index,
                       const bool loose_message, const tripoint_bub_ms cable_position )
{
    if( !has_link_data() ) {
        return has_flag( flag_NO_DROP );
    }
    if( unspool_if_too_long && link_has_state( link_state::needs_reeling ) ) {
        // Cables that need reeling should be reset with a reel_cable_activity_actor instead.
        return false;
    }

    if( vpart_index != -2 && link().t_veh ) {
        if( vpart_index == -1 ) {
            vehicle *t_veh = link().t_veh.get();
            // Find the vp_part index the cable is linked to.
            if( link().target == link_state::vehicle_port ) {
                for( int idx : t_veh->cable_ports ) {
                    if( t_veh->part( idx ).mount == link().t_mount ) {
                        vpart_index = idx;
                        break;
                    }
                }
            } else if( link().target == link_state::vehicle_battery ) {
                for( int idx : t_veh->batteries ) {
                    if( t_veh->part( idx ).mount == link().t_mount ) {
                        vpart_index = idx;
                        break;
                    }
                }
            } else if( link().target == link_state::vehicle_tow || link().source == link_state::vehicle_tow ) {
                vpart_index = t_veh->part_at( t_veh->coord_translate( link().t_mount ) );
            }
        }
        if( vpart_index != -1 ) {
            link().t_veh->part( vpart_index ).remove_flag( vp_flag::linked_flag );
        }
    }

    if( loose_message ) {
        if( p != nullptr ) {
            p->add_msg_if_player( m_warning, _( "Your %s has come loose." ), link_name() );
        } else {
            add_msg_if_player_sees( cable_position, m_warning, _( "The %s has come loose." ),
                                    link_name() );
        }
    }

    if( unspool_if_too_long && link().length > LINK_RESPOOL_THRESHOLD ) {
        // Cables that are too long need to be manually rewound before reuse.
        link().source = link_state::needs_reeling;
        return false;
    }

    if( loose_message && p ) {
        p->add_msg_if_player( m_info, _( "You reel in the %s and wind it up." ), link_name() );
    }

    link_.reset();
    if( !cables().empty() ) {
        // If there are extensions, keep link active to maintain max_length.
        update_link_traits();
    }
    return has_flag( flag_NO_DROP );
}

bool item::process_linked_item( Character *carrier, const tripoint_bub_ms & /*pos*/,
                                const link_state required_state )
{
    if( carrier == nullptr ) {
        erase_var( "cable" );
        active = false;
        return false;
    }
    bool has_connected_cable = carrier->cache_has_item_with( "can_link_up", &item::can_link_up,
    [&required_state]( const item & it ) {
        return it.link_has_state( required_state );
    } );
    if( !has_connected_cable ) {
        erase_var( "cable" );
        active = false;
    }
    return false;
}

units::energy item::energy_per_second() const
{
    units::energy energy_to_burn;
    if( type->tool->turns_per_charge > 0 ) {
        energy_to_burn += units::from_kilojoule( std::max<int64_t>( ammo_required(),
                          1 ) ) / type->tool->turns_per_charge;
    } else if( type->tool->power_draw > 0_mW ) {
        energy_to_burn += type->tool->power_draw * 1_seconds;
    }
    return energy_to_burn;
}

bool item::process_tool( Character *carrier, const tripoint_bub_ms &pos )
{
    map &here = get_map();

    // FIXME: remove this once power armors don't need to be TOOL_ARMOR anymore
    if( is_power_armor() && carrier && carrier->can_interface_armor() && carrier->has_power() ) {
        return false;
    }

    // if insufficient available charges shutdown the tool
    if( ( type->tool->power_draw > 0_W || type->tool->turns_per_charge > 0 ) &&
        ( ( uses_energy() && energy_remaining( carrier ) < energy_per_second() ) ||
          ( !uses_energy() && ammo_remaining_linked( here, carrier ) == 0 ) ) ) {
        if( carrier && has_flag( flag_USE_UPS ) ) {
            carrier->add_msg_if_player( m_info, _( "You need an UPS to run the %s!" ), tname() );
        }
        if( carrier ) {
            carrier->add_msg_if_player( m_info, _( "The %s ran out of energy!" ), tname() );
        }
        if( type->transform_into.has_value() ) {
            deactivate( carrier );
            return false;
        } else {
            return true;
        }
    }

    if( energy_remaining( carrier ) > 0_J ) {
        energy_consume( energy_per_second(), pos, carrier );
    } else {
        // Non-electrical charge consumption.
        int charges_to_use = 0;
        if( type->tool->turns_per_charge > 0 &&
            to_turn<int>( calendar::turn ) % type->tool->turns_per_charge == 0 ) {
            charges_to_use = std::max( ammo_required(), 1 );
        }

        if( charges_to_use > 0 ) {
            ammo_consume( charges_to_use, pos, carrier );
        }
    }

    // FIXME: some iuse functions return 1+ expecting to be destroyed (molotovs), others
    // to use charges, and others just because?
    // allow some items to opt into requesting destruction
    const int charges_used = type->tick( carrier, *this, pos );
    const bool destroy = has_flag( flag_DESTROY_ON_CHARGE_USE );
    if( !destroy && charges_used > 0 ) {
        debugmsg( "Item %s consumes charges via tick_action, but should not", tname() );
    }
    return destroy && charges_used > 0;
}

bool item::process_blackpowder_fouling( Character *carrier )
{
    // Rust is deterministic. At a total modifier of 1 (the max): 12 hours for first rust, then 24 (36 total), then 36 (72 total) and finally 48 (120 hours to go to XX)
    // this speeds up by the amount the gun is dirty, 2-6x as fast depending on dirt level. At minimum dirt, the modifier is 0.3x the speed of the above mentioned figures.
    set_var( "rust_timer", get_var( "rust_timer", 0 ) + std::min( 0.3 + get_var( "dirt", 0 ) / 200,
             1.0 ) );
    double time_mult = 1.0 + ( 4.0 * static_cast<double>( damage() ) ) / static_cast<double>
                       ( max_damage() );
    if( damage() < max_damage() && get_var( "rust_timer", 0 ) > 43200.0 * time_mult ) {
        inc_damage();
        set_var( "rust_timer", 0 );
        if( carrier ) {
            carrier->add_msg_if_player( m_bad, _( "Your %s rusts due to corrosive powder fouling." ), tname() );
        }
    }
    return false;
}

bool item::process_gun_cooling( Character *carrier )
{
    double heat = get_var( "gun_heat", 0 );
    double overheat_modifier = 0;
    float overheat_multiplier = 1.0f;
    double cooling_modifier = 0;
    float cooling_multiplier = 1.0f;
    for( const item *mod : gunmods() ) {
        overheat_modifier += mod->type->gunmod->overheat_threshold_modifier;
        overheat_multiplier *= mod->type->gunmod->overheat_threshold_multiplier;
        cooling_modifier += mod->type->gunmod->cooling_value_modifier;
        cooling_multiplier *= mod->type->gunmod->cooling_value_multiplier;
    }
    double threshold = std::max( ( type->gun->overheat_threshold * overheat_multiplier ) +
                                 overheat_modifier, 5.0 );
    heat -= std::max( ( type->gun->cooling_value * cooling_multiplier ) + cooling_modifier, 0.5 );
    set_var( "gun_heat", std::max( 0.0, heat ) );
    if( has_fault( fault_overheat_safety ) && heat < threshold * 0.2 ) {
        remove_fault( fault_overheat_safety );
        if( carrier ) {
            carrier->add_msg_if_player( m_good, _( "Your %s beeps as its cooling cycle concludes." ), tname() );
        }
    }
    if( carrier ) {
        if( heat > threshold ) {
            carrier->add_msg_if_player( m_bad,
                                        _( "You can feel the struggling systems of your %s beneath the blare of the overheat alarm." ),
                                        tname() );
        } else if( heat > threshold * 0.8 ) {
            carrier->add_msg_if_player( m_bad, _( "Your %s frantically sounds an overheat alarm." ), tname() );
        } else if( heat > threshold * 0.6 ) {
            carrier->add_msg_if_player( m_warning, _( "Your %s sounds a heat warning chime!" ), tname() );
        }
    }
    return false;
}

bool item::is_reloadable() const
{
    if( ( has_flag( flag_NO_RELOAD ) && !has_flag( flag_VEHICLE ) ) ||
        ( is_gun() && !ammo_default() ) ) {
        // turrets ignore NO_RELOAD flag
        // don't show guns without default ammo defined in reload ui
        return false;
    }

    for( const item_pocket *pocket : contents.get_all_reloadable_pockets() ) {
        if( pocket->is_type( pocket_type::MAGAZINE_WELL ) ) {
            if( pocket->empty() || !pocket->front().is_magazine_full() ) {
                return true;
            }
        } else if( pocket->is_type( pocket_type::MAGAZINE ) ) {
            if( remaining_ammo_capacity() > 0 ) {
                return true;
            }
        } else if( pocket->is_type( pocket_type::CONTAINER ) ) {
            // Container pockets are reloadable only if they are watertight, not full and do not contain non-liquid item
            if( pocket->full( false ) || !pocket->watertight() ) {
                continue;
            }
            if( pocket->empty() || pocket->front().made_of( phase_id::LIQUID ) ) {
                return true;
            }
        }
    }

    for( const item *gunmod : gunmods() ) {
        if( gunmod->is_reloadable() ) {
            return true;
        }
    }

    return false;
}

units::energy item::get_gun_energy_drain() const
{
    units::energy draincount = 0_kJ;
    if( type->gun ) {
        units::energy modifier = 0_kJ;
        float multiplier = 1.0f;
        for( const item *mod : gunmods() ) {
            modifier += mod->type->gunmod->energy_drain_modifier;
            multiplier *= mod->type->gunmod->energy_drain_multiplier;
        }
        draincount = ( type->gun->energy_drain * multiplier ) + modifier;
    }
    return draincount;
}

units::energy item::get_gun_ups_drain() const
{
    if( has_flag( flag_USE_UPS ) ) {
        return get_gun_energy_drain();
    }
    return 0_kJ;
}

units::energy item::get_gun_bionic_drain() const
{
    if( has_flag( flag_USES_BIONIC_POWER ) ) {
        return get_gun_energy_drain();
    }
    return 0_kJ;
}

int item::get_to_hit() const
{
    int to_hit = type->m_to_hit;
    for( const item *mod : gunmods() ) {
        to_hit += mod->type->gunmod->to_hit_mod;
    }

    return to_hit;
}

int item::get_min_str() const
{
    const Character &p = get_player_character();
    if( type->gun ) {
        int min_str = type->min_str;
        // we really need some better check for bows than its skill
        if( type->gun->skill_used == skill_archery ) {
            min_str -= p.get_proficiency_bonus( "archery", proficiency_bonus_type::strength );
        }
        for( const item *mod : gunmods() ) {
            min_str += mod->type->gunmod->min_str_required_mod;
        }
        if( p.is_prone() ) {
            for( const item *mod : gunmods() ) {
                min_str += mod->type->gunmod->min_str_required_mod_if_prone;
            }
        }
        return min_str > 0 ? min_str : 0;
    } else {
        return type->min_str;
    }
}

disp_mod_by_barrel::disp_mod_by_barrel() = default;
void disp_mod_by_barrel::deserialize( const JsonObject &jo )
{
    mandatory( jo, false, "barrel_length", barrel_length );
    mandatory( jo, false, "dispersion", dispersion_modifier );
}
