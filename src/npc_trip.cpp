#include "npc_trip.h"

#include <algorithm>
#include <functional>
#include <optional>
#include <utility>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "npc.h"
#include "pathfinding.h"
#include "point.h"

std::optional<tripoint_bub_ms> nearest_passable( const tripoint_bub_ms &p,
        const tripoint_bub_ms &closest_to )
{
    map &here = get_map();
    if( here.passable_through( p ) ) {
        return p;
    }

    // We need to path to adjacent tile, not the exact one
    // Let's pick the closest one to us that is passable
    std::vector<tripoint_bub_ms> candidates = closest_points_first( p, 1 );
    std::sort( candidates.begin(), candidates.end(), [ closest_to ]( const tripoint_bub_ms & l,
    const tripoint_bub_ms & r ) {
        return rl_dist( closest_to, l ) < rl_dist( closest_to, r );
    } );
    auto iter = std::find_if( candidates.begin(), candidates.end(),
    [&here]( const tripoint_bub_ms & pt ) {
        return here.passable_through( pt );
    } );
    if( iter != candidates.end() ) {
        return *iter;
    }

    return std::nullopt;
}

// get_path_avoid() rejects the NPC's own tile as occupied
bool npc::trip_endpoint_ok( const tripoint_bub_ms &p ) const
{
    if( p == pos_bub() ) {
        return true;
    }
    return get_map().passable_through( p ) && !get_path_avoid()( p );
}

// route()'s step rule + get_path_avoid(); ignore_occupant lets a creature stand on p
bool npc::trip_step_ok( const tripoint_bub_ms &from, const tripoint_bub_ms &p,
                        const bool no_bashing, const bool ignore_occupant ) const
{
    map &here = get_map();
    return !path_avoids( p, ignore_occupant ) &&
           here.can_pass( from, p, get_pathfinding_settings( no_bashing ) );
}

std::vector<tripoint_bub_ms> npc::trip_interaction_tiles( const npc_trip_spec &spec ) const
{
    std::vector<tripoint_bub_ms> tiles;
    switch( spec.kind ) {
        case npc_trip_kind::stand_on:
            if( trip_endpoint_ok( spec.source ) ) {
                tiles.push_back( spec.source );
            }
            break;
        case npc_trip_kind::legacy_nearest_passable:
            if( const std::optional<tripoint_bub_ms> d = nearest_passable( spec.source, pos_bub() );
                d && trip_endpoint_ok( *d ) ) {
                tiles.push_back( *d );
            }
            break;
        case npc_trip_kind::within_range:
            // same z level only: acting across floors is not modeled
            for( const tripoint_bub_ms &p : get_map().points_in_radius( spec.source, spec.range ) ) {
                if( trip_endpoint_ok( p ) ) {
                    tiles.push_back( p );
                }
            }
            break;
    }
    return tiles;
}

std::optional<npc_trip> npc::resolve_trip( const npc_trip_spec &spec ) const
{
    const tripoint_bub_ms pos = pos_bub();
    npc_trip &cached = ai_cache.last_trip;
    if( cached.turn == calendar::turn && cached.from == pos && cached.spec == spec ) {
        return cached.feasible ? std::optional<npc_trip>( cached ) : std::nullopt;
    }
    npc_trip trip;
    trip.turn = calendar::turn;
    trip.from = pos;
    trip.spec = spec;
    std::vector<tripoint_bub_ms> goals = trip_interaction_tiles( spec );
    if( std::find( goals.begin(), goals.end(), pos ) != goals.end() ) {
        trip.dest = pos;
        trip.feasible = true;
    } else if( !goals.empty() ) {
        // reuse kept path while it fits. walk costs one route() not one per turn
        const npc_retained_trip &kept = ai_cache.retained_trip;
        const bool reuse = kept.valid && kept.spec == spec && !path.empty() &&
                           ( path.front().z() != pos.z() || rl_dist( path.front(), pos ) <= 1 ) &&
                           std::find( goals.begin(), goals.end(), path.back() ) != goals.end() &&
        std::all_of( path.begin(), path.end(), [ &, from = pos]( const tripoint_bub_ms & p ) mutable {
            // a further creature may move on before the NPC reaches it
            const bool ok = trip_step_ok( from, p, spec.no_bashing, p != path.front() );
            from = p;
            return ok;
        } );
        if( reuse ) {
            trip.dest = path.back();
            trip.path = path;
            trip.feasible = true;
        } else {
            // first goal with a route wins, not always the shortest walk
            std::sort( goals.begin(), goals.end(), [&pos]( const tripoint_bub_ms & a,
            const tripoint_bub_ms & b ) {
                const int da = rl_dist( pos, a );
                const int db = rl_dist( pos, b );
                return da != db ? da < db : trig_dist( pos, a ) < trig_dist( pos, b );
            } );
            map &here = get_map();
            for( const tripoint_bub_ms &goal : goals ) {
                std::vector<tripoint_bub_ms> walk = here.route( pos, pathfinding_target::point( goal ),
                                                    get_pathfinding_settings( spec.no_bashing ), get_path_avoid() );
                ai_cache.trip_route_calls++;
                while( !walk.empty() && walk.front() == pos ) {
                    walk.erase( walk.begin() );
                }
                if( walk.empty() || walk.back() != goal ) {
                    continue;
                }
                trip.dest = goal;
                trip.path = std::move( walk );
                trip.feasible = true;
                break;
            }
        }
    }
    cached = trip;
    return trip.feasible ? std::optional<npc_trip>( trip ) : std::nullopt;
}

bool npc::move_to_and_verify( const npc_trip_spec &spec )
{
    const auto step_valid = [&]( const npc_trip & t ) {
        return !t.path.empty() && trip_step_ok( pos_bub(), t.path.front(), spec.no_bashing ) &&
               trip_endpoint_ok( t.dest );
    };
    const auto drop_path = [&]() {
        path.clear();
        ai_cache.retained_trip = {};
        ai_cache.last_trip = {};
    };
    std::optional<npc_trip> trip = resolve_trip( spec );
    if( trip && !trip->path.empty() && !step_valid( *trip ) ) {
        // the way changed since the walk was resolved: drop the cached walk, resolve once more
        const tripoint_bub_ms judged_dest = trip->dest;
        drop_path();
        trip = resolve_trip( spec );
        // a new end tile is not walked toward until the caller picks it again
        if( trip && trip->dest != judged_dest ) {
            drop_path();
            return false;
        }
    }
    if( !trip || trip->path.empty() || !step_valid( *trip ) ) {
        drop_path();
        return false;
    }
    path = trip->path;
    ai_cache.retained_trip = { true, spec };
    const tripoint_bub_ms before = pos_bub();
    move_to_next();
    return pos_bub() != before;
}
