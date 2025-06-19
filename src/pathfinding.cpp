#include "pathfinding.h"

#include <algorithm>
#include <array>
#include <bitset>
#include <cstdlib>
#include <functional>
#include <memory>
#include <optional>
#include <queue>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "coordinates.h"
#include "creature.h"
#include "debug.h"
#include "game.h"
#include "line.h"
#include "map.h"
#include "map_scale_constants.h"
#include "mapdata.h"
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

    const int bash = settings.bash_strength;
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
            } else if( bash > 0 ) {
                // Car obstacle that isn't a door
                // TODO: Account for armor
                int hp = veh->part( part ).hp();
                if( hp / 20 > bash ) {
                    // Threshold damage thing means we just can't bash this down
                    return PF_IMPASSABLE;
                } else if( hp / 10 > bash ) {
                    // Threshold damage thing means we will fail to deal damage pretty often
                    hp *= 2;
                }

                return 2 * hp / bash + 8 + 4;
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
    if( bash > 0 ) {
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
                            // Otherwise this would have been a huge fall
                            path_data_layer &layer = pf.get_layer( p.z() - 1 );
                            // From cur, not p, because we won't be walking on air
                            pf.add_point( layer.gscore[parent_index] + 10,
                                          layer.score[parent_index] + 10 + 2 * rl_dist( below, t ),
                                          cur, below );
                        }

                        // Close p, because we won't be walking on it
                        layer.closed[index] = true;
                        continue;
                    }
                }
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
                path_data_layer &layer = pf.get_layer( dest.z() );
                pf.add_point( layer.gscore[parent_index] + 2,
                              layer.score[parent_index] + 2 * rl_dist( dest, t ),
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
                path_data_layer &layer = pf.get_layer( dest.z() );
                pf.add_point( layer.gscore[parent_index] + 2,
                              layer.score[parent_index] + 2 * rl_dist( dest, t ),
                              cur, dest );
            }
        }
        if( cur.z() < max.z() && parent_terrain.has_flag( ter_furn_flag::TFLAG_RAMP ) &&
            valid_move( cur, cur + tripoint::above, false, true ) ) {
            path_data_layer &layer = pf.get_layer( cur.z() + 1 );
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint_bub_ms above( cur.x() + x_offset[it], cur.y() + y_offset[it], cur.z() + 1 );
                if( !inbounds( above ) ) {
                    continue;
                }
                pf.add_point( layer.gscore[parent_index] + 4,
                              layer.score[parent_index] + 4 + 2 * rl_dist( above, t ),
                              cur, above );
            }
        }
        if( cur.z() < max.z() && parent_terrain.has_flag( ter_furn_flag::TFLAG_RAMP_UP ) &&
            valid_move( cur, cur + tripoint::above, false, true, true ) ) {
            path_data_layer &layer = pf.get_layer( cur.z() + 1 );
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint_bub_ms above( cur.x() + x_offset[it], cur.y() + y_offset[it], cur.z() + 1 );
                if( !inbounds( above ) ) {
                    continue;
                }
                pf.add_point( layer.gscore[parent_index] + 4,
                              layer.score[parent_index] + 4 + 2 * rl_dist( above, t ),
                              cur, above );
            }
        }
        if( cur.z() > min.z() && parent_terrain.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN ) &&
            valid_move( cur, cur + tripoint::below, false, true, true ) ) {
            path_data_layer &layer = pf.get_layer( cur.z() - 1 );
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint_bub_ms below( cur.x() + x_offset[it], cur.y() + y_offset[it], cur.z() - 1 );
                if( !inbounds( below ) ) {
                    continue;
                }
                pf.add_point( layer.gscore[parent_index] + 4,
                              layer.score[parent_index] + 4 + 2 * rl_dist( below, t ),
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

bool pathfinding_target::contains( const tripoint_bub_ms &p ) const
{
    if( r == 0 ) {
        return center == p;
    }
    return square_dist( center, p ) <= r;
}
