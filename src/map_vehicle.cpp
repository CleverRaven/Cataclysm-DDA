#include "map.h" // IWYU pragma: associated

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>

#include "avatar.h"
#include "calendar.h"
#include "cata_assert.h"
#include "character.h"
#include "character_id.h"
#include "clzones.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "debug.h"
#include "do_turn.h"
#include "enums.h"
#include "game.h"
#include "line.h"
#include "mapdata.h"
#include "math_defines.h"
#include "messages.h"
#include "overmapbuffer.h"
#include "player_activity.h"
#include "power_network.h"
#include "rng.h"
#include "sounds.h"
#include "string_formatter.h"
#include "submap.h"
#include "tileray.h"
#include "translations.h"
#include "ui_manager.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const damage_type_id damage_bash( "bash" );

static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_dirtmound( "t_dirtmound" );
static const ter_str_id ter_t_grass( "t_grass" );

#define dbg(x) DebugLog((x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

VehicleList map::get_vehicles()
{
    if( !zlevels ) {
        return get_vehicles( tripoint_bub_ms( 0, 0, abs_sub.z() ),
                             tripoint_bub_ms( SEEX * my_MAPSIZE, SEEY * my_MAPSIZE, abs_sub.z() ) );
    }

    return get_vehicles( tripoint_bub_ms( 0, 0, -OVERMAP_DEPTH ),
                         tripoint_bub_ms( SEEX * my_MAPSIZE, SEEY * my_MAPSIZE, OVERMAP_HEIGHT ) );
}

bool map::place_vehicle( std::unique_ptr<vehicle> &&new_vehicle )
{
    bool collision = false;
    // This works in the dimension shift case specifically because
    // the old map origin and the new map origin are the same,
    // if that is no longer the case something else will need to happen here.
    tripoint_bub_ms vehicle_origin = new_vehicle->pos_bub( *this );
    for( std::pair<const point_rel_ms, std::vector<int>> &mount_point : new_vehicle->relative_parts ) {
        if( impassable( vehicle_origin + mount_point.first ) ) {
            collision = true;
            break;
        }
    }
    submap *place_on_submap = get_submap_at_grid( new_vehicle->sm_pos - get_abs_sub().xy() );
    if( place_on_submap == nullptr ) {
        debugmsg( "Tried to add vehicle at %s but the submap is not loaded",
                  new_vehicle->sm_pos.to_string() );
    } else {
        place_on_submap->ensure_nonuniform();
        place_on_submap->vehicles.push_back( std::move( new_vehicle ) );
    }
    return collision;
}

void map::rebuild_vehicle_level_caches()
{
    clear_vehicle_level_caches();

    for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
        // Cache all vehicles
        level_cache *ch = get_cache_lazy( gridz );

        if( ch ) {
            for( vehicle * const &elem : ch->vehicle_list ) {
                add_vehicle_to_cache( elem );
            }
        }
    }
}

void map::add_vehicle_to_cache( vehicle *veh )
{
    if( veh == nullptr ) {
        debugmsg( "Tried to add null vehicle to cache" );
        return;
    }

    // Get parts
    for( const vpart_reference &vpr : veh->get_all_parts_with_fakes() ) {
        if( vpr.part().removed ) {
            continue;
        }
        const tripoint_bub_ms p = veh->bub_part_pos( *this, vpr.part() );
        level_cache &ch = get_cache( p.z() );
        ch.set_veh_cached_parts( p, *veh, static_cast<int>( vpr.part_index() ) );
        if( inbounds( p ) ) {
            ch.set_veh_exists_at( p, true );
            set_transparency_cache_dirty( p );
        }
    }
}

void map::clear_vehicle_point_from_cache( vehicle *veh, const tripoint_bub_ms &pt )
{
    if( veh == nullptr ) {
        debugmsg( "Tried to add null vehicle to cache" );
        return;
    }

    level_cache *ch = get_cache_lazy( pt.z() );
    if( ch ) {
        if( inbounds( pt ) ) {
            ch->set_veh_exists_at( pt, false );
        }
        ch->clear_veh_from_veh_cached_parts( pt, veh );
    }
}

void map::clear_vehicle_level_caches( )
{
    for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
        level_cache *ch = get_cache_lazy( gridz );
        if( ch ) {
            ch->clear_vehicle_cache();
        }
    }
}

void map::remove_vehicle_from_cache( vehicle *veh, int zmin, int zmax )
{
    for( int gridz = zmin; gridz <= zmax; gridz++ ) {
        level_cache *const ch = get_cache_lazy( gridz );
        if( ch != nullptr ) {
            ch->vehicle_list.erase( veh );
            ch->zone_vehicles.erase( veh );
        }
        dirty_vehicle_list.erase( veh );
    }
}

namespace
{
void _add_vehicle_to_list( level_cache &ch, vehicle *veh )
{
    ch.vehicle_list.insert( veh );
    if( !veh->loot_zones.empty() ) {
        ch.zone_vehicles.insert( veh );
    }
}
} // namespace

void map::reset_vehicles_sm_pos()
{
    int const zmin = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    int const zmax = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    for( int z = zmin; z <= zmax; z++ ) {
        level_cache &ch = get_cache( z );
        for( int x = 0; x < getmapsize(); x++ ) {
            for( int y = 0; y < getmapsize(); y++ ) {
                tripoint_rel_sm const grid( x, y, z );
                submap *const sm = get_submap_at_grid( grid );
                if( sm != nullptr ) {
                    for( auto const &elem : sm->vehicles ) {
                        // This should be redundant.
                        elem->sm_pos = abs_sub.xy() + grid;
                        add_vehicle_to_cache( &*elem );
                        _add_vehicle_to_list( ch, &*elem );
                    }
                }
            }
        }
    }
}

void map::clear_vehicle_list( const int zlev )
{
    auto *ch = get_cache_lazy( zlev );
    if( ch ) {
        ch->vehicle_list.clear();
        ch->zone_vehicles.clear();
    }
}

void map::update_vehicle_list( const submap *const to, const int zlev )
{
    // Update vehicle data
    level_cache &ch = get_cache( zlev );
    for( const auto &elem : to->vehicles ) {
        _add_vehicle_to_list( ch, elem.get() );
    }
}

