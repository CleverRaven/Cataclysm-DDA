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
#include "npc_arbitration.h"
#include "pathfinding.h"
#include "point.h"
#include "vehicle.h"
#include "vpart_position.h"

bool npc_reach::contains( const tripoint_abs_ms &p ) const
{
    if( aboard != nullptr ) {
        const optional_vpart_position vp = get_map().veh_at( p );
        return vp && &vp->vehicle() == aboard;
    }
    return p.z() == center.z() && rl_dist( p, center ) <= radius;
}

npc_reach_snapshot npc_reach_snapshot::of( const std::optional<npc_reach> &reach )
{
    npc_reach_snapshot snap;
    if( reach ) {
        snap.anchored = true;
        snap.center = reach->center;
        snap.radius = reach->radius;
        if( reach->aboard != nullptr ) {
            snap.has_vehicle = true;
            snap.aboard = reach->aboard->get_safe_reference();
        }
    }
    return snap;
}

bool npc_reach_snapshot::matches( const std::optional<npc_reach> &reach ) const
{
    if( !reach ) {
        return !anchored;
    }
    if( !anchored || reach->center != center || reach->radius != radius ||
        ( reach->aboard != nullptr ) != has_vehicle ) {
        return false;
    }
    return !has_vehicle || aboard.get() == reach->aboard;
}

bool npc_reach_policy::allows( const tripoint_abs_ms &p ) const
{
    if( reach.contains( p ) ) {
        return true;
    }
    return approach_bound >= 0 && p.z() == reach.center.z() &&
           rl_dist( p, reach.center ) <= approach_bound;
}

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
bool npc::trip_endpoint_ok( const tripoint_bub_ms &p, const npc_reach_policy *policy ) const
{
    map &here = get_map();
    if( policy != nullptr && !policy->allows( here.get_abs( p ) ) ) {
        return false;
    }
    if( p == pos_bub() ) {
        return true;
    }
    return here.passable_through( p ) && !get_path_avoid()( p );
}

// route()'s step rule + get_path_avoid(); ignore_occupant lets a creature stand on p
bool npc::trip_step_ok( const tripoint_bub_ms &from, const tripoint_bub_ms &p,
                        const bool no_bashing, const npc_reach_policy *policy,
                        const bool ignore_occupant ) const
{
    map &here = get_map();
    return !path_avoids( p, ignore_occupant ) &&
           here.can_pass( from, p, get_pathfinding_settings( no_bashing ) ) &&
           ( policy == nullptr || policy->allows( here.get_abs( p ) ) );
}

std::vector<tripoint_bub_ms> npc::trip_interaction_tiles( const npc_trip_spec &spec,
        const npc_reach_policy *policy ) const
{
    std::vector<tripoint_bub_ms> tiles;
    switch( spec.kind ) {
        case npc_trip_kind::stand_on:
            if( trip_endpoint_ok( spec.source, policy ) ) {
                tiles.push_back( spec.source );
            }
            break;
        case npc_trip_kind::legacy_nearest_passable:
            if( const std::optional<tripoint_bub_ms> d = nearest_passable( spec.source, pos_bub() );
                d && trip_endpoint_ok( *d, policy ) ) {
                tiles.push_back( *d );
            }
            break;
        case npc_trip_kind::within_range:
            // same z level only: acting across floors is not modeled
            for( const tripoint_bub_ms &p : get_map().points_in_radius( spec.source, spec.range ) ) {
                if( trip_endpoint_ok( p, policy ) ) {
                    tiles.push_back( p );
                }
            }
            break;
    }
    return tiles;
}

// bound fixed when a walk starts, kept while the reach is unchanged, so an
// approach bending outward stays acceptable
std::optional<npc_reach_policy> npc::reach_policy_for( const npc_trip_spec &spec ) const
{
    const std::optional<npc_reach> reach = obligation_reach();
    if( !reach ) {
        return std::nullopt;
    }
    npc_reach_policy policy{ *reach, -1 };
    const bool inside = reach->contains( pos_abs() );
    const npc_retained_trip &kept = ai_cache.retained_trip;
    const bool same_trip = kept.valid && kept.constrained && kept.spec == spec &&
                           kept.reach.matches( reach );
    if( same_trip && ( inside || kept.approach_bound >= 0 ) ) {
        policy.approach_bound = kept.approach_bound;
    } else if( !inside ) {
        policy.approach_bound = rl_dist( pos_abs(), reach->center );
    }
    return policy;
}

