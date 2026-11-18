#include "map.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <list>
#include <map>
#include <memory>
#include <numeric>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "ammo_effect.h"
#include "bodypart.h"
#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "creature_tracker.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "explosion.h"
#include "field.h"
#include "field_type.h"
#include "fungal_effects.h"
#include "item.h"
#include "item_group.h"
#include "item_location.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "map_accessories.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapdata.h"
#include "messages.h"
#include "mondeath.h"
#include "monster.h"
#include "mtype.h"
#include "point.h"
#include "projectile.h"
#include "requirements.h"
#include "rng.h"
#include "sounds.h"
#include "submap.h"
#include "translation.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "vehicle.h"
#include "viewer.h"
#include "vpart_position.h"

static const ammo_effect_str_id ammo_effect_IGNITE( "IGNITE" );
static const ammo_effect_str_id ammo_effect_INCENDIARY( "INCENDIARY" );
static const ammo_effect_str_id ammo_effect_LASER( "LASER" );
static const ammo_effect_str_id ammo_effect_LIGHTNING( "LIGHTNING" );
static const ammo_effect_str_id ammo_effect_PLASMA( "PLASMA" );

static const damage_type_id damage_bash( "bash" );

static const efftype_id effect_crushed( "crushed" );

static const item_group_id Item_spawn_data_EMPTY_GROUP( "EMPTY_GROUP" );

static const material_id material_glass( "glass" );

static const ter_str_id ter_t_bars( "t_bars" );
static const ter_str_id ter_t_card_industrial( "t_card_industrial" );
static const ter_str_id ter_t_card_military( "t_card_military" );
static const ter_str_id ter_t_card_reader_broken( "t_card_reader_broken" );
static const ter_str_id ter_t_card_science( "t_card_science" );
static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_door_b( "t_door_b" );
static const ter_str_id ter_t_door_bar_c( "t_door_bar_c" );
static const ter_str_id ter_t_door_bar_locked( "t_door_bar_locked" );
static const ter_str_id ter_t_door_bar_o( "t_door_bar_o" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_frame( "t_door_frame" );
static const ter_str_id ter_t_door_locked( "t_door_locked" );
static const ter_str_id ter_t_door_locked_alarm( "t_door_locked_alarm" );
static const ter_str_id ter_t_door_locked_peep( "t_door_locked_peep" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_floor_wax( "t_floor_wax" );
static const ter_str_id ter_t_gas_pump( "t_gas_pump" );
static const ter_str_id ter_t_gas_pump_smashed( "t_gas_pump_smashed" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_reb_cage( "t_reb_cage" );
static const ter_str_id ter_t_vat( "t_vat" );
static const ter_str_id ter_t_wall_glass( "t_wall_glass" );
static const ter_str_id ter_t_wall_glass_alarm( "t_wall_glass_alarm" );
static const ter_str_id ter_t_wax( "t_wax" );
static const ter_str_id ter_t_window( "t_window" );
static const ter_str_id ter_t_window_alarm( "t_window_alarm" );
static const ter_str_id ter_t_window_empty( "t_window_empty" );
static const ter_str_id ter_t_window_no_curtains( "t_window_no_curtains" );

int map::get_map_damage( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) ) {
        return 0;
    }

    point_sm_ms l;
    const submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Called get_map_damage for unloaded submap" );
        return 0;
    }
    return current_submap->get_map_damage( l );
}

void map::set_map_damage( const tripoint_bub_ms &p, int dmg )
{
    if( !inbounds( p ) ) {
        return;
    }

    point_sm_ms l;
    submap *const current_submap = unsafe_get_submap_at( p, l );
    if( current_submap == nullptr ) {
        debugmsg( "Called set_map_damage for unloaded submap" );
        return;
    }
    current_submap->set_map_damage( l, dmg );
}

// returns 0 if not damageable
int map_common_bash_info::damage_to( const std::map<damage_type_id, int> &str,
                                     bool supported, bool blocked ) const
{
    int min = str_min;
    if( supported && str_min_supported != -1 ) {
        min = str_min_supported;
    }
    if( blocked && str_min_blocked != -1 ) {
        min = str_min_blocked;
    }
    return damage_profile->damage_from( str, min );
}

int map_common_bash_info::hp( bool supported, bool blocked ) const
{
    if( supported  && str_max_supported != -1 ) {
        return str_max_supported;
    }
    if( blocked && str_max_blocked != -1 ) {
        return str_max_blocked;
    }
    return str_max;
}

// Bashable - common function

int map::bash_rating_internal( const std::map<damage_type_id, int> &str,
                               const furn_t &furniture, const ter_t &terrain,
                               const bool allow_floor, const vehicle *veh, const int part ) const
{
    bool furn_smash = false;
    bool ter_smash = false;
    ///\EFFECT_STR determines what furniture can be smashed
    if( furniture.id && furniture.bash ) {
        furn_smash = true;
        ///\EFFECT_STR determines what terrain can be smashed
    } else if( terrain.bash && ( !terrain.bash->bash_below || allow_floor ) ) {
        ter_smash = true;
    }

    if( veh != nullptr && vpart_position( const_cast<vehicle &>( *veh ), part ).obstacle_at_part() ) {
        // Monsters only care about rating > 0, NPCs should want to path around cars instead
        return 2; // Should probably be a function of part hp (+armor on tile)
    }

    int damage = 0;
    int hp = 0;
    if( furn_smash ) {
        damage = furniture.bash->damage_to( str );
        hp = furniture.bash->hp();
    } else if( ter_smash ) {
        damage = terrain.bash->damage_to( str );
        hp = terrain.bash->hp();
    } else {
        return -1;
    }

    ///\EFFECT_STR increases smashing damage
    if( damage <= 0 ) {
        return 0;
    } else if( damage > hp ) {
        return 10;
    }

    int ret = ( 10 * damage ) / hp;
    // Round up to 1, so that desperate NPCs can try to bash down walls
    return std::max( ret, 1 );
}

// 3D bashable

bool map::is_bashable( const tripoint_bub_ms &p, const bool allow_floor ) const
{
    if( !inbounds( p ) ) {
        DebugLog( D_WARNING, D_MAP ) << "Looking for out-of-bounds is_bashable at "
                                     << p.x() << ", " << p.y() << ", " << p.z();
        return false;
    }

    if( veh_at( p ).obstacle_at_part() ) {
        return true;
    }

    return is_bashable_ter_furn( p, allow_floor );
}