std::unique_ptr<vehicle> map::detach_vehicle( vehicle *veh )
{
    if( veh == nullptr ) {
        debugmsg( "map::detach_vehicle was passed nullptr" );
        return std::unique_ptr<vehicle>();
    }

    int z = veh->sm_pos.z();
    if( z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
        debugmsg( "detach_vehicle got a vehicle outside allowed z-level range!  name=%s, submap:%s",
                  veh->name, veh->sm_pos.to_string() );
        // Try to fix by moving the vehicle here
        z = veh->sm_pos.z() = abs_sub.z();
    }

    // Unboard all passengers before detaching
    for( const vpart_reference &part : veh->get_avail_parts( VPFLAG_BOARDABLE ) ) {
        Character *passenger = part.get_passenger();
        if( passenger ) {
            unboard_vehicle( part, passenger );
        }
    }
    veh->invalidate_towing( *this, true );
    submap *const current_submap = get_submap_at_grid( veh->sm_pos - abs_sub.xy() );
    if( current_submap == nullptr ) {
        debugmsg( "Tried to detach vehicle at %s but the submap is not loaded",
                  veh->sm_pos.to_string() );
        return std::unique_ptr<vehicle>();
    }

    level_cache &ch = get_cache( z );
    for( size_t i = 0; i < current_submap->vehicles.size(); i++ ) {
        if( current_submap->vehicles[i].get() == veh ) {
            for( const tripoint_abs_ms &pt : veh->get_points() ) {
                if( inbounds( pt ) ) {
                    memory_cache_dec_set_dirty( get_bub( pt ), true );
                }
                get_avatar().memorize_clear_decoration( pt, "vp_" );
            }
            // vehicle's walls, shelter and floors go with it
            for( const int level : veh->occupied_levels( *this ) ) {
                on_vehicle_moved( level );
            }
            ch.vehicle_list.erase( veh );
            ch.zone_vehicles.erase( veh );
            std::unique_ptr<vehicle> result = std::move( current_submap->vehicles[i] );
            current_submap->vehicles.erase( current_submap->vehicles.begin() + i );
            if( veh->tracking_on ) {
                overmap_buffer.remove_vehicle( veh );
            }
            dirty_vehicle_list.erase( veh );
            rebuild_vehicle_level_caches();
            set_pathfinding_cache_dirty( z );
            return result;
        }
    }
    debugmsg( "detach_vehicle can't find it!  name=%s, submap:%s", veh->name,
              veh->sm_pos.to_string() );
    return std::unique_ptr<vehicle>();
}

void map::destroy_vehicle( vehicle *veh )
{
    detach_vehicle( veh );
}

void map::on_vehicle_moved( const int smz )
{
    set_outside_cache_dirty( smz );
    set_transparency_cache_dirty( smz );
    set_floor_cache_dirty( smz );
    set_floor_cache_dirty( smz + 1 );
    set_pathfinding_cache_dirty( smz );
}

void map::resolve_off_map_grid_generation()
{
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    std::unordered_set<vehicle *> resolved;

    // Build set of in-bubble vehicles for fast membership check
    std::unordered_set<vehicle *> in_bubble;
    for( int zlev = minz; zlev <= maxz; ++zlev ) {
        const level_cache *cache = get_cache_lazy( zlev );
        if( !cache ) {
            continue;
        }
        in_bubble.insert( cache->vehicle_list.begin(), cache->vehicle_list.end() );
    }

    power_network_manager &pnm = g->power_networks();
    pnm.begin_rebuild();

    std::vector<vehicle *> ordered_in_bubble( in_bubble.begin(), in_bubble.end() );
    std::sort( ordered_in_bubble.begin(), ordered_in_bubble.end(),
    []( const vehicle * a, const vehicle * b ) {
        return a->pos_abs() < b->pos_abs();
    } );
    for( vehicle *veh : ordered_in_bubble ) {
        if( resolved.count( veh ) ) {
            continue;
        }
        std::map<vehicle *, float> grid = veh->search_connected_vehicles( *this );
        bool has_off_map_renewables = false;
        for( const auto &[grid_veh, loss] : grid ) {
            resolved.insert( grid_veh );
            if( !in_bubble.count( grid_veh ) &&
                ( !grid_veh->solar_panels.empty() ||
                  !grid_veh->wind_turbines.empty() ||
                  !grid_veh->water_wheels.empty() ) ) {
                has_off_map_renewables = true;
            }
        }

        // Record every grid in the power network manager
        pnm.add_grid( veh, grid );

        if( !has_off_map_renewables ) {
            continue;
        }
        // Process each off-map source individually so charge_battery()
        // computes the correct cable loss path from that source.
        for( const auto &[grid_veh, loss] : grid ) {
            if( in_bubble.count( grid_veh ) ) {
                continue;
            }
            int energy = grid_veh->catchup_off_map_renewables( calendar::turn );
            if( energy > 0 ) {
                grid_veh->charge_battery( *this, energy );
            }
        }
    }

    pnm.finish_rebuild();
}

void map::resolve_appliance_grid_power()
{
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    std::unordered_set<vehicle *> resolved;

    for( int zlev = minz; zlev <= maxz; ++zlev ) {
        const level_cache *cache = get_cache_lazy( zlev );
        if( !cache ) {
            continue;
        }
        for( vehicle *veh : cache->vehicle_list ) {
            if( !veh->is_appliance() ) {
                continue;
            }
            if( resolved.count( veh ) ) {
                continue;
            }
            bool has_power_disabled = false;
            for( const vpart_reference &vp : veh->get_all_parts() ) {
                if( vp.part().power_disabled ) {
                    has_power_disabled = true;
                    break;
                }
            }
            if( !has_power_disabled ) {
                continue;
            }
            // found a candidate -- resolve its entire grid
            std::map<vehicle *, float> grid = veh->search_connected_vehicles( *this );
            int total_charge = 0;
            for( const auto &[grid_veh, loss] : grid ) {
                total_charge += grid_veh->battery_power_level().first;
            }
            if( total_charge > 0 ) {
                for( const auto &[grid_veh, loss] : grid ) {
                    if( !grid_veh->is_appliance() ) {
                        continue;
                    }
                    for( const vpart_reference &vp : grid_veh->get_all_parts() ) {
                        vehicle_part &pt = vp.part();
                        if( !pt.power_disabled ) {
                            continue;
                        }
                        if( pt.is_unavailable() ) {
                            pt.power_disabled = false;
                            continue;
                        }
                        if( pt.enabled ) {
                            // user re-enabled it manually, just clear the flag
                            pt.power_disabled = false;
                            continue;
                        }
                        pt.enabled = true;
                        pt.power_disabled = false;
                    }
                }
            }
            for( const auto &[grid_veh, loss] : grid ) {
                resolved.insert( grid_veh );
            }
        }
    }
}

