#pragma once
#ifndef MAP_ITERATOR_H
#define MAP_ITERATOR_H

#include <cstddef>

#include "enums.h"
#include "point.h"

class tripoint_range
{
    private:
        /**
         * Generates points in a rectangle.
         */
        class point_generator
        {
                friend class tripoint_range;
            private:
                tripoint p;
                const tripoint_range &range;
            public:
                using value_type = tripoint;
                using difference_type = std::ptrdiff_t;
                using pointer = tripoint *;
                using reference = tripoint &;
                using iterator_category = std::forward_iterator_tag;

                point_generator( const tripoint &_p, const tripoint_range &_range )
                    : p( _p ), range( _range ) {
                }

                // Increment x, then if it goes outside range, "wrap around" and increment y
                // Same for y and z
                inline point_generator &operator++() {
                    p.x++;
                    if( p.x <= range.maxp.x ) {
                        return *this;
                    }

                    p.y++;
                    p.x = range.minp.x;
                    if( p.y <= range.maxp.y ) {
                        return *this;
                    }

                    p.z++;
                    p.y = range.minp.y;
                    return *this;
                }

                inline const tripoint &operator*() const {
                    return p;
                }

                inline bool operator!=( const point_generator &other ) const {
                    // Reverse coordinates order, because it will usually only be compared with endpoint
                    // which will always differ in Z, except for the very last comparison
                    const tripoint &pt = other.p;
                    return p.z != pt.z || p.y != pt.y || p.x != pt.x;
                }

                inline bool operator==( const point_generator &other ) const {
                    return !( *this != other );
                }
        };

        tripoint minp;
        tripoint maxp;
    public:
        using value_type = point_generator::value_type;
        using difference_type = point_generator::difference_type;
        using pointer = point_generator::pointer;
        using reference = point_generator::reference;
        using iterator_category = point_generator::iterator_category;

        tripoint_range( const tripoint &_minp, const tripoint &_maxp ) :
            minp( _minp ), maxp( _maxp ) {
        }

        tripoint_range( tripoint &&_minp, tripoint &&_maxp ) :
            minp( _minp ), maxp( _maxp ) {
        }

        point_generator begin() const {
            return point_generator( minp, *this );
        }

        point_generator end() const {
            // Return the point AFTER the last one
            // That is, point under (in z-levels) the first one, but one z-level below the last one
            return point_generator( tripoint( minp.xy(), maxp.z + 1 ), *this );
        }

        size_t size() const {
            tripoint range( maxp - minp );
            return std::max( ++range.x * ++range.y * ++range.z, 0 );
        }

        bool empty() const {
            return size() == 0;
        }

        bool is_point_inside( const tripoint &point ) const {
            for( const tripoint &current : *this ) {
                if( current == point ) {
                    return true;
                }
            }
            return false;
        }

        const tripoint &min() const {
            return minp;
        }
        const tripoint &max() const {
            return maxp;
        }
};

inline tripoint_range points_in_radius( const tripoint &center, const int radius,
                                        const int radiusz = 0 )
{
    const tripoint offset( radius, radius, radiusz );
    return tripoint_range( center - offset, center + offset );
}

#endif
