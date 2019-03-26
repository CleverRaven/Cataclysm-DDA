#include "map.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstring>

#include "ammo.h"
#include "artifact.h"
#include "calendar.h"
#include "coordinate_conversions.h"
#include "clzones.h"
#include "debug.h"
#include "drawing_primitives.h"
#include "event.h"
#include "fragment_cloud.h"
#include "fungal_effects.h"
#include "game.h"
#include "harvest.h"
#include "iexamine.h"
#include "item.h"
#include "item_factory.h"
#include "item_group.h"
#include "iuse_actor.h"
#include "lightmap.h"
#include "line.h"
#include "map_iterator.h"
#include "map_selector.h"
#include "mapbuffer.h"
#include "mapdata.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "pathfinding.h"
#include "projectile.h"
#include "rng.h"
#include "scent_map.h"
#include "sounds.h"
#include "string_formatter.h"
#include "submap.h"
#include "translations.h"
#include "trap.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "vpart_reference.h"
#include "weather.h"

const mtype_id mon_zombie( "mon_zombie" );

const skill_id skill_traps( "traps" );

const species_id ZOMBIE( "ZOMBIE" );

const efftype_id effect_boomered( "boomered" );
const efftype_id effect_crushed( "crushed" );
const efftype_id effect_stunned( "stunned" );

extern bool is_valid_in_w_terrain( int, int );

#include "overmapbuffer.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

static std::list<item>  nulitems;          // Returned when &i_at() is asked for an OOB value
static field            nulfield;          // Returned when &field_at() is asked for an OOB value
static int              null_temperature;  // Because radiation does it too
static level_cache      nullcache;         // Dummy cache for z-levels outside bounds

// Map stack methods.
std::list<item>::iterator map_stack::erase( std::list<item>::iterator it )
{
    return myorigin->i_rem( location, it );
}

void map_stack::push_back( const item &newitem )
{
    myorigin->add_item_or_charges( location, newitem );
}

void map_stack::insert_at( std::list<item>::iterator index,
                           const item &newitem )
{
    myorigin->add_item_at( location, index, newitem );
}

units::volume map_stack::max_volume() const
{
    if( !myorigin->inbounds( location ) ) {
        return 0_ml;
    } else if( myorigin->has_furn( location ) ) {
        return myorigin->furn( location ).obj().max_volume;
    }
    return myorigin->ter( location ).obj().max_volume;
}

// Map class methods.

map::map( int mapsize, bool zlev )
{
    my_MAPSIZE = mapsize;
    zlevels = zlev;
    if( zlevels ) {
        grid.resize( my_MAPSIZE * my_MAPSIZE * OVERMAP_LAYERS, nullptr );
    } else {
        grid.resize( my_MAPSIZE * my_MAPSIZE, nullptr );
    }

    for( auto &ptr : caches ) {
        ptr = std::unique_ptr<level_cache>( new level_cache() );
    }

    for( auto &ptr : pathfinding_caches ) {
        ptr = std::unique_ptr<pathfinding_cache>( new pathfinding_cache() );
    }

    dbg( D_INFO ) << "map::map(): my_MAPSIZE: " << my_MAPSIZE << " z-levels enabled:" << zlevels;
    traplocs.resize( trap::count() );
}

map::~map() = default;

static submap null_submap;

const maptile map::maptile_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return maptile( &null_submap, 0, 0 );
    }

    return maptile_at_internal( p );
}

maptile map::maptile_at( const tripoint &p )
{
    if( !inbounds( p ) ) {
        return maptile( &null_submap, 0, 0 );
    }

    return maptile_at_internal( p );
}

const maptile map::maptile_at_internal( const tripoint &p ) const
{
    point l;
    submap *const sm = get_submap_at( p, l );

    return maptile( sm, l );
}

maptile map::maptile_at_internal( const tripoint &p )
{
    point l;
    submap *const sm = get_submap_at( p, l );

    return maptile( sm, l );
}

// Vehicle functions

VehicleList map::get_vehicles()
{
    if( !zlevels ) {
        return get_vehicles( tripoint( 0, 0, abs_sub.z ),
                             tripoint( SEEX * my_MAPSIZE, SEEY * my_MAPSIZE, abs_sub.z ) );
    }

    return get_vehicles( tripoint( 0, 0, -OVERMAP_DEPTH ),
                         tripoint( SEEX * my_MAPSIZE, SEEY * my_MAPSIZE, OVERMAP_HEIGHT ) );
}

void map::reset_vehicle_cache( const int zlev )
{
    clear_vehicle_cache( zlev );
    // Cache all vehicles
    auto &ch = get_cache( zlev );
    ch.veh_in_active_range = false;
    for( const auto &elem : ch.vehicle_list ) {
        add_vehicle_to_cache( elem );
    }
}

void map::add_vehicle_to_cache( vehicle *veh )
{
    if( veh == nullptr ) {
        debugmsg( "Tried to add null vehicle to cache" );
        return;
    }

    auto &ch = get_cache( veh->smz );
    ch.veh_in_active_range = true;
    // Get parts
    std::vector<vehicle_part> &parts = veh->parts;
    int partid = 0;
    for( std::vector<vehicle_part>::iterator it = parts.begin(),
         end = parts.end(); it != end; ++it, ++partid ) {
        if( it->removed ) {
            continue;
        }
        const tripoint p = veh->global_part_pos3( *it );
        ch.veh_cached_parts.insert( std::make_pair( p,
                                    std::make_pair( veh, partid ) ) );
        if( inbounds( p ) ) {
            ch.veh_exists_at[p.x][p.y] = true;
        }
    }
}

void map::update_vehicle_cache( vehicle *veh, const int old_zlevel )
{
    if( veh == nullptr ) {
        debugmsg( "Tried to add null vehicle to cache" );
        return;
    }

    // Existing must be cleared
    auto &ch = get_cache( old_zlevel );
    auto it = ch.veh_cached_parts.begin();
    const auto end = ch.veh_cached_parts.end();
    while( it != end ) {
        if( it->second.first == veh ) {
            const tripoint p = it->first;
            if( inbounds( p ) ) {
                ch.veh_exists_at[p.x][p.y] = false;
            }
            ch.veh_cached_parts.erase( it++ );
            // If something was resting on vehicle, drop it
            support_dirty( tripoint( p.x, p.y, old_zlevel + 1 ) );
        } else {
            ++it;
        }
    }

    add_vehicle_to_cache( veh );
}

void map::clear_vehicle_cache( const int zlev )
{
    auto &ch = get_cache( zlev );
    while( !ch.veh_cached_parts.empty() ) {
        const auto part = ch.veh_cached_parts.begin();
        const auto &p = part->first;
        if( inbounds( p ) ) {
            ch.veh_exists_at[p.x][p.y] = false;
        }
        ch.veh_cached_parts.erase( part );
    }
}

void map::clear_vehicle_list( const int zlev )
{
    auto &ch = get_cache( zlev );
    ch.vehicle_list.clear();
    ch.zone_vehicles.clear();
}

void map::update_vehicle_list( submap *const to, const int zlev )
{
    // Update vehicle data
    auto &ch = get_cache( zlev );
    for( const auto &elem : to->vehicles ) {
        ch.vehicle_list.insert( elem.get() );
        if( !elem->loot_zones.empty() ) {
            ch.zone_vehicles.insert( elem.get() );
        }
    }
}

std::unique_ptr<vehicle> map::detach_vehicle( vehicle *veh )
{
    if( veh == nullptr ) {
        debugmsg( "map::detach_vehicle was passed nullptr" );
        return std::unique_ptr<vehicle>();
    }

    if( veh->smz < -OVERMAP_DEPTH || veh->smz > OVERMAP_HEIGHT ) {
        debugmsg( "detach_vehicle got a vehicle outside allowed z-level range! name=%s, submap:%d,%d,%d",
                  veh->name.c_str(), veh->smx, veh->smy, veh->smz );
        // Try to fix by moving the vehicle here
        veh->smz = abs_sub.z;
    }

    submap *const current_submap = get_submap_at_grid( {veh->smx, veh->smy, veh->smz} );
    auto &ch = get_cache( veh->smz );
    for( size_t i = 0; i < current_submap->vehicles.size(); i++ ) {
        if( current_submap->vehicles[i].get() == veh ) {
            const int zlev = veh->smz;
            ch.vehicle_list.erase( veh );
            ch.zone_vehicles.erase( veh );
            reset_vehicle_cache( zlev );
            std::unique_ptr<vehicle> result = std::move( current_submap->vehicles[i] );
            current_submap->vehicles.erase( current_submap->vehicles.begin() + i );
            if( veh->tracking_on ) {
                overmap_buffer.remove_vehicle( veh );
            }
            dirty_vehicle_list.erase( veh );
            return result;
        }
    }
    debugmsg( "detach_vehicle can't find it! name=%s, submap:%d,%d,%d", veh->name.c_str(), veh->smx,
              veh->smy, veh->smz );
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
    set_pathfinding_cache_dirty( smz );
}

void map::vehmove()
{
    // give vehicles movement points
    {
        VehicleList vehs = get_vehicles();
        for( auto &vehs_v : vehs ) {
            vehicle *veh = vehs_v.v;
            veh->gain_moves();
            veh->slow_leak();
        }
    }

    // 15 equals 3 >50mph vehicles, or up to 15 slow (1 square move) ones
    // But 15 is too low for V12 death-bikes, let's put 100 here
    for( int count = 0; count < 100; count++ ) {
        if( !vehproceed() ) {
            break;
        }
    }
    // Process item removal on the vehicles that were modified this turn.
    // Use a copy because part_removal_cleanup can modify the container.
    auto temp = dirty_vehicle_list;
    VehicleList vehicle_list = get_vehicles();
    for( const auto &elem : temp ) {
        auto same_ptr = [ elem ]( const struct wrapped_vehicle & tgt ) {
            return elem == tgt.v;
        };
        if( std::find_if( vehicle_list.begin(), vehicle_list.end(), same_ptr ) !=
            vehicle_list.end() ) {
            ( elem )->part_removal_cleanup();
        }
    }
    dirty_vehicle_list.clear();
}

bool map::vehproceed()
{
    VehicleList vehs = get_vehicles();
    vehicle *cur_veh = nullptr;
    float max_of_turn = 0;
    // First horizontal movement
    for( auto &vehs_v : vehs ) {
        if( vehs_v.v->of_turn > max_of_turn ) {
            cur_veh = vehs_v.v;
            max_of_turn = cur_veh->of_turn;
        }
    }

    // Then vertical-only movement
    if( cur_veh == nullptr ) {
        for( auto &vehs_v : vehs ) {
            vehicle &cveh = *vehs_v.v;
            if( cveh.is_falling ) {
                cur_veh = vehs_v.v;
                break;
            }
        }
    }

    if( cur_veh == nullptr ) {
        return false;
    }

    return cur_veh->act_on_map();
}

static bool sees_veh( const Creature &c, vehicle &veh, bool force_recalc )
{
    const auto &veh_points = veh.get_points( force_recalc );
    return std::any_of( veh_points.begin(), veh_points.end(), [&c]( const tripoint & pt ) {
        return c.sees( pt );
    } );
}

void map::move_vehicle( vehicle &veh, const tripoint &dp, const tileray &facing )
{
    const bool vertical = dp.z != 0;
    if( ( dp.x == 0 && dp.y == 0 && dp.z == 0 ) ||
        ( abs( dp.x ) > 1 || abs( dp.y ) > 1 || abs( dp.z ) > 1 ) ||
        ( vertical && ( dp.x != 0 || dp.y != 0 ) ) ) {
        debugmsg( "move_vehicle called with %d,%d,%d displacement vector", dp.x, dp.y, dp.z );
        return;
    }

    if( dp.z + veh.smz < -OVERMAP_DEPTH || dp.z + veh.smz > OVERMAP_HEIGHT ) {
        return;
    }

    veh.precalc_mounts( 1, veh.skidding ? veh.turn_dir : facing.dir(), veh.pivot_point() );

    // cancel out any movement of the vehicle due only to a change in pivot
    tripoint dp1 = dp - veh.pivot_displacement();

    int impulse = 0;

    std::vector<veh_collision> collisions;

    // Find collisions
    // Velocity of car before collision
    // Split into vertical and horizontal movement
    const int &coll_velocity = vertical ? veh.vertical_velocity : veh.velocity;
    const int velocity_before = coll_velocity;
    if( velocity_before == 0 ) {
        debugmsg( "%s tried to move %s with no velocity",
                  veh.name.c_str(), vertical ? "vertically" : "horizontally" );
        return;
    }

    bool veh_veh_coll_flag = false;
    // Try to collide multiple times
    size_t collision_attempts = 10;
    do {
        collisions.clear();
        veh.collision( collisions, dp1, false );

        // Vehicle collisions
        std::map<vehicle *, std::vector<veh_collision> > veh_collisions;
        for( auto &coll : collisions ) {
            if( coll.type != veh_coll_veh ) {
                continue;
            }

            veh_veh_coll_flag = true;
            // Only collide with each vehicle once
            veh_collisions[ static_cast<vehicle *>( coll.target ) ].push_back( coll );
        }

        for( auto &pair : veh_collisions ) {
            impulse += vehicle_vehicle_collision( veh, *pair.first, pair.second );
        }

        // Non-vehicle collisions
        for( const auto &coll : collisions ) {
            if( coll.type == veh_coll_veh ) {
                continue;
            }
            if( static_cast<size_t>( coll.part ) > veh.parts.size() ) {
                continue;
            }

            const point &collision_point = veh.parts[coll.part].mount;
            const int coll_dmg = coll.imp;
            impulse += coll_dmg;
            // Shock damage
            veh.damage( coll.part, coll_dmg, DT_BASH );
            veh.damage_all( coll_dmg / 2, coll_dmg, DT_BASH, collision_point );
        }
    } while( collision_attempts-- > 0 &&
             sgn( coll_velocity ) == sgn( velocity_before ) &&
             !collisions.empty() && !veh_veh_coll_flag );

    if( vertical && !collisions.empty() ) {
        // A big hack, should be removed when possible
        veh.vertical_velocity = 0;
    }

    const int velocity_after = coll_velocity;
    const bool can_move = velocity_after != 0 && sgn( velocity_after ) == sgn( velocity_before );

    int coll_turn = 0;
    if( impulse > 0 ) {
        coll_turn = shake_vehicle( veh, velocity_before, facing.dir() );
        const int volume = std::min<int>( 100, sqrtf( impulse ) );
        // TODO: Center the sound at weighted (by impulse) average of collisions
        sounds::sound( veh.global_pos3(), volume, sounds::sound_t::combat, _( "crash!" ),
                       false, "smash_success", "hit_vehicle" );
    }

    if( veh_veh_coll_flag ) {
        // Break here to let the hit vehicle move away
        return;
    }

    // If not enough wheels, mess up the ground a bit.
    if( !vertical && !veh.valid_wheel_config() && !veh.is_in_water() ) {
        veh.velocity += veh.velocity < 0 ? 2000 : -2000;
        for( const auto &p : veh.get_points() ) {
            const ter_id &pter = ter( p );
            if( pter == t_dirt || pter == t_grass ) {
                ter_set( p, t_dirtmound );
            }
        }
    }

    // Now we're gonna handle traps we're standing on (if we're still moving).
    if( !vertical && can_move ) {
        const auto wheel_indices = veh.wheelcache; // Don't use a reference here, it causes a crash.
        for( auto &w : wheel_indices ) {
            const tripoint wheel_p = veh.global_part_pos3( w );
            if( one_in( 2 ) && displace_water( wheel_p ) ) {
                sounds::sound( wheel_p, 4,  sounds::sound_t::movement, _( "splash!" ), false,
                               "environment", "splash" );
            }

            veh.handle_trap( wheel_p, w );
            if( !has_flag( "SEALED", wheel_p ) ) {
                // TODO: Make this value depend on the wheel
                smash_items( wheel_p, 5 );
            }
        }
    }

    const int last_turn_dec = 1;
    if( veh.last_turn < 0 ) {
        veh.last_turn += last_turn_dec;
        if( veh.last_turn > -last_turn_dec ) {
            veh.last_turn = 0;
        }
    } else if( veh.last_turn > 0 ) {
        veh.last_turn -= last_turn_dec;
        if( veh.last_turn < last_turn_dec ) {
            veh.last_turn = 0;
        }
    }

    const bool seen = sees_veh( g->u, veh, false );

    if( can_move ) {
        // Accept new direction
        if( veh.skidding ) {
            veh.face.init( veh.turn_dir );
        } else {
            veh.face = facing;
        }

        veh.move = facing;
        if( coll_turn != 0 ) {
            veh.skidding = true;
            veh.turn( coll_turn );
        }
        veh.on_move();
        // Actually change position
        tripoint pt = veh.global_pos3(); // displace_vehicle needs a non-const reference
        displace_vehicle( pt, dp1 );
    } else if( !vertical ) {
        veh.stop();
    }
    // If the PC is in the currently moved vehicle, adjust the
    //  view offset.
    if( g->u.controlling_vehicle && veh_pointer_or_null( veh_at( g->u.pos() ) ) == &veh ) {
        g->calc_driving_offset( &veh );
        if( veh.skidding && can_move ) {
            // TODO: Make skid recovery in air hard
            veh.possibly_recover_from_skid();
        }
    }
    // Redraw scene
    // But only if the vehicle was seen before or after the move
    if( seen || sees_veh( g->u, veh, true ) ) {
        g->draw();
        refresh_display();
    }
}

