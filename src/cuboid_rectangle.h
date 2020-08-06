#ifndef CATA_SRC_CUBOID_RECTANGLE_H
#define CATA_SRC_CUBOID_RECTANGLE_H

#include "point.h"
#include "point_traits.h"

template<typename Point>
struct rectangle {
    static_assert( Point::dimension == 2, "rectangle is for 2D points; use cuboid for 3D points" );

    Point p_min;
    Point p_max;
    constexpr rectangle() = default;
    constexpr rectangle( const Point &P_MIN, const Point &P_MAX ) :
        p_min( P_MIN ), p_max( P_MAX ) {}
};

template<typename Point>
struct half_open_rectangle : rectangle<Point> {
    using base = rectangle<Point>;
    using base::base;
    using base::p_min;
    using base::p_max;

    constexpr bool contains( const Point &p ) const {
        using Traits = point_traits<Point>;
        return Traits::x( p ) >= Traits::x( p_min ) && Traits::x( p ) < Traits::x( p_max ) &&
               Traits::y( p ) >= Traits::y( p_min ) && Traits::y( p ) < Traits::y( p_max );
    }
};

template<typename Point>
struct inclusive_rectangle : rectangle<Point> {
    using base = rectangle<Point>;
    using base::base;
    using base::p_min;
    using base::p_max;

    constexpr bool contains( const Point &p ) const {
        using Traits = point_traits<Point>;
        return Traits::x( p ) >= Traits::x( p_min ) && Traits::x( p ) <= Traits::x( p_max ) &&
               Traits::y( p ) >= Traits::y( p_min ) && Traits::y( p ) <= Traits::y( p_max );
    }
};

// Clamp p to the rectangle r.
// This independently clamps each coordinate of p to the bounds of the
// rectangle.
// Useful for example to round an arbitrary point to the nearest point on the
// screen, or the nearest point in a particular submap.
template<typename Point>
Point clamp( const Point &p, const half_open_rectangle<Point> &r )
{
    using Traits = point_traits<Point>;
    return Point( clamp( Traits::x( p ), Traits::x( r.p_min ), Traits::x( r.p_max ) - 1 ),
                  clamp( Traits::y( p ), Traits::y( r.p_min ), Traits::y( r.p_max ) - 1 ) );
}

template<typename Point>
Point clamp( const Point &p, const inclusive_rectangle<Point> &r )
{
    using Traits = point_traits<Point>;
    return Point( clamp( Traits::x( p ), Traits::x( r.p_min ), Traits::x( r.p_max ) ),
                  clamp( Traits::y( p ), Traits::y( r.p_min ), Traits::y( r.p_max ) ) );
}

template<typename Tripoint>
struct cuboid {
    static_assert( Tripoint::dimension == 3,
                   "cuboid is for 3D points; use rectangle for 2D points" );

    Tripoint p_min;
    Tripoint p_max;
    constexpr cuboid() = default;
    constexpr cuboid( const Tripoint &P_MIN, const Tripoint &P_MAX ) :
        p_min( P_MIN ), p_max( P_MAX )
    {}
    template<typename Rectangle>
    explicit constexpr cuboid( const Rectangle &R, int Z1, int Z2 ) :
        p_min( Tripoint( R.p_min, Z1 ) ), p_max( Tripoint( R.p_max, Z2 ) ) {}

    void shrink( const tripoint &amount ) {
        p_min += amount;
        p_max -= amount;
    }
};

template<typename Tripoint>
struct half_open_cuboid : cuboid<Tripoint> {
    using base = cuboid<Tripoint>;
    using base::base;
    using base::p_min;
    using base::p_max;

    constexpr bool contains( const Tripoint &p ) const {
        using Traits = point_traits<Tripoint>;
        return Traits::x( p ) >= Traits::x( p_min ) && Traits::x( p ) < Traits::x( p_max ) &&
               Traits::y( p ) >= Traits::y( p_min ) && Traits::y( p ) < Traits::y( p_max ) &&
               Traits::z( p ) >= Traits::z( p_min ) && Traits::z( p ) < Traits::z( p_max );
    }
};

template<typename Tripoint>
struct inclusive_cuboid : cuboid<Tripoint> {
    using base = cuboid<Tripoint>;
    using base::base;
    using base::p_min;
    using base::p_max;

    constexpr bool contains( const Tripoint &p ) const {
        using Traits = point_traits<Tripoint>;
        return Traits::x( p ) >= Traits::x( p_min ) && Traits::x( p ) <= Traits::x( p_max ) &&
               Traits::y( p ) >= Traits::y( p_min ) && Traits::y( p ) <= Traits::y( p_max ) &&
               Traits::z( p ) >= Traits::z( p_min ) && Traits::z( p ) <= Traits::z( p_max );
    }
};

static constexpr rectangle<point> rectangle_zero( point_zero, point_zero );
static constexpr cuboid<tripoint> cuboid_zero( tripoint_zero, tripoint_zero );

#endif // CATA_SRC_CUBOID_RECTANGLE_H
