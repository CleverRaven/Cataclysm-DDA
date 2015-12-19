#include "map.h"
#include "lightmap.h"
#include "output.h"
#include "rng.h"
#include "game.h"
#include "line.h"
#include "options.h"
#include "item_factory.h"
#include "mapbuffer.h"
#include "translations.h"
#include "sounds.h"
#include "debug.h"
#include "trap.h"
#include "messages.h"
#include "mapsharing.h"
#include "iuse_actor.h"
#include "mongroup.h"
#include "npc.h"
#include "event.h"
#include "monster.h"
#include "vehicle.h"
#include "veh_type.h"
#include "artifact.h"
#include "omdata.h"
#include "submap.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mtype.h"
#include "weather.h"
#include "item_group.h"

#include <cmath>
#include <stdlib.h>
#include <fstream>
#include <cstring>

const mtype_id mon_spore( "mon_spore" );
const mtype_id mon_zombie( "mon_zombie" );

const skill_id skill_driving( "driving" );
const skill_id skill_traps( "traps" );

const species_id FUNGUS( "FUNGUS" );

extern bool is_valid_in_w_terrain(int,int);

#include "overmapbuffer.h"

#define SGN(a) (((a)<0) ? -1 : 1)
#define INBOUNDS(x, y) \
 (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE)
#define dbg(x) DebugLog((DebugLevel)(x),D_MAP) << __FILE__ << ":" << __LINE__ << ": "

static std::list<item>  nulitems;          // Returned when &i_at() is asked for an OOB value
static field            nulfield;          // Returned when &field_at() is asked for an OOB value
static int              null_temperature;  // Because radiation does it too
static level_cache      nullcache;         // Dummy cache for z-levels outside bounds
static item             nulitem;           // Returned when item adding functions fail to add an item

// Less for performance and more so that it's visible for when ter_t gets its string_id
static std::string null_ter_t = "t_null";

// Map stack methods.
size_t map_stack::size() const
{
    return mystack->size();
}

bool map_stack::empty() const
{
    return mystack->empty();
}

std::list<item>::iterator map_stack::erase( std::list<item>::iterator it )
{
    return myorigin->i_rem(location, it);
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

std::list<item>::iterator map_stack::begin()
{
    return mystack->begin();
}

std::list<item>::iterator map_stack::end()
{
    return mystack->end();
}

std::list<item>::const_iterator map_stack::begin() const
{
    return mystack->cbegin();
}

std::list<item>::const_iterator map_stack::end() const
{
    return mystack->cend();
}

std::list<item>::reverse_iterator map_stack::rbegin()
{
    return mystack->rbegin();
}

std::list<item>::reverse_iterator map_stack::rend()
{
    return mystack->rend();
}

std::list<item>::const_reverse_iterator map_stack::rbegin() const
{
    return mystack->crbegin();
}

std::list<item>::const_reverse_iterator map_stack::rend() const
{
    return mystack->crend();
}

item &map_stack::front()
{
    return mystack->front();
}

item &map_stack::operator[]( size_t index )
{
    return *(std::next(mystack->begin(), index));
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

    dbg(D_INFO) << "map::map(): my_MAPSIZE: " << my_MAPSIZE << " zlevels enabled:" << zlevels;
    traplocs.resize( trap::count() );
}

map::~map()
{
}

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
    int lx, ly;
    submap *const sm = get_submap_at( p, lx, ly );

    return maptile( sm, lx, ly );
}

maptile map::maptile_at_internal( const tripoint &p )
{
    int lx, ly;
    submap *const sm = get_submap_at( p, lx, ly );

    return maptile( sm, lx, ly );
}

// Vehicle functions