float map::vehicle_vehicle_collision( vehicle &veh, vehicle &veh2,
                                      const std::vector<veh_collision> &collisions )
{
    if( &veh == &veh2 ) {
        debugmsg( "Vehicle %s collided with itself", veh.name.c_str() );
        return 0.0f;
    }

    // Effects of colliding with another vehicle:
    //  transfers of momentum, skidding,
    //  parts are damaged/broken on both sides,
    //  remaining times are normalized
    const veh_collision &c = collisions[0];
    add_msg( m_bad, _( "The %1$s's %2$s collides with %3$s's %4$s." ),
             veh.name.c_str(),  veh.part_info( c.part ).name().c_str(),
             veh2.name.c_str(), veh2.part_info( c.target_part ).name().c_str() );

    const bool vertical = veh.smz != veh2.smz;

    // Used to calculate the epicenter of the collision.
    point epicenter1( 0, 0 );
    point epicenter2( 0, 0 );

    float dmg;
    // Vertical collisions will be simpler for a while (1D)
    if( !vertical ) {
        // For reference, a cargo truck weighs ~25300, a bicycle 690,
        //  and 38mph is 3800 'velocity'
        rl_vec2d velo_veh1 = veh.velo_vec();
        rl_vec2d velo_veh2 = veh2.velo_vec();
        const float m1 = to_kilogram( veh.total_mass() );
        const float m2 = to_kilogram( veh2.total_mass() );
        //Energy of vehicle1 and vehicle2 before collision
        float E = 0.5 * m1 * velo_veh1.magnitude() * velo_veh1.magnitude() +
                  0.5 * m2 * velo_veh2.magnitude() * velo_veh2.magnitude();

        // Collision_axis
        point cof1 = veh .rotated_center_of_mass();
        point cof2 = veh2.rotated_center_of_mass();
        int &x_cof1 = cof1.x;
        int &y_cof1 = cof1.y;
        int &x_cof2 = cof2.x;
        int &y_cof2 = cof2.y;
        rl_vec2d collision_axis_y;

        collision_axis_y.x = ( veh.global_pos3().x + x_cof1 ) - ( veh2.global_pos3().x + x_cof2 );
        collision_axis_y.y = ( veh.global_pos3().y + y_cof1 ) - ( veh2.global_pos3().y + y_cof2 );
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

        veh.move.init( final1.x, final1.y );
        veh.velocity = final1.magnitude();

        veh2.move.init( final2.x, final2.y );
        veh2.velocity = final2.magnitude();
        //give veh2 the initiative to proceed next before veh1
        float avg_of_turn = ( veh2.of_turn + veh.of_turn ) / 2;
        if( avg_of_turn < .1f ) {
            avg_of_turn = .1f;
        }

        veh.of_turn = avg_of_turn * .9;
        veh2.of_turn = avg_of_turn * 1.1;

        //Energy after collision
        float E_a = 0.5 * m1 * final1.magnitude() * final1.magnitude() +
                    0.5 * m2 * final2.magnitude() * final2.magnitude();
        float d_E = E - E_a;  //Lost energy at collision -> deformation energy
        dmg = std::abs( d_E / 1000 / 2000 );  //adjust to balance damage
    } else {
        const float m1 = to_kilogram( veh.total_mass() );
        // Collision is perfectly inelastic for simplicity
        // Assume veh2 is standing still
        dmg = abs( veh.vertical_velocity / 100 ) * m1 / 10;
        veh.vertical_velocity = 0;
    }

    float dmg_veh1 = dmg * 0.5;
    float dmg_veh2 = dmg * 0.5;

    int coll_parts_cnt = 0; //quantity of colliding parts between veh1 and veh2
    for( const auto &veh_veh_coll : collisions ) {
        if( &veh2 == static_cast<vehicle *>( veh_veh_coll.target ) ) {
            coll_parts_cnt++;
        }
    }

    const float dmg1_part = dmg_veh1 / coll_parts_cnt;
    const float dmg2_part = dmg_veh2 / coll_parts_cnt;

    //damage colliding parts (only veh1 and veh2 parts)
    for( const auto &veh_veh_coll : collisions ) {
        if( &veh2 != static_cast<vehicle *>( veh_veh_coll.target ) ) {
            continue;
        }

        int parm1 = veh.part_with_feature( veh_veh_coll.part, VPFLAG_ARMOR, true );
        if( parm1 < 0 ) {
            parm1 = veh_veh_coll.part;
        }
        int parm2 = veh2.part_with_feature( veh_veh_coll.target_part, VPFLAG_ARMOR, true );
        if( parm2 < 0 ) {
            parm2 = veh_veh_coll.target_part;
        }

        epicenter1 += veh.parts[parm1].mount;
        veh.damage( parm1, dmg1_part, DT_BASH );

        epicenter2 += veh2.parts[parm2].mount;
        veh2.damage( parm2, dmg2_part, DT_BASH );
    }

    epicenter1.x /= coll_parts_cnt;
    epicenter1.y /= coll_parts_cnt;
    epicenter2.x /= coll_parts_cnt;
    epicenter2.y /= coll_parts_cnt;

    if( dmg2_part > 100 ) {
        // Shake vehicle because of collision
        veh2.damage_all( dmg2_part / 2, dmg2_part, DT_BASH, epicenter2 );
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
    for( auto veh : get_cache( zlev ).zone_vehicles ) {
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
    for( auto veh : get_cache( zlev ).zone_vehicles ) {
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
    auto &ch = get_cache( zlev );
    ch.zone_vehicles.insert( veh );
}

bool map::deregister_vehicle_zone( zone_data &zone )
{
    if( const cata::optional<vpart_reference> vp = g->m.veh_at( g->m.getlocal(
                zone.get_start_point() ) ).part_with_feature( "CARGO", false ) ) {
        auto bounds = vp->vehicle().loot_zones.equal_range( vp->mount() );
        for( auto it = bounds.first; it != bounds.second; it++ ) {
            if( &zone == &( it->second ) ) {
                vp->vehicle().loot_zones.erase( it );
                return true;
            }
        }
    }
    return false;
}

// 3D vehicle functions

VehicleList map::get_vehicles( const tripoint &start, const tripoint &end )
{
    const int chunk_sx = std::max( 0, ( start.x / SEEX ) - 1 );
    const int chunk_ex = std::min( my_MAPSIZE - 1, ( end.x / SEEX ) + 1 );
    const int chunk_sy = std::max( 0, ( start.y / SEEY ) - 1 );
    const int chunk_ey = std::min( my_MAPSIZE - 1, ( end.y / SEEY ) + 1 );
    const int chunk_sz = start.z;
    const int chunk_ez = end.z;
    VehicleList vehs;

    for( int cx = chunk_sx; cx <= chunk_ex; ++cx ) {
        for( int cy = chunk_sy; cy <= chunk_ey; ++cy ) {
            for( int cz = chunk_sz; cz <= chunk_ez; ++cz ) {
                submap *current_submap = get_submap_at_grid( { cx, cy, cz } );
                for( const auto &elem : current_submap->vehicles ) {
                    // Ensure the vehicle z-position is correct
                    elem->smz = cz;
                    wrapped_vehicle w;
                    w.v = elem.get();
                    w.x = w.v->posx + cx * SEEX;
                    w.y = w.v->posy + cy * SEEY;
                    w.z = cz;
                    w.i = cx;
                    w.j = cy;
                    vehs.push_back( w );
                }
            }
        }
    }

    return vehs;
}

optional_vpart_position map::veh_at( const tripoint &p ) const
{
    if( !const_cast<map *>( this )->get_cache( p.z ).veh_in_active_range || !inbounds( p ) ) {
        return optional_vpart_position( cata::nullopt );
    }

    int part_num = 1;
    vehicle *const veh = const_cast<map *>( this )->veh_at_internal( p, part_num );
    if( !veh ) {
        return optional_vpart_position( cata::nullopt );
    }
    return optional_vpart_position( vpart_position( *veh, part_num ) );

}

const vehicle *map::veh_at_internal( const tripoint &p, int &part_num ) const
{
    // This function is called A LOT. Move as much out of here as possible.
    const auto &ch = get_cache_ref( p.z );
    if( !ch.veh_in_active_range || !ch.veh_exists_at[p.x][p.y] ) {
        part_num = -1;
        return nullptr; // Clear cache indicates no vehicle. This should optimize a great deal.
    }

    const auto it = ch.veh_cached_parts.find( p );
    if( it != ch.veh_cached_parts.end() ) {
        part_num = it->second.second;
        return it->second.first;
    }

    debugmsg( "vehicle part cache indicated vehicle not found: %d %d %d", p.x, p.y, p.z );
    part_num = -1;
    return nullptr;
}

vehicle *map::veh_at_internal( const tripoint &p, int &part_num )
{
    return const_cast<vehicle *>( const_cast<const map *>( this )->veh_at_internal( p, part_num ) );
}

void map::board_vehicle( const tripoint &pos, player *p )
{
    if( p == nullptr ) {
        debugmsg( "map::board_vehicle: null player" );
        return;
    }

    const cata::optional<vpart_reference> vp = veh_at( pos ).part_with_feature( VPFLAG_BOARDABLE,
            true );
    if( !vp ) {
        if( p->grab_point.x == 0 && p->grab_point.y == 0 ) {
            debugmsg( "map::board_vehicle: vehicle not found" );
        }
        return;
    }
    if( vp->part().has_flag( vehicle_part::passenger_flag ) ) {
        player *psg = vp->vehicle().get_passenger( vp->part_index() );
        debugmsg( "map::board_vehicle: passenger (%s) is already there",
                  psg ? psg->name.c_str() : "<null>" );
        unboard_vehicle( pos );
    }
    vp->part().set_flag( vehicle_part::passenger_flag );
    vp->part().passenger_id = p->getID();
    vp->vehicle().invalidate_mass();

    p->setpos( pos );
    p->in_vehicle = true;
    if( p == &g->u ) {
        g->update_map( g->u );
    }
}

void map::unboard_vehicle( const tripoint &p, bool dead_passenger )
{
    const cata::optional<vpart_reference> vp = veh_at( p ).part_with_feature( VPFLAG_BOARDABLE, false );
    player *passenger = nullptr;
    if( !vp ) {
        debugmsg( "map::unboard_vehicle: vehicle not found" );
        // Try and force unboard the player anyway.
        passenger = g->critter_at<player>( p );
        if( passenger ) {
            passenger->in_vehicle = false;
            passenger->controlling_vehicle = false;
        }
        return;
    }
    passenger = vp->vehicle().get_passenger( vp->part_index() );
    // Mark the part as un-occupied regardless of whether there's a live passenger here.
    vp->part().remove_flag( vehicle_part::passenger_flag );
    if( !passenger ) {
        if( !dead_passenger ) {
            debugmsg( "map::unboard_vehicle: passenger not found" );
        }
        return;
    }
    passenger->in_vehicle = false;
    // Only make vehicle go out of control if the driver is the one unboarding.
    if( passenger->controlling_vehicle ) {
        vp->vehicle().skidding = true;
    }
    passenger->controlling_vehicle = false;
    vp->vehicle().invalidate_mass();
}

vehicle *map::displace_vehicle( tripoint &p, const tripoint &dp )
{
    const tripoint p2 = p + dp;
    const tripoint src = p;
    const tripoint dst = p2;

    if( !inbounds( src ) ) {
        add_msg( m_debug, "map::displace_vehicle: coordinates out of bounds %d,%d,%d->%d,%d,%d",
                 src.x, src.y, src.z, dst.x, dst.y, dst.z );
        return nullptr;
    }

    point src_offset;
    point dst_offset;
    submap *src_submap = get_submap_at( src, src_offset );
    submap *const dst_submap = get_submap_at( dst, dst_offset );

    // first, let's find our position in current vehicles vector
    int our_i = -1;
    for( size_t i = 0; i < src_submap->vehicles.size(); i++ ) {
        if( src_submap->vehicles[i]->posx == src_offset.x &&
            src_submap->vehicles[i]->posy == src_offset.y ) {
            our_i = i;
            break;
        }
    }
    if( our_i < 0 ) {
        vehicle *v = veh_pointer_or_null( veh_at( p ) );
        for( auto &smap : grid ) {
            for( size_t i = 0; i < smap->vehicles.size(); i++ ) {
                if( smap->vehicles[i].get() == v ) {
                    our_i = i;
                    src_submap = smap;
                    break;
                }
            }
        }
    }
    if( our_i < 0 ) {
        add_msg( m_debug, "displace_vehicle our_i=%d", our_i );
        return nullptr;
    }
    // move the vehicle
    vehicle *veh = src_submap->vehicles[our_i].get();
    // don't let it go off grid
    if( !inbounds( p2 ) ) {
        veh->stop();
        // Silent debug
        dbg( D_ERROR ) << "map:displace_vehicle: Stopping vehicle, displaced dp=("
                       << dp.x << ", " << dp.y << ", " << dp.z << ")";
        return veh;
    }

    // Need old coordinates to check for remote control
    const bool remote = veh->remote_controlled( g->u );

    // record every passenger inside
    std::vector<int> psg_parts = veh->boarded_parts();
    std::vector<player *> psgs;
    for( auto &prt : psg_parts ) {
        psgs.push_back( veh->get_passenger( prt ) );
    }

    bool need_update = false;
    int z_change = 0;
    // Move passengers
    for( size_t i = 0; i < psg_parts.size(); i++ ) {
        player *psg = psgs[i];
        const int prt = psg_parts[i];
        const tripoint part_pos = veh->global_part_pos3( prt );
        if( psg == nullptr ) {
            debugmsg( "Empty passenger part %d pcoord=%d,%d,%d u=%d,%d,%d?",
                      prt,
                      part_pos.x, part_pos.y, part_pos.z,
                      g->u.posx(), g->u.posy(), g->u.posz() );
            veh->parts[prt].remove_flag( vehicle_part::passenger_flag );
            continue;
        }

        if( psg->pos() != part_pos ) {
            add_msg( m_debug, "Passenger/part position mismatch: passenger %d,%d,%d, part %d %d,%d,%d",
                     g->u.posx(), g->u.posy(), g->u.posz(),
                     prt,
                     part_pos.x, part_pos.y, part_pos.z );
        }

        // Place passenger on the new part location
        const vehicle_part &veh_part = veh->parts[prt];
        tripoint psgp( part_pos.x + dp.x + veh_part.precalc[1].x - veh_part.precalc[0].x,
                       part_pos.y + dp.y + veh_part.precalc[1].y - veh_part.precalc[0].y,
                       psg->posz() );
        if( psg == &g->u ) {
            // If passenger is you, we need to update the map
            psg->setpos( psgp );
            need_update = true;
            z_change = dp.z;
        } else {
            // Player gets z position changed by g->vertical_move()
            psgp.z += dp.z;
            psg->setpos( psgp );
        }
    }

    veh->shed_loose_parts();
    for( auto &prt : veh->parts ) {
        prt.precalc[0] = prt.precalc[1];
    }
    veh->pivot_anchor[0] = veh->pivot_anchor[1];
    veh->pivot_rotation[0] = veh->pivot_rotation[1];

    veh->posx = dst_offset.x;
    veh->posy = dst_offset.y;
    veh->smz = p2.z;
    // Invalidate vehicle's point cache
    veh->occupied_cache_time = calendar::before_time_starts;
    if( src_submap != dst_submap ) {
        veh->set_submap_moved( int( p2.x / SEEX ), int( p2.y / SEEY ) );
        auto src_submap_veh_it = src_submap->vehicles.begin() + our_i;
        dst_submap->vehicles.push_back( std::move( *src_submap_veh_it ) );
        src_submap->vehicles.erase( src_submap_veh_it );
        dst_submap->is_uniform = false;
    }

    p = p2;

    update_vehicle_cache( veh, src.z );

    if( need_update ) {
        g->update_map( g->u );
    }

    if( z_change != 0 ) {
        g->vertical_move( z_change, true );
    }

    if( remote ) {
        // Has to be after update_map or coordinates won't be valid
        g->setremoteveh( veh );
    }

    veh->check_falling_or_floating();

    //global positions of vehicle loot zones have changed.
    veh->zones_dirty = true;

    on_vehicle_moved( veh->smz );
    return veh;
}

bool map::displace_water( const tripoint &p )
{
    // Check for shallow water
    if( has_flag( "SWIMMABLE", p ) && !has_flag( TFLAG_DEEP_WATER, p ) ) {
        int dis_places = 0;
        int sel_place = 0;
        for( int pass = 0; pass < 2; pass++ ) {
            // we do 2 passes.
            // first, count how many non-water places around
            // then choose one within count and fill it with water on second pass
            if( pass != 0 ) {
                sel_place = rng( 0, dis_places - 1 );
                dis_places = 0;
            }
            for( const tripoint &temp : points_in_radius( p, 1 ) ) {
                if( temp != p
                    || impassable_ter_furn( temp )
                    || has_flag( TFLAG_DEEP_WATER, temp ) ) {
                    continue;
                }
                ter_id ter0 = ter( temp );
                if( ter0 == t_water_sh ||
                    ter0 == t_water_dp || ter0 == t_water_moving_sh || ter0 == t_water_moving_dp ) {
                    continue;
                }
                if( pass != 0 && dis_places == sel_place ) {
                    ter_set( temp, t_water_sh );
                    ter_set( temp, t_dirt );
                    return true;
                }

                dis_places++;
            }
        }
    }
    return false;
}

// End of 3D vehicle

// 2D overloads for furniture
// To be removed once not needed
void map::set( const int x, const int y, const ter_id &new_terrain, const furn_id &new_furniture )
{
    furn_set( x, y, new_furniture );
    ter_set( x, y, new_terrain );
}

std::string map::name( const int x, const int y )
{
    return name( tripoint( x, y, abs_sub.z ) );
}

bool map::has_furn( const int x, const int y ) const
{
    return furn( x, y ) != f_null;
}

furn_id map::furn( const int x, const int y ) const
{
    const point p( x, y );
    if( !inbounds( p ) ) {
        return f_null;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->get_furn( l );
}

void map::furn_set( const int x, const int y, const furn_id &new_furniture )
{
    furn_set( tripoint( x, y, abs_sub.z ), new_furniture );
}

std::string map::furnname( const int x, const int y )
{
    return furnname( tripoint( x, y, abs_sub.z ) );
}
// End of 2D overloads for furniture

void map::set( const tripoint &p, const ter_id &new_terrain, const furn_id &new_furniture )
{
    furn_set( p, new_furniture );
    ter_set( p, new_terrain );
}

std::string map::name( const tripoint &p )
{
    return has_furn( p ) ? furnname( p ) : tername( p );
}

std::string map::disp_name( const tripoint &p )
{
    return string_format( _( "the %s" ), name( p ).c_str() );
}

std::string map::obstacle_name( const tripoint &p )
{
    if( const cata::optional<vpart_reference> vp = veh_at( p ).obstacle_at_part() ) {
        return vp->info().name();
    }
    return name( p );
}

bool map::has_furn( const tripoint &p ) const
{
    return furn( p ) != f_null;
}

furn_id map::furn( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return f_null;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->get_furn( l );
}

void map::furn_set( const tripoint &p, const furn_id &new_furniture )
{
    if( !inbounds( p ) ) {
        return;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );
    const furn_id old_id = current_submap->get_furn( l );
    if( old_id == new_furniture ) {
        // Nothing changed
        return;
    }

    current_submap->set_furn( l, new_furniture );

    // Set the dirty flags
    const furn_t &old_t = old_id.obj();
    const furn_t &new_t = new_furniture.obj();

    // If player has grabbed this furniture and it's no longer grabbable, release the grab.
    if( g->u.get_grab_type() == OBJECT_FURNITURE && g->u.grab_point == p && new_t.move_str_req < 0 ) {
        add_msg( _( "The %s you were grabbing is destroyed!" ), old_t.name().c_str() );
        g->u.grab( OBJECT_NONE );
    }

    if( old_t.transparent != new_t.transparent ) {
        set_transparency_cache_dirty( p.z );
    }

    if( old_t.has_flag( TFLAG_INDOORS ) != new_t.has_flag( TFLAG_INDOORS ) ) {
        set_outside_cache_dirty( p.z );
    }

    if( old_t.has_flag( TFLAG_NO_FLOOR ) != new_t.has_flag( TFLAG_NO_FLOOR ) ) {
        set_floor_cache_dirty( p.z );
    }
    set_memory_seen_cache_dirty( p );

    // TODO: Limit to changes that affect move cost, traps and stairs
    set_pathfinding_cache_dirty( p.z );

    // Make sure the furniture falls if it needs to
    support_dirty( p );
    tripoint above( p.x, p.y, p.z + 1 );
    // Make sure that if we supported something and no longer do so, it falls down
    support_dirty( above );
}

bool map::can_move_furniture( const tripoint &pos, player *p )
{
    const furn_t &furniture_type = furn( pos ).obj();
    int required_str = furniture_type.move_str_req;

    // Object can not be moved (or nothing there)
    if( required_str < 0 ) {
        return false;
    }

    ///\EFFECT_STR determines what furniture the player can move
    if( p != nullptr && p->str_cur < required_str ) {
        return false;
    }

    return true;
}

std::string map::furnname( const tripoint &p )
{
    const furn_t &f = furn( p ).obj();
    if( f.has_flag( "PLANT" ) && !i_at( p ).empty() ) {
        const item &seed = i_at( p ).front();
        const std::string &plant = seed.get_plant_name();
        return string_format( "%s (%s)", f.name().c_str(), plant.c_str() );
    } else {
        return f.name();
    }
}

// 2D overloads for terrain
// To be removed once not needed

ter_id map::ter( const int x, const int y ) const
{
    const point p( x, y );
    if( !inbounds( p ) ) {
        return t_null;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );
    return current_submap->get_ter( l );
}

bool map::ter_set( const int x, const int y, const ter_id &new_terrain )
{
    return ter_set( tripoint( x, y, abs_sub.z ), new_terrain );
}

std::string map::tername( const int x, const int y ) const
{
    return tername( tripoint( x, y, abs_sub.z ) );
}
// End of 2D overloads for terrain

/*
 * Get the terrain integer id. This is -not- a number guaranteed to remain
 * the same across revisions; it is a load order, and can change when mods
 * are loaded or removed. The old t_floor style constants will still work but
 * are -not- guaranteed; if a mod removes t_lava, t_lava will equal t_null;
 * New terrains added to the core game generally do not need this, it's
 * retained for high performance comparisons, save/load, and gradual transition
 * to string terrain.id
 */
ter_id map::ter( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return t_null;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->get_ter( l );
}

uint8_t map::get_known_connections( const tripoint &p, int connect_group ) const
{
    constexpr std::array<point, 4> offsets = {{
            { 0, 1 }, { 1, 0 }, { -1, 0 }, { 0, -1 }
        }
    };
    auto &ch = access_cache( p.z );
    bool is_transparent =
        ch.transparency_cache[p.x][p.y] > LIGHT_TRANSPARENCY_SOLID;
    uint8_t val = 0;
    std::function<bool( const tripoint & )> is_memorized;
    if( use_tiles ) {
        is_memorized =
        [&]( const tripoint & q ) {
            return !g->u.get_memorized_tile( getabs( q ) ).tile.empty();
        };
    } else {
        is_memorized =
        [&]( const tripoint & q ) {
            return g->u.get_memorized_symbol( getabs( q ) );
        };
    }

    // populate connection information
    for( int i = 0; i < 4; ++i ) {
        tripoint neighbour = p + offsets[i];
        if( !inbounds( neighbour ) ) {
            continue;
        }
        if( is_transparent ||
            ch.visibility_cache[neighbour.x][neighbour.y] <= LL_BRIGHT ||
            is_memorized( neighbour ) ) {
            const ter_t &neighbour_terrain = ter( neighbour ).obj();
            if( neighbour_terrain.connects_to( connect_group ) ) {
                val += 1 << i;
            }
        }
    }

    return val;
}

/*
 * Get the results of harvesting this tile's furniture or terrain
 */
const harvest_id &map::get_harvest( const tripoint &pos ) const
{
    const auto furn_here = furn( pos );
    if( furn_here->examine != iexamine::none ) {
        // Note: if furniture can be examined, the terrain can NOT (until furniture is removed)
        if( furn_here->has_flag( TFLAG_HARVESTED ) ) {
            return harvest_id::NULL_ID();
        }

        return furn_here->get_harvest();
    }

    const auto ter_here = ter( pos );
    if( ter_here->has_flag( TFLAG_HARVESTED ) ) {
        return harvest_id::NULL_ID();
    }

    return ter_here->get_harvest();
}

const std::set<std::string> &map::get_harvest_names( const tripoint &pos ) const
{
    static const std::set<std::string> null_harvest_names = {};
    const auto furn_here = furn( pos );
    if( furn_here->examine != iexamine::none ) {
        if( furn_here->has_flag( TFLAG_HARVESTED ) ) {
            return null_harvest_names;
        }

        return furn_here->get_harvest_names();
    }

    const auto ter_here = ter( pos );
    if( ter_here->has_flag( TFLAG_HARVESTED ) ) {
        return null_harvest_names;
    }

    return ter_here->get_harvest_names();
}

/*
 * Get the terrain transforms_into id (what will the terrain transforms into)
 */
ter_id map::get_ter_transforms_into( const tripoint &p ) const
{
    return ter( p ).obj().transforms_into.id();
}

/**
 * Examines the tile pos, with character as the "examinator"
 * Casts Character to player because player/NPC split isn't done yet
 */
void map::examine( Character &p, const tripoint &pos )
{
    const auto furn_here = furn( pos ).obj();
    if( furn_here.examine != iexamine::none ) {
        furn_here.examine( dynamic_cast<player &>( p ), pos );
    } else {
        ter( pos ).obj().examine( dynamic_cast<player &>( p ), pos );
    }
}

bool map::is_harvestable( const tripoint &pos ) const
{
    const auto &harvest_here = get_harvest( pos );
    return !harvest_here.is_null() && !harvest_here->empty();
}

/*
 * set terrain via string; this works for -any- terrain id
 */
bool map::ter_set( const tripoint &p, const ter_id &new_terrain )
{
    if( !inbounds( p ) ) {
        return false;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );
    const ter_id old_id = current_submap->get_ter( l );
    if( old_id == new_terrain ) {
        // Nothing changed
        return false;
    }

    current_submap->set_ter( l, new_terrain );

    // Set the dirty flags
    const ter_t &old_t = old_id.obj();
    const ter_t &new_t = new_terrain.obj();

    // Hack around ledges in traplocs or else it gets NASTY in z-level mode
    if( old_t.trap != tr_null && old_t.trap != tr_ledge ) {
        auto &traps = traplocs[old_t.trap];
        const auto iter = std::find( traps.begin(), traps.end(), p );
        if( iter != traps.end() ) {
            traps.erase( iter );
        }
    }
    if( new_t.trap != tr_null && new_t.trap != tr_ledge ) {
        traplocs[new_t.trap].push_back( p );
    }

    if( old_t.transparent != new_t.transparent ) {
        set_transparency_cache_dirty( p.z );
    }

    if( old_t.has_flag( TFLAG_INDOORS ) != new_t.has_flag( TFLAG_INDOORS ) ) {
        set_outside_cache_dirty( p.z );
    }

    if( new_t.has_flag( TFLAG_NO_FLOOR ) && !old_t.has_flag( TFLAG_NO_FLOOR ) ) {
        set_floor_cache_dirty( p.z );
        // It's a set, not a flag
        support_cache_dirty.insert( p );
    }
    set_memory_seen_cache_dirty( p );

    // TODO: Limit to changes that affect move cost, traps and stairs
    set_pathfinding_cache_dirty( p.z );

    tripoint above( p.x, p.y, p.z + 1 );
    // Make sure that if we supported something and no longer do so, it falls down
    support_dirty( above );

    return true;
}

std::string map::tername( const tripoint &p ) const
{
    return ter( p ).obj().name();
}

std::string map::features( const int x, const int y )
{
    return features( tripoint( x, y, abs_sub.z ) );
}

std::string map::features( const tripoint &p )
{
    // This is used in an info window that is 46 characters wide, and is expected
    // to take up one line.  So, make sure it does that.
    // FIXME: can't control length of localized text.
    std::stringstream ret;
    if( is_bashable( p ) ) {
        ret << _( "Smashable. " );
    }
    if( has_flag( "DIGGABLE", p ) ) {
        ret << _( "Diggable. " );
    }
    if( has_flag( "PLOWABLE", p ) ) {
        ret << _( "Plowable. " );
    }
    if( has_flag( "ROUGH", p ) ) {
        ret << _( "Rough. " );
    }
    if( has_flag( "UNSTABLE", p ) ) {
        ret << _( "Unstable. " );
    }
    if( has_flag( "SHARP", p ) ) {
        ret << _( "Sharp. " );
    }
    if( has_flag( "FLAT", p ) ) {
        ret << _( "Flat. " );
    }
    if( has_flag( "EASY_DECONSTRUCT", p ) ) {
        ret << _( "Simple. " );
    }
    if( has_flag( "MOUNTABLE", p ) ) {
        ret << _( "Mountable. " );
    }
    return ret.str();
}

int map::move_cost_internal( const furn_t &furniture, const ter_t &terrain, const vehicle *veh,
                             const int vpart ) const
{
    if( terrain.movecost == 0 || ( furniture.id && furniture.movecost < 0 ) ) {
        return 0;
    }

    if( veh != nullptr ) {
        const vpart_position vp( const_cast<vehicle &>( *veh ), vpart );
        if( vp.obstacle_at_part() ) {
            return 0;
        } else if( vp.part_with_feature( VPFLAG_AISLE, true ) ) {
            return 2;
        } else {
            return 8;
        }
    }

    if( furniture.id ) {
        return std::max( terrain.movecost + furniture.movecost, 0 );
    }

    return std::max( terrain.movecost, 0 );
}

// Move cost: 2D overloads

int map::move_cost( const int x, const int y, const vehicle *ignored_vehicle ) const
{
    return move_cost( tripoint( x, y, abs_sub.z ), ignored_vehicle );
}

bool map::impassable( const int x, const int y ) const
{
    return !passable( x, y );
}

bool map::passable( const int x, const int y ) const
{
    return passable( tripoint( x, y, abs_sub.z ) );
}

int map::move_cost_ter_furn( const int x, const int y ) const
{
    const point p( x, y );
    if( !inbounds( p ) ) {
        return 0;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    const int tercost = current_submap->get_ter( l ).obj().movecost;
    if( tercost == 0 ) {
        return 0;
    }

    const int furncost =  current_submap->get_furn( l ).obj().movecost;
    if( furncost < 0 ) {
        return 0;
    }

    const int cost = tercost + furncost;
    return cost > 0 ? cost : 0;
}

// Move cost: 3D

int map::move_cost( const tripoint &p, const vehicle *ignored_vehicle ) const
{
    if( !inbounds( p ) ) {
        return 0;
    }

    const furn_t &furniture = furn( p ).obj();
    const ter_t &terrain = ter( p ).obj();
    const optional_vpart_position vp = veh_at( p );
    vehicle *const veh = ( !vp || &vp->vehicle() == ignored_vehicle ) ? nullptr : &vp->vehicle();
    const int part = veh ? vp->part_index() : -1;

    return move_cost_internal( furniture, terrain, veh, part );
}

bool map::impassable( const tripoint &p ) const
{
    return !passable( p );
}

bool map::passable( const tripoint &p ) const
{
    return move_cost( p ) != 0;
}

int map::move_cost_ter_furn( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return 0;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    const int tercost = current_submap->get_ter( l ).obj().movecost;
    if( tercost == 0 ) {
        return 0;
    }

    const int furncost = current_submap->get_furn( l ).obj().movecost;
    if( furncost < 0 ) {
        return 0;
    }

    const int cost = tercost + furncost;
    return cost > 0 ? cost : 0;
}

bool map::impassable_ter_furn( const tripoint &p ) const
{
    return !passable_ter_furn( p );
}

bool map::passable_ter_furn( const tripoint &p ) const
{
    return move_cost_ter_furn( p ) != 0;
}

int map::combined_movecost( const tripoint &from, const tripoint &to,
                            const vehicle *ignored_vehicle,
                            const int modifier, const bool flying ) const
{
    const int mults[4] = { 0, 50, 71, 100 };
    const int cost1 = move_cost( from, ignored_vehicle );
    const int cost2 = move_cost( to, ignored_vehicle );
    // Multiply cost depending on the number of differing axes
    // 0 if all axes are equal, 100% if only 1 differs, 141% for 2, 200% for 3
    size_t match = trigdist ? ( from.x != to.x ) + ( from.y != to.y ) + ( from.z != to.z ) : 1;
    if( flying || from.z == to.z ) {
        return ( cost1 + cost2 + modifier ) * mults[match] / 2;
    }

    // Inter-z-level movement by foot (not flying)
    if( !valid_move( from, to, false ) ) {
        return 0;
    }

    // TODO: Penalize for using stairs
    return ( cost1 + cost2 + modifier ) * mults[match] / 2;
}

bool map::valid_move( const tripoint &from, const tripoint &to,
                      const bool bash, const bool flying ) const
{
    // Used to account for the fact that older versions of GCC can trip on the if statement here.
    assert( to.z > std::numeric_limits<int>::min() );
    // Note: no need to check inbounds here, because maptile_at will do that
    // If oob tile is supplied, the maptile_at will be an unpassable "null" tile
    if( abs( from.x - to.x ) > 1 || abs( from.y - to.y ) > 1 || abs( from.z - to.z ) > 1 ) {
        return false;
    }

    if( from.z == to.z ) {
        // But here we need to, to prevent bashing critters
        return passable( to ) || ( bash && inbounds( to ) );
    } else if( !zlevels ) {
        return false;
    }

    const bool going_up = from.z < to.z;

    const tripoint &up_p = going_up ? to : from;
    const tripoint &down_p = going_up ? from : to;

    const maptile up = maptile_at( up_p );
    const ter_t &up_ter = up.get_ter_t();
    if( up_ter.id.is_null() ) {
        return false;
    }
    // Checking for ledge is a workaround for the case when mapgen doesn't
    // actually make a valid ledge drop location with zlevels on, this forces
    // at least one zlevel drop and if down_ter is impassible it's probably
    // inside a wall, we could workaround that further but it's unnecessary.
    const bool up_is_ledge = tr_at( up_p ).loadid == tr_ledge;

    if( up_ter.movecost == 0 ) {
        // Unpassable tile
        return false;
    }

    const maptile down = maptile_at( down_p );
    const ter_t &down_ter = down.get_ter_t();
    if( down_ter.id.is_null() ) {
        return false;
    }

    if( !up_is_ledge && down_ter.movecost == 0 ) {
        // Unpassable tile
        return false;
    }

    if( !up_ter.has_flag( TFLAG_NO_FLOOR ) && !up_ter.has_flag( TFLAG_GOES_DOWN ) && !up_is_ledge ) {
        // Can't move from up to down
        if( abs( from.x - to.x ) == 1 || abs( from.y - to.y ) == 1 ) {
            // Break the move into two - vertical then horizontal
            tripoint midpoint( down_p.x, down_p.y, up_p.z );
            return valid_move( down_p, midpoint, bash, flying ) &&
                   valid_move( midpoint, up_p, bash, flying );
        }
        return false;
    }

    if( !flying && !down_ter.has_flag( TFLAG_GOES_UP ) && !down_ter.has_flag( TFLAG_RAMP ) &&
        !up_is_ledge ) {
        // Can't safely reach the lower tile
        return false;
    }

    if( bash ) {
        return true;
    }

    int part_up;
    const vehicle *veh_up = veh_at_internal( up_p, part_up );
    if( veh_up != nullptr ) {
        // TODO: Hatches below the vehicle, passable frames
        return false;
    }

    int part_down;
    const vehicle *veh_down = veh_at_internal( down_p, part_down );
    if( veh_down != nullptr && veh_down->roof_at_part( part_down ) >= 0 ) {
        // TODO: OPEN (and only open) hatches from above
        return false;
    }

    // Currently only furniture can block movement if everything else is OK
    // TODO: Vehicles with boards in the given spot
    return up.get_furn_t().movecost >= 0;
}

// End of move cost

double map::ranged_target_size( const tripoint &p ) const
{
    if( impassable( p ) ) {
        return 1.0;
    }

    if( !has_floor( p ) ) {
        return 0.0;
    }

    // TODO: Handle cases like shrubs, trees, furniture, sandbags...
    return 0.1;
}

int map::climb_difficulty( const tripoint &p ) const
{
    if( p.z > OVERMAP_HEIGHT || p.z < -OVERMAP_DEPTH ) {
        debugmsg( "climb_difficulty on out of bounds point: %d, %d, %d", p.x, p.y, p.z );
        return INT_MAX;
    }

    int best_difficulty = INT_MAX;
    int blocks_movement = 0;
    if( has_flag( "LADDER", p ) ) {
        // Really easy, but you have to stand on the tile
        return 1;
    } else if( has_flag( TFLAG_RAMP, p ) ) {
        // We're on something stair-like, so halfway there already
        best_difficulty = 7;
    }

    for( const auto &pt : points_in_radius( p, 1 ) ) {
        if( impassable_ter_furn( pt ) ) {
            // TODO: Non-hardcoded climbability
            best_difficulty = std::min( best_difficulty, 10 );
            blocks_movement++;
        } else if( veh_at( pt ) ) {
            // Vehicle tiles are quite good for climbing
            // TODO: Penalize spiked parts?
            best_difficulty = std::min( best_difficulty, 7 );
        }

        if( best_difficulty > 5 && has_flag( "CLIMBABLE", pt ) ) {
            best_difficulty = 5;
        }
    }

    // TODO: Make this more sensible - check opposite sides, not just movement blocker count
    return best_difficulty - blocks_movement;
}

bool map::has_floor( const tripoint &p ) const
{
    if( !zlevels || p.z < -OVERMAP_DEPTH + 1 || p.z > OVERMAP_HEIGHT ) {
        return true;
    }

    if( !inbounds( p ) ) {
        return true;
    }

    return get_cache_ref( p.z ).floor_cache[p.x][p.y];
}

bool map::supports_above( const tripoint &p ) const
{
    const maptile tile = maptile_at( p );
    const ter_t &ter = tile.get_ter_t();
    if( ter.movecost == 0 ) {
        return true;
    }

    const furn_id frn_id = tile.get_furn();
    if( frn_id != f_null ) {
        const furn_t &frn = frn_id.obj();
        if( frn.movecost < 0 ) {
            return true;
        }
    }

    if( veh_at( p ) ) {
        return true;
    }

    return false;
}

bool map::has_floor_or_support( const tripoint &p ) const
{
    const tripoint below( p.x, p.y, p.z - 1 );
    return !valid_move( p, below, false, true );
}

void map::drop_everything( const tripoint &p )
{
    if( has_floor( p ) ) {
        return;
    }

    drop_furniture( p );
    drop_items( p );
    drop_vehicle( p );
    drop_fields( p );
}

void map::drop_furniture( const tripoint &p )
{
    const furn_id frn = furn( p );
    if( frn == f_null ) {
        return;
    }

    enum support_state {
        SS_NO_SUPPORT = 0,
        SS_BAD_SUPPORT, // TODO: Implement bad, shaky support
        SS_GOOD_SUPPORT,
        SS_FLOOR, // Like good support, but bash floor instead of tile below
        SS_CREATURE
    };

    // Checks if the tile:
    // has floor (supports unconditionally)
    // has support below
    // has unsupporting furniture below (bad support, things should "slide" if possible)
    // has no support and thus allows things to fall through
    const auto check_tile = [this]( const tripoint & pt ) {
        if( has_floor( pt ) ) {
            return SS_FLOOR;
        }

        tripoint below_dest( pt.x, pt.y, pt.z - 1 );
        if( supports_above( below_dest ) ) {
            return SS_GOOD_SUPPORT;
        }

        const furn_id frn_id = furn( below_dest );
        if( frn_id != f_null ) {
            const furn_t &frn = frn_id.obj();
            // Allow crushing tiny/nocollide furniture
            if( !frn.has_flag( "TINY" ) && !frn.has_flag( "NOCOLLIDE" ) ) {
                return SS_BAD_SUPPORT;
            }
        }

        if( g->critter_at( below_dest ) != nullptr ) {
            // Smash a critter
            return SS_CREATURE;
        }

        return SS_NO_SUPPORT;
    };

    tripoint current( p.x, p.y, p.z + 1 );
    support_state last_state = SS_NO_SUPPORT;
    while( last_state == SS_NO_SUPPORT ) {
        current.z--;
        // Check current tile
        last_state = check_tile( current );
    }

    if( current == p ) {
        // Nothing happened
        if( last_state != SS_FLOOR ) {
            support_dirty( current );
        }

        return;
    }

    furn_set( p, f_null );
    furn_set( current, frn );

    // If it's sealed, we need to drop items with it
    const auto &frn_obj = frn.obj();
    if( frn_obj.has_flag( TFLAG_SEALED ) && has_items( p ) ) {
        auto old_items = i_at( p );
        auto new_items = i_at( current );
        for( const auto &it : old_items ) {
            new_items.push_back( it );
        }

        i_clear( p );
    }

    // Approximate weight/"bulkiness" based on strength to drag
    int weight;
    if( frn_obj.has_flag( "TINY" ) || frn_obj.has_flag( "NOCOLLIDE" ) ) {
        weight = 5;
    } else {
        weight = frn_obj.move_str_req >= 0 ? frn_obj.move_str_req : 20;
    }

    if( frn_obj.has_flag( "ROUGH" ) || frn_obj.has_flag( "SHARP" ) ) {
        weight += 5;
    }

    // TODO: Balance this.
    int dmg = weight * ( p.z - current.z );

    if( last_state == SS_FLOOR ) {
        // Bash the same tile twice - once for furniture, once for the floor
        bash( current, dmg, false, false, true );
        bash( current, dmg, false, false, true );
    } else if( last_state == SS_BAD_SUPPORT || last_state == SS_GOOD_SUPPORT ) {
        bash( current, dmg, false, false, false );
        tripoint below( current.x, current.y, current.z - 1 );
        bash( below, dmg, false, false, false );
    } else if( last_state == SS_CREATURE ) {
        const std::string &furn_name = frn_obj.name();
        bash( current, dmg, false, false, false );
        tripoint below( current.x, current.y, current.z - 1 );
        Creature *critter = g->critter_at( below );
        if( critter == nullptr ) {
            debugmsg( "drop_furniture couldn't find creature at %d,%d,%d",
                      below.x, below.y, below.z );
            return;
        }

        critter->add_msg_player_or_npc( m_bad, _( "Falling %s hits you!" ),
                                        _( "Falling %s hits <npcname>" ),
                                        furn_name.c_str() );
        // TODO: A chance to dodge/uncanny dodge
        player *pl = dynamic_cast<player *>( critter );
        monster *mon = dynamic_cast<monster *>( critter );
        if( pl != nullptr ) {
            pl->deal_damage( nullptr, bp_torso, damage_instance( DT_BASH, rng( dmg / 3, dmg ), 0, 0.5f ) );
            pl->deal_damage( nullptr, bp_head,  damage_instance( DT_BASH, rng( dmg / 3, dmg ), 0, 0.5f ) );
            pl->deal_damage( nullptr, bp_leg_l, damage_instance( DT_BASH, rng( dmg / 2, dmg ), 0, 0.4f ) );
            pl->deal_damage( nullptr, bp_leg_r, damage_instance( DT_BASH, rng( dmg / 2, dmg ), 0, 0.4f ) );
            pl->deal_damage( nullptr, bp_arm_l, damage_instance( DT_BASH, rng( dmg / 2, dmg ), 0, 0.4f ) );
            pl->deal_damage( nullptr, bp_arm_r, damage_instance( DT_BASH, rng( dmg / 2, dmg ), 0, 0.4f ) );
        } else if( mon != nullptr ) {
            // TODO: Monster's armor and size - don't crush hulks with chairs
            mon->apply_damage( nullptr, bp_torso, rng( dmg, dmg * 2 ) );
        }
    }

    // Re-queue for another check, in case bash destroyed something
    support_dirty( current );
}

void map::drop_items( const tripoint &p )
{
    if( !has_items( p ) ) {
        return;
    }

    auto items = i_at( p );
    // TODO: Make items check the volume tile below can accept
    // rather than disappearing if it would be overloaded

    tripoint below( p );
    while( !has_floor( below ) ) {
        below.z--;
    }

    if( below == p ) {
        return;
    }

    for( const auto &i : items ) {
        // TODO: Bash the item up before adding it
        // TODO: Bash the creature, terrain, furniture and vehicles on the tile
        add_item_or_charges( below, i );
    }

    // Just to make a sound for now
    bash( below, 1 );
    i_clear( p );
}

void map::drop_vehicle( const tripoint &p )
{
    const optional_vpart_position vp = veh_at( p );
    if( !vp ) {
        return;
    }

    vp->vehicle().is_falling = true;
}

void map::drop_fields( const tripoint &p )
{
    field &fld = field_at( p );
    if( fld.fieldCount() == 0 ) {
        return;
    }

    std::list<field_id> dropped;
    const tripoint below = p - tripoint( 0, 0, 1 );
    for( const auto &iter : fld ) {
        const field_entry &entry = iter.second;
        // For now only drop cosmetic fields, which don't warrant per-turn check
        // Active fields "drop themselves"
        if( entry.decays_on_actualize() ) {
            add_field( below, entry.getFieldType(), entry.getFieldDensity(), entry.getFieldAge() );
            dropped.push_back( entry.getFieldType() );
        }
    }

    for( const auto &entry : dropped ) {
        fld.removeField( entry );
    }
}

void map::support_dirty( const tripoint &p )
{
    if( zlevels ) {
        support_cache_dirty.insert( p );
    }
}

void map::process_falling()
{
    if( !zlevels ) {
        support_cache_dirty.clear();
        return;
    }

    if( !support_cache_dirty.empty() ) {
        add_msg( m_debug, "Checking %d tiles for falling objects",
                 support_cache_dirty.size() );
        // We want the cache to stay constant, but falling can change it
        std::set<tripoint> last_cache = std::move( support_cache_dirty );
        support_cache_dirty.clear();
        for( const tripoint &p : last_cache ) {
            drop_everything( p );
        }
    }
}

// 2D flags

bool map::has_flag( const std::string &flag, const int x, const int y ) const
{
    return has_flag( flag, tripoint( x, y, abs_sub.z ) );
}

bool map::can_put_items_ter_furn( const int x, const int y ) const
{
    return !has_flag( "NOITEM", x, y ) && !has_flag( "SEALED", x, y );
}

bool map::has_flag_ter( const std::string &flag, const int x, const int y ) const
{
    return has_flag_ter( flag, tripoint( x, y, abs_sub.z ) );
}

bool map::has_flag_furn( const std::string &flag, const int x, const int y ) const
{
    return has_flag_furn( flag, tripoint( x, y, abs_sub.z ) );
}

bool map::has_flag_ter_or_furn( const std::string &flag, const int x, const int y ) const
{
    const point p( x, y );
    if( !inbounds( p ) ) {
        return false;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->get_ter( l ).obj().has_flag( flag ) ||
           current_submap->get_furn( l ).obj().has_flag( flag );
}

/////
bool map::has_flag( const ter_bitflags flag, const int x, const int y ) const
{
    return has_flag( flag, tripoint( x, y, abs_sub.z ) );
}

bool map::has_flag_ter( const ter_bitflags flag, const int x, const int y ) const
{
    return has_flag_ter( flag, tripoint( x, y, abs_sub.z ) );
}

bool map::has_flag_furn( const ter_bitflags flag, const int x, const int y ) const
{
    return has_flag_furn( flag, tripoint( x, y, abs_sub.z ) );
}

bool map::has_flag_ter_or_furn( const ter_bitflags flag, const int x, const int y ) const
{
    const point p( x, y );
    if( !inbounds( p ) ) {
        return false;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->get_ter( l ).obj().has_flag( flag ) ||
           current_submap->get_furn( l ).obj().has_flag( flag );
}

// End of 2D flags

bool map::has_flag( const std::string &flag, const tripoint &p ) const
{
    return has_flag_ter_or_furn( flag, p ); // Does bound checking
}

bool map::can_put_items( const tripoint &p ) const
{
    if( can_put_items_ter_furn( p ) ) {
        return true;
    }
    const optional_vpart_position vp = veh_at( p );
    return static_cast<bool>( vp.part_with_feature( "CARGO", true ) );
}

bool map::can_put_items_ter_furn( const tripoint &p ) const
{
    return !has_flag( "NOITEM", p ) && !has_flag( "SEALED", p );
}

bool map::has_flag_ter( const std::string &flag, const tripoint &p ) const
{
    return ter( p ).obj().has_flag( flag );
}

bool map::has_flag_furn( const std::string &flag, const tripoint &p ) const
{
    return furn( p ).obj().has_flag( flag );
}

bool map::has_flag_ter_or_furn( const std::string &flag, const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->get_ter( l ).obj().has_flag( flag ) ||
           current_submap->get_furn( l ).obj().has_flag( flag );
}

bool map::has_flag( const ter_bitflags flag, const tripoint &p ) const
{
    return has_flag_ter_or_furn( flag, p ); // Does bound checking
}

bool map::has_flag_ter( const ter_bitflags flag, const tripoint &p ) const
{
    return ter( p ).obj().has_flag( flag );
}

bool map::has_flag_furn( const ter_bitflags flag, const tripoint &p ) const
{
    return furn( p ).obj().has_flag( flag );
}

bool map::has_flag_ter_or_furn( const ter_bitflags flag, const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->get_ter( l ).obj().has_flag( flag ) ||
           current_submap->get_furn( l ).obj().has_flag( flag );
}

// End of 3D flags

// Bashable - common function

int map::bash_rating_internal( const int str, const furn_t &furniture,
                               const ter_t &terrain, const bool allow_floor,
                               const vehicle *veh, const int part ) const
{
    bool furn_smash = false;
    bool ter_smash = false;
    ///\EFFECT_STR determines what furniture can be smashed
    if( furniture.id && furniture.bash.str_max != -1 ) {
        furn_smash = true;
        ///\EFFECT_STR determines what terrain can be smashed
    } else if( terrain.bash.str_max != -1 && ( !terrain.bash.bash_below || allow_floor ) ) {
        ter_smash = true;
    }

    if( veh != nullptr && vpart_position( const_cast<vehicle &>( *veh ), part ).obstacle_at_part() ) {
        // Monsters only care about rating > 0, NPCs should want to path around cars instead
        return 2; // Should probably be a function of part hp (+armor on tile)
    }

    int bash_min = 0;
    int bash_max = 0;
    if( furn_smash ) {
        bash_min = furniture.bash.str_min;
        bash_max = furniture.bash.str_max;
    } else if( ter_smash ) {
        bash_min = terrain.bash.str_min;
        bash_max = terrain.bash.str_max;
    } else {
        return -1;
    }

    ///\EFFECT_STR increases smashing damage
    if( str < bash_min ) {
        return 0;
    } else if( str >= bash_max ) {
        return 10;
    }

    int ret = ( 10 * ( str - bash_min ) ) / ( bash_max - bash_min );
    // Round up to 1, so that desperate NPCs can try to bash down walls
    return std::max( ret, 1 );
}

// 2D bashable

bool map::is_bashable( const int x, const int y ) const
{
    return is_bashable( tripoint( x, y, abs_sub.z ) );
}

bool map::is_bashable_ter( const int x, const int y ) const
{
    return is_bashable_ter( tripoint( x, y, abs_sub.z ) );
}

bool map::is_bashable_furn( const int x, const int y ) const
{
    return is_bashable_furn( tripoint( x, y, abs_sub.z ) );
}

bool map::is_bashable_ter_furn( const int x, const int y ) const
{
    return is_bashable_ter_furn( tripoint( x, y, abs_sub.z ) );
}

int map::bash_strength( const int x, const int y ) const
{
    return bash_strength( tripoint( x, y, abs_sub.z ) );
}

int map::bash_resistance( const int x, const int y ) const
{
    return bash_resistance( tripoint( x, y, abs_sub.z ) );
}

int map::bash_rating( const int str, const int x, const int y ) const
{
    return bash_rating( str, tripoint( x, y, abs_sub.z ) );
}

// 3D bashable

bool map::is_bashable( const tripoint &p, const bool allow_floor ) const
{
    if( !inbounds( p ) ) {
        DebugLog( D_WARNING, D_MAP ) << "Looking for out-of-bounds is_bashable at "
                                     << p.x << ", " << p.y << ", " << p.z;
        return false;
    }

    if( veh_at( p ).obstacle_at_part() ) {
        return true;
    }

    if( has_furn( p ) && furn( p ).obj().bash.str_max != -1 ) {
        return true;
    }

    const auto &ter_bash = ter( p ).obj().bash;
    return ter_bash.str_max != -1 && ( !ter_bash.bash_below || allow_floor );
}

bool map::is_bashable_ter( const tripoint &p, const bool allow_floor ) const
{
    const auto &ter_bash = ter( p ).obj().bash;
    return ter_bash.str_max != -1 && ( !ter_bash.bash_below || allow_floor );
}

bool map::is_bashable_furn( const tripoint &p ) const
{
    return has_furn( p ) && furn( p ).obj().bash.str_max != -1;
}

bool map::is_bashable_ter_furn( const tripoint &p, const bool allow_floor ) const
{
    return is_bashable_furn( p ) || is_bashable_ter( p, allow_floor );
}

int map::bash_strength( const tripoint &p, const bool allow_floor ) const
{
    if( has_furn( p ) && furn( p ).obj().bash.str_max != -1 ) {
        return furn( p ).obj().bash.str_max;
    }

    const auto &ter_bash = ter( p ).obj().bash;
    if( ter_bash.str_max != -1 && ( !ter_bash.bash_below || allow_floor ) ) {
        return ter_bash.str_max;
    }

    return -1;
}

int map::bash_resistance( const tripoint &p, const bool allow_floor ) const
{
    if( has_furn( p ) && furn( p ).obj().bash.str_min != -1 ) {
        return furn( p ).obj().bash.str_min;
    }

    const auto &ter_bash = ter( p ).obj().bash;
    if( ter_bash.str_min != -1 && ( !ter_bash.bash_below || allow_floor ) ) {
        return ter_bash.str_min;
    }

    return -1;
}

int map::bash_rating( const int str, const tripoint &p, const bool allow_floor ) const
{
    if( !inbounds( p ) ) {
        DebugLog( D_WARNING, D_MAP ) << "Looking for out-of-bounds is_bashable at "
                                     << p.x << ", " << p.y << ", " << p.z;
        return -1;
    }

    if( str <= 0 ) {
        return -1;
    }

    const furn_t &furniture = furn( p ).obj();
    const ter_t &terrain = ter( p ).obj();
    const optional_vpart_position vp = veh_at( p );
    vehicle *const veh = vp ? &vp->vehicle() : nullptr;
    const int part = vp ? vp->part_index() : -1;
    return bash_rating_internal( str, furniture, terrain, allow_floor, veh, part );
}

// End of 3D bashable

void map::make_rubble( const tripoint &p )
{
    make_rubble( p, f_rubble, false, t_dirt, false );
}

void map::make_rubble( const tripoint &p, const furn_id &rubble_type, const bool items )
{
    make_rubble( p, rubble_type, items, t_dirt, false );
}

void map::make_rubble( const tripoint &p, const furn_id &rubble_type, const bool items,
                       const ter_id &floor_type, bool overwrite )
{
    if( overwrite ) {
        ter_set( p, floor_type );
        furn_set( p, rubble_type );
    } else {
        // First see if there is existing furniture to destroy
        if( is_bashable_furn( p ) ) {
            destroy_furn( p, true );
        }
        // Leave the terrain alone unless it interferes with furniture placement
        if( impassable( p ) && is_bashable_ter( p ) ) {
            destroy( p, true );
        }
        // Check again for new terrain after potential destruction
        if( impassable( p ) ) {
            ter_set( p, floor_type );
        }

        furn_set( p, rubble_type );
    }

    if( !items ) {
        return;
    }

    //Still hardcoded, but a step up from the old stuff due to being in only one place
    if( rubble_type == f_wreckage ) {
        item chunk( "steel_chunk", calendar::turn );
        item scrap( "scrap", calendar::turn );
        add_item_or_charges( p, chunk );
        add_item_or_charges( p, scrap );
        if( one_in( 5 ) ) {
            item pipe( "pipe", calendar::turn );
            item wire( "wire", calendar::turn );
            add_item_or_charges( p, pipe );
            add_item_or_charges( p, wire );
        }
    } else if( rubble_type == f_rubble_rock ) {
        item rock( "rock", calendar::turn );
        int rock_count = rng( 1, 3 );
        for( int i = 0; i < rock_count; i++ ) {
            add_item_or_charges( p, rock );
        }
    } else if( rubble_type == f_rubble ) {
        item splinter( "splinter", calendar::turn );
        item nail( "nail", calendar::turn );
        int splinter_count = rng( 2, 8 );
        int nail_count = rng( 5, 10 );
        for( int i = 0; i < splinter_count; i++ ) {
            add_item_or_charges( p, splinter );
        }
        for( int i = 0; i < nail_count; i++ ) {
            add_item_or_charges( p, nail );
        }
    }
}

/**
 * Returns whether or not the terrain at the given location can be dived into
 * (by monsters that can swim or are aquatic or non-breathing).
 * @param x The x coordinate to look at.
 * @param y The y coordinate to look at.
 * @return true if the terrain can be dived into; false if not.
 */
bool map::is_divable( const int x, const int y ) const
{
    return has_flag( "SWIMMABLE", x, y ) && has_flag( TFLAG_DEEP_WATER, x, y );
}

bool map::is_divable( const tripoint &p ) const
{
    return has_flag( "SWIMMABLE", p ) && has_flag( TFLAG_DEEP_WATER, p );
}

bool map::is_outside( const int x, const int y ) const
{
    const point p( x, y );
    if( !inbounds( p ) ) {
        return true;
    }

    const auto &outside_cache = get_cache_ref( abs_sub.z ).outside_cache;
    return outside_cache[x][y];
}

bool map::is_outside( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return true;
    }

    const auto &outside_cache = get_cache_ref( p.z ).outside_cache;
    return outside_cache[p.x][p.y];
}

bool map::is_last_ter_wall( const bool no_furn, const int x, const int y,
                            const int xmax, const int ymax, const direction dir ) const
{
    int xmov = 0;
    int ymov = 0;
    switch( dir ) {
        case NORTH:
            ymov = -1;
            break;
        case SOUTH:
            ymov = 1;
            break;
        case WEST:
            xmov = -1;
            break;
        case EAST:
            xmov = 1;
            break;
        default:
            break;
    }
    int x2 = x;
    int y2 = y;
    bool result = true;
    bool loop = true;
    while( ( loop ) && ( ( dir == NORTH && y2 >= 0 ) ||
                         ( dir == SOUTH && y2 < ymax ) ||
                         ( dir == WEST  && x2 >= 0 ) ||
                         ( dir == EAST  && x2 < xmax ) ) ) {
        if( no_furn && has_furn( x2, y2 ) ) {
            loop = false;
            result = false;
        } else if( !has_flag_ter( "FLAT", x2, y2 ) ) {
            loop = false;
            if( !has_flag_ter( "WALL", x2, y2 ) ) {
                result = false;
            }
        }
        x2 += xmov;
        y2 += ymov;
    }
    return result;
}

bool map::flammable_items_at( const tripoint &p, int threshold )
{
    if( !has_items( p ) ||
        ( has_flag( TFLAG_SEALED, p ) && !has_flag( TFLAG_ALLOW_FIELD_EFFECT, p ) ) ) {
        // Sealed containers don't allow fire, so shouldn't allow setting the fire either
        return false;
    }

    for( const auto &i : i_at( p ) ) {
        if( i.flammable( threshold ) ) {
            return true;
        }
    }

    return false;
}

void map::decay_fields_and_scent( const time_duration &amount )
{
    // Decay scent separately, so that later we can use field count to skip empty submaps
    tripoint tmp;
    tmp.z = abs_sub.z; // TODO: Make this happen on all z-levels
    g->scent.decay();

    const time_duration amount_fire = amount / 3; // Decay fire by this much
    const time_duration amount_liquid = amount / 2; // Decay washable fields (blood, guts etc.) by this
    const time_duration amount_gas = amount / 5; // Decay gas type fields by this
    // Coordinate code copied from lightmap calculations
    // TODO: Z
    const int smz = abs_sub.z;
    const auto &outside_cache = get_cache_ref( smz ).outside_cache;
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const auto cur_submap = get_submap_at_grid( { smx, smy, smz } );
            int to_proc = cur_submap->field_count;
            if( to_proc < 1 ) {
                if( to_proc < 0 ) {
                    cur_submap->field_count = 0;
                    dbg( D_ERROR ) << "map::decay_fields_and_scent: submap at "
                                   << abs_sub.x + smx << "," << abs_sub.y + smy << "," << abs_sub.z
                                   << "has " << to_proc << " field_count";
                }
                // This submap has no fields
                continue;
            }

            for( int sx = 0; sx < SEEX; ++sx ) {
                if( to_proc < 1 ) {
                    // This submap had some fields, but all got proc'd already
                    break;
                }

                for( int sy = 0; sy < SEEY; ++sy ) {
                    const int x = sx + smx * SEEX;
                    const int y = sy + smy * SEEY;

                    field &fields = cur_submap->fld[sx][sy];
                    if( !outside_cache[x][y] ) {
                        to_proc -= fields.fieldCount();
                        continue;
                    }

                    for( auto &fp : fields ) {
                        to_proc--;
                        field_entry &cur = fp.second;
                        const field_id type = cur.getFieldType();
                        switch( type ) {
                            case fd_fire:
                                cur.setFieldAge( cur.getFieldAge() + amount_fire );
                                break;
                            case fd_blood:
                            case fd_bile:
                            case fd_gibs_flesh:
                            case fd_gibs_veggy:
                            case fd_slime:
                            case fd_blood_veggy:
                            case fd_blood_insect:
                            case fd_blood_invertebrate:
                            case fd_gibs_insect:
                            case fd_gibs_invertebrate:
                                cur.setFieldAge( cur.getFieldAge() + amount_liquid );
                                break;
                            case fd_smoke:
                            case fd_toxic_gas:
                            case fd_fungicidal_gas:
                            case fd_tear_gas:
                            case fd_nuke_gas:
                            case fd_cigsmoke:
                            case fd_weedsmoke:
                            case fd_cracksmoke:
                            case fd_methsmoke:
                            case fd_relax_gas:
                            case fd_fungal_haze:
                            case fd_hot_air1:
                            case fd_hot_air2:
                            case fd_hot_air3:
                            case fd_hot_air4:
                                cur.setFieldAge( cur.getFieldAge() + amount_gas );
                                break;
                            default:
                                break;
                        }
                    }
                }
            }

            if( to_proc > 0 ) {
                cur_submap->field_count = cur_submap->field_count - to_proc;
                dbg( D_ERROR ) << "map::decay_fields_and_scent: submap at "
                               << abs_sub.x + smx << "," << abs_sub.y + smy << "," << abs_sub.z
                               << "has " << cur_submap->field_count - to_proc << "fields, but "
                               << cur_submap->field_count << " field_count";
            }
        }
    }
}

point map::random_outdoor_tile()
{
    std::vector<point> options;
    for( int x = 0; x < SEEX * my_MAPSIZE; x++ ) {
        for( int y = 0; y < SEEY * my_MAPSIZE; y++ ) {
            if( is_outside( x, y ) ) {
                options.push_back( point( x, y ) );
            }
        }
    }
    return random_entry( options, point( -1, -1 ) );
}

bool map::has_adjacent_furniture( const tripoint &p )
{
    const signed char cx[4] = { 0, -1, 0, 1};
    const signed char cy[4] = { -1,  0, 1, 0};

    for( int i = 0; i < 4; i++ ) {
        const int adj_x = p.x + cx[i];
        const int adj_y = p.y + cy[i];
        if( has_furn( tripoint( adj_x, adj_y, p.z ) ) &&
            furn( tripoint( adj_x, adj_y, p.z ) ).obj().has_flag( "BLOCKSDOOR" ) ) {
            return true;
        }
    }

    return false;
}

bool map::has_nearby_fire( const tripoint &p, int radius )
{
    for( const tripoint &pt : points_in_radius( p, radius ) ) {
        if( get_field( pt, fd_fire ) != nullptr ) {
            return true;
        }
        if( has_flag_ter_or_furn( "USABLE_FIRE", p ) ) {
            return true;
        }
    }
    return false;
}

bool map::mop_spills( const tripoint &p )
{
    bool retval = false;

    if( !has_flag( "LIQUIDCONT", p ) ) {
        auto items = i_at( p );
        auto new_end = std::remove_if( items.begin(), items.end(), []( const item & it ) {
            return it.made_of( LIQUID );
        } );
        retval = new_end != items.end();
        while( new_end != items.end() ) {
            new_end = items.erase( new_end );
        }
    }

    field &fld = field_at( p );
    static const std::vector<field_id> to_check = {
        fd_blood,
        fd_blood_veggy,
        fd_blood_insect,
        fd_blood_invertebrate,
        fd_gibs_flesh,
        fd_gibs_veggy,
        fd_gibs_insect,
        fd_gibs_invertebrate,
        fd_bile,
        fd_slime,
        fd_sludge
    };
    for( field_id fid : to_check ) {
        retval |= fld.removeField( fid );
    }

    if( const optional_vpart_position vp = veh_at( p ) ) {
        vehicle *const veh = &vp->vehicle();
        std::vector<int> parts_here = veh->parts_at_relative( vp->mount(), true );
        for( auto &elem : parts_here ) {
            if( veh->parts[elem].blood > 0 ) {
                veh->parts[elem].blood = 0;
                retval = true;
            }
            //remove any liquids that somehow didn't fall through to the ground
            vehicle_stack here = veh->get_items( elem );
            auto new_end = std::remove_if( here.begin(), here.end(), []( const item & it ) {
                return it.made_of( LIQUID );
            } );
            retval |= ( new_end != here.end() );
            while( new_end != here.end() ) {
                new_end = here.erase( new_end );
            }
        }
    } // if veh != 0
    return retval;
}

int map::collapse_check( const tripoint &p )
{
    const bool collapses = has_flag( "COLLAPSES", p );
    const bool supports_roof = has_flag( "SUPPORTS_ROOF", p );

    int num_supports = 0;

    for( const tripoint &t : points_in_radius( p, 1 ) ) {
        if( p == t ) {
            continue;
        }

        if( collapses ) {
            if( has_flag( "COLLAPSES", t ) ) {
                num_supports++;
            } else if( has_flag( "SUPPORTS_ROOF", t ) ) {
                num_supports += 2;
            }
        } else if( supports_roof ) {
            if( has_flag( "SUPPORTS_ROOF", t ) && !has_flag( "COLLAPSES", t ) ) {
                num_supports += 3;
            }
        }
    }

    return 1.7 * num_supports;
}

void map::collapse_at( const tripoint &p, const bool silent )
{
    destroy( p, silent );
    crush( p );
    make_rubble( p );
    for( const tripoint &t : points_in_radius( p, 1 ) ) {
        if( p == t ) {
            continue;
        }
        if( has_flag( "COLLAPSES", t ) && one_in( collapse_check( t ) ) ) {
            destroy( t, silent );
            // We only check for rubble spread if it doesn't already collapse to prevent double crushing
        } else if( has_flag( "FLAT", t ) && one_in( 8 ) ) {
            crush( t );
            make_rubble( t );
        }
    }
}

void map::smash_items( const tripoint &p, const int power )
{
    if( !has_items( p ) ) {
        return;
    }

    std::vector<item> contents;
    auto items = g->m.i_at( p );
    for( auto i = items.begin(); i != items.end(); ) {
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

        // The volume check here pretty much only influences corpses and very large items
        const float volume_factor = std::max<float>( 40, i->volume() / units::legacy_volume_factor );
        float damage_chance = 10.0f * power / volume_factor;
        // Example:
        // Power 40 (just below C4 epicenter) vs two-by-four
        // damage_chance = 10 * 40 / 40 = 10, material_factor = 8
        // Will deal 1 damage, then 20% chance for another point
        // Power 20 (grenade minus shrapnel) vs glass bottle
        // 10 * 20 / 40 = 5 vs 1
        // 5 damage (destruction)

        const bool by_charges = i->count_by_charges();
        // See if they were damaged
        if( by_charges ) {
            damage_chance *= i->charges_per_volume( units::from_milliliter( 250 ) );
            while( ( damage_chance > material_factor ||
                     x_in_y( damage_chance, material_factor ) ) &&
                   i->charges > 0 ) {
                i->charges--;
                damage_chance -= material_factor;
            }
        } else {
            const field_id type_blood = i->is_corpse() ? i->get_mtype()->bloodType() : fd_null;
            while( ( damage_chance > material_factor ||
                     x_in_y( damage_chance, material_factor ) ) &&
                   i->damage() < i->max_damage() ) {
                i->inc_damage( DT_BASH );
                add_splash( type_blood, p, 1, damage_chance );
                damage_chance -= material_factor;
            }
        }
        // Remove them if they were damaged too much
        if( i->damage() == i->max_damage() || ( by_charges && i->charges == 0 ) ) {
            // But save the contents, except for irremovable gunmods
            for( auto &elem : i->contents ) {
                if( !elem.is_irremovable() ) {
                    contents.push_back( elem );
                }
            }

            i = i_rem( p, i );
        } else {
            i++;
        }
    }

    for( const item &it : contents ) {
        add_item_or_charges( p, it );
    }
}

ter_id map::get_roof( const tripoint &p, const bool allow_air )
{
    // This function should not be called from the 2D mode
    // Just use t_dirt instead
    assert( zlevels );

    if( p.z <= -OVERMAP_DEPTH ) {
        // Could be magma/"void" instead
        return t_rock_floor;
    }

    const auto &ter_there = ter( p ).obj();
    const auto &roof = ter_there.roof;
    if( !roof ) {
        // No roof
        // Not acceptable if the tile is not passable
        if( !allow_air ) {
            return t_dirt;
        }

        return t_open_air;
    }

    ter_id new_ter = roof.id();
    if( new_ter == t_null ) {
        debugmsg( "map::get_new_floor: %d,%d,%d has invalid roof type %s",
                  p.x, p.y, p.z, roof.c_str() );
        return t_dirt;
    }

    if( p.z == -1 && new_ter == t_rock_floor ) {
        // A hack to work around not having a "solid earth" tile
        new_ter = t_dirt;
    }

    return new_ter;
}

void map::bash_ter_furn( const tripoint &p, bash_params &params )
{
    std::string sound;
    int sound_volume = 0;
    std::string soundfxid;
    std::string soundfxvariant;
    const auto &terid = ter( p ).obj();
    const auto &furnid = furn( p ).obj();
    bool smash_furn = false;
    bool smash_ter = false;
    const map_bash_info *bash = nullptr;

    bool success = false;

    if( has_furn( p ) && furnid.bash.str_max != -1 ) {
        bash = &furnid.bash;
        smash_furn = true;
    } else if( ter( p ).obj().bash.str_max != -1 ) {
        bash = &ter( p ).obj().bash;
        smash_ter = true;
    }

    // Floor bashing check
    // Only allow bashing floors when we want to bash floors and we're in z-level mode
    // Unless we're destroying, then it gets a little weird
    if( smash_ter && bash->bash_below && ( !zlevels || !params.bash_floor ) ) {
        if( !params.destroy ) {
            smash_ter = false;
            bash = nullptr;
        } else if( !bash->ter_set && zlevels ) {
            // A hack for destroy && !bash_floor
            // We have to check what would we create and cancel if it is what we have now
            tripoint below( p.x, p.y, p.z - 1 );
            const auto roof = get_roof( below, false );
            if( roof == ter( p ) ) {
                smash_ter = false;
                bash = nullptr;
            }
        } else if( !bash->ter_set && ter( p ) == t_dirt ) {
            // As above, except for no-z-levels case
            smash_ter = false;
            bash = nullptr;
        }
    }

    // TODO: what if silent is true?
    if( has_flag( "ALARMED", p ) && !g->events.queued( EVENT_WANTED ) ) {
        sounds::sound( p, 40, sounds::sound_t::alarm, _( "an alarm go off!" ),
                       false, "environment", "alarm" );
        // Blame nearby player
        if( rl_dist( g->u.pos(), p ) <= 3 ) {
            g->u.add_memorial_log( pgettext( "memorial_male", "Set off an alarm." ),
                                   pgettext( "memorial_female", "Set off an alarm." ) );
            const point abs = ms_to_sm_copy( getabs( p.x, p.y ) );
            g->events.add( EVENT_WANTED, calendar::turn + 30_minutes, 0, tripoint( abs.x, abs.y, p.z ) );
        }
    }

    if( bash == nullptr || ( bash->destroy_only && !params.destroy ) ) {
        // Nothing bashable here
        if( impassable( p ) ) {
            if( !params.silent ) {
                sounds::sound( p, 18, sounds::sound_t::combat, _( "thump!" ),
                               false, "smash_thump", "smash_success" );
            }

            params.did_bash = true;
            params.bashed_solid = true;
        }

        return;
    }

    int smin = bash->str_min;
    int smax = bash->str_max;
    int sound_vol = bash->sound_vol;
    int sound_fail_vol = bash->sound_fail_vol;
    if( !params.destroy ) {
        if( bash->str_min_blocked != -1 || bash->str_max_blocked != -1 ) {
            if( has_adjacent_furniture( p ) ) {
                if( bash->str_min_blocked != -1 ) {
                    smin = bash->str_min_blocked;
                }
                if( bash->str_max_blocked != -1 ) {
                    smax = bash->str_max_blocked;
                }
            }
        }

        if( bash->str_min_supported != -1 || bash->str_max_supported != -1 ) {
            tripoint below( p.x, p.y, p.z - 1 );
            if( !zlevels || has_flag( "SUPPORTS_ROOF", below ) ) {
                if( bash->str_min_supported != -1 ) {
                    smin = bash->str_min_supported;
                }
                if( bash->str_max_supported != -1 ) {
                    smax = bash->str_max_supported;
                }
            }
        }
        // Linear interpolation from str_min to str_max
        const int resistance = smin + ( params.roll * ( smax - smin ) );
        if( params.strength >= resistance ) {
            success = true;
        }
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

        sound = bash->sound_fail.empty() ? _( "Thnk!" ) : _( bash->sound_fail.c_str() );
        params.did_bash = true;
        if( !params.silent ) {
            sounds::sound( p, sound_volume, sounds::sound_t::combat, sound, false,
                           "smash_fail", soundfxvariant );
        }

        return;
    }

    // Clear out any partially grown seeds
    if( has_flag_ter_or_furn( "PLANT", p ) ) {
        i_clear( p );
    }

    if( ( smash_furn && has_flag_furn( "FUNGUS", p ) ) ||
        ( smash_ter && has_flag_ter( "FUNGUS", p ) ) ) {
        fungal_effects( *g, *this ).create_spores( p );
    }

    if( params.destroy ) {
        sound_volume = smin * 2;
    } else {
        if( sound_vol == -1 ) {
            sound_volume = std::min( static_cast<int>( smin * 1.5 ), smax );
        } else {
            sound_volume = sound_vol;
        }
    }

    soundfxid = "smash_success";
    sound = _( bash->sound.c_str() );
    // Set this now in case the ter_set below changes this
    const bool collapses = smash_ter && has_flag( "COLLAPSES", p );
    const bool supports = smash_ter && has_flag( "SUPPORTS_ROOF", p );

    const bool tent = smash_furn && !bash->tent_centers.empty();
    // Special code to collapse the tent if destroyed
    if( tent ) {
        // Get ids of possible centers
        std::set<furn_id> centers;
        for( const auto &cur_id : bash->tent_centers ) {
            if( cur_id.is_valid() ) {
                centers.insert( cur_id );
            }
        }

        cata::optional<std::pair<tripoint, furn_id>> tentp;

        // Find the center of the tent
        // First check if we're not currently bashing the center
        if( centers.count( furn( p ) ) > 0 ) {
            tentp.emplace( p, furn( p ) );
        } else {
            for( const tripoint &pt : points_in_radius( p, bash->collapse_radius ) ) {
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
            spawn_items( p, item_group::items_from( bash->drop_group, calendar::turn ) );
            furn_set( p, bash->furn_set );
        } else {
            // Take the tent down
            const int rad = tentp->second.obj().bash.collapse_radius;
            for( const tripoint &pt : points_in_radius( tentp->first, rad ) ) {
                const auto frn = furn( pt );
                if( frn == f_null ) {
                    continue;
                }

                const auto recur_bash = &frn.obj().bash;
                // Check if we share a center type and thus a "tent type"
                for( const auto &cur_id : recur_bash->tent_centers ) {
                    if( centers.count( cur_id.id() ) > 0 ) {
                        // Found same center, wreck current tile
                        spawn_items( p, item_group::items_from( recur_bash->drop_group, calendar::turn ) );
                        furn_set( pt, recur_bash->furn_set );
                        break;
                    }
                }
            }
        }
        soundfxvariant = "smash_cloth";
    } else if( smash_furn ) {
        furn_set( p, bash->furn_set );
        for( item &it : i_at( p ) )  {
            it.on_drop( p, *this );
        }
        // Hack alert.
        // Signs have cosmetics associated with them on the submap since
        // furniture can't store dynamic data to disk. To prevent writing
        // mysteriously appearing for a sign later built here, remove the
        // writing from the submap.
        delete_signage( p );
    } else if( !smash_ter ) {
        // Handle error earlier so that we can assume smash_ter is true below
        debugmsg( "data/json/terrain.json does not have %s.bash.ter_set set!",
                  ter( p ).obj().id.c_str() );
    } else if( params.bashing_from_above && bash->ter_set_bashed_from_above ) {
        // If this terrain is being bashed from above and this terrain
        // has a valid post-destroy bashed-from-above terrain, set it
        ter_set( p, bash->ter_set_bashed_from_above );
    } else if( bash->ter_set ) {
        // If the terrain has a valid post-destroy terrain, set it
        ter_set( p, bash->ter_set );
    } else {
        tripoint below( p.x, p.y, p.z - 1 );
        const auto &ter_below = ter( below ).obj();
        if( bash->bash_below && ter_below.has_flag( "SUPPORTS_ROOF" ) ) {
            // When bashing the tile below, don't allow bashing the floor
            bash_params params_below = params; // Make a copy
            params_below.bashing_from_above = true;
            bash_ter_furn( below, params_below );
        }

        ter_set( p, t_open_air );
    }

    if( !tent ) {
        spawn_items( p, item_group::items_from( bash->drop_group, calendar::turn ) );
    }

    if( smash_ter && ter( p ) == t_open_air ) {
        if( !zlevels ) {
            // We destroyed something, so we aren't just "plugging" air with dirt here
            ter_set( p, t_dirt );
        } else {
            tripoint below( p.x, p.y, p.z - 1 );
            const auto roof = get_roof( below, params.bash_floor && ter( below ).obj().movecost != 0 );
            ter_set( p, roof );
        }
    }

    if( bash->explosive > 0 ) {
        g->explosion( p, bash->explosive, 0.8, false );
    }

    if( collapses ) {
        collapse_at( p, params.silent );
    }
    // Check the flag again to ensure the new terrain doesn't support anything
    if( supports && !has_flag( "SUPPORTS_ROOF", p ) ) {
        for( const tripoint &t : points_in_radius( p, 1 ) ) {
            if( p == t || !has_flag( "COLLAPSES", t ) ) {
                continue;
            }

            if( one_in( collapse_check( t ) ) ) {
                collapse_at( t, params.silent );
            }
        }
    }

    params.did_bash = true;
    params.success |= success; // Not always true, so that we can tell when to stop destroying
    params.bashed_solid = true;
    if( !sound.empty() && !params.silent ) {
        sounds::sound( p, sound_volume, sounds::sound_t::combat, sound, false,
                       soundfxid, soundfxvariant );
    }
}

bash_params map::bash( const tripoint &p, const int str,
                       bool silent, bool destroy, bool bash_floor,
                       const vehicle *bashing_vehicle )
{
    bash_params bsh{
        str, silent, destroy, bash_floor, static_cast<float>( rng_float( 0, 1.0f ) ), false, false, false, false
    };
    if( !inbounds( p ) ) {
        return bsh;
    }

    bash_field( p, bsh );
    bash_items( p, bsh );
    // Don't bash the vehicle doing the bashing
    const vehicle *veh = veh_pointer_or_null( veh_at( p ) );
    if( veh != nullptr && veh != bashing_vehicle ) {
        bash_vehicle( p, bsh );
    }

    // If we still didn't bash anything solid (a vehicle), bash furn/ter
    if( !bsh.bashed_solid ) {
        bash_ter_furn( p, bsh );
    }

    return bsh;
}

void map::bash_items( const tripoint &p, bash_params &params )
{
    if( !has_items( p ) ) {
        return;
    }

    std::vector<item> smashed_contents;
    auto bashed_items = i_at( p );
    bool smashed_glass = false;
    for( auto bashed_item = bashed_items.begin(); bashed_item != bashed_items.end(); ) {
        // the check for active suppresses Molotovs smashing themselves with their own explosion
        if( bashed_item->made_of( material_id( "glass" ) ) && !bashed_item->active && one_in( 2 ) ) {
            params.did_bash = true;
            smashed_glass = true;
            for( const item &bashed_content : bashed_item->contents ) {
                smashed_contents.push_back( bashed_content );
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
        sounds::sound( p, 12, sounds::sound_t::combat, _( "glass shattering" ), false,
                       "smash_success", "smash_glass_contents" );
    }
}

void map::bash_vehicle( const tripoint &p, bash_params &params )
{
    // Smash vehicle if present
    if( const optional_vpart_position vp = veh_at( p ) ) {
        vp->vehicle().damage( vp->part_index(), params.strength, DT_BASH );
        if( !params.silent ) {
            sounds::sound( p, 18, sounds::sound_t::combat, _( "crash!" ), false,
                           "smash_success", "hit_vehicle" );
        }

        params.did_bash = true;
        params.success = true;
        params.bashed_solid = true;
    }
}

void map::bash_field( const tripoint &p, bash_params &params )
{
    if( get_field( p, fd_web ) != nullptr ) {
        params.did_bash = true;
        params.bashed_solid = true; // To prevent bashing furniture/vehicles
        remove_field( p, fd_web );
    }
}

void map::destroy( const tripoint &p, const bool silent )
{
    // Break if it takes more than 25 destructions to remove to prevent infinite loops
    // Example: A bashes to B, B bashes to A leads to A->B->A->...
    int count = 0;
    while( count <= 25 && bash( p, 999, silent, true ).success ) {
        count++;
    }
}

void map::destroy_furn( const tripoint &p, const bool silent )
{
    // Break if it takes more than 25 destructions to remove to prevent infinite loops
    // Example: A bashes to B, B bashes to A leads to A->B->A->...
    int count = 0;
    while( count <= 25 && furn( p ) != f_null && bash( p, 999, silent, true ).success ) {
        count++;
    }
}

void map::crush( const tripoint &p )
{
    player *crushed_player = g->critter_at<player>( p );

    if( crushed_player != nullptr ) {
        bool player_inside = false;
        if( crushed_player->in_vehicle ) {
            const optional_vpart_position vp = veh_at( p );
            player_inside = vp && vp->is_inside();
        }
        if( !player_inside ) { //If there's a player at p and he's not in a covered vehicle...
            //This is the roof coming down on top of us, no chance to dodge
            crushed_player->add_msg_player_or_npc( m_bad, _( "You are crushed by the falling debris!" ),
                                                   _( "<npcname> is crushed by the falling debris!" ) );
            // TODO: Make this depend on the ceiling material
            const int dam = rng( 0, 40 );
            // Torso and head take the brunt of the blow
            body_part hit = bp_head;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .25 ) );
            hit = bp_torso;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .45 ) );
            // Legs take the next most through transferred force
            hit = bp_leg_l;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .10 ) );
            hit = bp_leg_r;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .10 ) );
            // Arms take the least
            hit = bp_arm_l;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .05 ) );
            hit = bp_arm_r;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .05 ) );

            // Pin whoever got hit
            crushed_player->add_effect( effect_crushed, 1_turns, num_bp, true );
            crushed_player->check_dead_state();
        }
    }

    if( monster *const monhit = g->critter_at<monster>( p ) ) {
        // 25 ~= 60 * .45 (torso)
        monhit->deal_damage( nullptr, bp_torso, damage_instance( DT_BASH, rng( 0, 25 ) ) );

        // Pin whoever got hit
        monhit->add_effect( effect_crushed, 1_turns, num_bp, true );
        monhit->check_dead_state();
    }

    if( const optional_vpart_position vp = veh_at( p ) ) {
        // Arbitrary number is better than collapsing house roof crushing APCs
        vp->vehicle().damage( vp->part_index(), rng( 100, 1000 ), DT_BASH, false );
    }
}

