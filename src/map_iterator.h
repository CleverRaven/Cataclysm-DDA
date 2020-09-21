#pragma once
#ifndef CATA_SRC_MAP_ITERATOR_H
#define CATA_SRC_MAP_ITERATOR_H

#include <cstddef>

#include "enums.h"
#include "point.h"
#include "point_traits.h"

template<typename Tripoint>
class tripoint_range
{
        static_assert( Tripoint::dimension == 3, "Requires tripoint type" );
    private:
        using traits = point_traits<Tripoint>;
        /**
         * Generates points in a rectangle.
         */
        class point_generator
        {
                friend class tripoint_range;
            private:
                Tripoint p;
                const tripoint_range &range;

            public:
                using value_type = Tripoint;
                using difference_type = std::ptrdiff_t;
                using pointer = value_type *;
                using reference = value_type &;
                using iterator_category = std::forward_iterator_tag;

                point_generator( const Tripoint &_p, const tripoint_range &_range )
                    : p( _p ), range( _range ) {
                }

                // Increment x, then if it goes outside range, "wrap around" and increment y
                // Same for y and z
                inline point_generator &operator++() {
                    traits::x( p )++;
                    if( traits::x( p ) <= traits::x( range.maxp ) ) {
                        return *this;
                    }

                    traits::y( p )++;
                    traits::x( p ) = traits::x( range.minp );
                    if( traits::y( p ) <= traits::y( range.maxp ) ) {
                        return *this;
                    }

                    traits::z( p )++;
                    traits::y( p ) = traits::y( range.minp );
                    return *this;
                }

                inline const Tripoint &operator*() const {
                    return p;
                }

                inline bool operator!=( const point_generator &other ) const {
                    // Reverse coordinates order, because it will usually only be compared with endpoint
                    // which will always differ in Z, except for the very last comparison
                    // TODO: In C++17 this range should use a sentinel to
                    // optimise the comparison.
                    const Tripoint &pt = other.p;
                    return traits::z( p ) != traits::z( pt ) || p.xy() != pt.xy();
                }

                inline bool operator==( const point_generator &other ) const {
                    return !( *this != other );
                }
        };

        Tripoint minp;
        Tripoint maxp;
    public:
        using value_type = typename point_generator::value_type;
        using difference_type = typename point_generator::difference_type;
        using pointer = typename point_generator::pointer;
        using reference = typename point_generator::reference;
        using iterator_category = typename point_generator::iterator_category;

        tripoint_range( const Tripoint &_minp, const Tripoint &_maxp ) :
            minp( _minp ), maxp( _maxp ) {
        }

        point_generator begin() const {
            return point_generator( minp, *this );
        }

        point_generator end() const {
            // Return the point AFTER the last one
            // That is, point under (in z-levels) the first one, but one z-level below the last one
            return point_generator( Tripoint( minp.xy(), traits::z( maxp ) + 1 ), *this );
        }

        size_t size() const {
            Tripoint range( maxp - minp );
            return std::max( ++traits::x( range ) * ++traits::y( range ) * ++traits::z( range ), 0 );
        }

        bool empty() const {
            return size() == 0;
        }

        bool is_point_inside( const Tripoint &point ) const {
            for( const Tripoint &current : *this ) {
                if( current == point ) {
                    return true;
                }
            }
            return false;
        }

        const Tripoint &min() const {
            return minp;
        }
        const Tripoint &max() const {
            return maxp;
        }
};

template<typename Tripoint>
inline tripoint_range<Tripoint> points_in_radius( const Tripoint &center, const int radius,
        const int radiusz = 0 )
{
    static_assert( Tripoint::dimension == 3, "Requires tripoint type" );
    const tripoint offset( radius, radius, radiusz );
    return tripoint_range<Tripoint>( center - offset, center + offset );
}

#endif // CATA_SRC_MAP_ITERATOR_H
