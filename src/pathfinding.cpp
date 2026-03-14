#include "pathfinding.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdlib>
#include <functional>
#include <memory>
#include <numeric>
#include <optional>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "cata_utility.h"
#include "character.h"
#include "coordinates.h"
#include "creature.h"
#include "damage.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
#include "messages.h"
#include "maptile_fwd.h"
#include "point.h"
#include "submap.h"
#include "trap.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

class field;

// Turns two indexed to a 2D array into an index to equivalent 1D array
static constexpr int flat_index( const point_bub_ms &p )
{
    return ( p.x() * MAPSIZE_Y ) + p.y();
}

// Flattened 2D array representing a single z-level worth of pathfinding data
struct path_data_layer {
    // Closed/open is accessed way more often than all other values here
    std::bitset< MAPSIZE_X *MAPSIZE_Y > closed;
    std::bitset< MAPSIZE_X *MAPSIZE_Y > open;
    std::array< int, MAPSIZE_X *MAPSIZE_Y > score;
    std::array< int, MAPSIZE_X *MAPSIZE_Y > gscore;
    std::array< tripoint_bub_ms, MAPSIZE_X *MAPSIZE_Y > parent;

    void reset() {
        closed.reset();
        open.reset();
    }
};

struct pathfinder {
    using queue_type =
        std::priority_queue< std::pair<int, tripoint_bub_ms>, std::vector< std::pair<int, tripoint_bub_ms> >, pair_greater_cmp_first >;
    queue_type open;
    std::array< std::unique_ptr< path_data_layer >, OVERMAP_LAYERS > path_data;

    path_data_layer &get_layer( const int z ) {
        std::unique_ptr< path_data_layer > &ptr = path_data[z + OVERMAP_DEPTH];
        if( ptr != nullptr ) {
            return *ptr;
        }
        ptr = std::make_unique<path_data_layer>();
        return *ptr;
    }

    void reset( int minz, int maxz ) {
        for( int i = minz; i <= maxz; ++i ) {
            std::unique_ptr< path_data_layer > &ptr = path_data[i + OVERMAP_DEPTH];
            if( ptr != nullptr ) {
                path_data[i + OVERMAP_DEPTH]->reset();
            }
        }
        open = queue_type();
    }

    bool empty() const {
        return open.empty();
    }

    tripoint_bub_ms get_next() {
        const auto pt = open.top();
        open.pop();
        return pt.second;
    }

    void add_point( const int gscore, const int score, const tripoint_bub_ms &from,
                    const tripoint_bub_ms &to ) {
        path_data_layer &layer = get_layer( to.z() );
        const int index = flat_index( to.xy() );
        if( layer.closed[index] ) {
            return;
        }
        if( layer.open[index] && gscore >= layer.gscore[index] ) {
            return;
        }

        layer.open[index] = true;
        layer.gscore[index] = gscore;
        layer.parent[index] = from;
        layer.score [index] = score;
        open.emplace( score, to );
    }

    void close_point( const tripoint_bub_ms &p ) {
        path_data_layer &layer = get_layer( p.z() );
        const int index = flat_index( p.xy() );
        layer.closed[index] = true;
    }

    void unclose_point( const tripoint_bub_ms &p ) {
        path_data_layer &layer = get_layer( p.z() );
        const int index = flat_index( p.xy() );
        layer.closed[index] = false;
    }
};

static pathfinder pf;

// Modifies `t` to point to a tile with `flag` in a 1-submap radius of `t`'s original value,
// searching nearest points first (starting with `t` itself).
// return false if it could not find a suitable point
static bool vertical_move_destination( const map &m, ter_furn_flag flag, tripoint_bub_ms &t )
{
    const pathfinding_cache &pf_cache = m.get_pathfinding_cache_ref( t.z() );
    for( const point_bub_ms &p : closest_points_first( t.xy(), SEEX ) ) {
        if( pf_cache.special[p.x()][p.y()] & ( PathfindingFlag::GoesDown | PathfindingFlag::GoesUp ) ) {
            const tripoint_bub_ms t2( p, t.z() );
            if( m.has_flag( flag, t2 ) ) {
                t = t2;
                return true;
            }
        }
    }
    return false;
}

template<class Set1, class Set2>
static bool is_disjoint( const Set1 &set1, const Set2 &set2 )
{
    if( set1.empty() || set2.empty() ) {
        return true;
    }

    typename Set1::const_iterator it1 = set1.begin();
    typename Set1::const_iterator it1_end = set1.end();

    typename Set2::const_iterator it2 = set2.begin();
    typename Set2::const_iterator it2_end = set2.end();

    if( *set2.rbegin() < *it1 || *set1.rbegin() < *it2 ) {
        return true;
    }

    while( it1 != it1_end && it2 != it2_end ) {
        if( *it1 == *it2 ) {
            return false;
        }
        if( *it1 < *it2 ) {
            it1++;
        } else {
            it2++;
        }
    }

    return true;
}

std::vector<tripoint_bub_ms> map::straight_route( const tripoint_bub_ms &f,
        const tripoint_bub_ms &t ) const
{
    std::vector<tripoint_bub_ms> ret;
    if( f == t || !inbounds( f ) ) {
        return ret;
    }
    if( !inbounds( t ) ) {
        tripoint_bub_ms clipped = t;
        clip_to_bounds( clipped );
        return straight_route( f, clipped );
    }
    if( f.z() == t.z() ) {
        ret = line_to( f, t );
        const pathfinding_cache &pf_cache = get_pathfinding_cache_ref( f.z() );
        // Check all points for any special case (including just hard terrain)
        if( std::any_of( ret.begin(), ret.end(), [&pf_cache]( const tripoint_bub_ms & p ) {
        constexpr PathfindingFlags non_normal = PathfindingFlag::Slow |
                                                PathfindingFlag::Obstacle | PathfindingFlag::Vehicle | PathfindingFlag::DangerousTrap |
                                                PathfindingFlag::Sharp;
        return pf_cache.special[p.x()][p.y()] & non_normal;
        } ) ) {
            ret.clear();
        }
    }
    return ret;
}