void map::vehmove()
{
    // give vehicles movement points
    VehicleList vehicle_list;
    int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z();
    int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z();
    const tripoint_abs_ms player_pos = get_player_character().pos_abs();
    for( int zlev = minz; zlev <= maxz; ++zlev ) {
        level_cache *cache_lazy = get_cache_lazy( zlev );
        if( !cache_lazy ) {
            continue;
        }
        level_cache &cache = *cache_lazy;
        for( vehicle *veh : cache.vehicle_list ) {
            if( veh->is_following ) {
                veh->drive_to_local_target( this, player_pos, true );
            } else if( veh->is_patrolling ) {
                veh->autopilot_patrol( this );
            }
            veh->gain_moves( *this );
            veh->slow_leak( *this );
            wrapped_vehicle w;
            w.v = veh;
            vehicle_list.push_back( w );
        }
    }

    // 15 equals 3 >50mph vehicles, or up to 15 slow (1 square move) ones
    // But 15 is too low for V12 death-bikes, let's put 100 here
    for( int count = 0; count < 100; count++ ) {
        if( !vehproceed( vehicle_list ) ) {
            break;
        }
    }
    // Process item removal on the vehicles that were modified this turn.
    // Use a copy because part_removal_cleanup can modify the container.
    auto temp = dirty_vehicle_list;
    for( vehicle * const &elem : temp ) {
        auto same_ptr = [ elem ]( const struct wrapped_vehicle & tgt ) {
            return elem == tgt.v;
        };
        if( std::find_if( vehicle_list.begin(), vehicle_list.end(), same_ptr ) !=
            vehicle_list.end() ) {
            elem->part_removal_cleanup( *this );
        }
    }
    dirty_vehicle_list.clear();
    std::map<vehicle *, bool> vehs; // value true means in on map
    std::unordered_set<vehicle *> connected_vehs;
    for( int zlev = minz; zlev <= maxz; ++zlev ) {
        const level_cache *cache = get_cache_lazy( zlev );
        if( !cache ) {
            continue;
        }
        for( vehicle *veh : cache->vehicle_list ) {
            vehs[veh] = true; // force on map vehicles to true
            veh->get_connected_vehicles( *this, connected_vehs );
        }
    }
    std::vector<vehicle *> ordered_connected( connected_vehs.begin(), connected_vehs.end() );
    std::sort( ordered_connected.begin(), ordered_connected.end(),
    []( const vehicle * a, const vehicle * b ) {
        return a->pos_abs() < b->pos_abs();
    } );
    for( vehicle *connected_veh : ordered_connected ) {
        vehs.emplace( connected_veh, false ); // add with 'false' if does not exist (off map)
    }
    for( const std::pair<vehicle *const, bool> &veh_pair : vehs ) {
        veh_pair.first->process_effects();
        veh_pair.first->recalculate_enchantment_cache();
        veh_pair.first->idle( *this, /* on_map = */ veh_pair.second );
    }

    resolve_off_map_grid_generation();
    resolve_appliance_grid_power();

    // refresh vehicle zones for moved vehicles
    zone_manager::get_manager().cache_vzones( this );
}

bool map::vehproceed( VehicleList &vehicle_list )
{
    wrapped_vehicle *cur_veh = nullptr;
    float max_of_turn = 0.0f;
    // First horizontal movement
    for( wrapped_vehicle &vehs_v : vehicle_list ) {
        if( vehs_v.v->of_turn > max_of_turn ) {
            cur_veh = &vehs_v;
            max_of_turn = cur_veh->v->of_turn;
        }
    }

    // Then vertical-only movement
    if( cur_veh == nullptr ) {
        for( wrapped_vehicle &vehs_v : vehicle_list ) {
            if( vehs_v.v->is_falling || ( vehs_v.v->is_rotorcraft( *this ) &&
                                          vehs_v.v->get_z_change() != 0 ) ) {
                cur_veh = &vehs_v;
                break;
            }
        }
    }

    if( cur_veh == nullptr ) {
        return false;
    }

    cur_veh->v = cur_veh->v->act_on_map( *this );
    if( cur_veh->v == nullptr ) {
        vehicle_list = get_vehicles();
    }

    return true;
}

// TODO: Make reality bubble independent.
static bool sees_veh( const Creature &c, vehicle &veh, bool force_recalc )
{
    const map &here = get_map();

    const std::set<tripoint_abs_ms> &veh_points = veh.get_points( force_recalc );
    return std::any_of( veh_points.begin(), veh_points.end(), [&c,
    &here]( const tripoint_abs_ms & pt ) {
        return c.sees( here, here.get_bub( pt ) );
    } );
}