void map::shoot( const tripoint &p, projectile &proj, const bool hit_items )
{
    // TODO: Make bashing count fully, but other types much less
    const float initial_damage = proj.impact.total_damage();
    if( initial_damage < 0 ) {
        return;
    }

    float dam = initial_damage;
    const auto &ammo_effects = proj.proj_effects;

    if( has_flag( "ALARMED", p ) && !g->events.queued( EVENT_WANTED ) ) {
        sounds::sound( p, 30, sounds::sound_t::alarm, _( "an alarm sound!" ) );
        const tripoint abs = ms_to_sm_copy( getabs( p ) );
        g->events.add( EVENT_WANTED, calendar::turn + 30_minutes, 0, abs );
    }

    const bool inc = ( ammo_effects.count( "INCENDIARY" ) || ammo_effects.count( "FLAME" ) );
    if( const optional_vpart_position vp = veh_at( p ) ) {
        dam = vp->vehicle().damage( vp->part_index(), dam, inc ? DT_HEAT : DT_STAB, hit_items );
    }
    const auto break_glass = []( const tripoint & p, int vol ) {
        sounds::sound( p, vol, sounds::sound_t::combat, _( "glass breaking!" ), false,
                       "smash", "glass" );
    };

    ter_id terrain = ter( p );
    if( terrain == t_wall_wood_broken ||
        terrain == t_wall_log_broken ||
        terrain == t_door_b ) {
        if( hit_items || one_in( 8 ) ) { // 1 in 8 chance of hitting the door
            dam -= rng( 20, 40 );
            if( dam > 0 ) {
                sounds::sound( p, 10, sounds::sound_t::combat, _( "crash!" ), false,
                               "smash", "wall" );
                ter_set( p, t_dirt );
            }
        } else {
            dam -= rng( 0, 1 );
        }
    } else if( terrain == t_door_c ||
               terrain == t_door_locked ||
               terrain == t_door_locked_peep ||
               terrain == t_door_locked_alarm ) {
        dam -= rng( 15, 30 );
        if( dam > 0 ) {
            sounds::sound( p, 10, sounds::sound_t::combat, _( "smash!" ), false, "smash", "door" );
            ter_set( p, t_door_b );
        }
    } else if( terrain == t_door_boarded ||
               terrain == t_door_boarded_damaged ||
               terrain == t_rdoor_boarded ||
               terrain == t_rdoor_boarded_damaged ) {
        dam -= rng( 15, 35 );
        if( dam > 0 ) {
            sounds::sound( p, 10, sounds::sound_t::combat, _( "crash!" ), false,
                           "smash", "door_boarded" );
            ter_set( p, t_door_b );
        }
    } else if( terrain == t_window_domestic_taped ||
               terrain == t_curtains ||
               terrain == t_window_domestic ) {
        if( ammo_effects.count( "LASER" ) ) {
            if( terrain == t_window_domestic_taped ||
                terrain == t_curtains ) {
                dam -= rng( 1, 5 );
            }
            dam -= rng( 0, 5 );
        } else {
            dam -= rng( 1, 3 );
            if( dam > 0 ) {
                break_glass( p, 16 );
                ter_set( p, t_window_frame );
                spawn_item( p, "sheet", 1 );
                spawn_item( p, "stick" );
                spawn_item( p, "string_36" );
            }
        }
    } else if( terrain == t_window_taped ||
               terrain == t_window_alarm_taped ||
               terrain == t_window ||
               terrain == t_window_no_curtains ||
               terrain == t_window_no_curtains_taped ||
               terrain == t_window_alarm ) {
        if( ammo_effects.count( "LASER" ) ) {
            if( terrain == t_window_taped ||
                terrain == t_window_alarm_taped ||
                terrain == t_window_no_curtains_taped ) {
                dam -= rng( 1, 5 );
            }
            dam -= rng( 0, 5 );
        } else {
            dam -= rng( 1, 3 );
            if( dam > 0 ) {
                break_glass( p, 16 );
                ter_set( p, t_window_frame );
            }
        }
    } else if( terrain == t_window_bars_alarm ) {
        dam -= rng( 1, 3 );
        if( dam > 0 ) {
            break_glass( p, 16 );
            ter_set( p, t_window_bars );
            spawn_item( p, "glass_shard", 5 );
        }
    } else if( terrain == t_window_boarded ) {
        dam -= rng( 10, 30 );
        if( dam > 0 ) {
            break_glass( p, 16 );
            ter_set( p, t_window_frame );
        }
    } else if( terrain == t_wall_glass  ||
               terrain == t_wall_glass_alarm ||
               terrain == t_door_glass_c ) {
        if( ammo_effects.count( "LASER" ) ) {
            dam -= rng( 0, 5 );
        } else {
            dam -= rng( 1, 8 );
            if( dam > 0 ) {
                break_glass( p, 16 );
                ter_set( p, t_floor );
            }
        }
    } else if( terrain == t_reinforced_glass || terrain == t_reinforced_door_glass_c ) {
        // reinforced glass stops most bullets
        // laser beams are attenuated
        if( ammo_effects.count( "LASER" ) ) {
            dam -= rng( 0, 8 );
        } else {
            //Greatly weakens power of bullets
            dam -= 40;
            if( dam <= 0 && g->u.sees( p ) ) {
                if( terrain == t_reinforced_glass ) {
                    add_msg( _( "The shot is stopped by the reinforced glass wall!" ) );
                } else {
                    add_msg( _( "The shot is stopped by the reinforced glass door!" ) );
                }
            } else if( dam >= 40 ) {
                //high powered bullets penetrate the glass, but only extremely strong
                // ones (80 before reduction) actually destroy the glass itself.
                break_glass( p, 16 );
                ter_set( p, t_floor );
            }
        }
    } else if( terrain == t_paper ) {
        dam -= rng( 4, 16 );
        if( dam > 0 ) {
            sounds::sound( p, 8, sounds::sound_t::combat, _( "rrrrip!" ) );
            ter_set( p, t_dirt );
        }
        if( inc ) {
            add_field( p, fd_fire, 1 );
        }
    } else if( terrain == t_gas_pump ) {
        if( hit_items || one_in( 3 ) ) {
            if( dam > 15 ) {
                if( inc ) {
                    g->explosion( p, 40, 0.8, true );
                } else {
                    for( const tripoint &pt : points_in_radius( p, 2 ) ) {
                        if( one_in( 3 ) && passable( pt ) ) {
                            int gas_amount = rng( 10, 100 );
                            item gas_spill( "gasoline", calendar::turn );
                            gas_spill.charges = gas_amount;
                            add_item_or_charges( pt, gas_spill );
                        }
                    }

                    sounds::sound( p, 10, sounds::sound_t::combat, _( "smash!" ) );
                }
                ter_set( p, t_gas_pump_smashed );
            }
            dam -= 60;
        }
    } else if( terrain == t_vat ) {
        if( dam >= 10 ) {
            sounds::sound( p, 20, sounds::sound_t::combat, _( "ke-rash!" ) );
            ter_set( p, t_floor );
        } else {
            dam = 0;
        }
    } else if( impassable( p ) && !trans( p ) ) {
        bash( p, dam, false );
        dam = 0; // TODO: Preserve some residual damage when it makes sense.
    }

    if( ammo_effects.count( "TRAIL" ) && !one_in( 4 ) ) {
        add_field( p, fd_smoke, rng( 1, 2 ) );
    }

    if( ammo_effects.count( "STREAM" ) && !one_in( 3 ) ) {
        add_field( p, fd_fire, rng( 1, 2 ) );
    }

    if( ammo_effects.count( "STREAM_GAS_FUNGICIDAL" ) && !one_in( 3 ) ) {
        add_field( p, fd_fungicidal_gas, rng( 1, 2 ) );
    }

    if( ammo_effects.count( "STREAM_BIG" ) && !one_in( 4 ) ) {
        add_field( p, fd_fire, 2 );
    }

    if( ammo_effects.count( "LIGHTNING" ) ) {
        add_field( p, fd_electricity, rng( 2, 3 ) );
    }

    if( ammo_effects.count( "PLASMA" ) && one_in( 2 ) ) {
        add_field( p, fd_plasma, rng( 1, 2 ) );
    }

    if( ammo_effects.count( "LASER" ) || ammo_effects.count( "DRAW_LASER_BEAM" ) ) {
        add_field( p, fd_laser, 2 );
    }

    dam = std::max( 0.0f, dam );

    // Check fields?
    const field_entry *fieldhit = get_field( p, fd_web );
    if( fieldhit != nullptr ) {
        if( inc ) {
            add_field( p, fd_fire, fieldhit->getFieldDensity() - 1 );
        } else if( dam > 5 + fieldhit->getFieldDensity() * 5 &&
                   one_in( 5 - fieldhit->getFieldDensity() ) ) {
            dam -= rng( 1, 2 + fieldhit->getFieldDensity() * 2 );
            remove_field( p, fd_web );
        }
    }

    // Rescale the damage
    if( dam <= 0 ) {
        proj.impact.damage_units.clear();
        return;
    } else if( dam < initial_damage ) {
        proj.impact.mult_damage( dam / static_cast<double>( initial_damage ) );
    }

    // Now, destroy items on that tile.
    if( ( move_cost( p ) == 2 && !hit_items ) || !inbounds( p ) ) {
        return; // Items on floor-type spaces won't be shot up.
    }

    // dam / 3, because bullets aren't all that good at destroying items...
    smash_items( p, dam / 3 );
}

