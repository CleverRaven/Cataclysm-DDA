#ifndef PATHFINDIND_H
#define PATHFINDIND_H

#include "debug.h"
#include "enums.h"

#include <queue>
#include <vector>

namespace pf
{

struct node {
    int x;
    int y;
    int dir;
    int priority;

    node( int x, int y, int dir, int priority = 0 ) :
        x( x ),
        y( y ),
        dir( dir ),
        priority( priority ) {}

    // Operator overload required by priority queue interface.
    bool operator< ( const node &n ) const {
        return priority > n.priority;
    }
};

/// @param estimate BinaryPredicate( node &previous, node &current ) returns
/// integer estimation (smaller - better) for the current node or a negative value
/// if the node is unsuitable.
template<int MAX_X, int MAX_Y, class BinaryPredicate>
std::vector<node> find_path( const tripoint &source,
                             const tripoint &dest,
                             BinaryPredicate estimator )
{
    static const int dx[4] = { 1, 0, -1, 0 };
    static const int dy[4] = { 0, 1, 0, -1 };

    std::vector<node> res;

    if( source == dest ) {
        return res;
    }

    if( source.z != dest.z ) {
        debugmsg( "Pathfinding through z-levels is not supported yet." );
        return res;
    }

    std::priority_queue<node, std::deque<node> > nodes[2];
    bool closed[MAX_X][MAX_Y] = {{ false }};
    int open[MAX_X][MAX_Y] = {{ 0 }};
    short dirs[MAX_X][MAX_Y] = {{ 0 }};
    int i = 0;

    const int x1 = source.x;
    const int y1 = source.y;
    const int x2 = dest.x;
    const int y2 = dest.y;

    nodes[i].emplace( x1, y1, 5, 1000 );
    open[x1][y1] = 1000;

    // use A* to find the shortest path from (x1,y1) to (x2,y2)
    while( !nodes[i].empty() ) {
        const node mn( nodes[i].top() ); // get the best-looking node

        nodes[i].pop();
        // make sure it's in bounds
        if( mn.x < 0 || mn.x >= MAX_X || mn.y < 0 || mn.y >= MAX_Y ) {
            continue;
        }
        // mark it visited
        closed[mn.x][mn.y] = true;

        // if we've reached the end, draw the path and return
        if( mn.x == x2 && mn.y == y2 ) {
            int x = mn.x;
            int y = mn.y;

            res.reserve( nodes[i].size() );

            while( x != x1 || y != y1 ) {
                int d = dirs[x][y];
                x += dx[d];
                y += dy[d];
                res.emplace_back( x, y, d, 0 );
            }

            return res;
        }

        for( int d = 0; d < 4; d++ ) {
            const int x = mn.x + dx[d];
            const int y = mn.y + dy[d];
            // don't allow:
            // * out of bounds
            // * already traversed tiles
            if( x < 1 || x + 1 >= MAX_X || y < 1 || y + 1 >= MAX_Y || closed[x][y] ) {
                continue;
            }

            node cn( x, y, d );
            cn.priority = estimator( mn, cn );

            if( cn.priority < 0 ) {
                continue; // rejected by the estimator
            }
            // record direction to shortest path
            if( open[x][y] == 0 || open[x][y] > cn.priority ) {
                dirs[x][y] = ( d + 2 ) % 4;

                if( open[x][y] != 0 ) {
                    while( nodes[i].top().x != x || nodes[i].top().y != y ) {
                        nodes[1 - i].push( nodes[i].top() );
                        nodes[i].pop();
                    }
                    nodes[i].pop();

                    if( nodes[i].size() > nodes[1 - i].size() ) {
                        i = 1 - i;
                    }
                    while( !nodes[i].empty() ) {
                        nodes[1 - i].push( nodes[i].top() );
                        nodes[i].pop();
                    }
                    i = 1 - i;
                }
                open[x][y] = cn.priority;
                nodes[i].push( cn );
            }
        }
    }

    return res;
}

}

#endif