VehicleList map::get_vehicles() {
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
    for( const auto & elem : ch.vehicle_list ) {
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
    const tripoint gpos = veh->global_pos3();
    int partid = 0;
    for( std::vector<vehicle_part>::iterator it = parts.begin(),
         end = parts.end(); it != end; ++it, ++partid ) {
        if( it->removed ) {
            continue;
        }
        const tripoint p = gpos + it->precalc[0];
        ch.veh_cached_parts.insert( std::make_pair( p,
                                    std::make_pair( veh, partid ) ) );
        if( inbounds( p.x, p.y ) ) {
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
            const auto &p = it->first;
            if( inbounds( p.x, p.y ) ) {
                ch.veh_exists_at[p.x][p.y] = false;
            }
            ch.veh_cached_parts.erase( it++ );
            // If something was resting on veh, drop it
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
}

void map::update_vehicle_list( submap *const to, const int zlev )
{
    // Update vehicle data
    auto &ch = get_cache( zlev );
    for( auto & elem : to->vehicles ) {
        ch.vehicle_list.insert( elem );
    }
}

void map::destroy_vehicle (vehicle *veh)
{
    if( veh == nullptr ) {
        debugmsg("map::destroy_vehicle was passed NULL");
        return;
    }

    if( veh->smz < -OVERMAP_DEPTH && veh->smz > OVERMAP_HEIGHT ) {
        debugmsg( "destroy_vehicle got a vehicle outside allowed z-level range! name=%s, submap:%d,%d,%d",
                  veh->name.c_str(), veh->smx, veh->smy, veh->smz );
        // Try to fix by moving the vehicle here
        veh->smz = abs_sub.z;
    }

    submap * const current_submap = get_submap_at_grid(veh->smx, veh->smy, veh->smz);
    auto &ch = get_cache( veh->smz );
    for (size_t i = 0; i < current_submap->vehicles.size(); i++) {
        if (current_submap->vehicles[i] == veh) {
            const int zlev = veh->smz;
            ch.vehicle_list.erase(veh);
            reset_vehicle_cache( zlev );
            current_submap->vehicles.erase (current_submap->vehicles.begin() + i);
            if( veh->tracking_on ) {
                overmap_buffer.remove_vehicle( veh );
            }
            delete veh;
            return;
        }
    }
    debugmsg( "destroy_vehicle can't find it! name=%s, submap:%d,%d,%d", veh->name.c_str(), veh->smx, veh->smy, veh->smz );
}

void map::on_vehicle_moved( const int smz ) {
    set_outside_cache_dirty( smz );
    set_transparency_cache_dirty( smz );
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
    // But 15 is too low for V12 deathbikes, let's put 100 here
    for( int count = 0; count < 100; count++ ) {
        if( !vehproceed() ) {
            break;
        }
    }
    // Process item removal on the vehicles that were modified this turn.
    for( const auto &elem : dirty_vehicle_list ) {
        ( elem )->part_removal_cleanup();
    }
    dirty_vehicle_list.clear();
}

bool map::vehproceed()
{
    VehicleList vehs = get_vehicles();
    vehicle* cur_veh = nullptr;
    float max_of_turn = 0;
    tripoint pt;
    // First horizontal movement
    for( auto &vehs_v : vehs ) {
        if( vehs_v.v->of_turn > max_of_turn ) {
            cur_veh = vehs_v.v;
            pt = tripoint( vehs_v.x, vehs_v.y, vehs_v.z );
            max_of_turn = cur_veh->of_turn;
        }
    }

    // Then vertical-only movement
    if( cur_veh == nullptr ) {
        for( auto &vehs_v : vehs ) {
            vehicle &cveh = *vehs_v.v;
            if( cveh.falling ) {
                cur_veh = vehs_v.v;
                pt = tripoint( vehs_v.x, vehs_v.y, vehs_v.z );
                break;
            }
        }
    }

    if( cur_veh == nullptr ) {
        return false;
    }

    vehicle &veh = *cur_veh;
    if( !inbounds( pt ) ) {
        dbg( D_INFO ) << "stopping out-of-map vehicle. (x,y,z)=(" << pt.x << "," << pt.y << "," << pt.z << ")";
        veh.stop();
        veh.of_turn = 0;
        veh.falling = false;
        return true;
    }

    // It needs to fall when it has no support OR was falling before
    //  so that vertical collisions happen.
    const bool should_fall = veh.falling &&
        ( veh.vertical_velocity != 0 || vehicle_falling( veh ) );
    const bool pl_ctrl = veh.player_in_control( g->u );

    // TODO: Saner diagonal movement, so that you can jump off cliffs properly
    // The ratio of vertical to horizontal movement should be vertical_velocity/velocity
    //  for as long as of_turn doesn't run out.
    if( should_fall ) {
        const float tile_height = 4; // 4 meters
        const float g = 9.8f; // 9.8 m/s^2
        // Convert from 100*mph to m/s
        const float old_vel = veh.vertical_velocity / 2.23694 / 100;
        // Formula is v_2 = sqrt( 2*d*g + v_1^2 )
        // Note: That drops the sign
        const float new_vel = -sqrt( 2 * tile_height * g +
                                     old_vel * old_vel );
        veh.vertical_velocity = new_vel * 2.23694 * 100;
    } else {
        // Not actually falling, was just marked for fall test
        veh.falling = false;
    }

    // Mph lost per tile when coasting
    int base_slowdown = veh.skidding ? 200 : 20;
    if( should_fall ) {
        // Just air resistance
        base_slowdown = 2;
    }

    // k slowdown second.
    const float k_slowdown = (0.1 + veh.k_dynamics()) / ((0.1) + veh.k_mass());
    const int slowdown = veh.drag() + (int)ceil( k_slowdown * base_slowdown );
    if( slowdown > abs( veh.velocity ) ) {
        veh.stop();
    } else if( veh.velocity < 0 ) {
        veh.velocity += slowdown;
    } else {
        veh.velocity -= slowdown;
    }

    // Low enough for bicycles to go in reverse.
    if( !should_fall && abs( veh.velocity ) < 20 ) {
        veh.stop();
    }

    if( !should_fall && abs( veh.velocity ) < 20 ) {
        veh.of_turn -= .321f;
        return true;
    }

    const float traction = vehicle_traction( veh );
    // TODO: Remove this hack, have vehicle sink a z-level
    if( traction < 0 ) {
        add_msg(m_bad, _("Your %s sank."), veh.name.c_str());
        if( pl_ctrl ) {
            veh.unboard_all();
        }
        if( g->remoteveh() == &veh ) {
            g->setremoteveh( nullptr );
        }

        on_vehicle_moved( veh.smz );
        // Destroy vehicle (sank to nowhere)
        destroy_vehicle( &veh );
        return true;
    } else if( traction < 0.001f ) {
        veh.of_turn = 0;
        if( !should_fall ) {
            veh.stop();
            // TODO: Remove this hack
            // TODO: Amphibious vehicles
            if( veh.floating.empty() ) {
                add_msg(m_info, _("Your %s can't move on this terrain."), veh.name.c_str());
            } else {
                add_msg(m_info, _("Your %s is beached."), veh.name.c_str());
            }
        }
    }
    // One-tile step take some of movement
    //  terrain cost is 1000 on roads.
    // This is stupid btw, it makes veh magically seem
    //  to accelerate when exiting rubble areas.
    // TODO: Replicate this in vehicle::thrust and above in slowdown
    const float ter_turn_cost = 1000.0f / std::max<float>( 0.0001f, traction * abs( veh.velocity ) );

    // Can't afford it this turn?
    // Low speed shouldn't prevent vehicle from falling, though
    bool falling_only = false;
    if( ter_turn_cost >= veh.of_turn ) {
        if( !should_fall ) {
            veh.of_turn_carry = veh.of_turn;
            veh.of_turn = 0;
            return true;
        }

        falling_only = true;
    }

    // Decrease of_turn if falling+moving, but not when it's lower than move cost
    if( !falling_only ) {
        veh.of_turn -= ter_turn_cost;
    }

    if( veh.skidding ) {
        if( one_in( 4 ) ) { // might turn uncontrollably while skidding
            veh.turn( one_in( 2 ) ? -15 : 15 );
        }
    ///\EFFECT_DRIVING reduces chance of fumbling vehicle controls
    } else if( !should_fall && pl_ctrl && rng(0, 4) > g->u.skillLevel( skill_driving ) && one_in(20) ) {
        add_msg( m_warning, _("You fumble with the %s's controls."), veh.name.c_str() );
        veh.turn( one_in( 2 ) ? -15 : 15 );
    }
    // Eventually send it skidding if no control
    // But not if it's remotely controlled
    if( !pl_ctrl && veh.boarded_parts().empty() && one_in(10) ) {
        veh.skidding = true;
    }

    if( should_fall ) {
        // TODO: Insert a (hard) driving test to stop this from happening
        veh.skidding = true;
    }

    // Where do we go
    tileray mdir; // The direction we're moving
    if( veh.skidding || should_fall ) {
        // If skidding, it's the move vector
        // Same for falling - no air control
        mdir = veh.move;
    } else if( veh.turn_dir != veh.face.dir() ) {
        // Driver turned vehicle, get turn_dir
        mdir.init( veh.turn_dir );
    } else {
        // Not turning, keep face.dir
        mdir = veh.face;
    }

    tripoint dp;
    if( abs( veh.velocity ) >= 20 && !falling_only ) {
        mdir.advance( veh.velocity < 0 ? -1 : 1 );
        dp.x = mdir.dx();
        dp.y = mdir.dy();
    }

    if( should_fall ) {
        dp.z = -1;
    }

    // Split the movement into horizontal and vertical for easier processing
    if( dp.x != 0 || dp.y != 0 ) {
        move_vehicle( veh, tripoint( dp.x, dp.y, 0 ), mdir );
    }

    if( dp.z != 0 ) {
        move_vehicle( veh, tripoint( 0, 0, dp.z ), mdir );
    }

    return true;
}

bool map::vehicle_falling( vehicle &veh )
{
    if( !zlevels ) {
        return false;
    }

    // TODO: Make the vehicle "slide" towards its center of weight
    //  when it's not properly supported
    const auto &pts = veh.get_points( true );
    for( const tripoint &p : pts ) {
        if( has_floor( p ) ) {
            return false;
        }

        tripoint below( p.x, p.y, p.z - 1 );
        if( p.z <= -OVERMAP_DEPTH || supports_above( below ) ) {
            return false;
        }
    }

    if( pts.empty() ) {
        // Dirty vehicle with no parts
        return false;
    }

    return true;
}

float map::vehicle_traction( vehicle &veh ) const
{
    const tripoint pt = veh.global_pos3();
    // TODO: Remove this and allow amphibious vehicles
    if( !veh.floating.empty() ) {
        return vehicle_buoyancy( veh );
    }

    // Sink in water?
    const auto &wheel_indices = veh.wheelcache;
    int num_wheels = wheel_indices.size();
    if( num_wheels == 0 ) {
        // TODO: Assume it is digging in dirt
        // TODO: Return something that could be reused for dragging
        return 1.0f;
    }

    int submerged_wheels = 0;
    float traction_wheel_area = 0;
    float total_wheel_area = 0;
    for( int w = 0; w < num_wheels; w++ ) {
        const int p = wheel_indices[w];
        const tripoint pp = pt + veh.parts[p].precalc[0];

        // Not using vehicle::wheels_area so that if it changes,
        //  this section will stay correct (ie. proportion of wheel area/total area)
        const int width = veh.part_info( p ).wheel_width;
        const int bigness = veh.parts[p].bigness;
        const float wheel_area = width * bigness / 9.0f;
        total_wheel_area += wheel_area;

        const auto &tr = ter_at( pp );
        // Deep water and air
        if( tr.has_flag( TFLAG_DEEP_WATER ) ) {
            submerged_wheels++;
            // No traction from wheel in water
            continue;
        } else if( tr.has_flag( TFLAG_NO_FLOOR ) ) {
            // Ditto for air, but with no submerging
            continue;
        }

        const int move_mod = move_cost_ter_furn( pp );
        if( move_mod == 0 ) {
            // Vehicle locked in wall
            // Shouldn't happen, but does
            return 0.0f;
        } else {
            traction_wheel_area += 2 * wheel_area / move_mod;
        }
    }

    // Submerged wheels threshold is 2/3.
    if( num_wheels > 0 && (float)submerged_wheels / num_wheels > .666) {
        return -1;
    }

    if( total_wheel_area <= 0.01f ) {
        debugmsg( "%s has wheel area <0.01", veh.name.c_str() );
        return 0.0f;
    }

    return traction_wheel_area / total_wheel_area;
}

float map::vehicle_buoyancy( vehicle &veh ) const
{
    const tripoint pt = veh.global_pos3();
    const auto &float_indices = veh.floating;
    const int num = float_indices.size();
    int moored = 0;
    for( int w = 0; w < num; w++ ) {
        const int p = float_indices[w];
        const tripoint pp = pt + veh.parts[p].precalc[0];

        if( !has_flag( "SWIMMABLE", pp ) ) {
            moored++;
        }
    }

    if( moored > num - 1 ) {
        return 0.0f;
    }

    // TODO: Actually implement buoyancy
    return 1.0f;
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

    tripoint pt = veh.global_pos3();
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
    do
    {
        collisions.clear();
        veh.collision( collisions, dp1, false );

        // Vehicle collisions
        std::map<vehicle*, std::vector<veh_collision> > veh_collisions;
        for( auto &coll : collisions ) {
            if( coll.type != veh_coll_veh ) {
                continue;
            }

            veh_veh_coll_flag = true;
            // Only collide with each vehicle once
            veh_collisions[ static_cast<vehicle*>( coll.target ) ].push_back( coll );
        }

        for( auto &pair : veh_collisions ) {
            impulse += vehicle_vehicle_collision( veh, *pair.first, pair.second );
        }

        // Non-vehicle collisions
        for( const auto &coll : collisions ) {
            if( coll.type == veh_coll_veh ) {
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
             sgn(coll_velocity) == sgn(velocity_before) &&
             !collisions.empty() && !veh_veh_coll_flag );

    if( vertical && !collisions.empty() ) {
        // A big hack, should be removed when possible
        veh.vertical_velocity = 0;
    }

    const int velocity_after = coll_velocity;
    const bool can_move = velocity_after != 0 && sgn(velocity_after) == sgn(velocity_before);

    int coll_turn = 0;
    if( impulse > 0 ) {
        coll_turn = shake_vehicle( veh, velocity_before, facing.dir() );
        const int volume = std::min<int>( 100, sqrtf( impulse ) );
        // TODO: Center the sound at weighted (by impulse) average of collisions
        sounds::sound( veh.global_pos3(), volume, _("crash!"), false, "smash_success", "hit_vehicle" );
    }

    if( veh_veh_coll_flag ) {
        // Break here to let the hit vehicle move away
        return;
    }

    // If not enough wheels, mess up the ground a bit.
    if( !vertical && !veh.valid_wheel_config() ) {
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
        const auto &wheel_indices = veh.wheelcache;
        for( auto &w : wheel_indices ) {
            const tripoint wheel_p = pt + veh.parts[w].precalc[0];
            if( one_in( 2 ) && displace_water( wheel_p ) ) {
                sounds::sound( wheel_p, 4, _("splash!"), false, "environment", "splash");
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
        displace_vehicle( pt, dp1 );
    } else if( !vertical ) {
        veh.stop();
    }
    // If the PC is in the currently moved vehicle, adjust the
    //  view offset.
    if( g->u.controlling_vehicle && veh_at( g->u.pos() ) == &veh ) {
        g->calc_driving_offset( &veh );
        if( veh.skidding && can_move ) {
            // TODO: Make skid recovery in air hard
            veh.possibly_recover_from_skid();
        }
    }
    // Redraw scene
    // TODO: Make this not happen on unseen vehicles
    g->draw();
}

int map::shake_vehicle( vehicle &veh, const int velocity_before, const int direction )
{
    const tripoint &pt = veh.global_pos3();
    const int d_vel = abs( veh.velocity - velocity_before ) / 100;

    const std::vector<int> boarded = veh.boarded_parts();

    int coll_turn = 0;
    for( const auto &ps : boarded ) {
        player *psg = veh.get_passenger( ps );
        if( psg == nullptr ) {
            debugmsg( "throw passenger: empty passenger at part %d", ps );
            continue;
        }

        const tripoint part_pos = pt + veh.parts[ps].precalc[0];
        if( psg->pos() != part_pos ) {
            debugmsg( "throw passenger: passenger at %d,%d,%d, part at %d,%d,%d",
                psg->posx(), psg->posy(), psg->posz(), part_pos.x, part_pos.y, part_pos.z );
            veh.parts[ps].remove_flag( vehicle_part::passenger_flag );
            continue;
        }

        bool throw_from_seat = false;
        if( veh.part_with_feature( ps, VPFLAG_SEATBELT ) == -1 ) {
            ///\EFFECT_STR reduces chance of being thrown from your seat when not wearing a seatbelt
            throw_from_seat = d_vel * rng( 80, 120 ) / 100 > ( psg->str_cur * 1.5 + 5 );
        }

        // Damage passengers if d_vel is too high
        if( d_vel > 60 * rng( 50,100 ) / 100 && !throw_from_seat ) {
            const int dmg = d_vel / 4 * rng( 70,100 ) / 100;
            psg->hurtall( dmg, nullptr );
            psg->add_msg_player_or_npc( m_bad,
                _("You take %d damage by the power of the impact!"),
                _("<npcname> takes %d damage by the power of the impact!"),  dmg );
        }

        if( veh.player_in_control( *psg ) ) {
            const int lose_ctrl_roll = rng( 0, d_vel );
            ///\EFFECT_DEX reduces chance of losing control of vehicle when shaken

            ///\EFFECT_DRIVING reduces chance of losing control of vehicle when shaken
            if( lose_ctrl_roll > psg->dex_cur * 2 + psg->skillLevel( skill_driving ) * 3 ) {
                psg->add_msg_player_or_npc( m_warning,
                    _("You lose control of the %s."),
                    _("<npcname> loses control of the %s."),
                    veh.name.c_str() );
                int turn_amount = (rng(1, 3) * sqrt((double)abs( veh.velocity ) ) / 2) / 15;
                if( turn_amount < 1 ) {
                    turn_amount = 1;
                }
                turn_amount *= 15;
                if( turn_amount > 120 ) {
                    turn_amount = 120;
                }
                coll_turn = one_in( 2 ) ? turn_amount : -turn_amount;
            }
        }

        if( throw_from_seat ) {
            psg->add_msg_player_or_npc(m_bad,
                _("You are hurled from the %s's seat by the power of the impact!"),
                _("<npcname> is hurled from the %s's seat by the power of the impact!"),
                veh.name.c_str());
            unboard_vehicle( part_pos );
            ///\EFFECT_STR reduces distance thrown from seat in a vehicle impact
            g->fling_creature(psg, direction + rng(0, 60) - 30,
                ( d_vel - psg->str_cur < 10 ) ? 10 : d_vel - psg->str_cur );
        }
    }

    return coll_turn;
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
    add_msg(m_bad, _("The %1$s's %2$s collides with %3$s's %4$s."),
                   veh.name.c_str(),  veh.part_info(c.part).name.c_str(),
                   veh2.name.c_str(), veh2.part_info(c.target_part).name.c_str());

    const bool vertical = veh.smz != veh2.smz;

    // Used to calculate the epicenter of the collision.
    point epicenter1(0, 0);
    point epicenter2(0, 0);

    float dmg;
    // Vertical collisions will be simpler for a while (1D)
    if( !vertical ) {
        // For reference, a cargo truck weighs ~25300, a bicycle 690,
        //  and 38mph is 3800 'velocity'
        rl_vec2d velo_veh1 = veh.velo_vec();
        rl_vec2d velo_veh2 = veh2.velo_vec();
        const float m1 = veh.total_mass();
        const float m2 = veh2.total_mass();
        //Energy of vehicle1 annd vehicle2 before collision
        float E = 0.5 * m1 * velo_veh1.norm() * velo_veh1.norm() +
            0.5 * m2 * velo_veh2.norm() * velo_veh2.norm();

        // Collision_axis
        int x_cof1 = 0, y_cof1 = 0, x_cof2 = 0, y_cof2 = 0;
        veh .center_of_mass(x_cof1, y_cof1);
        veh2.center_of_mass(x_cof2, y_cof2);
        rl_vec2d collision_axis_y;

        collision_axis_y.x = ( veh.global_x() + x_cof1 ) - ( veh2.global_x() + x_cof2 );
        collision_axis_y.y = ( veh.global_y() + y_cof1 ) - ( veh2.global_y() + y_cof2 );
        collision_axis_y = collision_axis_y.normalized();
        rl_vec2d collision_axis_x = collision_axis_y.get_vertical();
        // imp? & delta? & final? reworked:
        // newvel1 =( vel1 * ( mass1 - mass2 ) + ( 2 * mass2 * vel2 ) ) / ( mass1 + mass2 )
        // as per http://en.wikipedia.org/wiki/Elastic_collision
        //velocity of veh1 before collision in the direction of collision_axis_y
        float vel1_y = collision_axis_y.dot_product(velo_veh1);
        float vel1_x = collision_axis_x.dot_product(velo_veh1);
        //velocity of veh2 before collision in the direction of collision_axis_y
        float vel2_y = collision_axis_y.dot_product(velo_veh2);
        float vel2_x = collision_axis_x.dot_product(velo_veh2);
        // e = 0 -> inelastic collision
        // e = 1 -> elastic collision
        float e = get_collision_factor(vel1_y/100 - vel2_y/100);

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
        veh.velocity = final1.norm();

        veh2.move.init( final2.x, final2.y );
        veh2.velocity = final2.norm();
        //give veh2 the initiative to proceed next before veh1
        float avg_of_turn = (veh2.of_turn + veh.of_turn) / 2;
        if( avg_of_turn < .1f ) {
            avg_of_turn = .1f;
        }

        veh.of_turn = avg_of_turn * .9;
        veh2.of_turn = avg_of_turn * 1.1;

        //Energy after collision
        float E_a = 0.5 * m1 * final1.norm() * final1.norm() +
            0.5 * m2 * final2.norm() * final2.norm();
        float d_E = E - E_a;  //Lost energy at collision -> deformation energy
        dmg = std::abs( d_E / 1000 / 2000 );  //adjust to balance damage
    } else {
        const float m1 = veh.total_mass();
        // Collision is perfectly inelastic for simplicity
        // Assume veh2 is standing still
        dmg = abs(veh.vertical_velocity / 100) * m1 / 10;
        veh.vertical_velocity = 0;
    }

    float dmg_veh1 = dmg * 0.5;
    float dmg_veh2 = dmg * 0.5;

    int coll_parts_cnt = 0; //quantity of colliding parts between veh1 and veh2
    for( const auto &veh_veh_coll : collisions ) {
        if( &veh2 == (vehicle*)veh_veh_coll.target ) {
            coll_parts_cnt++;
        }
    }

    const float dmg1_part = dmg_veh1 / coll_parts_cnt;
    const float dmg2_part = dmg_veh2 / coll_parts_cnt;

    //damage colliding parts (only veh1 and veh2 parts)
    for( const auto &veh_veh_coll : collisions ) {
        if( &veh2 != (vehicle*)veh_veh_coll.target ) {
            continue;
        }

        int parm1 = veh.part_with_feature (veh_veh_coll.part, VPFLAG_ARMOR);
        if( parm1 < 0 ) {
            parm1 = veh_veh_coll.part;
        }
        int parm2 = veh2.part_with_feature (veh_veh_coll.target_part, VPFLAG_ARMOR);
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
        // Shake veh because of collision
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

// 3D vehicle functions

VehicleList map::get_vehicles( const tripoint &start, const tripoint &end )
{
    const int chunk_sx = std::max( 0, (start.x / SEEX) - 1 );
    const int chunk_ex = std::min( my_MAPSIZE - 1, (end.x / SEEX) + 1 );
    const int chunk_sy = std::max( 0, (start.y / SEEY) - 1 );
    const int chunk_ey = std::min( my_MAPSIZE - 1, (end.y / SEEY) + 1 );
    const int chunk_sz = start.z;
    const int chunk_ez = end.z;
    VehicleList vehs;

    for( int cx = chunk_sx; cx <= chunk_ex; ++cx ) {
        for( int cy = chunk_sy; cy <= chunk_ey; ++cy ) {
            for( int cz = chunk_sz; cz <= chunk_ez; ++cz ) {
                submap *current_submap = get_submap_at_grid( cx, cy, cz );
                for( auto &elem : current_submap->vehicles ) {
                    // Ensure the veh's z-position is correct
                    elem->smz = cz;
                    wrapped_vehicle w;
                    w.v = elem;
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

vehicle* map::veh_at( const tripoint &p, int &part_num )
{
    if( !get_cache( p.z ).veh_in_active_range || !inbounds( p ) ) {
        return nullptr; // Out-of-bounds - null vehicle
    }

    return veh_at_internal( p, part_num );
}

const vehicle* map::veh_at( const tripoint &p, int &part_num ) const
{
    if( !get_cache_ref( p.z ).veh_in_active_range || !inbounds( p ) ) {
        return nullptr; // Out-of-bounds - null vehicle
    }

    return veh_at_internal( p, part_num );
}

const vehicle* map::veh_at_internal( const tripoint &p, int &part_num ) const
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

vehicle* map::veh_at_internal( const tripoint &p, int &part_num )
{
    return const_cast<vehicle *>( const_cast<const map*>(this)->veh_at_internal( p, part_num ) );
}

vehicle* map::veh_at( const tripoint &p )
{
    int part = 0;
    return veh_at( p, part );
}

const vehicle* map::veh_at( const tripoint &p ) const
{
    int part = 0;
    return veh_at( p, part );
}

point map::veh_part_coordinates( const tripoint &p )
{
    int part_num = 0;
    vehicle* veh = veh_at( p, part_num );

    if(veh == nullptr) {
        return point( 0,0 );
    }

    return veh->parts[part_num].mount;
}

void map::board_vehicle( const tripoint &pos, player *p )
{
    if( p == nullptr ) {
        debugmsg ("map::board_vehicle: null player");
        return;
    }

    int part = 0;
    vehicle *veh = veh_at( pos, part );
    if( veh == nullptr ) {
        if( p->grab_point.x == 0 && p->grab_point.y == 0 ) {
            debugmsg ("map::board_vehicle: vehicle not found");
        }
        return;
    }

    const int seat_part = veh->part_with_feature( part, VPFLAG_BOARDABLE );
    if( seat_part < 0 ) {
        debugmsg( "map::board_vehicle: boarding %s (not boardable)",
                  veh->part_info(part).name.c_str() );
        return;
    }
    if( veh->parts[seat_part].has_flag( vehicle_part::passenger_flag ) ) {
        player *psg = veh->get_passenger( seat_part );
        debugmsg( "map::board_vehicle: passenger (%s) is already there",
                  psg ? psg->name.c_str() : "<null>" );
        unboard_vehicle( pos );
    }
    veh->parts[seat_part].set_flag(vehicle_part::passenger_flag);
    veh->parts[seat_part].passenger_id = p->getID();

    p->setpos( pos );
    p->in_vehicle = true;
    if( p == &g->u ) {
        g->update_map( &g->u );
    }
}

void map::unboard_vehicle( const tripoint &p )
{
    int part = 0;
    vehicle *veh = veh_at( p, part );
    player *passenger = nullptr;
    if( !veh ) {
        debugmsg ("map::unboard_vehicle: vehicle not found");
        // Try and force unboard the player anyway.
        if( g->u.pos3() == p ) {
            passenger = &(g->u);
        } else {
            int npcdex = g->npc_at( p );
            if( npcdex != -1 ) {
                passenger = g->active_npc[npcdex];
            }
        }
        if( passenger ) {
            passenger->in_vehicle = false;
            passenger->driving_recoil = 0;
            passenger->controlling_vehicle = false;
        }
        return;
    }
    const int seat_part = veh->part_with_feature( part, VPFLAG_BOARDABLE, false );
    if( seat_part < 0 ) {
        debugmsg ("map::unboard_vehicle: unboarding %s (not boardable)",
                  veh->part_info(part).name.c_str());
        return;
    }
    passenger = veh->get_passenger(seat_part);
    if( !passenger ) {
        debugmsg ("map::unboard_vehicle: passenger not found");
        return;
    }
    passenger->in_vehicle = false;
    passenger->driving_recoil = 0;
    passenger->controlling_vehicle = false;
    veh->parts[seat_part].remove_flag(vehicle_part::passenger_flag);
    veh->skidding = true;
}

void map::displace_vehicle( tripoint &p, const tripoint &dp )
{
    const tripoint p2 = p + dp;
    const tripoint src = p;
    const tripoint dst = p2;

    if( !inbounds( src ) ) {
        add_msg( m_debug, "map::displace_vehicle: coords out of bounds %d,%d,%d->%d,%d,%d",
                        src.x, src.y, src.z, dst.x, dst.y, dst.z );
        return;
    }

    int src_offset_x, src_offset_y, dst_offset_x, dst_offset_y;
    submap *const src_submap = get_submap_at( src, src_offset_x, src_offset_y );
    submap *const dst_submap = get_submap_at( dst, dst_offset_x, dst_offset_y );

    // first, let's find our position in current vehicles vector
    int our_i = -1;
    for( size_t i = 0; i < src_submap->vehicles.size(); i++ ) {
        if( src_submap->vehicles[i]->posx == src_offset_x &&
            src_submap->vehicles[i]->posy == src_offset_y ) {
            our_i = i;
            break;
        }
    }
    if( our_i < 0 ) {
        vehicle *v = veh_at( p );
        for( auto & smap : grid ) {
            for (size_t i = 0; i < smap->vehicles.size(); i++) {
                if (smap->vehicles[i] == v) {
                    our_i = i;
                    const_cast<submap*&>(src_submap) = smap;
                    break;
                }
            }
        }
    }
    if( our_i < 0 ) {
        add_msg( m_debug, "displace_vehicle our_i=%d", our_i );
        return;
    }
    // move the vehicle
    vehicle *veh = src_submap->vehicles[our_i];
    // don't let it go off grid
    if( !inbounds( p2 ) ) {
        veh->stop();
        // Silent debug
        dbg(D_ERROR) << "map:displace_vehicle: Stopping vehicle, displaced dp=("
                     << dp.x << ", " << dp.y << ", " << dp.z << ")";
        return;
    }

    // Need old coords to check for remote control
    const bool remote = veh->remote_controlled( g->u );

    // record every passenger inside
    std::vector<int> psg_parts = veh->boarded_parts();
    std::vector<player *> psgs;
    for( auto &prt : psg_parts ) {
        psgs.push_back( veh->get_passenger( prt ) );
    }

    const int rec = abs( veh->velocity ) * 3 / 100;

    bool need_update = false;
    int z_change = 0;
    // Move passengers
    const tripoint old_veh_pos = veh->global_pos3();
    for( size_t i = 0; i < psg_parts.size(); i++ ) {
        player *psg = psgs[i];
        const int prt = psg_parts[i];
        const tripoint part_pos = old_veh_pos + veh->parts[prt].precalc[0];
        if( psg == nullptr ) {
            debugmsg( "Empty passenger part %d pcoord=%d,%d,%d u=%d,%d,%d?",
                prt,
                part_pos.x, part_pos.y, part_pos.z,
                g->u.posx(), g->u.posy(), g->u.posz() );
            veh->parts[prt].remove_flag(vehicle_part::passenger_flag);
            continue;
        }

        if( psg->pos() != part_pos ) {
            debugmsg( "Passenger/part position mismatch: passenger %d,%d,%d, part %d %d,%d,%d",
                g->u.posx(), g->u.posy(), g->u.posz(),
                prt,
                part_pos.x, part_pos.y, part_pos.z );
            veh->parts[prt].remove_flag(vehicle_part::passenger_flag);
            continue;
        }

        // Place passenger on the new part location
        tripoint psgp( part_pos.x + dp.x + veh->parts[prt].precalc[1].x - veh->parts[prt].precalc[0].x,
                       part_pos.y + dp.y + veh->parts[prt].precalc[1].y - veh->parts[prt].precalc[0].y,
                       psg->posz() );
        // Add recoil
        psg->driving_recoil = rec;
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

    veh->posx = dst_offset_x;
    veh->posy = dst_offset_y;
    veh->smz = p2.z;
    // Invalidate vehicle's point cache
    veh->occupied_cache_turn = -1;
    if( src_submap != dst_submap ) {
        veh->set_submap_moved( int( p2.x / SEEX ), int( p2.y / SEEY ) );
        dst_submap->vehicles.push_back( veh );
        src_submap->vehicles.erase( src_submap->vehicles.begin() + our_i );
        dst_submap->is_uniform = false;
    }

    p = p2;

    update_vehicle_cache( veh, src.z );

    if( need_update ) {
        g->update_map( &g->u );
    }

    if( z_change != 0 ) {
        g->vertical_move( z_change, true );
    }

    if( remote ) {
        // Has to be after update_map or coords won't be valid
        g->setremoteveh( veh );
    }

    if( !veh->falling ) {
        veh->falling = vehicle_falling( *veh );
    }

    on_vehicle_moved( veh->smz );
}

bool map::displace_water( const tripoint &p )
{
    // Check for shallow water
    if( has_flag( "SWIMMABLE", p ) && !has_flag( TFLAG_DEEP_WATER, p ) ) {
        int dis_places = 0, sel_place = 0;
        for( int pass = 0; pass < 2; pass++ ) {
            // we do 2 passes.
            // first, count how many non-water places around
            // then choose one within count and fill it with water on second pass
            if( pass != 0 ) {
                sel_place = rng( 0, dis_places - 1 );
                dis_places = 0;
            }
            tripoint temp( p );
            int &tx = temp.x;
            int &ty = temp.y;
            for( tx = p.x - 1; tx <= p.x + 1; tx++ ) {
                for( ty = p.y -1; ty <= p.y + 1; ty++ ) {
                    if( ( tx != p.x && ty != p.y )
                            || move_cost_ter_furn( temp ) == 0
                            || has_flag( TFLAG_DEEP_WATER, temp ) ) {
                        continue;
                    }
                    ter_id ter0 = ter( temp );
                    if( ter0 == t_water_sh ||
                        ter0 == t_water_dp) {
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
    }
    return false;
}

// End of 3D vehicle

// 2D overloads for furniture
// To be removed once not needed
void map::set(const int x, const int y, const ter_id new_terrain, const furn_id new_furniture)
{
    furn_set(x, y, new_furniture);
    ter_set(x, y, new_terrain);
}

void map::set(const int x, const int y, const std::string new_terrain, const std::string new_furniture) {
    furn_set(x, y, new_furniture);
    ter_set(x, y, new_terrain);
}

std::string map::name(const int x, const int y)
{
    return name( tripoint( x, y, abs_sub.z ) );
}

bool map::has_furn(const int x, const int y) const
{
  return furn(x, y) != f_null;
}

std::string map::get_furn(const int x, const int y) const
{
    return furn_at(x, y).id;
}

const furn_t & map::furn_at(const int x, const int y) const
{
    return furn(x,y).obj();
}

furn_id map::furn(const int x, const int y) const
{
    if( !INBOUNDS(x, y) ) {
        return f_null;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return current_submap->get_furn(lx, ly);
}

void map::furn_set(const int x, const int y, const furn_id new_furniture)
{
    furn_set( tripoint( x, y, abs_sub.z ), new_furniture );
}

void map::furn_set(const int x, const int y, const std::string new_furniture) {
    if ( furnmap.find(new_furniture) == furnmap.end() ) {
        return;
    }
    furn_set(x, y, furnmap[ new_furniture ].loadid );
}

std::string map::furnname(const int x, const int y) {
 return furn_at(x, y).name;
}
// End of 2D overloads for furniture

void map::set( const tripoint &p, const ter_id new_terrain, const furn_id new_furniture)
{
    furn_set( p, new_furniture );
    ter_set( p, new_terrain );
}

void map::set( const tripoint &p, const std::string new_terrain, const std::string new_furniture) {
    furn_set( p, new_furniture );
    ter_set( p, new_terrain );
}

std::string map::name( const tripoint &p )
{
    return has_furn( p ) ? furnname( p ) : tername( p );
}

std::string map::disp_name( const tripoint &p )
{
    return string_format( _("the %s"), name( p ).c_str() );
}

bool map::has_furn( const tripoint &p ) const
{
  return furn( p ) != f_null;
}

std::string map::get_furn( const tripoint &p ) const
{
    return furn_at( p ).id;
}

const furn_t & map::furn_at( const tripoint &p ) const
{
    return furn( p ).obj();
}

furn_id map::furn( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return f_null;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->get_furn( lx, ly );
}

void map::furn_set( const tripoint &p, const furn_id new_furniture )
{
    if( !inbounds( p ) ) {
        return;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    const furn_id old_id = current_submap->get_furn( lx, ly );
    if( old_id == new_furniture ) {
        // Nothing changed
        return;
    }

    current_submap->set_furn( lx, ly, new_furniture );

    // Set the dirty flags
    const furn_t &old_t = old_id.obj();
    const furn_t &new_t = new_furniture.obj();

    if( old_t.transparent != new_t.transparent ) {
        set_transparency_cache_dirty( p.z );
    }

    if( old_t.has_flag( TFLAG_INDOORS ) != new_t.has_flag( TFLAG_INDOORS ) ) {
        set_outside_cache_dirty( p.z );
    }

    // Make sure the furniture falls if it needs to
    support_dirty( p );
    tripoint above( p.x, p.y, p.z + 1 );
    // Make sure that if we supported something and no longer do so, it falls down
    support_dirty( above );
}

void map::furn_set( const tripoint &p, const std::string new_furniture) {
    if( furnmap.find(new_furniture) == furnmap.end() ) {
        return;
    }

    furn_set( p, furnmap[ new_furniture ].loadid );
}

bool map::can_move_furniture( const tripoint &pos, player *p ) {
    furn_t furniture_type = furn_at( pos );
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

std::string map::furnname( const tripoint &p ) {
    const furn_t &f = furn_at( p );
    if( f.has_flag( "PLANT" ) && !i_at( p ).empty() ) {
        const item &seed = i_at( p ).front();
        const std::string &plant = seed.get_plant_name();
        return string_format( "%s (%s)", f.name.c_str(), plant.c_str() );
    } else {
        return f.name;
    }
}

// 2D overloads for terrain
// To be removed once not needed

ter_id map::ter(const int x, const int y) const
{
    if( !INBOUNDS(x, y) ) {
        return t_null;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);
    return current_submap->get_ter( lx, ly );
}

std::string map::get_ter(const int x, const int y) const {
    return ter_at(x, y).id;
}

std::string map::get_ter_harvestable(const int x, const int y) const {
    return ter_at(x, y).harvestable;
}

ter_id map::get_ter_transforms_into(const int x, const int y) const {
    return termap[ ter_at(x, y).transforms_into ].loadid;
}

int map::get_ter_harvest_season(const int x, const int y) const {
    return ter_at(x, y).harvest_season;
}

const ter_t & map::ter_at(const int x, const int y) const
{
    return ter(x,y).obj();
}

void map::ter_set(const int x, const int y, const std::string new_terrain) {
    if ( termap.find(new_terrain) == termap.end() ) {
        return;
    }
    ter_set(x, y, termap[ new_terrain ].loadid );
}

void map::ter_set(const int x, const int y, const ter_id new_terrain) {
    ter_set( tripoint( x, y, abs_sub.z ), new_terrain );
}

std::string map::tername(const int x, const int y) const
{
 return ter_at(x, y).name;
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

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->get_ter( lx, ly );
}

/*
 * Get the terrain string id. This will remain the same across revisions,
 * unless a mod eliminates or changes it. Generally this is less efficient
 * than ter_id, but only an issue if thousands of comparisons are made.
 */
std::string map::get_ter( const tripoint &p ) const {
    return ter_at( p ).id;
}

/*
 * Get the terrain harvestable string (what will get harvested from the terrain)
 */
std::string map::get_ter_harvestable( const tripoint &p ) const {
    return ter_at( p ).harvestable;
}

/*
 * Get the terrain transforms_into id (what will the terrain transforms into)
 */
ter_id map::get_ter_transforms_into( const tripoint &p ) const {
    return termap[ ter_at( p ).transforms_into ].loadid;
}

/*
 * Get the harvest season from the terrain
 */
int map::get_ter_harvest_season( const tripoint &p ) const {
    return ter_at( p ).harvest_season;
}

/*
 * Get a reference to the actual terrain struct.
 */
const ter_t & map::ter_at( const tripoint &p ) const
{
    return ter( p ).obj();
}

/*
 * set terrain via string; this works for -any- terrain id
 */
void map::ter_set( const tripoint &p, const std::string new_terrain) {
    if( termap.find(new_terrain) == termap.end() ) {
        return;
    }

    ter_set( p, termap[ new_terrain ].loadid );
}

void map::ter_set( const tripoint &p, const ter_id new_terrain )
{
    if( !inbounds( p ) ) {
        return;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    const ter_id old_id = current_submap->get_ter( lx, ly );
    if( old_id == new_terrain ) {
        // Nothing changed
        return;
    }

    current_submap->set_ter( lx, ly, new_terrain );

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
        // It's a set, not a flag
        support_cache_dirty.insert( p );
    }

    tripoint above( p.x, p.y, p.z + 1 );
    // Make sure that if we supported something and no longer do so, it falls down
    support_dirty( above );
}

std::string map::tername( const tripoint &p ) const
{
 return ter_at( p ).name;
}

std::string map::features(const int x, const int y)
{
    return features( tripoint( x, y, abs_sub.z ) );
}

std::string map::features( const tripoint &p )
{
    // This is used in an info window that is 46 characters wide, and is expected
    // to take up one line.  So, make sure it does that.
    // FIXME: can't control length of localized text.
    // Make the caller wrap properly, if it does not already.
    std::string ret;
    if (is_bashable(p)) {
        ret += _("Smashable. ");
    }
    if (has_flag("DIGGABLE", p)) {
        ret += _("Diggable. ");
    }
    if (has_flag("ROUGH", p)) {
        ret += _("Rough. ");
    }
    if (has_flag("UNSTABLE", p)) {
        ret += _("Unstable. ");
    }
    if (has_flag("SHARP", p)) {
        ret += _("Sharp. ");
    }
    if (has_flag("FLAT", p)) {
        ret += _("Flat. ");
    }
    return ret;
}

int map::move_cost_internal(const furn_t &furniture, const ter_t &terrain, const vehicle *veh, const int vpart) const
{
    if( terrain.movecost == 0 || ( furniture.loadid != f_null && furniture.movecost < 0 ) ) {
        return 0;
    }

    if( veh != nullptr ) {
        if( veh->obstacle_at_part( vpart ) >= 0 ) {
            return 0;
        } else {
            const int ipart = veh->part_with_feature( vpart, VPFLAG_AISLE );
            if( ipart >= 0 ) {
                return 2;
            }

            return 8;
        }
    }

    if( furniture.loadid != f_null ) {
        return std::max( terrain.movecost + furniture.movecost, 0 );
    }

    return std::max( terrain.movecost, 0 );
}

// Move cost: 2D overloads

int map::move_cost(const int x, const int y, const vehicle *ignored_vehicle) const
{
    return move_cost( tripoint( x, y, abs_sub.z ), ignored_vehicle );
}

int map::move_cost_ter_furn(const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return 0;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    const int tercost = current_submap->get_ter( lx, ly ).obj().movecost;
    if ( tercost == 0 ) {
        return 0;
    }

    const int furncost =  current_submap->get_furn(lx, ly).obj().movecost;
    if ( furncost < 0 ) {
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

    int part;
    const furn_t &furniture = furn_at( p );
    const ter_t &terrain = ter_at( p );
    const vehicle *veh = veh_at( p, part );
    if( veh == ignored_vehicle ) {
        veh = nullptr;
    }

    return move_cost_internal( furniture, terrain, veh, part );
}

int map::move_cost_ter_furn( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return 0;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );

    const int tercost = current_submap->get_ter( lx, ly ).obj().movecost;
    if ( tercost == 0 ) {
        return 0;
    }

    const int furncost = current_submap->get_furn( lx, ly ).obj().movecost;
    if ( furncost < 0 ) {
        return 0;
    }

    const int cost = tercost + furncost;
    return cost > 0 ? cost : 0;
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
        return (cost1 + cost2 + modifier) * mults[match] / 2;
    }

    // Inter-z-level movement by foot (not flying)
    if( !valid_move( from, to, false ) ) {
        return 0;
    }

    // TODO: Penalize for using stairs
    return (cost1 + cost2 + modifier) * mults[match] / 2;
}

bool map::valid_move( const tripoint &from, const tripoint &to,
                      const bool bash, const bool flying ) const
{
    if( rl_dist( from, to ) != 1 || !inbounds( from ) || !inbounds( to ) ) {
        return false;
    }

    if( from.z == to.z ) {
        return bash || move_cost( to ) > 0;
    } else if( !zlevels ) {
        return false;
    }

    const bool going_up = from.z < to.z;

    const tripoint &up_p = going_up ? to : from;
    const tripoint &down_p = going_up ? from : to;
    const maptile up = maptile_at( up_p );
    const maptile down = maptile_at( down_p );

    const ter_t &up_ter = up.get_ter_t();
    const ter_t &down_ter = down.get_ter_t();

    if( up_ter.movecost == 0 || down_ter.movecost == 0 ) {
        // Unpassable tile
        return false;
    }

    if( !up_ter.has_flag( TFLAG_NO_FLOOR ) && !up_ter.has_flag( TFLAG_GOES_DOWN ) ) {
        // Can't move from up to down
        if( from.x != to.x || from.y != to.y ) {
            // Break the move into two - vertical then horizontal
            tripoint midpoint( down_p.x, down_p.y, up_p.z );
            return valid_move( down_p, midpoint, bash, flying ) &&
                   valid_move( midpoint, up_p, bash, flying );
        }
        return false;
    }

    if( !flying && !down_ter.has_flag( TFLAG_GOES_UP ) && !down_ter.has_flag( TFLAG_RAMP ) ) {
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
        if( move_cost_ter_furn( pt ) == 0 ) {
            // TODO: Non-hardcoded climbability
            best_difficulty = std::min( best_difficulty, 10 );
            blocks_movement++;
        } else {
            int part;
            const vehicle *veh = veh_at( pt, part );
            if( veh != nullptr ) {
                // Vehicle tiles are quite good for climbing
                // TODO: Penalize spiked parts?
                best_difficulty = std::min( best_difficulty, 7 );
            }
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
    return !zlevels ||
           p.z < -OVERMAP_DEPTH + 1 || p.z > OVERMAP_HEIGHT ||
           !has_flag( TFLAG_NO_FLOOR, p );
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

    if( veh_at( p ) != nullptr ) {
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
    const auto check_tile = [this]( const tripoint &pt ) {
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
        const std::string &furn_name = frn_obj.name;
        bash( current, dmg, false, false, false );
        tripoint below( current.x, current.y, current.z - 1 );
        Creature *critter = g->critter_at( below );
        if( critter == nullptr ) {
            debugmsg( "drop_furniture couldn't find creature at %d,%d,%d",
                      below.x, below.y, below.z );
            return;
        }

        critter->add_msg_player_or_npc( m_bad, _("Falling %s hits you!"),
                                               _("Falling %s hits <npcname>"),
                                        furn_name.c_str() );
        // TODO: A chance to dodge/uncanny dodge
        player *pl = dynamic_cast<player*>( critter );
        monster *mon = dynamic_cast<monster*>( critter );
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
    vehicle *veh = veh_at( p );
    if( veh == nullptr ) {
        return;
    }

    veh->falling = true;
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

bool map::has_flag(const std::string &flag, const int x, const int y) const
{
    return has_flag_ter_or_furn(flag, x, y); // Does bound checking
}

bool map::can_put_items(const int x, const int y)
{
    return !has_flag("NOITEM", x, y) && !has_flag("SEALED", x, y);
}

bool map::has_flag_ter(const std::string & flag, const int x, const int y) const
{
 return ter_at(x, y).has_flag(flag);
}

bool map::has_flag_furn(const std::string & flag, const int x, const int y) const
{
 return furn_at(x, y).has_flag(flag);
}

bool map::has_flag_ter_or_furn(const std::string & flag, const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return false;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return current_submap->get_ter( lx, ly ).obj().has_flag(flag) || current_submap->get_furn(lx, ly).obj().has_flag(flag);
}

bool map::has_flag_ter_and_furn(const std::string & flag, const int x, const int y) const
{
 return ter_at(x, y).has_flag(flag) && furn_at(x, y).has_flag(flag);
}
/////
bool map::has_flag(const ter_bitflags flag, const int x, const int y) const
{
    return has_flag_ter_or_furn(flag, x, y); // Does bound checking
}

bool map::has_flag_ter(const ter_bitflags flag, const int x, const int y) const
{
 return ter_at(x, y).has_flag(flag);
}

bool map::has_flag_furn(const ter_bitflags flag, const int x, const int y) const
{
 return furn_at(x, y).has_flag(flag);
}

bool map::has_flag_ter_or_furn(const ter_bitflags flag, const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return false;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at(x, y, lx, ly);

    return current_submap->get_ter( lx, ly ).obj().has_flag(flag) || current_submap->get_furn(lx, ly).obj().has_flag(flag);
}

bool map::has_flag_ter_and_furn(const ter_bitflags flag, const int x, const int y) const
{
    if (!INBOUNDS(x, y)) {
        return false;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at( x, y, lx, ly );

    return current_submap->get_ter( lx, ly ).obj().has_flag(flag) && current_submap->get_furn(lx, ly).obj().has_flag(flag);
}

// End of 2D flags

bool map::has_flag( const std::string &flag, const tripoint &p ) const
{
    return has_flag_ter_or_furn( flag, p ); // Does bound checking
}

bool map::can_put_items( const tripoint &p )
{
    return !has_flag( "NOITEM", p ) && !has_flag( "SEALED", p );
}

bool map::has_flag_ter( const std::string & flag, const tripoint &p ) const
{
    return ter_at( p ).has_flag( flag );
}

bool map::has_flag_furn( const std::string & flag, const tripoint &p ) const
{
    return furn_at( p ).has_flag( flag );
}

bool map::has_flag_ter_or_furn( const std::string & flag, const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->get_ter( lx, ly ).obj().has_flag( flag ) ||
           current_submap->get_furn( lx, ly ).obj().has_flag( flag );
}

bool map::has_flag_ter_and_furn( const std::string & flag, const tripoint &p ) const
{
    return ter_at( p ).has_flag( flag ) && furn_at( p ).has_flag( flag );
}

bool map::has_flag( const ter_bitflags flag, const tripoint &p ) const
{
    return has_flag_ter_or_furn( flag, p ); // Does bound checking
}

bool map::has_flag_ter( const ter_bitflags flag, const tripoint &p ) const
{
    return ter_at( p ).has_flag( flag );
}

bool map::has_flag_furn( const ter_bitflags flag, const tripoint &p ) const
{
    return furn_at( p ).has_flag( flag );
}

bool map::has_flag_ter_or_furn( const ter_bitflags flag, const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->get_ter( lx, ly ).obj().has_flag( flag ) ||
           current_submap->get_furn( lx, ly ).obj().has_flag( flag );
}

bool map::has_flag_ter_and_furn( const ter_bitflags flag, const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->get_ter( lx, ly ).obj().has_flag( flag ) &&
           current_submap->get_furn( lx, ly ).obj().has_flag( flag );
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
    if( furniture.loadid != f_null && furniture.bash.str_max != -1 ) {
        furn_smash = true;
    ///\EFFECT_STR determines what terrain can be smashed
    } else if( terrain.bash.str_max != -1 && ( !terrain.bash.bash_below || allow_floor ) ) {
        ter_smash = true;
    }

    if( veh != nullptr && veh->obstacle_at_part( part ) >= 0 ) {
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

    int ret = (10 * (str - bash_min)) / (bash_max - bash_min);
    // Round up to 1, so that desperate NPCs can try to bash down walls
    return std::max( ret, 1 );
}

// 2D bashable

bool map::is_bashable(const int x, const int y) const
{
    return is_bashable( tripoint( x, y, abs_sub.z ) );
}

bool map::is_bashable_ter(const int x, const int y) const
{
    return is_bashable_ter( tripoint( x, y, abs_sub.z ) );
}

bool map::is_bashable_furn(const int x, const int y) const
{
    return is_bashable_furn( tripoint( x, y, abs_sub.z ) );
}

bool map::is_bashable_ter_furn(const int x, const int y) const
{
    return is_bashable_ter_furn( tripoint( x, y, abs_sub.z ) );
}

int map::bash_strength(const int x, const int y) const
{
    return bash_strength( tripoint( x, y, abs_sub.z ) );
}

int map::bash_resistance(const int x, const int y) const
{
    return bash_resistance( tripoint( x, y, abs_sub.z ) );
}

int map::bash_rating(const int str, const int x, const int y) const
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

    int vpart = -1;
    const vehicle *veh = veh_at( p , vpart );
    if( veh != nullptr && veh->obstacle_at_part( vpart ) >= 0 ) {
        return true;
    }

    if( has_furn( p ) && furn_at( p ).bash.str_max != -1 ) {
        return true;
    }

    const auto &ter_bash = ter_at( p ).bash;
    if( ter_bash.str_max != -1 && ( !ter_bash.bash_below || allow_floor ) ) {
        return true;
    }

    return false;
}

bool map::is_bashable_ter( const tripoint &p, const bool allow_floor ) const
{
    const auto &ter_bash = ter_at( p ).bash;
    if( ter_bash.str_max != -1 && ( !ter_bash.bash_below || allow_floor ) ) {
        return true;
    }

    return false;
}

bool map::is_bashable_furn( const tripoint &p ) const
{
    if ( has_furn( p ) && furn_at( p ).bash.str_max != -1 ) {
        return true;
    }
    return false;
}

bool map::is_bashable_ter_furn( const tripoint &p, const bool allow_floor ) const
{
    return is_bashable_furn( p ) || is_bashable_ter( p, allow_floor );
}

int map::bash_strength( const tripoint &p, const bool allow_floor ) const
{
    if( has_furn( p ) && furn_at( p ).bash.str_max != -1 ) {
        return furn_at( p ).bash.str_max;
    }

    const auto &ter_bash = ter_at( p ).bash;
    if( ter_bash.str_max != -1 && ( !ter_bash.bash_below || allow_floor ) ) {
        return ter_bash.str_max;
    }

    return -1;
}

int map::bash_resistance( const tripoint &p, const bool allow_floor ) const
{
    if( has_furn( p ) && furn_at( p ).bash.str_min != -1 ) {
        return furn_at( p ).bash.str_min;
    }

    const auto &ter_bash = ter_at( p ).bash;
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

    int part = -1;
    const furn_t &furniture = furn_at( p );
    const ter_t &terrain = ter_at( p );
    const vehicle *veh = veh_at( p, part );
    return bash_rating_internal( str, furniture, terrain, allow_floor, veh, part );
}

// End of 3D bashable

void map::make_rubble( const tripoint &p )
{
    make_rubble( p, f_rubble, false, t_dirt, false );
}

void map::make_rubble( const tripoint &p, const furn_id rubble_type, const bool items )
{
    make_rubble( p, rubble_type, items, t_dirt, false );
}

void map::make_rubble( const tripoint &p, furn_id rubble_type, bool items, ter_id floor_type, bool overwrite)
{
    if( overwrite ) {
        ter_set(p, floor_type);
        furn_set(p, rubble_type);
    } else {
        // First see if there is existing furniture to destroy
        if( is_bashable_furn( p ) ) {
            destroy_furn( p, true );
        }
        // Leave the terrain alone unless it interferes with furniture placement
        if( move_cost(p) <= 0 && is_bashable_ter( p ) ) {
            destroy( p, true );
        }
        // Check again for new terrain after potential destruction
        if( move_cost(p) <= 0 ) {
            ter_set(p, floor_type);
        }

        furn_set(p, rubble_type);
    }

    if( !items ) {
        return;
    }

    //Still hardcoded, but a step up from the old stuff due to being in only one place
    if (rubble_type == f_wreckage) {
        item chunk("steel_chunk", calendar::turn);
        item scrap("scrap", calendar::turn);
        item pipe("pipe", calendar::turn);
        item wire("wire", calendar::turn);
        add_item_or_charges(p, chunk);
        add_item_or_charges(p, scrap);
        if (one_in(5)) {
            add_item_or_charges(p, pipe);
            add_item_or_charges(p, wire);
        }
    } else if (rubble_type == f_rubble_rock) {
        item rock("rock", calendar::turn);
        int rock_count = rng(1, 3);
        for (int i = 0; i < rock_count; i++) {
            add_item_or_charges(p, rock);
        }
    } else if (rubble_type == f_rubble) {
        item splinter("splinter", calendar::turn);
        item nail("nail", calendar::turn);
        int splinter_count = rng(2, 8);
        int nail_count = rng(5, 10);
        for (int i = 0; i < splinter_count; i++) {
            add_item_or_charges(p, splinter);
        }
        for (int i = 0; i < nail_count; i++) {
            add_item_or_charges(p, nail);
        }
    }
}

/**
 * Returns whether or not the terrain at the given location can be dived into
 * (by monsters that can swim or are aquatic or nonbreathing).
 * @param x The x coordinate to look at.
 * @param y The y coordinate to look at.
 * @return true if the terrain can be dived into; false if not.
 */
bool map::is_divable(const int x, const int y) const
{
  return has_flag("SWIMMABLE", x, y) && has_flag(TFLAG_DEEP_WATER, x, y);
}

bool map::is_divable( const tripoint &p ) const
{
    return has_flag( "SWIMMABLE", p ) && has_flag( TFLAG_DEEP_WATER, p );
}

bool map::is_outside(const int x, const int y) const
{
    if(!INBOUNDS(x, y)) {
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

bool map::is_last_ter_wall(const bool no_furn, const int x, const int y,
                           const int xmax, const int ymax, const direction dir) const {
    int xmov = 0;
    int ymov = 0;
    switch ( dir ) {
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
    while ( (loop) && ((dir == NORTH && y2 >= 0) ||
                       (dir == SOUTH && y2 < ymax) ||
                       (dir == WEST  && x2 >= 0) ||
                       (dir == EAST  && x2 < xmax)) ) {
        if ( no_furn && has_furn(x2, y2) ) {
            loop = false;
            result = false;
        } else if ( !has_flag_ter("FLAT", x2, y2) ) {
            loop = false;
            if ( !has_flag_ter("WALL", x2, y2) ) {
                result = false;
            }
        }
        x2 += xmov;
        y2 += ymov;
    }
    return result;
}

bool map::flammable_items_at( const tripoint &p )
{
    if( has_flag( TFLAG_SEALED, p ) && !has_flag( TFLAG_ALLOW_FIELD_EFFECT, p ) ) {
        // Sealed containers don't allow fire, so shouldn't allow setting the fire either
        return false;
    }

    for( const auto &i : i_at( p ) ) {
        if( i.flammable() ) {
            // Total fire resistance == 0
            return true;
        }
    }

    return false;
}

bool map::moppable_items_at( const tripoint &p )
{
    for (auto &i : i_at(p)) {
        if (i.made_of(LIQUID)) {
            return true;
        }
    }
    const field &fld = field_at(p);
    if(fld.findField(fd_blood) != 0 || fld.findField(fd_blood_veggy) != 0 ||
          fld.findField(fd_blood_insect) != 0 || fld.findField(fd_blood_invertebrate) != 0
          || fld.findField(fd_gibs_flesh) != 0 || fld.findField(fd_gibs_veggy) != 0 ||
          fld.findField(fd_gibs_insect) != 0 || fld.findField(fd_gibs_invertebrate) != 0
          || fld.findField(fd_bile) != 0 || fld.findField(fd_slime) != 0 ||
          fld.findField(fd_sludge) != 0) {
        return true;
    }
    int vpart;
    vehicle *veh = veh_at(p, vpart);
    if(veh != 0) {
        std::vector<int> parts_here = veh->parts_at_relative(veh->parts[vpart].mount.x, veh->parts[vpart].mount.y);
        for(auto &i : parts_here) {
            if(veh->parts[i].blood > 0) {
                return true;
            }
        }
    }
    return false;
}

void map::decay_fields_and_scent( const int amount )
{
    // Decay scent separately, so that later we can use field count to skip empty submaps
    tripoint tmp;
    tmp.z = abs_sub.z; // TODO: Make this happen on all z-levels
    for( tmp.x = 0; tmp.x < my_MAPSIZE * SEEX; tmp.x++ ) {
        for( tmp.y = 0; tmp.y < my_MAPSIZE * SEEY; tmp.y++ ) {
            if( g->scent( tmp ) > 0 ) {
                g->scent( tmp )--;
            }
        }
    }

    const int amount_fire = amount / 3; // Decay fire by this much
    const int amount_liquid = amount / 2; // Decay washable fields (blood, guts etc.) by this
    const int amount_gas = amount / 5; // Decay gas type fields by this
    // Coord code copied from lightmap calculations
    // TODO: Z
    const int smz = abs_sub.z;
    const auto &outside_cache = get_cache_ref( smz ).outside_cache;
    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            auto const cur_submap = get_submap_at_grid( smx, smy, smz );
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
 for (int x = 0; x < SEEX * my_MAPSIZE; x++) {
  for (int y = 0; y < SEEY * my_MAPSIZE; y++) {
   if (is_outside(x, y))
    options.push_back(point(x, y));
  }
 }
 return random_entry( options, point( -1, -1 ) );
}

bool map::has_adjacent_furniture( const tripoint &p )
{
    const signed char cx[4] = { 0, -1, 0, 1};
    const signed char cy[4] = {-1,  0, 1, 0};

    for (int i = 0; i < 4; i++)
    {
        const int adj_x = p.x + cx[i];
        const int adj_y = p.y + cy[i];
        if ( has_furn( tripoint( adj_x, adj_y, p.z ) ) &&
             furn_at( tripoint( adj_x, adj_y, p.z ) ).has_flag("BLOCKSDOOR") ) {
            return true;
        }
    }

 return false;
}

bool map::has_nearby_fire( const tripoint &p, int radius )
{
    for(int dx = -radius; dx <= radius; dx++) {
        for(int dy = -radius; dy <= radius; dy++) {
            const tripoint pt( p.x + dx, p.y + dy, p.z );
            if( get_field( pt, fd_fire ) != nullptr ) {
                return true;
            }
            if (ter(pt) == t_lava) {
                return true;
            }
        }
    }
    return false;
}

void map::mop_spills( const tripoint &p ) {
    auto items = i_at( p );
    for( auto it = items.begin(); it != items.end(); ) {
        if( it->made_of(LIQUID) ) {
            it = items.erase( it );
        } else {
            it++;
        }
    }
    remove_field( p, fd_blood );
    remove_field( p, fd_blood_veggy );
    remove_field( p, fd_blood_insect );
    remove_field( p, fd_blood_invertebrate );
    remove_field( p, fd_gibs_flesh );
    remove_field( p, fd_gibs_veggy );
    remove_field( p, fd_gibs_insect );
    remove_field( p, fd_gibs_invertebrate );
    remove_field( p, fd_bile );
    remove_field( p, fd_slime );
    remove_field( p, fd_sludge );
    int vpart;
    vehicle *veh = veh_at(p, vpart);
    if(veh != 0) {
        std::vector<int> parts_here = veh->parts_at_relative( veh->parts[vpart].mount.x,
                                                              veh->parts[vpart].mount.y );
        for( auto &elem : parts_here ) {
            veh->parts[elem].blood = 0;
        }
    }
}

void map::fungalize( const tripoint &sporep, Creature *origin, double spore_chance )
{
    int mondex = g->mon_at( sporep );
    if( mondex != -1 ) { // Spores hit a monster
        if( g->u.sees(sporep) &&
            !g->zombie(mondex).type->in_species( FUNGUS )) {
            add_msg(_("The %s is covered in tiny spores!"),
                    g->zombie(mondex).name().c_str());
        }
        monster &critter = g->zombie( mondex );
        if( !critter.make_fungus() ) {
            // Don't insta-kill non-fungables. Jabberwocks, for example
            critter.add_effect( "stunned", rng( 1, 3 ) );
            critter.apply_damage( origin, bp_torso, rng( 25, 50 ) );
        }
    } else if( g->u.pos() == sporep ) {
        player &pl = g->u; // TODO: Make this accept NPCs when they understand fungals
        ///\EFFECT_DEX increases chance of knocking fungal spores away with your TAIL_CATTLE

        ///\EFFECT_MELEE increases chance of knocking fungal sports away with your TAIL_CATTLE
        if( pl.has_trait("TAIL_CATTLE") &&
            one_in( 20 - pl.dex_cur - pl.skillLevel( skill_id( "melee" ) ) ) ) {
            pl.add_msg_if_player( _("The spores land on you, but you quickly swat them off with your tail!" ) );
            return;
        }
        // Spores hit the player--is there any hope?
        bool hit = false;
        hit |= one_in(4) && pl.add_env_effect("spores", bp_head, 3, 90, bp_head);
        hit |= one_in(2) && pl.add_env_effect("spores", bp_torso, 3, 90, bp_torso);
        hit |= one_in(4) && pl.add_env_effect("spores", bp_arm_l, 3, 90, bp_arm_l);
        hit |= one_in(4) && pl.add_env_effect("spores", bp_arm_r, 3, 90, bp_arm_r);
        hit |= one_in(4) && pl.add_env_effect("spores", bp_leg_l, 3, 90, bp_leg_l);
        hit |= one_in(4) && pl.add_env_effect("spores", bp_leg_r, 3, 90, bp_leg_r);
        if( hit ) {
            add_msg(m_warning, _("You're covered in tiny spores!"));
        }
    } else if( g->num_zombies() < 250 && x_in_y( spore_chance, 1.0 ) ) { // Spawn a spore
        if( g->summon_mon( mon_spore, sporep ) ) {
            monster *spore = g->monster_at(sporep);
            monster *origin_mon = dynamic_cast<monster*>( origin );
            if( origin_mon != nullptr ) {
                spore->make_ally( origin_mon );
            } else if( origin != nullptr && origin->is_player() && g->u.has_trait("THRESH_MYCUS") ) {
                spore->friendly = 1000;
            }
        }
    } else {
        g->spread_fungus( sporep );
    }
}

void map::create_spores( const tripoint &p, Creature* source )
{
    tripoint tmp = p;
    int &i = tmp.x;
    int &j = tmp.y;
    for( i = p.x - 1; i <= p.x + 1; i++ ) {
        for( j = p.y - 1; j <= p.y + 1; j++ ) {
            fungalize( tmp, source, 0.25 );
        }
    }
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
        } else if( has_flag("FLAT", t ) && one_in( 8 ) ) {
            crush( t );
            make_rubble( t );
        }
    }
}

void map::smash_items(const tripoint &p, const int power)
{
    if( !has_items( p ) ) {
        return;
    }

    std::vector<item> contents;
    auto items = g->m.i_at(p);
    for( auto i = items.begin(); i != items.end(); ) {
        if (i->active == true) {
            // Get the explosion item actor
            if (i->type->get_use( "explosion" ) != nullptr) {
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
        const float volume_factor = std::max<float>( 40, i->volume() );
        float damage_chance = 10.0f * power / volume_factor;
        // Example:
        // Power 40 (just below C4 epicenter) vs two-by-four
        // damage_chance = 10 * 40 / 40 = 10, material_factor = 8
        // Will deal 1 damage, then 20% chance for another point
        // Power 20 (grenade minus shrapnel) vs glass bottle
        // 10 * 20 / 40 = 5 vs 1
        // 5 damage (destruction)

        field_id type_blood = fd_null;
        if( i->is_corpse() ) {
            type_blood = i->get_mtype()->bloodType();
        }
        const bool by_charges = i->count_by_charges();
        // See if they were damaged
        if( by_charges ) {
            const int def_charges = i->liquid_charges( 1 );
            damage_chance *= def_charges;
            while( ( damage_chance > material_factor ||
                     x_in_y( damage_chance, material_factor ) ) &&
                   i->charges > 0 ) {
                i->charges--;
                damage_chance -= material_factor;
            }
        } else {
            while( ( damage_chance > material_factor ||
                     x_in_y( damage_chance, material_factor ) ) &&
                   i->damage < 4 ) {
                i->damage++;
                if( type_blood != fd_null ) {
                    for( const tripoint &pt : points_in_radius( p, 1 ) ) {
                        if( !one_in(damage_chance) ) {
                            g->m.add_field( pt, type_blood, 1, 0 );
                        }
                    }
                }
                damage_chance -= material_factor;
            }
        }
        // Remove them if they were damaged too much
        if( i->damage >= 4 || ( by_charges && i->charges == 0 ) ) {
            // But save the contents
            for( auto &elem : i->contents ) {
                contents.push_back( elem );
            }

            i = i_rem( p, i );
        } else {
            i++;
        }
    }

    for( auto it : contents ) {
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

    const auto &ter_there = ter_at( p );
    const auto &roof = ter_there.roof;
    if( roof.empty() || roof == null_ter_t ) {
        // No roof
        // Not acceptable if the tile is not passable
        if( !allow_air ) {
            return t_dirt;
        }

        return t_open_air;
    }

    ter_id new_ter = terfind( roof );
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
    const auto &terid = ter_at( p );
    const auto &furnid = furn_at( p );
    bool smash_furn = false;
    bool smash_ter = false;
    const map_bash_info *bash = nullptr;

    bool success = false;

    if( has_furn(p) && furn_at(p).bash.str_max != -1 ) {
        bash = &furn_at(p).bash;
        smash_furn = true;
    } else if( ter_at(p).bash.str_max != -1 ) {
        bash = &ter_at( p ).bash;
        smash_ter = true;
    }

    // Floor bashing check
    // Only allow bashing floors when we want to bash floors and we're in z-level mode
    // Unless we're destroying, then it gets a little weird
    if( smash_ter && bash->bash_below && ( !zlevels || !params.bash_floor ) ) {
        if( !params.destroy ) {
            smash_ter = false;
            bash = nullptr;
        } else if( bash->ter_set == null_ter_t && zlevels ) {
            // A hack for destroy && !bash_floor
            // We have to check what would we create and cancel if it is what we have now
            tripoint below( p.x, p.y, p.z - 1 );
            const auto roof = get_roof( below, false );
            if( roof == ter( p ) ) {
                smash_ter = false;
                bash = nullptr;
            }
        } else if( bash->ter_set == null_ter_t && ter( p ) == t_dirt ) {
            // As above, except for no-z-levels case
            smash_ter = false;
            bash = nullptr;
        }
    }

    // TODO: what if silent is true?
    if( has_flag("ALARMED", p) && !g->event_queued(EVENT_WANTED) ) {
        sounds::sound(p, 40, _("an alarm go off!"), false, "environment", "alarm");
        // Blame nearby player
        if( rl_dist( g->u.pos(), p ) <= 3 ) {
            g->u.add_memorial_log(pgettext("memorial_male", "Set off an alarm."),
                                  pgettext("memorial_female", "Set off an alarm."));
            const point abs = overmapbuffer::ms_to_sm_copy( getabs( p.x, p.y ) );
            g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, tripoint( abs.x, abs.y, p.z ) );
        }
    }

    if( bash == nullptr || (bash->destroy_only && !params.destroy) ) {
        // Nothing bashable here
        if( move_cost( p ) <= 0 ) {
            if( !params.silent ) {
                sounds::sound( p, 18, _("thump!"), false, "smash_thump", "smash_success" );
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
        if ( bash->str_min_blocked != -1 || bash->str_max_blocked != -1 ) {
            if( has_adjacent_furniture( p ) ) {
                if ( bash->str_min_blocked != -1 ) {
                    smin = bash->str_min_blocked;
                }
                if ( bash->str_max_blocked != -1 ) {
                    smax = bash->str_max_blocked;
                }
            }
        }

        if( bash->str_min_supported != -1 || bash->str_max_supported != -1 ) {
            tripoint below( p.x, p.y, p.z - 1 );
            if( !zlevels || has_flag( "SUPPORTS_ROOF", below ) ) {
                if ( bash->str_min_supported != -1 ) {
                    smin = bash->str_min_supported;
                }
                if ( bash->str_max_supported != -1 ) {
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
        soundfxvariant = furnid.id;
    } else {
        soundfxvariant = terid.id;
    }

    if( !params.destroy && !success ) {
        if( sound_fail_vol == -1 ) {
            sound_volume = 12;
        } else {
            sound_volume = sound_fail_vol;
        }

        sound = _(bash->sound_fail.c_str());
        params.did_bash = true;
        if( !params.silent ) {
            sounds::sound( p, sound_volume, sound, false, "smash_fail", soundfxvariant );
        }

        return;
    }

    // Clear out any partially grown seeds
    if( has_flag_ter_or_furn( "PLANT", p ) ) {
        i_clear( p );
    }

    if( ( smash_furn && has_flag_furn("FUNGUS", p) ) ||
        ( smash_ter && has_flag_ter("FUNGUS", p) ) ) {
        create_spores( p );
    }

    if( params.destroy ) {
        sound_volume = smin * 2;
    } else {
        if( sound_vol == -1 ) {
            sound_volume = std::min(int(smin * 1.5), smax);
        } else {
            sound_volume = sound_vol;
        }
    }

    soundfxid = "smash_success";
    sound = _(bash->sound.c_str());
    // Set this now in case the ter_set below changes this
    const bool collapses = smash_ter && has_flag("COLLAPSES", p);
    const bool supports = smash_ter && has_flag("SUPPORTS_ROOF", p);

    const bool tent = smash_furn && !bash->tent_centers.empty();
    // Special code to collapse the tent if destroyed
    if( tent ) {
        // Get ids of possible centers
        std::set<furn_id> centers;
        for( const auto &center : bash->tent_centers ) {
            const furn_id cur_id = furnfind( center );
            if( cur_id != f_null ) {
                centers.insert( cur_id );
            }
        }

        tripoint tentp = tripoint_min;
        furn_id center_type = f_null;

        // Find the center of the tent
        // First check if we're not currently bashing the center
        if( centers.count( furn( p ) ) > 0 ) {
            tentp = p;
            center_type = furn( p );
        } else {
            for( const tripoint &pt : points_in_radius( p, bash->collapse_radius ) ) {
                const furn_id &f_at = furn( pt );
                // Check if we found the center of the current tent
                if( centers.count( f_at ) > 0 ) {
                    tentp = pt;
                    center_type = f_at;
                    break;
                }
            }
        }
        // Didn't find any tent center, wreck the current tile
        if( center_type == f_null || tentp == tripoint_min ) {
            spawn_items( p, item_group::items_from( bash->drop_group, calendar::turn ) );
            furn_set( p, bash->furn_set );
        } else {
            // Take the tent down
            const int rad = center_type.obj().bash.collapse_radius;
            for( const tripoint &pt : points_in_radius( tentp, rad ) ) {
                const auto frn = furn( pt );
                if( frn == f_null ) {
                    continue;
                }

                const auto recur_bash = &frn.obj().bash;
                // Check if we share a center type and thus a "tent type"
                for( const auto &center : recur_bash->tent_centers ) {
                    const furn_id cur_id = furnfind( center );
                    if( centers.count( cur_id ) > 0 ) {
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
        // Hack alert.
        // Signs have cosmetics associated with them on the submap since
        // furniture can't store dynamic data to disk. To prevent writing
        // mysteriously appearing for a sign later built here, remove the
        // writing from the submap.
        delete_signage( p );
    } else if( !smash_ter ) {
        // Handle error earlier so that we can assume smash_ter is true below
        debugmsg( "data/json/terrain.json does not have %s.bash.ter_set set!",
                  ter_at(p).id.c_str() );
    } else if( bash->ter_set != null_ter_t ) {
        // If the terrain has a valid post-destroy terrain, set it
        ter_set( p, bash->ter_set );
    } else {
        tripoint below( p.x, p.y, p.z - 1 );
        const auto &ter_below = ter_at( below );
        if( bash->bash_below && ter_below.has_flag( "SUPPORTS_ROOF" ) ) {
            // When bashing the tile below, don't allow bashing the floor
            bash_params params_below = params; // Make a copy
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
            const auto roof = get_roof( below, params.bash_floor && ter_at( below ).movecost != 0 );
            ter_set( p, roof );
        }
    }

    if( bash->explosive > 0 ) {
        g->explosion( p, bash->explosive, 0.8, 0, false );
    }

    if( collapses ) {
        collapse_at( p, params.silent );
    }
    // Check the flag again to ensure the new terrain doesn't support anything
    if( supports && !has_flag( "SUPPORTS_ROOF", p ) ) {
        for( const tripoint t : points_in_radius( p, 1 ) ) {
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
        sounds::sound( p, sound_volume, sound, false, soundfxid, soundfxvariant );
    }
}

bash_params map::bash( const tripoint &p, const int str,
                       bool silent, bool destroy, bool bash_floor,
                       const vehicle *bashing_vehicle )
{
    bash_params bsh{
        str, silent, destroy, bash_floor, (float)rng_float( 0, 1.0f ), false, false, false
    };

    bash_field( p, bsh );
    bash_items( p, bsh );
    // Don't bash the vehicle doing the bashing
    const vehicle *veh = veh_at( p );
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
        // the check for active supresses molotovs smashing themselves with their own explosion
        if( bashed_item->made_of("glass") && !bashed_item->active && one_in(2) ) {
            params.did_bash = true;
            smashed_glass = true;
            for( auto bashed_content : bashed_item->contents ) {
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
        sounds::sound( p, 12, _("glass shattering"), false, "smash_success", "smash_glass_contents" );
    }
}

void map::bash_vehicle( const tripoint &p, bash_params &params )
{
    // Smash vehicle if present
    int vpart;
    vehicle *veh = veh_at( p, vpart );
    if( veh != nullptr ) {
        veh->damage( vpart, params.strength, DT_BASH );
        if( !params.silent ) {
            sounds::sound( p, 18, _("crash!"), false, "smash_success", "hit_vehicle" );
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
    int veh_part;
    player *crushed_player = nullptr;
    int npc_index = g->npc_at( p );
    if( g->u.pos() == p ) {
        crushed_player = &(g->u);
    } else if( npc_index != -1 ) {
        crushed_player = static_cast<player *>(g->active_npc[npc_index]);
    }

    if( crushed_player != nullptr ) {
        bool player_inside = false;
        if( crushed_player->in_vehicle ) {
            vehicle *veh = veh_at(p, veh_part);
            player_inside = veh != nullptr && veh->is_inside(veh_part);
        }
        if (!player_inside) { //If there's a player at p and he's not in a covered vehicle...
            //This is the roof coming down on top of us, no chance to dodge
            crushed_player->add_msg_player_or_npc( m_bad, _("You are crushed by the falling debris!"),
                                                   _("<npcname> is crushed by the falling debris!") );
            // TODO: Make this depend on the ceiling material
            const int dam = rng(0, 40);
            // Torso and head take the brunt of the blow
            body_part hit = bp_head;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .25 ) );
            hit = bp_torso;
            crushed_player->deal_damage( nullptr, hit, damage_instance( DT_BASH, dam * .45 ) );
            // Legs take the next most through transfered force
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
            crushed_player->add_effect("crushed", 1, num_bp, true);
            crushed_player->check_dead_state();
        }
    }

    int mon = g->mon_at( p );
    if( mon != -1 ) {
        monster* monhit = &(g->zombie(mon));
        // 25 ~= 60 * .45 (torso)
        monhit->deal_damage(nullptr, bp_torso, damage_instance(DT_BASH, rng(0,25)));

        // Pin whoever got hit
        monhit->add_effect("crushed", 1, num_bp, true);
        monhit->check_dead_state();
    }

    vehicle *veh = veh_at(p, veh_part);
    if( veh != nullptr ) {
        // Arbitrary number is better than collapsing house roof crushing APCs
        veh->damage(veh_part, rng(100, 1000), DT_BASH, false);
    }
}

void map::shoot( const tripoint &p, projectile &proj, const bool hit_items )
{
    // TODO: Make bashing count fully, but other types much less
    const int initial_damage = proj.impact.total_damage();
    if( initial_damage < 0 ) {
        return;
    }

    int dam = initial_damage;
    const auto &ammo_effects = proj.proj_effects;

    if( has_flag("ALARMED", p) && !g->event_queued(EVENT_WANTED) ) {
        sounds::sound(p, 30, _("An alarm sounds!"));
        const tripoint abs = overmapbuffer::ms_to_sm_copy( getabs( p ) );
        g->add_event(EVENT_WANTED, int(calendar::turn) + 300, 0, abs );
    }

    const bool inc = (ammo_effects.count("INCENDIARY") || ammo_effects.count("FLAME"));
    int vpart;
    vehicle *veh = veh_at(p, vpart);
    if( veh != nullptr ) {
        dam = veh->damage( vpart, dam, inc ? DT_HEAT : DT_BASH, hit_items );
    }

    ter_id terrain = ter( p );
    if( terrain == t_wall_wood_broken ||
        terrain == t_wall_log_broken ||
        terrain == t_door_b ) {
        if (hit_items || one_in(8)) { // 1 in 8 chance of hitting the door
            dam -= rng(20, 40);
            if (dam > 0) {
                sounds::sound(p, 10, _("crash!"), false, "smash", "wall");
                ter_set(p, t_dirt);
            }
        }
        else {
            dam -= rng(0, 1);
        }
    } else if( terrain == t_door_c ||
               terrain == t_door_locked ||
               terrain == t_door_locked_peep ||
               terrain == t_door_locked_alarm ) {
        dam -= rng(15, 30);
        if (dam > 0) {
            sounds::sound(p, 10, _("smash!"), false, "smash", "door");
            ter_set(p, t_door_b);
        }
    } else if( terrain == t_door_boarded ||
               terrain == t_door_boarded_damaged ||
               terrain == t_rdoor_boarded ||
               terrain == t_rdoor_boarded_damaged ) {
        dam -= rng(15, 35);
        if (dam > 0) {
            sounds::sound(p, 10, _("crash!"), false, "smash", "door_boarded");
            ter_set(p, t_door_b);
        }
    } else if( terrain == t_window_domestic_taped ||
               terrain == t_curtains ||
               terrain == t_window_domestic ) {
        if (ammo_effects.count("LASER")) {
            if ( terrain == t_window_domestic_taped ||
                 terrain == t_curtains ){
              dam -= rng(1, 5);
            }
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                sounds::sound(p, 16, _("glass breaking!"), false, "smash", "glass");
                ter_set(p, t_window_frame);
                spawn_item(p, "sheet", 1);
                spawn_item(p, "stick");
                spawn_item(p, "string_36");
            }
        }
    } else if( terrain == t_window_taped ||
               terrain == t_window_alarm_taped ||
               terrain == t_window ||
               terrain == t_window_no_curtains ||
               terrain == t_window_no_curtains_taped ||
               terrain == t_window_alarm ) {
        if (ammo_effects.count("LASER")) {
            if ( terrain == t_window_taped ||
                 terrain == t_window_alarm_taped ||
                 terrain == t_window_no_curtains_taped ){
                    dam -= rng(1, 5);
                }
            dam -= rng(0, 5);
        } else {
            dam -= rng(1,3);
            if (dam > 0) {
                sounds::sound(p, 16, _("glass breaking!"), false, "smash", "glass");
                ter_set(p, t_window_frame);
            }
        }
    } else if( terrain == t_window_boarded ) {
        dam -= rng(10, 30);
        if (dam > 0) {
                sounds::sound(p, 16, _("glass breaking!"), false, "smash", "glass");
            ter_set(p, t_window_frame);
        }
    } else if( terrain == t_wall_glass  ||
               terrain == t_wall_glass_alarm ||
               terrain == t_door_glass_c ) {
        if (ammo_effects.count("LASER")) {
            dam -= rng(0,5);
        } else {
            dam -= rng(1,8);
            if (dam > 0) {
                sounds::sound(p, 16, _("glass breaking!"), false, "smash", "glass");
                ter_set(p, t_floor);
            }
        }
    } else if( terrain == t_reinforced_glass ) {
        // reinforced glass stops most bullets
        // laser beams are attenuated
        if (ammo_effects.count("LASER")) {
            dam -= rng(0, 8);
        } else {
            //Greatly weakens power of bullets
            dam -= 40;
            if (dam <= 0) {
                add_msg(_("The shot is stopped by the reinforced glass wall!"));
            } else if (dam >= 40) {
                //high powered bullets penetrate the glass, but only extremely strong
                // ones (80 before reduction) actually destroy the glass itself.
                sounds::sound(p, 16, _("glass breaking!"), false, "smash", "glass");
                ter_set(p, t_floor);
            }
        }
    } else if( terrain == t_paper ) {
        dam -= rng(4, 16);
        if( dam > 0 ) {
            sounds::sound(p, 8, _("rrrrip!"));
            ter_set(p, t_dirt);
        }
        if( inc ) {
            add_field(p, fd_fire, 1, 0);
        }
    } else if( terrain == t_gas_pump ) {
        if (hit_items || one_in(3)) {
            if (dam > 15) {
                if( inc ) {
                    g->explosion( p, 40, 0.8, 0, true);
                } else {
                    for( const tripoint &pt : points_in_radius( p, 2 ) ) {
                        if( one_in( 3 ) && move_cost( pt ) > 0 ) {
                            int gas_amount = rng(10, 100);
                            item gas_spill("gasoline", calendar::turn);
                            gas_spill.charges = gas_amount;
                            add_item_or_charges( pt, gas_spill );
                        }
                    }

                    sounds::sound(p, 10, _("smash!"));
                }
                ter_set(p, t_gas_pump_smashed);
            }
            dam -= 60;
        }
    } else if( terrain == t_vat ) {
        if (dam >= 10) {
            sounds::sound(p, 20, _("ke-rash!"));
            ter_set(p, t_floor);
        } else {
            dam = 0;
        }
    } else if( move_cost( p ) == 0 && !trans( p ) ) {
        bash( p, dam, false );
        dam = 0; // TODO: Preserve some residual damage when it makes sense.
    } else {
        dam -= (rng(0, 1) * rng(0, 1) * rng(0, 1));
    }

    if (ammo_effects.count("TRAIL") && !one_in(4)) {
        add_field(p, fd_smoke, rng(1, 2), 0 );
    }

    if (ammo_effects.count("STREAM") && !one_in(3)) {
        add_field(p, fd_fire, rng(1, 2), 0 );
    }

    if (ammo_effects.count("STREAM_GAS_FUNGICIDAL") && !one_in(3)) {
        add_field(p, fd_fungicidal_gas, rng(1, 2), 0 );
    }

    if (ammo_effects.count("STREAM_BIG") && !one_in(4)) {
        add_field(p, fd_fire, 2, 0 );
    }

    if (ammo_effects.count("LIGHTNING")) {
        add_field(p, fd_electricity, rng(2, 3), 0 );
    }

    if (ammo_effects.count("PLASMA") && one_in(2)) {
        add_field(p, fd_plasma, rng(1, 2), 0 );
    }

    if (ammo_effects.count("LASER")) {
        add_field(p, fd_laser, 2, 0 );
    }

    // Set damage to 0 if it's less
    if( dam < 0 ) {
        dam = 0;
    }

    // Check fields?
    const field_entry *fieldhit = get_field( p, fd_web );
    if( fieldhit != nullptr ) {
        if( inc ) {
            add_field( p, fd_fire, fieldhit->getFieldDensity() - 1, 0 );
        } else if (dam > 5 + fieldhit->getFieldDensity() * 5 &&
                   one_in(5 - fieldhit->getFieldDensity())) {
            dam -= rng(1, 2 + fieldhit->getFieldDensity() * 2);
            remove_field(p,fd_web);
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
    if( (move_cost( p ) == 2 && !hit_items) || !inbounds( p ) ) {
        return; // Items on floor-type spaces won't be shot up.
    }

    // dam / 3, because bullets aren't all that good at destroying items...
    smash_items( p, dam / 3 );
}

bool map::hit_with_acid( const tripoint &p )
{
    if( move_cost( p ) != 0 ) {
        return false;    // Didn't hit the tile!
    }
    const ter_id t = ter( p );
    if( t == t_wall_glass || t == t_wall_glass_alarm ||
        t == t_vat ) {
        ter_set( p, t_floor );
    } else if( t == t_door_c || t == t_door_locked || t == t_door_locked_peep || t == t_door_locked_alarm ) {
        if( one_in( 3 ) ) {
            ter_set( p, t_door_b );
        }
    } else if( t == t_door_bar_c || t == t_door_bar_o || t == t_door_bar_locked || t == t_bars ) {
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
    if (move_cost( p ) != 0)
        return false; // Didn't hit the tile!

    // non passable but flammable terrain, set it on fire
    if (has_flag("FLAMMABLE", p ) || has_flag("FLAMMABLE_ASH", p))
    {
        add_field(p, fd_fire, 3, 0);
    }
    return true;
}

bool map::marlossify( const tripoint &p )
{
    if (one_in(25) && (ter_at(p).movecost != 0 && !has_furn(p))
            && !ter_at(p).has_flag(TFLAG_DEEP_WATER)) {
        ter_set(p, t_marloss);
        return true;
    }
    for (int i = 0; i < 25; i++) {
        if(!g->spread_fungus( p )) {
            return true;
        }
    }
    return false;
}

bool map::open_door( const tripoint &p, const bool inside, const bool check_only )
{
    const auto &ter = ter_at( p );
    const auto &furn = furn_at( p );
    int vpart = -1;
    vehicle *veh = veh_at( p, vpart );
    if ( !ter.open.empty() && ter.open != "t_null" ) {
        if ( termap.find( ter.open ) == termap.end() ) {
            debugmsg("terrain %s.open == non existant terrain '%s'\n", ter.id.c_str(), ter.open.c_str() );
            return false;
        }

        if ( has_flag("OPENCLOSE_INSIDE", p) && inside == false ) {
            return false;
        }

        if(!check_only) {
            sounds::sound( p, 6, "", true, "open_door", ter.id );
            ter_set(p, ter.open );
        }

        return true;
    } else if ( !furn.open.empty() && furn.open != "t_null" ) {
        if ( furnmap.find( furn.open ) == furnmap.end() ) {
            debugmsg("terrain %s.open == non existant furniture '%s'\n", furn.id.c_str(), furn.open.c_str() );
            return false;
        }

        if ( has_flag("OPENCLOSE_INSIDE", p) && inside == false ) {
            return false;
        }

        if(!check_only) {
            sounds::sound( p, 6, "", true, "open_door", furn.id );
            furn_set(p, furn.open );
        }

        return true;
    } else if( veh != nullptr ) {
        int openable = veh->next_part_to_open( vpart, true );
        if (openable >= 0) {
            if( !check_only ) {
                veh->open_all_at( openable );
            }

            return true;
        }

        return false;
    }

    return false;
}

void map::translate(const ter_id from, const ter_id to)
{
    if (from == to) {
        debugmsg( "map::translate %s => %s",
                  from.obj().name.c_str(),
                  from.obj().name.c_str() );
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
void map::translate_radius(const ter_id from, const ter_id to, float radi, const tripoint &p )
{
    if( from == to ) {
        debugmsg( "map::translate %s => %s",
                  from.obj().name.c_str(),
                  from.obj().name.c_str() );
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
                float radiX = sqrt(float((uX-x)*(uX-x) + (uY-y)*(uY-y)));
                if( radiX <= radi ){
                    ter_set( t, to);
                }
            }
        }
    }
}

bool map::close_door( const tripoint &p, const bool inside, const bool check_only )
{
 const auto &ter = ter_at( p );
 const auto &furn = furn_at( p );
 if ( !ter.close.empty() && ter.close != "t_null" ) {
     if ( termap.find( ter.close ) == termap.end() ) {
         debugmsg("terrain %s.close == non existant terrain '%s'\n", ter.id.c_str(), ter.close.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", p) && inside == false ) {
         return false;
     }
     if (!check_only) {
        sounds::sound( p, 10, "", true, "close_door", ter.id );
        ter_set(p, ter.close );
     }
     return true;
 } else if ( !furn.close.empty() && furn.close != "t_null" ) {
     if ( furnmap.find( furn.close ) == furnmap.end() ) {
         debugmsg("terrain %s.close == non existant furniture '%s'\n", furn.id.c_str(), furn.close.c_str() );
         return false;
     }
     if ( has_flag("OPENCLOSE_INSIDE", p) && inside == false ) {
         return false;
     }
     if (!check_only) {
         sounds::sound( p, 10, "", true, "close_door", furn.id );
         furn_set(p, furn.close );
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

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );

    return current_submap->get_signage(lx, ly);
}
void map::set_signage( const tripoint &p, std::string message ) const
{
    if( !inbounds( p ) ) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );

    current_submap->set_signage(lx, ly, message);
}
void map::delete_signage( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );

    current_submap->delete_signage(lx, ly);
}

int map::get_radiation( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return 0;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->get_radiation( lx, ly );
}

void map::set_radiation( const int x, const int y, const int value )
{
    set_radiation( tripoint( x, y, abs_sub.z ), value );
}

void map::set_radiation( const tripoint &p, const int value)
{
    if( !inbounds( p ) ) {
        return;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    current_submap->set_radiation( lx, ly, value );
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

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    int current_radiation = current_submap->get_radiation( lx, ly );
    current_submap->set_radiation( lx, ly, current_radiation + delta );
}

int& map::temperature( const tripoint &p )
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
    if( !INBOUNDS(x, y) ) {
        nulitems.clear();
        return map_stack{ &nulitems, tripoint( x, y, abs_sub.z ), this };
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( x, y, lx, ly );

    return map_stack{ &current_submap->itm[lx][ly], tripoint( x, y, abs_sub.z ), this };
}

std::list<item>::iterator map::i_rem( const point location, std::list<item>::iterator it )
{
    return i_rem( tripoint( location, abs_sub.z ), it );
}

int map::i_rem(const int x, const int y, const int index)
{
    return i_rem( tripoint( x, y, abs_sub.z ), index );
}

void map::i_rem(const int x, const int y, item *it)
{
    i_rem( tripoint( x, y, abs_sub.z ), it );
}

void map::i_clear(const int x, const int y)
{
    i_clear( tripoint( x, y, abs_sub.z ) );
}

void map::spawn_an_item(const int x, const int y, item new_item,
                        const long charges, const int damlevel)
{
    spawn_an_item( tripoint( x, y, abs_sub.z ), new_item, charges, damlevel );
}

void map::spawn_items(const int x, const int y, const std::vector<item> &new_items)
{
    spawn_items( tripoint( x, y, abs_sub.z ), new_items );
}

void map::spawn_item(const int x, const int y, const std::string &type_id,
                     const unsigned quantity, const long charges,
                     const unsigned birthday, const int damlevel, const bool rand)
{
    spawn_item( tripoint( x, y, abs_sub.z ), type_id,
                quantity, charges, birthday, damlevel, rand );
}

int map::max_volume(const int x, const int y)
{
    const ter_t &ter = ter_at(x, y);
    if (has_furn(x, y)) {
        return furn_at(x, y).max_volume;
    }
    return ter.max_volume;
}

int map::stored_volume(const int x, const int y)
{
    return stored_volume( tripoint( x, y, abs_sub.z ) );
}

int map::free_volume(const int x, const int y)
{
    return free_volume( tripoint( x, y, abs_sub.z ) );
}

bool map::add_item_or_charges(const int x, const int y, item new_item, int overflow_radius)
{
    return !add_item_or_charges( tripoint( x, y, abs_sub.z ), new_item, overflow_radius ).is_null();
}

void map::add_item(const int x, const int y, item new_item)
{
    add_item( tripoint( x, y, abs_sub.z ), new_item );
}

// Items: 3D

map_stack map::i_at( const tripoint &p )
{
    if( !inbounds(p) ) {
        nulitems.clear();
        return map_stack{ &nulitems, p, this };
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return map_stack{ &current_submap->itm[lx][ly], p, this };
}

std::list<item>::iterator map::i_rem( const tripoint &p, std::list<item>::iterator it )
{
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    if( current_submap->active_items.has( it, point( lx, ly ) ) ) {
        current_submap->active_items.remove( it, point( lx, ly ) );
    }

    current_submap->update_lum_rem(*it, lx, ly);

    return current_submap->itm[lx][ly].erase( it );
}

int map::i_rem(const tripoint &p, const int index)
{
    if( index < 0 ) {
        debugmsg( "i_rem called with negative index %d", index );
        return index;
    }

    if( index >= (int)i_at( p ).size() ) {
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
    auto map_items = i_at(p);

    for( auto iter = map_items.begin(); iter != map_items.end(); iter++ ) {
        //delete the item if the pointer memory addresses are the same
        if(it == &*iter) {
            map_items.erase(iter);
            break;
        }
    }
}

void map::i_clear(const tripoint &p)
{
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    for( auto item_it = current_submap->itm[lx][ly].begin();
         item_it != current_submap->itm[lx][ly].end(); ++item_it ) {
        if( current_submap->active_items.has( item_it, point( lx, ly ) ) ) {
            current_submap->active_items.remove( item_it, point( lx, ly ) );
        }
    }

    current_submap->lum[lx][ly] = 0;
    current_submap->itm[lx][ly].clear();
}

item &map::spawn_an_item(const tripoint &p, item new_item,
                        const long charges, const int damlevel)
{
    if( charges && new_item.charges > 0 ) {
        //let's fail silently if we specify charges for an item that doesn't support it
        new_item.charges = charges;
    }
    new_item = new_item.in_its_container();
    if( (new_item.made_of(LIQUID) && has_flag("SWIMMABLE", p)) ||
        has_flag("DESTROY_ITEM", p) ) {
        return nulitem;
    }
    // bounds checking for damage level
    if( damlevel < -1 ) {
        new_item.damage = -1;
    } else if( damlevel > 4 ) {
        new_item.damage = 4;
    } else {
        new_item.damage = damlevel;
    }

    return add_item_or_charges(p, new_item);
}

std::vector<item*> map::spawn_items(const tripoint &p, const std::vector<item> &new_items)
{
    std::vector<item*> ret;
    if (!inbounds(p) || has_flag("DESTROY_ITEM", p)) {
        return ret;
    }
    const bool swimmable = has_flag("SWIMMABLE", p);
    for( auto new_item : new_items ) {

        if (new_item.made_of(LIQUID) && swimmable) {
            continue;
        }
        item &it = add_item_or_charges(p, new_item);
        if( !it.is_null() ) {
            ret.push_back( &it );
        }
    }

    return ret;
}

void map::spawn_artifact(const tripoint &p)
{
    add_item_or_charges( p, item( new_artifact(), 0 ) );
}

void map::spawn_natural_artifact(const tripoint &p, artifact_natural_property prop)
{
    add_item_or_charges( p, item( new_natural_artifact( prop ), 0 ) );
}

//New spawn_item method, using item factory
// added argument to spawn at various damage levels
void map::spawn_item(const tripoint &p, const std::string &type_id,
                     const unsigned quantity, const long charges,
                     const unsigned birthday, const int damlevel, const bool rand)
{
    if( type_id == "null" ) {
        return;
    }

    if( item_is_blacklisted( type_id ) ) {
        return;
    }
    // recurse to spawn (quantity - 1) items
    for( size_t i = 1; i < quantity; i++ )
    {
        spawn_item( p, type_id, 1, charges, birthday, damlevel );
    }
    // spawn the item
    item new_item(type_id, birthday, rand);
    if( one_in( 3 ) && new_item.has_flag( "VARSIZE" ) ) {
        new_item.item_tags.insert( "FIT" );
    }

    spawn_an_item(p, new_item, charges, damlevel);
}

int map::max_volume(const tripoint &p)
{
    const ter_t &ter = ter_at(p);
    if (has_furn(p)) {
        return furn_at(p).max_volume;
    }
    return ter.max_volume;
}

// total volume of all the things
int map::stored_volume(const tripoint &p) {
    if(!inbounds(p)) {
        return 0;
    }
    int cur_volume = 0;
    for( auto &n : i_at(p) ) {
        cur_volume += n.volume();
    }
    return cur_volume;
}

// free space
int map::free_volume(const tripoint &p) {
   const int maxvolume = this->max_volume(p);
   if(!inbounds(p)) return 0;
   return ( maxvolume - stored_volume(p) );
}

// returns true if full, modified by arguments:
// (none):                            size >= max || volume >= max
// (addvolume >= 0):                  size+1 > max || volume + addvolume > max
// (addvolume >= 0, addnumber >= 0):  size + addnumber > max || volume + addvolume > max
bool map::is_full(const tripoint &p, const int addvolume, const int addnumber ) {
   const int maxitems = MAX_ITEM_IN_SQUARE; // (game.h) 1024
   const int maxvolume = this->max_volume(p);

   if( ! (inbounds(p) && move_cost(p) > 0 && !has_flag("NOITEM", p) ) ) {
       return true;
   }

   if ( addvolume == -1 ) {
       if ( (int)i_at(p).size() < maxitems ) return true;
       int cur_volume=stored_volume(p);
       return (cur_volume >= maxvolume ? true : false );
   } else {
       if ( (int)i_at(p).size() + ( addnumber == -1 ? 1 : addnumber ) > maxitems ) return true;
       int cur_volume=stored_volume(p);
       return ( cur_volume + addvolume > maxvolume ? true : false );
   }

}

// adds an item to map point, or stacks charges.
// returns false if item exceeds tile's weight limits or item count. This function is expensive, and meant for
// user initiated actions, not mapgen!
// overflow_radius > 0: if x,y is full, attempt to drop item up to overflow_radius squares away, if x,y is full
item &map::add_item_or_charges(const tripoint &p, item new_item, int overflow_radius) {

    if(!inbounds(p) ) {
        // Complain about things that should never happen.
        dbg(D_INFO) << p.x << "," << p.y << "," << p.z << ", liquid "
                    <<(new_item.made_of(LIQUID) && has_flag("SWIMMABLE", p)) <<
                    ", destroy_item "<<has_flag("DESTROY_ITEM", p);

        return nulitem;
    }
    if( (new_item.made_of(LIQUID) && has_flag("SWIMMABLE", p)) ||
        has_flag("DESTROY_ITEM", p) || new_item.has_flag("NO_DROP") ||
        (new_item.is_gunmod() && new_item.has_flag("IRREMOVABLE") ) ) {
        // Silently fail on mundane things that prevent item spawn.
        return nulitem;
    }


    const bool tryaddcharges = new_item.charges != -1 && new_item.count_by_charges();
    std::vector<tripoint> ps = closest_tripoints_first(overflow_radius, p);
    for( const auto &p_it : ps ) {
        if( !inbounds(p_it) || new_item.volume() > free_volume(p_it) ||
            has_flag("DESTROY_ITEM", p_it) || has_flag("NOITEM", p_it) ) {
            continue;
        }

        if( tryaddcharges ) {
            for( auto &i : i_at( p_it ) ) {
                if( i.merge_charges( new_item ) ) {
                    return i;
                }
            }
        }

        if( i_at( p_it ).size() < MAX_ITEM_IN_SQUARE ) {
            support_dirty( p_it );
            return add_item( p_it, new_item );
        }
    }

    return nulitem;
}

// Place an item on the map, despite the parameter name, this is not necessaraly a new item.
// WARNING: does -not- check volume or stack charges. player functions (drop etc) should use
// map::add_item_or_charges
item &map::add_item(const tripoint &p, item new_item)
{
    if( !inbounds( p ) ) {
        return nulitem;
    }
    int lx, ly;
    submap * const current_submap = get_submap_at(p, lx, ly);

    // Process foods when they are added to the map, here instead of add_item_at()
    // to avoid double processing food during active item processing.
    if( new_item.needs_processing() && new_item.is_food() ) {
        new_item.process( nullptr, p, false );
    }
    return add_item_at(p, current_submap->itm[lx][ly].end(), new_item);
}

item &map::add_item_at( const tripoint &p,
                        std::list<item>::iterator index, item new_item )
{
    if( new_item.made_of(LIQUID) && has_flag( "SWIMMABLE", p ) ) {
        return nulitem;
    }

    if( has_flag( "DESTROY_ITEM", p ) ) {
        return nulitem;
    }

    if( new_item.has_flag("ACT_IN_FIRE") && get_field( p, fd_fire ) != nullptr ) {
        new_item.active = true;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );
    current_submap->is_uniform = false;

    current_submap->update_lum_add(new_item, lx, ly);
    const auto new_pos = current_submap->itm[lx][ly].insert( index, new_item );
    if( new_item.needs_processing() ) {
        current_submap->active_items.add( new_pos, point(lx, ly) );
    }

    return *new_pos;
}

item map::water_from(const tripoint &p)
{
    if( has_flag( "SALT_WATER", p ) ) {
        item ret( "salt_water", 0 );
        return ret;
    }

    item ret( "water", 0 );
    if( ter( p ) == t_water_sh && one_in( 3 ) ) {
        ret.poison = rng(1, 4);
    } else if( ter( p ) == t_water_dp && one_in( 4 ) ) {
        ret.poison = rng(1, 4);
    } else if( ter( p ) == t_sewage ) {
        ret.poison = rng( 1, 7 );
    }
    return ret;
}

// Check if it's in a fridge and is food, set the fridge
// date to current time, and also check contents.
static void apply_in_fridge(item &it)
{
    if (it.is_food() && it.fridge == 0) {
        it.fridge = (int) calendar::turn;
        // cool down of the HOT flag, is unsigned, don't go below 1
        if ((it.has_flag("HOT")) && (it.item_counter > 10)) {
            it.item_counter -= 10;
        }
        // This sets the COLD flag, and doesn't go above 600
        if ((it.has_flag("EATEN_COLD")) && (!it.has_flag("COLD"))) {
            it.item_tags.insert("COLD");
            it.active = true;
        }
        if ((it.has_flag("COLD")) && (it.item_counter <= 590) && it.fridge > 0) {
            it.item_counter += 10;
        }
    }
    if (it.is_container()) {
        for( auto &elem : it.contents ) {
            apply_in_fridge( elem );
        }
    }
}

template <typename Iterator>
static bool process_item( item_stack &items, Iterator &n, const tripoint &location, bool activate )
{
    // make a temporary copy, remove the item (in advance)
    // and use that copy to process it
    item temp_item = *n;
    auto insertion_point = items.erase( n );
    if( !temp_item.process( nullptr, location, activate ) ) {
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
}

static bool process_map_items( item_stack &items, std::list<item>::iterator &n,
                               const tripoint &location, std::string )
{
    return process_item( items, n, location, false );
}

static void process_vehicle_items( vehicle *cur_veh, int part )
{
    const bool fridge_here = cur_veh->fridge_on && cur_veh->part_flag(part, VPFLAG_FRIDGE);
    if( fridge_here ) {
        for( auto &n : cur_veh->get_items( part ) ) {
            apply_in_fridge(n);
        }
    }
    if( cur_veh->recharger_on && cur_veh->part_with_feature(part, VPFLAG_RECHARGE) >= 0 ) {
        for( auto &n : cur_veh->get_items( part ) ) {
            if( !n.has_flag("RECHARGE") ) {
                continue;
            }
            int full_charge = dynamic_cast<const it_tool*>(n.type)->max_charges;
            if( n.has_flag("DOUBLE_AMMO") ) {
                full_charge = full_charge * 2;
            }
            if( n.is_tool() && full_charge > n.charges ) {
                if( one_in(10) ) {
                    n.charges++;
                }
            }
        }
    }
}

void map::process_active_items()
{
    process_items( true, process_map_items, std::string {} );
}

template<typename T>
void map::process_items( bool const active, T processor, std::string const &signal )
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
                    process_items_in_vehicles(current_submap, processor, signal);
                }
                if( !active || !current_submap->active_items.empty() ) {
                    process_items_in_submap(current_submap, gp, processor, signal);
                }
            }
        }
    }
}

template<typename T>
void map::process_items_in_submap( submap *const current_submap,
                                   const tripoint &gridp,
                                   T processor, std::string const &signal )
{
    // Get a COPY of the active item list for this submap.
    // If more are added as a side effect of processing, they are ignored this turn.
    // If they are destroyed before processing, they don't get processed.
    std::list<item_reference> active_items = current_submap->active_items.get();
    auto const grid_offset = point {gridp.x * SEEX, gridp.y * SEEY};
    for( auto &active_item : active_items ) {
        if( !current_submap->active_items.has( active_item ) ) {
            continue;
        }

        const tripoint map_location = tripoint( grid_offset + active_item.location, gridp.z );
        auto items = i_at( map_location );
        processor( items, active_item.item_iterator, map_location, signal );
    }
}

template<typename T>
void map::process_items_in_vehicles( submap *const current_submap, T processor,
                                     std::string const &signal )
{
    std::vector<vehicle*> const &veh_in_nonant = current_submap->vehicles;
    // a copy, important if the vehicle list changes because a
    // vehicle got destroyed by a bomb (an active item!), this list
    // won't change, but veh_in_nonant will change.
    std::vector<vehicle*> const vehicles = veh_in_nonant;
    for( auto &cur_veh : vehicles ) {
        if (std::find(begin(veh_in_nonant), end(veh_in_nonant), cur_veh) == veh_in_nonant.end()) {
            // vehicle not in the vehicle list of the nonant, has been
            // destroyed (or moved to another nonant?)
            // Can't be sure that it still exists, so skip it
            continue;
        }

        process_items_in_vehicle( cur_veh, current_submap, processor, signal );
    }
}

template<typename T>
void map::process_items_in_vehicle( vehicle *const cur_veh, submap *const current_submap,
                                    T processor, std::string const &signal )
{
    std::vector<int> cargo_parts = cur_veh->all_parts_with_feature(VPFLAG_CARGO, true);
    for( int part : cargo_parts ) {
        process_vehicle_items( cur_veh, part );
    }

    for( auto &active_item : cur_veh->active_items.get() ) {
        if ( cargo_parts.empty() ) {
            return;
        } else if( !cur_veh->active_items.has( active_item ) ) {
            continue;
        }

        auto const it = std::find_if(begin(cargo_parts), end(cargo_parts), [&](int const part) {
            return active_item.location == cur_veh->parts[static_cast<size_t>(part)].mount;
        });

        if (it == std::end(cargo_parts)) {
            continue; // Can't find a cargo part matching the active item.
        }

        // Find the cargo part and coordinates corresponding to the current active item.
        auto const part_index = static_cast<size_t>(*it);
        const point partloc = cur_veh->global_pos() + cur_veh->parts[part_index].precalc[0];
        // TODO: Make this 3D when vehicles know their Z coord
        const tripoint item_location = tripoint( partloc, abs_sub.z );
        auto items = cur_veh->get_items(static_cast<int>(part_index));
        if(!processor(items, active_item.item_iterator, item_location, signal)) {
            // If the item was NOT destroyed, we can skip the remainder,
            // which handles fallout from the vehicle being damaged.
            continue;
        }

        // item does not exist anymore, might have been an exploding bomb,
        // check if the vehicle is still valid (does exist)
        auto const &veh_in_nonant = current_submap->vehicles;
        if(std::find(begin(veh_in_nonant), end(veh_in_nonant), cur_veh) == veh_in_nonant.end()) {
            // Nope, vehicle is not in the vehicle list of the submap,
            // it might have moved to another submap (unlikely)
            // or be destroyed, anywaay it does not need to be processed here
            return;
        }

        // Vehicle still valid, reload the list of cargo parts,
        // the list of cargo parts might have changed (imagine a part with
        // a low index has been removed by an explosion, all the other
        // parts would move up to fill the gap).
        cargo_parts = cur_veh->all_parts_with_feature(VPFLAG_CARGO, false);
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
    const bool container = has_flag_ter_or_furn( "CONTAINER", p );
    const bool sealed = has_flag_ter_or_furn( "SEALED", p );
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

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );

    return !current_submap->itm[lx][ly].empty();
}

template <typename Stack>
std::list<item> use_amount_stack( Stack stack, const itype_id type, long &quantity )
{
    std::list<item> ret;
    for( auto a = stack.begin(); a != stack.end() && quantity > 0; ) {
        if( a->use_amount(type, quantity, ret) ) {
            a = stack.erase( a );
        } else {
            ++a;
        }
    }
    return ret;
}

std::list<item> map::use_amount_square( const tripoint &p, const itype_id type,
                                        long &quantity )
{
    std::list<item> ret;
    int vpart = -1;
    vehicle *veh = veh_at( p, vpart );

    if( veh ) {
        const int cargo = veh->part_with_feature(vpart, "CARGO");
        if( cargo >= 0 ) {
            std::list<item> tmp = use_amount_stack( veh->get_items(cargo), type,
                                                    quantity );
            ret.splice( ret.end(), tmp );
        }
    }
    std::list<item> tmp = use_amount_stack( i_at( p ), type, quantity );
    ret.splice( ret.end(), tmp );
    return ret;
}

std::list<item> map::use_amount( const tripoint &origin, const int range, const itype_id type,
                                 long &quantity )
{
    std::list<item> ret;
    for( int radius = 0; radius <= range && quantity > 0; radius++ ) {
        tripoint p( origin.x - radius, origin.y - radius, origin.z );
        int &x = p.x;
        int &y = p.y;
        for( x = origin.x - radius; x <= origin.x + radius; x++ ) {
            for( y = origin.y - radius; y <= origin.y + radius; y++ ) {
                if( rl_dist( origin, p ) >= radius ) {
                    std::list<item> tmp = use_amount_square( p , type, quantity );
                    ret.splice( ret.end(), tmp );
                }
            }
        }
    }
    return ret;
}

template <typename Stack>
std::list<item> use_charges_from_stack( Stack stack, const itype_id type, long &quantity)
{
    std::list<item> ret;
    for( auto a = stack.begin(); a != stack.end() && quantity > 0; ) {
        if( !a->made_of(LIQUID) && a->use_charges(type, quantity, ret) ) {
            a = stack.erase( a );
        } else {
            ++a;
        }
    }
    return ret;
}

long remove_charges_in_list(const itype *type, map_stack stack, long quantity)
{
    auto target = stack.begin();
    for( ; target != stack.end(); ++target ) {
        if( target->type == type ) {
            break;
        }
    }

    if( target != stack.end() ) {
        if( target->charges > quantity) {
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
                            map *m, const tripoint &p, std::list<item> &ret )
{
    itype *itt = f.crafting_pseudo_item_type();
    if (itt == NULL || itt->id != type) {
        return;
    }
    const itype *ammo = f.crafting_ammo_item_type();
    if (ammo != NULL) {
        item furn_item(itt->id, 0);
        furn_item.charges = remove_charges_in_list(ammo, m->i_at( p ), quantity);
        if (furn_item.charges > 0) {
            ret.push_back(furn_item);
            quantity -= furn_item.charges;
        }
    }
}

std::list<item> map::use_charges(const tripoint &origin, const int range,
                                 const itype_id type, long &quantity)
{
    std::list<item> ret;
    for( const tripoint &p : closest_tripoints_first( range, origin ) ) {
        if( has_furn( p ) && accessible_furniture( origin, p, range ) ) {
            use_charges_from_furn( furn_at( p ), type, quantity, this, p, ret );
            if( quantity <= 0 ) {
                return ret;
            }
        }

        if( !accessible_items( origin, p, range ) ) {
            continue;
        }

        std::list<item> tmp = use_charges_from_stack( i_at( p ), type, quantity );
        ret.splice(ret.end(), tmp);
        if (quantity <= 0) {
            return ret;
        }

        int vpart = -1;
        vehicle *veh = veh_at( p, vpart );
        if( veh == nullptr ) {
            continue;
        }

        const int kpart = veh->part_with_feature(vpart, "KITCHEN");
        const int weldpart = veh->part_with_feature(vpart, "WELDRIG");
        const int craftpart = veh->part_with_feature(vpart, "CRAFTRIG");
        const int forgepart = veh->part_with_feature(vpart, "FORGE");
        const int chempart = veh->part_with_feature(vpart, "CHEMLAB");
        const int cargo = veh->part_with_feature(vpart, "CARGO");

        if (kpart >= 0) { // we have a kitchen, now to see what to drain
            ammotype ftype = "NULL";

            if (type == "water_clean") {
                ftype = "water_clean";
            } else if (type == "water") {
                ftype = "water";
            } else if (type == "hotplate") {
                ftype = "battery";
            }

            item tmp(type, 0); //TODO add a sane birthday arg
            tmp.charges = veh->drain(ftype, quantity);
            // TODO: Handle water poison when crafting starts respecting it
            quantity -= tmp.charges;
            ret.push_back(tmp);

            if (quantity == 0) {
                return ret;
            }
        }

        if (weldpart >= 0) { // we have a weldrig, now to see what to drain
            ammotype ftype = "NULL";

            if (type == "welder") {
                ftype = "battery";
            } else if (type == "soldering_iron") {
                ftype = "battery";
            }

            item tmp(type, 0); //TODO add a sane birthday arg
            tmp.charges = veh->drain(ftype, quantity);
            quantity -= tmp.charges;
            ret.push_back(tmp);

            if (quantity == 0) {
                return ret;
            }
        }

        if (craftpart >= 0) { // we have a craftrig, now to see what to drain
            ammotype ftype = "NULL";

            if (type == "press") {
                ftype = "battery";
            } else if (type == "vac_sealer") {
                ftype = "battery";
            } else if (type == "dehydrator") {
                ftype = "battery";
            }

            item tmp(type, 0); //TODO add a sane birthday arg
            tmp.charges = veh->drain(ftype, quantity);
            quantity -= tmp.charges;
            ret.push_back(tmp);

            if (quantity == 0) {
                return ret;
            }
        }

        if (forgepart >= 0) { // we have a veh_forge, now to see what to drain
            ammotype ftype = "NULL";

            if (type == "forge") {
                ftype = "battery";
            }

            item tmp(type, 0); //TODO add a sane birthday arg
            tmp.charges = veh->drain(ftype, quantity);
            quantity -= tmp.charges;
            ret.push_back(tmp);

            if (quantity == 0) {
                return ret;
            }
        }

        if (chempart >= 0) { // we have a chem_lab, now to see what to drain
            ammotype ftype = "NULL";

            if (type == "chemistry_set") {
                ftype = "battery";
            } else if (type == "hotplate") {
                ftype = "battery";
            }

            item tmp(type, 0); //TODO add a sane birthday arg
            tmp.charges = veh->drain(ftype, quantity);
            quantity -= tmp.charges;
            ret.push_back(tmp);

            if (quantity == 0) {
                return ret;
            }
        }

        if (cargo >= 0) {
            std::list<item> tmp =
                use_charges_from_stack( veh->get_items(cargo), type, quantity );
            ret.splice(ret.end(), tmp);
            if (quantity <= 0) {
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
    (void)z;
    pos.z = abs_sub.z;
    for( pos.x = 0; pos.x < SEEX * MAPSIZE; pos.x++ ) {
        if( x != -1 && x != pos.x ) {
            continue;
        }
        for( pos.y = 0; pos.y < SEEY * MAPSIZE; pos.y++ ) {
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
                                const tripoint &pos,
                                std::string signal )
{
    bool trigger_item = false;
    // Check for charges != 0 not >0, so that -1 charge tools can still be used
    if( n->charges != 0 && n->has_flag("RADIO_ACTIVATION") && n->has_flag(signal) ) {
        sounds::sound(pos, 6, _("beep."));
        if( n->has_flag("RADIO_INVOKE_PROC") ) {
            // Invoke twice: first to transform, then later to proc
            // Can't use process_item here - invalidates our iterator
            n->process( nullptr, pos, true );
        }
        if( n->has_flag("BOMB") ) {
            // Set charges to 0 to ensure it detonates now
            n->charges = 0;
        }
        trigger_item = true;
    } else if( n->has_flag("RADIO_CONTAINER") && !n->contents.empty() &&
               n->contents[0].has_flag( signal ) ) {
        // A bomb is the only thing meaningfully placed in a container,
        // If that changes, this needs logic to handle the alternative.
        itype_id bomb_type = n->contents[0].type->id;

        n->make(bomb_type);
        if( n->has_flag("RADIO_INVOKE_PROC") ) {
            n->process( nullptr, pos, true );
        }

        n->charges = 0;
        trigger_item = true;
    }
    if( trigger_item ) {
        return process_item( items, n, pos, true );
    }
    return false;
}

void map::trigger_rc_items( std::string signal )
{
    process_items( false, trigger_radio_item, signal );
}

item *map::item_from( const tripoint &pos, size_t index ) {
    auto items = i_at( pos );

    if( index >= items.size() ) {
        return nullptr;
    } else {
        return &items[index];
    }
}

item *map::item_from( vehicle *veh, int cargo_part, size_t index ) {
   auto items = veh->get_items( cargo_part );

    if( index >= items.size() ) {
        return nullptr;
    } else {
        return &items[index];
    }
}

void map::trap_set( const tripoint &p, const trap_id id)
{
    add_trap( p, id );
}

const trap &map::tr_at( const tripoint &p ) const
{
    if( !inbounds( p.x, p.y, p.z ) ) {
        return tr_null.obj();
    }

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );

    if (current_submap->get_ter( lx, ly ).obj().trap != tr_null) {
        return current_submap->get_ter( lx, ly ).obj().trap.obj();
    }

    return current_submap->get_trap( lx, ly ).obj();
}

void map::add_trap( const tripoint &p, const trap_id t)
{
    if( !inbounds( p ) )
    {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );
    const ter_t &ter = current_submap->get_ter( lx, ly ).obj();
    if( ter.trap != tr_null ) {
        debugmsg( "set trap %s on top of terrain %s which already has a builit-in trap",
                  t.obj().name.c_str(), ter.name.c_str() );
        return;
    }

    // If there was already a trap here, remove it.
    if( current_submap->get_trap( lx, ly ) != tr_null ) {
        remove_trap( p );
    }

    current_submap->set_trap( lx, ly, t );
    if( t != tr_null ) {
        traplocs[t].push_back( p );
    }
}

void map::disarm_trap( const tripoint &p )
{
    const trap &tr = tr_at( p );
    if( tr.is_null() ) {
        debugmsg( "Tried to disarm a trap where there was none (%d %d %d)", p.x, p.y, p.z );
        return;
    }

    const int tSkillLevel = g->u.skillLevel( skill_traps );
    const int diff = tr.get_difficulty();
    int roll = rng(tSkillLevel, 4 * tSkillLevel);

    // Some traps are not actual traps. Skip the rolls, different message and give the option to grab it right away.
    if( tr.get_avoidance() ==  0 && tr.get_difficulty() == 0 ) {
        add_msg(_("You take down the %s."), tr.name.c_str());
        tr.on_disarmed( p );
        return;
    }

    ///\EFFECT_PER increases chance of disarming trap

    ///\EFFECT_DEX increases chance of disarming trap

    ///\EFFECT_TRAPS increases chance of disarming trap
    while ((rng(5, 20) < g->u.per_cur || rng(1, 20) < g->u.dex_cur) && roll < 50) {
        roll++;
    }
    if (roll >= diff) {
        add_msg(_("You disarm the trap!"));
        tr.on_disarmed( p );
        if(diff > 1.25 * tSkillLevel) { // failure might have set off trap
            g->u.practice( skill_traps, 1.5*(diff - tSkillLevel) );
        }
    } else if (roll >= diff * .8) {
        add_msg(_("You fail to disarm the trap."));
        if(diff > 1.25 * tSkillLevel) {
            g->u.practice( skill_traps, 1.5*(diff - tSkillLevel) );
        }
    } else {
        add_msg(m_bad, _("You fail to disarm the trap, and you set it off!"));
        tr.trigger( p, &g->u );
        if(diff - roll <= 6) {
            // Give xp for failing, but not if we failed terribly (in which
            // case the trap may not be disarmable).
            g->u.practice( skill_traps, 2*diff );
        }
    }
}

void map::remove_trap( const tripoint &p )
{
    if( !inbounds( p ) ) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );

    trap_id t = current_submap->get_trap(lx, ly);
    if (t != tr_null) {
        if( g != nullptr && this == &g->m ) {
            g->u.add_known_trap( p, tr_null.obj() );
        }

        current_submap->set_trap(lx, ly, tr_null);
        auto &traps = traplocs[t];
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

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->fld[lx][ly];
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

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->fld[lx][ly];
}

int map::adjust_field_age( const tripoint &p, const field_id t, const int offset ) {
    return set_field_age( p, t, offset, true);
}

int map::adjust_field_strength( const tripoint &p, const field_id t, const int offset ) {
    return set_field_strength( p, t, offset, true );
}

/*
 * Set age of field type at point, or increment/decrement if offset=true
 * returns resulting age or -1 if not present.
 */
int map::set_field_age( const tripoint &p, const field_id t, const int age, bool isoffset ) {
    field_entry *field_ptr = get_field( p, t );
    if( field_ptr != nullptr ) {
        int adj = ( isoffset ? field_ptr->getFieldAge() : 0 ) + age;
        field_ptr->setFieldAge( adj );
        return adj;
    }

    return -1;
}

/*
 * set strength of field type at point, creating if not present, removing if strength is 0
 * returns resulting strength, or 0 for not present
 */
int map::set_field_strength( const tripoint &p, const field_id t, const int str, bool isoffset ) {
    field_entry * field_ptr = get_field( p, t );
    if( field_ptr != nullptr ) {
        int adj = ( isoffset ? field_ptr->getFieldDensity() : 0 ) + str;
        if( adj > 0 ) {
            field_ptr->setFieldDensity( adj );
            return adj;
        } else {
            remove_field( p, t );
            return 0;
        }
    } else if( 0 + str > 0 ) {
        return ( add_field( p, t, str, 0 ) ? str : 0 );
    }

    return 0;
}

int map::get_field_age( const tripoint &p, const field_id t ) const
{
    auto field_ptr = field_at( p ).findField( t );
    return ( field_ptr == nullptr ? -1 : field_ptr->getFieldAge() );
}

int map::get_field_strength( const tripoint &p, const field_id t ) const
{
    auto field_ptr = field_at( p ).findField( t );
    return ( field_ptr == nullptr ? 0 : field_ptr->getFieldDensity() );
}

field_entry *map::get_field( const tripoint &p, const field_id t ) {
    if( !inbounds( p ) ) {
        return nullptr;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );

    return current_submap->fld[lx][ly].findField( t );
}

bool map::add_field(const tripoint &p, const field_id t, int density, const int age)
{
    if( !inbounds( p ) ) {
        return false;
    }

    if( density > 3) {
        density = 3;
    }

    if( density <= 0) {
        return false;
    }

    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    current_submap->is_uniform = false;

    if( current_submap->fld[lx][ly].addField( t, density, age ) ) {
        //Only adding it to the count if it doesn't exist.
        current_submap->field_count++;
    }

    if( g != nullptr && this == &g->m && p == g->u.pos3() ) {
        creature_in_field( g->u ); //Hit the player with the field if it spawned on top of them.
    }

    // Dirty the transparency cache now that field processing doesn't always do it
    // TODO: Make it skip transparent fields
    set_transparency_cache_dirty( p.z );
    return true;
}

void map::remove_field( const tripoint &p, const field_id field_to_remove )
{
    if( !inbounds( p ) ) {
        return;
    }

    int lx, ly;
    submap * const current_submap = get_submap_at( p, lx, ly );

    if( current_submap->fld[lx][ly].removeField( field_to_remove ) ) {
        // Only adjust the count if the field actually existed.
        current_submap->field_count--;
        const auto &fdata = fieldlist[ field_to_remove ];
        for( int i = 0; i < 3; ++i ) {
            if( !fdata.transparent[i] ) {
                set_transparency_cache_dirty( p.z );
                break;
            }
        }
    }
}

computer* map::computer_at( const tripoint &p )
{
    if( !inbounds( p ) ) {
        return nullptr;
    }

    submap * const current_submap = get_submap_at( p );

    if( current_submap->comp.name.empty() ) {
        return nullptr;
    }

    return &(current_submap->comp);
}

bool map::allow_camp( const tripoint &p, const int radius)
{
    return camp_at( p, radius ) == nullptr;
}

// locate the nearest camp in some radius (default CAMPSIZE)
basecamp* map::camp_at( const tripoint &p, const int radius)
{
    if( !inbounds( p ) ) {
        return nullptr;
    }

    const int sx = std::max(0, p.x / SEEX - radius);
    const int sy = std::max(0, p.y / SEEY - radius);
    const int ex = std::min(MAPSIZE - 1, p.x / SEEX + radius);
    const int ey = std::min(MAPSIZE - 1, p.y / SEEY + radius);

    for (int ly = sy; ly < ey; ++ly) {
        for (int lx = sx; lx < ex; ++lx) {
            submap * const current_submap = get_submap_at( p );
            if( current_submap->camp.is_valid() ) {
                // we only allow on camp per size radius, kinda
                return &(current_submap->camp);
            }
        }
    }

    return nullptr;
}

void map::add_camp( const tripoint &p, const std::string& name )
{
    if( !allow_camp( p ) ) {
        dbg(D_ERROR) << "map::add_camp: Attempting to add camp when one in local area.";
        return;
    }

    get_submap_at( p )->camp = basecamp( name, p.x, p.y );
}

void map::debug()
{
 mvprintw(0, 0, "MAP DEBUG");
 getch();
 for (int i = 0; i <= SEEX * 2; i++) {
  for (int j = 0; j <= SEEY * 2; j++) {
   if (i_at(i, j).size() > 0) {
    mvprintw(1, 0, "%d, %d: %d items", i, j, i_at(i, j).size());
    mvprintw(2, 0, "%c, %d", i_at(i, j)[0].symbol(), i_at(i, j)[0].color());
    getch();
   }
  }
 }
 getch();
}

void map::update_visibility_cache( visibility_variables &cache, const int zlev ) {
    cache.variables_set = true; // Not used yet
    cache.g_light_level = (int)g->light_level( zlev );
    cache.vision_threshold = g->u.get_vision_threshold(
        get_cache_ref(g->u.posz()).lm[g->u.posx()][g->u.posy()] );

    cache.u_clairvoyance = g->u.clairvoyance();
    cache.u_sight_impaired = g->u.sight_impaired();
    cache.u_is_boomered = g->u.has_effect("boomered");

    int sm_squares_seen[MAPSIZE][MAPSIZE];
    std::memset(sm_squares_seen, 0, sizeof(sm_squares_seen));

    auto &visibility_cache = get_cache( zlev ).visibility_cache;

    tripoint p;
    p.z = zlev;
    int &x = p.x;
    int &y = p.y;
    for( x = 0; x < MAPSIZE * SEEX; x++ ) {
        for( y = 0; y < MAPSIZE * SEEY; y++ ) {
            lit_level ll = apparent_light_at( p, cache );
            visibility_cache[x][y] = ll;
            sm_squares_seen[ x / SEEX ][ y / SEEY ] += (ll == LL_BRIGHT || ll == LL_LIT);
        }
    }

    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            if( sm_squares_seen[gridx][gridy] > 36 ) { // 25% of the submap is visible
                const tripoint sm( gridx, gridy, 0 );
                const auto abs_sm = map::abs_sub + sm;
                const auto abs_omt = overmapbuffer::sm_to_omt_copy( abs_sm );
                overmap_buffer.set_seen( abs_omt.x, abs_omt.y, abs_omt.z, true);
            }
        }
    }
}

lit_level map::apparent_light_at( const tripoint &p, const visibility_variables &cache ) const {
    const int dist = rl_dist(g->u.posx(), g->u.posy(), p.x, p.y);

    // Clairvoyance overrides everything.
    if( dist <= cache.u_clairvoyance ) {
        return LL_BRIGHT;
    }
    const auto &map_cache = get_cache_ref(p.z);
    bool obstructed = map_cache.seen_cache[p.x][p.y] <= LIGHT_TRANSPARENCY_SOLID + 0.1;
    const float apparent_light = map_cache.seen_cache[p.x][p.y] * map_cache.lm[p.x][p.y];

    // Unimpaired range is an override to strictly limit vision range based on various conditions,
    // but the player can still see light sources.
    if( dist > g->u.unimpaired_range() ) {
        if( !obstructed && map_cache.sm[p.x][p.y] > 0.0) {
            return LL_BRIGHT_ONLY;
        } else {
            return LL_DARK;
        }
    }
    if( obstructed ) {
        if( apparent_light > LIGHT_AMBIENT_LIT ) {
            if( apparent_light > cache.g_light_level ) {
                // This represents too hazy to see detail,
                // but enough light getting through to illuminate.
                return LL_BRIGHT_ONLY;
            } else {
                // If it's not brighter than the surroundings, it just ends up shadowy.
                return LL_LOW;
            }
        } else {
            return LL_BLANK;
        }
    }
    // Then we just search for the light level in descending order.
    if( apparent_light > LIGHT_SOURCE_BRIGHT || map_cache.sm[p.x][p.y] > 0.0 ) {
        return LL_BRIGHT;
    }
    if( apparent_light > LIGHT_AMBIENT_LIT ) {
        return LL_LIT;
    }
    if( apparent_light > cache.vision_threshold ) {
        return LL_LOW;
    } else {
        return LL_BLANK;
    }
    // Is this ever supposed to happen?
    return LL_DARK;
}

visibility_type map::get_visibility( const lit_level ll, const visibility_variables &cache ) const {
    switch (ll) {
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
        return VIS_HIDDEN;
    }
    return VIS_HIDDEN;
}

bool map::apply_vision_effects( WINDOW *w, lit_level ll,
                                const visibility_variables &cache ) const {
    int symbol = ' ';
    nc_color color = c_black;

    switch( get_visibility(ll, cache) ) {
        case VIS_CLEAR:
            // Drew the tile, so bail out now.
            return false;
        case VIS_LIT: // can only tell that this square is bright
            symbol = '#';
            color = c_ltgray;
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

void map::draw( WINDOW* w, const tripoint &center )
{
    // We only need to draw anything if we're not in tiles mode.
    if( is_draw_tiles_mode() ) {
        return;
    }

    g->reset_light_level();

    visibility_variables cache;
    update_visibility_cache( cache, center.z );

    const auto &visibility_cache = get_cache_ref( center.z ).visibility_cache;

    // X and y are in map coordinates, but might be out of range of the map.
    // When they are out of range, we just draw '#'s.
    tripoint p;
    p.z = center.z;
    int &x = p.x;
    int &y = p.y;
    for( y = center.y - getmaxy(w) / 2; y <= center.y + getmaxy(w) / 2; y++ ) {
        if( y - center.y + getmaxy(w) / 2 >= getmaxy(w) ){
            continue;
        }

        wmove( w, y - center.y + getmaxy(w) / 2, 0 );

        if( y < 0 || y >= MAPSIZE * SEEY ) {
            for( int x = 0; x < getmaxx(w); x++ ) {
                wputch( w, c_black, ' ' );
            }
            continue;
        }

        x = center.x - getmaxx(w) / 2;
        while( x < 0 ) {
            wputch( w, c_black, ' ' );
            x++;
        }

        int lx;
        int ly;
        const int maxxrender = center.x - getmaxx(w) / 2 + getmaxx(w);
        const int maxx = std::min( MAPSIZE * SEEX, maxxrender );
        while( x < maxx ) {
            submap *cur_submap = get_submap_at( p, lx, ly );
            submap *sm_below = p.z > -OVERMAP_DEPTH ?
                get_submap_at( p.x, p.y, p.z - 1, lx, ly ) : cur_submap;
            while( lx < SEEX && x < maxx )  {
                const lit_level lighting = visibility_cache[x][y];
                if( !apply_vision_effects( w, lighting, cache ) ) {
                    const maptile curr_maptile = maptile( cur_submap, lx, ly );
                    const bool just_this_zlevel =
                        draw_maptile( w, g->u, p, curr_maptile,
                                      false, true, center,
                                      lighting == LL_LOW, lighting == LL_BRIGHT, true );
                    if( !just_this_zlevel ) {
                        p.z--;
                        const maptile tile_below = maptile( sm_below, lx, ly );
                        draw_from_above( w, g->u, p, tile_below, false, center,
                                         lighting == LL_LOW, lighting == LL_BRIGHT, false );
                        p.z++;
                    }
                }

                lx++;
                x++;
            }
        }

        while( x < maxxrender ) {
            wputch( w, c_black, ' ' );
            x++;
        }
    }

    if( g->u.posz() == center.z ) {
        g->draw_critter( g->u, center );
    }
}

void map::drawsq( WINDOW* w, player &u, const tripoint &p,
                  const bool invert, const bool show_items ) const
{
    drawsq( w, u, p, invert, show_items, u.pos(), false, false, false );
}

void map::drawsq( WINDOW* w, player &u, const tripoint &p, const bool invert_arg,
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

bool map::draw_maptile( WINDOW* w, player &u, const tripoint &p, const maptile &curr_maptile,
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
    static const long AUTO_WALL_PLACEHOLDER = 2; // this should never appear as a real symbol!

    if( curr_furn.loadid != f_null ) {
        sym = curr_furn.symbol();
        tercol = curr_furn.color();
    } else {
        if( curr_ter.has_flag( TFLAG_AUTO_WALL_SYMBOL ) ) {
            // If the terrain symbol is later overriden by something, we don't need to calculate
            // the wall symbol at all. This case will be detected by comparing sym to this
            // placeholder, if it's still the same, we have to calculate the wall symbol.
            sym = AUTO_WALL_PLACEHOLDER;
        } else {
            sym = curr_ter.symbol();
        }
        tercol = curr_ter.color();
    }
    if( curr_ter.has_flag( TFLAG_SWIMMABLE ) && curr_ter.has_flag( TFLAG_DEEP_WATER ) && !u.is_underwater() ) {
        show_items = false; // Can only see underwater items if WE are underwater
    }
    // If there's a trap here, and we have sufficient perception, draw that instead
    if( curr_trap.can_see( p, g->u ) ) {
        tercol = curr_trap.color;
        if (curr_trap.sym == '%') {
            switch(rng(1, 5)) {
            case 1: sym = '*'; break;
            case 2: sym = '0'; break;
            case 3: sym = '8'; break;
            case 4: sym = '&'; break;
            case 5: sym = '+'; break;
            }
        } else {
            sym = curr_trap.sym;
        }
    }
    if( curr_field.fieldCount() > 0 ) {
        const field_id& fid = curr_field.fieldSymbol();
        const field_entry* fe = curr_field.findField(fid);
        const field_t& f = fieldlist[fid];
        if (f.sym == '&' || fe == NULL) {
            // Do nothing, a '&' indicates invisible fields.
        } else if (f.sym == '*') {
            // A random symbol.
            switch (rng(1, 5)) {
            case 1: sym = '*'; break;
            case 2: sym = '0'; break;
            case 3: sym = '8'; break;
            case 4: sym = '&'; break;
            case 5: sym = '+'; break;
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
            draw_item_sym = (f.sym == '%');
            // If field priority is > 1, and the field is set to hide items,
            //draw the field as it obscures what's under it.
            if( (f.sym != '%' && f.priority > 1) || (f.sym != '%' && sym == '.'))  {
                // default terrain '.' and
                // non-default field symbol -> field symbol overrides terrain
                sym = f.sym;
            }
            tercol = f.color[fe->getFieldDensity() - 1];
        }
    }

    // If there are items here, draw those instead
    if( show_items && curr_maptile.get_item_count() > 0 && sees_some_items( p, g->u ) ) {
        // if there's furniture/terrain/trap/fields (sym!='.')
        // and we should not override it, then only highlight the square
        if (sym != '.' && sym != '%' && !draw_item_sym) {
            hi = true;
        } else {
            // otherwise override with the symbol of the last item
            sym = curr_maptile.get_last_item().symbol();
            if (!draw_item_sym) {
                tercol = curr_maptile.get_last_item().color();
            }
            if( curr_maptile.get_item_count() > 1 ) {
                invert = !invert;
            }
        }
    }

    int veh_part = 0;
    const vehicle *veh = veh_at_internal( p, veh_part );
    if( veh != nullptr ) {
        sym = special_symbol( veh->face.dir_symbol( veh->part_sym( veh_part ) ) );
        tercol = veh->part_color( veh_part );
    }

    // If there's graffiti here, change background color
    if( curr_maptile.has_graffiti() ) {
        graf = true;
    }

    //suprise, we're not done, if it's a wall adjacent to an other, put the right glyph
    if( sym == AUTO_WALL_PLACEHOLDER ) {
        sym = determine_wall_corner( p );
    }

    const auto u_vision = u.get_vision_modes();
    if( u_vision[BOOMERED] ) {
        tercol = c_magenta;
    } else if( u_vision[NV_GOGGLES] ) {
        tercol = (bright_light) ? c_white : c_ltgreen;
    } else if( low_light ) {
        tercol = c_dkgray;
    } else if( u_vision[DARKNESS] ) {
        tercol = c_dkgray;
    }

    if( invert ) {
        tercol = invert_color(tercol);
    } else if( hi ) {
        tercol = hilite(tercol);
    } else if( graf ) {
        tercol = red_background(tercol);
    }

    if( inorder ) {
        // Rastering the whole map, take advantage of automatically moving the cursor.
        wputch(w, tercol, sym);
    } else {
        // Otherwise move the cursor before drawing.
        const int k = p.x + getmaxx(w) / 2 - view_center.x;
        const int j = p.y + getmaxy(w) / 2 - view_center.y;
        mvwputch(w, j, k, tercol, sym);
    }

    return !zlevels || sym != ' ' || p.z <= -OVERMAP_DEPTH || !curr_ter.has_flag( TFLAG_NO_FLOOR );
}

void map::draw_from_above( WINDOW* w, player &u, const tripoint &p,
                           const maptile &curr_tile,
                           const bool invert,
                           const tripoint &view_center,
                           bool low_light, bool bright_light, bool inorder ) const
{
    static const long AUTO_WALL_PLACEHOLDER = 2; // this should never appear as a real symbol!

    nc_color tercol = c_dkgray;
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
        tercol = (roof >= 0 || veh->obstacle_at_part( part_below ) ) ? c_ltgray : c_ltgray_cyan;
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
        tercol = (bright_light) ? c_white : c_ltgreen;
    } else if( low_light ) {
        tercol = c_dkgray;
    } else if( u_vision[DARKNESS] ) {
        tercol = c_dkgray;
    }

    if( invert ) {
        tercol = invert_color( tercol );
    }

    if( inorder ) {
        wputch( w, tercol, sym );
    } else {
        const int k = p.x + getmaxx(w) / 2 - view_center.x;
        const int j = p.y + getmaxy(w) / 2 - view_center.y;
        mvwputch(w, j, k, tercol, sym);
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
    if( (range >= 0 && range < rl_dist( F, T )) ||
        !inbounds( T ) ) {
        bresenham_slope = 0;
        return false; // Out of range!
    }
    bool visible = true;

    // Ugly `if` for now
    if( !fov_3d ) {
        bresenham( F.x, F.y, T.x, T.y, bresenham_slope,
                   [this, &visible, &T]( const point &new_point ) {
                       // Exit before checking the last square, it's still visible even if opaque.
                       if( new_point.x == T.x && new_point.y == T.y ) {
                           return false;
                       }
                       if( !this->trans( tripoint( new_point, T.z ) ) ) {
                           visible = false;
                           return false;
                       }
                       return true;
                   });
        return visible;
    }

    bresenham( F, T, bresenham_slope, 0,
                [this, &visible, &T]( const tripoint &new_point ) {
               // Exit before checking the last square, it's still visible even if opaque.
               if( new_point == T ) {
                   return false;
               }
               if( !this->trans( new_point ) ) {
                   visible = false;
                   return false;
               }
               return true;
           });
    return visible;
}

// This method tries a bunch of initial offsets for the line to try and find a clear one.
// Basically it does, "Find a line from any point in the source that ends up in the target square".
std::vector<tripoint> map::find_clear_path( const tripoint &source, const tripoint &destination ) const
{
    // TODO: Push this junk down into the bresenham method, it's already doing it.
    const int dx = destination.x - source.x;
    const int dy = destination.y - source.y;
    const int ax = std::abs(dx) * 2;
    const int ay = std::abs(dy) * 2;
    const int dominant = std::max(ax, ay);
    const int minor = std::min(ax, ay);
    // This seems to be the method for finding the ideal start value for the error value.
    const int ideal_start_offset = minor - (dominant / 2);
    const int start_sign = (ideal_start_offset > 0) - (ideal_start_offset < 0);
    // Not totally sure of the derivation.
    const int max_start_offset = std::abs(ideal_start_offset) * 2 + 1;
    for ( int horizontal_offset = -1; horizontal_offset <= max_start_offset; ++horizontal_offset ) {
        int candidate_offset = horizontal_offset * start_sign;
        if( sees( source, destination, rl_dist(source, destination), candidate_offset ) ) {
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
    if( !fov_3d ) {
        if( f.z != t.z ) {
            return false;
        }

        if( (range >= 0 && range < rl_dist(f.x, f.y, t.x, t.y)) ||
            !INBOUNDS(t.x, t.y) ) {
            return false; // Out of range!
        }
        bool is_clear = true;
        bresenham( f.x, f.y, t.x, t.y, 0,
                   [this, &is_clear, cost_min, cost_max, &t](const point &new_point ) {
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

    if( (range >= 0 && range < rl_dist(f, t)) ||
        !inbounds( t ) ) {
        return false; // Out of range!
    }
    bool is_clear = true;
    bresenham( f, t, 0, 0,
               [this, &is_clear, cost_min, cost_max, t](const tripoint &new_point ) {
                   // Exit before checking the last square, it's still reachable even if it is an obstacle.
                   if( new_point == t ) {
                       return false;
                   }

                   const int cost = this->move_cost( new_point );
                   if( cost < cost_min || cost > cost_max ) {
                       is_clear = false;
                       return false;
                   }
                   return true;
               } );
    return is_clear;
}

bool map::accessible_items( const tripoint &f, const tripoint &t, const int range ) const
{
    return ( !has_flag( "SEALED", t ) || has_flag( "LIQUIDCONT", t ) ) &&
           ( f == t || clear_path( f, t, range, 1, 100 ) );
}

bool map::accessible_furniture( const tripoint &f, const tripoint &t, const int range ) const
{
    return ( f == t || clear_path( f, t, range, 1, 100 ) );
}

std::vector<tripoint> map::get_dir_circle( const tripoint &f, const tripoint &t ) const
{
    std::vector<tripoint> circle;
    circle.resize(8);

    // The line below can be crazy expensive - we only take the FIRST point of it
    const std::vector<tripoint> line = line_to( f, t, 0, 0 );
    const std::vector<tripoint> spiral = closest_tripoints_first( 1, f );
    const std::vector<int> pos_index {1,2,4,6,8,7,5,3};

    //  All possible constelations (closest_points_first goes clockwise)
    //  753  531  312  124  246  468  687  875
    //  8 1  7 2  5 4  3 6  1 8  2 7  4 5  6 3
    //  642  864  786  578  357  135  213  421

    size_t pos_offset = 0;
    for( unsigned int i = 1; i < spiral.size(); i++ ) {
        if( spiral[i] == line[0] ) {
            pos_offset = i-1;
            break;
        }
    }

    for( unsigned int i = 1; i < spiral.size(); i++ ) {
        if( pos_offset >= pos_index.size() ) {
            pos_offset = 0;
        }

        circle[pos_index[pos_offset++]-1] = spiral[i];
    }

    return circle;
}

int map::coord_to_angle ( const int x, const int y, const int tgtx, const int tgty ) const
{
    const double DBLRAD2DEG = 57.2957795130823f;
    //const double PI = 3.14159265358979f;
    const double DBLPI = 6.28318530717958f;
    double rad = atan2 ( static_cast<double>(tgty - y), static_cast<double>(tgtx - x) );
    if ( rad < 0 ) {
        rad = DBLPI - (0 - rad);
    }

    return int( rad * DBLRAD2DEG );
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

void map::load(const int wx, const int wy, const int wz, const bool update_vehicle)
{
    for( auto & traps : traplocs ) {
        traps.clear();
    }
    set_abs_sub( wx, wy, wz );
    for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
        for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
            loadn( gridx, gridy, update_vehicle );
        }
    }
}

void map::shift_traps( const tripoint &shift )
{
    // Offset needs to have sign opposite to shift direction
    const tripoint offset( -shift.x * SEEX, -shift.y * SEEY, -shift.z );
    for( auto & traps : traplocs ) {
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
        }
    }

// Shift the map sx submaps to the right and sy submaps down.
// sx and sy should never be bigger than +/-1.
// absx and absy are our position in the world, for saving/loading purposes.
    for( int gridz = zmin; gridz <= zmax; gridz++ ) {
        // Clear vehicle list and rebuild after shift
        clear_vehicle_cache( gridz );
        get_cache( gridz ).vehicle_list.clear();
        if (sx >= 0) {
            for (int gridx = 0; gridx < my_MAPSIZE; gridx++) {
                if (sy >= 0) {
                    for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
                        if (gridx + sx < my_MAPSIZE && gridy + sy < my_MAPSIZE) {
                            copy_grid( tripoint( gridx, gridy, gridz ),
                                       tripoint( gridx + sx, gridy + sy, gridz ) );
                            update_vehicle_list(get_submap_at_grid(gridx, gridy, gridz), gridz);
                        } else {
                            loadn( gridx, gridy, gridz, true );
                        }
                    }
                } else { // sy < 0; work through it backwards
                    for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
                        if (gridx + sx < my_MAPSIZE && gridy + sy >= 0) {
                            copy_grid( tripoint( gridx, gridy, gridz ),
                                       tripoint( gridx + sx, gridy + sy, gridz ) );
                            update_vehicle_list(get_submap_at_grid(gridx, gridy, gridz), gridz);
                        } else {
                            loadn( gridx, gridy, gridz, true );
                        }
                    }
                }
            }
        } else { // sx < 0; work through it backwards
            for (int gridx = my_MAPSIZE - 1; gridx >= 0; gridx--) {
                if (sy >= 0) {
                    for (int gridy = 0; gridy < my_MAPSIZE; gridy++) {
                        if (gridx + sx >= 0 && gridy + sy < my_MAPSIZE) {
                            copy_grid( tripoint( gridx, gridy, gridz ),
                                       tripoint( gridx + sx, gridy + sy, gridz ) );
                            update_vehicle_list(get_submap_at_grid(gridx, gridy, gridz), gridz);
                        } else {
                            loadn( gridx, gridy, gridz, true );
                        }
                    }
                } else { // sy < 0; work through it backwards
                    for (int gridy = my_MAPSIZE - 1; gridy >= 0; gridy--) {
                        if (gridx + sx >= 0 && gridy + sy >= 0) {
                            copy_grid( tripoint( gridx, gridy, gridz ),
                                       tripoint( gridx + sx, gridy + sy, gridz ) );
                            update_vehicle_list(get_submap_at_grid(gridx, gridy, gridz), gridz);
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
    dbg( D_INFO ) << "map::saven(worldx[" << abs_sub.x << "], worldy[" << abs_sub.y << "], worldz[" << abs_sub.z
                  << "], gridx[" << gridx << "], gridy[" << gridy << "], gridz[" << gridz << "])";
    const int gridn = get_nonant( gridx, gridy, gridz );
    submap *submap_to_save = getsubmap( gridn );
    if( submap_to_save == nullptr || submap_to_save->get_ter( 0, 0 ) == t_null ) {
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
    submap_to_save->turn_last_touched = int(calendar::turn);
    MAPBUFFER.add_submap( abs_x, abs_y, abs_z, submap_to_save );
}

// worldx & worldy specify where in the world this is;
// gridx & gridy specify which nonant:
// 0,0  1,0  2,0
// 0,1  1,1  2,1
// 0,2  1,2  2,2 etc
// (worldx,worldy,worldz) denotes the absolute coordinate of the submap
// in grid[0].
void map::loadn( const int gridx, const int gridy, const bool update_vehicles ) {
    if( zlevels ) {
        for( int gridz = -OVERMAP_DEPTH; gridz <= OVERMAP_HEIGHT; gridz++ ) {
            loadn( gridx, gridy, gridz, update_vehicles );
        }
    } else {
        loadn( gridx, gridy, abs_sub.z, update_vehicles );
    }
}

// Optimized mapgen function that only works properly for very simple overmap types
// Does not create or require a temporary map and does its own saving
static void generate_uniform( const int x, const int y, const int z, const oter_id &terrain_type )
{
    static const oter_id rock("empty_rock");
    static const oter_id air("open_air");

    dbg( D_INFO ) << "generate_uniform x: " << x << "  y: " << y << "  abs_z: " << z
                  << "  terrain_type: " << static_cast<std::string const&>(terrain_type);

    ter_id fill = t_null;
    if( terrain_type == rock ) {
        fill = t_rock;
    } else if( terrain_type == air ) {
        fill = t_open_air;
    } else {
        debugmsg( "map::generate_uniform called on non-uniform type: %s",
                  static_cast<std::string const&>(terrain_type).c_str() );
        return;
    }

    constexpr size_t block_size = SEEX * SEEY;
    for( int xd = 0; xd <= 1; xd++ ) {
        for( int yd = 0; yd <= 1; yd++ ) {
            submap *sm = new submap();
            sm->is_uniform = true;
            std::uninitialized_fill_n( &sm->ter[0][0], block_size, fill );
            sm->turn_last_touched = int(calendar::turn);
            MAPBUFFER.add_submap( x + xd, y + yd, z, sm );
        }
    }
}

void map::loadn( const int gridx, const int gridy, const int gridz, const bool update_vehicles )
{
    // Cache empty overmap types
    static const oter_id rock("empty_rock");
    static const oter_id air("open_air");

    dbg(D_INFO) << "map::loadn(game[" << g << "], worldx[" << abs_sub.x << "], worldy[" << abs_sub.y << "], gridx["
                << gridx << "], gridy[" << gridy << "], gridz[" << gridz << "])";

    const int absx = abs_sub.x + gridx,
              absy = abs_sub.y + gridy;
    const size_t gridn = get_nonant( gridx, gridy, gridz );

    dbg(D_INFO) << "map::loadn absx: " << absx << "  absy: " << absy
                << "  gridn: " << gridn;

    const int old_abs_z = abs_sub.z; // Ugly, but necessary at the moment
    abs_sub.z = gridz;

    submap *tmpsub = MAPBUFFER.lookup_submap(absx, absy, gridz);
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
        overmapbuffer::sm_to_omt( overx, overy );
        oter_id terrain_type = overmap_buffer.ter( overx, overy, gridz );
        if( terrain_type == rock || terrain_type == air ) {
            generate_uniform( newmapx, newmapy, gridz, terrain_type );
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
    setsubmap( gridn, tmpsub );

    for( auto it : tmpsub->vehicles ) {
        // Always fix submap coords for easier z-level-related operations
        it->smx = gridx;
        it->smy = gridy;
        it->smz = gridz;
    }

    // Update vehicle data
    if( update_vehicles ) {
        auto &map_cache = get_cache( gridz );
        for( auto it : tmpsub->vehicles ) {
            // Only add if not tracking already.
            if( map_cache.vehicle_list.find( it ) == map_cache.vehicle_list.end() ) {
                map_cache.vehicle_list.insert( it );
                add_vehicle_to_cache( it );
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
        return itm.get_rot() > DAYS( 10 ) && !itm.can_revive();
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
            it = i_rem( pnt, it );
        } else {
            ++it;
        }
    }
}

void map::fill_funnels( const tripoint &p )
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
    int maxvolume = 0;
    auto biggest_container = items.end();
    for( auto candidate = items.begin(); candidate != items.end(); ++candidate ) {
        if( candidate->is_funnel_container( maxvolume ) ) {
            biggest_container = candidate;
        }
    }
    if( biggest_container != items.end() ) {

        retroactively_fill_from_funnel( *biggest_container, tr, calendar::turn, getabs( p ) );
    }
}

void map::grow_plant( const tripoint &p )
{
    const auto &furn = furn_at( p );
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
    const int plantEpoch = seed.get_plant_epoch();

    if ( calendar::turn >= seed.bday + plantEpoch ) {
        if (calendar::turn < seed.bday + plantEpoch * 2 ) {
                i_rem(p, 1);
                furn_set(p, "f_plant_seedling");
        } else if (calendar::turn < seed.bday + plantEpoch * 3 ) {
                i_rem(p, 1);
                furn_set(p, "f_plant_mature");
        } else {
                furn_set(p, "f_plant_harvest");
        }
    }
}

void map::restock_fruits( const tripoint &p, int time_since_last_actualize )
{
    const auto &ter = ter_at( p );
    //if the fruit-bearing season of the already harvested terrain has passed, make it harvestable again
    if( !ter.has_flag( TFLAG_HARVESTED ) ) {
        return;
    }
    if( ter.harvest_season != calendar::turn.get_season() ||
        time_since_last_actualize >= DAYS( calendar::season_length() ) ) {
        ter_set( p, ter.transforms_into );
    }
}

void map::actualize( const int gridx, const int gridy, const int gridz )
{
    submap *const tmpsub = get_submap_at_grid( gridx, gridy, gridz );
    if( tmpsub == nullptr ) {
        debugmsg( "Actualize called on null submap (%d,%d,%d)", gridx, gridy, gridz );
        return;
    }

    const auto time_since_last_actualize = calendar::turn - tmpsub->turn_last_touched;
    const bool do_funnels = ( gridz >= 0 );

    // check spoiled stuff, and fill up funnels while we're at it
    for( int x = 0; x < SEEX; x++ ) {
        for( int y = 0; y < SEEY; y++ ) {
            const tripoint pnt( gridx * SEEX + x, gridy * SEEY + y, gridz );

            const auto &furn = furn_at( pnt );
            // plants contain a seed item which must not be removed under any circumstances
            if( !furn.has_flag( "PLANT" ) ) {
                remove_rotten_items( tmpsub->itm[x][y], pnt );
            }

            const auto trap_here = tmpsub->get_trap( x, y );
            if( trap_here != tr_null ) {
                traplocs[trap_here].push_back( pnt );
            }
            const ter_t &ter = tmpsub->get_ter( x, y ).obj();
            if( ter.trap != tr_null && ter.trap != tr_ledge ) {
                traplocs[trap_here].push_back( pnt );
            }

            if( do_funnels ) {
                fill_funnels( pnt );
            }

            grow_plant( pnt );

            restock_fruits( pnt, time_since_last_actualize );
        }
    }

    //Merchants will restock their inventories every three days
    const int merchantRestock = 14400 * 3; //14400 is the length of one day
    //Check for Merchants to restock
    for( auto & i : g->active_npc ) {
        if( i->restock != -1 && calendar::turn > ( i->restock + merchantRestock ) ) {
            i->shop_restock();
            i->restock = int( calendar::turn );
        }
    }

    // the last time we touched the submap, is right now.
    tmpsub->turn_last_touched = calendar::turn;
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
    const int s_range = std::min(SEEX * (MAPSIZE / 2), g->u.sight_range( g->light_level( g->u.posz() ) ) );
    int pop = group.population;
    std::vector<tripoint> locations;
    if( !ignore_sight ) {
        // If the submap is one of the outermost submaps, assume that monsters are
        // invisible there.
        // When the map shifts because of the player moving (called from game::plmove),
        // the player has still their *old* (not shifted) coordinates.
        // That makes the submaps that have come into view visible (if the sight range
        // is big enough).
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
        if( move_cost( upper_left ) == 0 ||
            ( !ignore_inside_checks && has_flag_ter_or_furn( TFLAG_INDOORS, upper_left ) ) ) {
            const tripoint glp = getabs( gp );
            dbg( D_ERROR ) << "Empty locations for group " << group.type.str() <<
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

            if( !ignore_terrain_checks && move_cost( fp ) == 0 ) {
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
        // TODO: what now? there is now possible place to spawn monsters, most
        // likely because the player can see all the places.
        const tripoint glp = getabs( gp );
        dbg( D_ERROR ) << "Empty locations for group " << group.type.str() <<
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
            for( int i = 0; i < spawn_details.pack_size; i++) {
                group.monsters.push_back( tmp );
            }
        }
    }

    for( auto &tmp : group.monsters ) {
        for( int tries = 0; tries < 10 && !locations.empty(); tries++ ) {
            const tripoint p = random_entry_removed( locations );
            if( !tmp.can_move_to( p ) ) {
                continue; // target can not contain the monster
            }
            tmp.spawn( p );
            g->add_zombie( tmp );
            break;
        }
    }
    // indicates the group is empty, and can be removed later
    group.clear();
}

void map::spawn_monsters_submap( const tripoint &gp, bool ignore_sight )
{
    auto groups = overmap_buffer.groups_at( abs_sub.x + gp.x, abs_sub.y + gp.y, gp.z );
    for( auto &mgp : groups ) {
        spawn_monsters_submap_group( gp, *mgp, ignore_sight );
    }

    submap * const current_submap = get_submap_at_grid( gp );
    for (auto &i : current_submap->spawns) {
        for (int j = 0; j < i.count; j++) {
            int tries = 0;
            int mx = i.posx;
            int my = i.posy;
            monster tmp( i.type );
            tmp.mission_id = i.mission_id;
            if (i.name != "NONE") {
                tmp.unique_name = i.name;
            }
            if (i.friendly) {
                tmp.friendly = -1;
            }
            int fx = mx + gp.x * SEEX;
            int fy = my + gp.y * SEEY;
            tripoint pos( fx, fy, gp.z );

            while( ( !g->is_empty( pos ) || !tmp.can_move_to( pos ) ) && tries < 10 ) {
                mx = (i.posx + rng(-3, 3)) % SEEX;
                my = (i.posy + rng(-3, 3)) % SEEY;
                if (mx < 0) {
                    mx += SEEX;
                }
                if (my < 0) {
                    my += SEEY;
                }
                fx = mx + gp.x * SEEX;
                fy = my + gp.y * SEEY;
                tries++;
                pos = tripoint( fx, fy, gp.z );
            }
            if (tries != 10) {
                tmp.spawn( pos );
                g->add_zombie(tmp);
            }
        }
    }
    current_submap->spawns.clear();
    overmap_buffer.spawn_monster( abs_sub.x + gp.x, abs_sub.y + gp.y, gp.z );
}

void map::spawn_monsters(bool ignore_sight)
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
    for( auto & smap : grid ) {
        smap->spawns.clear();
    }
}

void map::clear_traps()
{
    for( auto & smap : grid ) {
        for (int x = 0; x < SEEX; x++) {
            for (int y = 0; y < SEEY; y++) {
                smap->set_trap(x, y, tr_null);
            }
        }
    }

    // Forget about all trap locations.
    for( auto &i : traplocs ) {
        i.clear();
    }
}

const std::vector<tripoint> &map::trap_locations(trap_id t) const
{
    return traplocs[t];
}

bool map::inbounds(const int x, const int y) const
{
    return (x >= 0 && x < SEEX * my_MAPSIZE && y >= 0 && y < SEEY * my_MAPSIZE);
}

bool map::inbounds(const int x, const int y, const int z) const
{
    return (x >= 0 && x < SEEX * my_MAPSIZE &&
            y >= 0 && y < SEEY * my_MAPSIZE &&
            z >= -OVERMAP_DEPTH && z <= OVERMAP_HEIGHT);
}

bool map::inbounds( const tripoint &p ) const
{
 return (p.x >= 0 && p.x < SEEX * my_MAPSIZE &&
         p.y >= 0 && p.y < SEEY * my_MAPSIZE &&
         p.z >= -OVERMAP_DEPTH && p.z <= OVERMAP_HEIGHT);
}

void map::set_graffiti( const tripoint &p, const std::string &contents )
{
    if( !inbounds( p ) ) {
        return;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    current_submap->set_graffiti( lx, ly, contents );
}

void map::delete_graffiti( const tripoint &p )
{
    if( !inbounds( p ) ) {
        return;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    current_submap->delete_graffiti( lx, ly );
}

const std::string &map::graffiti_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        static const std::string empty_string;
        return empty_string;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    return current_submap->get_graffiti( lx, ly );
}

bool map::has_graffiti_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    int lx, ly;
    submap *const current_submap = get_submap_at( p, lx, ly );
    return current_submap->has_graffiti( lx, ly );
}

long map::determine_wall_corner( const tripoint &p ) const
{
    // This could be cached nicely
    const bool above_connects = has_flag_ter( TFLAG_CONNECT_TO_WALL, tripoint( p.x, p.y - 1, p.z ) );
    const bool below_connects = has_flag_ter( TFLAG_CONNECT_TO_WALL, tripoint( p.x, p.y + 1, p.z ) );
    const bool left_connects  = has_flag_ter( TFLAG_CONNECT_TO_WALL, tripoint( p.x - 1, p.y, p.z ) );
    const bool right_connects = has_flag_ter( TFLAG_CONNECT_TO_WALL, tripoint( p.x + 1, p.y, p.z ) );
    const auto bits = ( above_connects ? 1 : 0 ) +
                      ( right_connects ? 2 : 0 ) +
                      ( below_connects ? 4 : 0 ) +
                      ( left_connects  ? 8 : 0 );
    switch( bits ) {
        case 1 | 2 | 4 | 8: return LINE_XXXX;
        case 0 | 2 | 4 | 8: return LINE_OXXX;

        case 1 | 0 | 4 | 8: return LINE_XOXX;
        case 0 | 0 | 4 | 8: return LINE_OOXX;

        case 1 | 2 | 0 | 8: return LINE_XXOX;
        case 0 | 2 | 0 | 8: return LINE_OXOX;
        case 1 | 0 | 0 | 8: return LINE_XOOX;
        case 0 | 0 | 0 | 8: return LINE_OXOX; // LINE_OOOX would be better

        case 1 | 2 | 4 | 0: return LINE_XXXO;
        case 0 | 2 | 4 | 0: return LINE_OXXO;
        case 1 | 0 | 4 | 0: return LINE_XOXO;
        case 0 | 0 | 4 | 0: return LINE_XOXO; // LINE_OOXO would be better
        case 1 | 2 | 0 | 0: return LINE_XXOO;
        case 0 | 2 | 0 | 0: return LINE_OXOX; // LINE_OXOO would be better
        case 1 | 0 | 0 | 0: return LINE_XOXO; // LINE_XOOO would be better

        case 0 | 0 | 0 | 0: return ter_at( p ).symbol(); // technically just a column

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
    const size_t padded_w = ( MAPSIZE * SEEX ) + 2;
    const size_t padded_h = ( MAPSIZE * SEEY ) + 2;
    bool padded_cache[padded_w][padded_h];

    auto &outside_cache = ch.outside_cache;
    if( zlev < 0 )
    {
        std::uninitialized_fill_n(
            &outside_cache[0][0], ( MAPSIZE * SEEX ) * ( MAPSIZE * SEEY ), false );
        return;
    }

    std::uninitialized_fill_n(
            &padded_cache[0][0], padded_w * padded_h, true );

    for( int smx = 0; smx < my_MAPSIZE; ++smx ) {
        for( int smy = 0; smy < my_MAPSIZE; ++smy ) {
            auto const cur_submap = get_submap_at_grid( smx, smy, zlev );

            for( int sx = 0; sx < SEEX; ++sx ) {
                for( int sy = 0; sy < SEEY; ++sy ) {
                    if( cur_submap->get_ter( sx, sy ).obj().has_flag( TFLAG_INDOORS ) ||
                        cur_submap->get_furn( sx, sy ).obj().has_flag( TFLAG_INDOORS ) ) {
                        const int x = sx + ( smx * SEEX );
                        const int y = sy + ( smy * SEEY );
                        // Add 1 to both coords, because we're operating on the padded cache
                        for( int dx = 0; dx <= 2; dx++ )
                        {
                            for( int dy = 0; dy <= 2; dy++ )
                            {
                                padded_cache[x + dx][y + dy] = false;
                            }
                        }
                    }
                }
            }
        }
    }

    // Copy the padded cache back to the proper one, but with no padding
    for( int x = 0; x < my_MAPSIZE * SEEX; x++ ) {
        std::copy_n( &padded_cache[x + 1][1], my_MAPSIZE * SEEX, &outside_cache[x][0] );
    }

    ch.outside_cache_dirty = false;
}

void map::build_map_cache( const int zlev, bool skip_lightmap )
{
    build_outside_cache( zlev );
    build_transparency_cache( zlev );

    tripoint start( 0, 0, zlev );
    tripoint end( my_MAPSIZE * SEEX, my_MAPSIZE * SEEY, zlev );

    VehicleList vehs = get_vehicles( start, end );
    auto &outside_cache = get_cache( zlev ).outside_cache;
    auto &transparency_cache = get_cache( zlev ).transparency_cache;
    // Cache all the vehicle stuff in one loop
    for( auto &v : vehs ) {
        for( size_t part = 0; part < v.v->parts.size(); part++ ) {
            int px = v.x + v.v->parts[part].precalc[0].x;
            int py = v.y + v.v->parts[part].precalc[0].y;
            if(INBOUNDS(px, py)) {
                if (v.v->is_inside(part)) {
                    outside_cache[px][py] = false;
                }
                if (v.v->part_flag(part, VPFLAG_OPAQUE) && v.v->parts[part].hp > 0) {
                    int dpart = v.v->part_with_feature( part, VPFLAG_OPENABLE );
                    if (dpart < 0 || !v.v->parts[dpart].open) {
                        transparency_cache[px][py] = LIGHT_TRANSPARENCY_SOLID;
                    }
                }
            }
        }
    }

    build_seen_cache( g->u.pos(), zlev );
    if( !skip_lightmap ) {
        generate_lightmap( zlev );
    }
}

std::vector<point> closest_points_first(int radius, point p)
{
    return closest_points_first(radius, p.x, p.y);
}

//this returns points in a spiral pattern starting at center_x/center_y until it hits the radius. clockwise fashion
//credit to Tom J Nowell; http://stackoverflow.com/a/1555236/1269969
std::vector<point> closest_points_first(int radius, int center_x, int center_y)
{
    std::vector<point> points;
    int X,Y,x,y,dx,dy;
    X = Y = (radius * 2) + 1;
    x = y = dx = 0;
    dy = -1;
    int t = std::max(X,Y);
    int maxI = t * t;
    for(int i = 0; i < maxI; i++)
    {
        if ((-X/2 <= x) && (x <= X/2) && (-Y/2 <= y) && (y <= Y/2))
        {
            points.push_back(point(x + center_x, y + center_y));
        }
        if( (x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y)))
        {
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
    int X,Y,x,y,dx,dy;
    X = Y = (radius * 2) + 1;
    x = y = dx = 0;
    dy = -1;
    int t = std::max(X,Y);
    int maxI = t * t;
    for(int i = 0; i < maxI; i++)
    {
        if ((-X/2 <= x) && (x <= X/2) && (-Y/2 <= y) && (y <= Y/2))
        {
            points.push_back( tripoint( x + center.x, y + center.y, center.z ) );
        }
        if( (x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1 - y)))
        {
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

point map::getabs(const int x, const int y) const
{
    return point( x + abs_sub.x * SEEX, y + abs_sub.y * SEEY );
}

tripoint map::getabs( const tripoint &p ) const
{
    return tripoint( p.x + abs_sub.x * SEEX, p.y + abs_sub.y * SEEY, p.z );
}

point map::getlocal(const int x, const int y) const {
    return point( x - abs_sub.x * SEEX, y - abs_sub.y * SEEY );
}

tripoint map::getlocal( const tripoint &p ) const
{
    return tripoint( p.x - abs_sub.x * SEEX, p.y - abs_sub.y * SEEY, p.z );
}

void map::set_abs_sub(const int x, const int y, const int z)
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

void map::setsubmap( const size_t grididx, submap * const smap )
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

submap *map::get_submap_at( const int x, const int y, const int z ) const
{
    if( !inbounds( x, y, z ) ) {
        debugmsg( "Tried to access invalid map position (%d, %d, %d)", x, y, z );
        return nullptr;
    }
    return get_submap_at_grid( x / SEEX, y / SEEY, z );
}

submap *map::get_submap_at( const tripoint &p ) const
{
    if( !inbounds( p ) ) {
        debugmsg( "Tried to access invalid map position (%d, %d, %d)", p.x, p.y, p.z );
        return nullptr;
    }
    return get_submap_at_grid( p.x / SEEX, p.y / SEEY, p.z );
}

submap *map::get_submap_at( const int x, const int y ) const
{
    return get_submap_at( x, y, abs_sub.z );
}

submap *map::get_submap_at( const int x, const int y, int &offset_x, int &offset_y ) const
{
    return get_submap_at( x, y, abs_sub.z, offset_x, offset_y );
}

submap *map::get_submap_at( const int x, const int y, const int z, int &offset_x, int &offset_y ) const
{
    offset_x = x % SEEX;
    offset_y = y % SEEY;
    return get_submap_at( x, y, z );
}

submap *map::get_submap_at( const tripoint &p, int &offset_x, int &offset_y ) const
{
    offset_x = p.x % SEEX;
    offset_y = p.y % SEEY;
    return get_submap_at( p );
}

submap *map::get_submap_at_grid( const int gridx, const int gridy ) const
{
    return getsubmap( get_nonant( gridx, gridy ) );
}

submap *map::get_submap_at_grid( const int gridx, const int gridy, const int gridz ) const
{
    return getsubmap( get_nonant( gridx, gridy, gridz ) );
}

submap *map::get_submap_at_grid( const tripoint &p ) const
{
    return getsubmap( get_nonant( p.x, p.y, p.z ) );
}

size_t map::get_nonant( const int gridx, const int gridy ) const
{
    return get_nonant( gridx, gridy, abs_sub.z );
}

size_t map::get_nonant( const int gridx, const int gridy, const int gridz ) const
{
    if( gridx < 0 || gridx >= my_MAPSIZE ||
        gridy < 0 || gridy >= my_MAPSIZE ||
        gridz < -OVERMAP_DEPTH || gridz > OVERMAP_HEIGHT ) {
        debugmsg( "Tried to access invalid map position at grid (%d,%d,%d)", gridx, gridy, gridz );
        return 0;
    }

    if( zlevels ) {
        const int indexz = gridz + OVERMAP_HEIGHT; // Can't be lower than 0
        return indexz + ( gridx + gridy * my_MAPSIZE ) * OVERMAP_LAYERS;
    } else {
        return gridx + gridy * my_MAPSIZE;
    }
}

size_t map::get_nonant( const tripoint &gridp ) const
{
    return get_nonant( gridp.x, gridp.y, gridp.z );
}

tinymap::tinymap(int mapsize, bool zlevels)
: map(mapsize, zlevels)
{
}

ter_id find_ter_id(const std::string id, bool complain=true) {
    (void)complain; //FIXME: complain unused
    if( termap.find(id) == termap.end() ) {
         debugmsg("Can't find termap[%s]",id.c_str());
         return ter_id( 0 );
    }
    return termap[id].loadid;
}

furn_id find_furn_id(const std::string id, bool complain=true) {
    (void)complain; //FIXME: complain unused
    if( furnmap.find(id) == furnmap.end() ) {
         debugmsg("Can't find furnmap[%s]",id.c_str());
         return furn_id( 0 );
    }
    return furnmap[id].loadid;
}
void map::draw_line_ter(const ter_id type, int x1, int y1, int x2, int y2)
{
    std::vector<point> line = line_to(x1, y1, x2, y2, 0);
    for (auto &i : line) {
        ter_set(i.x, i.y, type);
    }
    ter_set(x1, y1, type);
}
void map::draw_line_ter(const std::string type, int x1, int y1, int x2, int y2) {
    draw_line_ter(find_ter_id(type), x1, y1, x2, y2);
}


void map::draw_line_furn(furn_id type, int x1, int y1, int x2, int y2) {
    std::vector<point> line = line_to(x1, y1, x2, y2, 0);
    for (auto &i : line) {
        furn_set(i.x, i.y, type);
    }
    furn_set(x1, y1, type);
}
void map::draw_line_furn(const std::string type, int x1, int y1, int x2, int y2) {
    draw_line_furn(find_furn_id(type), x1, y1, x2, y2);
}

void map::draw_fill_background(ter_id type) {
    // Need to explicitly set caches dirty - set_ter would do it before
    set_transparency_cache_dirty( abs_sub.z );
    set_outside_cache_dirty( abs_sub.z );

    // Fill each submap rather than each tile
    constexpr size_t block_size = SEEX * SEEY;
    for( int gridx = 0; gridx < my_MAPSIZE; gridx++ ) {
        for( int gridy = 0; gridy < my_MAPSIZE; gridy++ ) {
            auto sm = get_submap_at_grid( gridx, gridy );
            sm->is_uniform = true;
            std::uninitialized_fill_n( &sm->ter[0][0], block_size, type );
        }
    }
}

void map::draw_fill_background(std::string type) {
    draw_fill_background( find_ter_id(type) );
}
void map::draw_fill_background(ter_id (*f)()) {
    draw_square_ter(f, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}
void map::draw_fill_background(const id_or_id<ter_t> & f) {
    draw_square_ter(f, 0, 0, SEEX * my_MAPSIZE - 1, SEEY * my_MAPSIZE - 1);
}

void map::draw_square_ter(ter_id type, int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            ter_set(x, y, type);
        }
    }
}
void map::draw_square_ter(std::string type, int x1, int y1, int x2, int y2) {
    draw_square_ter(find_ter_id(type), x1, y1, x2, y2);
}

void map::draw_square_furn(furn_id type, int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            furn_set(x, y, type);
        }
    }
}
void map::draw_square_furn(std::string type, int x1, int y1, int x2, int y2) {
    draw_square_furn(find_furn_id(type), x1, y1, x2, y2);
}

void map::draw_square_ter(ter_id (*f)(), int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            ter_set(x, y, f());
        }
    }
}

void map::draw_square_ter(const id_or_id<ter_t> & f, int x1, int y1, int x2, int y2) {
    for (int x = x1; x <= x2; x++) {
        for (int y = y1; y <= y2; y++) {
            ter_set(x, y, f.get() );
        }
    }
}

void map::draw_rough_circle(ter_id type, int x, int y, int rad) {
    for (int i = x - rad; i <= x + rad; i++) {
        for (int j = y - rad; j <= y + rad; j++) {
            if (rl_dist(x, y, i, j) + rng(0, 3) <= rad) {
                ter_set(i, j, type);
            }
        }
    }
}
void map::draw_rough_circle(std::string type, int x, int y, int rad) {
    draw_rough_circle(find_ter_id(type), x, y, rad);
}

void map::draw_rough_circle_furn(furn_id type, int x, int y, int rad) {
    for (int i = x - rad; i <= x + rad; i++) {
        for (int j = y - rad; j <= y + rad; j++) {
            if (rl_dist(x, y, i, j) + rng(0, 3) <= rad) {
                furn_set(i, j, type);
            }
        }
    }
}
void map::draw_rough_circle_furn(std::string type, int x, int y, int rad) {
    draw_rough_circle_furn(find_furn_id(type), x, y, rad);
}

void map::add_corpse( const tripoint &p ) {
    item body;

    const bool isReviveSpecial = one_in(10);

    if (!isReviveSpecial){
        body.make_corpse();
    } else {
        body.make_corpse( mon_zombie, calendar::turn );
        body.item_tags.insert("REVIVE_SPECIAL");
        body.active = true;
    }

    add_item_or_charges(p, body);
    put_items_from_loc( "shoes",  p, 0);
    put_items_from_loc( "pants",  p, 0);
    put_items_from_loc( "shirts", p, 0);
    if (one_in(6)) {
        put_items_from_loc("jackets", p, 0);
    }
    if (one_in(15)) {
        put_items_from_loc("bags", p, 0);
    }
}

field &map::get_field( const tripoint &p )
{
    return field_at( p );
}

void map::creature_on_trap( Creature &c, bool const may_avoid )
{
    auto const &tr = tr_at( c.pos3() );
    if( tr.is_null() ) {
        return;
    }
    // boarded in a vehicle means the player is above the trap, like a flying monster and can
    // never trigger the trap.
    const player * const p = dynamic_cast<const player *>( &c );
    if( p != nullptr && p->in_vehicle ) {
        return;
    }
    if( may_avoid && c.avoid_trap( c.pos3(), tr ) ) {
        return;
    }
    tr.trigger( c.pos3(), &c );
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
    const int minx = std::max( std::min(stx, enx ), 0 );
    const int miny = std::max( std::min(sty, eny ), 0 );
    const int minz = std::max( std::min(stz, enz ), -OVERMAP_DEPTH );
    const int maxx = std::min( std::max(stx, enx ), my_MAPSIZE * SEEX );
    const int maxy = std::min( std::max(sty, eny ), my_MAPSIZE * SEEY );
    const int maxz = std::min( std::max(stz, enz ), OVERMAP_HEIGHT );

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
                submap const *cur_submap = get_submap_at_grid( smx, smy, z );
                // Bounds on the submap coords
                const int sm_minx = smx > min_smx ? 0 : minx % SEEX;
                const int sm_miny = smy > min_smy ? 0 : miny % SEEY;
                const int sm_maxx = smx < max_smx ? (SEEX - 1) : maxx % SEEX;
                const int sm_maxy = smy < max_smy ? (SEEY - 1) : maxy % SEEY;

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

void map::scent_blockers( bool (&blocks_scent)[SEEX * MAPSIZE][SEEY * MAPSIZE],
                          bool (&reduces_scent)[SEEX * MAPSIZE][SEEY * MAPSIZE],
                          const int minx, const int miny, const int maxx, const int maxy )
{
    auto reduce = TFLAG_REDUCE_SCENT;
    auto block = TFLAG_WALL;
    auto fill_values = [&]( const tripoint &gp, const submap *sm, const point &lp ) {
        if( sm->get_ter( lp.x, lp.y ).obj().has_flag( block ) ) {
            // We need to generate the x/y coords, because we can't get them "for free"
            const int x = ( gp.x * SEEX ) + lp.x;
            const int y = ( gp.y * SEEY ) + lp.y;
            blocks_scent[x][y] = true;
        } else if( sm->get_ter( lp.x, lp.y ).obj().has_flag( reduce ) || sm->get_furn( lp.x, lp.y ).obj().has_flag( reduce ) ) {
            const int x = ( gp.x * SEEX ) + lp.x;
            const int y = ( gp.y * SEEY ) + lp.y;
            reduces_scent[x][y] = true;
        }

        return ITER_CONTINUE;
    };

    function_over( minx, miny, abs_sub.z, maxx, maxy, abs_sub.z, fill_values );

    // Now vehicles

    // Currently the scentmap is limited to an area around the player rather than entire map
    auto local_bounds = [=]( const point &coord ) {
        return coord.x >= minx && coord.x <= maxx && coord.y >= miny && coord.y <= maxy;
    };

    auto vehs = get_vehicles();
    for( auto &wrapped_veh : vehs ) {
        vehicle &veh = *(wrapped_veh.v);
        auto obstacles = veh.all_parts_with_feature( VPFLAG_OBSTACLE, true );
        for( const int p : obstacles ) {
            const point part_pos = veh.global_pos() + veh.parts[p].precalc[0];
            if( local_bounds( part_pos ) ) {
                reduces_scent[part_pos.x][part_pos.y] = true;
            }
        }

        // Doors, but only the closed ones
        auto doors = veh.all_parts_with_feature( VPFLAG_OPENABLE, true );
        for( const int p : doors ) {
            if( veh.parts[p].open ) {
                continue;
            }

            const point part_pos = veh.global_pos() + veh.parts[p].precalc[0];
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
    transparency_cache_dirty = true;
    outside_cache_dirty = true;
    veh_in_active_range = false;
    std::fill_n( &veh_exists_at[0][0], SEEX * MAPSIZE * SEEY * MAPSIZE, false );
}

void map::clip_to_bounds( tripoint &p ) const
{
    clip_to_bounds( p.x, p.y, p.z );
}

void map::clip_to_bounds( int &x, int &y ) const
{
    if( x < 0 ) {
        x = 0;
    } else if( x >= my_MAPSIZE * SEEX ) {
        x = my_MAPSIZE * SEEX - 1;
    }

    if( y < 0 ) {
        y = 0;
    } else if( y >= my_MAPSIZE * SEEY ) {
        y = my_MAPSIZE * SEEY - 1;
    }
}

void map::clip_to_bounds( int &x, int &y, int &z ) const
{
    clip_to_bounds( x, y );
    if( z < -OVERMAP_DEPTH ) {
        z = OVERMAP_DEPTH;
    } else if( z > OVERMAP_HEIGHT ) {
        z = OVERMAP_HEIGHT;
    }
}
