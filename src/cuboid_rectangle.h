#ifndef CATA_SRC_CUBOID_RECTANGLE_H
#define CATA_SRC_CUBOID_RECTANGLE_H

#include "point.h"

struct rectangle {
    point p_min;
    point p_max;
    constexpr rectangle() = default;
    constexpr rectangle( const point &P_MIN, const point &P_MAX ) :
        p_min( P_MIN ), p_max( P_MAX ) {}
};

struct half_open_rectangle : rectangle {
    using rectangle::rectangle;

    constexpr bool contains( const point &p ) const {
        return p.x >= p_min.x && p.x < p_max.x &&
               p.y >= p_min.y && p.y < p_max.y;
    }
};

struct inclusive_rectangle : rectangle {
    using rectangle::rectangle;

    constexpr bool contains( const point &p ) const {
        return p.x >= p_min.x && p.x <= p_max.x &&
               p.y >= p_min.y && p.y <= p_max.y;
    }
};

// Clamp p to the rectangle r.
// This independently clamps each coordinate of p to the bounds of the
// rectangle.
// Useful for example to round an arbitrary point to the nearest point on the
// screen, or the nearest point in a particular submap.
point clamp( const point &p, const half_open_rectangle &r );
point clamp( const point &p, const inclusive_rectangle &r );

struct cuboid {
    tripoint p_min;
    tripoint p_max;
    constexpr cuboid() = default;
    constexpr cuboid( const tripoint &P_MIN, const tripoint &P_MAX ) :
        p_min( P_MIN ), p_max( P_MAX )
    {}
    explicit constexpr cuboid( const rectangle &R, int Z1, int Z2 ) :
        p_min( tripoint( R.p_min, Z1 ) ), p_max( tripoint( R.p_max, Z2 ) ) {}

    void shrink( const tripoint &amount ) {
        p_min += amount;
        p_max -= amount;
    }
};

struct half_open_cuboid : cuboid {
    using cuboid::cuboid;

    constexpr bool contains( const tripoint &p ) const {
        return p.x >= p_min.x && p.x < p_max.x &&
               p.y >= p_min.y && p.y < p_max.y &&
               p.z >= p_min.z && p.z < p_max.z;
    }
};

struct inclusive_cuboid : cuboid {
    using cuboid::cuboid;

    constexpr bool contains( const tripoint &p ) const {
        return p.x >= p_min.x && p.x <= p_max.x &&
               p.y >= p_min.y && p.y <= p_max.y &&
               p.z >= p_min.z && p.z <= p_max.z;
    }
};

static constexpr cuboid cuboid_zero( tripoint_zero, tripoint_zero );
static constexpr rectangle rectangle_zero( point_zero, point_zero );

#endif // CATA_SRC_CUBOID_RECTANGLE_H