static constexpr int PF_IMPASSABLE = -1;
static constexpr int PF_IMPASSABLE_FROM_HERE = -2;
int map::cost_to_pass( const tripoint_bub_ms &cur, const tripoint_bub_ms &p,
                       const pathfinding_settings &settings,
                       PathfindingFlags p_special ) const
{
    constexpr PathfindingFlags non_normal = PathfindingFlag::Slow |
                                            PathfindingFlag::Obstacle | PathfindingFlag::Vehicle | PathfindingFlag::DangerousTrap |
                                            PathfindingFlag::Sharp;
    if( !( p_special & non_normal ) ) {
        // Boring flat dirt - the most common case above the ground
        return 2;
    }

    if( settings.avoid_rough_terrain ) {
        return PF_IMPASSABLE;
    }

    if( settings.avoid_sharp && ( p_special & PathfindingFlag::Sharp ) ) {
        return PF_IMPASSABLE;
    }

    // RestrictTiny isn't checked since it's unclear how it would actually work as there's no category smaller than tiny
    if( settings.size && (
            ( p_special & PathfindingFlag::RestrictSmall && settings.size > creature_size::tiny ) ||
            ( p_special & PathfindingFlag::RestrictMedium && settings.size > creature_size::small ) ||
            ( p_special & PathfindingFlag::RestrictLarge && settings.size > creature_size::medium ) ||
            ( p_special & PathfindingFlag::RestrictHuge && settings.size > creature_size::large ) ) ) {
        return PF_IMPASSABLE;
    }

    const std::map<damage_type_id, int> &bash = settings.bash_strength;
    const bool allow_open_doors = settings.allow_open_doors;
    const bool allow_unlock_doors = settings.allow_unlock_doors;
    const int climb_cost = settings.climb_cost;

    int part = -1;
    const const_maptile &tile = maptile_at_internal( p );
    const ter_t &terrain = tile.get_ter_t();
    const furn_t &furniture = tile.get_furn_t();
    const field &field = tile.get_field();
    const vehicle *veh = veh_at_internal( p, part );

    const int cost = move_cost_internal( furniture, terrain, field, veh, part );

    // If we can just walk into the tile, great. That's the cost.
    if( cost != 0 ) {
        return cost;
    }

    // Otherwise we'll consider climbing, opening doors, and bashing, in that order.
    // Should match logic in monster::move_to and npc::move_to.

    // If there's a vehicle here, we need to assess that instead of the terrain.
    if( veh != nullptr ) {
        const auto vpobst = vpart_position( const_cast<vehicle &>( *veh ), part ).obstacle_at_part();
        part = vpobst ? vpobst->part_index() : -1;
        int dummy = -1;
        const bool is_outside_veh = veh_at_internal( cur, dummy ) != veh;

        if( part >= 0 ) {
            if( allow_open_doors && veh->next_part_to_open( part, is_outside_veh ) != -1 ) {
                // Handle car doors, but don't try to path through curtains
                return 10; // One turn to open, 4 to move there
            } else if( allow_unlock_doors && veh->next_part_to_unlock( part, is_outside_veh ) != -1 ) {
                return 12; // 2 turns to open, 4 to move there
            } else if( !bash.empty() ) {
                int damage = std::accumulate( bash.begin(), bash.end(), 0, []( int so_far,
                const std::pair<damage_type_id, int> &pr ) {
                    return so_far + static_cast<int>( pr.second * pr.first->bash_conversion_factor );
                } );
                // Car obstacle that isn't a door
                // TODO: Account for armor
                int hp = veh->part( part ).hp();
                if( hp / 20 > damage ) {
                    // Threshold damage thing means we just can't bash this down
                    return PF_IMPASSABLE;
                } else if( hp / 10 > damage ) {
                    // Threshold damage thing means we will fail to deal damage pretty often
                    hp *= 2;
                }

                if( damage == 0 ) {
                    return PF_IMPASSABLE;
                }
                return 2 * hp / damage + 8 + 4;
            } else {
                const vehicle_part &vp = veh->part( part );
                if( allow_open_doors && vp.info().has_flag( VPFLAG_OPENABLE ) ) {
                    // If we can open doors in general but weren't
                    // able to open this one, we might be able to
                    // open it if we try from another direction.
                    return PF_IMPASSABLE_FROM_HERE;
                } else {
                    // Won't be openable, don't try from other sides
                    return PF_IMPASSABLE;
                }
            }
        }
    }

    // If we can climb it, great!
    if( climb_cost > 0 && p_special & PathfindingFlag::Climbable ) {
        return climb_cost;
    }

    // If terrain/furniture is openable but we can't fit through the open version, ignore the tile
    if( settings.size && allow_open_doors &&
        ( ( terrain.open && terrain.open->has_flag( ter_furn_flag::TFLAG_SMALL_PASSAGE ) ) ||
          ( furniture.open && furniture.open->has_flag( ter_furn_flag::TFLAG_SMALL_PASSAGE ) ) ||
          // Windows with curtains need to be opened twice
          ( terrain.open->open && terrain.open->open->has_flag( ter_furn_flag::TFLAG_SMALL_PASSAGE ) ) ) &&
        settings.size > creature_size::medium
      ) {
        return PF_IMPASSABLE;
    }

    // If it's a door and we can open it from the tile we're on, cool.
    if( allow_open_doors && ( terrain.open || furniture.open ) &&
        ( ( !terrain.has_flag( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE ) &&
            !furniture.has_flag( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE ) ) ||
          !is_outside( cur ) ) ) {
        // Only try to open INSIDE doors from the inside
        // To open and then move onto the tile
        return 4;
    }

    // Otherwise, if we can bash, we'll consider that.
    if( !bash.empty() ) {
        const int rating = bash_rating_internal( bash, furniture, terrain, false, veh, part );

        if( rating > 1 ) {
            // Expected number of turns to bash it down, 1 turn to move there
            // and 5 turns of penalty not to trash everything just because we can
            return ( 20 / rating ) + 2 + 10;
        }

        if( rating == 1 ) {
            // Desperate measures, avoid whenever possible
            return 500;
        }
    }

    // If we can open doors generally but couldn't open this one, maybe we can
    // try from another direction.
    if( allow_open_doors && terrain.open && furniture.open ) {
        return PF_IMPASSABLE_FROM_HERE;
    }

    return PF_IMPASSABLE;
}

