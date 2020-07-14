#pragma once
#ifndef CATA_SRC_FLOOD_FILL_H
#define CATA_SRC_FLOOD_FILL_H

#include <queue>
#include <vector>
#include <unordered_set>

#include "enums.h"
#include "point.h"

namespace ff
{
/**
* Given a starting point, flood fill out to the 4-connected points, applying the provided predicate
* to determine if a given point should be added to the collection of flood-filled points, and then
* return that collection.
* @param starting_point starting point of the flood fill. No assumptions made about if it will satisfy
* the predicate.
* @param visited externally provided set of points that have already been designated as visited which
* will be updated by this call.
* @param predicate UnaryPredicate that will be provided with a point for evaluation as to whether or
* not the point should be filled.
*/
template<typename UnaryPredicate>
std::vector<point> point_flood_fill_4_connected( const point &starting_point,
        std::unordered_set<point> &visited, UnaryPredicate predicate )
{
    std::vector<point> filled_points;
    std::queue<point> to_check;
    to_check.push( starting_point );
    while( !to_check.empty() ) {
        const point current_point = to_check.front();
        to_check.pop();

        if( visited.find( current_point ) != visited.end() ) {
            continue;
        }

        visited.emplace( current_point );

        if( predicate( current_point ) ) {
            filled_points.emplace_back( current_point );
            to_check.push( current_point + point_south );
            to_check.push( current_point + point_north );
            to_check.push( current_point + point_east );
            to_check.push( current_point + point_west );
        }
    }

    return filled_points;
}
} // namespace ff

#endif // CATA_SRC_FLOOD_FILL_H
