#include "pathfinding.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <iterator>
#include <memory>
#include <optional>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "coordinates.h"
#include "debug.h"
#include "flood_fill.h"
#include "game.h"
#include "gates.h"
#include "line.h"
#include "map.h"
#include "mapdata.h"
#include "point.h"
#include "submap.h"
#include "trap.h"
#include "type_id.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"

enum astar_state : uint8_t {
    ASL_NONE,
    ASL_OPEN,
    ASL_CLOSED
};

// Turns two indexed to a 2D array into an index to equivalent 1D array
static constexpr int flat_index( const point &p )
{
    return ( p.x * MAPSIZE_Y ) + p.y;
}

// Flattened 2D array representing a single z-level worth of pathfinding data
struct path_data_layer {
    // State is accessed way more often than all other values here
    std::array< astar_state, MAPSIZE_X *MAPSIZE_Y > state;
    std::array< int, MAPSIZE_X *MAPSIZE_Y > score;
    std::array< int, MAPSIZE_X *MAPSIZE_Y > gscore;
    std::array< tripoint, MAPSIZE_X *MAPSIZE_Y > parent;

    void init( const point &min, const point &max ) {
        for( int x = min.x; x <= max.x; x++ ) {
            for( int y = min.y; y <= max.y; y++ ) {
                const int ind = flat_index( point( x, y ) );
                state[ind] = ASL_NONE; // Mark as unvisited
            }
        }
    }
};

namespace
{
// Pathfinder is only ever finding a single path at a time, so it's fine for these to be shared between runs.
std::array< bool, OVERMAP_LAYERS > initialized_path_data;
std::array< path_data_layer, OVERMAP_LAYERS > path_data;
}

struct pathfinder {
    point min;
    point max;
    pathfinder( const point &_min, const point &_max ) :
        min( _min ), max( _max ) {
        initialized_path_data.fill( false );
    }

    std::priority_queue< std::pair<int, tripoint>, std::vector< std::pair<int, tripoint> >, pair_greater_cmp_first >
    open;

    path_data_layer &get_layer( const int z ) {
        path_data_layer &ptr = path_data[z + OVERMAP_DEPTH];
        if( !initialized_path_data[z + OVERMAP_DEPTH] ) {
            initialized_path_data[z + OVERMAP_DEPTH] = true;
            ptr.init( min, max );
        }
        return ptr;
    }

    bool empty() const {
        return open.empty();
    }

    tripoint get_next() {
        const auto pt = open.top();
        open.pop();
        return pt.second;
    }

    void add_point( const int gscore, const int score, const tripoint &from, const tripoint &to ) {
        path_data_layer &layer = get_layer( to.z );
        const int index = flat_index( to.xy() );
        if( ( layer.state[index] == ASL_OPEN && gscore >= layer.gscore[index] ) ||
            layer.state[index] == ASL_CLOSED ) {
            return;
        }

        layer.state [index] = ASL_OPEN;
        layer.gscore[index] = gscore;
        layer.parent[index] = from;
        layer.score [index] = score;
        open.emplace( score, to );
    }

    void close_point( const tripoint &p ) {
        path_data_layer &layer = get_layer( p.z );
        const int index = flat_index( p.xy() );
        layer.state[index] = ASL_CLOSED;
    }

    void unclose_point( const tripoint &p ) {
        path_data_layer &layer = get_layer( p.z );
        const int index = flat_index( p.xy() );
        layer.state[index] = ASL_NONE;
    }
};

// Modifies `t` to point to a tile with `flag` in a 1-submap radius of `t`'s original value,
// searching nearest points first (starting with `t` itself).
// return false if it could not find a suitable point
static bool vertical_move_destination( const map &m, ter_furn_flag flag, tripoint &t )
{
    const pathfinding_cache &pf_cache = m.get_pathfinding_cache_ref( t.z );
    for( const point &p : closest_points_first( t.xy(), SEEX ) ) {
        if( pf_cache.special[p.x][p.y] & PF_UPDOWN ) {
            const tripoint t2( p, t.z );
            if( m.has_flag( flag, t2 ) ) {
                t = t2;
                return true;
            }
        }
    }
    return false;
}