npc_trip npc::route_trip( const npc_trip_spec &spec, const npc_reach_policy *policy ) const
{
    const tripoint_bub_ms pos = pos_bub();
    npc_trip trip;
    trip.turn = calendar::turn;
    trip.from = pos;
    trip.spec = spec;
    trip.constrained = policy != nullptr;
    trip.approach_bound = policy != nullptr ? policy->approach_bound : -1;
    std::vector<tripoint_bub_ms> goals = trip_interaction_tiles( spec, policy );
    if( goals.empty() ) {
        return trip;
    }
    if( std::find( goals.begin(), goals.end(), pos ) != goals.end() ) {
        trip.dest = pos;
        trip.feasible = true;
        return trip;
    }
    // reuse kept path while it fits. walk costs one route() not one per turn
    const npc_retained_trip &kept = ai_cache.retained_trip;
    const bool reuse = kept.valid && kept.spec == spec && kept.constrained == trip.constrained &&
                       ( policy == nullptr || ( kept.approach_bound == policy->approach_bound &&
                               kept.reach.matches( policy->reach ) ) ) &&
                       !path.empty() &&
                       ( path.front().z() != pos.z() || rl_dist( path.front(), pos ) <= 1 ) &&
                       std::find( goals.begin(), goals.end(), path.back() ) != goals.end() &&
    std::all_of( path.begin(), path.end(), [ &, from = pos]( const tripoint_bub_ms & p ) mutable {
        // a further creature may move on before the NPC reaches it
        const bool ok = trip_step_ok( from, p, spec.no_bashing, policy, p != path.front() );
        from = p;
        return ok;
    } );
    if( reuse ) {
        trip.dest = path.back();
        trip.path = path;
        trip.feasible = true;
        return trip;
    }
    // first goal with a route wins, not always the shortest walk
    std::sort( goals.begin(), goals.end(), [&pos]( const tripoint_bub_ms & a,
    const tripoint_bub_ms & b ) {
        const int da = rl_dist( pos, a );
        const int db = rl_dist( pos, b );
        return da != db ? da < db : trig_dist( pos, a ) < trig_dist( pos, b );
    } );
    map &here = get_map();
    const std::function<bool( const tripoint_bub_ms & )> avoid = get_path_avoid();
    for( const tripoint_bub_ms &goal : goals ) {
        std::vector<tripoint_bub_ms> walk = here.route( pos, pathfinding_target::point( goal ),
                                            get_pathfinding_settings( spec.no_bashing ),
        [&]( const tripoint_bub_ms & p ) {
            return avoid( p ) || ( policy != nullptr && !policy->allows( here.get_abs( p ) ) );
        } );
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
    return trip;
}

std::optional<npc_trip> npc::resolve_trip( const npc_trip_spec &spec ) const
{
    npc_trip &cached = ai_cache.last_trip;
    if( cached.turn == calendar::turn && cached.from == pos_bub() && cached.spec == spec ) {
        return cached.feasible ? std::optional<npc_trip>( cached ) : std::nullopt;
    }
    const std::optional<npc_reach_policy> policy = reach_policy_for( spec );
    npc_trip trip = route_trip( spec, policy ? &*policy : nullptr );
    if( policy && !trip.feasible ) {
        // none inside the reach: route freely and let the verdict decide
        // TODO: a refused option pays both searches every turn it's judged; skip it when
        // the need can't reach the floor
        trip = route_trip( spec, nullptr );
        trip.route_in_reach = false;
    }
    cached = trip;
    return trip.feasible ? std::optional<npc_trip>( trip ) : std::nullopt;
}

std::optional<npc_option_cost> npc::trip_cost( const npc_trip_spec &spec,
        const bool responsive ) const
{
    map &here = get_map();
    if( !obligation_reach() ) {
        // without reach the route can't change a verdict
        if( spec.kind == npc_trip_kind::stand_on && !here.passable_through( spec.source ) ) {
            return std::nullopt;
        }
        const bool at_hand = spec.kind == npc_trip_kind::within_range ?
                             spec.source.z() == posz() && square_dist( pos_bub(), spec.source ) <= spec.range :
                             pos_bub() == spec.source;
        return npc_option_cost::trip( here.get_abs( spec.source ), !at_hand, responsive, true );
    }
    const std::optional<npc_trip> trip = resolve_trip( spec );
    if( !trip ) {
        return std::nullopt;
    }
    return npc_option_cost::trip( here.get_abs( trip->dest ), !trip->path.empty(), responsive,
                                  trip->route_in_reach );
}

bool npc::move_to_and_verify( const npc_trip_spec &spec )
{
    const auto step_valid = [&]( const npc_trip & t ) {
        const std::optional<npc_reach_policy> policy = t.constrained ? reach_policy_for( spec ) :
                std::nullopt;
        const npc_reach_policy *pol = policy ? &*policy : nullptr;
        return !t.path.empty() && trip_step_ok( pos_bub(), t.path.front(), spec.no_bashing, pol ) &&
               trip_endpoint_ok( t.dest, pol );
    };
    const auto drop_path = [&]() {
        path.clear();
        ai_cache.retained_trip = {};
        ai_cache.last_trip = {};
    };
    std::optional<npc_trip> trip = resolve_trip( spec );
    if( trip && !trip->path.empty() && !step_valid( *trip ) ) {
        // the way changed since the walk was resolved: drop the cached walk, resolve once more
        const npc_trip judged = *trip;
        drop_path();
        trip = resolve_trip( spec );
        // a new end tile or a walk out of the reach is not taken until the caller picks it again
        if( trip && ( trip->dest != judged.dest || trip->constrained != judged.constrained ||
                      trip->route_in_reach != judged.route_in_reach ) ) {
            drop_path();
            return false;
        }
    }
    if( !trip || trip->path.empty() || !step_valid( *trip ) ) {
        drop_path();
        return false;
    }
    path = trip->path;
    ai_cache.retained_trip = { true, spec, trip->constrained, trip->approach_bound,
                               npc_reach_snapshot::of( obligation_reach() )
                             };
    const tripoint_bub_ms before = pos_bub();
    move_to_next();
    return pos_bub() != before;
}
