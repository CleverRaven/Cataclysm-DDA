#include "coordinates.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include <algorithm>
#include <queue>

#include "messages.h"

enum astar_state {
 ASL_NONE,
 ASL_OPEN,
 ASL_CLOSED
};

// Turns two indexed to a 2D array into an index to equivalent 1D array
constexpr int flat_index( const int x, const int y ) {
    return (x * MAPSIZE * SEEY) + y;
};

struct pair_greater_cmp
{
    bool operator()( const std::pair<int, tripoint> &a, const std::pair<int, tripoint> &b)
    {
        return a.first > b.first;
    }
};

// Flattened 2D array representing a single z-level worth of pathfinding data
struct path_data_layer
{
    // State is accessed way more often than all other values here
    std::array< astar_state, SEEX * MAPSIZE * SEEY * MAPSIZE > state;
    std::array< int, SEEX * MAPSIZE * SEEY * MAPSIZE > score;
    std::array< int, SEEX * MAPSIZE * SEEY * MAPSIZE > gscore;
    std::array< tripoint, SEEX * MAPSIZE * SEEY * MAPSIZE > parent;

    void init( const int minx, const int miny, const int maxx, const int maxy ) {
        for( int x = minx; x <= maxx; x++ ) {
            for( int y = miny; y <= maxy; y++ ) {
                const int ind = flat_index( x, y );
                state[ind] = ASL_NONE; // Mark as unvisited
            }
        }
    };
};

struct pathfinder
{
    int minx;
    int miny;
    int maxx;
    int maxy;
    pathfinder( int _minx, int _miny, int _maxx, int _maxy ) :
        minx( _minx ), miny( _miny ), maxx( _maxx ), maxy( _maxy )
    {}

    std::priority_queue< std::pair<int, tripoint>, std::vector< std::pair<int, tripoint> >, pair_greater_cmp > open;
    std::array< std::unique_ptr< path_data_layer >, OVERMAP_LAYERS > path_data;

    path_data_layer &get_layer( const int z ) {
        auto &ptr = path_data[z + OVERMAP_DEPTH];
        if( ptr != nullptr ) {
            return *ptr;
        }

        ptr = std::unique_ptr<path_data_layer>( new path_data_layer() );
        ptr->init( minx, miny, maxx, maxy );
        return *ptr;
    }

    bool empty() const {
        return open.empty();
    }