int map::cost_to_avoid( const tripoint_bub_ms & /*cur*/, const tripoint_bub_ms &p,
                        const pathfinding_settings &settings, PathfindingFlags p_special ) const
{
    if( settings.avoid_traps && ( p_special & PathfindingFlag::DangerousTrap ) ) {
        const const_maptile &tile = maptile_at_internal( p );
        const ter_t &terrain = tile.get_ter_t();
        const trap &ter_trp = terrain.trap.obj();
        const trap &trp = ter_trp.is_benign() ? tile.get_trap_t() : ter_trp;

        // NO_FLOOR is a special case handled in map::route
        if( !trp.is_benign() && !terrain.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
            return 500;
        }
    }

    if( settings.avoid_dangerous_fields && ( p_special & PathfindingFlag::DangerousField ) ) {
        // We'll walk through even known-dangerous fields if we absolutely have to.
        return 500;
    }

    return 0;
}

int map::extra_cost( const tripoint_bub_ms &cur, const tripoint_bub_ms &p,
                     const pathfinding_settings &settings,
                     PathfindingFlags p_special ) const
{
    int pass_cost = cost_to_pass( tripoint_bub_ms( cur ), tripoint_bub_ms( p ), settings, p_special );
    if( pass_cost < 0 ) {
        return pass_cost;
    }

    int avoid_cost = cost_to_avoid( cur, p, settings, p_special );
    if( avoid_cost < 0 ) {
        return avoid_cost;
    }
    return pass_cost + avoid_cost;
}

std::vector<tripoint_bub_ms> map::route( const Creature &who,
        const pathfinding_target &target ) const
{
    return route( who.pos_bub(), target, who.get_pathfinding_settings(), who.get_path_avoid() );
}