bool map::is_bashable_ter( const tripoint_bub_ms &p, const bool allow_floor ) const
{
    const std::optional<map_ter_bash_info> &ter_bash = ter( p ).obj().bash;
    return ter_bash && ( !ter_bash->bash_below || allow_floor );
}

bool map::is_bashable_furn( const tripoint_bub_ms &p ) const
{
    return has_furn( p ) && furn( p ).obj().bash;
}

bool map::is_bashable_ter_furn( const tripoint_bub_ms &p, const bool allow_floor ) const
{
    return is_bashable_furn( p ) || is_bashable_ter( p, allow_floor );
}

int map::bash_strength( const tripoint_bub_ms &p, const bool allow_floor ) const
{
    if( is_bashable_furn( p ) ) {
        return furn( p ).obj().bash->str_max;
    }

    if( is_bashable_ter( p, allow_floor ) ) {
        return ter( p ).obj().bash->str_max;
    }

    return -1;
}

int map::bash_resistance( const tripoint_bub_ms &p, const bool allow_floor ) const
{
    if( is_bashable_furn( p ) ) {
        return furn( p ).obj().bash->str_min;
    }

    if( is_bashable_ter( p, allow_floor ) ) {
        return ter( p ).obj().bash->str_min;
    }

    return -1;
}

int map::bash_rating( const std::map<damage_type_id, int> &str, const tripoint_bub_ms &p,
                      const bool allow_floor ) const
{
    if( !inbounds( p ) ) {
        DebugLog( D_WARNING, D_MAP ) << "Looking for out-of-bounds is_bashable at "
                                     << p.x() << ", " << p.y() << ", " << p.z();
        return -1;
    }

    if( str.empty() ) {
        return -1;
    }

    const furn_t &furniture = furn( p ).obj();
    const ter_t &terrain = ter( p ).obj();
    const optional_vpart_position vp = veh_at( p );
    vehicle *const veh = vp ? &vp->vehicle() : nullptr;
    const int part = vp ? vp->part_index() : -1;
    return bash_rating_internal( str, furniture, terrain, allow_floor, veh, part );
}

void map::smash_items( const tripoint_bub_ms &p, int power, const std::string &cause_message,
                       vehicle_part *vp_wheel, vehicle *veh )
{
    if( !has_items( p ) || has_flag_ter_or_furn( ter_furn_flag::TFLAG_PLANT, p ) ) {
        return;
    }

    // Keep track of how many items have been damaged, and what the first one is
    bool item_was_damaged = false;
    int items_damaged = 0;
    int items_destroyed = 0;
    std::string damaged_item_name;

    std::vector<item> contents;
    map_stack items = i_at( p );
    std::vector<std::string> wheel_damage_messages;

    for( auto i = items.begin(); i != items.end() && power > 0; ) {
        if( i->made_of( phase_id::LIQUID ) ) {
            i++;
            continue;
        }

        if( vp_wheel != nullptr && vehicle::hit_probability( *i, vp_wheel ) < rng_float( 0.0, 1.0 ) ) {
            // The wheel missed the item.
            continue;
        }

        if( vp_wheel != nullptr ) {
            veh->damage_wheel_on_item( vp_wheel, *i, &wheel_damage_messages );
        }

        if( i->active ) {
            // Get the explosion item actor
            if( i->type->get_use( "explosion" ) != nullptr ) {
                const explosion_iuse *actor = dynamic_cast<const explosion_iuse *>(
                                                  i->type->get_use( "explosion" )->get_actor_ptr() );
                if( actor != nullptr ) {
                    // If we're looking at another bomb, don't blow it up early for now.
                    // i++ here because we aren't iterating in the loop header.
                    i++;
                    continue;
                }
            }
        }

        const float material_factor = i->chip_resistance( true );
        if( power < material_factor ) {
            i++;
            continue;
        }

        if( veh && vp_wheel ) {
            const double relative_mass = static_cast<double>( i->weight().value() ) /
                                         static_cast<double>( veh->total_mass( *this ).value() );
            // Make sure our velocity doesn't flip signs. i.e. we don't "bounce" off an object, even one that's more than 2.0x as heavy as our vehicle.
            const double remaining_velocity_factor = std::clamp( ( 1.0 - ( relative_mass / 2.0 ) ), 0.0, 1.0 );
            // Wheel runs over object --> Vehicle loses some speed
            veh->velocity = veh->velocity * remaining_velocity_factor;

            // Always reduce power of remaining wheel damage.
            power -= material_factor;

            // Wheels running over items can do one of three things to the item:
            // For non-pulped corpses, they can gib the corpse.
            // For salvageable items, they salvage them, at extreme loss. e.g. a pile of sticks can be turned into scattered "splintered wood"
            // For all other items they are either ejected (rarely) or the wheels roll over them (do nothing).
            if( relative_mass < 0.5 && i->is_corpse() && i->can_revive() ) {
                damaged_item_name = i->tname();
                items_damaged++;
                items_destroyed++;
                // Extremely funny implementation: Making a fake monster and splattering it.
                monster mon( i->get_corpse_mon()->id );
                // Remove the corpse first, to make sure we have space for the resulting gibs.
                i = i_rem( p, i );
                mon.set_hp( mon.get_hp_max() * -2 );
                mon.setpos( get_abs( p ) );
                mdeath::splatter( this, mon );
            } else if( i->is_salvageable() ) {
                damaged_item_name = i->tname();
                items_damaged++;
                items_destroyed++;
                item_location there( map_cursor( this, p ), &*i );
                std::map<itype_id, int> salvage = salvage_actor::salvage_results( there, /*efficiency =*/ 0.1 );
                i = i_rem( p, i ); // Remove item we just fake "cut up" and preserve our iterator
                for( std::pair<const itype_id, int> pair : salvage ) {
                    add_item_or_charges( p, item( pair.first ), pair.second );
                }
            } else {
                // Small chance: Eject item away from wheel
                if( one_in( 5 ) ) {
                    point_rel_ms move_to( rng( 0, 1 ), rng( 0, 1 ) );
                    if( move_to != point_rel_ms() ) {  // Don't "move" to the same tile, always an adjacent one
                        add_item( p + move_to, *i );
                        i = i_rem( p, i );
                        continue;
                    }
                }
                // Nothing happens! Item stays where it is, vehicle keeps rolling with its remaining velocity.
                i++;
            }
            continue;
        }

        // The volume check here pretty much only influences corpses and very large items
        const float volume_factor = std::max<float>( 40, i->volume() / 250_ml );
        float damage_chance = 10.0f * power / volume_factor;
        // Example:
        // Power 40 (just below C4 epicenter) vs plank
        // damage_chance = 10 * 40 / 40 = 10, material_factor = 8
        // Will deal 1 damage, then 20% chance for another point
        // Power 20 (grenade minus shrapnel) vs glass bottle
        // 10 * 20 / 40 = 5 vs 1
        // 5 damage (destruction)

        const bool by_charges = i->count_by_charges();
        // See if they were damaged
        if( by_charges ) {
            damage_chance *= i->charges_per_volume( 250_ml );
            while( ( damage_chance > material_factor ||
                     x_in_y( damage_chance, material_factor ) ) &&
                   i->charges > 0 ) {
                i->charges--;
                damage_chance -= material_factor;
                // We can't increment items_damaged directly because a single item can be damaged more than once
                item_was_damaged = true;
            }
        } else {
            const field_type_id type_blood = i->is_corpse() ? i->get_mtype()->bloodType() : fd_null;
            while( damage_chance > material_factor || x_in_y( damage_chance, material_factor ) ) {
                add_splash( type_blood, p, 1, damage_chance );
                damage_chance -= material_factor;
                item_was_damaged = true;
                if( i->damage() < i->max_damage() ) {
                    i->inc_damage();
                } else {
                    break;
                }
            }
        }
        power -= material_factor;

        // If an item was damaged, increment the counter and set it as most recently damaged.
        if( item_was_damaged ) {
            // If this is the first item to be damaged, store its name in damaged_item_name.
            if( items_damaged == 0 ) {
                damaged_item_name = i->tname();
            }
            // Increment the counter, and reset the flag.
            items_damaged++;
            item_was_damaged = false;
            // Remove them if they were damaged too much
            // Only call this if we actually damaged something, so a corpse that
            // was already at max_damage level can be removed correctly.
            if( i->damage() == i->max_damage() || ( by_charges && i->charges == 0 ) ) {
                // But save the contents, except for irremovable gunmods
                for( item *elem : i->all_items_top() ) {
                    if( !elem->is_irremovable() ) {
                        contents.emplace_back( *elem );
                    }
                }

                i = i_rem( p, i );
                items_destroyed++;
            } else {
                i++;
            }
        } else {
            i++;
        }
    }

    // Let the player know that the item was damaged if they can see it.
    map &bubble_map = reality_bubble();

    if( items_destroyed > 1 ) {
        add_msg_if_player_sees( bubble_map.get_bub( get_abs( p ) ), m_bad,
                                _( "The %s destroys several items!" ), cause_message );
    } else if( items_destroyed == 1 && items_damaged == 1 ) {
        add_msg_if_player_sees( bubble_map.get_bub( get_abs( p ) ), m_bad,
                                //~ %1$s: the cause of destruction, %2$s: destroyed item name
                                _( "The %1$s destroys the %2$s!" ), cause_message,
                                damaged_item_name );
    } else if( items_damaged > 1 ) {
        add_msg_if_player_sees( bubble_map.get_bub( get_abs( p ) ), m_bad,
                                _( "The %s damages several items." ), cause_message );
    } else if( items_damaged == 1 ) {
        add_msg_if_player_sees( bubble_map.get_bub( get_abs( p ) ), m_bad,
                                //~ %1$s: the cause of damage, %2$s: damaged item name
                                _( "The %1$s damages the %2$s." ),
                                cause_message,
                                damaged_item_name );
    }

    const bool player_is_driver = veh != nullptr && &get_player_character() == veh->get_driver( *this );
    const bool player_sees_damage = get_player_character().sees( *this, p );

    if( !wheel_damage_messages.empty() && ( player_is_driver || player_sees_damage ) ) {
        for( const std::string &msg : wheel_damage_messages ) {
            add_msg( m_bad, msg );
        }
    }

    for( const item &it : contents ) {
        add_item_or_charges( p, it );
    }
}