vehicle *map::move_vehicle( vehicle &veh, const tripoint_rel_ms &dp, const tileray &facing )
{
    if( dp == tripoint_rel_ms::zero ) {
        debugmsg( "Empty displacement vector" );
        return &veh;
    } else if( std::abs( dp.x() ) > 1 || std::abs( dp.y() ) > 1 || std::abs( dp.z() ) > 1 ) {
        debugmsg( "Invalid displacement vector: %d, %d, %d", dp.x(), dp.y(), dp.z() );
        return &veh;
    }
    // Split the movement into horizontal and vertical for easier processing
    if( dp.xy() != point_rel_ms::zero && dp.z() != 0 ) {
        vehicle *const new_pointer = move_vehicle( veh, tripoint_rel_ms( dp.xy(), 0 ), facing );
        if( !new_pointer ) {
            return nullptr;
        }

        vehicle *const result = move_vehicle( *new_pointer, tripoint_rel_ms( 0, 0, dp.z() ), facing );
        if( !result ) {
            return nullptr;
        }

        result->is_falling = false;
        return result;
    }
    const bool vertical = dp.z() != 0;
    // Ensured by the splitting above
    cata_assert( vertical == ( dp.xy() == point_rel_ms::zero ) );

    const int target_z = dp.z() + veh.sm_pos.z();
    // limit vehicles to at most OVERMAP_HEIGHT - 1; this mitigates getting to zlevel 10 easily
    // and causing `get_cache_ref( zlev + 1 );` call in map::build_sunlight_cache overflowing
    if( target_z < -OVERMAP_DEPTH || target_z > OVERMAP_HEIGHT - 1 ) {
        return &veh;
    }

    veh.precalc_mounts( 1, veh.skidding ? veh.turn_dir : facing.dir(), veh.pivot_point( *this ) );

    // cancel out any movement of the vehicle due only to a change in pivot
    tripoint_rel_ms dp1 = dp - veh.pivot_displacement();

    int impulse = 0;

    std::vector<veh_collision> collisions;

    // Find collisions
    // Velocity of car before collision
    // Split into vertical and horizontal movement
    const int &coll_velocity = vertical ? veh.vertical_velocity : veh.velocity;
    const int velocity_before = coll_velocity;
    if( velocity_before == 0 && !veh.is_rotorcraft( *this ) && !veh.is_flying_in_air() ) {
        debugmsg( "%s tried to move %s with no velocity",
                  veh.name, vertical ? "vertically" : "horizontally" );
        return &veh;
    }

    bool veh_veh_coll_flag = false;
    // Try to collide multiple times
    size_t collision_attempts = 10;
    do {
        collisions.clear();
        veh.collision( *this, collisions, dp1, false );

        // Vehicle collisions
        std::map<vehicle *, std::vector<veh_collision> > veh_collisions;
        for( veh_collision &coll : collisions ) {
            if( coll.type != veh_coll_veh ) {
                continue;
            }

            veh_veh_coll_flag = true;
            // Only collide with each vehicle once
            veh_collisions[static_cast<vehicle *>( coll.target )].push_back( coll );
        }

        for( auto &pair : veh_collisions ) {
            impulse += vehicle_vehicle_collision( veh, *pair.first, pair.second );
        }

        // Non-vehicle collisions
        for( const veh_collision &coll : collisions ) {
            if( coll.type == veh_coll_veh ) {
                continue;
            }
            int part_num = veh.get_non_fake_part( coll.part );

            const point_rel_ms &collision_point = veh.part( coll.part ).mount;
            const int coll_dmg = coll.imp;

            // Shock damage, if the target part is a rotor treat as an aimed hit.
            // don't try to deal damage to invalid part (probably removed or destroyed)
            if( part_num != -1 ) {
                if( veh.part( part_num ).info().has_flag( VPFLAG_ROTOR ) ) {
                    veh.damage( *this, part_num, coll_dmg, damage_bash, true );
                } else {
                    impulse += coll_dmg;
                    veh.damage( *this, part_num, coll_dmg, damage_bash );
                    veh.damage_all( *this, coll_dmg / 2, coll_dmg, damage_bash, collision_point );
                }
            }
        }

        // prevent vehicle bouncing after the first collision
        if( vertical && velocity_before < 0 && coll_velocity > 0 ) {
            veh.vertical_velocity = 0; // also affects `coll_velocity` and thus exits the loop
        }

    } while( collision_attempts-- > 0 && coll_velocity != 0 &&
             sgn( coll_velocity ) == sgn( velocity_before ) &&
             !collisions.empty() && !veh_veh_coll_flag );

    const int velocity_after = coll_velocity;
    bool can_move = velocity_after != 0 && sgn( velocity_after ) == sgn( velocity_before );
    if( dp.z() != 0 && veh.is_rotorcraft( *this ) ) {
        can_move = true;
    }
    if( vertical ) {
        veh.check_falling_or_floating(); // fix for vehicle falling into floor
    }
    units::angle coll_turn = 0_degrees;
    if( impulse > 0 ) {
        coll_turn = shake_vehicle( veh, velocity_before, facing.dir() );
        veh.stop_autodriving();
        const int volume = std::min<int>( 100, std::sqrt( impulse ) );
        // TODO: Center the sound at weighted (by impulse) average of collisions
        sounds::sound( veh.pos_bub( *this ), volume, sounds::sound_t::combat, _( "crash!" ),
                       false, "smash_success", "hit_vehicle" );
    }

    if( veh_veh_coll_flag ) {
        // Break here to let the hit vehicle move away
        return nullptr;
    }

    // If not enough wheels, mess up the ground a bit.
    if( !vertical && !veh.valid_wheel_config( *this ) && !( veh.is_watercraft() &&
            veh.can_float( *this ) ) &&
        !veh.is_flying_in_air() && dp.z() == 0 ) {
        veh.velocity -= std::clamp( veh.velocity, -2000, 2000 ); // extra drag
        for( const tripoint_abs_ms &p : veh.get_points() ) {
            const tripoint_bub_ms pos = get_bub( p );
            const ter_id &pter = ter( pos );
            if( pter == ter_t_dirt || pter == ter_t_grass ) {
                ter_set( pos, ter_t_dirtmound );
            }
        }
    }

    const units::angle last_turn_dec = 1_degrees;
    if( veh.last_turn < 0_degrees ) {
        veh.last_turn += last_turn_dec;
        if( veh.last_turn > -last_turn_dec ) {
            veh.last_turn = 0_degrees;
        }
    } else if( veh.last_turn > 0_degrees ) {
        veh.last_turn -= last_turn_dec;
        if( veh.last_turn < last_turn_dec ) {
            veh.last_turn = 0_degrees;
        }
    }

    Character &player_character = get_player_character();
    const bool seen = sees_veh( player_character, veh, false );

    if( can_move || ( vertical && veh.is_falling ) ) {
        // Accept new direction
        if( veh.skidding ) {
            veh.face.init( veh.turn_dir );
        } else {
            veh.face = facing;
        }

        veh.move = facing;
        if( coll_turn != 0_degrees ) {
            veh.skidding = true;
            veh.turn( coll_turn );
        }
        veh.on_move( *this );
        // Actually change position
        displace_vehicle( veh, dp1 );
        level_vehicle( veh );
    } else if( !vertical ) {
        veh.stop( *this );
    }
    veh.check_falling_or_floating();
    // If the PC is in the currently moved vehicle, adjust the
    //  view offset.
    if( player_character.controlling_vehicle &&
        veh_pointer_or_null( veh_at( player_character.pos_bub() ) ) == &veh ) {
        g->calc_driving_offset( &veh );
        if( veh.skidding && can_move ) {
            // TODO: Make skid recovery in air hard
            veh.possibly_recover_from_skid();
        }
    }
    // Now we're gonna handle traps we're standing on (if we're still moving).
    if( !vertical && can_move ) {
        const auto wheel_indices = veh.wheelcache; // Don't use a reference here, it causes a crash.

        // Values to deal with crushing items.
        // The math needs to be floating-point to work, so the values might as well be.
        const float vehicle_grounded_wheel_area = static_cast<int>( vehicle_wheel_traction( veh, true ) );
        const float weight_to_damage_factor = 0.05f; // Nobody likes a magic number.
        const float vehicle_mass_kg = to_kilogram( veh.total_mass( *this ) );

        for( const int vp_wheel_idx : wheel_indices ) {
            vehicle_part &vp_wheel = veh.part( vp_wheel_idx );
            const tripoint_bub_ms wheel_p = veh.bub_part_pos( *this, vp_wheel );
            if( one_in( 2 ) && displace_water( wheel_p ) ) {
                sounds::sound( wheel_p, 4, sounds::sound_t::movement, _( "splash!" ), false,
                               "environment", "splash" );
            }

            veh.handle_trap( this, wheel_p, vp_wheel );
            // dont use vp_wheel or vp_wheel_idx below this - handle_trap might've removed it from parts

            if( has_items( wheel_p ) && !has_flag( ter_furn_flag::TFLAG_SEALED, wheel_p ) &&
                !has_flag( ter_furn_flag::TFLAG_DEEP_WATER, wheel_p ) ) {
                // Damage is calculated based on the weight of the vehicle,
                // The area of it's wheels, and the area of the wheel running over the items.
                // This number is multiplied by weight_to_damage_factor to get reasonable results, damage-wise.
                const int wheel_damage = vp_wheel.contact_area() / vehicle_grounded_wheel_area *
                                         vehicle_mass_kg * weight_to_damage_factor;

                //~ %1$s: vehicle name
                smash_items( wheel_p, wheel_damage, string_format( _( "weight of %1$s" ), veh.disp_name() ),
                             &vp_wheel, &veh );
            }
        }
    }
    if( veh.is_towing() ) {
        veh.do_towing_move( *this );
        // veh.do_towing_move() may cancel towing, so we need to recheck is_towing here
        if( veh.is_towing() && veh.tow_data.get_towed()->tow_cable_too_far() ) {
            add_msg( m_info, _( "A towing cable snaps off of %s." ),
                     veh.tow_data.get_towed()->disp_name() );
            veh.tow_data.get_towed()->invalidate_towing( *this, true );
        }
    }
    // Redraw scene, but only if the player is not engaged in an activity and
    // the vehicle was seen before or after the move.
    if( !player_character.activity && ( seen || sees_veh( player_character, veh, true ) ) ) {
        g->invalidate_main_ui_adaptor();
        ui_manager::redraw_invalidated();
        handle_key_blocking_activity();
    }
    return &veh;
}