std::vector<tripoint_bub_ms> map::route( const tripoint_bub_ms &f,
        const pathfinding_target &target,
        const pathfinding_settings &settings,
        const std::function<bool( const tripoint_bub_ms & )> &avoid ) const
{
    /* TODO: If the origin or destination is out of bound, figure out the closest
     * in-bounds point and go to that, then to the real origin/destination.
     */
    std::vector<tripoint_bub_ms> ret;
    const tripoint_bub_ms &t = target.center;

    if( f == t || !inbounds( f ) ) {
        return ret;
    }

    if( !inbounds( t ) ) {
        tripoint_bub_ms clipped = t;
        clip_to_bounds( clipped );
        const pathfinding_target clipped_target = { clipped, target.r };
        return route( f, clipped_target, settings, avoid );
    }
    // First, check for a simple straight line on flat ground
    // Except when the line contains a pre-closed tile - we need to do regular pathing then
    if( f.z() == t.z() ) {
        auto line_path = straight_route( f, t );
        if( !line_path.empty() ) {
            const pathfinding_cache &pf_cache = get_pathfinding_cache_ref( f.z() );
            auto should_avoid = [&avoid, &pf_cache]( const tripoint_bub_ms & p ) {
                PathfindingFlags flags_copy = PathfindingFlags( pf_cache.special[p.xy()] );
                flags_copy.set_clear( PathfindingFlag::Ground );
                if( flags_copy.is_any_set() ) {
                    // If the straight line goes through any tile with any sort of special, then we
                    // don't use the straight-line optimization. Instead, we fall back to regular
                    // pathfinding. The costs might make the pathfinder pick a different path.
                    return true;
                }
                return avoid( p );
            };
            if( std::none_of( line_path.begin(), line_path.end(), should_avoid ) ) {
                return line_path;
            }
        }
    }

    // If expected path length is greater than max distance, allow only line path, like above
    if( rl_dist( f, t ) > settings.max_dist ) {
        return ret;
    }

    const int max_length = settings.max_length;

    const int pad = 16;  // Should be much bigger - low value makes pathfinders dumb!
    tripoint_bub_ms min( std::min( f.x(), t.x() ) - pad, std::min( f.y(), t.y() ) - pad,
                         std::min( f.z(), t.z() ) );
    tripoint_bub_ms max( std::max( f.x(), t.x() ) + pad, std::max( f.y(), t.y() ) + pad,
                         std::max( f.z(), t.z() ) );
    clip_to_bounds( min.x(), min.y(), min.z() );
    clip_to_bounds( max.x(), max.y(), max.z() );

    pf.reset( min.z(), max.z() );

    pf.add_point( 0, 0, f, f );

    bool done = false;
    tripoint_bub_ms found_target;

    do {
        tripoint_bub_ms cur( pf.get_next() );

        const int parent_index = flat_index( cur.xy() );
        path_data_layer &layer = pf.get_layer( cur.z() );
        if( layer.closed[parent_index] ) {
            continue;
        }

        if( layer.gscore[parent_index] > max_length ) {
            // Shortest path would be too long, return empty vector
            return std::vector<tripoint_bub_ms>();
        }

        if( target.contains( cur ) ) {
            done = true;
            found_target = cur;
            break;
        }

        layer.closed[parent_index] = true;

        const pathfinding_cache &pf_cache = get_pathfinding_cache_ref( cur.z() );
        const PathfindingFlags cur_special = pf_cache.special[cur.x()][cur.y()];

        // 7 3 5
        // 1 . 2
        // 6 4 8
        constexpr std::array<int, 8> x_offset{ { -1,  1,  0,  0,  1, -1, -1, 1 } };
        constexpr std::array<int, 8> y_offset{ {  0,  0, -1,  1, -1,  1, -1, 1 } };
        for( size_t i = 0; i < 8; i++ ) {
            const tripoint_bub_ms p( cur.x() + x_offset[i], cur.y() + y_offset[i], cur.z() );
            const int index = flat_index( p.xy() );

            // TODO: Remove this and instead have sentinels at the edges
            if( p.x() < min.x() || p.x() >= max.x() || p.y() < min.y() || p.y() >= max.y() ) {
                continue;
            }

            if( !target.contains( p ) && avoid( p ) ) {
                layer.closed[index] = true;
                continue;
            }

            if( layer.closed[index] ) {
                continue;
            }

            // Penalize for diagonals or the path will look "unnatural"
            int newg = layer.gscore[parent_index] + ( ( cur.x() != p.x() && cur.y() != p.y() ) ? 1 : 0 );

            const PathfindingFlags p_special = pf_cache.special[p.x()][p.y()];
            const int cost = extra_cost( cur, p, settings, p_special );
            if( cost < 0 ) {
                if( cost == PF_IMPASSABLE ) {
                    layer.closed[index] = true;
                }
                continue;
            }
            newg += cost;

            // Special case: pathfinders that avoid traps can avoid ledges by
            // climbing down. This can't be covered by |extra_cost| because it
            // can add a new point to the search.
            if( settings.avoid_traps && ( p_special & PathfindingFlag::DangerousTrap ) ) {
                const const_maptile &tile = maptile_at_internal( p );
                const ter_t &terrain = tile.get_ter_t();
                const trap &ter_trp = terrain.trap.obj();
                const trap &trp = ter_trp.is_benign() ? tile.get_trap_t() : ter_trp;
                if( !trp.is_benign() && terrain.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
                    // Warning: really expensive, needs a cache
                    tripoint_bub_ms below( p + tripoint::below );
                    if( valid_move( p, below, false, true ) ) {
                        if( !has_flag( ter_furn_flag::TFLAG_NO_FLOOR, below ) ) {
                            // From cur, not p, because we won't be walking on air.
                            // Use outer layer (cur.z()) for gscore -- the destination
                            // layer may contain stale data at parent_index.
                            int new_g = layer.gscore[parent_index] + 10;
                            pf.add_point( new_g, new_g + 2 * rl_dist( below, t ),
                                          cur, below );
                        }

                        // Close p on the current z-level -- we won't walk on air
                        layer.closed[index] = true;
                        continue;
                    }
                }
            }

            // Bare NO_FLOOR tiles without a trap: try to climb down, then close.
            if( settings.avoid_traps && ( p_special & PathfindingFlag::Air ) ) {
                tripoint_bub_ms below( p + tripoint::below );
                if( valid_move( p, below, false, true ) ) {
                    if( !has_flag( ter_furn_flag::TFLAG_NO_FLOOR, below ) ) {
                        int new_g = layer.gscore[parent_index] + 10;
                        pf.add_point( new_g, new_g + 2 * rl_dist( below, t ),
                                      cur, below );
                    }
                }
                layer.closed[index] = true;
                continue;
            }

            pf.add_point( newg, newg + 2 * rl_dist( p, t ), cur, p );
        }

        // TODO: We should be able to go up ramps even if we can't climb stairs.
        if( !( cur_special & ( PathfindingFlag::GoesUp | PathfindingFlag::GoesDown ) ) ||
            !settings.allow_climb_stairs ) {
            // The part below is only for z-level pathing
            continue;
        }

        bool rope_ladder = false;
        const const_maptile &parent_tile = maptile_at_internal( cur );
        const ter_t &parent_terrain = parent_tile.get_ter_t();
        if( settings.allow_climb_stairs && cur.z() > min.z() &&
            parent_terrain.has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ) {
            std::optional<tripoint_bub_ms> opt_dest = g->find_or_make_stairs( get_map(),
                    cur.z() - 1, rope_ladder, false, cur );
            if( !opt_dest ) {
                continue;
            }
            tripoint_bub_ms dest( opt_dest.value() );
            if( vertical_move_destination( *this, ter_furn_flag::TFLAG_GOES_UP, dest ) ) {
                if( !inbounds( dest ) ) {
                    continue;
                }
                // Use outer layer (cur.z()) for gscore -- the destination
                // layer may contain stale data at parent_index.
                int new_g = layer.gscore[parent_index] + 2;
                pf.add_point( new_g, new_g + 2 * rl_dist( dest, t ),
                              cur, dest );
            }
        }
        if( settings.allow_climb_stairs && cur.z() < max.z() &&
            parent_terrain.has_flag( ter_furn_flag::TFLAG_GOES_UP ) ) {
            std::optional<tripoint_bub_ms> opt_dest = g->find_or_make_stairs( get_map(),
                    cur.z() + 1, rope_ladder, false, cur );
            if( !opt_dest ) {
                continue;
            }
            tripoint_bub_ms dest( opt_dest.value() );
            if( vertical_move_destination( *this, ter_furn_flag::TFLAG_GOES_DOWN, dest ) ) {
                if( !inbounds( dest ) ) {
                    continue;
                }
                int new_g = layer.gscore[parent_index] + 2;
                pf.add_point( new_g, new_g + 2 * rl_dist( dest, t ),
                              cur, dest );
            }
        }
        if( cur.z() < max.z() && parent_terrain.has_flag( ter_furn_flag::TFLAG_RAMP ) &&
            valid_move( cur, cur + tripoint::above, false, true ) ) {
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint_bub_ms above( cur.x() + x_offset[it], cur.y() + y_offset[it], cur.z() + 1 );
                if( !inbounds( above ) ) {
                    continue;
                }
                int new_g = layer.gscore[parent_index] + 4;
                pf.add_point( new_g, new_g + 2 * rl_dist( above, t ),
                              cur, above );
            }
        }
        if( cur.z() < max.z() && parent_terrain.has_flag( ter_furn_flag::TFLAG_RAMP_UP ) &&
            valid_move( cur, cur + tripoint::above, false, true, true ) ) {
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint_bub_ms above( cur.x() + x_offset[it], cur.y() + y_offset[it], cur.z() + 1 );
                if( !inbounds( above ) ) {
                    continue;
                }
                int new_g = layer.gscore[parent_index] + 4;
                pf.add_point( new_g, new_g + 2 * rl_dist( above, t ),
                              cur, above );
            }
        }
        if( cur.z() > min.z() && parent_terrain.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN ) &&
            valid_move( cur, cur + tripoint::below, false, true, true ) ) {
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint_bub_ms below( cur.x() + x_offset[it], cur.y() + y_offset[it], cur.z() - 1 );
                if( !inbounds( below ) ) {
                    continue;
                }
                int new_g = layer.gscore[parent_index] + 4;
                pf.add_point( new_g, new_g + 2 * rl_dist( below, t ),
                              cur, below );
            }
        }

    } while( !done && !pf.empty() );

    if( done ) {
        ret.reserve( rl_dist( f, found_target ) * 2 );
        tripoint_bub_ms cur = found_target;
        // Just to limit max distance, in case something weird happens
        for( int fdist = max_length; fdist != 0; fdist-- ) {
            const int cur_index = flat_index( cur.xy() );
            const path_data_layer &layer = pf.get_layer( cur.z() );
            const tripoint_bub_ms &par = layer.parent[cur_index];
            if( cur == f ) {
                break;
            }

            ret.push_back( cur );
            // Jumps are acceptable on 1 z-level changes
            // This is because stairs teleport the player too
            if( rl_dist( cur, par ) > 1 && std::abs( cur.z() - par.z() ) != 1 ) {
                debugmsg( "Jump in our route!  %d:%d:%d->%d:%d:%d",
                          cur.x(), cur.y(), cur.z(), par.x(), par.y(), par.z() );
                return ret;
            }

            cur = par;
        }

        std::reverse( ret.begin(), ret.end() );
    }

    return ret;
}