// Check if there is supporting furniture cardinally adjacent to the bashed furniture
// For example, a washing machine behind the bashed door
static bool furn_is_supported( const map &m, const tripoint_bub_ms &p )
{
    static constexpr std::array<int8_t, 4> cx = { 0, -1, 0, 1 };
    static constexpr std::array<int8_t, 4> cy = { -1,  0, 1, 0 };

    for( int i = 0; i < 4; i++ ) {
        const point_bub_ms adj( p.xy() + point( cx[i], cy[i] ) );
        if( m.has_furn( tripoint_bub_ms( adj, p.z() ) ) &&
            m.furn( tripoint_bub_ms( adj, p.z() ) ).obj().has_flag( ter_furn_flag::TFLAG_BLOCKSDOOR ) ) {
            return true;
        }
    }

    return false;
}

void map::bash_ter_furn( const tripoint_bub_ms &p, bash_params &params, bool repair_missing_ground )
{
    int sound_volume = 0;
    std::string soundfxid;
    std::string soundfxvariant;
    const ter_t &terid = ter( p ).obj();
    const furn_t &furnid = furn( p ).obj();
    bool smash_furn = false;
    bool smash_ter = false;
    std::optional<map_common_bash_info> bash;
    map_ter_bash_info ter_bash;
    map_furn_bash_info furn_bash;

    tripoint_bub_ms below = p + tripoint_rel_ms::below;
    ter_str_id below_tile_roof = ter_t_dirt;
    bool roof_of_below_tile = false;

    if( zlevels ) {
        below_tile_roof = get_roof( below, false );
        roof_of_below_tile = ( terid.id.id() == below_tile_roof.id() );
    }

    bool success = false;

    if( has_furn( p ) && furnid.bash ) {
        furn_bash = *furnid.bash;
        bash = static_cast<map_common_bash_info &>( furn_bash );
        smash_furn = true;
    } else if( ter( p ).obj().bash ) {
        ter_bash = *ter( p ).obj().bash;
        bash = static_cast<map_common_bash_info &>( ter_bash );
        smash_ter = true;
    }

    // Floor bashing check
    // Only allow bashing floors when we want to bash floors and we're in z-level mode
    // Unless we're destroying, then it gets a little weird
    if( smash_ter && bash->bash_below && ( !zlevels || !params.bash_floor ) ) {
        if( !params.destroy ) { // NOLINT(bugprone-branch-clone)
            smash_ter = false;
            bash.reset();
        } else if( !ter_bash.ter_set && zlevels ) {
            // HACK: A hack for destroy && !bash_floor
            // We have to check what would we create and cancel if it is what we have now
            if( roof_of_below_tile ) {
                smash_ter = false;
                bash.reset();
            }
        } else if( !ter_bash.ter_set && ter( p ) == ter_t_dirt ) {
            // As above, except for no-z-levels case
            smash_ter = false;
            bash.reset();
        }
    }

    if( has_flag( ter_furn_flag::TFLAG_ALARMED, p ) ) {
        sounds::sound( p, 40, sounds::sound_t::alarm, _( "an alarm go off!" ),
                       false, "environment", "alarm" );
    }

    if( !bash || ( bash->destroy_only && !params.destroy ) ) {
        // Nothing bashable here
        if( impassable( p ) ) {
            if( !params.silent ) {
                sounds::sound( p, 18, sounds::sound_t::combat, _( "thump!" ),
                               false, "smash_fail", "default" );
            }

            params.did_bash = true;
            params.bashed_solid = true;
        }

        return;
    }

    bool blocked = false;
    bool supported = false;
    int sound_vol = bash->sound_vol;
    int sound_fail_vol = bash->sound_fail_vol;
    if( !params.destroy ) {
        blocked = ( bash->str_min_blocked != -1 || bash->str_max_blocked != -1 ) &&
                  furn_is_supported( *this, p );

        if( bash->str_min_supported != -1 || bash->str_max_supported != -1 ) {
            tripoint_bub_ms below = p + tripoint_rel_ms::below;
            if( !zlevels || has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, below ) ) {
                supported = true;
            }
        }
        // Semi-persistant map damage. Increment by one for each bash over smin
        // Gradually makes hard bashes easier
        int damage = get_map_damage( tripoint_bub_ms( p ) );
        int damage_dealt = bash->damage_to( params.strength, supported, blocked );
        add_msg_debug( debugmode::DF_MAP,
                       "Smash: damage is %d + %d, hp: %d)",
                       damage_dealt, damage, bash->hp( supported, blocked ) );
        if( damage_dealt + damage >= bash->hp( supported, blocked ) ) {
            damage = 0;
            success = true;
        } else if( damage_dealt > 0 ) {
            damage += damage_dealt;
            params.can_bash = true;
        }
        set_map_damage( p, damage );
    }

    if( smash_furn ) {
        soundfxvariant = furnid.id.str();
    } else {
        soundfxvariant = terid.id.str();
    }

    if( !params.destroy && !success ) {
        if( sound_fail_vol == -1 ) {
            sound_volume = 12;
        } else {
            sound_volume = sound_fail_vol;
        }

        params.did_bash = true;
        if( !params.silent ) {
            sounds::sound( p, sound_volume, sounds::sound_t::combat, bash->sound_fail, false,
                           "smash_fail", soundfxvariant );
        }

        // add here because early return, otherwise they're added at the end
        if( !bash->hit_field.first.is_null() ) {
            add_field( p, bash->hit_field.first, bash->hit_field.second );
        }

        return;
    }

    // Clear out any partially grown seeds
    if( has_flag_ter_or_furn( ter_furn_flag::TFLAG_PLANT, p ) ) {
        i_clear( p );
    }

    if( ( ( smash_furn && has_flag_furn( ter_furn_flag::TFLAG_FUNGUS, p ) ) ||
          ( smash_ter && has_flag_ter( ter_furn_flag::TFLAG_FUNGUS, p ) ) ) &&
        field_at( p ).find_field( fd_fire ) == nullptr ) {
        fungal_effects().create_spores( p );
    }

    if( params.destroy ) {
        sound_volume = bash->str_min * 2;
    } else {
        if( sound_vol == -1 ) {
            sound_volume = std::min<int>( bash->str_min * 1.5, bash->str_max );
        } else {
            sound_volume = sound_vol;
        }
    }

    soundfxid = "smash_success";
    const translation &sound = bash->sound;
    // Set this now in case the ter_set below changes this
    const bool will_collapse = smash_ter &&
                               has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, p ) && !has_flag( ter_furn_flag::TFLAG_INDOORS, p );
    const bool tent = smash_furn && !bash->tent_centers.empty();
    bool set_to_air = false;
    bool phased = false;

    // Special code to collapse the tent if destroyed
    if( tent ) {
        // Get ids of possible centers
        std::set<furn_id> centers;
        for( const auto &cur_id : bash->tent_centers ) {
            if( cur_id.is_valid() ) {
                centers.insert( cur_id );
            }
        }

        std::optional<std::pair<tripoint_bub_ms, furn_id>> tentp;

        // Find the center of the tent
        // First check if we're not currently bashing the center
        if( centers.count( furn( p ) ) > 0 ) {
            tentp.emplace( p, furn( p ) );
        } else {
            for( const tripoint_bub_ms &pt : points_in_radius( p, bash->collapse_radius ) ) {
                const furn_id &f_at = furn( pt );
                // Check if we found the center of the current tent
                if( centers.count( f_at ) > 0 ) {
                    tentp.emplace( pt, f_at );
                    break;
                }
            }
        }
        // Didn't find any tent center, wreck the current tile
        if( !tentp ) {
            if( bash ) {
                if( smash_ter ) {
                    drop_bash_results( terid, p );
                } else {
                    drop_bash_results( furnid, p );
                }
                spawn_items( p, item_group::items_from( bash->drop_group, calendar::turn ) );
                furn_set( p, furn_bash.furn_set );
            }
        } else {
            // Take the tent down
            const std::optional<map_furn_bash_info> &tent_bash = tentp->second.obj().bash;
            const int rad = tent_bash ? tent_bash->collapse_radius : 0;
            for( const tripoint_bub_ms &pt : points_in_radius( tentp->first, rad ) ) {
                const furn_id &frn = furn( pt );
                if( frn == furn_str_id::NULL_ID() ) {
                    continue;
                }
                const std::optional<map_furn_bash_info> &tent_recur_bash = frn.obj().bash;
                // Check if we share a center type and thus a "tent type"
                if( tent_recur_bash ) {
                    for( const auto &cur_id : tent_recur_bash->tent_centers ) {
                        if( centers.count( cur_id.id() ) > 0 ) {
                            // Found same center, wreck current tile
                            spawn_items( p, item_group::items_from( tent_recur_bash->drop_group, calendar::turn ) );
                            furn_set( pt, tent_recur_bash->furn_set );
                            break;
                        }
                    }
                }
            }
        }
        soundfxvariant = "smash_cloth";
    } else if( smash_furn ) {
        furn_set( p, furn_bash.furn_set );
        for( item &it : i_at( p ) ) {
            it.on_drop( p, *this );
        }
        // HACK: Hack alert.
        // Signs have cosmetics associated with them on the submap since
        // furniture can't store dynamic data to disk. To prevent writing
        // mysteriously appearing for a sign later built here, remove the
        // writing from the submap.
        delete_signage( p );
    } else if( !smash_ter ) {
        // Handle error earlier so that we can assume smash_ter is true below
        debugmsg( "data/json/terrain.json does not have %s.bash.ter_set set!",
                  ter( p ).obj().id.c_str() );
    } else if( has_original_terrain_at( p ) && ter( p )->has_flag( "PHASE_BACK" ) ) {
        const ter_id orig = get_original_terrain_at( p );
        ter_set( p, orig );
        phased = true;
    } else if( params.bashing_from_above && ter_bash.ter_set_bashed_from_above ) {
        // If this terrain is being bashed from above and this terrain
        // has a valid post-destroy bashed-from-above terrain, set it
        ter_set( p, ter_bash.ter_set_bashed_from_above );
    } else if( ter_bash.ter_set && !roof_of_below_tile ) {
        // If the terrain has a valid post-destroy terrain, set it
        ter_set( p, ter_bash.ter_set );
    } else {
        const ter_t &ter_below = ter( below ).obj();
        if( bash->bash_below && ter_below.has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF ) ) {
            // When bashing the tile below, don't allow bashing the floor
            bash_params params_below = params; // Make a copy
            params_below.bashing_from_above = true;
            //the roof tile will be destroyed, so the below tile should also be destroyed
            params_below.destroy = true;
            bash_ter_furn( below, params_below );
        }

        set_to_air = roof_of_below_tile; //do not add the roof for the tile below if it was already removed
        furn_set( p, furn_str_id::NULL_ID() );
        if( repair_missing_ground && ter_below.has_flag( "NATURAL_UNDERGROUND" ) ) {
            ter_set( p, ter_below.roof );
        } else {
            ter_set( p, ter_t_open_air );
        }
    }

    if( !tent ) {
        if( smash_ter ) {
            drop_bash_results( terid, p );
        } else {
            drop_bash_results( furnid, p );
        }
    }
    //regenerates roofs for tiles that should be walkable from above
    if( zlevels && smash_ter && !set_to_air && ter( p )->has_flag( "EMPTY_SPACE" ) &&
        ter( below )->has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF ) && !phased ) {
        const ter_str_id roof = get_roof( below, params.bash_floor && ter( below ).obj().movecost != 0 );
        ter_set( p, roof );
    }

    if( bash->explosive > 0 ) {
        explosion_handler::explosion( nullptr, p, bash->explosive, 0.8, false );
    }

    if( will_collapse && !has_flag( ter_furn_flag::TFLAG_SUPPORTS_ROOF, p ) ) {
        collapse_at( tripoint_bub_ms( p ), params.silent, true, bash->explosive > 0 );
    }

    if( !bash->hit_field.first.is_null() ) {
        add_field( p, bash->hit_field.first, bash->hit_field.second );
    }
    if( !bash->destroyed_field.first.is_null() ) {
        add_field( p, bash->destroyed_field.first, bash->destroyed_field.second );
    }

    params.did_bash = true;
    params.success |= success; // Not always true, so that we can tell when to stop destroying
    params.bashed_solid = true;
    if( !sound.empty() && !params.silent ) {
        sounds::sound( p, sound_volume, sounds::sound_t::combat, sound, false,
                       soundfxid, soundfxvariant );
    }
}