bool map::hit_with_acid( const tripoint &p )
{
    if( passable( p ) ) {
        return false;    // Didn't hit the tile!
    }
    const ter_id t = ter( p );
    if( t == t_wall_glass || t == t_wall_glass_alarm ||
        t == t_vat ) {
        ter_set( p, t_floor );
    } else if( t == t_door_c || t == t_door_locked || t == t_door_locked_peep ||
               t == t_door_locked_alarm ) {
        if( one_in( 3 ) ) {
            ter_set( p, t_door_b );
        }
    } else if( t == t_door_bar_c || t == t_door_bar_o || t == t_door_bar_locked || t == t_bars ||
               t == t_reb_cage ) {
        ter_set( p, t_floor );
        add_msg( m_warning, _( "The metal bars melt!" ) );
    } else if( t == t_door_b ) {
        if( one_in( 4 ) ) {
            ter_set( p, t_door_frame );
        } else {
            return false;
        }
    } else if( t == t_window || t == t_window_alarm || t == t_window_no_curtains ) {
        ter_set( p, t_window_empty );
    } else if( t == t_wax ) {
        ter_set( p, t_floor_wax );
    } else if( t == t_gas_pump || t == t_gas_pump_smashed ) {
        return false;
    } else if( t == t_card_science || t == t_card_military ) {
        ter_set( p, t_card_reader_broken );
    }
    return true;
}

// returns true if terrain stops fire
bool map::hit_with_fire( const tripoint &p )
{
    if( passable( p ) ) {
        return false;    // Didn't hit the tile!
    }

    // non passable but flammable terrain, set it on fire
    if( has_flag( "FLAMMABLE", p ) || has_flag( "FLAMMABLE_ASH", p ) ) {
        add_field( p, fd_fire, 3 );
    }
    return true;
}

bool map::open_door( const tripoint &p, const bool inside, const bool check_only )
{
    const auto &ter = this->ter( p ).obj();
    const auto &furn = this->furn( p ).obj();
    if( ter.open ) {
        if( has_flag( "OPENCLOSE_INSIDE", p ) && !inside ) {
            return false;
        }

        if( !check_only ) {
            sounds::sound( p, 6, sounds::sound_t::movement, _( "swish" ), true,
                           "open_door", ter.id.str() );
            ter_set( p, ter.open );

            if( ( g->u.has_trait( trait_id( "SCHIZOPHRENIC" ) ) || g->u.has_artifact_with( AEP_SCHIZO ) )
                && one_in( 50 ) && !ter.has_flag( "TRANSPARENT" ) ) {
                tripoint mp = p + tripoint( ( p.x - g->u.pos().x ) * 2, ( p.y - g->u.pos().y ) * 2, p.z );
                g->spawn_hallucination( mp );
            }
        }

        return true;
    } else if( furn.open ) {
        if( has_flag( "OPENCLOSE_INSIDE", p ) && !inside ) {
            return false;
        }

        if( !check_only ) {
            sounds::sound( p, 6, sounds::sound_t::movement, _( "swish" ), true,
                           "open_door", furn.id.str() );
            furn_set( p, furn.open );
        }

        return true;
    } else if( const optional_vpart_position vp = veh_at( p ) ) {
        int openable = vp->vehicle().next_part_to_open( vp->part_index(), true );
        if( openable >= 0 ) {
            if( !check_only ) {
                vp->vehicle().open_all_at( openable );
            }

            return true;
        }

        return false;
    }

    return false;
}

void map::translate( const ter_id &from, const ter_id &to )
{
    if( from == to ) {
        debugmsg( "map::translate %s => %s",
                  from.obj().name().c_str(),
                  from.obj().name().c_str() );
        return;
    }

    tripoint p( 0, 0, abs_sub.z );
    int &x = p.x;
    int &y = p.y;
    for( x = 0; x < SEEX * my_MAPSIZE; x++ ) {
        for( y = 0; y < SEEY * my_MAPSIZE; y++ ) {
            if( ter( p ) == from ) {
                ter_set( p, to );
            }
        }
    }
}

//This function performs the translate function within a given radius of the player.
void map::translate_radius( const ter_id &from, const ter_id &to, float radi, const tripoint &p,
                            const bool same_submap )
{
    if( from == to ) {
        debugmsg( "map::translate %s => %s",
                  from.obj().name().c_str(),
                  from.obj().name().c_str() );
        return;
    }

    const int uX = p.x;
    const int uY = p.y;
    tripoint t( 0, 0, abs_sub.z );
    int &x = t.x;
    int &y = t.y;
    for( x = 0; x < SEEX * my_MAPSIZE; x++ ) {
        for( y = 0; y < SEEY * my_MAPSIZE; y++ ) {
            if( ter( t ) == from ) {
                float radiX = sqrt( static_cast<float>( ( uX - x ) * ( uX - x ) + ( uY - y ) * ( uY - y ) ) );
                // within distance, and either no submap limitation or same overmap coords.
                if( radiX <= radi && ( !same_submap ||
                                       ms_to_omt_copy( getabs( x, y ) ) == ms_to_omt_copy( getabs( uX, uY ) ) ) ) {
                    ter_set( t, to );
                }
            }
        }
    }
}

bool map::close_door( const tripoint &p, const bool inside, const bool check_only )
{
    if( has_flag( "OPENCLOSE_INSIDE", p ) && !inside ) {
        return false;
    }

    const auto &ter = this->ter( p ).obj();
    const auto &furn = this->furn( p ).obj();
    if( ter.close && !furn.id ) {
        if( !check_only ) {
            sounds::sound( p, 10, sounds::sound_t::movement, _( "swish" ), true,
                           "close_door", ter.id.str() );
            ter_set( p, ter.close );
        }
        return true;
    } else if( furn.close ) {
        if( !check_only ) {
            sounds::sound( p, 10, sounds::sound_t::movement, _( "swish" ), true,
                           "close_door", furn.id.str() );
            furn_set( p, furn.close );
        }
        return true;
    }
    return false;
}