// --- Grab-aware pathfinding helpers ---

// Number of grab direction slots: 3x3 grid encoding (x+1)*3 + (y+1), index 4 = center = unused.
static constexpr int GRAB_DIRS = 9;

static int grab_to_idx( const tripoint_rel_ms &g )
{
    return ( g.x() + 1 ) * 3 + ( g.y() + 1 );
}

static tripoint_rel_ms idx_to_grab( int idx )
{
    return tripoint_rel_ms( idx / 3 - 1, idx % 3 - 1, 0 );
}

static int grab_state_index( const point_bub_ms &pos, int grab_idx )
{
    return ( pos.x() * MAPSIZE_Y + pos.y() ) * GRAB_DIRS + grab_idx;
}

static point_bub_ms state_to_pos( int state_idx )
{
    int pos_idx = state_idx / GRAB_DIRS;
    return point_bub_ms( pos_idx / MAPSIZE_Y, pos_idx % MAPSIZE_Y );
}

// Estimate vehicle drag difficulty using FLAT/ROAD terrain flags (matching
// wheel terrain_modifiers): non-FLAT +4, FLAT non-ROAD +3, FLAT+ROAD +0.
static int veh_drag_cost( const map &here, const tripoint_bub_ms &pos )
{
    const int base = here.move_cost_ter_furn( pos );
    if( base <= 0 ) {
        return 0;
    }
    if( !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_FLAT, pos ) ) {
        return base + 4;
    }
    if( !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_ROAD, pos ) ) {
        return base + 3;
    }
    return base;
}

// Check if a tile would cause a vehicle collision, matching part_collision
// logic: impassable tiles block, bashable non-flat terrain/furniture blocks.
// allow_doors: pull moves pass true (vehicle follows player through opened
// doors); push/zigzag pass false (vehicle goes to unvisited tiles).
static bool tile_blocks_vehicle( const map &here, const tripoint_bub_ms &pos,
                                 bool allow_doors = true )
{
    const int ter_furn_cost = here.move_cost_ter_furn( pos );

    // Impassable terrain (walls, closed windows, locked doors).
    if( ter_furn_cost == 0 ) {
        if( allow_doors ) {
            // Openable doors: player opens them during auto-move, vehicle follows.
            const bool is_door = ( here.ter( pos ).obj().open &&
                                   here.ter( pos ).obj().has_flag( ter_furn_flag::TFLAG_DOOR ) ) ||
                                 ( here.has_furn( pos ) && here.furn( pos ).obj().open &&
                                   here.furn( pos ).obj().has_flag( ter_furn_flag::TFLAG_DOOR ) );
            if( is_door ) {
                return false;
            }
        }
        return true;
    }

    // No-floor tiles (open air) -- vehicle would fall
    if( here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, pos ) ) {
        return true;
    }

    // Flat ground (move_cost 2) never causes collision.
    if( ter_furn_cost == 2 ) {
        return false;
    }

    // Bashable non-flat terrain/furniture causes collision (bushes, open
    // windows, fences). NOCOLLIDE excluded (e.g. railroad tracks).
    if( here.is_bashable_ter_furn( pos, false ) &&
        !here.has_flag_ter_or_furn( ter_furn_flag::TFLAG_NOCOLLIDE, pos ) ) {
        return true;
    }

    return false;
}

// Returns the danger cost for a tile based on pathfinding flags and settings.
// 0 = safe, 500 = dangerous (trap or field), PF_IMPASSABLE = sharp.
static int tile_danger_cost( const pathfinding_settings &settings,
                             PathfindingFlags p_special )
{
    if( settings.avoid_sharp && ( p_special & PathfindingFlag::Sharp ) ) {
        return PF_IMPASSABLE;
    }

    if( settings.avoid_traps && ( p_special & PathfindingFlag::DangerousTrap ) ) {
        return 500;
    }

    if( settings.avoid_dangerous_fields && ( p_special & PathfindingFlag::DangerousField ) ) {
        return 500;
    }

    return 0;
}