bash_params map::bash( const tripoint_bub_ms &p, const int str,
                       bool silent, bool destroy, bool bash_floor,
                       const vehicle *bashing_vehicle, bool repair_missing_ground )
{
    // Temporary map is bound to a const reference for the call duration; bash returns by value.
    // NOLINTNEXTLINE(clang-analyzer-core.StackAddressEscape)
    return bash( p, {{{damage_bash, str}}}, silent, destroy, bash_floor, bashing_vehicle,
    repair_missing_ground );
}
bash_params map::bash( const tripoint_bub_ms &p, const std::map<damage_type_id, int> &str,
                       bool silent, bool destroy, bool bash_floor,
                       const vehicle *bashing_vehicle, bool repair_missing_ground )
{
    bash_params bsh{
        str, silent, destroy, bash_floor, static_cast<float>( rng_float( 0, 1.0f ) ), false, false, false, false
    };
    if( !inbounds( p ) ) {
        return bsh;
    }

    bool bashed_sealed = false;
    if( has_flag( ter_furn_flag::TFLAG_SEALED, p ) ) {
        bash_ter_furn( p, bsh, repair_missing_ground );
        bashed_sealed = true;
    }

    bash_field( p, bsh );

    // Don't bash items inside terrain/furniture with SEALED flag
    if( !bashed_sealed ) {
        bash_items( p, bsh );
    }
    // Don't bash the vehicle doing the bashing
    const vehicle *veh = veh_pointer_or_null( veh_at( p ) );
    if( veh != nullptr && veh != bashing_vehicle ) {
        bash_vehicle( p, bsh );
    }

    // If we still didn't bash anything solid (a vehicle) or a tile with SEALED flag, bash ter/furn
    if( !bsh.bashed_solid && !bashed_sealed ) {
        bash_ter_furn( p, bsh, repair_missing_ground );
    }

    // If the bash operation modified the tile (solid) then treat it as a
    // deliberate map modification and clear any recorded original terrain
    // to avoid incorrect phase-change reversion.
    if( bsh.did_bash && bsh.bashed_solid ) {
        clear_original_terrain_at( p );
    }

    return bsh;
}

