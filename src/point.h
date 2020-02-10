#pragma once
#ifndef CATA_POINT_H
#define CATA_POINT_H

// The CATA_NO_STL macro is used by the cata clang-tidy plugin tests so they
// can include this header when compiling with -nostdinc++
#ifndef CATA_NO_STL

#include <array>
#include <cassert>
#include <cstddef>
#include <climits>
#include <functional>
#include <ostream>
#include <vector>

#else

#define assert(...)

namespace std
{
class string;
class ostream;
}

#endif // CATA_NO_STL

class JsonOut;
class JsonIn;

// NOLINTNEXTLINE(cata-xy)
struct point {
    int x = 0;
    int y = 0;
    constexpr point() = default;
    constexpr point( int X, int Y ) : x( X ), y( Y ) {}

    constexpr point operator+( const point &rhs ) const {
        return point( x + rhs.x, y + rhs.y );
    }
    point &operator+=( const point &rhs ) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    constexpr point operator-() const {
        return point( -x, -y );
    }
    constexpr point operator-( const point &rhs ) const {
        return point( x - rhs.x, y - rhs.y );
    }
    point &operator-=( const point &rhs ) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    constexpr point operator*( const int rhs ) const {
        return point( x * rhs, y * rhs );
    }
    friend constexpr point operator*( int lhs, const point &rhs ) {
        return rhs * lhs;
    }
    point &operator*=( const int rhs ) {
        x *= rhs;
        y *= rhs;
        return *this;
    }
    constexpr point operator/( const int rhs ) const {
        return point( x / rhs, y / rhs );
    }

    /**
     * Rotate point clockwise @param turns times, 90 degrees per turn,
     * around the center of a rectangle with the dimensions specified
     * by @param dim
     * By default rotates around the origin (0, 0).
     * NOLINTNEXTLINE(cata-use-named-point-constants) */
    point rotate( int turns, const point &dim = { 1, 1 } ) const {
        assert( turns >= 0 );
        assert( turns <= 4 );

        switch( turns ) {
            case 1:
                return { dim.y - y - 1, x };
            case 2:
                return { dim.x - x - 1, dim.y - y - 1 };
            case 3:
                return { y, dim.x - x - 1 };
        }

        return *this;
    }

    std::string to_string() const;
};

std::ostream &operator<<( std::ostream &, const point & );

void serialize( const point &p, JsonOut &jsout );
void deserialize( point &p, JsonIn &jsin );

inline constexpr bool operator<( const point &a, const point &b )
{
    return a.x < b.x || ( a.x == b.x && a.y < b.y );
}
inline constexpr bool operator==( const point &a, const point &b )
{
    return a.x == b.x && a.y == b.y;
}
inline constexpr bool operator!=( const point &a, const point &b )
{
    return !( a == b );
}

// NOLINTNEXTLINE(cata-xy)
struct tripoint {
    int x = 0;
    int y = 0;
    int z = 0;
    constexpr tripoint() = default;
    constexpr tripoint( int X, int Y, int Z ) : x( X ), y( Y ), z( Z ) {}
    constexpr tripoint( const point &p, int Z ) : x( p.x ), y( p.y ), z( Z ) {}

    constexpr tripoint operator+( const tripoint &rhs ) const {
        return tripoint( x + rhs.x, y + rhs.y, z + rhs.z );
    }
    constexpr tripoint operator-( const tripoint &rhs ) const {
        return tripoint( x - rhs.x, y - rhs.y, z - rhs.z );
    }
    tripoint &operator+=( const tripoint &rhs ) {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
        return *this;
    }
    constexpr tripoint operator-() const {
        return tripoint( -x, -y, -z );
    }
    constexpr tripoint operator*( const int rhs ) const {
        return tripoint( x * rhs, y * rhs, z * rhs );
    }
    friend constexpr tripoint operator*( int lhs, const tripoint &rhs ) {
        return rhs * lhs;
    }
    tripoint &operator*=( const int rhs ) {
        x *= rhs;
        y *= rhs;
        z *= rhs;
        return *this;
    }
    /*** some point operators and functions ***/
    constexpr tripoint operator+( const point &rhs ) const {
        return tripoint( x + rhs.x, y + rhs.y, z );
    }
    friend constexpr tripoint operator+( const point &lhs, const tripoint &rhs ) {
        return rhs + lhs;
    }
    constexpr tripoint operator-( const point &rhs ) const {
        return tripoint( x - rhs.x, y - rhs.y, z );
    }
    tripoint &operator+=( const point &rhs ) {
        x += rhs.x;
        y += rhs.y;
        return *this;
    }
    tripoint &operator-=( const point &rhs ) {
        x -= rhs.x;
        y -= rhs.y;
        return *this;
    }
    tripoint &operator-=( const tripoint &rhs ) {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
        return *this;
    }