const std::string map::get_signage( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return "";
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->get_signage( l );
}
void map::set_signage( const tripoint &p, const std::string &message ) const
{
    if( !inbounds( p ) ) {
        return;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    current_submap->set_signage( l, message );
}
void map::delete_signage( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    current_submap->delete_signage( l );
}

int map::get_radiation( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return 0;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->get_radiation( l );
}

void map::set_radiation( const int x, const int y, const int value )
{
    set_radiation( tripoint( x, y, abs_sub.z ), value );
}

void map::set_radiation( const tripoint &p, const int value )
{
    if( !inbounds( p ) ) {
        return;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    current_submap->set_radiation( l, value );
}

void map::adjust_radiation( const int x, const int y, const int delta )
{
    adjust_radiation( tripoint( x, y, abs_sub.z ), delta );
}

void map::adjust_radiation( const tripoint &p, const int delta )
{
    if( !inbounds( p ) ) {
        return;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    int current_radiation = current_submap->get_radiation( l );
    current_submap->set_radiation( l, current_radiation + delta );
}

int &map::temperature( const tripoint &p )
{
    if( !inbounds( p ) ) {
        null_temperature = 0;
        return null_temperature;
    }

    return get_submap_at( p )->temperature;
}

void map::set_temperature( const tripoint &p, int new_temperature )
{
    temperature( p ) = new_temperature;
    temperature( tripoint( p.x + SEEX, p.y, p.z ) ) = new_temperature;
    temperature( tripoint( p.x, p.y + SEEY, p.z ) ) = new_temperature;
    temperature( tripoint( p.x + SEEX, p.y + SEEY, p.z ) ) = new_temperature;
}

void map::set_temperature( const int x, const int y, int new_temperature )
{
    set_temperature( tripoint( x, y, abs_sub.z ), new_temperature );
}

// Items: 2D
map_stack map::i_at( const int x, const int y )
{
    const point p( x, y );
    if( !inbounds( p ) ) {
        nulitems.clear();
        return map_stack{ &nulitems, tripoint( p, abs_sub.z ), this };
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return map_stack{ &current_submap->itm[l.x][l.y], tripoint( p, abs_sub.z ), this };
}

std::list<item>::iterator map::i_rem( const point &location, std::list<item>::iterator it )
{
    return i_rem( tripoint( location, abs_sub.z ), it );
}

int map::i_rem( const int x, const int y, const int index )
{
    return i_rem( tripoint( x, y, abs_sub.z ), index );
}

void map::i_rem( const int x, const int y, item *it )
{
    i_rem( tripoint( x, y, abs_sub.z ), it );
}

void map::i_clear( const int x, const int y )
{
    i_clear( tripoint( x, y, abs_sub.z ) );
}

void map::spawn_an_item( const int x, const int y, item new_item,
                         const long charges, const int damlevel )
{
    spawn_an_item( tripoint( x, y, abs_sub.z ), new_item, charges, damlevel );
}

void map::spawn_items( const int x, const int y, const std::vector<item> &new_items )
{
    spawn_items( tripoint( x, y, abs_sub.z ), new_items );
}

void map::spawn_item( const int x, const int y, const std::string &type_id,
                      const unsigned quantity, const long charges,
                      const time_point &birthday, const int damlevel )
{
    spawn_item( tripoint( x, y, abs_sub.z ), type_id,
                quantity, charges, birthday, damlevel );
}

item &map::add_item_or_charges( const int x, const int y, item obj, bool overflow )
{
    return add_item_or_charges( tripoint( x, y, abs_sub.z ), obj, overflow );
}

void map::add_item( const int x, const int y, item new_item )
{
    add_item( tripoint( x, y, abs_sub.z ), new_item );
}

// Items: 3D

map_stack map::i_at( const tripoint &p )
{
    if( !inbounds( p ) ) {
        nulitems.clear();
        return map_stack{ &nulitems, p, this };
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return map_stack{ &current_submap->itm[l.x][l.y], p, this };
}

std::list<item>::iterator map::i_rem( const tripoint &p, std::list<item>::iterator it )
{
    point l;
    submap *const current_submap = get_submap_at( p, l );

    if( current_submap->active_items.has( it, l ) ) {
        current_submap->active_items.remove( it, l );
    }

    current_submap->update_lum_rem( l, *it );

    return current_submap->itm[l.x][l.y].erase( it );
}

int map::i_rem( const tripoint &p, const int index )
{
    if( index < 0 ) {
        debugmsg( "i_rem called with negative index %d", index );
        return index;
    }

    if( index >= static_cast<int>( i_at( p ).size() ) ) {
        return index;
    }

    auto map_items = i_at( p );
    auto iter = map_items.begin();
    std::advance( iter, index );
    map_items.erase( iter );
    return index;
}

void map::i_rem( const tripoint &p, const item *it )
{
    auto map_items = i_at( p );

    for( auto iter = map_items.begin(); iter != map_items.end(); iter++ ) {
        //delete the item if the pointer memory addresses are the same
        if( it == &*iter ) {
            map_items.erase( iter );
            break;
        }
    }
}

void map::i_clear( const tripoint &p )
{
    point l;
    submap *const current_submap = get_submap_at( p, l );

    for( auto item_it = current_submap->itm[l.x][l.y].begin();
         item_it != current_submap->itm[l.x][l.y].end(); ++item_it ) {
        if( current_submap->active_items.has( item_it, l ) ) {
            current_submap->active_items.remove( item_it, l );
        }
    }

    current_submap->lum[l.x][l.y] = 0;
    current_submap->itm[l.x][l.y].clear();
}

item &map::spawn_an_item( const tripoint &p, item new_item,
                          const long charges, const int damlevel )
{
    if( charges && new_item.charges > 0 ) {
        //let's fail silently if we specify charges for an item that doesn't support it
        new_item.charges = charges;
    }
    new_item = new_item.in_its_container();
    if( ( new_item.made_of( LIQUID ) && has_flag( "SWIMMABLE", p ) ) ||
        has_flag( "DESTROY_ITEM", p ) ) {
        return null_item_reference();
    }

    new_item.set_damage( damlevel );

    return add_item_or_charges( p, new_item );
}

std::vector<item *> map::spawn_items( const tripoint &p, const std::vector<item> &new_items )
{
    std::vector<item *> ret;
    if( !inbounds( p ) || has_flag( "DESTROY_ITEM", p ) ) {
        return ret;
    }
    const bool swimmable = has_flag( "SWIMMABLE", p );
    for( const item &new_item : new_items ) {

        if( new_item.made_of( LIQUID ) && swimmable ) {
            continue;
        }
        item &it = add_item_or_charges( p, new_item );
        if( !it.is_null() ) {
            ret.push_back( &it );
        }
    }

    return ret;
}

void map::spawn_artifact( const tripoint &p )
{
    add_item_or_charges( p, item( new_artifact(), 0 ) );
}

void map::spawn_natural_artifact( const tripoint &p, artifact_natural_property prop )
{
    add_item_or_charges( p, item( new_natural_artifact( prop ), 0 ) );
}

void map::spawn_item( const tripoint &p, const std::string &type_id,
                      const unsigned quantity, const long charges,
                      const time_point &birthday, const int damlevel )
{
    if( type_id == "null" ) {
        return;
    }

    if( item_is_blacklisted( type_id ) ) {
        return;
    }
    // recurse to spawn (quantity - 1) items
    for( size_t i = 1; i < quantity; i++ ) {
        spawn_item( p, type_id, 1, charges, birthday, damlevel );
    }
    // spawn the item
    item new_item( type_id, birthday );
    if( one_in( 3 ) && new_item.has_flag( "VARSIZE" ) ) {
        new_item.item_tags.insert( "FIT" );
    }

    spawn_an_item( p, new_item, charges, damlevel );
}

units::volume map::max_volume( const tripoint &p )
{
    return i_at( p ).max_volume();
}

// total volume of all the things
units::volume map::stored_volume( const tripoint &p )
{
    return i_at( p ).stored_volume();
}

// free space
units::volume map::free_volume( const tripoint &p )
{
    return i_at( p ).free_volume();
}

item &map::add_item_or_charges( const tripoint &pos, item obj, bool overflow )
{
    // Checks if item would not be destroyed if added to this tile
    auto valid_tile = [&]( const tripoint & e ) {
        if( !inbounds( e ) ) {
            dbg( D_INFO ) << e; // should never happen
            return false;
        }

        // Some tiles destroy items (e.g. lava)
        if( has_flag( "DESTROY_ITEM", e ) ) {
            return false;
        }

        // Cannot drop liquids into tiles that are comprised of liquid
        if( obj.made_of_from_type( LIQUID ) && has_flag( "SWIMMABLE", e ) ) {
            return false;
        }

        return true;
    };

    // Checks if sufficient space at tile to add item
    auto valid_limits = [&]( const tripoint & e ) {
        return obj.volume() <= free_volume( e ) && i_at( e ).size() < MAX_ITEM_IN_SQUARE;
    };

    // Performs the actual insertion of the object onto the map
    auto place_item = [&]( const tripoint & tile ) -> item& {
        if( obj.count_by_charges() )
        {
            for( auto &e : i_at( tile ) ) {
                if( e.merge_charges( obj ) ) {
                    return e;
                }
            }
        }

        support_dirty( tile );
        return add_item( tile, obj );
    };

    // Some items never exist on map as a discrete item (must be contained by another item)
    if( obj.has_flag( "NO_DROP" ) ) {
        return null_item_reference();
    }

    // If intended drop tile destroys the item then we don't attempt to overflow
    if( !valid_tile( pos ) ) {
        return null_item_reference();
    }

    if( ( !has_flag( "NOITEM", pos ) || ( has_flag( "LIQUIDCONT", pos ) && obj.made_of( LIQUID ) ) )
        && valid_limits( pos ) ) {
        // Pass map into on_drop, because this map may not be the global map object (in mapgen, for instance).
        if( obj.on_drop( pos, *this ) ) {
            return null_item_reference();
        }

        // If tile can contain items place here...
        return place_item( pos );

    } else if( overflow ) {
        // ...otherwise try to overflow to adjacent tiles (if permitted)
        auto tiles = closest_tripoints_first( 2, pos );
        tiles.erase( tiles.begin() ); // we already tried this position
        for( const auto &e : tiles ) {
            if( !inbounds( e ) ) {
                continue;
            }

            if( obj.on_drop( e, *this ) ) {
                return null_item_reference();
            }

            if( !valid_tile( e ) || has_flag( "NOITEM", e ) || !valid_limits( e ) ) {
                continue;
            }
            return place_item( e );
        }
    }

    // failed due to lack of space at target tile (+/- overflow tiles)
    return null_item_reference();
}

item &map::add_item( const tripoint &p, item new_item )
{
    if( !inbounds( p ) ) {
        return null_item_reference();
    }
    point l;
    submap *const current_submap = get_submap_at( p, l );

    // Process foods when they are added to the map, here instead of add_item_at()
    // to avoid double processing food during active item processing.
    if( /*new_item.needs_processing() &&*/ new_item.is_food() ) {
        new_item.process( nullptr, p, false );
    }
    return add_item_at( p, current_submap->itm[l.x][l.y].end(), new_item );
}

item &map::add_item_at( const tripoint &p,
                        std::list<item>::iterator index, item new_item )
{
    if( new_item.made_of( LIQUID ) && has_flag( "SWIMMABLE", p ) ) {
        return null_item_reference();
    }

    if( has_flag( "DESTROY_ITEM", p ) ) {
        return null_item_reference();
    }

    if( new_item.has_flag( "ACT_IN_FIRE" ) && get_field( p, fd_fire ) != nullptr ) {
        new_item.active = true;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );
    current_submap->is_uniform = false;

    if( new_item.is_map() && !new_item.has_var( "reveal_map_center_omt" ) ) {
        new_item.set_var( "reveal_map_center_omt", ms_to_omt_copy( g->m.getabs( p ) ) );
    }

    current_submap->update_lum_add( l, new_item );
    const auto new_pos = current_submap->itm[l.x][l.y].insert( index, new_item );
    if( new_item.needs_processing() ) {
        current_submap->active_items.add( new_pos, l );
    }

    return *new_pos;
}

item map::water_from( const tripoint &p )
{
    if( has_flag( "SALT_WATER", p ) ) {
        return item( "salt_water", 0, item::INFINITE_CHARGES );
    }

    const ter_id terrain_id = g->m.ter( p );
    if( terrain_id == t_sewage ) {
        item ret( "water_sewage", 0, item::INFINITE_CHARGES );
        ret.poison = rng( 1, 7 );
        return ret;
    }

    item ret( "water", 0, item::INFINITE_CHARGES );
    // iexamine::water_source requires a valid liquid from this function.
    if( terrain_id.obj().examine == &iexamine::water_source ) {
        int poison_chance = 0;
        if( terrain_id.obj().has_flag( TFLAG_DEEP_WATER ) ) {
            if( terrain_id.obj().has_flag( TFLAG_CURRENT ) ) {
                poison_chance = 20;
            } else {
                poison_chance = 4;
            }
        } else {
            if( terrain_id.obj().has_flag( TFLAG_CURRENT ) ) {
                poison_chance = 10;
            } else {
                poison_chance = 3;
            }
        }
        if( one_in( poison_chance ) ) {
            ret.poison = rng( 1, 4 );
        }
        return ret;
    }
    if( furn( p ).obj().examine == &iexamine::water_source ) {
        return ret;
    }
    return item();
}

void map::make_active( item_location &loc )
{
    item *target = loc.get_item();

    // Trust but verify, don't let stinking callers set items active when they shouldn't be.
    if( !target->needs_processing() ) {
        return;
    }
    point l;
    submap *const current_submap = get_submap_at( loc.position(), l );
    auto &item_stack = current_submap->itm[l.x][l.y];
    auto iter = std::find_if( item_stack.begin(), item_stack.end(),
    [&target]( const item & i ) {
        return &i == target;
    } );

    current_submap->active_items.add( iter, l );
}

void map::update_lum( item_location &loc, bool add )
{
    item *target = loc.get_item();

    // if the item is not emissive, do nothing
    if( !target->is_emissive() ) {
        return;
    }

    point l;
    submap *const current_submap = get_submap_at( loc.position(), l );

    if( add ) {
        current_submap->update_lum_add( l, *target );
    } else {
        current_submap->update_lum_rem( l, *target );
    }
}

// This is an ugly and dirty hack to prevent invalidating the item_location
// references the player is using for an activity.  What needs to happen is
// activity targets gets refactored in some way that it can reference items
// between turns that doesn't rely on a pointer to the item.  A really nice
// solution would be something like UUIDs but that requires special
// considerations.
static bool item_is_in_activity( const item *it )
{
    const auto targs = &g->u.activity.targets;
    return !targs->empty() &&
    std::find_if( targs->begin(), targs->end(), [it]( const item_location & it_loc ) {
        return it_loc.get_item() == it;
    } ) != targs->end();
}

static bool process_item( item_stack &items, std::list<item>::iterator &n, const tripoint &location,
                          const bool activate, const int temp, const float insulation )
{
    if( !item_is_in_activity( &*n ) ) {
        // make a temporary copy, remove the item (in advance)
        // and use that copy to process it
        item temp_item = *n;
        auto insertion_point = items.erase( n );
        if( !temp_item.process( nullptr, location, activate, temp, insulation ) ) {
            // Not destroyed, must be inserted again.
            // If the item lost its active flag in processing,
            // it won't be re-added to the active list, tidy!
            // Re-insert at the item's previous position.
            // This assumes that the item didn't invalidate any iterators
            // As a result of activation, because everything that does that
            // destroys itself.
            items.insert_at( insertion_point, temp_item );
            return false;
        }
        return true;
    } else if( n->process( nullptr, location, activate, temp, insulation ) ) {
        items.erase( n );
        return true;
    }
    return false;
}

static bool process_map_items( item_stack &items, std::list<item>::iterator &n,
                               const tripoint &location, const std::string &, const int temp,
                               const float insulation )
{
    return process_item( items, n, location, false, temp, insulation );
}

static void process_vehicle_items( vehicle &cur_veh, int part )
{
    const bool washmachine_here = cur_veh.part_flag( part, VPFLAG_WASHING_MACHINE ) &&
                                  cur_veh.is_part_on( part );
    bool washing_machine_finished = false;
    if( washmachine_here ) {
        for( auto &n : cur_veh.get_items( part ) ) {
            const time_duration washing_time = 90_minutes;
            const time_duration time_left = washing_time - n.age();
            static const std::string filthy( "FILTHY" );
            if( time_left <= 0_turns ) {
                n.item_tags.erase( filthy );
                washing_machine_finished = true;
                cur_veh.parts[part].enabled = false;
            } else if( calendar::once_every( 15_minutes ) ) {
                add_msg( _( "It should take %d minutes to finish washing items in the %s." ),
                         to_minutes<int>( time_left ) + 1, cur_veh.name.c_str() );
                break;
            }
        }
        if( washing_machine_finished ) {
            add_msg( _( "The washing machine in the %s has finished washing." ), cur_veh.name.c_str() );
        }
    }

    if( cur_veh.part_with_feature( part, VPFLAG_RECHARGE, true ) >= 0 &&
        cur_veh.has_part( "RECHARGE", true ) ) {
        for( auto &n : cur_veh.get_items( part ) ) {
            static const std::string recharge_s( "RECHARGE" );
            static const std::string ups_s( "USE_UPS" );
            if( !n.has_flag( recharge_s ) && !n.has_flag( ups_s ) ) {
                continue;
            }
            if( n.ammo_capacity() > n.ammo_remaining() ) {
                constexpr int per_charge = 10;
                const int missing = cur_veh.discharge_battery( per_charge, false );
                if( missing < per_charge &&
                    ( missing == 0 || x_in_y( per_charge - missing, per_charge ) ) ) {
                    n.ammo_set( "battery", n.ammo_remaining() + 1 );
                }

                if( missing > 0 ) {
                    // Not enough charge - stop charging
                    break;
                }
            }
        }
    }
}

void map::process_active_items()
{
    process_items( true, process_map_items, std::string {} );
}

void map::process_items( const bool active, map::map_process_func processor,
                         const std::string &signal )
{
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z;
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z;
    tripoint gp( 0, 0, 0 );
    int &gx = gp.x;
    int &gy = gp.y;
    int &gz = gp.z;
    for( gz = minz; gz <= maxz; ++gz ) {
        for( gx = 0; gx < my_MAPSIZE; ++gx ) {
            for( gy = 0; gy < my_MAPSIZE; ++gy ) {
                submap *const current_submap = get_submap_at_grid( gp );
                // Vehicles first in case they get blown up and drop active items on the map.
                if( !current_submap->vehicles.empty() ) {
                    process_items_in_vehicles( *current_submap, gz, processor, signal );
                }
            }
        }
    }
    for( gz = minz; gz <= maxz; ++gz ) {
        for( gx = 0; gx < my_MAPSIZE; ++gx ) {
            for( gy = 0; gy < my_MAPSIZE; ++gy ) {
                submap *const current_submap = get_submap_at_grid( gp );
                if( !active || !current_submap->active_items.empty() ) {
                    process_items_in_submap( *current_submap, gp, processor, signal );
                }
            }
        }
    }
}

void map::process_items_in_submap( submap &current_submap, const tripoint &gridp,
                                   map::map_process_func processor, const std::string &signal )
{
    // Get a COPY of the active item list for this submap.
    // If more are added as a side effect of processing, they are ignored this turn.
    // If they are destroyed before processing, they don't get processed.
    std::list<item_reference> active_items = current_submap.active_items.get();
    const auto grid_offset = point {gridp.x * SEEX, gridp.y * SEEY};
    for( auto &active_item : active_items ) {
        if( !current_submap.active_items.has( active_item ) ) {
            continue;
        }

        const tripoint map_location = tripoint( grid_offset + active_item.location, gridp.z );
        // root cellars are special
        const int loc_temp = g->m.ter( map_location ) == t_rootcellar ?
                             AVERAGE_ANNUAL_TEMPERATURE :
                             g->get_temperature( map_location );
        auto items = i_at( map_location );
        processor( items, active_item.item_iterator, map_location, signal, loc_temp, 1 );
    }
}

void map::process_items_in_vehicles( submap &current_submap, const int gridz,
                                     map::map_process_func processor, const std::string &signal )
{
    // a copy, important if the vehicle list changes because a
    // vehicle got destroyed by a bomb (an active item!), this list
    // won't change, but veh_in_nonant will change.
    std::vector<vehicle *> vehicles;
    for( const auto &veh : current_submap.vehicles ) {
        vehicles.push_back( veh.get() );
    }
    for( auto &cur_veh : vehicles ) {
        if( !current_submap.contains_vehicle( cur_veh ) ) {
            // vehicle not in the vehicle list of the nonant, has been
            // destroyed (or moved to another nonant?)
            // Can't be sure that it still exists, so skip it
            continue;
        }

        process_items_in_vehicle( *cur_veh, current_submap, gridz, processor, signal );
    }
}

void map::process_items_in_vehicle( vehicle &cur_veh, submap &current_submap, const int /*gridz*/,
                                    map::map_process_func processor, const std::string &signal )
{
    const bool engine_heater_is_on = cur_veh.has_part( "E_HEATER", true ) && cur_veh.engine_on;
    for( const vpart_reference &vp : cur_veh.get_any_parts( VPFLAG_FLUIDTANK ) ) {
        vp.part().process_contents( vp.pos(), engine_heater_is_on );
    }

    auto cargo_parts = cur_veh.get_parts_including_carried( VPFLAG_CARGO );
    for( const vpart_reference &vp : cargo_parts ) {
        process_vehicle_items( cur_veh, vp.part_index() );
    }

    for( auto &active_item : cur_veh.active_items.get() ) {
        if( empty( cargo_parts ) ) {
            return;
        } else if( !cur_veh.active_items.has( active_item ) ) {
            continue;
        }
        const auto it = std::find_if( begin( cargo_parts ),
        end( cargo_parts ), [&]( const vpart_reference & part ) {
            return active_item.location == part.mount();
        } );

        if( it == end( cargo_parts ) ) {
            continue; // Can't find a cargo part matching the active item.
        }
        auto &item_iter = active_item.item_iterator;
        // Find the cargo part and coordinates corresponding to the current active item.
        const vehicle_part &pt = it->part();
        const tripoint item_loc = it->pos();
        auto items = cur_veh.get_items( static_cast<int>( it->part_index() ) );
        int it_temp = g->get_temperature( item_loc );
        float it_insulation = 1.0;
        if( item_iter->is_food() || item_iter->is_food_container() ) {
            const vpart_info &pti = pt.info();
            if( engine_heater_is_on ) {
                it_temp = std::max( it_temp, temperatures::normal );
            }
            // some vehicle parts provide insulation, default is 1
            it_insulation = item::find_type( pti.item )->insulation_factor;

            if( pt.enabled && pti.has_flag( VPFLAG_FRIDGE ) ) {
                it_temp = std::min( it_temp, temperatures::fridge );
                it_insulation = 1; // ignore fridge insulation if on
            } else if( pt.enabled && pti.has_flag( VPFLAG_FREEZER ) ) {
                it_temp = std::min( it_temp, temperatures::freezer );
                it_insulation = 1; // ignore freezer insulation if on
            }
        }
        if( !processor( items, item_iter, item_loc, signal, it_temp, it_insulation ) ) {
            // If the item was NOT destroyed, we can skip the remainder,
            // which handles fallout from the vehicle being damaged.
            continue;
        }

        // item does not exist anymore, might have been an exploding bomb,
        // check if the vehicle is still valid (does exist)
        if( !current_submap.contains_vehicle( &cur_veh ) ) {
            // Nope, vehicle is not in the vehicle list of the submap,
            // it might have moved to another submap (unlikely)
            // or be destroyed, anyway it does not need to be processed here
            return;
        }

        // Vehicle still valid, reload the list of cargo parts,
        // the list of cargo parts might have changed (imagine a part with
        // a low index has been removed by an explosion, all the other
        // parts would move up to fill the gap).
        cargo_parts = cur_veh.get_any_parts( VPFLAG_CARGO );
    }
}

// Crafting/item finding functions

// Note: this is called quite a lot when drawing tiles
// Console build has the most expensive parts optimized out
bool map::sees_some_items( const tripoint &p, const Creature &who ) const
{
    // Can only see items if there are any items.
    return has_items( p ) && could_see_items( p, who );
}

bool map::could_see_items( const tripoint &p, const Creature &who ) const
{
    static const std::string container_string( "CONTAINER" );
    const bool container = has_flag_ter_or_furn( container_string, p );
    const bool sealed = has_flag_ter_or_furn( TFLAG_SEALED, p );
    if( sealed && container ) {
        // never see inside of sealed containers
        return false;
    }
    if( container ) {
        // can see inside of containers if adjacent or
        // on top of the container
        return ( abs( p.x - who.posx() ) <= 1 &&
                 abs( p.y - who.posy() ) <= 1 &&
                 abs( p.z - who.posz() ) <= 1 );
    }
    return true;
}

bool map::has_items( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return !current_submap->itm[l.x][l.y].empty();
}

template <typename Stack>
std::list<item> use_amount_stack( Stack stack, const itype_id type, long &quantity,
                                  const std::function<bool( const item & )> &filter )
{
    std::list<item> ret;
    for( auto a = stack.begin(); a != stack.end() && quantity > 0; ) {
        if( a->use_amount( type, quantity, ret, filter ) ) {
            a = stack.erase( a );
        } else {
            ++a;
        }
    }
    return ret;
}

std::list<item> map::use_amount_square( const tripoint &p, const itype_id type,
                                        long &quantity, const std::function<bool( const item & )> &filter )
{
    std::list<item> ret;
    // Handle infinite map sources.
    item water = water_from( p );
    if( water.typeId() == type ) {
        ret.push_back( water );
        quantity = 0;
        return ret;
    }

    if( const cata::optional<vpart_reference> vp = veh_at( p ).part_with_feature( "CARGO", true ) ) {
        std::list<item> tmp = use_amount_stack( vp->vehicle().get_items( vp->part_index() ), type,
                                                quantity, filter );
        ret.splice( ret.end(), tmp );
    }
    std::list<item> tmp = use_amount_stack( i_at( p ), type, quantity, filter );
    ret.splice( ret.end(), tmp );
    return ret;
}

std::list<item> map::use_amount( const tripoint &origin, const int range, const itype_id type,
                                 long &quantity, const std::function<bool( const item & )> &filter )
{
    std::list<item> ret;
    for( int radius = 0; radius <= range && quantity > 0; radius++ ) {
        for( const tripoint &p : points_in_radius( origin, radius ) ) {
            if( rl_dist( origin, p ) >= radius ) {
                std::list<item> tmp = use_amount_square( p, type, quantity, filter );
                ret.splice( ret.end(), tmp );
            }
        }
    }
    return ret;
}

template <typename Stack>
std::list<item> use_charges_from_stack( Stack stack, const itype_id type, long &quantity,
                                        const tripoint &pos, const std::function<bool( const item & )> &filter )
{
    std::list<item> ret;
    for( auto a = stack.begin(); a != stack.end() && quantity > 0; ) {
        if( filter( *a ) && !a->made_of( LIQUID ) && a->use_charges( type, quantity, ret, pos ) ) {
            a = stack.erase( a );
        } else {
            ++a;
        }
    }
    return ret;
}

long remove_charges_in_list( const itype *type, map_stack stack, long quantity )
{
    auto target = stack.begin();
    for( ; target != stack.end(); ++target ) {
        if( target->type == type ) {
            break;
        }
    }

    if( target != stack.end() ) {
        if( target->charges > quantity ) {
            target->charges -= quantity;
            return quantity;
        } else {
            const long charges = target->charges;
            target->charges = 0;
            if( target->destroyed_at_zero_charges() ) {
                stack.erase( target );
            }
            return charges;
        }
    }
    return 0;
}

void use_charges_from_furn( const furn_t &f, const itype_id &type, long &quantity,
                            map *m, const tripoint &p, std::list<item> &ret, const std::function<bool( const item & )> &filter )
{
    if( m->has_flag( "LIQUIDCONT", p ) ) {
        auto item_list = m->i_at( p );
        auto current_item = item_list.begin();
        for( ; current_item != item_list.end(); ++current_item ) {
            // looking for a liquid that matches
            if( filter( *current_item ) && current_item->made_of( LIQUID ) && type == current_item->typeId() ) {
                ret.push_back( *current_item );
                if( current_item->charges - quantity > 0 ) {
                    // Update the returned liquid amount to match the requested amount
                    ret.back().charges = quantity;
                    // Update the liquid item in the world to contain the leftover liquid
                    current_item->charges -= quantity;
                    // All the liquid needed was found, no other sources will be needed
                    quantity = 0;
                } else {
                    // The liquid copy in ret already contains how much was available
                    // The leftover quantity returned will check other sources
                    quantity -= current_item->charges;
                    // Remove liquid item from the world
                    item_list.erase( current_item );
                }
                return;
            }
        }
    }

    const itype *itt = f.crafting_pseudo_item_type();
    if( itt != nullptr && itt->tool && itt->tool->ammo_id ) {
        const itype_id ammo = itt->tool->ammo_id->default_ammotype();
        auto stack = m->i_at( p );
        auto iter = std::find_if( stack.begin(), stack.end(),
        [ammo]( const item & i ) {
            return i.typeId() == ammo;
        } );
        if( iter != stack.end() ) {
            item furn_item( itt, -1, iter->charges );
            if( !filter( furn_item ) ) {
                return;
            }
            // The const itemructor limits the charges to the (type specific) maximum.
            // Setting it separately circumvents that it is synchronized with the code that creates
            // the pseudo item (and fills its charges) in inventory.cpp
            furn_item.charges = iter->charges;
            if( furn_item.use_charges( type, quantity, ret, p ) ) {
                stack.erase( iter );
            } else {
                iter->charges = furn_item.charges;
            }
        }
    }
}

std::list<item> map::use_charges( const tripoint &origin, const int range,
                                  const itype_id type, long &quantity, const std::function<bool( const item & )> &filter )
{
    std::list<item> ret;

    // We prefer infinite map sources where available, so search for those
    // first
    for( const tripoint &p : closest_tripoints_first( range, origin ) ) {
        // can not reach this -> can not access its contents
        if( origin != p && !clear_path( origin, p, range, 1, 100 ) ) {
            continue;
        }

        // Handle infinite map sources.
        item water = water_from( p );
        if( water.typeId() == type ) {
            ret.push_back( water );
            quantity = 0;
            return ret;
        }
    }

    for( const tripoint &p : closest_tripoints_first( range, origin ) ) {
        // can not reach this -> can not access its contents
        if( origin != p && !clear_path( origin, p, range, 1, 100 ) ) {
            continue;
        }

        if( has_furn( p ) ) {
            use_charges_from_furn( furn( p ).obj(), type, quantity, this, p, ret, filter );
            if( quantity <= 0 ) {
                return ret;
            }
        }

        if( accessible_items( p ) ) {
            std::list<item> tmp = use_charges_from_stack( i_at( p ), type, quantity, p, filter );
            ret.splice( ret.end(), tmp );
            if( quantity <= 0 ) {
                return ret;
            }
        }

        const optional_vpart_position vp = veh_at( p );
        if( !vp ) {
            continue;
        }

        const cata::optional<vpart_reference> kpart = vp.part_with_feature( "FAUCET", true );
        const cata::optional<vpart_reference> weldpart = vp.part_with_feature( "WELDRIG", true );
        const cata::optional<vpart_reference> craftpart = vp.part_with_feature( "CRAFTRIG", true );
        const cata::optional<vpart_reference> forgepart = vp.part_with_feature( "FORGE", true );
        const cata::optional<vpart_reference> kilnpart = vp.part_with_feature( "KILN", true );
        const cata::optional<vpart_reference> chempart = vp.part_with_feature( "CHEMLAB", true );
        const cata::optional<vpart_reference> cargo = vp.part_with_feature( "CARGO", true );

        if( kpart ) { // we have a faucet, now to see what to drain
            itype_id ftype = "null";

            // Special case hotplates which draw battery power
            if( type == "hotplate" ) {
                ftype = "battery";
            } else {
                ftype = type;
            }

            item tmp( type, 0 ); // TODO: add a sane birthday arg
            tmp.charges = kpart->vehicle().drain( ftype, quantity );
            // TODO: Handle water poison when crafting starts respecting it
            quantity -= tmp.charges;
            ret.push_back( tmp );

            if( quantity == 0 ) {
                return ret;
            }
        }

        if( weldpart ) { // we have a weldrig, now to see what to drain
            itype_id ftype = "null";

            if( type == "welder" ) {
                ftype = "battery";
            } else if( type == "soldering_iron" ) {
                ftype = "battery";
            }

            item tmp( type, 0 ); // TODO: add a sane birthday arg
            tmp.charges = weldpart->vehicle().drain( ftype, quantity );
            quantity -= tmp.charges;
            ret.push_back( tmp );

            if( quantity == 0 ) {
                return ret;
            }
        }

        if( craftpart ) { // we have a craftrig, now to see what to drain
            itype_id ftype = "null";

            if( type == "press" ) {
                ftype = "battery";
            } else if( type == "vac_sealer" ) {
                ftype = "battery";
            } else if( type == "dehydrator" ) {
                ftype = "battery";
            } else if( type == "food_processor" ) {
                ftype = "battery";
            }

            item tmp( type, 0 ); // TODO: add a sane birthday arg
            tmp.charges = craftpart->vehicle().drain( ftype, quantity );
            quantity -= tmp.charges;
            ret.push_back( tmp );

            if( quantity == 0 ) {
                return ret;
            }
        }

        if( forgepart ) { // we have a veh_forge, now to see what to drain
            itype_id ftype = "null";

            if( type == "forge" ) {
                ftype = "battery";
            }

            item tmp( type, 0 ); // TODO: add a sane birthday arg
            tmp.charges = forgepart->vehicle().drain( ftype, quantity );
            quantity -= tmp.charges;
            ret.push_back( tmp );

            if( quantity == 0 ) {
                return ret;
            }
        }

        if( kilnpart ) { // we have a veh_kiln, now to see what to drain
            itype_id ftype = "null";

            if( type == "kiln" ) {
                ftype = "battery";
            }

            item tmp( type, 0 ); // TODO: add a sane birthday arg
            tmp.charges = kilnpart->vehicle().drain( ftype, quantity );
            quantity -= tmp.charges;
            ret.push_back( tmp );

            if( quantity == 0 ) {
                return ret;
            }
        }

        if( chempart ) { // we have a chem_lab, now to see what to drain
            itype_id ftype = "null";

            if( type == "chemistry_set" ) {
                ftype = "battery";
            } else if( type == "hotplate" ) {
                ftype = "battery";
            }

            item tmp( type, 0 ); // TODO: add a sane birthday arg
            tmp.charges = chempart->vehicle().drain( ftype, quantity );
            quantity -= tmp.charges;
            ret.push_back( tmp );

            if( quantity == 0 ) {
                return ret;
            }
        }

        if( cargo ) {
            std::list<item> tmp =
                use_charges_from_stack( cargo->vehicle().get_items( cargo->part_index() ), type, quantity, p,
                                        filter );
            ret.splice( ret.end(), tmp );
            if( quantity <= 0 ) {
                return ret;
            }
        }
    }

    return ret;
}

std::list<std::pair<tripoint, item *> > map::get_rc_items( int x, int y, int z )
{
    std::list<std::pair<tripoint, item *> > rc_pairs;
    tripoint pos;
    ( void )z;
    pos.z = abs_sub.z;
    for( pos.x = 0; pos.x < MAPSIZE_X; pos.x++ ) {
        if( x != -1 && x != pos.x ) {
            continue;
        }
        for( pos.y = 0; pos.y < MAPSIZE_Y; pos.y++ ) {
            if( y != -1 && y != pos.y ) {
                continue;
            }
            auto items = i_at( pos );
            for( auto &elem : items ) {
                if( elem.has_flag( "RADIO_ACTIVATION" ) || elem.has_flag( "RADIO_CONTAINER" ) ) {
                    rc_pairs.push_back( std::make_pair( pos, &( elem ) ) );
                }
            }
        }
    }

    return rc_pairs;
}

static bool trigger_radio_item( item_stack &items, std::list<item>::iterator &n,
                                const tripoint &pos, const std::string &signal,
                                const int, const float )
{
    bool trigger_item = false;
    if( n->has_flag( "RADIO_ACTIVATION" ) && n->has_flag( signal ) ) {
        sounds::sound( pos, 6, sounds::sound_t::alarm, _( "beep." ) );
        if( n->has_flag( "RADIO_INVOKE_PROC" ) ) {
            // Invoke twice: first to transform, then later to proc
            // Can't use process_item here - invalidates our iterator
            n->process( nullptr, pos, true );
        }
        if( n->has_flag( "BOMB" ) ) {
            // Set charges to 0 to ensure it detonates now
            n->charges = 0;
            n->item_counter = 0;
        }
        trigger_item = true;
    } else if( n->has_flag( "RADIO_CONTAINER" ) && !n->contents.empty() ) {
        auto it = std::find_if( n->contents.begin(), n->contents.end(), [ &signal ]( const item & c ) {
            return c.has_flag( signal );
        } );
        if( it != n->contents.end() ) {
            n->convert( it->typeId() );
            if( n->has_flag( "RADIO_INVOKE_PROC" ) ) {
                n->process( nullptr, pos, true );
            }

            // Clear possible mods to prevent unnecessary pop-ups.
            n->contents.clear();

            n->charges = 0;
            trigger_item = true;
        }
    }
    if( trigger_item ) {
        return process_item( items, n, pos, true, 0, 1 );
    }
    return false;
}

void map::trigger_rc_items( const std::string &signal )
{
    process_items( false, trigger_radio_item, signal );
}

item *map::item_from( const tripoint &pos, size_t index )
{
    auto items = i_at( pos );

    if( index >= items.size() ) {
        return nullptr;
    } else {
        return &items[index];
    }
}

item *map::item_from( vehicle *veh, int cargo_part, size_t index )
{
    auto items = veh->get_items( cargo_part );

    if( index >= items.size() ) {
        return nullptr;
    } else {
        return &items[index];
    }
}

const trap &map::tr_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return tr_null.obj();
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    if( current_submap->get_ter( l ).obj().trap != tr_null ) {
        return current_submap->get_ter( l ).obj().trap.obj();
    }

    return current_submap->get_trap( l ).obj();
}

void map::trap_set( const tripoint &p, const trap_id type )
{
    if( !inbounds( p ) ) {
        return;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );
    const ter_t &ter = current_submap->get_ter( l ).obj();
    if( ter.trap != tr_null ) {
        debugmsg( "set trap %s on top of terrain %s which already has a builit-in trap",
                  type.obj().name().c_str(), ter.name().c_str() );
        return;
    }

    // If there was already a trap here, remove it.
    if( current_submap->get_trap( l ) != tr_null ) {
        remove_trap( p );
    }

    current_submap->set_trap( l, type );
    if( type != tr_null ) {
        traplocs[type].push_back( p );
    }
}

void map::disarm_trap( const tripoint &p )
{
    const trap &tr = tr_at( p );
    if( tr.is_null() ) {
        debugmsg( "Tried to disarm a trap where there was none (%d %d %d)", p.x, p.y, p.z );
        return;
    }

    const int tSkillLevel = g->u.get_skill_level( skill_traps );
    const int diff = tr.get_difficulty();
    int roll = rng( tSkillLevel, 4 * tSkillLevel );

    // Some traps are not actual traps. Skip the rolls, different message and give the option to grab it right away.
    if( tr.get_avoidance() ==  0 && tr.get_difficulty() == 0 ) {
        add_msg( _( "You take down the %s." ), tr.name().c_str() );
        tr.on_disarmed( *this, p );
        return;
    }

    ///\EFFECT_PER increases chance of disarming trap

    ///\EFFECT_DEX increases chance of disarming trap

    ///\EFFECT_TRAPS increases chance of disarming trap
    while( ( rng( 5, 20 ) < g->u.per_cur || rng( 1, 20 ) < g->u.dex_cur ) && roll < 50 ) {
        roll++;
    }
    if( roll >= diff ) {
        add_msg( _( "You disarm the trap!" ) );
        tr.on_disarmed( *this, p );
        if( diff > 1.25 * tSkillLevel ) { // failure might have set off trap
            g->u.practice( skill_traps, 1.5 * ( diff - tSkillLevel ) );
        }
    } else if( roll >= diff * .8 ) {
        add_msg( _( "You fail to disarm the trap." ) );
        if( diff > 1.25 * tSkillLevel ) {
            g->u.practice( skill_traps, 1.5 * ( diff - tSkillLevel ) );
        }
    } else {
        add_msg( m_bad, _( "You fail to disarm the trap, and you set it off!" ) );
        tr.trigger( p, &g->u );
        if( diff - roll <= 6 ) {
            // Give xp for failing, but not if we failed terribly (in which
            // case the trap may not be disarmable).
            g->u.practice( skill_traps, 2 * diff );
        }
    }
}

void map::remove_trap( const tripoint &p )
{
    if( !inbounds( p ) ) {
        return;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    trap_id tid = current_submap->get_trap( l );
    if( tid != tr_null ) {
        if( g != nullptr && this == &g->m ) {
            g->u.add_known_trap( p, tr_null.obj() );
        }

        current_submap->set_trap( l, tr_null );
        auto &traps = traplocs[tid];
        const auto iter = std::find( traps.begin(), traps.end(), p );
        if( iter != traps.end() ) {
            traps.erase( iter );
        }
    }
}
/*
 * Get wrapper for all fields at xyz
 */
const field &map::field_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        nulfield = field();
        return nulfield;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->fld[l.x][l.y];
}

/*
 * As above, except not const
 */
field &map::field_at( const tripoint &p )
{
    if( !inbounds( p ) ) {
        nulfield = field();
        return nulfield;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->fld[l.x][l.y];
}

time_duration map::adjust_field_age( const tripoint &p, const field_id type,
                                     const time_duration &offset )
{
    return set_field_age( p, type, offset, true );
}

int map::adjust_field_strength( const tripoint &p, const field_id type, const int offset )
{
    return set_field_strength( p, type, offset, true );
}

time_duration map::set_field_age( const tripoint &p, const field_id type, const time_duration &age,
                                  const bool isoffset )
{
    if( field_entry *const field_ptr = get_field( p, type ) ) {
        return field_ptr->setFieldAge( ( isoffset ? field_ptr->getFieldAge() : 0_turns ) + age );
    }
    return -1_turns;
}

/*
 * set strength of field type at point, creating if not present, removing if strength is 0
 * returns resulting strength, or 0 for not present
 */
int map::set_field_strength( const tripoint &p, const field_id type, const int str, bool isoffset )
{
    field_entry *field_ptr = get_field( p, type );
    if( field_ptr != nullptr ) {
        int adj = ( isoffset ? field_ptr->getFieldDensity() : 0 ) + str;
        if( adj > 0 ) {
            field_ptr->setFieldDensity( adj );
            return adj;
        } else {
            remove_field( p, type );
            return 0;
        }
    } else if( 0 + str > 0 ) {
        return add_field( p, type, str ) ? str : 0;
    }

    return 0;
}

time_duration map::get_field_age( const tripoint &p, const field_id type ) const
{
    auto field_ptr = field_at( p ).findField( type );
    return field_ptr == nullptr ? -1_turns : field_ptr->getFieldAge();
}

int map::get_field_strength( const tripoint &p, const field_id type ) const
{
    auto field_ptr = field_at( p ).findField( type );
    return ( field_ptr == nullptr ? 0 : field_ptr->getFieldDensity() );
}

field_entry *map::get_field( const tripoint &p, const field_id type )
{
    if( !inbounds( p ) ) {
        return nullptr;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    return current_submap->fld[l.x][l.y].findField( type );
}

bool map::add_field( const tripoint &p, const field_id type, int density, const time_duration &age )
{
    if( !inbounds( p ) ) {
        return false;
    }

    density = std::min( density, MAX_FIELD_DENSITY );
    if( density <= 0 ) {
        return false;
    }

    if( type == fd_null ) {
        return false;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );
    current_submap->is_uniform = false;

    if( current_submap->fld[l.x][l.y].addField( type, density, age ) ) {
        //Only adding it to the count if it doesn't exist.
        current_submap->field_count++;
    }

    if( g != nullptr && this == &g->m && p == g->u.pos() ) {
        creature_in_field( g->u ); //Hit the player with the field if it spawned on top of them.
    }

    // Dirty the transparency cache now that field processing doesn't always do it
    // TODO: Make it skip transparent fields
    set_transparency_cache_dirty( p.z );

    const field_t &ft = fieldlist[type];
    if( field_type_dangerous( type ) ) {
        set_pathfinding_cache_dirty( p.z );
    }

    // Ensure blood type fields don't hang in the air
    if( zlevels && ft.accelerated_decay ) {
        support_dirty( p );
    }

    return true;
}

void map::remove_field( const tripoint &p, const field_id field_to_remove )
{
    if( !inbounds( p ) ) {
        return;
    }

    point l;
    submap *const current_submap = get_submap_at( p, l );

    if( current_submap->fld[l.x][l.y].removeField( field_to_remove ) ) {
        // Only adjust the count if the field actually existed.
        current_submap->field_count--;
        const auto &fdata = fieldlist[ field_to_remove ];
        for( bool i : fdata.transparent ) {
            if( !i ) {
                set_transparency_cache_dirty( p.z );
                break;
            }
        }

        for( bool danger : fdata.dangerous ) {
            if( danger ) {
                set_pathfinding_cache_dirty( p.z );
                break;
            }
        }
    }
}

void map::add_splatter( const field_id type, const tripoint &where, int intensity )
{
    if( intensity <= 0 ) {
        return;
    }
    if( type == fd_blood || type == fd_gibs_flesh ) { // giblets are also good for painting
        if( const optional_vpart_position vp = veh_at( where ) ) {
            vehicle *const veh = &vp->vehicle();
            // Might be -1 if all the vehicle's parts at where are marked for removal
            const int part = veh->part_displayed_at( vp->mount() );
            if( part != -1 ) {
                veh->parts[part].blood += 200 * std::min( intensity, 3 ) / 3;
                return;
            }
        }
    }
    adjust_field_strength( where, type, intensity );
}

void map::add_splatter_trail( const field_id type, const tripoint &from, const tripoint &to )
{
    if( type == fd_null ) {
        return;
    }
    const auto trail = line_to( from, to );
    int remainder = trail.size();
    for( const auto &elem : trail ) {
        add_splatter( type, elem );
        remainder--;
        if( impassable( elem ) ) { // Blood splatters stop at walls.
            add_splatter( type, elem, remainder );
            return;
        }
    }
}

void map::add_splash( const field_id type, const tripoint &center, int radius, int density )
{
    if( type == fd_null ) {
        return;
    }
    // TODO: use Bresenham here and take obstacles into account
    for( const tripoint &pnt : points_in_radius( center, radius ) ) {
        if( trig_dist( pnt, center ) <= radius && !one_in( density ) ) {
            add_splatter( type, pnt );
        }
    }
}

computer *map::computer_at( const tripoint &p )
{
    if( !inbounds( p ) ) {
        return nullptr;
    }

    return get_submap_at( p )->comp.get();
}

void map::remove_submap_camp( const tripoint &p )
{
    basecamp camp;
    get_submap_at( p )->camp = camp;
}

basecamp map::hoist_submap_camp( const tripoint &p )
{
    basecamp camp = get_submap_at( p )->camp;
    return camp;
}

void map::add_camp( const tripoint &p, const std::string &name )
{
    tripoint omt_pos = ms_to_omt_copy( g->m.getabs( p ) );
    basecamp temp_camp = basecamp( name, omt_pos );
    overmap_buffer.add_camp( temp_camp );
    g->u.camps.insert( omt_pos );
    g->validate_camps();
}

void map::update_visibility_cache( const int zlev )
{
    visibility_variables_cache.variables_set = true; // Not used yet
    visibility_variables_cache.g_light_level = static_cast<int>( g->light_level( zlev ) );
    visibility_variables_cache.vision_threshold = g->u.get_vision_threshold(
                get_cache_ref( g->u.posz() ).lm[g->u.posx()][g->u.posy()].max() );

    visibility_variables_cache.u_clairvoyance = g->u.clairvoyance();
    visibility_variables_cache.u_sight_impaired = g->u.sight_impaired();
    visibility_variables_cache.u_is_boomered = g->u.has_effect( effect_boomered );

    int sm_squares_seen[MAPSIZE][MAPSIZE];
    std::memset( sm_squares_seen, 0, sizeof( sm_squares_seen ) );

    auto &visibility_cache = get_cache( zlev ).visibility_cache;

    tripoint p;
    p.z = zlev;
    int &x = p.x;
    int &y = p.y;
    for( x = 0; x < MAPSIZE_X; x++ ) {
        for( y = 0; y < MAPSIZE_Y; y++ ) {
            lit_level ll = apparent_light_at( p, visibility_variables_cache );
            visibility_cache[x][y] = ll;
            sm_squares_seen[ x / SEEX ][ y / SEEY ] += ( ll == LL_BRIGHT || ll == LL_LIT );
        }
    }

    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            if( sm_squares_seen[gridx][gridy] > 36 ) { // 25% of the submap is visible
                const tripoint sm( gridx, gridy, 0 );
                const auto abs_sm = map::abs_sub + sm;
                const auto abs_omt = sm_to_omt_copy( abs_sm );
                overmap_buffer.set_seen( abs_omt.x, abs_omt.y, abs_omt.z, true );
            }
        }
    }
}

const visibility_variables &map::get_visibility_variables_cache() const
{
    return visibility_variables_cache;
}

visibility_type map::get_visibility( const lit_level ll, const visibility_variables &cache ) const
{
    switch( ll ) {
        case LL_DARK: // can't see this square at all
            if( cache.u_is_boomered ) {
                return VIS_BOOMER_DARK;
            } else {
                return VIS_DARK;
            }
        case LL_BRIGHT_ONLY: // can only tell that this square is bright
            if( cache.u_is_boomered ) {
                return VIS_BOOMER;
            } else {
                return VIS_LIT;
            }
        case LL_LOW: // low light, square visible in monochrome
        case LL_LIT: // normal light
        case LL_BRIGHT: // bright light
            return VIS_CLEAR;
        case LL_BLANK:
        case LL_MEMORIZED:
            return VIS_HIDDEN;
    }
    return VIS_HIDDEN;
}

bool map::apply_vision_effects( const catacurses::window &w, const visibility_type vis ) const
{
    long symbol = ' ';
    nc_color color = c_black;

    switch( vis ) {
        case VIS_CLEAR:
            // Drew the tile, so bail out now.
            return false;
        case VIS_LIT: // can only tell that this square is bright
            symbol = '#';
            color = c_light_gray;
            break;
        case VIS_BOOMER:
            symbol = '#';
            color = c_pink;
            break;
        case VIS_BOOMER_DARK:
            symbol = '#';
            color = c_magenta;
            break;
        case VIS_DARK: // can't see this square at all
        case VIS_HIDDEN:
            symbol = ' ';
            color = c_black;
            break;
    }
    wputch( w, color, symbol );
    return true;
}

bool map::draw_maptile_from_memory( const catacurses::window &w, const tripoint &p,
                                    const tripoint &view_center, bool move_cursor ) const
{
    long sym = g->u.get_memorized_symbol( getabs( p ) );
    if( sym == 0 ) {
        return false;
    }
    if( move_cursor ) {
        const int k = p.x + getmaxx( w ) / 2 - view_center.x;
        const int j = p.y + getmaxy( w ) / 2 - view_center.y;

        mvwputch( w, j, k, c_brown, sym );
    } else {
        wputch( w, c_brown, sym );
    }
    return true;
}

void map::draw( const catacurses::window &w, const tripoint &center )
{
    // We only need to draw anything if we're not in tiles mode.
    if( is_draw_tiles_mode() ) {
        return;
    }

    g->reset_light_level();

    update_visibility_cache( center.z );
    const visibility_variables &cache = g->m.get_visibility_variables_cache();

    const auto &visibility_cache = get_cache_ref( center.z ).visibility_cache;

    // X and y are in map coordinates, but might be out of range of the map.
    // When they are out of range, we just draw '#'s.
    tripoint p;
    p.z = center.z;
    int &x = p.x;
    int &y = p.y;
    const bool do_map_memory = g->u.should_show_map_memory();
    for( y = center.y - getmaxy( w ) / 2; y <= center.y + getmaxy( w ) / 2; y++ ) {
        if( y - center.y + getmaxy( w ) / 2 >= getmaxy( w ) ) {
            continue;
        }

        wmove( w, y - center.y + getmaxy( w ) / 2, 0 );

        const int maxxrender = center.x - getmaxx( w ) / 2 + getmaxx( w );
        x = center.x - getmaxx( w ) / 2;
        if( y < 0 || y >= MAPSIZE_Y ) {
            for( ; x < maxxrender; x++ ) {
                if( !do_map_memory || !draw_maptile_from_memory( w, p, center, false ) ) {
                    wputch( w, c_black, ' ' );
                }
            }
            continue;
        }

        while( x < 0 ) {
            if( !do_map_memory || !draw_maptile_from_memory( w, p, center, false ) ) {
                wputch( w, c_black, ' ' );
            }
            x++;
        }

        point l;
        const int maxx = std::min( MAPSIZE_X, maxxrender );
        while( x < maxx ) {
            submap *cur_submap = get_submap_at( p, l );
            submap *sm_below = p.z > -OVERMAP_DEPTH ?
                               get_submap_at( {p.x, p.y, p.z - 1}, l ) : cur_submap;
            while( l.x < SEEX && x < maxx )  {
                const lit_level lighting = visibility_cache[x][y];
                const visibility_type vis = get_visibility( lighting, cache );
                if( !apply_vision_effects( w, vis ) ) {
                    const maptile curr_maptile = maptile( cur_submap, l );
                    const bool just_this_zlevel =
                        draw_maptile( w, g->u, p, curr_maptile,
                                      false, true, center,
                                      lighting == LL_LOW, lighting == LL_BRIGHT, true );
                    if( !just_this_zlevel ) {
                        p.z--;
                        const maptile tile_below = maptile( sm_below, l );
                        draw_from_above( w, g->u, p, tile_below, false, center,
                                         lighting == LL_LOW, lighting == LL_BRIGHT, false );
                        p.z++;
                    }
                } else if( do_map_memory && ( vis == VIS_HIDDEN || vis == VIS_DARK ) ) {
                    draw_maptile_from_memory( w, p, center );
                }

                l.x++;
                x++;
            }
        }

        while( x < maxxrender ) {
            if( !do_map_memory || !draw_maptile_from_memory( w, p, center, false ) ) {
                wputch( w, c_black, ' ' );
            }
            x++;
        }
    }
}

void map::drawsq( const catacurses::window &w, player &u, const tripoint &p,
                  const bool invert, const bool show_items ) const
{
    drawsq( w, u, p, invert, show_items, u.pos() + u.view_offset, false, false, false );
}

void map::drawsq( const catacurses::window &w, player &u, const tripoint &p, const bool invert_arg,
                  const bool show_items_arg, const tripoint &view_center,
                  const bool low_light, const bool bright_light, const bool inorder ) const
{
    // We only need to draw anything if we're not in tiles mode.
    if( is_draw_tiles_mode() ) {
        return;
    }

    if( !inbounds( p ) ) {
        return;
    }

    const maptile tile = maptile_at( p );
    const bool done = draw_maptile( w, u, p, tile, invert_arg, show_items_arg,
                                    view_center, low_light, bright_light, inorder );
    if( !done ) {
        tripoint below( p.x, p.y, p.z - 1 );
        const maptile tile_below = maptile_at( below );
        draw_from_above( w, u, below, tile_below,
                         invert_arg, view_center,
                         low_light, bright_light, false );
    }
}

// a check to see if the lower floor needs to be rendered in tiles
bool map::need_draw_lower_floor( const tripoint &p )
{
    return !( !zlevels || p.z <= -OVERMAP_DEPTH || !ter( p ).obj().has_flag( TFLAG_NO_FLOOR ) );
}

bool map::draw_maptile( const catacurses::window &w, const player &u, const tripoint &p,
                        const maptile &curr_maptile,
                        bool invert, bool show_items,
                        const tripoint &view_center,
                        const bool low_light, const bool bright_light, const bool inorder ) const
{
    nc_color tercol;
    const ter_t &curr_ter = curr_maptile.get_ter_t();
    const furn_t &curr_furn = curr_maptile.get_furn_t();
    const trap &curr_trap = curr_maptile.get_trap().obj();
    const field &curr_field = curr_maptile.get_field();
    long sym;
    bool hi = false;
    bool graf = false;
    bool draw_item_sym = false;

    long terrain_sym;
    if( curr_ter.has_flag( TFLAG_AUTO_WALL_SYMBOL ) ) {
        terrain_sym = determine_wall_corner( p );
    } else {
        terrain_sym = curr_ter.symbol();
    }

    if( curr_furn.id ) {
        sym = curr_furn.symbol();
        tercol = curr_furn.color();
    } else {
        sym = terrain_sym;
        tercol = curr_ter.color();
    }
    if( curr_ter.has_flag( TFLAG_SWIMMABLE ) && curr_ter.has_flag( TFLAG_DEEP_WATER ) &&
        !u.is_underwater() ) {
        show_items = false; // Can only see underwater items if WE are underwater
    }
    // If there's a trap here, and we have sufficient perception, draw that instead
    if( curr_trap.can_see( p, g->u ) ) {
        tercol = curr_trap.color;
        if( curr_trap.sym == '%' ) {
            switch( rng( 1, 5 ) ) {
                case 1:
                    sym = '*';
                    break;
                case 2:
                    sym = '0';
                    break;
                case 3:
                    sym = '8';
                    break;
                case 4:
                    sym = '&';
                    break;
                case 5:
                    sym = '+';
                    break;
            }
        } else {
            sym = curr_trap.sym;
        }
    }
    if( curr_field.fieldCount() > 0 ) {
        const field_id &fid = curr_field.fieldSymbol();
        const field_entry *fe = curr_field.findField( fid );
        const field_t &f = fieldlist[fid];
        if( f.sym == '&' || fe == nullptr ) {
            // Do nothing, a '&' indicates invisible fields.
        } else if( f.sym == '*' ) {
            // A random symbol.
            switch( rng( 1, 5 ) ) {
                case 1:
                    sym = '*';
                    break;
                case 2:
                    sym = '0';
                    break;
                case 3:
                    sym = '8';
                    break;
                case 4:
                    sym = '&';
                    break;
                case 5:
                    sym = '+';
                    break;
            }
        } else {
            // A field symbol '%' indicates the field should not hide
            // items/terrain. When the symbol is not '%' it will
            // hide items (the color is still inverted if there are items,
            // but the tile symbol is not changed).
            // draw_item_sym indicates that the item symbol should be used
            // even if sym is not '.'.
            // As we don't know at this stage if there are any items
            // (that are visible to the player!), we always set the symbol.
            // If there are items and the field does not hide them,
            // the code handling items will override it.
            draw_item_sym = ( f.sym == '%' );
            // If field priority is > 1, and the field is set to hide items,
            //draw the field as it obscures what's under it.
            if( ( f.sym != '%' && f.priority > 1 ) || ( f.sym != '%' && sym == '.' ) )  {
                // default terrain '.' and
                // non-default field symbol -> field symbol overrides terrain
                sym = f.sym;
            }
            tercol = fe->color();
        }
    }

    // TODO: change the local variable sym to std::string and use it instead of this hack.
    // Currently this are different variables because terrain/... uses long as symbol type and
    // item now use string. Ideally they should all be strings.
    std::string item_sym;

    // If there are items here, draw those instead
    if( show_items && curr_maptile.get_item_count() > 0 && sees_some_items( p, g->u ) ) {
        // if there's furniture/terrain/trap/fields (sym!='.')
        // and we should not override it, then only highlight the square
        if( sym != '.' && sym != '%' && !draw_item_sym ) {
            hi = true;
        } else {
            // otherwise override with the symbol of the last item
            item_sym = curr_maptile.get_uppermost_item().symbol();
            if( !draw_item_sym ) {
                tercol = curr_maptile.get_uppermost_item().color();
            }
            if( curr_maptile.get_item_count() > 1 ) {
                invert = !invert;
            }
        }
    }

    long memory_sym = sym;
    int veh_part = 0;
    const vehicle *veh = veh_at_internal( p, veh_part );
    if( veh != nullptr ) {
        sym = special_symbol( veh->face.dir_symbol( veh->part_sym( veh_part ) ) );
        tercol = veh->part_color( veh_part );
        item_sym.clear(); // clear the item symbol so `sym` is used instead.

        if( !veh->forward_velocity() && !veh->player_in_control( g->u ) ) {
            memory_sym = sym;
        }
    }

    if( !check_and_set_seen_cache( p ) ) {
        g->u.memorize_symbol( getabs( p ), memory_sym );
    }

    // If there's graffiti here, change background color
    if( curr_maptile.has_graffiti() ) {
        graf = true;
    }

    const auto u_vision = u.get_vision_modes();
    if( u_vision[BOOMERED] ) {
        tercol = c_magenta;
    } else if( u_vision[NV_GOGGLES] ) {
        tercol = ( bright_light ) ? c_white : c_light_green;
    } else if( low_light ) {
        tercol = c_dark_gray;
    } else if( u_vision[DARKNESS] ) {
        tercol = c_dark_gray;
    }

    if( invert ) {
        tercol = invert_color( tercol );
    } else if( hi ) {
        tercol = hilite( tercol );
    } else if( graf ) {
        tercol = red_background( tercol );
    }

    if( inorder ) {
        // Rastering the whole map, take advantage of automatically moving the cursor.
        if( item_sym.empty() ) {
            wputch( w, tercol, sym );
        } else {
            wprintz( w, tercol, item_sym );
        }
    } else {
        // Otherwise move the cursor before drawing.
        const int k = p.x + getmaxx( w ) / 2 - view_center.x;
        const int j = p.y + getmaxy( w ) / 2 - view_center.y;
        if( item_sym.empty() ) {
            mvwputch( w, j, k, tercol, sym );
        } else {
            mvwprintz( w, j, k, tercol, item_sym );
        }
    }

    return !zlevels || sym != ' ' || !item_sym.empty() || p.z <= -OVERMAP_DEPTH ||
           !curr_ter.has_flag( TFLAG_NO_FLOOR );
}

void map::draw_from_above( const catacurses::window &w, const player &u, const tripoint &p,
                           const maptile &curr_tile,
                           const bool invert,
                           const tripoint &view_center,
                           bool low_light, bool bright_light, bool inorder ) const
{
    static const long AUTO_WALL_PLACEHOLDER = 2; // this should never appear as a real symbol!

    nc_color tercol = c_dark_gray;
    long sym = ' ';

    const ter_t &curr_ter = curr_tile.get_ter_t();
    const furn_t &curr_furn = curr_tile.get_furn_t();
    int part_below;
    const vehicle *veh;
    if( curr_furn.has_flag( TFLAG_SEEN_FROM_ABOVE ) ) {
        sym = curr_furn.symbol();
        tercol = curr_furn.color();
    } else if( curr_furn.movecost < 0 ) {
        sym = '.';
        tercol = curr_furn.color();
    } else if( ( veh = veh_at_internal( p, part_below ) ) != nullptr ) {
        const int roof = veh->roof_at_part( part_below );
        const int displayed_part = roof >= 0 ? roof : part_below;
        sym = special_symbol( veh->face.dir_symbol( veh->part_sym( displayed_part, true ) ) );
        tercol = ( roof >= 0 ||
                   vpart_position( const_cast<vehicle &>( *veh ),
                                   part_below ).obstacle_at_part() ) ? c_light_gray : c_light_gray_cyan;
    } else if( curr_ter.has_flag( TFLAG_SEEN_FROM_ABOVE ) ) {
        if( curr_ter.has_flag( TFLAG_AUTO_WALL_SYMBOL ) ) {
            sym = AUTO_WALL_PLACEHOLDER;
        } else if( curr_ter.has_flag( TFLAG_RAMP ) ) {
            sym = '>';
        } else {
            sym = curr_ter.symbol();
        }
        tercol = curr_ter.color();
    } else if( curr_ter.movecost == 0 ) {
        sym = '.';
        tercol = curr_ter.color();
    } else if( !curr_ter.has_flag( TFLAG_NO_FLOOR ) ) {
        sym = '.';
        if( curr_ter.color() != c_cyan ) {
            // Need a special case here, it doesn't cyanize well
            tercol = cyan_background( curr_ter.color() );
        } else {
            tercol = c_black_cyan;
        }
    } else {
        sym = curr_ter.symbol();
        tercol = curr_ter.color();
    }

    if( sym == AUTO_WALL_PLACEHOLDER ) {
        sym = determine_wall_corner( p );
    }

    const auto u_vision = u.get_vision_modes();
    if( u_vision[BOOMERED] ) {
        tercol = c_magenta;
    } else if( u_vision[NV_GOGGLES] ) {
        tercol = ( bright_light ) ? c_white : c_light_green;
    } else if( low_light ) {
        tercol = c_dark_gray;
    } else if( u_vision[DARKNESS] ) {
        tercol = c_dark_gray;
    }

    if( invert ) {
        tercol = invert_color( tercol );
    }

    if( inorder ) {
        wputch( w, tercol, sym );
    } else {
        const int k = p.x + getmaxx( w ) / 2 - view_center.x;
        const int j = p.y + getmaxy( w ) / 2 - view_center.y;
        mvwputch( w, j, k, tercol, sym );
    }
}

bool map::sees( const tripoint &F, const tripoint &T, const int range ) const
{
    int dummy = 0;
    return sees( F, T, range, dummy );
}

/**
 * This one is internal-only, we don't want to expose the slope tweaking ickiness outside the map class.
 **/
bool map::sees( const tripoint &F, const tripoint &T, const int range, int &bresenham_slope ) const
{
    if( ( range >= 0 && range < rl_dist( F, T ) ) ||
        !inbounds( T ) ) {
        bresenham_slope = 0;
        return false; // Out of range!
    }
    bool visible = true;

    // Ugly `if` for now
    if( !fov_3d || F.z == T.z ) {
        bresenham( F.x, F.y, T.x, T.y, bresenham_slope,
        [this, &visible, &T]( const point & new_point ) {
            // Exit before checking the last square, it's still visible even if opaque.
            if( new_point.x == T.x && new_point.y == T.y ) {
                return false;
            }
            if( !this->trans( tripoint( new_point, T.z ) ) ) {
                visible = false;
                return false;
            }
            return true;
        } );
        return visible;
    }

    tripoint last_point = F;
    bresenham( F, T, bresenham_slope, 0,
    [this, &visible, &T, &last_point]( const tripoint & new_point ) {
        // Exit before checking the last square, it's still visible even if opaque.
        if( new_point == T ) {
            return false;
        }

        // TODO: Allow transparent floors (and cache them!)
        if( new_point.z == last_point.z ) {
            if( !this->trans( new_point ) ) {
                visible = false;
                return false;
            }
        } else {
            const int max_z = std::max( new_point.z, last_point.z );
            if( ( has_floor_or_support( {new_point.x, new_point.y, max_z} ) ||
                  !trans( {new_point.x, new_point.y, last_point.z} ) ) &&
                ( has_floor_or_support( {last_point.x, last_point.y, max_z} ) ||
                  !trans( {last_point.x, last_point.y, new_point.z} ) ) ) {
                visible = false;
                return false;
            }
        }

        last_point = new_point;
        return true;
    } );
    return visible;
}

// This method tries a bunch of initial offsets for the line to try and find a clear one.
// Basically it does, "Find a line from any point in the source that ends up in the target square".
std::vector<tripoint> map::find_clear_path( const tripoint &source,
        const tripoint &destination ) const
{
    // TODO: Push this junk down into the Bresenham method, it's already doing it.
    const int dx = destination.x - source.x;
    const int dy = destination.y - source.y;
    const int ax = std::abs( dx ) * 2;
    const int ay = std::abs( dy ) * 2;
    const int dominant = std::max( ax, ay );
    const int minor = std::min( ax, ay );
    // This seems to be the method for finding the ideal start value for the error value.
    const int ideal_start_offset = minor - ( dominant / 2 );
    const int start_sign = ( ideal_start_offset > 0 ) - ( ideal_start_offset < 0 );
    // Not totally sure of the derivation.
    const int max_start_offset = std::abs( ideal_start_offset ) * 2 + 1;
    for( int horizontal_offset = -1; horizontal_offset <= max_start_offset; ++horizontal_offset ) {
        int candidate_offset = horizontal_offset * start_sign;
        if( sees( source, destination, rl_dist( source, destination ), candidate_offset ) ) {
            return line_to( source, destination, candidate_offset, 0 );
        }
    }
    // If we couldn't find a clear LoS, just return the ideal one.
    return line_to( source, destination, ideal_start_offset, 0 );
}

bool map::clear_path( const tripoint &f, const tripoint &t, const int range,
                      const int cost_min, const int cost_max ) const
{
    // Ugly `if` for now
    if( !fov_3d && f.z != t.z ) {
        return false;
    }

    if( f.z == t.z ) {
        if( ( range >= 0 && range < rl_dist( f.x, f.y, t.x, t.y ) ) ||
            !inbounds( t ) ) {
            return false; // Out of range!
        }
        bool is_clear = true;
        bresenham( f.x, f.y, t.x, t.y, 0,
        [this, &is_clear, cost_min, cost_max, &t]( const point & new_point ) {
            // Exit before checking the last square, it's still reachable even if it is an obstacle.
            if( new_point.x == t.x && new_point.y == t.y ) {
                return false;
            }

            const int cost = this->move_cost( new_point.x, new_point.y );
            if( cost < cost_min || cost > cost_max ) {
                is_clear = false;
                return false;
            }
            return true;
        } );
        return is_clear;
    }

    if( ( range >= 0 && range < rl_dist( f, t ) ) ||
        !inbounds( t ) ) {
        return false; // Out of range!
    }
    bool is_clear = true;
    tripoint last_point = f;
    bresenham( f, t, 0, 0,
    [this, &is_clear, cost_min, cost_max, t, &last_point]( const tripoint & new_point ) {
        // Exit before checking the last square, it's still reachable even if it is an obstacle.
        if( new_point == t ) {
            return false;
        }

        // We have to check a weird case where the move is both vertical and horizontal
        if( new_point.z == last_point.z ) {
            const int cost = move_cost( new_point );
            if( cost < cost_min || cost > cost_max ) {
                is_clear = false;
                return false;
            }
        } else {
            bool this_clear = false;
            const int max_z = std::max( new_point.z, last_point.z );
            if( !has_floor_or_support( {new_point.x, new_point.y, max_z} ) ) {
                const int cost = move_cost( {new_point.x, new_point.y, last_point.z} );
                if( cost > cost_min && cost < cost_max ) {
                    this_clear = true;
                }
            }

            if( !this_clear && has_floor_or_support( {last_point.x, last_point.y, max_z} ) ) {
                const int cost = move_cost( {last_point.x, last_point.y, new_point.z} );
                if( cost > cost_min && cost < cost_max ) {
                    this_clear = true;
                }
            }

            if( !this_clear ) {
                is_clear = false;
                return false;
            }
        }

        last_point = new_point;
        return true;
    } );
    return is_clear;
}

bool map::accessible_items( const tripoint &t ) const
{
    return !has_flag( "SEALED", t ) || has_flag( "LIQUIDCONT", t );
}

std::vector<tripoint> map::get_dir_circle( const tripoint &f, const tripoint &t ) const
{
    std::vector<tripoint> circle;
    circle.resize( 8 );

    // The line below can be crazy expensive - we only take the FIRST point of it
    const std::vector<tripoint> line = line_to( f, t, 0, 0 );
    const std::vector<tripoint> spiral = closest_tripoints_first( 1, f );
    const std::vector<int> pos_index {1, 2, 4, 6, 8, 7, 5, 3};

    //  All possible constellations (closest_points_first goes clockwise)
    //  753  531  312  124  246  468  687  875
    //  8 1  7 2  5 4  3 6  1 8  2 7  4 5  6 3
    //  642  864  786  578  357  135  213  421

    size_t pos_offset = 0;
    for( unsigned int i = 1; i < spiral.size(); i++ ) {
        if( spiral[i] == line[0] ) {
            pos_offset = i - 1;
            break;
        }
    }

    for( unsigned int i = 1; i < spiral.size(); i++ ) {
        if( pos_offset >= pos_index.size() ) {
            pos_offset = 0;
        }

        circle[pos_index[pos_offset++] - 1] = spiral[i];
    }

    return circle;
}

void map::save()
{
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            if( zlevels ) {
                for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
                    saven( gridx, gridy, gridz );
                }
            } else {
                saven( gridx, gridy, abs_sub.z );
            }
        }
    }
}

void map::load( const int wx, const int wy, const int wz, const bool update_vehicle )
{
    for( auto &traps : traplocs ) {
        traps.clear();
    }
    set_abs_sub( wx, wy, wz );
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            loadn( gridx, gridy, update_vehicle );
        }
    }
}

