#pragma once
#ifndef CATA_SRC_SIMPLE_PATHFINDING_H
#define CATA_SRC_SIMPLE_PATHFINDING_H

#include <functional>
#include <vector>

#include "enums.h"
#include "omdata.h"
#include "optional.h"
#include "point.h"

namespace pf
{

/*
 * A node in a path, containing position and direction.
 */
template<typename Point>
struct directed_node {
    Point pos;
    om_direction::type dir;

    explicit directed_node( Point pos,
                            om_direction::type dir = om_direction::type::invalid ) : pos( pos ), dir( dir ) {}
};

/*
 * Data structure representing a path from a source to a destination.
 * The nodes are given in reverse order (from destination to source) in order to allow
 * efficient consumption through pop_back().
 */
template<typename Point>
struct directed_path {
    std::vector<directed_node<Point>> nodes;
};

// Data structure returned by a node scoring function.
struct node_score {
    // cost of traversing the "current" node
    // value < 0 means it can not be traversed
    int node_cost;
    // estimated cost to reach the destination from the "current" node
    // if node_cost is negative, this is ignored
    int estimated_dest_cost;

    node_score( int node_cost, int estimated_dest_cost ) : node_cost( node_cost ),
        estimated_dest_cost( estimated_dest_cost ) {}

    static const node_score rejected;
};

// A node scoring function that provides a node to score and optionally provides the
// previous node in the path as context.
template<typename Point>
using two_node_scoring_fn =
    std::function<node_score( directed_node<Point>, cata::optional<directed_node<Point>> )>;

// non-templated implementation
directed_path<point> greedy_path( const point &source, const point &dest, const point &max,
                                  two_node_scoring_fn<point> scorer );

/**
 * Uses Greedy Best-First-Search to find a short path from source to destination. Supports
 * only 2D point types.
 *
 * @param source Starting point of path
 * @param dest End point of path
 * @param max Max permissible coordinates for a point on the path
 * @param scorer function of (node &current, node *previous) that returns node_score.
 */
template<typename Point, typename = std::enable_if_t<Point::dimension == 2>>
directed_path<Point> greedy_path( const Point &source, const Point &dest, const Point &max,
                                  two_node_scoring_fn<Point> scorer )
{
    directed_path<Point> res;
    const two_node_scoring_fn<point> point_scorer
    = [scorer]( directed_node<point> current, cata::optional<directed_node<point>> prev ) {
        cata::optional<directed_node<Point>> prev_node;
        if( prev ) {
            prev_node = directed_node<Point>( Point( prev->pos ), prev->dir );
        }
        return scorer( directed_node<Point>( Point( current.pos ), current.dir ), prev_node );
    };
    directed_path<point> path = greedy_path( source.raw(), dest.raw(), max.raw(), point_scorer );
    res.nodes.reserve( path.nodes.size() );
    for( const auto &node : path.nodes ) {
        res.nodes.emplace_back( Point( node.pos ), node.dir );
    }
    return res;
}

} // namespace pf

#endif // CATA_SRC_SIMPLE_PATHFINDING_H