    constexpr point xy() const {
        return point( x, y );
    }

    std::string to_string() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( JsonIn &jsin );
};

std::ostream &operator<<( std::ostream &, const tripoint & );

inline constexpr bool operator==( const tripoint &a, const tripoint &b )
{
    return a.x == b.x && a.y == b.y && a.z == b.z;
}
inline constexpr bool operator!=( const tripoint &a, const tripoint &b )
{
    return !( a == b );
}
inline bool operator<( const tripoint &a, const tripoint &b )
{
    if( a.x != b.x ) {
        return a.x < b.x;
    }
    if( a.y != b.y ) {
        return a.y < b.y;
    }
    if( a.z != b.z ) {
        return a.z < b.z;
    }
    return false;
}

struct rectangle {
    point p_min;
    point p_max;
    constexpr rectangle() = default;
    constexpr rectangle( const point &P_MIN, const point &P_MAX ) : p_min( P_MIN ), p_max( P_MAX ) {}

    constexpr bool contains_half_open( const point &p ) const {
        return p.x >= p_min.x && p.x < p_max.x &&
               p.y >= p_min.y && p.y < p_max.y;
    }

    constexpr bool contains_inclusive( const point &p ) const {
        return p.x >= p_min.x && p.x <= p_max.x &&
               p.y >= p_min.y && p.y <= p_max.y;
    }
};

// Clamp p to the half-open rectangle r.
// This independently clamps each coordinate of p to the bounds of the
// rectangle.
// Useful for example to round an arbitrary point to the nearest point on the
// screen, or the nearest point in a particular submap.
point clamp_half_open( const point &p, const rectangle &r );
point clamp_inclusive( const point &p, const rectangle &r );

struct box {
    tripoint p_min;
    tripoint p_max;
    constexpr box() = default;
    constexpr box( const tripoint &P_MIN, const tripoint &P_MAX ) : p_min( P_MIN ), p_max( P_MAX ) {}
    explicit constexpr box( const rectangle &R, int Z1, int Z2 ) :
        p_min( tripoint( R.p_min, Z1 ) ), p_max( tripoint( R.p_max, Z2 ) ) {}

    constexpr bool contains_half_open( const tripoint &p ) const {
        return p.x >= p_min.x && p.x < p_max.x &&
               p.y >= p_min.y && p.y < p_max.y &&
               p.z >= p_min.z && p.z < p_max.z;
    }

    constexpr bool contains_inclusive( const tripoint &p ) const {
        return p.x >= p_min.x && p.x <= p_max.x &&
               p.y >= p_min.y && p.y <= p_max.y &&
               p.z >= p_min.z && p.z <= p_max.z;
    }

    void shrink( const tripoint &amount ) {
        p_min += amount;
        p_max -= amount;
    }
};

static constexpr tripoint tripoint_zero { 0, 0, 0 };
static constexpr point point_zero{ tripoint_zero.xy() };

static constexpr point point_north{ 0, -1 };
static constexpr point point_north_east{ 1, -1 };
static constexpr point point_east{ 1, 0 };
static constexpr point point_south_east{ 1, 1 };
static constexpr point point_south{ 0, 1 };
static constexpr point point_south_west{ -1, 1 };
static constexpr point point_west{ -1, 0 };
static constexpr point point_north_west{ -1, -1 };

