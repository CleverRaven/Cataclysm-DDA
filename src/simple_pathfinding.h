#pragma once
#ifndef CATA_SRC_SIMPLE_PATHFINDING_H
#define CATA_SRC_SIMPLE_PATHFINDING_H

#include <cstddef>
#include <functional>
#include <optional>
#include <type_traits>
#include <vector>

#include "coords_fwd.h"
#include "omdata.h"
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

/*
 * Data structure representing a path from a source to a destination.
 * The points are given in reverse order (from destination to source) in order to allow
 * efficient consumption through pop_back().
 */
template<typename Point>
struct simple_path {
    //points along the path, typically starting with the last point of the path
    std::vector<Point> points;
    //total distance of the path, possibly measured in tiles/meters depending on the pathing operation
    int dist;
    //total cost of the path, possibly measured in seconds depending on the scoring function
    int cost;
};

// Data structure returned by a node scoring function.
struct node_score {
    // cost of traversing the "current" node
    // value < 0 means it can not be traversed
    int node_cost;
    // estimated cost to reach the destination from the "current" node
    // if node_cost is negative, this is ignored
    int estimated_dest_cost;

    node_score( int node_cost, int estimated_dest_cost );

    static const node_score rejected;
};

// A node scoring function that provides a node to score and optionally provides the
// previous node in the path as context.
template<typename Point>
using two_node_scoring_fn =
    std::function<node_score( directed_node<Point>, std::optional<directed_node<Point>> )>;

// non-templated implementation
directed_path<point> greedy_path( const point &source, const point &dest, const point &max,
                                  const two_node_scoring_fn<point> &scorer );

/**
 * Uses Greedy Best-First-Search to find a short path from source to destination [2D only].
 * The search area is a rectangle with corners at (0,0) and max.
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
    = [scorer]( directed_node<point> current, std::optional<directed_node<point>> prev ) {
        std::optional<directed_node<Point>> prev_node;
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

struct omt_score {
    // cost of traversing the "current" OMT
    // value < 0 means it can not be traversed
    int node_cost;
    // Set to true if it *may* be possible to go up/down from the "current" node.
    // The relevant scoring function will be invoked to determine if it is
    // actually possible.
    bool allow_z_change;

    explicit omt_score( int node_cost, bool allow_z_change = false );

    static const omt_score rejected;
};

using omt_scoring_fn = std::function<omt_score( tripoint_abs_omt )>;

/**
 * Uses A* to find an approximately-cheapest path from source to destination (in 3D).
 *
 * @param source Starting point of path
 * @param dest End point of path
 * @param radius Maximum search radius
 * @param scorer function that returns the omt_score for the given OMT
 * @param max_cost Maximum path cost (optional)
 */
simple_path<tripoint_abs_omt> find_overmap_path( const tripoint_abs_omt &source,
        const tripoint_abs_omt &dest, int radius, const omt_scoring_fn &scorer,
        const std::function<void( size_t, size_t )> &progress_fn,
        const std::optional<int> &max_cost = std::nullopt, bool allow_diagonal = false );

} // namespace pf

#endif // CATA_SRC_SIMPLE_PATHFINDING_H