std::vector<tripoint> map::route( const tripoint &f, const tripoint &t,
                                  const pathfinding_settings &settings,
                                  std::function<bool( const tripoint & )> should_avoid ) const
{
    /* TODO: If the origin or destination is out of bound, figure out the closest
     * in-bounds point and go to that, then to the real origin/destination.
     */
    std::vector<tripoint> ret;

    if( f == t || !inbounds( f ) ) {
        return ret;
    }

    if( !inbounds( t ) ) {
        tripoint clipped = t;
        clip_to_bounds( clipped );
        return route( f, clipped, settings, std::move( should_avoid ) );
    }
    // First, check for a simple straight line on flat ground
    // Except when the line contains a pre-closed tile - we need to do regular pathing then
    static const pf_special non_normal = PF_SLOW | PF_WALL | PF_VEHICLE | PF_TRAP | PF_SHARP;
    if( f.z == t.z ) {
        auto line_path = line_to( f, t );
        const pathfinding_cache &pf_cache = get_pathfinding_cache_ref( f.z );
        // Check all points for any special case (including just hard terrain)
        if( !std::any_of( line_path.begin(), line_path.end(), [&pf_cache]( const tripoint & p ) {
        return pf_cache.special[p.x][p.y] & non_normal;
        } ) ) {
            if( !std::any_of( line_path.begin(), line_path.end(), should_avoid ) ) {
                return line_path;
            }
        }
    }

    // If expected path length is greater than max distance, allow only line path, like above
    if( rl_dist( f, t ) > settings.max_dist ) {
        return ret;
    }

    const int max_length = settings.max_length;
    const int bash = settings.bash_strength;
    const int climb_cost = settings.climb_cost;
    const bool doors = settings.allow_open_doors;
    const bool locks = settings.allow_unlock_doors;
    const bool trapavoid = settings.avoid_traps;
    const bool roughavoid = settings.avoid_rough_terrain;
    const bool sharpavoid = settings.avoid_sharp;

    const pathfinding_cache &pf_cache = get_pathfinding_cache_ref( f.z );
    const pathfinding_cache &other_pf_cache = get_pathfinding_cache_ref( t.z );

    // One of us might be stuck in a room we can't get out of.
    const int zone = pf_cache.zones[f.x][f.y];
    const int other_zone = other_pf_cache.zones[t.x][t.y];

    // Are they stuck in here with us?
    if( f.z != t.z || zone != other_zone ) {
        // Not in same zone, check the max bash needed to reach.
        const int bash_needed = std::max( pf_cache.stuck_threshold_by_zone[zone],
                                          other_pf_cache.stuck_threshold_by_zone[other_zone] );
        if( bash < bash_needed ) {
            // Give up now.
            return ret;
        }
    }

    int max_bash_larger_than_ours = bash + 1;

    const int pad = 16;  // Should be much bigger - low value makes pathfinders dumb!
    tripoint min( std::min( f.x, t.x ) - pad, std::min( f.y, t.y ) - pad, std::min( f.z, t.z ) );
    tripoint max( std::max( f.x, t.x ) + pad, std::max( f.y, t.y ) + pad, std::max( f.z, t.z ) );
    clip_to_bounds( min.x, min.y, min.z );
    clip_to_bounds( max.x, max.y, max.z );

    pathfinder pf( min.xy(), max.xy() );

    // Start and end must not be closed
    pf.unclose_point( f );
    pf.unclose_point( t );
    pf.add_point( 0, 0, f, f );

    bool done = false;

    do {
        tripoint cur = pf.get_next();

        const int parent_index = flat_index( cur.xy() );
        path_data_layer &layer = pf.get_layer( cur.z );
        auto &cur_state = layer.state[parent_index];
        if( cur_state == ASL_CLOSED ) {
            continue;
        }

        if( layer.gscore[parent_index] > max_length ) {
            // Shortest path would be too long, return empty vector
            return std::vector<tripoint>();
        }

        if( cur == t ) {
            done = true;
            break;
        }

        cur_state = ASL_CLOSED;

        if( should_avoid( cur ) ) {
            continue;
        }

        const pathfinding_cache &pf_cache = get_pathfinding_cache_ref( cur.z );
        const pf_special cur_special = pf_cache.special[cur.x][cur.y];

        // Cache friendly traversal order.
        // 1 4 6
        // 2 . 7
        // 3 5 8
        constexpr std::array<int, 8> x_offset{{ -1, -1, -1,  0, 0,  1, 1, 1 }};
        constexpr std::array<int, 8> y_offset{{ -1,  0,  1, -1, 1, -1, 0, 1 }};
        for( size_t i = 0; i < 8; i++ ) {
            const tripoint p( cur.x + x_offset[i], cur.y + y_offset[i], cur.z );
            const int index = flat_index( p.xy() );

            // TODO: Remove this and instead have sentinels at the edges
            if( p.x < min.x || p.x >= max.x || p.y < min.y || p.y >= max.y ) {
                continue;
            }

            if( layer.state[index] == ASL_CLOSED ) {
                continue;
            }

            // Penalize for diagonals or the path will look "unnatural"
            int newg = layer.gscore[parent_index] + ( ( cur.x != p.x && cur.y != p.y ) ? 1 : 0 );

            const pf_special p_special = pf_cache.special[p.x][p.y];
            // TODO: De-uglify, de-huge-n
            if( !( p_special & non_normal ) ) {
                // Boring flat dirt - the most common case above the ground
                newg += 2;
            } else {
                if( roughavoid ) {
                    layer.state[index] = ASL_CLOSED; // Close all rough terrain tiles
                    continue;
                }

                const int cost = pf_cache.move_costs[p.x][p.y];
                const std::pair<int, int> &bash_range = pf_cache.bash_ranges[p.x][p.y];
                max_bash_larger_than_ours = std::max( max_bash_larger_than_ours, bash_range.first );
                const int rating = cost == 0 ? bash_rating_from_range_internal( bash, bash_range ) : -1;

                const bool is_door = doors && p_special & PF_DOOR;
                const bool is_inside_door = is_door && p_special & PF_INSIDE_DOOR;
                const bool is_vehicle = p_special & PF_VEHICLE;

                if( cost == 0 && rating <= 0 && !is_door && !is_vehicle && climb_cost <= 0 ) {
                    layer.state[index] = ASL_CLOSED; // Close it so that next time we won't try to calculate costs
                    continue;
                }

                newg += cost;
                if( cost == 0 ) {
                    if( climb_cost > 0 && p_special & PF_CLIMBABLE ) {
                        // Climbing fences
                        newg += climb_cost;
                    } else if( is_door && ( !is_inside_door || !is_outside( cur ) ) ) {
                        // Only try to open INSIDE doors from the inside
                        // To open and then move onto the tile
                        newg += 4;
                    } else if( is_vehicle ) {
                        int dummy = -1;
                        if( is_door && ( !is_inside_door ||
                                         veh_at_internal( cur, dummy ) == veh_at_internal( p, dummy ) ) ) {
                            // Handle car doors, but don't try to path through curtains
                            newg += 10; // One turn to open, 4 to move there
                        } else if( locks && ( p_special & PF_LOCK ) ) {
                            newg += 12; // 2 turns to open, 4 to move there
                        } else if( bash > 0 && rating > 0 ) {
                            // Car obstacle that isn't a door
                            // Expected number of turns to bash it down, 1 turn to move there
                            // and 5 turns of penalty not to trash everything just because we can
                            newg += ( 20 / rating ) + 2 + 10;
                        } else {
                            if( !is_door ) {
                                // Won't be openable, don't try from other sides
                                layer.state[index] = ASL_CLOSED;
                            }
                            continue;
                        }
                    } else if( rating > 1 ) {
                        // Expected number of turns to bash it down, 1 turn to move there
                        // and 5 turns of penalty not to trash everything just because we can
                        newg += ( 20 / rating ) + 2 + 10;
                    } else if( rating == 1 ) {
                        // Desperate measures, avoid whenever possible
                        newg += 500;
                    } else {
                        // Unbashable and unopenable from here
                        if( !is_door ) {
                            // Or anywhere else for that matter
                            layer.state[index] = ASL_CLOSED;
                        }

                        continue;
                    }
                }

                if( trapavoid && ( p_special & PF_TRAP ) ) {
                    const const_maptile &tile = maptile_at_internal( p );
                    const ter_t &terrain = tile.get_ter_t();
                    const trap &ter_trp = terrain.trap.obj();
                    const trap &trp = ter_trp.is_benign() ? tile.get_trap_t() : ter_trp;
                    if( !trp.is_benign() ) {
                        // For now make them detect all traps
                        if( terrain.has_flag( ter_furn_flag::TFLAG_NO_FLOOR ) ) {
                            // Special case - ledge in z-levels
                            // Warning: really expensive, needs a cache
                            if( valid_move( p, tripoint( p.xy(), p.z - 1 ), false, true ) ) {
                                tripoint below( p.xy(), p.z - 1 );
                                if( !has_flag( ter_furn_flag::TFLAG_NO_FLOOR, below ) ) {
                                    // Otherwise this would have been a huge fall
                                    path_data_layer &layer = pf.get_layer( p.z - 1 );
                                    // From cur, not p, because we won't be walking on air
                                    pf.add_point( layer.gscore[parent_index] + 10,
                                                  layer.score[parent_index] + 10 + 2 * rl_dist( below, t ),
                                                  cur, below );
                                }

                                // Close p, because we won't be walking on it
                                layer.state[index] = ASL_CLOSED;
                                continue;
                            }
                        } else {
                            // Otherwise it's walkable
                            newg += 500;
                        }
                    }
                }

                if( sharpavoid && p_special & PF_SHARP ) {
                    layer.state[index] = ASL_CLOSED; // Avoid sharp things
                }

            }

            // If not visited, add as open
            // If visited, add it only if we can do so with better score
            if( layer.state[index] == ASL_NONE || newg < layer.gscore[index] ) {
                pf.add_point( newg, newg + 2 * rl_dist( p, t ), cur, p );
            }
        }

        if( !( cur_special & PF_UPDOWN ) || !settings.allow_climb_stairs ) {
            // The part below is only for z-level pathing
            continue;
        }

        bool rope_ladder = false;
        const const_maptile &parent_tile = maptile_at_internal( cur );
        const ter_t &parent_terrain = parent_tile.get_ter_t();
        if( settings.allow_climb_stairs && cur.z > min.z &&
            parent_terrain.has_flag( ter_furn_flag::TFLAG_GOES_DOWN ) ) {
            std::optional<tripoint> opt_dest = g->find_or_make_stairs( get_map(),
                                               cur.z - 1, rope_ladder, false, cur );
            if( !opt_dest ) {
                continue;
            }
            tripoint dest = opt_dest.value();
            if( vertical_move_destination( *this, ter_furn_flag::TFLAG_GOES_UP, dest ) ) {
                if( !inbounds( dest ) ) {
                    continue;
                }
                path_data_layer &layer = pf.get_layer( dest.z );
                pf.add_point( layer.gscore[parent_index] + 2,
                              layer.score[parent_index] + 2 * rl_dist( dest, t ),
                              cur, dest );
            }
        }
        if( settings.allow_climb_stairs && cur.z < max.z &&
            parent_terrain.has_flag( ter_furn_flag::TFLAG_GOES_UP ) ) {
            std::optional<tripoint> opt_dest = g->find_or_make_stairs( get_map(),
                                               cur.z + 1, rope_ladder, false, cur );
            if( !opt_dest ) {
                continue;
            }
            tripoint dest = opt_dest.value();
            if( vertical_move_destination( *this, ter_furn_flag::TFLAG_GOES_DOWN, dest ) ) {
                if( !inbounds( dest ) ) {
                    continue;
                }
                path_data_layer &layer = pf.get_layer( dest.z );
                pf.add_point( layer.gscore[parent_index] + 2,
                              layer.score[parent_index] + 2 * rl_dist( dest, t ),
                              cur, dest );
            }
        }
        if( cur.z < max.z && parent_terrain.has_flag( ter_furn_flag::TFLAG_RAMP ) &&
            valid_move( cur, tripoint( cur.xy(), cur.z + 1 ), false, true ) ) {
            path_data_layer &layer = pf.get_layer( cur.z + 1 );
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint above( cur.x + x_offset[it], cur.y + y_offset[it], cur.z + 1 );
                if( !inbounds( above ) ) {
                    continue;
                }
                pf.add_point( layer.gscore[parent_index] + 4,
                              layer.score[parent_index] + 4 + 2 * rl_dist( above, t ),
                              cur, above );
            }
        }
        if( cur.z < max.z && parent_terrain.has_flag( ter_furn_flag::TFLAG_RAMP_UP ) &&
            valid_move( cur, tripoint( cur.xy(), cur.z + 1 ), false, true, true ) ) {
            path_data_layer &layer = pf.get_layer( cur.z + 1 );
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint above( cur.x + x_offset[it], cur.y + y_offset[it], cur.z + 1 );
                if( !inbounds( above ) ) {
                    continue;
                }
                pf.add_point( layer.gscore[parent_index] + 4,
                              layer.score[parent_index] + 4 + 2 * rl_dist( above, t ),
                              cur, above );
            }
        }
        if( cur.z > min.z && parent_terrain.has_flag( ter_furn_flag::TFLAG_RAMP_DOWN ) &&
            valid_move( cur, tripoint( cur.xy(), cur.z - 1 ), false, true, true ) ) {
            path_data_layer &layer = pf.get_layer( cur.z - 1 );
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint below( cur.x + x_offset[it], cur.y + y_offset[it], cur.z - 1 );
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
        ret.reserve( rl_dist( f, t ) * 2 );
        tripoint cur = t;
        // Just to limit max distance, in case something weird happens
        for( int fdist = max_length; fdist != 0; fdist-- ) {
            const int cur_index = flat_index( cur.xy() );
            const path_data_layer &layer = pf.get_layer( cur.z );
            const tripoint &par = layer.parent[cur_index];
            if( cur == f ) {
                break;
            }

            ret.push_back( cur );
            // Jumps are acceptable on 1 z-level changes
            // This is because stairs teleport the player too
            if( rl_dist( cur, par ) > 1 && std::abs( cur.z - par.z ) != 1 ) {
                debugmsg( "Jump in our route!  %d:%d:%d->%d:%d:%d",
                          cur.x, cur.y, cur.z, par.x, par.y, par.z );
                return ret;
            }

            cur = par;
        }

        std::reverse( ret.begin(), ret.end() );
    } else {
        // The only way we can get to this point is if we're stuck in a room
        // with no way out, but we want to get a target on the other side
        // of unbreakable windows or bars.
        //
        // In that case we'll just end up A*'ing the whole room every turn
        // for no reason, so we're gonna mark the whole area as hopeless
        // for anyone with the same or less bash rating than the highest
        // rating we saw that was too hard for us.
        pathfinding_cache &pf_cache = get_pathfinding_cache( f.z );
        int zone_number = pf_cache.stuck_threshold_by_zone.size();
        ff::flood_fill_visit_10_connected( tripoint_bub_ms( f ),
        [&pf_cache, bash, zone_number]( const tripoint_bub_ms & loc, int direction ) {
            // Limit to one Z level for now.
            if( direction != 0 ) {
                return false;
            }
            return pf_cache.move_costs[loc.x()][loc.y()] > 0 ||
                   pf_cache.bash_ranges[loc.x()][loc.y()].first <= bash;
        },
        [&pf_cache, zone_number]( const tripoint_bub_ms & loc ) {
            pf_cache.zones[loc.x()][loc.y()] = zone_number;
        } );
        pf_cache.stuck_threshold_by_zone.push_back( max_bash_larger_than_ours );
    }

    return ret;
}

std::vector<tripoint_bub_ms> map::route( const tripoint_bub_ms &f, const tripoint_bub_ms &t,
        const pathfinding_settings &settings,
        std::function<bool( const tripoint & )> should_avoid ) const
{
    std::vector<tripoint> raw_result = route( f.raw(), t.raw(), settings, std::move( should_avoid ) );
    std::vector<tripoint_bub_ms> result;
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const tripoint & p ) {
        return tripoint_bub_ms( p );
    } );
    return result;
}
