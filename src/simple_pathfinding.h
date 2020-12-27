#pragma once
#ifndef CATA_SRC_SIMPLE_PATHFINDING_H
#define CATA_SRC_SIMPLE_PATHFINDING_H

#include <limits>
#include <queue>
#include <vector>

#include "enums.h"
#include "point.h"

namespace pf
{

static const int rejected = std::numeric_limits<int>::min();

struct node {
    point pos;
    int dir;
    int priority;

    node( const point &p, int dir, int priority = 0 ) :
        pos( p ),
        dir( dir ),
        priority( priority ) {}

    // Operator overload required by priority queue interface.
    bool operator< ( const node &n ) const {
        return priority > n.priority;
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
 * @param reporter void ProgressReporter() called on each step of the algorithm.
 */
template<class Offsets, class BinaryPredicate, class ProgressReporter>
path find_path( const point &source,
                const point &dest,
                const int max_x,
                const int max_y,
                Offsets offsets,
                BinaryPredicate estimator,
                ProgressReporter reporter )
{
    const auto inbounds = [ max_x, max_y ]( const point & p ) {
        return p.x >= 0 && p.x < max_x && p.y >= 0 && p.y < max_y;
    };

    const auto map_index = [ max_x ]( const point & p ) {
        return p.y * max_x + p.x;
    };

    path res;

    if( source == dest ) {
        return res;
    }

    if( !inbounds( source ) || !inbounds( dest ) ) {
        return res;
    }

    const node first_node( source, 5, 1000 );

    if( estimator( first_node, nullptr ) == rejected ) {
        return res;
    }

    const size_t map_size = max_x * max_y;

    std::vector<bool> closed( map_size, false );
    std::vector<int> open( map_size, 0 );
    std::vector<short> dirs( map_size, 0 );
    std::priority_queue<node, std::vector<node>> nodes;

    nodes.push( first_node );
    open[map_index( source )] = std::numeric_limits<int>::max();

    // use A* to find the shortest path from (x1,y1) to (x2,y2)
    while( !nodes.empty() ) {
        reporter();

        const node mn( nodes.top() ); // get the best-looking node

        nodes.pop();
        // mark it visited
        closed[map_index( mn.pos )] = true;

        // if we've reached the end, draw the path and return
        if( mn.pos == dest ) {
            point p = mn.pos;

            res.nodes.reserve( nodes.size() );

            while( p != source ) {
                const int n = map_index( p );
                const int dir = dirs[n];
                res.nodes.emplace_back( p, dir );
                p += offsets[dir];
            }

            res.nodes.emplace_back( p, -1 );

            return res;
        }

        for( size_t dir = 0; dir < offsets.size(); dir++ ) {
            const point p = mn.pos + offsets[dir];
            const int n = map_index( p );
            // don't allow:
            // * out of bounds
            // * already traversed tiles
            if( !inbounds( p ) || closed[n] ) {
                continue;
            }

            node cn( p, dir );
            cn.priority = estimator( cn, &mn );

            if( cn.priority == rejected ) {
                continue;
            }
            // record direction to shortest path
            if( open[n] == 0 || open[n] > cn.priority ) {
                // Note: Only works if the offsets are CW/CCW!
                dirs[n] = ( dir + offsets.size() / 2 ) % offsets.size();
                open[n] = cn.priority;
                nodes.push( cn );
            }
        }
    }

    return res;
}

template<class BinaryPredicate>
path find_path_4dir( const point &source,
                     const point &dest,
                     const int max_x,
                     const int max_y,
                     BinaryPredicate estimator )
{
    return find_path( source, dest, max_x, max_y, four_adjacent_offsets, estimator, []() {} );
}

template<class BinaryPredicate, class ProgressReporter>
path find_path_4dir( const point &source,
                     const point &dest,
                     const int max_x,
                     const int max_y,
                     BinaryPredicate estimator,
                     ProgressReporter reporter )
{
    return find_path( source, dest, max_x, max_y, four_adjacent_offsets, estimator, reporter );
}

template<class BinaryPredicate>
path find_path_8dir( const point &source,
                     const point &dest,
                     const int max_x,
                     const int max_y,
                     BinaryPredicate estimator )
{
    return find_path( source, dest, max_x, max_y, eight_adjacent_offsets, estimator, []() {} );
}

inline path straight_path( const point &source,
                           int dir,
                           size_t len )
{
    path res;

    if( len == 0 ) {
        return res;
    }

    point p = source;

    res.nodes.reserve( len );

    for( size_t i = 0; i + 1 < len; ++i ) {
        res.nodes.emplace_back( p, dir );

        p += four_adjacent_offsets[dir];
    }

    res.nodes.emplace_back( p, -1 );

    return res;
}

} // namespace pf

#endif // CATA_SRC_SIMPLE_PATHFINDING_H