float map::vehicle_vehicle_collision( vehicle &veh, vehicle &veh2,
                                      const std::vector<veh_collision> &collisions )
{
    if( &veh == &veh2 ) {
        debugmsg( "Vehicle %s collided with itself", veh.name );
        return 0.0f;
    }

    // Effects of colliding with another vehicle:
    //  transfers of momentum, skidding,
    //  parts are damaged/broken on both sides,
    //  remaining times are normalized
    const veh_collision &c = collisions[0];
    const bool vertical = veh.sm_pos.z() != veh2.sm_pos.z();

    if( c.part < 0 || c.part >= static_cast<int>( veh.part_count() ) ) {
        debugmsg( "invalid c.part %d", c.part );
        return 0.0f;
    }

    if( c.target_part < 0 || c.target_part >= static_cast<int>( veh2.part_count() ) ) {
        debugmsg( "invalid c.target_part %d", c.target_part );
        return 0.0f;
    }
    vehicle_part &vp1 = veh.part( c.part );
    vehicle_part &vp2 = veh2.part( c.target_part );

    // Check whether avatar sees the collision, and log a message if so
    const avatar &you = get_avatar();
    const tripoint_bub_ms part1_pos = veh.bub_part_pos( *this, vp1 );
    const tripoint_bub_ms part2_pos = veh2.bub_part_pos( *this, vp2 );
    if( you.sees( *this, part1_pos ) || you.sees( *this, part2_pos ) ) {
        //~ %1$s: first vehicle name (without "the")
        //~ %2$s: first part name
        //~ %3$s: second vehicle display name (with "the")
        //~ %4$s: second part name
        add_msg( m_bad, _( "The %1$s's %2$s collides with %3$s's %4$s." ),
                 veh.name,  vp1.info().name(),
                 veh2.disp_name(), vp2.info().name() );
    }

    // Used to calculate the epicenter of the collision.
    point_rel_ms epicenter1;
    point_rel_ms epicenter2;

    float dmg;
    // Vertical collisions will be simpler for a while (1D)
    if( !vertical ) {
        // For reference, a cargo truck weighs ~25300, a bicycle 690,
        //  and 38mph is 3800 'velocity'
        rl_vec2d velo_veh1 = veh.velo_vec();
        rl_vec2d velo_veh2 = veh2.velo_vec();
        const float m1 = to_kilogram( veh.total_mass( *this ) );
        const float m2 = to_kilogram( veh2.total_mass( *this ) );
        //Energy of vehicle1 and vehicle2 before collision
        float E = 0.5 * m1 * velo_veh1.magnitude() * velo_veh1.magnitude() +
                  0.5 * m2 * velo_veh2.magnitude() * velo_veh2.magnitude();

        // Collision_axis
        point_rel_ms cof1 = veh .rotated_center_of_mass( *this );
        point_rel_ms cof2 = veh2.rotated_center_of_mass( *this );
        int &x_cof1 = cof1.x();
        int &y_cof1 = cof1.y();
        int &x_cof2 = cof2.x();
        int &y_cof2 = cof2.y();
        rl_vec2d collision_axis_y;

        const tripoint_bub_ms veh_pos = veh.pos_bub( *this );
        const tripoint_bub_ms veh2_pos = veh2.pos_bub( *this );
        collision_axis_y.x = ( veh_pos.x() + x_cof1 ) - ( veh2_pos.x() + x_cof2 );
        collision_axis_y.y = ( veh_pos.y() + y_cof1 ) - ( veh2_pos.y() + y_cof2 );
        collision_axis_y = collision_axis_y.normalized();
        rl_vec2d collision_axis_x = collision_axis_y.rotated( M_PI / 2 );
        // imp? & delta? & final? reworked:
        // newvel1 =( vel1 * ( mass1 - mass2 ) + ( 2 * mass2 * vel2 ) ) / ( mass1 + mass2 )
        // as per http://en.wikipedia.org/wiki/Elastic_collision
        //velocity of veh1 before collision in the direction of collision_axis_y
        float vel1_y = collision_axis_y.dot_product( velo_veh1 );
        float vel1_x = collision_axis_x.dot_product( velo_veh1 );
        //velocity of veh2 before collision in the direction of collision_axis_y
        float vel2_y = collision_axis_y.dot_product( velo_veh2 );
        float vel2_x = collision_axis_x.dot_product( velo_veh2 );
        // e = 0 -> inelastic collision
        // e = 1 -> elastic collision
        float e = get_collision_factor( vel1_y / 100 - vel2_y / 100 );

        // Velocity after collision
        // vel1_x_a = vel1_x, because in x-direction we have no transmission of force
        float vel1_x_a = vel1_x;
        float vel2_x_a = vel2_x;
        // Transmission of force only in direction of collision_axix_y
        // Equation: partially elastic collision
        float vel1_y_a = ( m2 * vel2_y * ( 1 + e ) + vel1_y * ( m1 - m2 * e ) ) / ( m1 + m2 );
        float vel2_y_a = ( m1 * vel1_y * ( 1 + e ) + vel2_y * ( m2 - m1 * e ) ) / ( m1 + m2 );
        // Add both components; Note: collision_axis is normalized
        rl_vec2d final1 = collision_axis_y * vel1_y_a + collision_axis_x * vel1_x_a;
        rl_vec2d final2 = collision_axis_y * vel2_y_a + collision_axis_x * vel2_x_a;

        veh.move.init( point_rel_ms( final1.as_point() ) );
        if( final1.dot_product( veh.face_vec() ) < 0 ) {
            // Car is being pushed backwards. Make it move backwards
            veh.velocity = -final1.magnitude();
        } else {
            veh.velocity = final1.magnitude();
        }

        veh2.move.init( point_rel_ms( final2.as_point() ) );
        if( final2.dot_product( veh2.face_vec() ) < 0 ) {
            // Car is being pushed backwards. Make it move backwards
            veh2.velocity = -final2.magnitude();
        } else {
            veh2.velocity = final2.magnitude();
        }

        //give veh2 the initiative to proceed next before veh1
        float avg_of_turn = ( veh2.of_turn + veh.of_turn ) / 2.0f;
        if( avg_of_turn < 0.1f ) {
            avg_of_turn = 0.1f;
        }

        veh.of_turn = avg_of_turn * 0.9f;
        veh2.of_turn = avg_of_turn * 1.1f;

        //Energy after collision
        float E_a = 0.5 * m1 * final1.magnitude() * final1.magnitude() +
                    0.5 * m2 * final2.magnitude() * final2.magnitude();
        float d_E = E - E_a;  //Lost energy at collision -> deformation energy
        dmg = std::abs( d_E / 1000 / 2000 );  //adjust to balance damage
    } else {
        const float m1 = to_kilogram( veh.total_mass( *this ) );
        // Collision is perfectly inelastic for simplicity
        // Assume veh2 is standing still
        dmg = std::abs( veh.vertical_velocity / 100.0f ) * m1 / 10;
        veh.vertical_velocity = 0;
    }

    float dmg_veh1 = dmg * 0.5f;
    float dmg_veh2 = dmg * 0.5f;

    int coll_parts_cnt = 0; //quantity of colliding parts between veh1 and veh2
    for( const veh_collision &veh_veh_coll : collisions ) {
        if( &veh2 == static_cast<vehicle *>( veh_veh_coll.target ) ) {
            coll_parts_cnt++;
        }
    }

    // Prevent potential division by zero
    if( coll_parts_cnt == 0 ) {
        debugmsg( "Vehicle %s collided with bugs in the code", veh.name );
        return 0.0f;
    }

    const float dmg1_part = dmg_veh1 / coll_parts_cnt;
    const float dmg2_part = dmg_veh2 / coll_parts_cnt;

    //damage colliding parts (only veh1 and veh2 parts)
    for( const veh_collision &veh_veh_coll : collisions ) {
        if( &veh2 != static_cast<vehicle *>( veh_veh_coll.target ) ) {
            continue;
        }
        int coll_part = veh.get_non_fake_part( veh_veh_coll.part );
        int target_part = veh2.get_non_fake_part( veh_veh_coll.target_part );

        int coll_parm = veh.part_with_feature( coll_part, VPFLAG_ARMOR, true );
        if( coll_parm < 0 ) {
            coll_parm = coll_part;
        }
        int target_parm = veh2.part_with_feature( target_part, VPFLAG_ARMOR, true );
        if( target_parm < 0 ) {
            target_parm = target_part;
        }

        epicenter1 += veh.part( coll_parm ).mount;
        veh.damage( *this, coll_parm, dmg1_part, damage_bash );

        epicenter2 += veh2.part( target_parm ).mount;
        veh2.damage( *this, target_parm, dmg2_part, damage_bash );
    }

    epicenter2.x() /= coll_parts_cnt;
    epicenter2.y() /= coll_parts_cnt;

    if( dmg2_part > 100 ) {
        // Shake vehicle because of collision
        veh2.damage_all( *this, dmg2_part / 2, dmg2_part, damage_bash, epicenter2 );
    }

    if( dmg_veh1 > 800 ) {
        veh.skidding = true;
    }

    if( dmg_veh2 > 800 ) {
        veh2.skidding = true;
    }

    // Return the impulse of the collision
    return dmg_veh1;
}

