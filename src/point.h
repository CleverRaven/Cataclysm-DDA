#pragma once
#ifndef CATA_SRC_POINT_H
#define CATA_SRC_POINT_H

#include <algorithm>
#include <array>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <functional>
#include <optional>
#include <ostream>
#include <string>
#include <tuple>
#include <vector>

class JsonArray;
class JsonOut;

// NOLINTNEXTLINE(cata-xy)
struct point {
    static constexpr int dimension = 2;

    // Point representing the origin
    static const point zero;
    // Point with minimum representable coordinates
    static const point min;
    // Point with maximum representable coordinates
    static const point max;
    // Sentinel value for an invalid point
    // Equal to @ref min for backward compatibility.
    static const point invalid;
    inline bool is_invalid() const {
        return *this == invalid;
    }

    // Points representing unit steps in cardinal directions
    static const point north;
    static const point north_east;
    static const point east;
    static const point south_east;
    static const point south;
    static const point south_west;
    static const point west;
    static const point north_west;

    int x = 0;
    int y = 0;
    constexpr point() = default;
    constexpr point( int X, int Y ) : x( X ), y( Y ) {}

    static point from_string( const std::string & );

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

    point abs() const {
        return point( std::abs( x ), std::abs( y ) );
    }

    // Dummy implementation of raw() to allow reasoning about
    // generic points.
    constexpr point raw() const {
        return *this;
    }

    /**
     * Rotate point clockwise @param turns times, 90 degrees per turn,
     * around the center of a rectangle with the dimensions specified
     * by @param dim
     * By default rotates around the origin (0, 0).
     * NOLINTNEXTLINE(cata-use-named-point-constants) */
    point rotate( int turns, const point &dim = { 1, 1 } ) const;

    float distance( const point &rhs ) const;
    int distance_manhattan( const point &rhs ) const;

    std::string to_string() const;
    std::string to_string_writable() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonArray &jsin );

    friend inline constexpr bool operator<( const point &a, const point &b ) {
        return a.x < b.x || ( a.x == b.x && a.y < b.y );
    }
    friend inline constexpr bool operator==( const point &a, const point &b ) {
        return a.x == b.x && a.y == b.y;
    }
    friend inline constexpr bool operator!=( const point &a, const point &b ) {
        return !( a == b );
    }

    friend std::ostream &operator<<( std::ostream &, const point & );
    friend std::istream &operator>>( std::istream &, point & );
};

inline int divide_round_to_minus_infinity( int n, int d )
{
    // The NOLINT comments here are to suppress a clang-tidy warning that seems
    // to be a clang-tidy bug.  I'd like to get rid of them if the bug is ever
    // fixed.  The warning comes via a project_remain call in
    // mission_companion.cpp.
    if( n >= 0 ) {
        return n / d; // NOLINT(clang-analyzer-core.DivideZero)
    }
    return ( n - d + 1 ) / d; // NOLINT(clang-analyzer-core.DivideZero)
}

inline point multiply_xy( const point &p, int f )
{
    return point( p.x * f, p.y * f );
}

inline point divide_xy_round_to_minus_infinity( const point &p, int d )
{
    return point( divide_round_to_minus_infinity( p.x, d ),
                  divide_round_to_minus_infinity( p.y, d ) );
}

inline point divide_xy_round_to_minus_infinity_non_negative( const point &p, int d )
{
    // This results in code only being generated for the case where x/y are positive.
    return point( static_cast<unsigned int>( p.x ) / d, static_cast<unsigned int>( p.y ) / d );
}

// NOLINTNEXTLINE(cata-xy)
struct tripoint {
    static constexpr int dimension = 3;

    // Tripoint representing the origin
    static const tripoint zero;
    // Tripoint with minimum representable coordinates
    static const tripoint min;
    // Tripoint with maximum representable coordinates
    static const tripoint max;
    // Sentinel value for an invalid tripoint
    // Equal to @ref min for backward compatibility.
    static const tripoint invalid;
    inline bool is_invalid() const {
        return *this == invalid;
    }


    // Tripoints representing unit steps in cardinal directions
    static const tripoint north;
    static const tripoint north_east;
    static const tripoint east;
    static const tripoint south_east;
    static const tripoint south;
    static const tripoint south_west;
    static const tripoint west;
    static const tripoint north_west;
    static const tripoint above;
    static const tripoint below;

    int x = 0;
    int y = 0;
    int z = 0;
    constexpr tripoint() = default;
    constexpr tripoint( int X, int Y, int Z ) : x( X ), y( Y ), z( Z ) {}
    constexpr tripoint( const point &p, int Z ) : x( p.x ), y( p.y ), z( Z ) {}

