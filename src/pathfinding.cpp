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

// Turns two indexed to a 2D array into an index to equivalent 1D array
static constexpr int flat_index( const point &p )
{
    return ( p.x * MAPSIZE_Y ) + p.y;
}

// Flattened 2D array representing a single z-level worth of pathfinding data
struct path_data_layer {
    // Closed/open is accessed way more often than all other values here
    std::bitset< MAPSIZE_X *MAPSIZE_Y > closed;
    std::bitset< MAPSIZE_X *MAPSIZE_Y > open;
    std::array< int, MAPSIZE_X *MAPSIZE_Y > score;
    std::array< int, MAPSIZE_X *MAPSIZE_Y > gscore;
    std::array< tripoint, MAPSIZE_X *MAPSIZE_Y > parent;

    void reset() {
        closed.reset();
        open.reset();
    }
};

struct pathfinder {
    using queue_type =
        std::priority_queue< std::pair<int, tripoint>, std::vector< std::pair<int, tripoint> >, pair_greater_cmp_first >;
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

    tripoint get_next() {
        const auto pt = open.top();
        open.pop();
        return pt.second;
    }

    void add_point( const int gscore, const int score, const tripoint &from, const tripoint &to ) {
        path_data_layer &layer = get_layer( to.z );
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

    void close_point( const tripoint &p ) {
        path_data_layer &layer = get_layer( p.z );
        const int index = flat_index( p.xy() );
        layer.closed[index] = true;
    }

    void unclose_point( const tripoint &p ) {
        path_data_layer &layer = get_layer( p.z );
        const int index = flat_index( p.xy() );
        layer.closed[index] = false;
    }
};

static pathfinder pf;

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

std::vector<tripoint> map::straight_route( const tripoint &f, const tripoint &t ) const
{
    std::vector<tripoint> ret;
    if( f == t || !inbounds( f ) ) {
        return ret;
    }
    if( !inbounds( t ) ) {
        tripoint clipped = t;
        clip_to_bounds( clipped );
        return straight_route( f, clipped );
    }
    if( f.z == t.z ) {
        ret = line_to( f, t );
        const pathfinding_cache &pf_cache = get_pathfinding_cache_ref( f.z );
        // Check all points for any special case (including just hard terrain)
        if( std::any_of( ret.begin(), ret.end(), [&pf_cache]( const tripoint & p ) {
        constexpr pf_special non_normal = PF_SLOW | PF_WALL | PF_VEHICLE | PF_TRAP | PF_SHARP;
        return pf_cache.special[p.x][p.y] & non_normal;
        } ) ) {
            ret.clear();
        }
    }
    return ret;
}

std::vector<tripoint> map::route( const tripoint &f, const tripoint &t,
                                  const pathfinding_settings &settings,
                                  const std::unordered_set<tripoint> &pre_closed ) const
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
    if( f.z == t.z ) {
        auto line_path = straight_route( f, t );
        if( !line_path.empty() ) {
            if( std::none_of( line_path.begin(), line_path.end(), [&pre_closed]( const tripoint & p ) {
            return pre_closed.count( p );
            } ) ) {
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

    const int pad = 16;  // Should be much bigger - low value makes pathfinders dumb!
    tripoint min( std::min( f.x, t.x ) - pad, std::min( f.y, t.y ) - pad, std::min( f.z, t.z ) );
    tripoint max( std::max( f.x, t.x ) + pad, std::max( f.y, t.y ) + pad, std::max( f.z, t.z ) );
    clip_to_bounds( min.x, min.y, min.z );
    clip_to_bounds( max.x, max.y, max.z );

    pf.reset( min.z, max.z );
    // Make NPCs not want to path through player
    // But don't make player pathing stop working
    for( const tripoint &p : pre_closed ) {
        if( p.x >= min.x && p.x < max.x && p.y >= min.y && p.y < max.y ) {
            pf.close_point( p );
        }
    }

    // Start and end must not be closed
    pf.unclose_point( f );
    pf.unclose_point( t );
    pf.add_point( 0, 0, f, f );

    bool done = false;

    constexpr pf_special non_normal = PF_SLOW | PF_WALL | PF_VEHICLE | PF_TRAP | PF_SHARP;
    do {
        tripoint cur = pf.get_next();

        const int parent_index = flat_index( cur.xy() );
        path_data_layer &layer = pf.get_layer( cur.z );
        if( layer.closed[parent_index] ) {
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

        layer.closed[parent_index] = true;

        const pathfinding_cache &pf_cache = get_pathfinding_cache_ref( cur.z );
        const pf_special cur_special = pf_cache.special[cur.x][cur.y];

        // 7 3 5
        // 1 . 2
        // 6 4 8
        constexpr std::array<int, 8> x_offset{{ -1,  1,  0,  0,  1, -1, -1, 1 }};
        constexpr std::array<int, 8> y_offset{{  0,  0, -1,  1, -1,  1, -1, 1 }};
        for( size_t i = 0; i < 8; i++ ) {
            const tripoint p( cur.x + x_offset[i], cur.y + y_offset[i], cur.z );
            const int index = flat_index( p.xy() );

            // TODO: Remove this and instead have sentinels at the edges
            if( p.x < min.x || p.x >= max.x || p.y < min.y || p.y >= max.y ) {
                continue;
            }

            if( layer.closed[index] ) {
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
                    layer.closed[index] = true; // Close all rough terrain tiles
                    continue;
                }

                int part = -1;
                const const_maptile &tile = maptile_at_internal( p );
                const ter_t &terrain = tile.get_ter_t();
                const furn_t &furniture = tile.get_furn_t();
                const field &field = tile.get_field();
                const vehicle *veh = veh_at_internal( p, part );

                const int cost = move_cost_internal( furniture, terrain, field, veh, part );
                // Don't calculate bash rating unless we intend to actually use it
                const int rating = ( bash == 0 || cost != 0 ) ? -1 :
                                   bash_rating_internal( bash, furniture, terrain, false, veh, part );

                if( cost == 0 && rating <= 0 && ( !doors || !terrain.open || !furniture.open ) && veh == nullptr &&
                    climb_cost <= 0 ) {
                    layer.closed[index] = true; // Close it so that next time we won't try to calculate costs
                    continue;
                }

                newg += cost;
                if( cost == 0 ) {
                    if( climb_cost > 0 && p_special & PF_CLIMBABLE ) {
                        // Climbing fences
                        newg += climb_cost;
                    } else if( doors && ( terrain.open || furniture.open ) &&
                               ( ( !terrain.has_flag( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE ) &&
                                   !furniture.has_flag( ter_furn_flag::TFLAG_OPENCLOSE_INSIDE ) ) ||
                                 !is_outside( cur ) ) ) {
                        // Only try to open INSIDE doors from the inside
                        // To open and then move onto the tile
                        newg += 4;
                    } else if( veh != nullptr ) {
                        const auto vpobst = vpart_position( const_cast<vehicle &>( *veh ), part ).obstacle_at_part();
                        part = vpobst ? vpobst->part_index() : -1;
                        int dummy = -1;
                        const bool is_outside_veh = veh_at_internal( cur, dummy ) != veh;

                        if( doors && part != -1 && veh->next_part_to_open( part, is_outside_veh ) != -1 ) {
                            // Handle car doors, but don't try to path through curtains
                            newg += 10; // One turn to open, 4 to move there
                        } else if( locks && veh->next_part_to_unlock( part, is_outside_veh ) != -1 ) {
                            newg += 12; // 2 turns to open, 4 to move there
                        } else if( part >= 0 && bash > 0 ) {
                            // Car obstacle that isn't a door
                            // TODO: Account for armor
                            int hp = veh->part( part ).hp();
                            if( hp / 20 > bash ) {
                                // Threshold damage thing means we just can't bash this down
                                layer.closed[index] = true;
                                continue;
                            } else if( hp / 10 > bash ) {
                                // Threshold damage thing means we will fail to deal damage pretty often
                                hp *= 2;
                            }

                            newg += 2 * hp / bash + 8 + 4;
                        } else if( part >= 0 ) {
                            const vehicle_part &vp = veh->part( part );
                            if( !doors || !vp.info().has_flag( VPFLAG_OPENABLE ) ) {
                                // Won't be openable, don't try from other sides
                                layer.closed[index] = true;
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
                            layer.closed[index] = true;
                        }

                        continue;
                    }
                }

                if( trapavoid && ( p_special & PF_TRAP ) ) {
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
                                layer.closed[index] = true;
                                continue;
                            }
                        } else {
                            // Otherwise it's walkable
                            newg += 500;
                        }
                    }
                }

                if( sharpavoid && p_special & PF_SHARP ) {
                    layer.closed[index] = true; // Avoid sharp things
                }

            }

            pf.add_point( newg, newg + 2 * rl_dist( p, t ), cur, p );
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
    }

    return ret;
}

std::vector<tripoint_bub_ms> map::route( const tripoint_bub_ms &f, const tripoint_bub_ms &t,
        const pathfinding_settings &settings,
        const std::unordered_set<tripoint> &pre_closed ) const
{
    std::vector<tripoint> raw_result = route( f.raw(), t.raw(), settings, pre_closed );
    std::vector<tripoint_bub_ms> result;
    std::transform( raw_result.begin(), raw_result.end(), std::back_inserter( result ),
    []( const tripoint & p ) {
        return tripoint_bub_ms( p );
    } );
    return result;
}