    tripoint get_next() {
        auto pt = open.top();
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
};

// Returns a tile with `flag` in the overmap tile that `t` is on
template<ter_bitflags flag>
tripoint vertical_move_destination( const map &m, const tripoint &t )
{
    if( !m.has_zlevels() ) {
        return tripoint_min;
    }

    constexpr int omtileszx = SEEX * 2;
    constexpr int omtileszy = SEEY * 2;
    real_coords rc( m.getabs( t.x, t.y ) );
    point omtile_align_start(
        m.getlocal( rc.begin_om_pos() )
    );

    tripoint from( omtile_align_start.x, omtile_align_start.y, t.z );
    tripoint to( omtile_align_start.x + omtileszx, omtile_align_start.y + omtileszy, t.z );

    // TODO: Avoid up to 576 bounds checks by using methods that don't check bounds
    for( const tripoint &p : m.points_in_rectangle( from, to ) ) {
        if( m.has_flag( flag, p ) ) {
            return p;
        }
    }

    return tripoint_min;
}

std::vector<tripoint> map::route( const tripoint &f, const tripoint &t,
                                  const int bash, const int maxdist ) const
{
    /* TODO: If the origin or destination is out of bound, figure out the closest
     * in-bounds point and go to that, then to the real origin/destination.
     */

    int linet1 = 0, linet2 = 0;
    if( !inbounds( f ) || !inbounds( t ) ) {
        // Note: The creature needs to understand not-moving upwards
        // or else the plans can cause it to do so.
        if( sees( f, t, -1, linet1, linet2 ) ) {
            return line_to( f, t, linet1, linet2 );
        } else {
            std::vector<tripoint> empty;
            return empty;
        }
    }
    // First, check for a simple straight line on flat ground
    // Except when the player is on the line - we need to do regular pathing then
    const tripoint &pl_pos = g->u.pos();
    if( f.z == t.z && clear_path( f, t, -1, 2, 2, linet1, linet2 ) ) {
        const auto line_path = line_to( f, t, linet1, linet2 );
        if( pl_pos.z != f.z ) {
            // Player on different z-level, certainly not on the line
            return line_path;
        }

        if( std::find( line_path.begin(), line_path.end(), pl_pos ) == line_path.end() ) {
            return line_path;
        }
    }

    const int pad = 8;  // Should be much bigger - low value makes pathfinders dumb!
    int minx = std::min( f.x, t.x ) - pad;
    int miny = std::min( f.y, t.y ) - pad;
    int minz = std::min( f.z, t.z ); // TODO: Make this way bigger
    int maxx = std::max( f.x, t.x ) + pad;
    int maxy = std::max( f.y, t.y ) + pad;
    int maxz = std::max( f.z, t.z ); // Same TODO as above
    clip_to_bounds( minx, miny, minz );
    clip_to_bounds( maxx, maxy, maxz );

    pathfinder pf( minx, miny, maxx, maxy );
    pf.add_point( 0, 0, f, f );
    // Make NPCs not want to path through player
    // But don't make player pathing stop working
    if( f != pl_pos && t != pl_pos ) {
        pf.close_point( pl_pos );
    }

    bool done = false;

    do {
        auto cur = pf.get_next();

        const int parent_index = flat_index( cur.x, cur.y );
        auto &layer = pf.get_layer( cur.z );
        auto &cur_state = layer.state[parent_index];
        if( cur_state == ASL_CLOSED ) {
            continue;
        }

        if( layer.gscore[parent_index] > maxdist ) {
            // Shortest path would be too long, return empty vector
            return std::vector<tripoint>();
        }

        if( cur == t ) {
            done = true;
            break;
        }

        cur_state = ASL_CLOSED;
        std::vector<tripoint> neighbors = closest_tripoints_first( 1, cur );

        for( const auto &p : neighbors ) {
            const int index = flat_index( p.x, p.y );

            // TODO: Remove this and instead have sentinels at the edges
            if( p.x < minx || p.x >= maxx || p.y < miny || p.y >= maxy ) {
                continue;
            }

            if( layer.state[index] == ASL_CLOSED ) {
                continue;
            }

            int part = -1;
            const maptile &tile = maptile_at_internal( p );
            const auto &terrain = terlist[tile.get_ter()];
            const auto &furniture = furnlist[tile.get_furn()];
            const vehicle *veh = veh_at_internal( p, part );

            const int cost = move_cost_internal( furniture, terrain, veh, part );
            // Don't calculate bash rating unless we intend to actually use it
            const int rating = ( bash == 0 || cost != 0 ) ? -1 :
                                 bash_rating_internal( bash, furniture, terrain, veh, part );

            if( cost == 0 && rating <= 0 && terrain.open.empty() ) {
                layer.state[index] = ASL_CLOSED; // Close it so that next time we won't try to calc costs
                continue;
            }

            int newg = layer.gscore[parent_index] + cost + ( (cur.x != p.x && cur.y != p.y ) ? 1 : 0);
            if( cost == 0 ) {
                // Handle all kinds of doors
                // Only try to open INSIDE doors from the inside

                if( !terrain.open.empty() &&
                    ( !terrain.has_flag( "OPENCLOSE_INSIDE" ) || !is_outside( cur ) ) ) {
                    newg += 4; // To open and then move onto the tile
                } else if( veh != nullptr ) {
                    part = veh->obstacle_at_part( part );
                    int dummy = -1;
                    if( !veh->part_flag( part, "OPENCLOSE_INSIDE" ) || veh_at_internal( cur, dummy ) == veh ) {
                        // Handle car doors, but don't try to path through curtains
                        newg += 10; // One turn to open, 4 to move there
                    } else {
                        // Car obstacle that isn't a door
                        newg += veh->parts[part].hp / bash + 8 + 4;
                    }
                } else if( rating > 1 ) {
                    // Expected number of turns to bash it down, 1 turn to move there
                    // and 2 turns of penalty not to trash everything just because we can
                    newg += ( 20 / rating ) + 2 + 4;
                } else if( rating == 1 ) {
                    // Desperate measures, avoid whenever possible
                    newg += 500;
                } else {
                    continue; // Unbashable and unopenable from here
                }
            }

            // If not visited, add as open
            // If visited, add it only if we can do so with better score
            if( layer.state[index] == ASL_NONE || newg < layer.gscore[index] ) {
                pf.add_point( newg, newg + 2 * rl_dist( p, t ), cur, p );
            }
        }

        if( !has_zlevels() ) {
            // The part below is only for z-level pathing
            continue;
        }

        const maptile &parent_tile = maptile_at_internal( cur );
        const auto &parent_terrain = terlist[parent_tile.get_ter()];
        if( cur.z > minz && parent_terrain.has_flag( TFLAG_GOES_DOWN ) ) {
            tripoint dest( cur.x, cur.y, cur.z - 1 );
            dest = vertical_move_destination<TFLAG_GOES_UP>( *this, dest );
            if( inbounds( dest ) ) {
                auto &layer = pf.get_layer( dest.z );
                pf.add_point( layer.gscore[parent_index] + 2,
                              layer.score[parent_index] + 2 * rl_dist( dest, t ),
                              cur, dest );
            }
        }
        if( cur.z < maxz && parent_terrain.has_flag( TFLAG_GOES_UP ) ) {
            tripoint dest( cur.x, cur.y, cur.z + 1 );
            dest = vertical_move_destination<TFLAG_GOES_DOWN>( *this, dest );
            if( inbounds( dest ) ) {
                auto &layer = pf.get_layer( dest.z );
                pf.add_point( layer.gscore[parent_index] + 2,
                              layer.score[parent_index] + 2 * rl_dist( dest, t ),
                              cur, dest );
            }
        }
    } while( !done && !pf.empty() );

    std::vector<tripoint> ret;
    ret.reserve( rl_dist( f, t ) * 2 );
    if( done ) {
        tripoint cur = t;
        // Just to limit max distance, in case something weird happens
        for( int fdist = maxdist; fdist != 0; fdist-- ) {
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
                debugmsg( "Jump in our route! %d:%d:%d->%d:%d:%d",
                          cur.x, cur.y, cur.z, par.x, par.y, par.z );
                return ret;
            }

            cur = par;
        }

        std::reverse( ret.begin(), ret.end() );
    }

    return ret;
}
