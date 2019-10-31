#include "pathfinding.h"

#include <cstdlib>
#include <algorithm>
#include <queue>
#include <set>
#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "cata_utility.h"
#include "coordinates.h"
#include "debug.h"
#include "map.h"
#include "mapdata.h"
#include "optional.h"
#include "submap.h"
#include "trap.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "line.h"
#include "type_id.h"
#include "point.h"

enum astar_state {
    ASL_NONE,
    ASL_OPEN,
    ASL_CLOSED
};

// Turns two indexed to a 2D array into an index to equivalent 1D array
constexpr int flat_index( const int x, const int y )
{
    return ( x * MAPSIZE_Y ) + y;
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
                const int ind = flat_index( x, y );
                state[ind] = ASL_NONE; // Mark as unvisited
            }
        }
    }
};

struct pathfinder {
    point min;
    point max;
    pathfinder( int _minx, int _miny, int _maxx, int _maxy ) :
        min( _minx, _miny ), max( _maxx, _maxy ) {
    }

    std::priority_queue< std::pair<int, tripoint>, std::vector< std::pair<int, tripoint> >, pair_greater_cmp_first >
    open;
    std::array< std::unique_ptr< path_data_layer >, OVERMAP_LAYERS > path_data;

    path_data_layer &get_layer( const int z ) {
        std::unique_ptr< path_data_layer > &ptr = path_data[z + OVERMAP_DEPTH];
        if( ptr != nullptr ) {
            return *ptr;
        }

        ptr = std::make_unique<path_data_layer>();
        ptr->init( min, max );
        return *ptr;
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
        auto &layer = get_layer( to.z );
        const int index = flat_index( to.x, to.y );
        if( ( layer.state[index] == ASL_OPEN && gscore >= layer.gscore[index] ) ||
            layer.state[index] == ASL_CLOSED ) {
            return;
        }

        layer.state [index] = ASL_OPEN;
        layer.gscore[index] = gscore;
        layer.parent[index] = from;
        layer.score [index] = score;
        open.push( std::make_pair( score, to ) );
    }

    void close_point( const tripoint &p ) {
        auto &layer = get_layer( p.z );
        const int index = flat_index( p.x, p.y );
        layer.state[index] = ASL_CLOSED;
    }

    void unclose_point( const tripoint &p ) {
        auto &layer = get_layer( p.z );
        const int index = flat_index( p.x, p.y );
        layer.state[index] = ASL_NONE;
    }
};

// Modifies `t` to be a tile with `flag` in the overmap tile that `t` was originally on
// return false if it could not find a suitable point
template<ter_bitflags flag>
bool vertical_move_destination( const map &m, tripoint &t )
{
    if( !m.has_zlevels() ) {
        return false;
    }

    constexpr int omtileszx = SEEX * 2;
    constexpr int omtileszy = SEEY * 2;
    real_coords rc( m.getabs( t.xy() ) );
    const point omtile_align_start(
        m.getlocal( rc.begin_om_pos() )
    );

    const auto &pf_cache = m.get_pathfinding_cache_ref( t.z );
    for( int x = omtile_align_start.x; x < omtile_align_start.x + omtileszx; x++ ) {
        for( int y = omtile_align_start.y; y < omtile_align_start.y + omtileszy; y++ ) {
            if( pf_cache.special[x][y] & PF_UPDOWN ) {
                const tripoint p( x, y, t.z );
                if( m.has_flag( flag, p ) ) {
                    t = p;
                    return true;
                }
            }
        }
    }

    return false;
}

template<class Set1, class Set2>
bool is_disjoint( const Set1 &set1, const Set2 &set2 )
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