void map::bash_items( const tripoint_bub_ms &p, bash_params &params )
{
    if( !has_items( p ) || has_flag_ter_or_furn( ter_furn_flag::TFLAG_PLANT, p ) ) {
        return;
    }

    std::vector<item> smashed_contents;
    map_stack bashed_items = i_at( p );
    bool smashed_glass = false;
    for( auto bashed_item = bashed_items.begin(); bashed_item != bashed_items.end(); ) {
        // the check for active suppresses Molotovs smashing themselves with their own explosion
        int glass_portion = bashed_item->made_of( material_glass );
        float glass_fraction = glass_portion / static_cast<float>( bashed_item->type->mat_portion_total );
        if( glass_portion && !bashed_item->active && rng_float( 0.0f, 1.0f ) < glass_fraction * 0.5f ) {
            params.did_bash = true;
            smashed_glass = true;
            for( const item *bashed_content : bashed_item->all_items_top() ) {
                smashed_contents.emplace_back( *bashed_content );
            }
            bashed_item = bashed_items.erase( bashed_item );
        } else {
            ++bashed_item;
        }
    }
    // Now plunk in the contents of the smashed items.
    spawn_items( p, smashed_contents );

    // Add a glass sound even when something else also breaks
    if( smashed_glass && !params.silent ) {
        sounds::sound( p, 12, sounds::sound_t::combat, _( "glass shattering." ), false,
                       "smash_success", "smash_glass_contents" );
    }
}