bool map::check_vehicle_zones( const int zlev )
{
    for( vehicle *veh : get_cache( zlev ).zone_vehicles ) {
        if( veh->zones_dirty ) {
            return true;
        }
    }
    return false;
}

std::vector<zone_data *> map::get_vehicle_zones( const int zlev )
{
    std::vector<zone_data *> veh_zones;
    bool rebuild = false;
    for( vehicle *veh : get_cache( zlev ).zone_vehicles ) {
        if( veh->refresh_zones() ) {
            rebuild = true;
        }
        for( auto &zone : veh->loot_zones ) {
            veh_zones.emplace_back( &zone.second );
        }
    }
    if( rebuild ) {
        zone_manager::get_manager().cache_vzones();
    }
    return veh_zones;
}

void map::register_vehicle_zone( vehicle *veh, const int zlev )
{
    level_cache &ch = get_cache( zlev );
    ch.zone_vehicles.insert( veh );
}

bool map::deregister_vehicle_zone( zone_data &zone ) const
{
    const tripoint_bub_ms pos = get_bub( zone.get_start_point() );
    if( const std::optional<vpart_reference> vp = veh_at( pos ).cargo() ) {
        vehicle &veh = vp->vehicle();
        auto bounds = veh.loot_zones.equal_range( vp->mount_pos() );
        for( auto it = bounds.first; it != bounds.second; it++ ) {
            if( &zone == &( it->second ) ) {
                veh.loot_zones.erase( it );
                return true;
            }
        }
    }
    return false;
}

std::set<tripoint_bub_ms> map::get_moving_vehicle_targets( const Creature &z, int max_range )
{
    const tripoint_bub_ms zpos( z.pos_bub() );
    std::set<tripoint_bub_ms> priority;
    std::set<tripoint_bub_ms> visible;
    for( wrapped_vehicle &v : get_vehicles() ) {
        if( !v.v->is_moving() ) {
            continue;
        }
        if( v.v->get_driver( *this ) != nullptr &&
            z.attitude_to( *v.v->get_driver( *this ) ) != Creature::Attitude::HOSTILE ) {
            continue;
        }
        if( std::abs( v.pos.z() - zpos.z() ) > fov_3d_z_range ) {
            continue;
        }
        if( rl_dist( zpos, tripoint_bub_ms( v.pos ) ) > max_range + 40 ) {
            continue; // coarse distance filter, 40 = ~24 * sqrt(2) - rough max diameter of a vehicle
        }
        for( const vpart_reference &vpr : v.v->get_all_parts() ) {
            const tripoint_bub_ms vppos = vpr.pos_bub( *this );
            if( rl_dist( zpos, vppos ) > max_range ) {
                continue;
            }
            if( !z.sees( *this, vppos ) ) {
                continue;
            }
            if( vpr.has_feature( VPFLAG_CONTROLS ) ||
                vpr.has_feature( VPFLAG_ENGINE ) ||
                vpr.has_feature( VPFLAG_WHEEL ) ) {
                priority.emplace( vppos );
            } else {
                visible.emplace( vppos );
            }
        }
    }
    return !priority.empty() ? priority : visible;
}

// 3D vehicle functions

VehicleList map::get_vehicles( const tripoint_bub_ms &start, const tripoint_bub_ms &end )
{
    const int chunk_sx = std::max( 0, ( start.x() / SEEX ) - 1 );
    const int chunk_ex = std::min( my_MAPSIZE - 1, ( end.x() / SEEX ) + 1 );
    const int chunk_sy = std::max( 0, ( start.y() / SEEY ) - 1 );
    const int chunk_ey = std::min( my_MAPSIZE - 1, ( end.y() / SEEY ) + 1 );
    const int chunk_sz = start.z();
    const int chunk_ez = end.z();
    VehicleList vehs;

    for( int cx = chunk_sx; cx <= chunk_ex; ++cx ) {
        for( int cy = chunk_sy; cy <= chunk_ey; ++cy ) {
            for( int cz = chunk_sz; cz <= chunk_ez; ++cz ) {
                submap *current_submap = get_submap_at_grid( tripoint_rel_sm{ cx, cy, cz } );
                if( current_submap == nullptr ) {
                    debugmsg( "Tried to process vehicle at (%d,%d,%d) but the submap is not loaded", cx, cy, cz );
                    continue;
                }
                for( const auto &elem : current_submap->vehicles ) {
                    // Ensure the vehicle z-position is correct
                    elem->sm_pos.z() = cz;
                    wrapped_vehicle w;
                    w.v = elem.get();
                    w.pos = w.v->pos_bub( *this );
                    vehs.push_back( w );
                }
            }
        }
    }

    return vehs;
}

optional_vpart_position map::veh_at( const tripoint_abs_ms &p ) const
{
    return veh_at( get_bub( p ) );
}