void map::shift_traps( const tripoint &shift )
{
    // Offset needs to have sign opposite to shift direction
    const tripoint offset( -shift.x * SEEX, -shift.y * SEEY, -shift.z );
    for( auto &traps : traplocs ) {
        for( auto iter = traps.begin(); iter != traps.end(); ) {
            tripoint &pos = *iter;
            pos += offset;
            if( inbounds( pos ) ) {
                ++iter;
            } else {
                // Theoretical enhancement: if this is not the last entry of the vector,
                // move the last entry into pos and remove the last entry instead of iter.
                // This would avoid moving all the remaining entries.
                iter = traps.erase( iter );
            }
        }
    }
}

void shift_map_memory_seen_cache(
    std::bitset<MAPSIZE_X *MAPSIZE_Y> &map_memory_seen_cache,
    const int sx, const int sy )
{
    // sx shifts by SEEX rows, sy shifts by SEEX columns.
    int shift_amount = ( sx * SEEX ) + ( sy * MAPSIZE_Y * SEEX );
    if( shift_amount > 0 ) {
        map_memory_seen_cache >>= static_cast<size_t>( shift_amount );
    } else if( shift_amount < 0 ) {
        map_memory_seen_cache <<= static_cast<size_t>( -shift_amount );
    }
    // Shifting in the y direction shifted in 0 values, no no additional clearing is necessary, but
    // a shift in the x direction makes values "wrap" to the next row, and they need to be zeroed.
    if( sx == 0 ) {
        return;
    }
    const size_t x_offset = ( sx > 0 ) ? MAPSIZE_X - SEEX : 0;
    for( size_t y = 0; y < MAPSIZE_X; ++y ) {
        size_t y_offset = y * MAPSIZE_X;
        for( size_t x = 0; x < SEEX; ++x ) {
            map_memory_seen_cache.reset( y_offset + x_offset + x );
        }
    }
}

void map::shift( const int sx, const int sy )
{
    // Special case of 0-shift; refresh the map
    if( sx == 0 && sy == 0 ) {
        return; // Skip this?
    }
    const int absx = get_abs_sub().x;
    const int absy = get_abs_sub().y;
    const int wz = get_abs_sub().z;

    set_abs_sub( absx + sx, absy + sy, wz );

    // if player is in vehicle, (s)he must be shifted with vehicle too
    if( g->u.in_vehicle ) {
        g->u.setx( g->u.posx() - sx * SEEX );
        g->u.sety( g->u.posy() - sy * SEEY );
    }

    shift_traps( tripoint( sx, sy, 0 ) );

    vehicle *remoteveh = g->remoteveh();

    const int zmin = zlevels ? -OVERMAP_DEPTH : wz;
    const int zmax = zlevels ? OVERMAP_HEIGHT : wz;
    for( int gridz = zmin; gridz <= zmax; gridz++ ) {
        for( vehicle *veh : get_cache( gridz ).vehicle_list ) {
            veh->smx += sx;
            veh->smy += sy;
            veh->zones_dirty = true;
        }
    }

    // Shift the map sx submaps to the right and sy submaps down.
    // sx and sy should never be bigger than +/-1.
    // absx and absy are our position in the world, for saving/loading purposes.
    for( int gridz = zmin; gridz <= zmax; gridz++ ) {
        // Clear vehicle list and rebuild after shift
        clear_vehicle_cache( gridz );
        clear_vehicle_list( gridz );
        shift_map_memory_seen_cache( get_cache( gridz ).map_memory_seen_cache, sx, sy );
        if( sx >= 0 ) {
            for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
                if( sy >= 0 ) {
                    for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
                        if( gridx + sx < my_MAPSIZE && gridy + sy < my_MAPSIZE ) {
                            copy_grid( tripoint( gridx, gridy, gridz ),
                                       tripoint( gridx + sx, gridy + sy, gridz ) );
                            update_vehicle_list( get_submap_at_grid( {gridx, gridy, gridz} ), gridz );
                        } else {
                            loadn( gridx, gridy, gridz, true );
                        }
                    }
                } else { // sy < 0; work through it backwards
                    for( int gridy = my_MAPSIZE - 1; gridy >= 0; gridy-- ) {
                        if( gridx + sx < my_MAPSIZE && gridy + sy >= 0 ) {
                            copy_grid( tripoint( gridx, gridy, gridz ),
                                       tripoint( gridx + sx, gridy + sy, gridz ) );
                            update_vehicle_list( get_submap_at_grid( { gridx, gridy, gridz } ), gridz );
                        } else {
                            loadn( gridx, gridy, gridz, true );
                        }
                    }
                }
            }
        } else { // sx < 0; work through it backwards
            for( int gridx = my_MAPSIZE - 1; gridx >= 0; gridx-- ) {
                if( sy >= 0 ) {
                    for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
                        if( gridx + sx >= 0 && gridy + sy < my_MAPSIZE ) {
                            copy_grid( tripoint( gridx, gridy, gridz ),
                                       tripoint( gridx + sx, gridy + sy, gridz ) );
                            update_vehicle_list( get_submap_at_grid( { gridx, gridy, gridz } ), gridz );
                        } else {
                            loadn( gridx, gridy, gridz, true );
                        }
                    }
                } else { // sy < 0; work through it backwards
                    for( int gridy = my_MAPSIZE - 1; gridy >= 0; gridy-- ) {
                        if( gridx + sx >= 0 && gridy + sy >= 0 ) {
                            copy_grid( tripoint( gridx, gridy, gridz ),
                                       tripoint( gridx + sx, gridy + sy, gridz ) );
                            update_vehicle_list( get_submap_at_grid( { gridx, gridy, gridz } ), gridz );
                        } else {
                            loadn( gridx, gridy, gridz, true );
                        }
                    }
                }
            }
        }

        reset_vehicle_cache( gridz );
    }

    g->setremoteveh( remoteveh );

    if( !support_cache_dirty.empty() ) {
        std::set<tripoint> old_cache = std::move( support_cache_dirty );
        support_cache_dirty.clear();
        for( const auto &pt : old_cache ) {
            support_cache_dirty.insert( tripoint( pt.x - sx * SEEX, pt.y - sy * SEEY, pt.z ) );
        }
    }
}

void map::vertical_shift( const int newz )
{
    if( !zlevels ) {
        debugmsg( "Called map::vertical_shift in a non-z-level world" );
        return;
    }

    if( newz < -OVERMAP_DEPTH || newz > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to get z-level %d outside allowed range of %d-%d",
                  newz, -OVERMAP_DEPTH, OVERMAP_HEIGHT );
        return;
    }

    tripoint trp = get_abs_sub();
    set_abs_sub( trp.x, trp.y, newz );

    // TODO: Remove the function when it's safe
    return;
}

