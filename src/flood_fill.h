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
template<typename Point, typename UnaryPredicate>
std::vector<Point> point_flood_fill_4_connected( const Point &starting_point,
        std::unordered_set<Point> &visited, UnaryPredicate predicate )
{
    std::vector<Point> filled_points;
    std::queue<Point> to_check;
    to_check.push( starting_point );
    while( !to_check.empty() ) {
        const Point current_point = to_check.front();
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

struct span {
    span() : startX( 0 ), endX( 0 ), y( 0 ), dy( 0 ), z( 0 ),
        dz( 0 ) {}
    span( uint8_t p_startX, uint8_t p_endX, uint8_t p_y, int8_t p_dy, int8_t p_z,
          int8_t p_dz ) : startX( p_startX ), endX( p_endX ), y( p_y ), dy( p_dy ), z( p_z ),
        dz( p_dz ) {}

    uint8_t startX;
    uint8_t endX;
    uint8_t y;
    int8_t dy;
    int8_t z;
    int8_t dz;
};

inline void add_span( std::vector<span> &spans, int startX, int endX,
                      int y, int dy, int z, int dz )
{
    const int clamped_z = clamp<int8_t>( z, -OVERMAP_DEPTH, OVERMAP_HEIGHT );
    spans.emplace_back( std::max( startX, 0 ), std::min( endX, MAPSIZE_X - 1 ),
                        clamp( y, 0, MAPSIZE_Y - 1 ), dy, clamped_z, dz );
}


template<typename UnaryPredicate, typename UnaryVisitor>
void flood_fill_visit_10_connected( const tripoint_bub_ms &starting_point, UnaryPredicate predicate,
                                    UnaryVisitor visitor )
{
    std::array<std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X>, OVERMAP_LAYERS> visited;
    std::array<std::array<std::bitset<MAPSIZE_Y>, MAPSIZE_X>, OVERMAP_LAYERS> visited_vertically;
    std::array<std::vector<span>, OVERMAP_LAYERS> spans_to_process;
    int current_z = starting_point.z();
    add_span( spans_to_process[current_z + OVERMAP_DEPTH], starting_point.x(), starting_point.x(),
              starting_point.y(), 1, starting_point.z(), 0 );
    add_span( spans_to_process[current_z + OVERMAP_DEPTH], starting_point.x(), starting_point.x(),
              starting_point.y() - 1, -1, starting_point.z(), 0 );
    auto check = [&predicate, &visited]( const tripoint_bub_ms loc, int dir ) {
        return loc.z() >= -OVERMAP_DEPTH && loc.z() <= OVERMAP_HEIGHT &&
               loc.y() >= 0 && loc.y() < MAPSIZE_Y &&
               loc.x() >= 0 && loc.x() < MAPSIZE_X &&
               !visited[loc.z() + OVERMAP_DEPTH][loc.y()][loc.x()] && predicate( loc, dir );
    };
    auto check_vertical = [&predicate, &visited_vertically]( const tripoint_bub_ms loc,
    int dir ) {
        return loc.z() >= -OVERMAP_DEPTH && loc.z() <= OVERMAP_HEIGHT &&
               loc.y() >= 0 && loc.y() < MAPSIZE_Y &&
               loc.x() >= 0 && loc.x() < MAPSIZE_X &&
               !visited_vertically[loc.z() + OVERMAP_DEPTH][loc.y()][loc.x()] && predicate( loc, dir );
    };
    while( true ) {
        struct span current_span;
        if( !spans_to_process[current_z + OVERMAP_DEPTH].empty() ) {
            current_span = spans_to_process[current_z + OVERMAP_DEPTH].back();
            spans_to_process[current_z + OVERMAP_DEPTH].pop_back();
        } else {
            for( current_z = OVERMAP_HEIGHT; current_z >= -OVERMAP_DEPTH; current_z-- ) {
                if( !spans_to_process[current_z + OVERMAP_DEPTH].empty() ) {
                    current_span = spans_to_process[current_z + OVERMAP_DEPTH].back();
                    spans_to_process[current_z + OVERMAP_DEPTH].pop_back();
                    break;
                }
            }
            if( current_z < -OVERMAP_DEPTH ) {
                break;
            }
        }
        tripoint_bub_ms current_point{ static_cast<int>( current_span.startX ), static_cast <int>( current_span.y ), static_cast<int>( current_span.z ) };
        // Special handling for spans with a vertical offset.
        if( current_span.dz != 0 ) {
            bool span_found = false;
            struct span new_span {
                current_span.startX, current_span.endX, current_span.y, current_span.dy, current_span.z, 0
            };
            // Iterate over tiles in the span, adding to new span as long as check passes.
            while( current_point.x() <= current_span.endX ) {
                if( check_vertical( current_point, current_span.dz ) ) {
                    span_found = true;
                    new_span.endX = current_point.x();
                    visited_vertically[current_point.z() + OVERMAP_DEPTH][current_point.y()].set( current_point.x() );
                } else {
                    if( span_found ) {
                        add_span( spans_to_process[current_z + OVERMAP_DEPTH], new_span.startX, new_span.endX,
                                  new_span.y, 1, current_point.z(), 0 );
                        add_span( spans_to_process[current_z + OVERMAP_DEPTH], new_span.startX, new_span.endX,
                                  new_span.y - 1, -1, current_point.z(), 0 );
                    }
                    new_span.startX = current_point.x() + 1;
                    span_found = false;
                }
                current_point.x()++;
            }
            if( span_found ) {
                add_span( spans_to_process[current_z + OVERMAP_DEPTH], new_span.startX, new_span.endX,
                          new_span.y, 1, current_point.z(), 0 );
                add_span( spans_to_process[current_z + OVERMAP_DEPTH], new_span.startX, new_span.endX,
                          new_span.y - 1, -1, current_point.z(), 0 );
            }
            continue;
        }

        // Scan to the left of the leftmost point in the current span.
        if( check( current_point, 0 ) ) {
            // This decrements before visiting because the next loop will visit the current value of startX
            current_point.x()--;
            while( check( current_point, 0 ) ) {
                visitor( current_point );
                visited[current_point.z() + OVERMAP_DEPTH][current_point.y()].set( current_point.x() );
                current_point.x()--;
            }
            current_point.x()++;
            // If we found visitable tiles to the left of the span, emit a new span going in the other y direction to go around corners.
            if( current_point.x() < current_span.startX ) {
                add_span( spans_to_process[current_z + OVERMAP_DEPTH], current_point.x() - 1,
                          current_span.startX - 1, current_span.y - current_span.dy, -current_span.dy,
                          current_point.z(), 0 );
            }
        }
        int furthestX = current_point.x();
        current_point.x() = current_span.startX;
        // Scan the span itself, running off the edge to the right if possible.
        while( current_point.x() <= current_span.endX ) {
            while( check( current_point, 0 ) ) {
                visitor( current_point );
                visited[current_point.z() + OVERMAP_DEPTH][current_point.y()].set( current_point.x() );
                current_point.x()++;
            }
            // If we have made any progress in the above loops, emit a span going in our initial y direction as well as in both vertical directions.
            if( current_point.x() > furthestX ) {
                add_span( spans_to_process[current_z + OVERMAP_DEPTH], furthestX - 1, current_point.x(),
                          current_span.y + current_span.dy, current_span.dy,
                          current_span.z, 0 );
                if( current_z + 1 < OVERMAP_HEIGHT ) {
                    add_span( spans_to_process[current_z + OVERMAP_DEPTH + 1], furthestX, current_point.x() - 1,
                              current_span.y, 0, current_span.z + 1, 1 );
                }
                if( current_z - 1 >= -OVERMAP_DEPTH ) {
                    add_span( spans_to_process[current_z + OVERMAP_DEPTH - 1], furthestX, current_point.x() - 1,
                              current_span.y, 0, current_span.z - 1, -1 );
                }
            }
            // If we found visitable tiles to the right of the span, emit a new span going in the other y direction to go around corners.
            if( current_point.x() - 1 > current_span.endX ) {
                add_span( spans_to_process[current_z + OVERMAP_DEPTH], current_span.endX + 1, current_point.x(),
                          current_span.y - current_span.dy, -current_span.dy, current_span.z, 0 );
            }
            // This is pointing to a tile that faied the predicate, so advance to the next tile.
            current_point.x()++;
            // Scan past any unvisitable tiles up to the end of the current span.
            while( current_point.x() < current_span.endX && !predicate( current_point, 0 ) ) {
                current_point.x()++;
            }
            // We either found a new visitable tile or ran off the end of the span, record our new scan start point regardless.
            furthestX = current_point.x();
        }
    }
}
} // namespace ff

#endif // CATA_SRC_FLOOD_FILL_H