void map::bash_vehicle( const tripoint_bub_ms &p, bash_params &params )
{
    // Smash vehicle if present
    if( const optional_vpart_position vp = veh_at( p ) ) {
        for( const std::pair<const damage_type_id, int> &dam : params.strength ) {
            vp->vehicle().damage( *this, vp->part_index(), dam.second, dam.first );
        }
        if( !params.silent ) {
            sounds::sound( p, 18, sounds::sound_t::combat, _( "crash!" ), false,
                           "smash_success", "hit_vehicle" );
        }

        params.did_bash = true;
        params.success = true;
        params.bashed_solid = true;
    }
}

void map::bash_field( const tripoint_bub_ms &p, bash_params &params )
{
    std::vector<field_type_id> to_remove;
    for( const std::pair<const field_type_id, field_entry> &fd : field_at( p ) ) {
        if( fd.first->bash_info && !fd.first->indestructible ) {
            params.did_bash = true;
            params.bashed_solid = true; // To prevent bashing furniture/vehicles
            to_remove.push_back( fd.first );
        }
    }
    for( field_type_id fd : to_remove ) {
        remove_field( p, fd );
    }
}

void map::drop_bash_results( const map_data_common_t &ter_furn, const tripoint_bub_ms &p )
{
    auto spawn_items_or_charges = []( map & here, const tripoint_bub_ms & p, const itype_id it,
    const int qty ) {
        // there seems to be some way to replace it with add_item_or_charges()
        // but it results in charges spawned in dumb ways, so i dropped it
        if( it->count_by_charges() ) {
            here.spawn_item( p, it, 1, qty );
        } else {
            here.spawn_item( p, it, qty );
        }
    };

    auto drop_components_or_subcomponents = [spawn_items_or_charges](
    const std::vector<std::pair<itype_id, int>> &it_qty_v, const tripoint_bub_ms & p )->void {

        map &here = get_map();

        if( it_qty_v.empty() )
        {
            return;
        }

        add_msg_debug( debugmode::DF_MAP, "drop_bash_results():" );

        for( const std::pair<itype_id, int> &it_qty : it_qty_v )
        {
            const std::vector<item_comp> sub_uncraft_components = item( it_qty.first ).get_uncraft_components();

            if( sub_uncraft_components.empty() ) {
                // if no subcomponents, just straight drop 100% of item count
                spawn_items_or_charges( here, p, it_qty.first->id, it_qty.second );
                add_msg_debug( debugmode::DF_MAP, "type 1, plain item: %s, %s pcs.",
                               it_qty.first->id.c_str(), it_qty.second );
            } else {
                // if subcomponents, drop a bit of subcomponents and a bit of components
                const float broken_qty = rng_float( 0.1f, 0.4f );
                for( const item_comp &sub_comp : sub_uncraft_components ) {
                    const int qty_or_charges_comp =  sub_comp.count * std::round( it_qty.second * broken_qty );
                    spawn_items_or_charges( here, p, sub_comp.type, qty_or_charges_comp );
                    add_msg_debug( debugmode::DF_MAP, "type 2, subcomponent: %s, %s pcs.",
                                   sub_comp.type->id.c_str(), qty_or_charges_comp );
                }
                const int qty_or_charges = std::round( it_qty.second * ( 1 - broken_qty ) );
                spawn_items_or_charges( here, p, it_qty.first->id, qty_or_charges );
                add_msg_debug( debugmode::DF_MAP, "type 3, component: %s, %s pcs.",
                               it_qty.first->id.c_str(), qty_or_charges ) ;
            }
        }
    };

    // todo: do the same component recursive loop for generic salvaging
    if( !ter_furn.base_item.is_null() ) {
        // if has underlying item, take it's uncraft components as bash result

        std::vector<std::pair<itype_id, int>> it_qty_v;
        it_qty_v.reserve( ter_furn.get_uncraft_components().size() );
        for( const item_comp &comp : ter_furn.get_uncraft_components() ) {
            it_qty_v.emplace_back( comp.type, comp.count );
        }
        drop_components_or_subcomponents( it_qty_v, p );
    } else {
        if( ter_furn.bash_info().has_value() ) {
            if( ter_furn.bash_info().value().drop_group == Item_spawn_data_EMPTY_GROUP
                && ter_furn.deconstruct_info().has_value()
                && ter_furn.deconstruct_info().value().drop_group != Item_spawn_data_EMPTY_GROUP ) {
                // if drop_group is not defined manually, and we have deconstruct group
                // then pick deconstruct group as a base, break it a bit, and then drop it.

                const std::map<const itype *, std::pair<int, int>> every_item = item_group::spawn_data_from_group(
                            ter_furn.deconstruct_info().value().drop_group )->every_item_min_max();
                std::vector<std::pair<itype_id, int>> it_qty_v;

                it_qty_v.reserve( every_item.size() );
                for( const std::pair<const itype *, std::pair<int, int>> item_instance : every_item ) {
                    it_qty_v.emplace_back( item_instance.first->id, rng( item_instance.second.first,
                                           item_instance.second.second ) );
                }
                drop_components_or_subcomponents( it_qty_v, p );
            } else {
                map &here = get_map();
                here.spawn_items( p, item_group::items_from( ter_furn.bash_info().value().drop_group,
                                  calendar::turn ) );
            }
        }
    }
}

void map::destroy( const tripoint_bub_ms &p, const bool silent )
{
    // Break if it takes more than 25 destructions to remove to prevent infinite loops
    // Example: A bashes to B, B bashes to A leads to A->B->A->...
    int count = 0;
    while( count <= 25 && bash( p, 999, silent, true ).success ) {
        count++;
    }
    clear_original_terrain_at( p );
}

void map::destroy_furn( const tripoint_bub_ms &p, const bool silent )
{
    // Break if it takes more than 25 destructions to remove to prevent infinite loops
    // Example: A bashes to B, B bashes to A leads to A->B->A->...
    int count = 0;
    while( count <= 25 && furn( p ) != furn_str_id::NULL_ID() &&
           bash( p, 999, silent, true ).success ) {
        count++;
    }

    clear_original_terrain_at( p );
}

