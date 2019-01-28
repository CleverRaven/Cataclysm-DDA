#pragma once
#ifndef MAP_ITERATOR_H
#define MAP_ITERATOR_H

#include <cstddef>

#include "enums.h"

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
                typedef tripoint                    value_type;
                typedef std::ptrdiff_t              difference_type;
                typedef tripoint                   *pointer;
                typedef tripoint                   &reference;
                typedef std::forward_iterator_tag   iterator_category;

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
        typedef point_generator::value_type         value_type;
        typedef point_generator::difference_type    difference_type;
        typedef point_generator::pointer            pointer;
        typedef point_generator::reference          reference;
        typedef point_generator::iterator_category  iterator_category;

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
            return point_generator( tripoint( minp.x, minp.y, maxp.z + 1 ), *this );
        }

        size_t size() const {
            tripoint range( maxp - minp );
            return std::max( ++range.x * ++range.y * ++range.z, 0 );
        }

        bool empty() const {
            return size() == 0;
        }

        const tripoint &min() const {
            return minp;
        }
        const tripoint &max() const {
            return maxp;
        }
};

#endif