// saven saves a single nonant.  worldx and worldy are used for the file
// name and specifies where in the world this nonant is.  gridx and gridy are
// the offset from the top left nonant:
// 0,0 1,0 2,0
// 0,1 1,1 2,1
// 0,2 1,2 2,2
// (worldx,worldy,worldz) denotes the absolute coordinate of the submap
// in grid[0].
void map::saven( const int gridx, const int gridy, const int gridz )
{
    dbg( D_INFO ) << "map::saven(worldx[" << abs_sub.x << "], worldy[" << abs_sub.y << "], worldz[" <<
                  abs_sub.z
                  << "], gridx[" << gridx << "], gridy[" << gridy << "], gridz[" << gridz << "])";
    const int gridn = get_nonant( { gridx, gridy, gridz } );
    submap *submap_to_save = getsubmap( gridn );
    if( submap_to_save == nullptr || submap_to_save->get_ter( point_zero ) == t_null ) {
        // This is a serious error and should be signaled as soon as possible
        debugmsg( "map::saven grid (%d,%d,%d) %s!", gridx, gridy, gridz,
                  submap_to_save == nullptr ? "null" : "uninitialized" );
        return;
    }

    const int abs_x = abs_sub.x + gridx;
    const int abs_y = abs_sub.y + gridy;
    const int abs_z = gridz;

    if( !zlevels && gridz != abs_sub.z ) {
        debugmsg( "Tried to save submap (%d,%d,%d) as (%d,%d,%d), which isn't supported in non-z-level builds",
                  abs_x, abs_y, abs_sub.z, abs_x, abs_y, gridz );
    }

    dbg( D_INFO ) << "map::saven abs_x: " << abs_x << "  abs_y: " << abs_y << "  abs_z: " << abs_z
                  << "  gridn: " << gridn;
    submap_to_save->last_touched = calendar::turn;
    MAPBUFFER.add_submap( abs_x, abs_y, abs_z, submap_to_save );
}

// worldx & worldy specify where in the world this is;
// gridx & gridy specify which nonant:
// 0,0  1,0  2,0
// 0,1  1,1  2,1
// 0,2  1,2  2,2 etc
// (worldx,worldy,worldz) denotes the absolute coordinate of the submap
// in grid[0].
void map::loadn( const int gridx, const int gridy, const bool update_vehicles )
{
    if( zlevels ) {
        for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
            loadn( gridx, gridy, gridz, update_vehicles );
        }

        // Note: we want it in a separate loop! It is a post-load cleanup
        // Since we're adding roofs, we want it to go up (from lowest to highest)
        for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
            add_roofs( gridx, gridy, gridz );
        }
    } else {
        loadn( gridx, gridy, abs_sub.z, update_vehicles );
    }
}

// Optimized mapgen function that only works properly for very simple overmap types
// Does not create or require a temporary map and does its own saving
static void generate_uniform( const int x, const int y, const int z, const ter_id &terrain_type )
{
    dbg( D_INFO ) << "generate_uniform x: " << x << "  y: " << y << "  abs_z: " << z
                  << "  terrain_type: " << terrain_type.id().str();

    constexpr size_t block_size = SEEX * SEEY;
    for( int xd = 0; xd <= 1; xd++ ) {
        for( int yd = 0; yd <= 1; yd++ ) {
            submap *sm = new submap();
            sm->is_uniform = true;
            std::uninitialized_fill_n( &sm->ter[0][0], block_size, terrain_type );
            sm->last_touched = calendar::turn;
            MAPBUFFER.add_submap( x + xd, y + yd, z, sm );
        }
    }
}

void map::loadn( const int gridx, const int gridy, const int gridz, const bool update_vehicles )
{
    // Cache empty overmap types
    static const oter_id rock( "empty_rock" );
    static const oter_id air( "open_air" );

    dbg( D_INFO ) << "map::loadn(game[" << g.get() << "], worldx[" << abs_sub.x
                  << "], worldy[" << abs_sub.y << "], gridx["
                  << gridx << "], gridy[" << gridy << "], gridz[" << gridz << "])";

    const int absx = abs_sub.x + gridx,
              absy = abs_sub.y + gridy;
    const size_t gridn = get_nonant( { gridx, gridy, gridz } );

    dbg( D_INFO ) << "map::loadn absx: " << absx << "  absy: " << absy
                  << "  gridn: " << gridn;

    const int old_abs_z = abs_sub.z; // Ugly, but necessary at the moment
    abs_sub.z = gridz;

    submap *tmpsub = MAPBUFFER.lookup_submap( absx, absy, gridz );
    if( tmpsub == nullptr ) {
        // It doesn't exist; we must generate it!
        dbg( D_INFO | D_WARNING ) << "map::loadn: Missing mapbuffer data. Regenerating.";

        // Each overmap square is two nonants; to prevent overlap, generate only at
        //  squares divisible by 2.
        const int newmapx = absx - ( abs( absx ) % 2 );
        const int newmapy = absy - ( abs( absy ) % 2 );
        // Short-circuit if the map tile is uniform
        int overx = newmapx;
        int overy = newmapy;
        sm_to_omt( overx, overy );

        const oter_id terrain_type = overmap_buffer.ter( overx, overy, gridz );

        // TODO: Replace with json mapgen functions.
        if( terrain_type == air ) {
            generate_uniform( newmapx, newmapy, gridz, t_open_air );
        } else if( terrain_type == rock ) {
            generate_uniform( newmapx, newmapy, gridz, t_rock );
        } else {
            tinymap tmp_map;
            tmp_map.generate( newmapx, newmapy, gridz, calendar::turn );
        }

        // This is the same call to MAPBUFFER as above!
        tmpsub = MAPBUFFER.lookup_submap( absx, absy, gridz );
        if( tmpsub == nullptr ) {
            dbg( D_ERROR ) << "failed to generate a submap at " << absx << absy << abs_sub.z;
            debugmsg( "failed to generate a submap at %d,%d,%d", absx, absy, abs_sub.z );
            return;
        }
    }

    // New submap changes the content of the map and all caches must be recalculated
    set_transparency_cache_dirty( gridz );
    set_outside_cache_dirty( gridz );
    set_floor_cache_dirty( gridz );
    set_pathfinding_cache_dirty( gridz );
    setsubmap( gridn, tmpsub );

    // Destroy bugged no-part vehicles
    auto &veh_vec = tmpsub->vehicles;
    for( auto iter = veh_vec.begin(); iter != veh_vec.end(); ) {
        vehicle *veh = iter->get();
        if( !veh->parts.empty() ) {
            // Always fix submap coordinates for easier Z-level-related operations
            veh->smx = gridx;
            veh->smy = gridy;
            veh->smz = gridz;
            iter++;
        } else {
            reset_vehicle_cache( gridz );
            if( veh->tracking_on ) {
                overmap_buffer.remove_vehicle( veh );
            }
            dirty_vehicle_list.erase( veh );
            iter = veh_vec.erase( iter );
        }
    }

    // Update vehicle data
    if( update_vehicles ) {
        auto &map_cache = get_cache( gridz );
        for( const auto &veh : tmpsub->vehicles ) {
            // Only add if not tracking already.
            if( map_cache.vehicle_list.find( veh.get() ) == map_cache.vehicle_list.end() ) {
                map_cache.vehicle_list.insert( veh.get() );
                if( !veh->loot_zones.empty() ) {
                    map_cache.zone_vehicles.insert( veh.get() );
                }
                add_vehicle_to_cache( veh.get() );
            }
        }
    }

    actualize( gridx, gridy, gridz );

    abs_sub.z = old_abs_z;
}

bool map::has_rotten_away( item &itm, const tripoint &pnt ) const
{
    if( itm.is_corpse() ) {
        itm.calc_rot( pnt );
        return itm.get_rot() > 10_days && !itm.can_revive();
    } else if( itm.goes_bad() ) {
        itm.calc_rot( pnt );
        return itm.has_rotten_away();
    } else if( itm.type->container && itm.type->container->preserves ) {
        // Containers like tin cans preserves all items inside, they do not rot at all.
        return false;
    } else if( itm.type->container && itm.type->container->seals ) {
        // Items inside rot but do not vanish as the container seals them in.
        for( auto &c : itm.contents ) {
            c.calc_rot( pnt );
        }
        return false;
    } else {
        // Check and remove rotten contents, but always keep the container.
        for( auto it = itm.contents.begin(); it != itm.contents.end(); ) {
            if( has_rotten_away( *it, pnt ) ) {
                it = itm.contents.erase( it );
            } else {
                ++it;
            }
        }

        return false;
    }
}

template <typename Container>
void map::remove_rotten_items( Container &items, const tripoint &pnt )
{
    const tripoint abs_pnt = getabs( pnt );
    for( auto it = items.begin(); it != items.end(); ) {
        if( has_rotten_away( *it, abs_pnt ) ) {
            if( it->is_comestible() ) {
                rotten_item_spawn( *it, pnt );
            }
            it = i_rem( pnt, it );
        } else {
            ++it;
        }
    }
}

void map::rotten_item_spawn( const item &item, const tripoint &pnt )
{
    if( g->critter_at( pnt ) != nullptr ) {
        return;
    }
    auto &comest = item.type->comestible;
    mongroup_id mgroup = comest->rot_spawn;
    if( mgroup == "GROUP_NULL" ) {
        return;
    }
    const int chance = ( comest->rot_spawn_chance * get_option<int>( "CARRION_SPAWNRATE" ) ) / 100;
    if( rng( 0, 100 ) < chance ) {
        MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( mgroup );
        add_spawn( spawn_details.name, 1, pnt.x, pnt.y, false );
        if( g->u.sees( pnt ) ) {
            if( item.is_seed() ) {
                add_msg( m_warning, _( "Something has crawled out of the %s plants!" ), item.get_plant_name() );
            } else {
                add_msg( m_warning, _( "Something has crawled out of the %s!" ), item.tname().c_str() );
            }
        }
    }
}

void map::fill_funnels( const tripoint &p, const time_point &since )
{
    const auto &tr = tr_at( p );
    if( !tr.is_funnel() ) {
        return;
    }
    // Note: the inside/outside cache might not be correct at this time
    if( has_flag_ter_or_furn( TFLAG_INDOORS, p ) ) {
        return;
    }
    auto items = i_at( p );
    units::volume maxvolume = 0_ml;
    auto biggest_container = items.end();
    for( auto candidate = items.begin(); candidate != items.end(); ++candidate ) {
        if( candidate->is_funnel_container( maxvolume ) ) {
            biggest_container = candidate;
        }
    }
    if( biggest_container != items.end() ) {
        retroactively_fill_from_funnel( *biggest_container, tr, since, calendar::turn, getabs( p ) );
    }
}

void map::grow_plant( const tripoint &p )
{
    const auto &furn = this->furn( p ).obj();
    if( !furn.has_flag( "PLANT" ) ) {
        return;
    }
    auto items = i_at( p );
    if( items.empty() ) {
        // No seed there anymore, we don't know what kind of plant it was.
        dbg( D_ERROR ) << "a seed item has vanished at " << p.x << "," << p.y << "," << p.z;
        furn_set( p, f_null );
        return;
    }

    auto seed = items.front();
    if( !seed.is_seed() ) {
        // No seed there anymore, we don't know what kind of plant it was.
        dbg( D_ERROR ) << "a planted item at " << p.x << "," << p.y << "," << p.z << " has no seed data";
        furn_set( p, f_null );
        return;
    }
    const time_duration plantEpoch = seed.get_plant_epoch();
    furn_id cur_furn = this->furn( p ).id();
    if( seed.age() >= plantEpoch && cur_furn != furn_str_id( "f_plant_harvest" ) ) {
        if( seed.age() < plantEpoch * 2 ) {
            if( cur_furn == furn_str_id( "f_plant_seedling" ) ) {
                return;
            }
            i_rem( p, 1 );
            rotten_item_spawn( seed, p );
            furn_set( p, furn_str_id( "f_plant_seedling" ) );
        } else if( seed.age() < plantEpoch * 3 ) {
            if( cur_furn == furn_str_id( "f_plant_mature" ) ) {
                return;
            }
            i_rem( p, 1 );
            rotten_item_spawn( seed, p );
            //You've skipped the seedling stage so roll monsters twice
            if( cur_furn != furn_str_id( "f_plant_seedling" ) ) {
                rotten_item_spawn( seed, p );
            }
            furn_set( p, furn_str_id( "f_plant_mature" ) );
        } else {
            //You've skipped two stages so roll monsters two times
            if( cur_furn == furn_str_id( "f_plant_seedling" ) ) {
                rotten_item_spawn( seed, p );
                rotten_item_spawn( seed, p );
                //One stage change
            } else if( cur_furn == furn_str_id( "f_plant_mature" ) ) {
                rotten_item_spawn( seed, p );
                //Goes from seed to harvest in one check
            } else {
                rotten_item_spawn( seed, p );
                rotten_item_spawn( seed, p );
                rotten_item_spawn( seed, p );
            }
            furn_set( p, furn_str_id( "f_plant_harvest" ) );
        }
    }
}

void map::restock_fruits( const tripoint &p, const time_duration &time_since_last_actualize )
{
    const auto &ter = this->ter( p ).obj();
    if( !ter.has_flag( TFLAG_HARVESTED ) ) {
        return; // Already harvestable. Do nothing.
    }
    // Make it harvestable again if the last actualization was during a different season or year.
    const time_point last_touched = calendar::turn - time_since_last_actualize;
    if( season_of_year( calendar::turn ) != season_of_year( last_touched ) ||
        time_since_last_actualize >= calendar::season_length() ) {
        ter_set( p, ter.transforms_into );
    }
}

void map::produce_sap( const tripoint &p, const time_duration &time_since_last_actualize )
{
    if( time_since_last_actualize <= 0_turns ) {
        return;
    }

    if( t_tree_maple_tapped != ter( p ) ) {
        return;
    }

    // Amount of maple sap liters produced per season per tap
    static const int maple_sap_per_season = 56;

    // How many turns to produce 1 charge (250 ml) of sap?
    const time_duration producing_length = 0.75 * calendar::season_length();

    const time_duration turns_to_produce = producing_length / ( maple_sap_per_season * 4 );

    // How long of this time_since_last_actualize have we been in the producing period (late winter, early spring)?
    time_duration time_producing = 0_turns;

    if( time_since_last_actualize >= calendar::year_length() ) {
        time_producing = producing_length;
    } else {
        // We are only producing sap on the intersection with the sap producing season.
        const time_duration early_spring_end = 0.5f * calendar::season_length();
        const time_duration late_winter_start = 3.75f * calendar::season_length();

        const time_point last_actualize = calendar::turn - time_since_last_actualize;
        const time_duration last_actualize_tof = time_past_new_year( last_actualize );
        bool last_producing = (
                                  last_actualize_tof >= late_winter_start ||
                                  last_actualize_tof < early_spring_end
                              );
        const time_duration current_tof = time_past_new_year( calendar::turn );
        bool current_producing = (
                                     current_tof >= late_winter_start ||
                                     current_tof < early_spring_end
                                 );

        const time_duration non_producing_length = 3.25 * calendar::season_length();

        if( last_producing && current_producing ) {
            if( time_since_last_actualize < non_producing_length ) {
                time_producing = time_since_last_actualize;
            } else {
                time_producing = time_since_last_actualize - non_producing_length;
            }
        } else if( !last_producing && !current_producing ) {
            if( time_since_last_actualize > non_producing_length ) {
                time_producing = time_since_last_actualize - non_producing_length;
            }
        } else if( last_producing && !current_producing ) {
            // We hit the end of early spring
            if( last_actualize_tof < early_spring_end ) {
                time_producing = early_spring_end - last_actualize_tof;
            } else {
                time_producing = calendar::year_length() - last_actualize_tof + early_spring_end;
            }
        } else if( !last_producing && current_producing ) {
            // We hit the start of late winter
            if( current_tof >= late_winter_start ) {
                time_producing = current_tof - late_winter_start;
            } else {
                time_producing = 0.25f * calendar::season_length() + current_tof;
            }
        }
    }

    long new_charges = roll_remainder( time_producing / turns_to_produce );
    // Not enough time to produce 1 charge of sap
    if( new_charges <= 0 ) {
        return;
    }

    item sap( "maple_sap", calendar::turn );

    // Is there a proper container?
    auto items = i_at( p );
    for( auto &it : items ) {
        if( it.is_bucket() || it.is_watertight_container() ) {
            const long capacity = it.get_remaining_capacity_for_liquid( sap, true );
            if( capacity > 0 ) {
                new_charges = std::min<long>( new_charges, capacity );

                // The environment might have poisoned the sap with animals passing by, insects, leaves or contaminants in the ground
                sap.poison = one_in( 10 ) ? 1 : 0;
                sap.charges = new_charges;

                it.fill_with( sap );
            }
            // Only fill up the first container.
            break;
        }
    }
}

void map::rad_scorch( const tripoint &p, const time_duration &time_since_last_actualize )
{
    const int rads = get_radiation( p );
    if( rads == 0 ) {
        return;
    }

    // TODO: More interesting rad scorch chance - base on season length?
    if( !x_in_y( 1.0 * rads * rads * time_since_last_actualize, 91_days ) ) {
        return;
    }

    // First destroy the farmable plants (those are furniture)
    // TODO: Rad-resistant mutant plants (that produce radioactive fruit)
    const furn_t &fid = furn( p ).obj();
    if( fid.has_flag( "PLANT" ) ) {
        i_clear( p );
        furn_set( p, f_null );
    }

    const ter_id tid = ter( p );
    // TODO: De-hardcode this
    static const std::map<ter_id, ter_str_id> dies_into {{
            {t_grass, ter_str_id( "t_dirt" )},
            {t_tree_young, ter_str_id( "t_dirt" )},
            {t_tree_pine, ter_str_id( "t_tree_deadpine" )},
            {t_tree_birch, ter_str_id( "t_tree_birch_harvested" )},
            {t_tree_willow, ter_str_id( "t_tree_willow_harvested" )},
            {t_tree_hickory, ter_str_id( "t_tree_hickory_dead" )},
            {t_tree_hickory_harvested, ter_str_id( "t_tree_hickory_dead" )},
        }};

    const auto iter = dies_into.find( tid );
    if( iter != dies_into.end() ) {
        ter_set( p, iter->second );
        return;
    }

    const ter_t &tr = tid.obj();
    if( tr.has_flag( "SHRUB" ) ) {
        ter_set( p, t_dirt );
    } else if( tr.has_flag( "TREE" ) ) {
        ter_set( p, ter_str_id( "t_tree_dead" ) );
    }
}

void map::decay_cosmetic_fields( const tripoint &p, const time_duration &time_since_last_actualize )
{
    for( auto &pr : field_at( p ) ) {
        auto &fd = pr.second;
        if( !fd.decays_on_actualize() ) {
            continue;
        }

        const time_duration added_age = 2 * time_since_last_actualize / rng( 2, 4 );
        fd.mod_age( added_age );
        const time_duration hl = fieldlist[ fd.getFieldType() ].halflife;
        const int density_drop = fd.getFieldAge() / hl;
        if( density_drop > 0 ) {
            fd.setFieldDensity( fd.getFieldDensity() - density_drop );
            fd.mod_age( -hl * density_drop );
        }
    }
}

void map::actualize( const int gridx, const int gridy, const int gridz )
{
    submap *const tmpsub = get_submap_at_grid( {gridx, gridy, gridz} );
    if( tmpsub == nullptr ) {
        debugmsg( "Actualize called on null submap (%d,%d,%d)", gridx, gridy, gridz );
        return;
    }

    const time_duration time_since_last_actualize = calendar::turn - tmpsub->last_touched;
    const bool do_funnels = ( gridz >= 0 );

    // check spoiled stuff, and fill up funnels while we're at it
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            const tripoint pnt( gridx * SEEX + x, gridy * SEEY + y, gridz );
            const point p( x, y );
            const auto &furn = this->furn( pnt ).obj();
            // plants contain a seed item which must not be removed under any circumstances
            if( !furn.has_flag( "DONT_REMOVE_ROTTEN" ) ) {
                remove_rotten_items( tmpsub->itm[x][y], pnt );
            }

            const auto trap_here = tmpsub->get_trap( p );
            if( trap_here != tr_null ) {
                traplocs[trap_here].push_back( pnt );
            }
            const ter_t &ter = tmpsub->get_ter( p ).obj();
            if( ter.trap != tr_null && ter.trap != tr_ledge ) {
                traplocs[trap_here].push_back( pnt );
            }

            if( do_funnels ) {
                fill_funnels( pnt, tmpsub->last_touched );
            }

            grow_plant( pnt );

            restock_fruits( pnt, time_since_last_actualize );

            produce_sap( pnt, time_since_last_actualize );

            rad_scorch( pnt, time_since_last_actualize );

            decay_cosmetic_fields( pnt, time_since_last_actualize );
        }
    }

    //Check for Merchants to restock
    for( npc &guy : g->all_npcs() ) {
        if( guy.restock != calendar::before_time_starts && calendar::turn > guy.restock ) {
            guy.shop_restock();
        }
    }

    // the last time we touched the submap, is right now.
    tmpsub->last_touched = calendar::turn;
}

void map::add_roofs( const int gridx, const int gridy, const int gridz )
{
    if( !zlevels ) {
        // No roofs required!
        // Why not? Because submaps below and above don't exist yet
        return;
    }

    submap *const sub_here = get_submap_at_grid( {gridx, gridy, gridz} );
    if( sub_here == nullptr ) {
        debugmsg( "Tried to add roofs/floors on null submap on %d,%d,%d",
                  gridx, gridy, gridz );
        return;
    }

    bool check_roof = gridz > -OVERMAP_DEPTH;

    submap *const sub_below = check_roof ? get_submap_at_grid( { gridx, gridy, gridz - 1 } ) : nullptr;

    if( check_roof && sub_below == nullptr ) {
        debugmsg( "Tried to add roofs to sm at %d,%d,%d, but sm below doesn't exist",
                  gridx, gridy, gridz );
        return;
    }

    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            const ter_id ter_here = sub_here->ter[x][y];
            if( ter_here != t_open_air ) {
                continue;
            }

            if( !check_roof ) {
                // Make sure we don't have open air at lowest z-level
                sub_here->ter[x][y] = t_rock_floor;
                continue;
            }

            const ter_t &ter_below = sub_below->ter[x][y].obj();
            if( ter_below.roof ) {
                // TODO: Make roof variable a ter_id to speed this up
                sub_here->ter[x][y] = ter_below.roof.id();
            }
        }
    }
}

void map::copy_grid( const tripoint &to, const tripoint &from )
{
    const auto smap = get_submap_at_grid( from );
    setsubmap( get_nonant( to ), smap );
    for( auto &it : smap->vehicles ) {
        it->smx = to.x;
        it->smy = to.y;
    }
}

void map::spawn_monsters_submap_group( const tripoint &gp, mongroup &group, bool ignore_sight )
{
    const int s_range = std::min( HALF_MAPSIZE_X,
                                  g->u.sight_range( g->light_level( g->u.posz() ) ) );
    int pop = group.population;
    std::vector<tripoint> locations;
    if( !ignore_sight ) {
        // If the submap is one of the outermost submaps, assume that monsters are
        // invisible there.
        if( gp.x == 0 || gp.y == 0 || gp.x + 1 == MAPSIZE || gp.y + 1 == MAPSIZE ) {
            ignore_sight = true;
        }
    }

    if( gp.z != g->u.posz() ) {
        // Note: this is only OK because 3D vision isn't a thing yet
        ignore_sight = true;
    }

    // If the submap is uniform, we can skip many checks
    const submap *current_submap = get_submap_at_grid( gp );
    bool ignore_terrain_checks = false;
    bool ignore_inside_checks = gp.z < 0;
    if( current_submap->is_uniform ) {
        const tripoint upper_left{ SEEX * gp.x, SEEY * gp.y, gp.z };
        if( impassable( upper_left ) ||
            ( !ignore_inside_checks && has_flag_ter_or_furn( TFLAG_INDOORS, upper_left ) ) ) {
            const tripoint glp = getabs( gp );
            dbg( D_WARNING ) << "Empty locations for group " << group.type.str() <<
                             " at uniform submap " << gp.x << "," << gp.y << "," << gp.z <<
                             " global " << glp.x << "," << glp.y << "," << glp.z;
            return;
        }

        ignore_terrain_checks = true;
        ignore_inside_checks = true;
    }

    for( int x = 0; x < SEEX; ++x ) {
        for( int y = 0; y < SEEY; ++y ) {
            int fx = x + SEEX * gp.x;
            int fy = y + SEEY * gp.y;
            tripoint fp{ fx, fy, gp.z };
            if( g->critter_at( fp ) != nullptr ) {
                continue; // there is already some creature
            }

            if( !ignore_terrain_checks && impassable( fp ) ) {
                continue; // solid area, impassable
            }

            if( !ignore_sight && sees( g->u.pos(), fp, s_range ) ) {
                continue; // monster must spawn outside the viewing range of the player
            }

            if( !ignore_inside_checks && has_flag_ter_or_furn( TFLAG_INDOORS, fp ) ) {
                continue; // monster must spawn outside.
            }

            locations.push_back( fp );
        }
    }

    if( locations.empty() ) {
        // TODO: what now? there is no possible place to spawn monsters, most
        // likely because the player can see all the places.
        const tripoint glp = getabs( gp );
        dbg( D_WARNING ) << "Empty locations for group " << group.type.str() <<
                         " at " << gp.x << "," << gp.y << "," << gp.z <<
                         " global " << glp.x << "," << glp.y << "," << glp.z;
        // Just kill the group. It's not like we're removing existing monsters
        // Unless it's a horde - then don't kill it and let it spawn behind a tree or smoke cloud
        if( !group.horde ) {
            group.clear();
        }

        return;
    }

    if( pop ) {
        // Populate the group from its population variable.
        for( int m = 0; m < pop; m++ ) {
            MonsterGroupResult spawn_details = MonsterGroupManager::GetResultFromGroup( group.type, &pop );
            if( !spawn_details.name ) {
                continue;
            }
            monster tmp( spawn_details.name );

            // If a monster came from a horde population, configure them to always be willing to rejoin a horde.
            if( group.horde ) {
                tmp.set_horde_attraction( MHA_ALWAYS );
            }
            for( int i = 0; i < spawn_details.pack_size; i++ ) {
                group.monsters.push_back( tmp );
            }
        }
    }

    // Find horde's target submap
    tripoint horde_target( group.target.x - abs_sub.x,
                           group.target.y - abs_sub.y, abs_sub.z );
    sm_to_ms( horde_target );
    for( auto &tmp : group.monsters ) {
        for( int tries = 0; tries < 10 && !locations.empty(); tries++ ) {
            const tripoint p = random_entry_removed( locations );
            if( !tmp.can_move_to( p ) ) {
                continue; // target can not contain the monster
            }
            tmp.spawn( p );
            if( group.horde ) {
                // Give monster a random point near horde's expected destination
                const tripoint rand_dest = horde_target +
                                           point( rng( 0, SEEX ), rng( 0, SEEY ) );
                const int turns = rl_dist( p, rand_dest ) + group.interest;
                tmp.wander_to( rand_dest, turns );
                add_msg( m_debug, "%s targeting %d,%d,%d", tmp.disp_name().c_str(),
                         tmp.wander_pos.x, tmp.wander_pos.y, tmp.wander_pos.z );
            }

            g->add_zombie( tmp );
            break;
        }
    }
    // indicates the group is empty, and can be removed later
    group.clear();
}

void map::spawn_monsters_submap( const tripoint &gp, bool ignore_sight )
{
    // Load unloaded monsters
    overmap_buffer.spawn_monster( abs_sub.x + gp.x, abs_sub.y + gp.y, gp.z );

    // Only spawn new monsters after existing monsters are loaded.
    auto groups = overmap_buffer.groups_at( abs_sub.x + gp.x, abs_sub.y + gp.y, gp.z );
    for( auto &mgp : groups ) {
        spawn_monsters_submap_group( gp, *mgp, ignore_sight );
    }

    submap *const current_submap = get_submap_at_grid( gp );
    for( auto &i : current_submap->spawns ) {
        for( int j = 0; j < i.count; j++ ) {
            int tries = 0;
            int mx = i.pos.x;
            int my = i.pos.y;
            monster tmp( i.type );
            tmp.mission_id = i.mission_id;
            if( i.name != "NONE" ) {
                tmp.unique_name = i.name;
            }
            if( i.friendly ) {
                tmp.friendly = -1;
            }
            int fx = mx + gp.x * SEEX;
            int fy = my + gp.y * SEEY;
            tripoint pos( fx, fy, gp.z );

            while( ( !g->is_empty( pos ) || !tmp.can_move_to( pos ) ) && tries < 10 ) {
                mx = ( i.pos.x + rng( -3, 3 ) ) % SEEX;
                my = ( i.pos.y + rng( -3, 3 ) ) % SEEY;
                if( mx < 0 ) {
                    mx += SEEX;
                }
                if( my < 0 ) {
                    my += SEEY;
                }
                fx = mx + gp.x * SEEX;
                fy = my + gp.y * SEEY;
                tries++;
                pos = tripoint( fx, fy, gp.z );
            }
            if( tries != 10 ) {
                tmp.spawn( pos );
                g->add_zombie( tmp );
            }
        }
    }
    current_submap->spawns.clear();
}

void map::spawn_monsters( bool ignore_sight )
{
    const int zmin = zlevels ? -OVERMAP_DEPTH : abs_sub.z;
    const int zmax = zlevels ? OVERMAP_HEIGHT : abs_sub.z;
    tripoint gp;
    int &gx = gp.x;
    int &gy = gp.y;
    int &gz = gp.z;
    for( gz = zmin; gz <= zmax; gz++ ) {
        for( gx = 0; gx < my_MAPSIZE; gx++ ) {
            for( gy = 0; gy < my_MAPSIZE; gy++ ) {
                spawn_monsters_submap( gp, ignore_sight );
            }
        }
    }
}

void map::clear_spawns()
{
    for( auto &smap : grid ) {
        smap->spawns.clear();
    }
}

void map::clear_traps()
{
    for( auto &smap : grid ) {
        for( int x = 0; x < SEEX; x++ ) {
            for( int y = 0; y < SEEY; y++ ) {
                const point p( x, y );
                smap->set_trap( p, tr_null );
            }
        }
    }

    // Forget about all trap locations.
    for( auto &i : traplocs ) {
        i.clear();
    }
}

const std::vector<tripoint> &map::trap_locations( const trap_id type ) const
{
    return traplocs[type];
}

bool map::inbounds( const tripoint &p ) const
{
    constexpr tripoint map_boundary_min( 0, 0, -OVERMAP_DEPTH );
    constexpr tripoint map_boundary_max( MAPSIZE_Y, MAPSIZE_X, OVERMAP_HEIGHT );
    constexpr tripoint map_clearance_min( tripoint_zero );
    constexpr tripoint map_clearance_max( 1, 1, 0 );

    constexpr box map_boundaries( map_boundary_min, map_boundary_max );
    constexpr box map_clearance( map_clearance_min, map_clearance_max );

    return generic_inbounds( p, map_boundaries, map_clearance );
}

bool tinymap::inbounds( const tripoint &p ) const
{
    constexpr tripoint map_boundary_min( 0, 0, -OVERMAP_DEPTH );
    constexpr tripoint map_boundary_max( SEEY * 2, SEEX * 2, OVERMAP_HEIGHT );
    constexpr tripoint map_clearance_min( tripoint_zero );
    constexpr tripoint map_clearance_max( 1, 1, 0 );

    constexpr box map_boundaries( map_boundary_min, map_boundary_max );
    constexpr box map_clearance( map_clearance_min, map_clearance_max );

    return generic_inbounds( p, map_boundaries, map_clearance );
}

void map::set_graffiti( const tripoint &p, const std::string &contents )
{
    if( !inbounds( p ) ) {
        return;
    }
    point l;
    submap *const current_submap = get_submap_at( p, l );
    current_submap->set_graffiti( l, contents );
}

void map::delete_graffiti( const tripoint &p )
{
    if( !inbounds( p ) ) {
        return;
    }
    point l;
    submap *const current_submap = get_submap_at( p, l );
    current_submap->delete_graffiti( l );
}

const std::string &map::graffiti_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        static const std::string empty_string;
        return empty_string;
    }
    point l;
    submap *const current_submap = get_submap_at( p, l );
    return current_submap->get_graffiti( l );
}

bool map::has_graffiti_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    point l;
    submap *const current_submap = get_submap_at( p, l );
    return current_submap->has_graffiti( l );
}