    static tripoint from_string( const std::string & );

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
    constexpr tripoint operator/( const int rhs ) const {
        return tripoint( x / rhs, y / rhs, z / rhs );
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

    tripoint abs() const {
        return tripoint( std::abs( x ), std::abs( y ), std::abs( z ) );
    }

    constexpr point xy() const {
        return point( x, y );
    }

    // Dummy implementation of raw() to allow reasoning about
    // abstract generic points.
    constexpr tripoint raw() const {
        return *this;
    }

    /**
     * Rotates just the x,y component of the tripoint. See point::rotate()
     * NOLINTNEXTLINE(cata-use-named-point-constants) */
    tripoint rotate( int turns, const point &dim = { 1, 1 } ) const {
        return tripoint( xy().rotate( turns, dim ), z );
    }

    std::string to_string() const;
    std::string to_string_writable() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonArray &jsin );

    friend std::ostream &operator<<( std::ostream &, const tripoint & );
    friend std::istream &operator>>( std::istream &, tripoint & );

    friend inline constexpr bool operator==( const tripoint &a, const tripoint &b ) {
        return a.x == b.x && a.y == b.y && a.z == b.z;
    }
    friend inline constexpr bool operator!=( const tripoint &a, const tripoint &b ) {
        return !( a == b );
    }
    friend inline bool operator<( const tripoint &a, const tripoint &b ) {
        return std::tie( a.x, a.y, a.z ) < std::tie( b.x, b.y, b.z );
    }
};

inline tripoint multiply_xy( const tripoint &p, int f )
{
    return tripoint( p.x * f, p.y * f, p.z );
}

inline tripoint divide_xy_round_to_minus_infinity( const tripoint &p, int d )
{
    return tripoint( divide_round_to_minus_infinity( p.x, d ),
                     divide_round_to_minus_infinity( p.y, d ),
                     p.z );
}

inline tripoint divide_xy_round_to_minus_infinity_non_negative( const tripoint &p, int d )
{
    return tripoint( divide_xy_round_to_minus_infinity_non_negative( p.xy(), d ), p.z );
}

inline constexpr const point point::min = { INT_MIN, INT_MIN };
inline constexpr const point point::max = { INT_MAX, INT_MAX };
inline constexpr const point point::invalid = point::min;
inline constexpr const point point::zero{};
inline constexpr const point point::north{ 0, -1 };
inline constexpr const point point::north_east{ 1, -1 };
inline constexpr const point point::east{ 1, 0 };
inline constexpr const point point::south_east{ 1, 1 };
inline constexpr const point point::south{ 0, 1 };
inline constexpr const point point::south_west{ -1, 1 };
inline constexpr const point point::west{ -1, 0 };
inline constexpr const point point::north_west{ -1, -1 };


inline constexpr const tripoint tripoint::min = { INT_MIN, INT_MIN, INT_MIN };
inline constexpr const tripoint tripoint::max = { INT_MAX, INT_MAX, INT_MAX };
inline constexpr const tripoint tripoint::invalid = tripoint::min;

inline constexpr const tripoint tripoint::zero{};
inline constexpr const tripoint tripoint::north{ point::north, 0 };
inline constexpr const tripoint tripoint::north_east{ point::north_east, 0 };
inline constexpr const tripoint tripoint::east{ point::east, 0 };
inline constexpr const tripoint tripoint::south_east{ point::south_east, 0 };
inline constexpr const tripoint tripoint::south{ point::south, 0 };
inline constexpr const tripoint tripoint::south_west{ point::south_west, 0 };
inline constexpr const tripoint tripoint::west{ point::west, 0 };
inline constexpr const tripoint tripoint::north_west{ point::north_west, 0 };
inline constexpr const tripoint tripoint::above{ 0, 0, 1 };
inline constexpr const tripoint tripoint::below{ 0, 0, -1 };


struct sphere {
    int radius = 0;
    tripoint center = tripoint::zero;

    sphere() = default;
    explicit sphere( const tripoint &center ) : radius( 1 ), center( center ) {}
    explicit sphere( const tripoint &center, int radius ) : radius( radius ), center( center ) {}
};

/**
 * Following functions return points in a spiral pattern starting at center_x/center_y until it hits the radius. Clockwise fashion.
 * Credit to Tom J Nowell; http://stackoverflow.com/a/1555236/1269969
 */
std::vector<tripoint> closest_points_first( const tripoint &center, int max_dist );
std::vector<tripoint> closest_points_first( const tripoint &center, int min_dist, int max_dist );

std::vector<point> closest_points_first( const point &center, int max_dist );
std::vector<point> closest_points_first( const point &center, int min_dist, int max_dist );