// Check if a straight push or pull path works, skipping the full A*.
// Only applies when grab direction is aligned with travel direction
// (push) or opposite (pull), and every tile on the line is passable
// for both the player and the dragged vehicle.
// Assumes a single-tile vehicle (caller gates on this).
static std::vector<tripoint_bub_ms> try_straight_grab_path(
    map &here, const tripoint_bub_ms &start, const pathfinding_target &target,
    const tripoint_rel_ms &cur_grab, vehicle &grabbed_veh,
    const pathfinding_settings &settings, const pathfinding_cache &pf_cache,
    int cached_arm_str,
    const std::function<bool( const tripoint_bub_ms & )> &avoid )
{
    const tripoint_bub_ms &dest = target.center;
    const point d( dest.x() - start.x(), dest.y() - start.y() );
    const point ad( std::abs( d.x ), std::abs( d.y ) );

    // Must be a pure cardinal or diagonal direction
    if( ( ad.x != ad.y && ad.x > 0 && ad.y > 0 ) || ( ad.x == 0 && ad.y == 0 ) ) {
        return {};
    }

    const point s( d.x > 0 ? 1 : ( d.x < 0 ? -1 : 0 ), d.y > 0 ? 1 : ( d.y < 0 ? -1 : 0 ) );
    const tripoint_rel_ms dir( s.x, s.y, 0 );

    const bool is_push = cur_grab == dir;
    const bool is_pull = cur_grab == -dir;
    if( !is_push && !is_pull ) {
        return {};
    }

    const int num_steps = std::max( ad.x, ad.y );
    std::vector<tripoint_bub_ms> route;
    route.reserve( num_steps );

    tripoint_bub_ms player = start;
    tripoint_bub_ms vehicle_pos = start + cur_grab;

    for( int step = 0; step < num_steps; step++ ) {
        const tripoint_bub_ms next_player = player + dir;
        // Push: vehicle advances one step in travel direction.
        // Pull: vehicle follows to player's old position, which is
        // also vehicle_pos + dir (vehicle is one step behind player).
        const tripoint_bub_ms next_vehicle = vehicle_pos + dir;

        // Player passability (grabbed vehicle vacates the tile on push)
        if( here.move_cost( next_player, &grabbed_veh ) == 0 ) {
            return {};
        }
        if( here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, next_player ) ) {
            return {};
        }

        // Danger check on player tile
        const PathfindingFlags p_special =
            pf_cache.special[next_player.x()][next_player.y()];
        if( tile_danger_cost( settings, p_special ) != 0 ) {
            return {};
        }

        // Avoid callback (matching map::route semantics)
        if( !target.contains( next_player ) && avoid && avoid( next_player ) ) {
            return {};
        }

        // Vehicle passability
        if( tile_blocks_vehicle( here, next_vehicle, is_pull ) ) {
            return {};
        }
        // The grabbed vehicle is always at vehicle_pos (its current position),
        // so any vehicle found at next_vehicle is necessarily a different one.
        if( here.veh_at( next_vehicle ) ) {
            return {};
        }

        // Strength check: vehicle moves for both push and pull
        const int req_cur = grabbed_veh.drag_str_req_at( here, vehicle_pos );
        const int req_new = grabbed_veh.drag_str_req_at( here, next_vehicle );
        if( req_cur > cached_arm_str || req_new > cached_arm_str ) {
            return {};
        }

        route.push_back( next_player );
        if( target.contains( next_player ) ) {
            return route;
        }

        player = next_player;
        vehicle_pos = next_vehicle;
    }

    return {};
}

bool has_grabbed_single_tile_vehicle( const Character &you, const map &here )
{
    if( !you.is_avatar() || you.as_avatar()->get_grab_type() != object_type::VEHICLE ) {
        return false;
    }
    const tripoint_bub_ms veh_pos = you.pos_bub() + you.as_avatar()->grab_point;
    const optional_vpart_position ovp = here.veh_at( veh_pos );
    return ovp && ovp->vehicle().get_points().size() == 1;
}