long map::determine_wall_corner( const tripoint &p ) const
{
    int test_connect_group = ter( tripoint( p.x, p.y, p.z ) ).obj().connect_group;
    uint8_t connections = get_known_connections( p, test_connect_group );
    // The bits in connections are SEWN, whereas the characters in LINE_
    // constants are NESW, so we want values in 8 | 2 | 1 | 4 order.
    switch( connections ) {
        case 8 | 2 | 1 | 4:
            return LINE_XXXX;
        case 0 | 2 | 1 | 4:
            return LINE_OXXX;

        case 8 | 0 | 1 | 4:
            return LINE_XOXX;
        case 0 | 0 | 1 | 4:
            return LINE_OOXX;

        case 8 | 2 | 0 | 4:
            return LINE_XXOX;
        case 0 | 2 | 0 | 4:
            return LINE_OXOX;
        case 8 | 0 | 0 | 4:
            return LINE_XOOX;
        case 0 | 0 | 0 | 4:
            return LINE_OXOX; // LINE_OOOX would be better

        case 8 | 2 | 1 | 0:
            return LINE_XXXO;
        case 0 | 2 | 1 | 0:
            return LINE_OXXO;
        case 8 | 0 | 1 | 0:
            return LINE_XOXO;
        case 0 | 0 | 1 | 0:
            return LINE_XOXO; // LINE_OOXO would be better
        case 8 | 2 | 0 | 0:
            return LINE_XXOO;
        case 0 | 2 | 0 | 0:
            return LINE_OXOX; // LINE_OXOO would be better
        case 8 | 0 | 0 | 0:
            return LINE_XOXO; // LINE_XOOO would be better

        case 0 | 0 | 0 | 0:
            return ter( p ).obj().symbol(); // technically just a column

        default:
            // assert( false );
            // this shall not happen
            return '?';
    }
}

void map::build_outside_cache( const int zlev )
{
    auto &ch = get_cache( zlev );
    if( !ch.outside_cache_dirty ) {
        return;
    }

    // Make a bigger cache to avoid bounds checking
    // We will later copy it to our regular cache
    const size_t padded_w = ( MAPSIZE_X ) + 2;
    const size_t padded_h = ( MAPSIZE_Y ) + 2;
    bool padded_cache[padded_w][padded_h];

    auto &outside_cache = ch.outside_cache;
    if( zlev < 0 ) {
        std::uninitialized_fill_n(
            &outside_cache[0][0], ( MAPSIZE_X ) * ( MAPSIZE_Y ), false );
        return;
    }

    std::uninitialized_fill_n(
        &padded_cache[0][0], padded_w * padded_h, true );

    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const auto cur_submap = get_submap_at_grid( { smx, smy, zlev } );

            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    point sp( sx, sy );
                    if( cur_submap->get_ter( sp ).obj().has_flag( TFLAG_INDOORS ) ||
                        cur_submap->get_furn( sp ).obj().has_flag( TFLAG_INDOORS ) ) {
                        const int x = sx + ( smx * SEEX );
                        const int y = sy + ( smy * SEEY );
                        // Add 1 to both coordinates, because we're operating on the padded cache
                        for( int dx = 0; dx <= 2; dx++ ) {
                            for( int dy = 0; dy <= 2; dy++ ) {
                                padded_cache[x + dx][y + dy] = false;
                            }
                        }
                    }
                }
            }
        }
    }

    // Copy the padded cache back to the proper one, but with no padding
    for( int x = 0; x < SEEX * my_MAPSIZE; x++ ) {
        std::copy_n( &padded_cache[x + 1][1], SEEX * my_MAPSIZE, &outside_cache[x][0] );
    }

    ch.outside_cache_dirty = false;
}

void map::build_obstacle_cache( const tripoint &start, const tripoint &end,
                                fragment_cloud( &obstacle_cache )[MAPSIZE_X][MAPSIZE_Y] )
{
    const point min_submap{ std::max( 0, start.x / SEEX ), std::max( 0, start.y / SEEY ) };
    const point max_submap{
        std::min( my_MAPSIZE - 1, end.x / SEEX ), std::min( my_MAPSIZE - 1, end.y / SEEY ) };
    // Find and cache all the map obstacles.
    // For now setting obstacles to be extremely dense and fill their squares.
    // In future, scale effective obstacle density by the thickness of the obstacle.
    // Also consider modelling partial obstacles.
    // TODO: Support z-levels.
    for( int smx = min_submap.x; smx <= max_submap.x; ++smx ) {
        for( int smy = min_submap.y; smy <= max_submap.y; ++smy ) {
            const auto cur_submap = get_submap_at_grid( { smx, smy, start.z } );

            // TODO: Init indices to prevent iterating over unused submap sections.
            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    const point sp( sx, sy );
                    int ter_move = cur_submap->get_ter( sp ).obj().movecost;
                    int furn_move = cur_submap->get_furn( sp ).obj().movecost;
                    const int x = sx + ( smx * SEEX );
                    const int y = sy + ( smy * SEEY );
                    if( ter_move == 0 || furn_move < 0 || ter_move + furn_move == 0 ) {
                        obstacle_cache[x][y].velocity = 1000.0f;
                        obstacle_cache[x][y].density = 0.0f;
                    } else {
                        // Magic number warning, this is the density of air at sea level at
                        // some nominal temp and humidity.
                        // TODO: figure out if our temp/altitude/humidity variation is
                        // sufficient to bother setting this differently.
                        obstacle_cache[x][y].velocity = 1.2f;
                        obstacle_cache[x][y].density = 1.0f;
                    }
                }
            }
        }
    }
    VehicleList vehs = get_vehicles( start, end );
    // Cache all the vehicle stuff in one loop
    for( auto &v : vehs ) {
        for( const vpart_reference &vp : v.v->get_all_parts() ) {
            int px = v.x + vp.part().precalc[0].x;
            int py = v.y + vp.part().precalc[0].y;
            if( v.z != start.z ) {
                break;
            }
            if( px < start.x || py < start.y || v.z < start.z ||
                px > end.x || py > end.y || v.z > end.z ) {
                continue;
            }

            if( vp.obstacle_at_part() ) {
                obstacle_cache[px][py].velocity = 1000.0f;
                obstacle_cache[px][py].density = 0.0f;
            }
        }
    }
    // Iterate over creatures and set them to block their squares relative to their size.
    for( Creature &critter : g->all_creatures() ) {
        const tripoint &loc = critter.pos();
        if( loc.z != start.z ) {
            continue;
        }
        // TODO: scale this with expected creature "thickness".
        obstacle_cache[loc.x][loc.y].velocity = 1000.0f;
        // ranged_target_size is "proportion of square that is blocked", and density needs to be
        // "transmissivity of square", so we need the reciprocal.
        obstacle_cache[loc.x][loc.y].density = 1.0 - critter.ranged_target_size();
    }
}

void map::build_floor_cache( const int zlev )
{
    auto &ch = get_cache( zlev );
    if( !ch.floor_cache_dirty ) {
        return;
    }

    auto &floor_cache = ch.floor_cache;
    std::uninitialized_fill_n(
        &floor_cache[0][0], ( MAPSIZE_X ) * ( MAPSIZE_Y ), true );

    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const auto cur_submap = get_submap_at_grid( { smx, smy, zlev } );

            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    // Note: furniture currently can't affect existence of floor
                    if( cur_submap->get_ter( { sx, sy } ).obj().has_flag( TFLAG_NO_FLOOR ) ) {
                        const int x = sx + ( smx * SEEX );
                        const int y = sy + ( smy * SEEY );
                        floor_cache[x][y] = false;
                    }
                }
            }
        }
    }

    ch.floor_cache_dirty = false;
}

void map::build_floor_caches()
{
    const int minz = zlevels ? -OVERMAP_DEPTH : abs_sub.z;
    const int maxz = zlevels ? OVERMAP_HEIGHT : abs_sub.z;
    for( int z = minz; z <= maxz; z++ ) {
        build_floor_cache( z );
    }
}

void map::build_map_cache( const int zlev, bool skip_lightmap )
{
    const int minz = zlevels ? -OVERMAP_DEPTH : zlev;
    const int maxz = zlevels ? OVERMAP_HEIGHT : zlev;
    for( int z = minz; z <= maxz; z++ ) {
        build_outside_cache( z );
        build_transparency_cache( z );
        build_floor_cache( z );
    }

    tripoint start( 0, 0, minz );
    tripoint end( SEEX * my_MAPSIZE, SEEY * my_MAPSIZE, maxz );
    VehicleList vehs = get_vehicles( start, end );
    // Cache all the vehicle stuff in one loop
    for( auto &v : vehs ) {
        auto &ch = get_cache( v.z );
        auto &outside_cache = ch.outside_cache;
        auto &transparency_cache = ch.transparency_cache;
        auto &floor_cache = ch.floor_cache;
        for( const vpart_reference &vp : v.v->get_all_parts() ) {
            const size_t part = vp.part_index();
            int px = v.x + vp.part().precalc[0].x;
            int py = v.y + vp.part().precalc[0].y;
            const point p( px, py );
            if( !inbounds( p ) ) {
                continue;
            }

            bool vehicle_is_opaque =
                vp.has_feature( VPFLAG_OPAQUE ) && !vp.part().is_broken();

            if( vehicle_is_opaque ) {
                int dpart = v.v->part_with_feature( part, VPFLAG_OPENABLE, true );
                if( dpart < 0 || !v.v->parts[dpart].open ) {
                    transparency_cache[px][py] = LIGHT_TRANSPARENCY_SOLID;
                } else {
                    vehicle_is_opaque = false;
                }
            }

            if( vehicle_is_opaque || vp.is_inside() ) {
                outside_cache[px][py] = false;
            }

            if( vp.has_feature( VPFLAG_BOARDABLE ) && !vp.part().is_broken() ) {
                floor_cache[px][py] = true;
            }
        }
    }

    build_seen_cache( g->u.pos(), zlev );
    if( !skip_lightmap ) {
        generate_lightmap( zlev );
    }
}

std::vector<point> closest_points_first( int radius, point p )
{
    return closest_points_first( radius, p.x, p.y );
}

//this returns points in a spiral pattern starting at center_x/center_y until it hits the radius. clockwise fashion
//credit to Tom J Nowell; http://stackoverflow.com/a/1555236/1269969
std::vector<point> closest_points_first( int radius, int center_x, int center_y )
{
    std::vector<point> points;
    int X = ( radius * 2 ) + 1;
    int Y = ( radius * 2 ) + 1;
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = -1;
    int t = std::max( X, Y );
    int maxI = t * t;
    for( int i = 0; i < maxI; i++ ) {
        if( ( -X / 2 <= x ) && ( x <= X / 2 ) && ( -Y / 2 <= y ) && ( y <= Y / 2 ) ) {
            points.push_back( point( x + center_x, y + center_y ) );
        }
        if( ( x == y ) || ( ( x < 0 ) && ( x == -y ) ) || ( ( x > 0 ) && ( x == 1 - y ) ) ) {
            t = dx;
            dx = -dy;
            dy = t;
        }
        x += dx;
        y += dy;
    }
    return points;
}

std::vector<tripoint> closest_tripoints_first( int radius, const tripoint &center )
{
    std::vector<tripoint> points;
    int X = ( radius * 2 ) + 1;
    int Y = ( radius * 2 ) + 1;
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = -1;
    int t = std::max( X, Y );
    int maxI = t * t;
    for( int i = 0; i < maxI; i++ ) {
        if( ( -X / 2 <= x ) && ( x <= X / 2 ) && ( -Y / 2 <= y ) && ( y <= Y / 2 ) ) {
            points.push_back( tripoint( x + center.x, y + center.y, center.z ) );
        }
        if( ( x == y ) || ( ( x < 0 ) && ( x == -y ) ) || ( ( x > 0 ) && ( x == 1 - y ) ) ) {
            t = dx;
            dx = -dy;
            dy = t;
        }
        x += dx;
        y += dy;
    }
    return points;
}
//////////
///// coordinate helpers

point map::getabs( const int x, const int y ) const
{
    return point( x + abs_sub.x * SEEX, y + abs_sub.y * SEEY );
}

tripoint map::getabs( const tripoint &p ) const
{
    return tripoint( p.x + abs_sub.x * SEEX, p.y + abs_sub.y * SEEY, p.z );
}

point map::getlocal( const int x, const int y ) const
{
    return point( x - abs_sub.x * SEEX, y - abs_sub.y * SEEY );
}

tripoint map::getlocal( const tripoint &p ) const
{
    return tripoint( p.x - abs_sub.x * SEEX, p.y - abs_sub.y * SEEY, p.z );
}

void map::set_abs_sub( const int x, const int y, const int z )
{
    abs_sub = tripoint( x, y, z );
}

tripoint map::get_abs_sub() const
{
    return abs_sub;
}

submap *map::getsubmap( const size_t grididx ) const
{
    if( grididx >= grid.size() ) {
        debugmsg( "Tried to access invalid grid index %d. Grid size: %d", grididx, grid.size() );
        return nullptr;
    }
    return grid[grididx];
}

void map::setsubmap( const size_t grididx, submap *const smap )
{
    if( grididx >= grid.size() ) {
        debugmsg( "Tried to access invalid grid index %d", grididx );
        return;
    } else if( smap == nullptr ) {
        debugmsg( "Tried to set NULL submap pointer at index %d", grididx );
        return;
    }
    grid[grididx] = smap;
}

submap *map::get_submap_at( const point &p ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "Tried to access invalid map position (%d, %d, %d)", p.x, p.y, abs_sub.z );
        return nullptr;
    }
    return get_submap_at_grid( { p.x / SEEX, p.y / SEEY, abs_sub.z } );
}

submap *map::get_submap_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "Tried to access invalid map position (%d, %d, %d)", p.x, p.y, p.z );
        return nullptr;
    }
    return get_submap_at_grid( { p.x / SEEX, p.y / SEEY, p.z } );
}

submap *map::get_submap_at( const point &p, point &offset_p ) const
{
    return get_submap_at( { p.x, p.y, abs_sub.z }, offset_p );
}

submap *map::get_submap_at( const tripoint &p, point &offset_p ) const
{
    offset_p.x = p.x % SEEX;
    offset_p.y = p.y % SEEY;
    return get_submap_at( p );
}

submap *map::get_submap_at_grid( const point &gridp ) const
{
    return getsubmap( get_nonant( gridp ) );
}

submap *map::get_submap_at_grid( const tripoint &gridp ) const
{
    return getsubmap( get_nonant( gridp ) );
}

size_t map::get_nonant( const point &gridp ) const
{
    return get_nonant( { gridp.x, gridp.y, abs_sub.z } );
}

size_t map::get_nonant( const tripoint &gridp ) const
{
    if( gridp.x < 0 || gridp.x >= my_MAPSIZE ||
        gridp.y < 0 || gridp.y >= my_MAPSIZE ||
        gridp.z < -OVERMAP_DEPTH || gridp.z > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to access invalid map position at grid (%d,%d,%d)", gridp.x, gridp.y, gridp.z );
        return 0;
    }

    if( zlevels ) {
        const int indexz = gridp.z + OVERMAP_HEIGHT; // Can't be lower than 0
        return indexz + ( gridp.x + gridp.y * my_MAPSIZE ) * OVERMAP_LAYERS;
    } else {
        return gridp.x + gridp.y * my_MAPSIZE;
    }
}

tinymap::tinymap( int mapsize, bool zlevels )
    : map( mapsize, zlevels )
{
}

void map::draw_line_ter( const ter_id type, int x1, int y1, int x2, int y2 )
{
    draw_line( [this, type]( int x, int y ) {
        this->ter_set( x, y, type );
    }, x1, y1, x2, y2 );
}

void map::draw_line_furn( const furn_id type, int x1, int y1, int x2, int y2 )
{
    draw_line( [this, type]( int x, int y ) {
        this->furn_set( x, y, type );
    }, x1, y1, x2, y2 );
}

void map::draw_fill_background( const ter_id type )
{
    // Need to explicitly set caches dirty - set_ter would do it before
    set_transparency_cache_dirty( abs_sub.z );
    set_outside_cache_dirty( abs_sub.z );
    set_pathfinding_cache_dirty( abs_sub.z );

    // Fill each submap rather than each tile
    constexpr size_t block_size = SEEX * SEEY;
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            auto sm = get_submap_at_grid( {gridx, gridy} );
            sm->is_uniform = true;
            std::uninitialized_fill_n( &sm->ter[0][0], block_size, type );
        }
    }
}

void map::draw_fill_background( ter_id( *f )() )
{
    draw_square_ter( f, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1 );
}
void map::draw_fill_background( const weighted_int_list<ter_id> &f )
{
    draw_square_ter( f, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1 );
}

void map::draw_square_ter( const ter_id type, int x1, int y1, int x2, int y2 )
{
    draw_square( [this, type]( int x, int y ) {
        this->ter_set( x, y, type );
    }, x1, y1, x2, y2 );
}

void map::draw_square_furn( const furn_id type, int x1, int y1, int x2, int y2 )
{
    draw_square( [this, type]( int x, int y ) {
        this->furn_set( x, y, type );
    }, x1, y1, x2, y2 );
}

void map::draw_square_ter( ter_id( *f )(), int x1, int y1, int x2, int y2 )
{
    draw_square( [this, f]( int x, int y ) {
        this->ter_set( x, y, f() );
    }, x1, y1, x2, y2 );
}

void map::draw_square_ter( const weighted_int_list<ter_id> &f, int x1, int y1, int x2, int y2 )
{
    draw_square( [this, f]( int x, int y ) {
        const ter_id *tid = f.pick();
        this->ter_set( x, y, tid != nullptr ? *tid : t_null );
    }, x1, y1, x2, y2 );
}

void map::draw_rough_circle_ter( const ter_id type, int x, int y, int rad )
{
    draw_rough_circle( [this, type]( int x, int y ) {
        this->ter_set( x, y, type );
    }, x, y, rad );
}

void map::draw_rough_circle_furn( const furn_id type, int x, int y, int rad )
{
    draw_rough_circle( [this, type]( int x, int y ) {
        this->furn_set( x, y, type );
    }, x, y, rad );
}

void map::draw_circle_ter( const ter_id type, double x, double y, double rad )
{
    draw_circle( [this, type]( int x, int y ) {
        this->ter_set( x, y, type );
    }, x, y, rad );
}

void map::draw_circle_ter( const ter_id type, int x, int y, int rad )
{
    draw_circle( [this, type]( int x, int y ) {
        this->ter_set( x, y, type );
    }, x, y, rad );
}

void map::draw_circle_furn( const furn_id type, int x, int y, int rad )
{
    draw_circle( [this, type]( int x, int y ) {
        this->furn_set( x, y, type );
    }, x, y, rad );
}

void map::add_corpse( const tripoint &p )
{
    item body;

    const bool isReviveSpecial = one_in( 10 );

    if( !isReviveSpecial ) {
        body = item::make_corpse();
    } else {
        body = item::make_corpse( mon_zombie );
        body.item_tags.insert( "REVIVE_SPECIAL" );
        body.active = true;
    }

    add_item_or_charges( p, body );
    put_items_from_loc( "shoes",  p, 0 );
    put_items_from_loc( "pants",  p, 0 );
    put_items_from_loc( "shirts", p, 0 );
    if( one_in( 6 ) ) {
        put_items_from_loc( "jackets", p, 0 );
    }
    if( one_in( 15 ) ) {
        put_items_from_loc( "bags", p, 0 );
    }
}

field &map::get_field( const tripoint &p )
{
    return field_at( p );
}

void map::creature_on_trap( Creature &c, const bool may_avoid )
{
    const auto &tr = tr_at( c.pos() );
    if( tr.is_null() ) {
        return;
    }
    // boarded in a vehicle means the player is above the trap, like a flying monster and can
    // never trigger the trap.
    const player *const p = dynamic_cast<const player *>( &c );
    if( p != nullptr && p->in_vehicle ) {
        return;
    }
    if( may_avoid && c.avoid_trap( c.pos(), tr ) ) {
        return;
    }
    tr.trigger( c.pos(), &c );
}

template<typename Functor>
void map::function_over( const tripoint &start, const tripoint &end, Functor fun ) const
{
    function_over( start.x, start.y, start.z, end.x, end.y, end.z, fun );
}

template<typename Functor>
void map::function_over( const int stx, const int sty, const int stz,
                         const int enx, const int eny, const int enz, Functor fun ) const
{
    // start and end are just two points, end can be "before" start
    // Also clip the area to map area
    const int minx = std::max( std::min( stx, enx ), 0 );
    const int miny = std::max( std::min( sty, eny ), 0 );
    const int minz = std::max( std::min( stz, enz ), -OVERMAP_DEPTH );
    const int maxx = std::min( std::max( stx, enx ), SEEX * my_MAPSIZE - 1 );
    const int maxy = std::min( std::max( sty, eny ), SEEY * my_MAPSIZE - 1 );
    const int maxz = std::min( std::max( stz, enz ), OVERMAP_HEIGHT );

    // Submaps that contain the bounding points
    const int min_smx = minx / SEEX;
    const int min_smy = miny / SEEY;
    const int max_smx = maxx / SEEX;
    const int max_smy = maxy / SEEY;
    // Z outermost, because submaps are flat
    tripoint gp;
    int &z = gp.z;
    int &smx = gp.x;
    int &smy = gp.y;
    for( z = minz; z <= maxz; z++ ) {
        for( smx = min_smx; smx <= max_smx; smx++ ) {
            for( smy = min_smy; smy <= max_smy; smy++ ) {
                submap const *cur_submap = get_submap_at_grid( { smx, smy, z } );
                // Bounds on the submap coordinates
                const int sm_minx = smx > min_smx ? 0 : minx % SEEX;
                const int sm_miny = smy > min_smy ? 0 : miny % SEEY;
                const int sm_maxx = smx < max_smx ? ( SEEX - 1 ) : maxx % SEEX;
                const int sm_maxy = smy < max_smy ? ( SEEY - 1 ) : maxy % SEEY;

                point lp;
                int &sx = lp.x;
                int &sy = lp.y;
                for( sx = sm_minx; sx <= sm_maxx; ++sx ) {
                    for( sy = sm_miny; sy <= sm_maxy; ++sy ) {
                        const iteration_state rval = fun( gp, cur_submap, lp );
                        if( rval != ITER_CONTINUE ) {
                            switch( rval ) {
                                case ITER_SKIP_ZLEVEL:
                                    smx = my_MAPSIZE + 1;
                                    smy = my_MAPSIZE + 1;
                                // Fall through
                                case ITER_SKIP_SUBMAP:
                                    sx = SEEX;
                                    sy = SEEY;
                                    break;
                                default:
                                    return;
                            }
                        }
                    }
                }
            }
        }
    }
}

void map::scent_blockers( std::array<std::array<bool, MAPSIZE_X>, MAPSIZE_Y> &blocks_scent,
                          std::array<std::array<bool, MAPSIZE_X>, MAPSIZE_Y> &reduces_scent,
                          const int minx, const int miny, const int maxx, const int maxy )
{
    auto reduce = TFLAG_REDUCE_SCENT;
    auto block = TFLAG_WALL;
    auto fill_values = [&]( const tripoint & gp, const submap * sm, const point & lp ) {
        // We need to generate the x/y coordinates, because we can't get them "for free"
        const int x = gp.x * SEEX + lp.x;
        const int y = gp.y * SEEY + lp.y;
        if( sm->get_ter( lp ).obj().has_flag( block ) ) {
            blocks_scent[x][y] = true;
            reduces_scent[x][y] = false;
        } else if( sm->get_ter( lp ).obj().has_flag( reduce ) ||
                   sm->get_furn( lp ).obj().has_flag( reduce ) ) {
            blocks_scent[x][y] = false;
            reduces_scent[x][y] = true;
        } else {
            blocks_scent[x][y] = false;
            reduces_scent[x][y] = false;
        }

        return ITER_CONTINUE;
    };

    function_over( minx, miny, abs_sub.z, maxx, maxy, abs_sub.z, fill_values );

    // Now vehicles

    // Currently the scentmap is limited to an area around the player rather than entire map
    auto local_bounds = [ = ]( const tripoint & coord ) {
        return coord.x >= minx && coord.x <= maxx && coord.y >= miny && coord.y <= maxy;
    };

    auto vehs = get_vehicles();
    for( auto &wrapped_veh : vehs ) {
        vehicle &veh = *( wrapped_veh.v );
        for( const vpart_reference &vp : veh.get_any_parts( VPFLAG_OBSTACLE ) ) {
            const tripoint part_pos = vp.pos();
            if( local_bounds( part_pos ) ) {
                reduces_scent[part_pos.x][part_pos.y] = true;
            }
        }

        // Doors, but only the closed ones
        for( const vpart_reference &vp : veh.get_any_parts( VPFLAG_OPENABLE ) ) {
            if( vp.part().open ) {
                continue;
            }

            const tripoint part_pos = vp.pos();
            if( local_bounds( part_pos ) ) {
                reduces_scent[part_pos.x][part_pos.y] = true;
            }
        }
    }
}

tripoint_range map::points_in_rectangle( const tripoint &from, const tripoint &to ) const
{
    const int minx = std::max( 0, std::min( from.x, to.x ) );
    const int miny = std::max( 0, std::min( from.y, to.y ) );
    const int minz = std::max( -OVERMAP_DEPTH, std::min( from.z, to.z ) );
    const int maxx = std::min( SEEX * my_MAPSIZE - 1, std::max( from.x, to.x ) );
    const int maxy = std::min( SEEX * my_MAPSIZE - 1, std::max( from.y, to.y ) );
    const int maxz = std::min( OVERMAP_HEIGHT, std::max( from.z, to.z ) );
    return tripoint_range( tripoint( minx, miny, minz ), tripoint( maxx, maxy, maxz ) );
}

tripoint_range map::points_in_radius( const tripoint &center, size_t radius, size_t radiusz ) const
{
    const int minx = std::max<int>( 0, center.x - radius );
    const int miny = std::max<int>( 0, center.y - radius );
    const int minz = std::max<int>( -OVERMAP_DEPTH, center.z - radiusz );
    const int maxx = std::min<int>( SEEX * my_MAPSIZE - 1, center.x + radius );
    const int maxy = std::min<int>( SEEX * my_MAPSIZE - 1, center.y + radius );
    const int maxz = std::min<int>( OVERMAP_HEIGHT, center.z + radiusz );
    return tripoint_range( tripoint( minx, miny, minz ), tripoint( maxx, maxy, maxz ) );
}

std::list<item_location> map::get_active_items_in_radius( const tripoint &center, int radius ) const
{
    std::list<item_location> result;

    const point minp( center.x - radius, center.y - radius );
    const point maxp( center.x + radius, center.y + radius );

    const point ming( std::max( minp.x / SEEX, 0 ),
                      std::max( minp.y / SEEY, 0 ) );
    const point maxg( std::min( maxp.x / SEEX, my_MAPSIZE - 1 ),
                      std::min( maxp.y / SEEY, my_MAPSIZE - 1 ) );

    for( int gx = ming.x; gx <= maxg.x; ++gx ) {
        for( int gy = ming.y; gy <= maxg.y; ++gy ) {
            const point sm_offset( gx * SEEX, gy * SEEY );

            for( const auto &elem : get_submap_at_grid( { gx, gy, center.z } )->active_items.get() ) {
                const tripoint pos( sm_offset + elem.location, center.z );

                if( rl_dist( pos, center ) > radius ) {
                    continue;
                }

                result.emplace_back( map_cursor( pos ), &*elem.item_iterator );
            }
        }
    }

    return result;
}

level_cache &map::access_cache( int zlev )
{
    if( zlev >= -OVERMAP_DEPTH && zlev <= OVERMAP_HEIGHT ) {
        return *caches[zlev + OVERMAP_DEPTH];
    }

    debugmsg( "access_cache called with invalid z-level: %d", zlev );
    return nullcache;
}

const level_cache &map::access_cache( int zlev ) const
{
    if( zlev >= -OVERMAP_DEPTH && zlev <= OVERMAP_HEIGHT ) {
        return *caches[zlev + OVERMAP_DEPTH];
    }

    debugmsg( "access_cache called with invalid z-level: %d", zlev );
    return nullcache;
}

level_cache::level_cache()
{
    const int map_dimensions = MAPSIZE_X * MAPSIZE_Y;
    transparency_cache_dirty = true;
    outside_cache_dirty = true;
    floor_cache_dirty = false;
    constexpr four_quadrants four_zeros( 0.0f );
    std::fill_n( &lm[0][0], map_dimensions, four_zeros );
    std::fill_n( &sm[0][0], map_dimensions, 0.0f );
    std::fill_n( &light_source_buffer[0][0], map_dimensions, 0.0f );
    std::fill_n( &outside_cache[0][0], map_dimensions, false );
    std::fill_n( &floor_cache[0][0], map_dimensions, false );
    std::fill_n( &transparency_cache[0][0], map_dimensions, 0.0f );
    std::fill_n( &seen_cache[0][0], map_dimensions, 0.0f );
    std::fill_n( &camera_cache[0][0], map_dimensions, 0.0f );
    std::fill_n( &visibility_cache[0][0], map_dimensions, LL_DARK );
    veh_in_active_range = false;
    std::fill_n( &veh_exists_at[0][0], map_dimensions, false );
}

pathfinding_cache::pathfinding_cache()
{
    dirty = true;
}

pathfinding_cache::~pathfinding_cache() = default;

pathfinding_cache &map::get_pathfinding_cache( int zlev ) const
{
    return *pathfinding_caches[zlev + OVERMAP_DEPTH];
}

void map::set_pathfinding_cache_dirty( const int zlev )
{
    if( inbounds_z( zlev ) ) {
        get_pathfinding_cache( zlev ).dirty = true;
    }
}

const pathfinding_cache &map::get_pathfinding_cache_ref( int zlev ) const
{
    if( !inbounds_z( zlev ) ) {
        debugmsg( "Tried to get pathfinding cache for out of bounds z-level %d", zlev );
        return *pathfinding_caches[ OVERMAP_DEPTH ];
    }
    auto &cache = get_pathfinding_cache( zlev );
    if( cache.dirty ) {
        update_pathfinding_cache( zlev );
    }

    return cache;
}

void map::update_pathfinding_cache( int zlev ) const
{
    auto &cache = get_pathfinding_cache( zlev );
    if( !cache.dirty ) {
        return;
    }

    std::uninitialized_fill_n( &cache.special[0][0], MAPSIZE_X * MAPSIZE_Y, PF_NORMAL );

    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            const auto cur_submap = get_submap_at_grid( { smx, smy, zlev } );

            tripoint p( 0, 0, zlev );

            for( int sx = 0; sx < SEEX; ++sx ) {
                p.x = sx + smx * SEEX;
                for( int sy = 0; sy < SEEY; ++sy ) {
                    p.y = sy + smy * SEEY;

                    pf_special cur_value = PF_NORMAL;

                    maptile tile( cur_submap, sx, sy );

                    const auto &terrain = tile.get_ter_t();
                    const auto &furniture = tile.get_furn_t();
                    int part;
                    const vehicle *veh = veh_at_internal( p, part );

                    const int cost = move_cost_internal( furniture, terrain, veh, part );

                    if( cost > 2 ) {
                        cur_value |= PF_SLOW;
                    } else if( cost <= 0 ) {
                        cur_value |= PF_WALL;
                        if( terrain.has_flag( TFLAG_CLIMBABLE ) ) {
                            cur_value |= PF_CLIMBABLE;
                        }
                    }

                    if( veh != nullptr ) {
                        cur_value |= PF_VEHICLE;
                    }

                    for( const auto &fld : tile.get_field() ) {
                        const field_entry &cur = fld.second;
                        const field_id type = cur.getFieldType();
                        const int density = cur.getFieldDensity();
                        if( fieldlist[type].dangerous[density - 1] ) {
                            cur_value |= PF_FIELD;
                        }
                    }

                    if( !tile.get_trap_t().is_benign() || !terrain.trap.obj().is_benign() ) {
                        cur_value |= PF_TRAP;
                    }

                    if( terrain.has_flag( TFLAG_GOES_DOWN ) || terrain.has_flag( TFLAG_GOES_UP ) ||
                        terrain.has_flag( TFLAG_RAMP ) ) {
                        cur_value |= PF_UPDOWN;
                    }

                    cache.special[p.x][p.y] = cur_value;
                }
            }
        }
    }

    cache.dirty = false;
}

void map::clip_to_bounds( tripoint &p ) const
{
    clip_to_bounds( p.x, p.y, p.z );
}

void map::clip_to_bounds( int &x, int &y ) const
{
    if( x < 0 ) {
        x = 0;
    } else if( x >= SEEX * my_MAPSIZE ) {
        x = SEEX * my_MAPSIZE - 1;
    }

    if( y < 0 ) {
        y = 0;
    } else if( y >= SEEY * my_MAPSIZE ) {
        y = SEEY * my_MAPSIZE - 1;
    }
}

void map::clip_to_bounds( int &x, int &y, int &z ) const
{
    clip_to_bounds( x, y );
    if( z < -OVERMAP_DEPTH ) {
        z = -OVERMAP_DEPTH;
    } else if( z > OVERMAP_HEIGHT ) {
        z = OVERMAP_HEIGHT;
    }
}