template <typename PredicateFn, typename Point>
std::optional<Point> find_point_closest_first( const Point &center, int min_dist, int max_dist,
        PredicateFn &&fn );

template <typename PredicateFn, typename Point>
std::optional<Point> find_point_closest_first( const Point &center, int max_dist,
        PredicateFn &&fn );


// Calculate the number of tiles in a square from min_dist to max_dist about an arbitrary centre.
std::optional<int> rectangle_size( int min_dist, int max_dist );

// Make point hashable so it can be used as an unordered_set or unordered_map key,
// or a component of one.
namespace std
{
template <>
struct hash<point> {
    std::size_t operator()( const point &k ) const noexcept {
        // We cast k.y to uint32_t because otherwise when promoting to uint64_t for binary `or` it
        // will sign extend and turn the upper 32 bits into all 1s.
        uint64_t x = static_cast<uint64_t>( k.x ) << 32 | static_cast<uint32_t>( k.y );

        // Found through https://nullprogram.com/blog/2018/07/31/
        // Public domain source from https://xoshiro.di.unimi.it/splitmix64.c
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9U;
        x ^= x >> 27;
        x *= 0x94d049bb133111ebU;
        x ^= x >> 31;
        return x;

        return x;
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
        // We cast k.y to uint32_t because otherwise when promoting to uint64_t for binary `or` it
        // will sign extend and turn the upper 32 bits into all 1s.
        uint64_t x = static_cast<uint64_t>( k.x ) << 32 | static_cast<uint32_t>( k.y );

        // Found through https://nullprogram.com/blog/2018/07/31/
        // Public domain source from https://xoshiro.di.unimi.it/splitmix64.c
        x ^= x >> 30;
        x *= 0xbf58476d1ce4e5b9U;
        x ^= x >> 27;

        // Sprinkle in z now.
        x ^= static_cast<uint64_t>( k.z );
        x *= 0x94d049bb133111ebU;
        x ^= x >> 31;
        return x;
    }
};
} // namespace std

inline constexpr const std::array<point, 4> four_adjacent_offsets{{
        point::north, point::east, point::south, point::west
    }};

inline constexpr const std::array<point, 4> neighborhood{ {
        point::south, point::east, point::west, point::north
    }};

inline constexpr const std::array<point, 4> offsets = {{
        point::south, point::east, point::west, point::north
    }
};

inline constexpr const std::array<point, 4> four_cardinal_directions{{
        point::west, point::east, point::north, point::south
    }};

inline constexpr const std::array<point, 5> five_cardinal_directions{{
        point::west, point::east, point::north, point::south, point::zero
    }};

inline constexpr const std::array<tripoint, 8> eight_horizontal_neighbors = { {
        { tripoint::north_west },
        { tripoint::north },
        { tripoint::north_east },
        { tripoint::west },
        { tripoint::east },
        { tripoint::south_west },
        { tripoint::south },
        { tripoint::south_east },
    }
};

/* Return points in a clockwise spiral from min_dist to max_dist, inclusive, ordered by
 * the closest points first. The distance is calculated using roguelike distances, so
 * movement in any direction only counts as 1.
 *
 * The predicate function is evaluated on each point. If the function returns true, the
 * point is returned. If the predicate function never returns true then std::nullopt is
 * returned once max_dist is reached.
 *
 * @param center The center of the spiral.
 * @param min_dist minimum distance to start the spiral from.
 * @param max_dist greatest distance from the centre that the spiral will go to.
 * @returns std::nullopt if min_dist > max_dist or predicate_fn evaluated to true for
 *      no points.
 */
template <typename PredicateFn, typename Point>
std::optional<Point> find_point_closest_first( const Point &center, int min_dist, int max_dist,
        PredicateFn &&predicate_fn )
{
    const std::optional<int> n = rectangle_size( min_dist, max_dist );

    if( n == std::nullopt ) {
        return {};
    }

    const bool is_center_included = min_dist == 0;

    if( is_center_included && predicate_fn( center ) ) {
        return center;
    }

    int x_init = std::max( min_dist, 1 );
    point p {x_init, 1 - x_init};

    point d = point::east;

    for( int i = is_center_included; i < *n; i++ ) {
        const Point next = Point{ center.raw() + p.raw() };
        if( predicate_fn( next ) ) {
            return next;
        }

        if( p.x == p.y || ( p.x < 0 && p.x == -p.y ) || ( p.x > 0 && p.x == 1 - p.y ) ) {
            std::swap( d.x, d.y );
            d.x = -d.x;
        }

        p += d;
    }

    return std::nullopt;
}

#endif // CATA_SRC_POINT_H
