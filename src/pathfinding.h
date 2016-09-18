#ifndef PATHFINDIND_H
#define PATHFINDIND_H

#include "debug.h"
#include "enums.h"

#include <functional>
#include <queue>
#include <vector>

namespace pf
{

struct node {
    int x;
    int y;
    int d;
    int p;

    node( int xp, int yp, int dir, int pri ) {
        x = xp;
        y = yp;
        d = dir;
        p = pri;
    }
    // Operator overload required by priority queue interface.
    bool operator< ( const node &n ) const {
        return this->p > n.p;
    }
};

template<int MAX_X, int MAX_Y>
std::vector<node> find_path( const tripoint &source,
                             const tripoint &dest,
                             const std::function<int( const node &, const node & )> &estimate )
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
    bool closed[MAX_X][MAX_Y] = {{false}};
    int open[MAX_X][MAX_Y] = {{0}};
    short dirs[MAX_X][MAX_Y] = {{0}};
    int i = 0;

    int x1 = source.x;
    int y1 = source.y;
    int x2 = dest.x;
    int y2 = dest.y;

    nodes[i].push( node( x1, y1, 5, 1000 ) );
    open[x1][y1] = 1000;

    // use A* to find the shortest path from (x1,y1) to (x2,y2)
    while( !nodes[i].empty() ) {
        // get the best-looking node
        node mn = nodes[i].top();
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
                res.emplace_back( node( x, y, d, 0 ) );
            }

            return res;
        }

        // otherwise, expand to
        for( int d = 0; d < 4; d++ ) {
            int x = mn.x + dx[d];
            int y = mn.y + dy[d];

            node cn = node( x, y, d, 0 );

            cn.p = estimate( mn, cn );
            // don't allow:
            // * out of bounds
            // * already traversed tiles
            // * rejected tiles
            if( x < 1 || x + 1 >= MAX_X || y < 1 || y + 1 >= MAX_Y || closed[x][y] || cn.p < 0 ) {
                continue;
            }
            // record direction to shortest path
            if( open[x][y] == 0 ) {
                dirs[x][y] = ( d + 2 ) % 4;
                open[x][y] = cn.p;
                nodes[i].push( cn );
            } else if( open[x][y] > cn.p ) {
                dirs[x][y] = ( d + 2 ) % 4;
                open[x][y] = cn.p;

                // wizardry
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
                nodes[i].push( cn );
            } else {
                // a shorter path has already been found
            }
        }
    }

    return res;
}

}

#endif
