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
                    // Make sure we start on a valid point
                    if( range.predicate && !( *range.predicate )( p ) && p != range.endp ) {
                        operator++();
                    }
                }

                // Increment x, then if it goes outside range, "wrap around" and increment y
                // Same for y and z
                inline point_generator &operator++() {
                    do {
                        traits::x( p )++;
                        if( traits::x( p ) <= traits::x( range.maxp ) ) {
                            continue;
                        }

                        traits::y( p )++;
                        traits::x( p ) = traits::x( range.minp );
                        if( traits::y( p ) <= traits::y( range.maxp ) ) {
                            continue;
                        }

                        traits::z( p )++;
                        traits::y( p ) = traits::y( range.minp );
                    } while( range.predicate && !( *range.predicate )( p ) && p != range.endp );

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

        Tripoint endp;

        cata::optional<std::function<bool( const Tripoint & )>> predicate;
    public:
        using value_type = typename point_generator::value_type;
        using difference_type = typename point_generator::difference_type;
        using pointer = typename point_generator::pointer;
        using reference = typename point_generator::reference;
        using iterator_category = typename point_generator::iterator_category;
        using iterator = point_generator;
        using const_iterator = point_generator;

        tripoint_range( const Tripoint &_minp, const Tripoint &_maxp,
                        const std::function<bool( const Tripoint & )> &pred ) :
            minp( _minp ), maxp( _maxp ), predicate( pred )  {
            endp = Tripoint( minp.xy(), traits::z( maxp ) + 1 );
        }

        tripoint_range( const Tripoint &_minp, const Tripoint &_maxp ) :
            minp( _minp ), maxp( _maxp ) {
            endp = Tripoint( minp.xy(), traits::z( maxp ) + 1 );
        }

        point_generator begin() const {
            return point_generator( minp, *this );
        }

        point_generator end() const {
            // Return the point AFTER the last one
            // That is, point under (in z-levels) the first one, but one z-level below the last one
            return point_generator( endp, *this );
        }

        size_t size() const {
            if( !predicate ) {
                Tripoint range( traits::x( maxp ) - traits::x( minp ), traits::y( maxp ) - traits::y( minp ),
                                traits::z( maxp ) - traits::z( minp ) );
                return std::max( ++traits::x( range ) * ++traits::y( range ) * ++traits::z( range ), 0 );
            } else {
                size_t count = 0;
                for( point_generator it = begin(); it != end(); ++it ) {
                    ++count;
                }
                return count;
            }
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

template<typename Tripoint>
inline tripoint_range<Tripoint> points_in_radius_circ( const Tripoint &center, const int radius,
        const int radiusz = 0 )
{
    static_assert( Tripoint::dimension == 3, "Requires tripoint type" );
    const tripoint offset( radius, radius, radiusz );
    return tripoint_range<Tripoint>( center - offset,
    center + offset, [center, radius]( const Tripoint & pt ) {
        return trig_dist( center, pt ) < radius + 0.5f;
    } );
}

template<typename Tripoint>
inline tripoint_range<Tripoint> points_on_radius_circ( const Tripoint &center, const int radius,
        const int radiusz = 0 )
{
    static_assert( Tripoint::dimension == 3, "Requires tripoint type" );
    const tripoint offset( radius, radius, radiusz );
    return tripoint_range<Tripoint>( center - offset,
    center + offset, [center, radius]( const Tripoint & pt ) {
        float r = trig_dist( center, pt );
        return radius - 0.5f < r && r < radius + 0.5f;
    } );
}

/* Template vodoo to allow passing lambdas to the below function without a compiler complaint
 * Courtesy of
 * https://stackoverflow.com/questions/13358672/how-to-convert-a-lambda-to-an-stdfunction-using-templates#13359347
 */
template <typename T>
struct tripoint_predicate_fun {
    using type = T;
};

template<typename Tripoint>
inline tripoint_range<Tripoint> points_in_radius_where( const Tripoint &center, const int radius,
        const typename tripoint_predicate_fun<std::function<bool( const Tripoint &pt )>>::type &func,
        const int radiusz = 0 )
{
    static_assert( Tripoint::dimension == 3, "Requires tripoint type" );
    const tripoint offset( radius, radius, radiusz );
    return tripoint_range<Tripoint>( center - offset, center + offset, func );
}

#endif // CATA_SRC_MAP_ITERATOR_H