void map::destroy_vehicle( const tripoint_bub_ms &p, const bool silent )
{
    if( !veh_at( p ) ) {
        return;
    }

    // Break if it takes more than 25 destructions to remove to prevent infinite loops
    // Example: A bashes to B, B bashes to A leads to A->B->A->...
    int count = 0;
    bash_params bsh{
        {{{damage_bash, 999}}}, silent, true, false, static_cast<float>( rng_float( 0, 1.0f ) ), false, false, false, false
    };
    while( count <= 25 && veh_at( p ) ) {
        bash_vehicle( p, bsh );
        count++;
    }
}

void map::batter( const tripoint_bub_ms &p, int power, int tries, const bool silent )
{
    int count = 0;
    while( count < tries && bash( p, power, silent ).success ) {
        count++;
    }
}

void map::crush( const tripoint_bub_ms &p )
{
    creature_tracker &creatures = get_creature_tracker();
    Character *crushed_player = creatures.creature_at<Character>( this->get_abs( p ) );

    if( crushed_player != nullptr ) {
        bool player_inside = false;
        if( crushed_player->in_vehicle ) {
            const optional_vpart_position vp = veh_at( p );
            player_inside = vp && vp->is_inside();
        }
        // FIXME: Hardcoded damage types
        if( !player_inside ) { //If there's a player at p and he's not in a covered vehicle...
            //This is the roof coming down on top of us, no chance to dodge
            crushed_player->add_msg_player_or_npc( m_bad, _( "You are crushed by the falling debris!" ),
                                                   _( "<npcname> is crushed by the falling debris!" ) );
            // TODO: Make this depend on the ceiling material
            const int dam = rng( 0, 40 );
            // Torso and head take the brunt of the blow
            crushed_player->deal_damage( nullptr, bodypart_id( "head" ), damage_instance( damage_bash,
                                         dam * .25 ) );
            crushed_player->deal_damage( nullptr, bodypart_id( "torso" ), damage_instance( damage_bash,
                                         dam * .45 ) );
            // Legs take the next most through transferred force
            crushed_player->deal_damage( nullptr, bodypart_id( "leg_l" ), damage_instance( damage_bash,
                                         dam * .10 ) );
            crushed_player->deal_damage( nullptr, bodypart_id( "leg_r" ), damage_instance( damage_bash,
                                         dam * .10 ) );
            // Arms take the least
            crushed_player->deal_damage( nullptr, bodypart_id( "arm_l" ), damage_instance( damage_bash,
                                         dam * .05 ) );
            crushed_player->deal_damage( nullptr, bodypart_id( "arm_r" ), damage_instance( damage_bash,
                                         dam * .05 ) );

            // Pin whoever got hit
            crushed_player->add_effect( effect_crushed, 1_turns, true );
            crushed_player->check_dead_state( this );
        }
    }

    if( monster *const monhit = creatures.creature_at<monster>( p ) ) {
        // 25 ~= 60 * .45 (torso)
        monhit->deal_damage( nullptr, bodypart_id( "torso" ), damage_instance( damage_bash, rng( 0,
                             25 ) ) );

        // Pin whoever got hit
        monhit->add_effect( effect_crushed, 1_turns, true );
        monhit->check_dead_state( this );
    }

    if( const optional_vpart_position vp = veh_at( p ) ) {
        // Arbitrary number is better than collapsing house roof crushing APCs
        vp->vehicle().damage( *this, vp->part_index(), rng( 100, 1000 ), damage_bash, false );
    }
}

