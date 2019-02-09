#pragma once
#ifndef SIMPLE_PATHFINDINDING_H
#define SIMPLE_PATHFINDINDING_H

#include <limits>
#include <queue>
#include <vector>

#include "enums.h"

namespace pf
{

static const int rejected = std::numeric_limits<int>::min();

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

    point pos() const {
        return point( x, y );
    }
};

struct path {
    std::vector<node> nodes;
};

/**
 * @param source Starting point of path
 * @param dest End point of path
 * @param max_x Max permissible x coordinate for a point on the path
 * @param max_y Max permissible y coordinate for a point on the path
 * @param estimator BinaryPredicate( node &previous, node *current ) returns
 * integer estimation (smaller - better) for the current node or a negative value
 * if the node is unsuitable.
 */
template<class BinaryPredicate>
path find_path( const point &source,
                const point &dest,
                const int max_x,
                const int max_y,
                BinaryPredicate estimator )
{
    static const int dx[4] = {  0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1,  0 };

    const auto inbounds = [ max_x, max_y ]( const int x, const int y ) {
        return x >= 0 && x < max_x && y >= 0 && y <= max_y;
    };

    const auto map_index = [ max_x ]( const int x, const int y ) {
        return y * max_x + x;
    };

    path res;

    if( source == dest ) {
        return res;
    }

    const int x1 = source.x;
    const int y1 = source.y;
    const int x2 = dest.x;
    const int y2 = dest.y;

    if( !inbounds( x1, y1 ) || !inbounds( x2, y2 ) ) {
        return res;
    }

    const node first_node( x1, y1, 5, 1000 );

    if( estimator( first_node, nullptr ) == rejected ) {
        return res;
    }

    const size_t map_size = max_x * max_y;

    std::vector<bool> closed( map_size, false );
    std::vector<int> open( map_size, 0 );
    std::vector<short> dirs( map_size, 0 );
    std::priority_queue<node, std::vector<node>> nodes[2];

    int i = 0;
    nodes[i].push( first_node );
    open[map_index( x1, y1 )] = std::numeric_limits<int>::max();

    // use A* to find the shortest path from (x1,y1) to (x2,y2)
    while( !nodes[i].empty() ) {
        const node mn( nodes[i].top() ); // get the best-looking node

        nodes[i].pop();
        // mark it visited
        closed[map_index( mn.x, mn.y )] = true;

        // if we've reached the end, draw the path and return
        if( mn.x == x2 && mn.y == y2 ) {
            int x = mn.x;
            int y = mn.y;

            res.nodes.reserve( nodes[i].size() );

            while( x != x1 || y != y1 ) {
                const int n = map_index( x, y );
                const int d = dirs[n];
                res.nodes.emplace_back( x, y, d );
                x += dx[d];
                y += dy[d];
            }

            res.nodes.emplace_back( x, y, -1 );

            return res;
        }

        for( int d = 0; d < 4; d++ ) {
            const int x = mn.x + dx[d];
            const int y = mn.y + dy[d];
            const int n = map_index( x, y );
            // don't allow:
            // * out of bounds
            // * already traversed tiles
            if( x < 1 || x + 1 >= max_x || y < 1 || y + 1 >= max_y || closed[n] ) {
                continue;
            }

            node cn( x, y, d );
            cn.priority = estimator( cn, &mn );

            if( cn.priority == rejected ) {
                continue;
            }
            // record direction to shortest path
            if( open[n] == 0 || open[n] > cn.priority ) {
                dirs[n] = ( d + 2 ) % 4;

                if( open[n] != 0 ) {
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
                open[n] = cn.priority;
                nodes[i].push( cn );
            }
        }
    }

    return res;
}

inline path straight_path( const point &source,
                           int dir,
                           size_t len )
{
    static const int dx[4] = {  0, 1, 0, -1 };
    static const int dy[4] = { -1, 0, 1,  0 };

    path res;

    if( len == 0 ) {
        return res;
    }

    int x = source.x;
    int y = source.y;

    res.nodes.reserve( len );

    for( size_t i = 0; i + 1 < len; ++i ) {
        res.nodes.emplace_back( x, y, dir );

        x += dx[dir];
        y += dy[dir];
    }

    res.nodes.emplace_back( x, y, -1 );

    return res;
}

}

#endif
