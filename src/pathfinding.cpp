#include "pathfinding.h"
#include "game.h"
#include "debug.h"

enum pf_state {
    PF_OPEN   = 0,
    PF_CLOSED = 1,
    PF_AVOID  = 2 // Path to it, but don't make it a parent of anything
};

// A circle of "implicit points". circle_x is the x coordinates, circle_y the y coordinates
// Arranged like:
// 7 4 8
// 3 . 1
// 6 2 5
static const int circle_x[8] = { 1, 0, -1, 0, 1, -1, -1, 1 };
static const int circle_y[8] = { 0, 1, 0, -1, 1, 1, -1, -1 };

struct pair_greater_cmp
{
    bool operator()( const std::pair<int, point> &a, const std::pair<int, point> &b)
    {
        return a.first > b.first;
    }
};

void pathfinder::generate_path( const map &m, const std::set< point > &from, const point &to )
{
    // Allocate (and remember) path_data structure
    path_data &pd = path_map[to];

    // TODO: Remove that error message not to repeat the blunder with rain field debug spam
    if( !m.inbounds( to.x, to.y ) ) {
        debugmsg( "Couldn't generate path to (%d,%d) because it is out of bounds",
                  to.x, to.y );
        return;
    }

    // TODO: Use a better structure than std::priority_queue, such as binomial heap
    std::priority_queue< std::pair<int, point>, std::vector< std::pair<int, point> >, pair_greater_cmp > open;
    std::set<point> closed;
    constexpr size_t block_size = SEEX * MAPSIZE * SEEY * MAPSIZE;
    pf_state state [SEEX * MAPSIZE][SEEY * MAPSIZE];
    int score      [SEEX * MAPSIZE][SEEY * MAPSIZE];

    std::uninitialized_fill_n( &state    [0][0], block_size, PF_OPEN );
    std::uninitialized_fill_n( &score    [0][0], block_size, INT_MAX );
    std::uninitialized_fill_n( &pd.parent[0][0], block_size, point( -1, -1 ) );

    open.push( std::make_pair( 0, to ) );
    score[to.x][to.y] = 0;
    pd.parent[to.x][to.y] = to;

    // Copy our `from` set - we'll be removing points from it.
    std::set< point > unreached = from;

    // Estimate the distance from `to` to the furthest point in `from`
    // Once we pass this distance, we need to start checking if all points
    // in `from` have been reached.
    int min_max_dist = -1;
    std::set< point > to_remove;
    for( const point &p : unreached ) {
        int dist = rl_dist( p, to );
        if( dist > max_dist ) {
            // This point won't be reached.
            // Let's accept "bad" points for now - later this should probably log errors
            to_remove.insert( p );
        } else if( !m.inbounds( p.x, p.y ) ) {
            to_remove.insert( p );
            // This time do print an error - this one is serious
            debugmsg( "Pathfinding attempted from out of bounds point (%d,%d)", p.x, p.y );
        } else if( dist > min_max_dist ) {
            min_max_dist = dist;
        }
    }

    // Remove the unreachable points so they aren't checked for being reached
    for( const point &p : to_remove ) {
        unreached.erase( p );
    }

    to_remove.clear(); // Reuse the set later when removing reached points

    for( const point &p : unreached ) {
        // Set point in `from` to PF_AVOID
        // Results in nice surrounding behavior
        state[p.x][p.y] = PF_AVOID;
    }

    // Mark map edges as closed, so that the algorithm can't go out of bounds
    // TODO: Use fill and make it not need to drop the edges
    for( int y = 0; y < SEEY * MAPSIZE; y++ ) {
        state[0][y] = PF_CLOSED;
        state[SEEX * MAPSIZE - 1][y] = PF_CLOSED;
    }
    for( int x = 0; x < SEEX * MAPSIZE; x++ ) {
        state[x][0] = PF_CLOSED;
        state[x][SEEY * MAPSIZE - 1] = PF_CLOSED;
    }

    // We don't need to check every iteration if all points in `from` have been reached.
    // This check can be delayed by at least `unreached.size()` iterations,
    // because every iteration adds exactly 1 point to closed set.
    size_t next_check = 0;

    do {
        auto pr = open.top();
        open.pop();
        if( pr.first > max_dist ) {
            // Shortest path would be too long, no more paths are possible
            return;
        }

        const point &cur = pr.second;
        if( state[cur.x][cur.y] == PF_CLOSED ) {
            continue;
        }

        state[cur.x][cur.y] = PF_CLOSED;

        // Remove reached points from set of points to reach
        // Check them all in bulk rather than checking if current one is reached
        // This should be faster
        if( pr.first >= min_max_dist ) {
            if( next_check > 0 ) {
                next_check--;
            } else {
                for( const point &p : unreached ) {
                    if( state[p.x][p.y] == PF_CLOSED ) {
                        to_remove.insert( p );
                    }
                }

                for( const point &p : to_remove ) {
                    unreached.erase( p );
                }

                to_remove.clear();
                if( unreached.empty() ) {
                    // All done!
                    return;
                }

                // Minimal number of iterations before next check would be `unreached.size()`
                // but actual number will probably be different.
                // Let's multiply by a magic number with no significance
                next_check = unreached.size() * 3;
            }
        }

        for( int i = 0; i < 8; i++ ) {
            const int x = cur.x + circle_x[i];
            const int y = cur.y + circle_y[i];

            if( state[x][y] == PF_CLOSED ) {
                continue;
            }

            int part = -1;
            const furn_t &furniture =   m.furn_at( x, y );
            const ter_t &terrain =      m.ter_at( x, y );
            const vehicle *veh =        m.veh_at_internal( x, y, part );

            const int cost = m.move_cost_internal( furniture, terrain, veh, part );
            // Don't calculate bash rating unless we intend to actually use it
            const int rating = ( bash_force == 0 || cost != 0 ) ? -1 :
                                 m.bash_rating_internal( bash_force, furniture, terrain, veh, part );

            if( cost == 0 && rating <= 0 && terrain.open.empty() ) {
                state[x][y] = PF_CLOSED; // Close it so that next time we won't try to calc costs
                continue;
            }

            // Penalize diagonals a bit (even if we're doing square distance)
            // pathfinder loves them for some reason
            // Heavily penalize paths going through ally tiles, but don't forbid them totally
            int newg = score[cur.x][cur.y] + cost + 
                        ( (circle_x[i] != 0 && circle_y[i] != 0) ? 1 : 0 ) +
                        ( state[x][y] == PF_AVOID ? 100 : 0 );
            if( cost == 0 ) {
                // Handle all kinds of doors
                // Only try to open INSIDE doors from the inside

                if ( open_doors && !terrain.open.empty() &&
                       ( !terrain.has_flag( "OPENCLOSE_INSIDE" ) || !m.is_outside( cur.x, cur.y ) ) ) {
                    newg += 4; // To open and then move onto the tile
                } else if( veh != nullptr ) {
                    part = veh->obstacle_at_part( part );
                    int dummy = -1;
                    if( open_doors && 
                        ( !veh->part_flag( part, "OPENCLOSE_INSIDE" ) || m.veh_at_internal( cur.x, cur.y, dummy ) == veh  ) ) {
                        // Handle car doors, but don't try to path through curtains
                        newg += 10; // One turn to open, 4 to move there
                    } else {
                        // Car obstacle that isn't a door or we can't open doors
                        newg += veh->parts[part].hp / bash_force + 8 + 4;
                    }
                } else if( rating > 1 ) {
                    // Expected number of turns to bash it down, 1 turn to move there
                    // and 2 turns of penalty not to trash everything just because we can
                    newg += ( 20 / rating ) + 2 + 4; 
                } else if( rating == 1 ) {
                    // Desperate measures, avoid whenever possible
                    newg += 1000;
                } else {
                    newg = 10000; // Unbashable and unopenable from here
                }
            }

            // If not in open, add it
            // If in open, add it only if we can do so with better score
            if( newg < score[x][y] ) {
                score    [x][y] = newg;
                pd.parent[x][y] = cur;
                open.push( std::make_pair( score[x][y], point(x, y) ) );
            }
        }
    } while( !open.empty() );
}