static constexpr tripoint tripoint_north{ point_north, 0 };
static constexpr tripoint tripoint_north_east{ point_north_east, 0 };
static constexpr tripoint tripoint_east{ point_east, 0 };
static constexpr tripoint tripoint_south_east{ point_south_east, 0 };
static constexpr tripoint tripoint_south{ point_south, 0 };
static constexpr tripoint tripoint_south_west{ point_south_west, 0 };
static constexpr tripoint tripoint_west{ point_west, 0 };
static constexpr tripoint tripoint_north_west{ point_north_west, 0 };

static constexpr tripoint tripoint_above{ 0, 0, 1 };
static constexpr tripoint tripoint_below{ 0, 0, -1 };

static constexpr box box_zero( tripoint_zero, tripoint_zero );
static constexpr rectangle rectangle_zero( point_zero, point_zero );

struct sphere {
    int radius = 0;
    tripoint center = tripoint_zero;

    sphere() = default;
    explicit sphere( const tripoint &center ) : radius( 1 ), center( center ) {}
    explicit sphere( const tripoint &center, int radius ) : radius( radius ), center( center ) {}
};

#ifndef CATA_NO_STL

/**
 * Following functions return points in a spiral pattern starting at center_x/center_y until it hits the radius. Clockwise fashion.
 * Credit to Tom J Nowell; http://stackoverflow.com/a/1555236/1269969
 */
std::vector<tripoint> closest_tripoints_first( const tripoint &center, int max_dist );
std::vector<tripoint> closest_tripoints_first( const tripoint &center, int min_dist, int max_dist );

std::vector<point> closest_points_first( const point &center, int max_dist );
std::vector<point> closest_points_first( const point &center, int min_dist, int max_dist );


inline point abs( const point &p )
{
    return point( abs( p.x ), abs( p.y ) );
}

inline tripoint abs( const tripoint &p )
{
    return tripoint( abs( p.x ), abs( p.y ), abs( p.z ) );
}

static constexpr tripoint tripoint_min { INT_MIN, INT_MIN, INT_MIN };
static constexpr tripoint tripoint_max{ INT_MAX, INT_MAX, INT_MAX };

static constexpr point point_min{ tripoint_min.xy() };
static constexpr point point_max{ tripoint_max.xy() };

// Make point hashable so it can be used as an unordered_set or unordered_map key,
// or a component of one.
namespace std
{
template <>
struct hash<point> {
    std::size_t operator()( const point &k ) const noexcept {
        constexpr uint64_t a = 2862933555777941757;
        size_t result = k.y;
        result *= a;
        result += k.x;
        return result;
    }
};
} // namespace std

// Make tripoint hashable so it can be used as an unordered_set or unordered_map key,
// or a component of one.
namespace std
{
template <>
struct hash<tripoint> {
    std::size_t operator()( const tripoint &k ) const noexcept {
        constexpr uint64_t a = 2862933555777941757;
        size_t result = k.z;
        result *= a;
        result += k.y;
        result *= a;
        result += k.x;
        return result;
    }
};
} // namespace std

static constexpr std::array<point, 4> four_adjacent_offsets{{
        point_north, point_east, point_south, point_west
    }};

static constexpr std::array<point, 4> neighborhood{ {
        point_south, point_east, point_west, point_north
    }};

static constexpr std::array<point, 4> offsets = {{
        point_south, point_east, point_west, point_north
    }
};

static constexpr std::array<point, 4> four_cardinal_directions{{
        point_west, point_east, point_north, point_south
    }};

static constexpr std::array<point, 5> five_cardinal_directions{{
        point_west, point_east, point_north, point_south, point_zero
    }};

static const std::array<tripoint, 8> eight_horizontal_neighbors = { {
        { tripoint_north_west },
        { tripoint_north },
        { tripoint_north_east },
        { tripoint_west },
        { tripoint_east },
        { tripoint_south_west },
        { tripoint_south },
        { tripoint_south_east },
    }
};

#endif // CATA_NO_STL

#endif // CATA_POINT_H