optional_vpart_position map::veh_at( const tripoint_bub_ms &p ) const
{
    if( !inbounds( p ) || !const_cast<map *>( this )->get_cache( p.z() ).get_veh_in_active_range() ) {
        return optional_vpart_position( std::nullopt );
    }

    int part_num = 1;
    vehicle *const veh = const_cast<map *>( this )->veh_at_internal( p, part_num );
    if( !veh ) {
        return optional_vpart_position( std::nullopt );
    }
    return optional_vpart_position( vpart_position( *veh, part_num ) );
}

const vehicle *map::veh_at_internal( const tripoint_bub_ms &p, int &part_num ) const
{
    // This function is called A LOT. Move as much out of here as possible.
    const level_cache &ch = get_cache( p.z() );
    if( !ch.get_veh_in_active_range() || !ch.get_veh_exists_at( p ) ) {
        part_num = -1;
        return nullptr; // Clear cache indicates no vehicle. This should optimize a great deal.
    }

    std::pair<vehicle *, int> ret = ch.get_veh_cached_parts( p );
    if( ret.first ) {
        part_num = ret.second;
        return ret.first;
    }

    debugmsg( "vehicle part cache indicated vehicle not found: %d %d %d", p.x(), p.y(), p.z() );
    part_num = -1;
    return nullptr;
}


vehicle *map::veh_at_internal( const tripoint_bub_ms &p, int &part_num )
{
    return const_cast<vehicle *>( const_cast<const map *>( this )->veh_at_internal( p, part_num ) );
}

void map::board_vehicle( const tripoint_bub_ms &pos, Character *p )
{
    if( p == nullptr ) {
        debugmsg( "map::board_vehicle: null player" );
        return;
    }

    const std::optional<vpart_reference> vp = veh_at( pos ).part_with_feature( VPFLAG_BOARDABLE,
            true );
    if( !vp ) {
        avatar *player_character = p->as_avatar();
        if( player_character != nullptr &&
            player_character->grab_point.x() == 0 && player_character->grab_point.y() == 0 ) {
            debugmsg( "map::board_vehicle: vehicle with unbroken and BOARDABLE part not found" );
        }
        return;
    }
    if( vp->part().has_flag( vp_flag::passenger_flag ) ) {
        Character *psg = vp->vehicle().get_passenger( vp->part_index() );
        debugmsg( "map::board_vehicle: %s failed to board passenger (%s) is already there",
                  p ? p->get_name() : "<null_boarder>",
                  psg ? psg->get_name() : "<null_passenger>" );
        unboard_vehicle( pos );
    }
    vp->part().set_flag( vp_flag::passenger_flag );
    vp->part().passenger_id = p->getID();
    vp->vehicle().invalidate_mass();

    p->setpos( *this, pos );
    p->in_vehicle = true;
    if( p->is_avatar() ) {
        g->update_map( *p->as_avatar() );
    }
}

void map::unboard_vehicle( const vpart_reference &vp, Character *passenger, bool dead_passenger )
{
    // Mark the part as un-occupied regardless of whether there's a live passenger here.
    vp.part().remove_flag( vp_flag::passenger_flag );
    vp.vehicle().invalidate_mass();

    if( !passenger ) {
        if( !dead_passenger ) {
            debugmsg( "map::unboard_vehicle: passenger not found" );
        }
        return;
    }
    passenger->in_vehicle = false;
    if( passenger->controlling_vehicle ) {
        // If the driver left, stop autodriving.
        vp.vehicle().stop_autodriving( false );
        // Only make vehicle go out of control if the driver is the one unboarding.
        vp.vehicle().skidding = true;
    }
    passenger->controlling_vehicle = false;
}

void map::unboard_vehicle( const tripoint_bub_ms &p, bool dead_passenger )
{
    const std::optional<vpart_reference> vp = veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, false );
    Character *passenger = nullptr;
    if( !vp ) {
        debugmsg( "map::unboard_vehicle: vehicle not found" );
        // Try and force unboard the player anyway.
        passenger = get_creature_tracker().creature_at<Character>( p );
        if( passenger ) {
            passenger->in_vehicle = false;
            passenger->controlling_vehicle = false;
        }
        return;
    }
    passenger = vp->get_passenger();
    unboard_vehicle( *vp, passenger, dead_passenger );
}