// State is (player_position, grab_direction), so it finds routes where
// both the player and the dragged vehicle can physically move.
// Returns an empty vector if no path exists or if the player isn't
// dragging a single-tile vehicle.
// TODO: XY-only -- no ramp/stair support.
std::vector<tripoint_bub_ms> route_with_grab(
    map &here, const Character &you, const pathfinding_target &target,
    const std::function<bool( const tripoint_bub_ms & )> &avoid )
{
    std::vector<tripoint_bub_ms> ret;

    if( !you.is_avatar() || you.as_avatar()->get_grab_type() != object_type::VEHICLE ) {
        return ret;
    }

    const tripoint_bub_ms start = you.pos_bub();
    const tripoint_rel_ms start_grab = you.as_avatar()->grab_point;
    const tripoint_bub_ms veh_pos = start + start_grab;
    const optional_vpart_position ovp = here.veh_at( veh_pos );
    if( !ovp ) {
        add_msg_debug( debugmode::DF_ACTIVITY,
                       "route_with_grab: no vehicle at grab point %s+%s",
                       start.to_string_writable(), start_grab.to_string_writable() );
        return ret;
    }
    vehicle &grabbed_veh = ovp->vehicle();
    if( grabbed_veh.get_points().size() > 1 ) {
        add_msg_debug( debugmode::DF_ACTIVITY,
                       "route_with_grab: multi-tile vehicle (%zu parts), skipping",
                       grabbed_veh.get_points().size() );
        return ret;
    }

    add_msg_debug( debugmode::DF_ACTIVITY,
                   "route_with_grab: start=%s grab=%s target=%s r=%d",
                   start.to_string_writable(), start_grab.to_string_writable(),
                   target.center.to_string_writable(), target.r );

    pathfinding_settings settings = you.get_pathfinding_settings();
    // Always avoid dangerous fields when dragging a vehicle -- there is
    // no scenario where routing through fire with a cart is desirable.
    settings.avoid_dangerous_fields = true;
    const pathfinding_cache &pf_cache = here.get_pathfinding_cache_ref( start.z() );

    // Cache arm strength for strength checks
    const int cached_arm_str = you.get_arm_str();

    // Fast path: if grab is aligned with travel direction, check the
    // straight line before spinning up the full A* with its 157K-state arrays.
    ret = try_straight_grab_path( here, start, target, start_grab, grabbed_veh,
                                  settings, pf_cache, cached_arm_str, avoid );
    if( !ret.empty() ) {
        add_msg_debug( debugmode::DF_ACTIVITY,
                       "route_with_grab: straight line path len=%zu", ret.size() );
        return ret;
    }

    const int max_length = settings.max_length;
    const int pad = 16;
    const tripoint_bub_ms &t = target.center;

    point_bub_ms min_bound( std::min( start.x(), t.x() ) - pad,
                            std::min( start.y(), t.y() ) - pad );
    point_bub_ms max_bound( std::max( start.x(), t.x() ) + pad,
                            std::max( start.y(), t.y() ) + pad );
    min_bound.x() = std::max( min_bound.x(), 0 );
    min_bound.y() = std::max( min_bound.y(), 0 );
    max_bound.x() = std::min( max_bound.x(), MAPSIZE_X );
    max_bound.y() = std::min( max_bound.y(), MAPSIZE_Y );

    const int total_states = MAPSIZE_X * MAPSIZE_Y * GRAB_DIRS;

    // Reuse heap-allocated arrays across calls
    static std::vector<bool> closed;
    static std::vector<bool> open;
    static std::vector<int> gscore;
    static std::vector<int> parent;

    if( static_cast<int>( closed.size() ) != total_states ) {
        closed.resize( total_states );
        open.resize( total_states );
        gscore.resize( total_states );
        parent.resize( total_states );
    }

    // Only closed and open need clearing; gscore and parent are only read
    // for states where open[state] is true.
    std::fill( closed.begin(), closed.end(), false );
    std::fill( open.begin(), open.end(), false );

    // Priority queue: (f-score, state_index), smallest f-score first
    using pq_entry = std::pair<int, int>;
    std::priority_queue<pq_entry, std::vector<pq_entry>, std::greater<>> pq;

    const int start_grab_idx = grab_to_idx( start_grab );
    const int start_state = grab_state_index( start.xy(), start_grab_idx );
    gscore[start_state] = 0;
    open[start_state] = true;
    parent[start_state] = start_state;
    // Heuristic: 4 * chebyshev - 2 (one sideways move costs only 2).
    // Uses square_dist to stay admissible regardless of trigdist setting.
    pq.emplace( std::max( 0, 4 * square_dist( start, t ) - 2 ), start_state );

    // Movement offsets: W, E, N, S, NE, SW, NW, SE
    constexpr std::array<int, 8> x_off{ { -1,  1,  0,  0,  1, -1, -1, 1 } };
    constexpr std::array<int, 8> y_off{ {  0,  0, -1,  1, -1,  1, -1, 1 } };

    bool done = false;
    int found_state = -1;
    int states_explored = 0;

    while( !pq.empty() ) {
        const auto [cur_score, cur_state] = pq.top();
        pq.pop();

        if( closed[cur_state] ) {
            continue;
        }

        states_explored++;
        const int cur_g = gscore[cur_state];
        if( cur_g > max_length ) {
            add_msg_debug( debugmode::DF_ACTIVITY,
                           "route_with_grab: ABORTED max_length=%d explored=%d grab=%s",
                           max_length, states_explored, start_grab.to_string_writable() );
            return ret;
        }

        const point_bub_ms cur_pos = state_to_pos( cur_state );
        const int cur_grab_idx = cur_state % GRAB_DIRS;
        const tripoint_rel_ms cur_grab = idx_to_grab( cur_grab_idx );
        const tripoint_bub_ms cur3d( cur_pos, start.z() );

        if( target.contains( cur3d ) ) {
            done = true;
            found_state = cur_state;
            break;
        }

        closed[cur_state] = true;

        for( size_t i = 0; i < 8; i++ ) {
            const point_bub_ms next_pos( cur_pos.x() + x_off[i], cur_pos.y() + y_off[i] );

            if( next_pos.x() < min_bound.x() || next_pos.x() >= max_bound.x() ||
                next_pos.y() < min_bound.y() || next_pos.y() >= max_bound.y() ) {
                continue;
            }

            const tripoint_bub_ms next3d( next_pos, start.z() );

            // Avoid callback (matching map::route semantics)
            if( !target.contains( next3d ) && avoid && avoid( next3d ) ) {
                continue;
            }

            // Danger check on player's next position
            const PathfindingFlags p_special = pf_cache.special[next_pos.x()][next_pos.y()];
            const int danger = tile_danger_cost( settings, p_special );
            if( danger == PF_IMPASSABLE ) {
                continue;
            }

            // Player passability (grabbed vehicle vacates the tile on push)
            if( here.has_flag( ter_furn_flag::TFLAG_NO_FLOOR, next3d ) ) {
                continue;
            }
            int tile_cost = here.move_cost( next3d, &grabbed_veh );
            if( tile_cost == 0 ) {
                // Closed doors cost 4 (auto-move opens them).
                // Windows excluded - multi-step open, vehicle can't follow.
                const bool is_door = ( here.ter( next3d ).obj().open &&
                                       here.ter( next3d ).obj().has_flag( ter_furn_flag::TFLAG_DOOR ) ) ||
                                     ( here.furn( next3d ).obj().open &&
                                       here.furn( next3d ).obj().has_flag( ter_furn_flag::TFLAG_DOOR ) );
                if( is_door ) {
                    tile_cost = 4;
                } else {
                    continue;
                }
            }

            if( danger > 0 ) {
                tile_cost += danger;
            }

            const tripoint_rel_ms dp( x_off[i], y_off[i], 0 );
            tripoint_rel_ms dp_veh = -cur_grab;
            tripoint_rel_ms next_grab = cur_grab;
            bool vehicle_moves = true;
            bool is_zigzag = false;
            bool is_push = false;

            if( dp == cur_grab ) {
                // PUSH: vehicle moves in same direction as player
                dp_veh = dp;
                is_push = true;
            } else if( std::abs( dp.x() + dp_veh.x() ) != 2 &&
                       std::abs( dp.y() + dp_veh.y() ) != 2 ) {
                // SIDEWAYS: vehicle stays put, grab rotates
                next_grab = -( dp + dp_veh );
                vehicle_moves = false;
            } else if( ( dp.x() == cur_grab.x() || dp.y() == cur_grab.y() ) &&
                       cur_grab.x() != 0 && cur_grab.y() != 0 ) {
                // ZIGZAG: player is diagonal to vehicle, moves partially away
                dp_veh.x() = dp.x() == -dp_veh.x() ? 0 : dp_veh.x();
                dp_veh.y() = dp.y() == -dp_veh.y() ? 0 : dp_veh.y();
                next_grab = -dp_veh;
                is_zigzag = true;
            } else {
                // PULL: vehicle moves to player's old position
                next_grab = -dp;
                // dp_veh stays as -cur_grab (initialized above)
            }

            // FLAT/ROAD drag penalty for vehicle terrain
            int veh_terrain_cost = 0;

            if( vehicle_moves ) {
                const tripoint_bub_ms veh_new( cur_pos + cur_grab.xy() + dp_veh.xy(), start.z() );
                // veh_at() needed because other vehicles (e.g. ambulance aisles)
                // have passable move_cost but still block placement. dp_veh is
                // always non-zero here, so veh_new != current vehicle position.
                const bool other_veh = static_cast<bool>( here.veh_at( veh_new ) );
                // Push/zigzag: doors stay closed (vehicle goes to unvisited tiles)
                const bool allow_doors = !( is_push || is_zigzag );
                const bool veh_blocked = other_veh || tile_blocks_vehicle( here, veh_new, allow_doors );
                if( veh_blocked ) {
                    // Zigzag recovery: fall back to pull when vehicle collides
                    if( is_zigzag ) {
                        dp_veh = -cur_grab;
                        next_grab = -dp;
                        const tripoint_bub_ms veh_recover( cur_pos + cur_grab.xy() + dp_veh.xy(),
                                                           start.z() );
                        if( here.veh_at( veh_recover ) || tile_blocks_vehicle( here, veh_recover ) ) {
                            continue;
                        }
                        // Strength check on recovery position
                        {
                            const tripoint_bub_ms veh_cur( cur_pos + cur_grab.xy(), start.z() );
                            const int req_cur = grabbed_veh.drag_str_req_at( here, veh_cur );
                            const int req_rec = grabbed_veh.drag_str_req_at( here, veh_recover );
                            if( req_cur > cached_arm_str || req_rec > cached_arm_str ) {
                                continue;
                            }
                        }
                        veh_terrain_cost = veh_drag_cost( here, veh_recover );
                    } else {
                        continue;
                    }
                } else {
                    // Strength check: both current and new vehicle positions
                    {
                        const tripoint_bub_ms veh_cur( cur_pos + cur_grab.xy(), start.z() );
                        const int req_cur = grabbed_veh.drag_str_req_at( here, veh_cur );
                        const int req_new = grabbed_veh.drag_str_req_at( here, veh_new );
                        if( req_cur > cached_arm_str || req_new > cached_arm_str ) {
                            continue;
                        }
                    }
                    veh_terrain_cost = veh_drag_cost( here, veh_new );
                }
            }

            const int next_grab_idx = grab_to_idx( next_grab );
            const int next_state = grab_state_index( next_pos, next_grab_idx );

            if( closed[next_state] ) {
                continue;
            }

            // Diagonal penalty
            const bool diagonal = cur_pos.x() != next_pos.x() && cur_pos.y() != next_pos.y();
            const int newg = cur_g + tile_cost + veh_terrain_cost + ( diagonal ? 1 : 0 );

            if( open[next_state] && newg >= gscore[next_state] ) {
                continue;
            }

            open[next_state] = true;
            gscore[next_state] = newg;
            parent[next_state] = cur_state;
            const int h = std::max( 0, 4 * square_dist( next3d, t ) - 2 );
            pq.emplace( newg + h, next_state );
        }
    }

    if( !done ) {
        add_msg_debug( debugmode::DF_ACTIVITY,
                       "route_with_grab: NO PATH FOUND %s grab=%s to %s explored=%d bounds=%s-%s",
                       start.to_string_writable(), start_grab.to_string_writable(),
                       target.center.to_string_writable(), states_explored,
                       min_bound.to_string_writable(), max_bound.to_string_writable() );
        return ret;
    }

    // Reconstruct path
    int trace = found_state;
    while( trace != start_state ) {
        const point_bub_ms pos = state_to_pos( trace );
        ret.emplace_back( pos, start.z() );
        trace = parent[trace];
    }
    std::reverse( ret.begin(), ret.end() );

    add_msg_debug( debugmode::DF_ACTIVITY,
                   "route_with_grab: found path len=%zu from %s to %s first_step=%s",
                   ret.size(), start.to_string_writable(), target.center.to_string_writable(),
                   ret.empty() ? std::string( "none" ) : ret.front().to_string_writable() );
    return ret;
}

bool pathfinding_target::contains( const tripoint_bub_ms &p ) const
{
    if( r == 0 ) {
        return center == p;
    }
    return square_dist( center, p ) <= r;
}