std::vector<tripoint> map::route( const tripoint &f, const tripoint &t,
                                  const pathfinding_settings &settings,
                                  const std::set<tripoint> &pre_closed ) const
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
        return route( f, clipped, settings, pre_closed );
    }
    // First, check for a simple straight line on flat ground
    // Except when the line contains a pre-closed tile - we need to do regular pathing then
    static const auto non_normal = PF_SLOW | PF_WALL | PF_VEHICLE | PF_TRAP;
    if( f.z == t.z ) {
        const auto line_path = line_to( f, t );
        const auto &pf_cache = get_pathfinding_cache_ref( f.z );
        // Check all points for any special case (including just hard terrain)
        if( std::all_of( line_path.begin(), line_path.end(), [&pf_cache]( const tripoint & p ) {
        return !( pf_cache.special[p.x][p.y] & non_normal );
        } ) ) {
            const std::set<tripoint> sorted_line( line_path.begin(), line_path.end() );

            if( is_disjoint( sorted_line, pre_closed ) ) {
                return line_path;
            }
        }
    }

    // If expected path length is greater than max distance, allow only line path, like above
    if( rl_dist( f, t ) > settings.max_dist ) {
        return ret;
    }

    int max_length = settings.max_length;
    int bash = settings.bash_strength;
    int climb_cost = settings.climb_cost;
    bool doors = settings.allow_open_doors;
    bool trapavoid = settings.avoid_traps;
    bool roughavoid = settings.avoid_rough_terrain;

    const int pad = 16;  // Should be much bigger - low value makes pathfinders dumb!
    int minx = std::min( f.x, t.x ) - pad;
    int miny = std::min( f.y, t.y ) - pad;
    int minz = std::min( f.z, t.z ); // TODO: Make this way bigger
    int maxx = std::max( f.x, t.x ) + pad;
    int maxy = std::max( f.y, t.y ) + pad;
    int maxz = std::max( f.z, t.z ); // Same TODO: as above
    clip_to_bounds( minx, miny, minz );
    clip_to_bounds( maxx, maxy, maxz );

    pathfinder pf( minx, miny, maxx, maxy );
    // Make NPCs not want to path through player
    // But don't make player pathing stop working
    for( const auto &p : pre_closed ) {
        if( p.x >= minx && p.x < maxx && p.y >= miny && p.y < maxy ) {
            pf.close_point( p );
        }
    }

    // Start and end must not be closed
    pf.unclose_point( f );
    pf.unclose_point( t );
    pf.add_point( 0, 0, f, f );

    bool done = false;

    do {
        auto cur = pf.get_next();

        const int parent_index = flat_index( cur.x, cur.y );
        auto &layer = pf.get_layer( cur.z );
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

        const auto &pf_cache = get_pathfinding_cache_ref( cur.z );
        const auto cur_special = pf_cache.special[cur.x][cur.y];

        // 7 3 5
        // 1 . 2
        // 6 4 8
        constexpr std::array<int, 8> x_offset{{ -1,  1,  0,  0,  1, -1, -1, 1 }};
        constexpr std::array<int, 8> y_offset{{  0,  0, -1,  1, -1,  1, -1, 1 }};
        for( size_t i = 0; i < 8; i++ ) {
            const tripoint p( cur.x + x_offset[i], cur.y + y_offset[i], cur.z );
            const int index = flat_index( p.x, p.y );

            // TODO: Remove this and instead have sentinels at the edges
            if( p.x < minx || p.x >= maxx || p.y < miny || p.y >= maxy ) {
                continue;
            }

            if( layer.state[index] == ASL_CLOSED ) {
                continue;
            }

            // Penalize for diagonals or the path will look "unnatural"
            int newg = layer.gscore[parent_index] + ( ( cur.x != p.x && cur.y != p.y ) ? 1 : 0 );

            const auto p_special = pf_cache.special[p.x][p.y];
            // TODO: De-uglify, de-huge-n
            if( !( p_special & non_normal ) ) {
                // Boring flat dirt - the most common case above the ground
                newg += 2;
            } else {
                if( roughavoid ) {
                    layer.state[index] = ASL_CLOSED; // Close all rough terrain tiles
                    continue;
                }

                int part = -1;
                const maptile &tile = maptile_at_internal( p );
                const auto &terrain = tile.get_ter_t();
                const auto &furniture = tile.get_furn_t();
                const vehicle *veh = veh_at_internal( p, part );

                const int cost = move_cost_internal( furniture, terrain, veh, part );
                // Don't calculate bash rating unless we intend to actually use it
                const int rating = ( bash == 0 || cost != 0 ) ? -1 :
                                   bash_rating_internal( bash, furniture, terrain, false, veh, part );

                if( cost == 0 && rating <= 0 && ( !doors || !terrain.open || !furniture.open ) && veh == nullptr &&
                    climb_cost <= 0 ) {
                    layer.state[index] = ASL_CLOSED; // Close it so that next time we won't try to calculate costs
                    continue;
                }

                newg += cost;
                if( cost == 0 ) {
                    if( climb_cost > 0 && p_special & PF_CLIMBABLE ) {
                        // Climbing fences
                        newg += climb_cost;
                    } else if( doors && ( terrain.open || furniture.open ) &&
                               ( !terrain.has_flag( "OPENCLOSE_INSIDE" ) || !furniture.has_flag( "OPENCLOSE_INSIDE" ) ||
                                 !is_outside( cur ) ) ) {
                        // Only try to open INSIDE doors from the inside
                        // To open and then move onto the tile
                        newg += 4;
                    } else if( veh != nullptr ) {
                        const auto vpobst = vpart_position( const_cast<vehicle &>( *veh ), part ).obstacle_at_part();
                        part = vpobst ? vpobst->part_index() : -1;
                        int dummy = -1;
                        if( doors && veh->part_flag( part, VPFLAG_OPENABLE ) &&
                            ( !veh->part_flag( part, "OPENCLOSE_INSIDE" ) ||
                              veh_at_internal( cur, dummy ) == veh ) ) {
                            // Handle car doors, but don't try to path through curtains
                            newg += 10; // One turn to open, 4 to move there
                        } else if( part >= 0 && bash > 0 ) {
                            // Car obstacle that isn't a door
                            // TODO: Account for armor
                            int hp = veh->parts[part].hp();
                            if( hp / 20 > bash ) {
                                // Threshold damage thing means we just can't bash this down
                                layer.state[index] = ASL_CLOSED;
                                continue;
                            } else if( hp / 10 > bash ) {
                                // Threshold damage thing means we will fail to deal damage pretty often
                                hp *= 2;
                            }

                            newg += 2 * hp / bash + 8 + 4;
                        } else if( part >= 0 ) {
                            if( !doors || !veh->part_flag( part, VPFLAG_OPENABLE ) ) {
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
                        if( !doors || !terrain.open || !furniture.open ) {
                            // Or anywhere else for that matter
                            layer.state[index] = ASL_CLOSED;
                        }

                        continue;
                    }
                }

                if( trapavoid && p_special & PF_TRAP ) {
                    const auto &ter_trp = terrain.trap.obj();
                    const auto &trp = ter_trp.is_benign() ? tile.get_trap_t() : ter_trp;
                    if( !trp.is_benign() ) {
                        // For now make them detect all traps
                        if( has_zlevels() && terrain.has_flag( TFLAG_NO_FLOOR ) ) {
                            // Special case - ledge in z-levels
                            // Warning: really expensive, needs a cache
                            if( valid_move( p, tripoint( p.xy(), p.z - 1 ), false, true ) ) {
                                tripoint below( p.xy(), p.z - 1 );
                                if( !has_flag( TFLAG_NO_FLOOR, below ) ) {
                                    // Otherwise this would have been a huge fall
                                    auto &layer = pf.get_layer( p.z - 1 );
                                    // From cur, not p, because we won't be walking on air
                                    pf.add_point( layer.gscore[parent_index] + 10,
                                                  layer.score[parent_index] + 10 + 2 * rl_dist( below, t ),
                                                  cur, below );
                                }

                                // Close p, because we won't be walking on it
                                layer.state[index] = ASL_CLOSED;
                                continue;
                            }
                        } else if( trapavoid ) {
                            // Otherwise it's walkable
                            newg += 500;
                        }
                    }
                }
            }

            // If not visited, add as open
            // If visited, add it only if we can do so with better score
            if( layer.state[index] == ASL_NONE || newg < layer.gscore[index] ) {
                pf.add_point( newg, newg + 2 * rl_dist( p, t ), cur, p );
            }
        }

        if( !has_zlevels() || !( cur_special & PF_UPDOWN ) || !settings.allow_climb_stairs ) {
            // The part below is only for z-level pathing
            continue;
        }

        const maptile &parent_tile = maptile_at_internal( cur );
        const auto &parent_terrain = parent_tile.get_ter_t();
        if( settings.allow_climb_stairs && cur.z > minz && parent_terrain.has_flag( TFLAG_GOES_DOWN ) ) {
            tripoint dest( cur.xy(), cur.z - 1 );
            if( vertical_move_destination<TFLAG_GOES_UP>( *this, dest ) ) {
                auto &layer = pf.get_layer( dest.z );
                pf.add_point( layer.gscore[parent_index] + 2,
                              layer.score[parent_index] + 2 * rl_dist( dest, t ),
                              cur, dest );
            }
        }
        if( settings.allow_climb_stairs && cur.z < maxz && parent_terrain.has_flag( TFLAG_GOES_UP ) ) {
            tripoint dest( cur.xy(), cur.z + 1 );
            if( vertical_move_destination<TFLAG_GOES_DOWN>( *this, dest ) ) {
                auto &layer = pf.get_layer( dest.z );
                pf.add_point( layer.gscore[parent_index] + 2,
                              layer.score[parent_index] + 2 * rl_dist( dest, t ),
                              cur, dest );
            }
        }
        if( cur.z < maxz && parent_terrain.has_flag( TFLAG_RAMP ) &&
            valid_move( cur, tripoint( cur.xy(), cur.z + 1 ), false, true ) ) {
            auto &layer = pf.get_layer( cur.z + 1 );
            for( size_t it = 0; it < 8; it++ ) {
                const tripoint above( cur.x + x_offset[it], cur.y + y_offset[it], cur.z + 1 );
                pf.add_point( layer.gscore[parent_index] + 4,
                              layer.score[parent_index] + 4 + 2 * rl_dist( above, t ),
                              cur, above );
            }
        }
    } while( !done && !pf.empty() );

    if( done ) {
        ret.reserve( rl_dist( f, t ) * 2 );
        tripoint cur = t;
        // Just to limit max distance, in case something weird happens
        for( int fdist = max_length; fdist != 0; fdist-- ) {
            const int cur_index = flat_index( cur.x, cur.y );
            const auto &layer = pf.get_layer( cur.z );
            const tripoint &par = layer.parent[cur_index];
            if( cur == f ) {
                break;
            }

            ret.push_back( cur );
            // Jumps are acceptable on 1 z-level changes
            // This is because stairs teleport the player too
            if( rl_dist( cur, par ) > 1 && abs( cur.z - par.z ) != 1 ) {
                debugmsg( "Jump in our route!  %d:%d:%d->%d:%d:%d",
                          cur.x, cur.y, cur.z, par.x, par.y, par.z );
                return ret;
            }

            cur = par;
        }

        std::reverse( ret.begin(), ret.end() );
    }

    return ret;
}