bool map::displace_vehicle( vehicle &veh, const tripoint_rel_ms &dp, const bool adjust_pos,
                            const std::set<int> &parts_to_move )
{
    const tripoint_bub_ms src = veh.pos_bub( *this );
    // handle vehicle ramps
    int ramp_offset = 0;
    if( adjust_pos ) {
        if( has_flag( ter_furn_flag::TFLAG_RAMP_UP, src + dp ) ) {
            ramp_offset += 1;
            veh.is_on_ramp = true;
        } else if( has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, src + dp ) ) {
            ramp_offset -= 1;
            veh.is_on_ramp = true;
        }
    }

    const tripoint_bub_ms dst =
        src + ( adjust_pos ? ( dp + tripoint( 0, 0, ramp_offset ) ) : tripoint_rel_ms::zero );

    if( !inbounds( src ) ) {
        add_msg_debug( debugmode::DF_MAP,
                       "map::displace_vehicle: coordinates out of bounds %d,%d,%d->%d,%d,%d",
                       src.x(), src.y(), src.z(), dst.x(), dst.y(), dst.z() );
        return false;
    }

    point_sm_ms src_offset;
    point_sm_ms dst_offset;
    submap *src_submap = get_submap_at( src, src_offset );
    submap *const dst_submap = get_submap_at( dst, dst_offset );
    if( src_submap == nullptr || dst_submap == nullptr ) {
        debugmsg( "Tried to displace vehicle at (%d,%d) but the submap is not loaded", src_offset.x(),
                  src_offset.y() );
        return true;
    }
    std::set<int> smzs;

    // first, let's find our position in current vehicles vector
    size_t our_i = 0;
    bool found = false;
    for( submap *&smap : grid ) {
        for( size_t i = 0; i < smap->vehicles.size(); i++ ) {
            if( smap->vehicles[i].get() == &veh ) {
                our_i = i;
                src_submap = smap;
                found = true;
                break;
            }
        }
        if( found ) {
            break;
        }
    }

    if( !found ) {
        add_msg_debug( debugmode::DF_MAP, "displace_vehicle [%s] failed", veh.name );
        return false;
    }

    // move the vehicle
    // don't let it go off grid
    if( !inbounds( dst ) ) {
        veh.stop( *this );
        // Silent debug
        dbg( D_ERROR ) << "map:displace_vehicle: Stopping vehicle, displaced dp=("
                       << dp.x() << ", " << dp.y() << ", " << dp.z() << ")";
        return true;
    }

    memory_clear_vehicle_points( veh );

    Character &player_character = get_player_character();
    // Need old coordinates to check for remote control
    const bool remote = veh.remote_controlled( player_character );

    // record every passenger and pet inside
    std::vector<rider_data> riders = veh.get_riders();

    bool need_update = false;
    int z_change = 0;
    // Move passengers and pets
    bool complete = false;
    creature_tracker &creatures = get_creature_tracker();
    // loop until everyone has moved or for each passenger
    for( size_t i = 0; !complete && i < riders.size(); i++ ) {
        complete = true;
        for( rider_data &r : riders ) {
            if( r.moved ) {
                continue;
            }
            const int prt = r.prt;
            if( !parts_to_move.empty() && parts_to_move.find( prt ) == parts_to_move.end() ) {
                r.moved = true;
                continue;
            }
            Creature *psg = r.psg;
            const tripoint_bub_ms part_pos = veh.bub_part_pos( *this, prt );
            if( psg == nullptr ) {
                debugmsg( "Empty passenger for part #%d at %s player at %s?",
                          prt, part_pos.to_string_writable(),
                          player_character.pos_bub( *this ).to_string_writable() );
                veh.part( prt ).remove_flag( vp_flag::passenger_flag );
                r.moved = true;
                continue;
            }

            if( psg->pos_bub() != part_pos ) {
                add_msg_debug( debugmode::DF_MAP, "Part/passenger position mismatch: part #%d at %s "
                               "passenger at %s", prt, part_pos.to_string_writable(),
                               psg->pos_bub( *this ).to_string_writable() );
            }
            const vehicle_part &veh_part = veh.part( prt );

            // ramps make everything super tricky
            int psg_offset_z = -ramp_offset;
            tripoint_rel_ms next_pos; // defaults to 0,0,0
            if( parts_to_move.empty() ) {
                next_pos = veh_part.precalc[1];
            }
            if( has_flag( ter_furn_flag::TFLAG_RAMP_UP, src + dp + next_pos ) ) {
                psg_offset_z += 1;
            } else if( has_flag( ter_furn_flag::TFLAG_RAMP_DOWN, src + dp + next_pos ) ) {
                psg_offset_z -= 1;
            }

            // Place passenger on the new part location
            tripoint_bub_ms psgp( dst + next_pos + tripoint( 0, 0, psg_offset_z ) );
            // someone is in the way so try again
            if( creatures.creature_at( psgp ) ) {
                complete = false;
                continue;
            }
            if( psg->is_avatar() ) {
                // If passenger is you, we need to update the map
                need_update = true;
                z_change = psgp.z() - part_pos.z();
            }

            psg->setpos( *this, psgp );
            r.moved = true;
        }
    }

    veh.shed_loose_parts( trinary::SOME, this, &dst );
    smzs = veh.advance_precalc_mounts( dst_offset, this, src, dp, ramp_offset,
                                       adjust_pos, parts_to_move );
    veh.update_active_fakes();

    if( src_submap != dst_submap ) {
        dst_submap->ensure_nonuniform();
        veh.set_submap_moved( this, { dst.x() / SEEX, dst.y() / SEEY, dst.z() } );
        auto src_submap_veh_it = src_submap->vehicles.begin() + our_i;
        dst_submap->vehicles.push_back( std::move( *src_submap_veh_it ) );
        src_submap->vehicles.erase( src_submap_veh_it );
        invalidate_max_populated_zlev( dst.z() );
    }
    if( need_update ) {
        g->update_map( player_character );
    }
    add_vehicle_to_cache( &veh );

    if( z_change || src.z() != dst.z() ) {
        if( z_change ) {
            g->vertical_move( z_change, true );
            // vertical moves can flush the caches, so make sure we're still in the cache
            add_vehicle_to_cache( &veh );
        }
        update_vehicle_list( dst_submap, dst.z() );
        // delete the vehicle from the source z-level vehicle cache set if it is no longer on
        // that z-level
        if( src.z() != dst.z() ) {
            level_cache &ch2 = get_cache( src.z() );
            for( const vehicle *elem : ch2.vehicle_list ) {
                if( elem == &veh ) {
                    ch2.vehicle_list.erase( &veh );
                    ch2.zone_vehicles.erase( &veh );
                    break;
                }
            }
        }
        veh.check_is_heli_landed( *this );
    }

    if( remote ) {
        // Has to be after update_map or coordinates won't be valid
        g->setremoteveh( &veh );
    }

    veh.zones_dirty = true; // invalidate zone positions

    for( int vsmz : smzs ) {
        on_vehicle_moved( dst.z() + vsmz );
    }
    return true;
}

void map::level_vehicle( vehicle &veh )
{
    int cnt = 0;
    while( !veh.level_vehicle( *this ) && cnt < ( 2 * OVERMAP_DEPTH ) ) {
        cnt++;
    }
}

std::string map::obstacle_name( const tripoint_bub_ms &p )
{
    if( const std::optional<vpart_reference> vp = veh_at( p ).obstacle_at_part() ) {
        return vp->info().name();
    }
    return name( p );
}

static void vehicle_caching_internal( map &here, level_cache &zch, const vpart_reference &vp,
                                      vehicle *v )
{
    auto &outside_cache = zch.outside_cache;
    auto &floor_cache = zch.floor_cache;

    const size_t part = vp.part_index();
    const tripoint_bub_ms part_pos =  v->bub_part_pos( here, vp.part() );

    bool vehicle_is_opaque = vp.has_feature( VPFLAG_OPAQUE ) && !vp.part().is_broken();

    if( vehicle_is_opaque ) {
        int dpart = v->part_with_feature( part, VPFLAG_OPENABLE, true );
        if( dpart < 0 || !v->part( dpart ).open ) {
            zch.vehicle_opaque_cache[part_pos.x()][part_pos.y()] = true;
            zch.vehicle_opaque_any = true;
        } else {
            vehicle_is_opaque = false;
        }
    }

    if( vehicle_is_opaque || vp.is_inside() ) {
        outside_cache[part_pos.x()][part_pos.y()] = false;
        zch.outside_rewritten = true;
    }

    if( vp.has_feature( VPFLAG_BOARDABLE ) && !vp.part().is_broken() ) {
        floor_cache[part_pos.x()][part_pos.y()] = true;
    }
}

static void vehicle_caching_internal_above( map &here, level_cache &zch_above,
        const vpart_reference &vp, vehicle *v )
{
    if( vp.has_feature( VPFLAG_ROOF ) || vp.has_feature( VPFLAG_OPAQUE ) ) {
        const tripoint_bub_ms part_pos = v->bub_part_pos( here, vp.part() );
        zch_above.floor_cache[part_pos.x()][part_pos.y()] = true;
    }
}

void map::do_vehicle_caching( int z )
{
    level_cache *ch = get_cache_lazy( z );
    if( !ch ) {
        return;
    }
    for( vehicle *v : ch->vehicle_list ) {
        for( const vpart_reference &vp : v->get_all_parts_with_fakes() ) {
            const tripoint_bub_ms part_pos = v->bub_part_pos( *this, vp.part() );
            if( !inbounds( part_pos.xy() ) ) {
                continue;
            }
            // positions on this map, which needn't be the reality bubble
            vehicle_caching_internal( *this, get_cache( part_pos.z() ), vp, v );
            if( part_pos.z() < OVERMAP_HEIGHT ) {
                vehicle_caching_internal_above( *this, get_cache( part_pos.z() + 1 ), vp, v );
            }
        }
    }
}