void pathfinder::request_path( const point &from, const point &to )
{
    seekers[to].insert( from );
}

void pathfinder::generate_paths( const map &m )
{
    for( const auto &p : seekers ) {
        // TODO: Use A* instead of the inverse Dijkstra when number of points is low
        generate_path( m, p.second, p.first );
    }
}

std::vector< point > pathfinder::get_path( const map &m, const point &from, const point &to ) const
{
    std::vector< point > ret;
    if( !m.inbounds( to.x, to.y ) ) {
        debugmsg( "Tried to get path to out of bounds point (%d,%d)", to.x, to.y );
        return ret;
    }

    const auto iter = path_map.find( to );
    if( iter == path_map.end() ) {
        // A big problem - worth an error message
        //debugmsg( "Tried to get unrequested path to (%d,%d)", to.x, to.y );
add_msg( "Tried to get unrequested path to (%d,%d)", to.x, to.y );
        return ret;
    }

    auto &parent = iter->second.parent;
    point cur = parent[from.x][from.y];
    if( cur.x == -1 && cur.y == -1 ) {
        // No path found
add_msg("empty path");
        return ret;
    }

    size_t res_len = rl_dist( from, to ) * 3; // Rough approx. of path length
    ret.reserve( res_len );
    size_t max_size = SEEX * MAPSIZE * SEEY * MAPSIZE;
    while( cur.x != to.x || cur.y != to.y ) {
        if( max_size == 0 ) {
            // Something is broken
            debugmsg( "Pathfinder found a path with cycles" );
            ret.clear();
            return ret;
        }

        max_size--;
        if( cur.x == -1 && cur.y == -1 ) {
            // Something is broken
            debugmsg( "Pathfinder found a bugged path" );
            ret.clear();
            return ret;
        }

        ret.push_back( cur );
        cur = parent[cur.x][cur.y];
    }

    return ret;
}

void path_manager::load_pathfinder( JsonObject &jo )
{
    std::string id = jo.get_string( "id" );

    pathfinder &pf = finders[id];

    pf.max_dist = jo.get_int    ( "max_dist", 9999 );
    pf.bash_force = jo.get_int  ( "bash_force", 0 );
    pf.open_doors = jo.get_bool ( "open_doors", false );
}