double map::shoot( const tripoint_bub_ms &p, projectile &proj, const bool hit_items )
{
    if( !inbounds( p ) ) {
        debugmsg( "Called map::shoot on out-of-bounds tile %s", p.to_string() );
        return 0;
    }
    // TODO: make bashing better at destroying, worse at penetrating
    std::map<damage_type_id, float> dmg_by_type {};
    damage_instance &impact = proj.multishot ? proj.shot_impact : proj.impact;
    for( const damage_unit &dam : impact ) {
        dmg_by_type[dam.type] +=
            dam.amount * dam.damage_multiplier * dam.unconditional_damage_mult +
            dam.res_pen * dam.res_mult * dam.unconditional_res_mult;
    }
    const float initial_damage = std::accumulate( dmg_by_type.begin(), dmg_by_type.end(), 0.0f,
    []( float acc, const std::pair<const damage_type_id, float> &dmg ) {
        return acc + dmg.second;
    } );
    if( initial_damage < 0 ) {
        return 0;
    }
    // TODO: use this for more than just vehicle parts
    const damage_type_id &main_damage_type = std::max_element( dmg_by_type.begin(), dmg_by_type.end(),
            []( const std::pair<const damage_type_id, float> &a,
    const std::pair<const damage_type_id, float> &b ) {
        return a.second < b.second;
    } )->first;

    // damage value that may be reduced by vehicles, furniture, terrain or fields
    float dam = initial_damage;

    const auto &ammo_effects = proj.proj_effects;
    const bool incendiary = ammo_effects.count( ammo_effect_INCENDIARY );
    const bool ignite = ammo_effects.count( ammo_effect_IGNITE );
    const bool laser = ammo_effects.count( ammo_effect_LASER );
    bool hit_something = false;

    if( const optional_vpart_position vp = veh_at( p ) ) {
        dam = vp->vehicle().damage( *this, vp->part_index(), dam, main_damage_type, hit_items );
    } else {
        // This lambda is only called if the furniture/terrain has shoot data!
        const auto shoot_furn_ter = [&]( const map_data_common_t &data ) {
            const map_shoot_info &shoot = *data.shoot;
            bool destroyed = false;

            // if you are aiming at this tile, you can never miss
            if( hit_items || x_in_y( shoot.chance_to_hit, 100 ) ) {
                if( laser ) {
                    dam -= rng( shoot.reduce_dmg_min_laser, shoot.reduce_dmg_max_laser );
                } else {
                    dam -= rng( shoot.reduce_dmg_min, shoot.reduce_dmg_max );
                }
                // lasers can't destroy some types of furn/ter you can shoot through
                if( !laser || !shoot.no_laser_destroy ) {
                    // important to use initial damage, energy from reduction has gone into the furn/ter
                    const int min_damage = int( initial_damage ) - shoot.destroy_dmg_min;
                    const int max_damage = shoot.destroy_dmg_max - shoot.destroy_dmg_min;
                    if( x_in_y( min_damage, max_damage ) ) {
                        // don't need to duplicate all the destruction logic here
                        bash_params bsh{ {}, false, true, false, 0.0, false, false, false, false };
                        bash_ter_furn( p, bsh );
                        destroyed = true;
                    }
                }
                if( dam <= 0 && get_player_view().sees( *this,  p ) ) {
                    add_msg( _( "The shot is stopped by the %s!" ), data.name() );
                }
                // only very flammable furn/ter can be set alight with incendiary rounds
                if( data.has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH ) ) {
                    if( incendiary && x_in_y( initial_damage, 100 ) ) {
                        // 1% chance for 1 damage
                        add_field( p, fd_fire, 1 );
                    }
                    if( ignite ) {
                        add_field( p, fd_fire, 1 );
                    }
                }
                // bash_ter_furn already triggers the alarm
                // TODO: fix alarm event weirdness (not just here, also in bash, hack, etc)
                if( !destroyed && data.has_flag( ter_furn_flag::TFLAG_ALARMED ) ) {
                    sounds::sound( p, 40, sounds::sound_t::alarm, _( "an alarm go off!" ),
                                   false, "environment", "alarm" );
                }
                return true;
            }
            return false;
        };

        const furn_id &furniture = furn( p );
        const ter_id &terrain = ter( p );

        // shoot through furniture or terrain and see if we hit something
        if( furniture->shoot ) { // Shoot data is optional, most furniture will never trigger this
            hit_something |= shoot_furn_ter( furniture.obj() );
        } else if( terrain->shoot ) { // Shoot data is optional, most terrain will never trigger this
            hit_something |= shoot_furn_ter( terrain.obj() );
            // fall back to just bashing when shoot data is not defined
        } else if( impassable( p ) && !is_transparent( p ) ) {
            bash( p, dam, false );
            dam = 0;
        }
    }
    dam = std::max( 0.0f, dam );

    // apply trail from ammo effect
    for( const ammo_effect_str_id &ae_str : proj.proj_effects ) {
        for( const trail_field_effect &trail : ae_str.obj().trail_field_types ) {
            if( !trail.field_type.is_null() && x_in_y( trail.chance, 100 ) ) {
                add_field( p, trail.field_type, rng( trail.intensity_min, trail.intensity_max ) );
            }
        }
    }

    // Check fields?
    field &fields_there = field_at( p );
    if( fields_there.field_count() > 0 ) {
        // Need to make a copy since 'remove_field' modifies the value
        field fields_copy = fields_there;
        for( const std::pair<const field_type_id, field_entry> &fd : fields_copy ) {
            const std::optional<map_fd_bash_info> &bash_info = fd.first->bash_info;
            if( bash_info && bash_info->str_min > 0 && !fd.first->indestructible ) {
                if( incendiary ) {
                    add_field( p, fd_fire, fd.second.get_field_intensity() - 1 );
                } else if( dam > 5 + fd.second.get_field_intensity() * 5 &&
                           one_in( 5 - fd.second.get_field_intensity() ) ) {
                    dam -= rng( 1, 2 + fd.second.get_field_intensity() * 2 );
                    remove_field( p, fd.first );
                }
            }
        }
    }

    // Rescale the damage
    double multiplier = 1.0;
    if( dam <= 0 ) {
        impact.clear();
        return 0;
    } else if( dam < initial_damage ) {
        multiplier = dam / static_cast<double>( initial_damage );
    }

    // for now, shooting furniture or terrain protects any items
    if( hit_items && !hit_something ) {
        // Make sure the message is sensible for the ammo effects. Lasers aren't projectiles.
        std::string damage_message;
        if( ammo_effects.count( ammo_effect_LASER ) ) {
            damage_message = _( "laser beam" );
        } else if( ammo_effects.count( ammo_effect_LIGHTNING ) ) {
            damage_message = _( "bolt of electricity" );
        } else if( ammo_effects.count( ammo_effect_PLASMA ) ) {
            damage_message = _( "bolt of plasma" );
        } else {
            damage_message = _( "flying projectile" );
        }

        // Now, smash items on that tile.
        // dam / 3, because bullets aren't all that good at destroying items...
        smash_items( p, dam / 3, damage_message );
    }
    return multiplier;
}

bool map::hit_with_acid( const tripoint_bub_ms &p )
{
    if( passable( p ) ) {
        return false;    // Didn't hit the tile!
    }
    const ter_id &t = ter( p );
    if( t == ter_t_wall_glass || t == ter_t_wall_glass_alarm ||
        t == ter_t_vat ) {
        ter_set( p, ter_t_floor );
    } else if( t == ter_t_door_c || t == ter_t_door_locked || t == ter_t_door_locked_peep ||
               t == ter_t_door_locked_alarm ) {
        if( one_in( 3 ) ) {
            ter_set( p, ter_t_door_b );
        }
    } else if( t == ter_t_door_bar_c || t == ter_t_door_bar_o || t == ter_t_door_bar_locked ||
               t == ter_t_bars ||
               t == ter_t_reb_cage ) {
        ter_set( p, ter_t_floor );
        add_msg_if_player_sees( p, m_warning, _( "The metal bars melt!" ) );
    } else if( t == ter_t_door_b ) {
        if( one_in( 4 ) ) {
            ter_set( p, ter_t_door_frame );
        } else {
            return false;
        }
    } else if( t == ter_t_window || t == ter_t_window_alarm || t == ter_t_window_no_curtains ) {
        ter_set( p, ter_t_window_empty );
    } else if( t == ter_t_wax ) {
        ter_set( p, ter_t_floor_wax );
    } else if( t == ter_t_gas_pump || t == ter_t_gas_pump_smashed ) {
        return false;
    } else if( t == ter_t_card_science || t == ter_t_card_military || t == ter_t_card_industrial ) {
        ter_set( p, ter_t_card_reader_broken );
    }
    return true;
}

// returns true if terrain stops fire
bool map::hit_with_fire( const tripoint_bub_ms &p )
{
    if( passable( p ) ) {
        return false;    // Didn't hit the tile!
    }

    // non passable but flammable terrain, set it on fire
    if( has_flag( ter_furn_flag::TFLAG_FLAMMABLE, p ) ||
        has_flag( ter_furn_flag::TFLAG_FLAMMABLE_ASH, p ) ) {
        add_field( p, fd_fire, 3 );
    }
    return true;
}
