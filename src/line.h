#pragma once
#ifndef CATA_SRC_LINE_H
#define CATA_SRC_LINE_H

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "coords_fwd.h"
#include "point.h"
#include "units.h"

namespace coords
{
template <typename Point, origin Origin, scale Scale> class coord_point_ob;
}  // namespace coords
struct rl_vec2d;
template <typename T> struct enum_traits;

extern bool trigdist;

/**
 * Calculate base of an isosceles triangle
 * @param distance one of the equal lengths
 * @param vertex the unequal angle
 * @returns base in equivalent units to distance
 */
double iso_tangent( double distance, const units::angle &vertex );

//! This compile-time usable function combines the sign of each (x, y, z) component into a single integer
//! to allow simple runtime and compile-time mapping of (x, y, z) tuples to @ref direction enumerators.
//! Specifically, (0, -, +) => (0, 1, 2); a base-3 number.
//! This only works correctly for inputs between -1,-1,-1 and 1,1,1.
//! For numbers outside that range, use make_xyz().
inline constexpr unsigned make_xyz_unit( const tripoint &p ) noexcept
{
    return ( ( p.x > 0 ) ? 2u : ( p.x < 0 ) ? 1u : 0u ) * 1u +
           ( ( p.y > 0 ) ? 2u : ( p.y < 0 ) ? 1u : 0u ) * 3u +
           ( ( p.z > 0 ) ? 2u : ( p.z < 0 ) ? 1u : 0u ) * 9u;
}

// This more general version of this function gives correct values for larger inputs.
unsigned make_xyz( const tripoint & );

enum class direction : unsigned {
    ABOVENORTHWEST = make_xyz_unit( tripoint::above + tripoint::north_west ),
    NORTHWEST      = make_xyz_unit( tripoint::north_west ),
    BELOWNORTHWEST = make_xyz_unit( tripoint::below + tripoint::north_west ),
    ABOVENORTH     = make_xyz_unit( tripoint::above + tripoint::north ),
    NORTH          = make_xyz_unit( tripoint::north ),
    BELOWNORTH     = make_xyz_unit( tripoint::below + tripoint::north ),
    ABOVENORTHEAST = make_xyz_unit( tripoint::above + tripoint::north_east ),
    NORTHEAST      = make_xyz_unit( tripoint::north_east ),
    BELOWNORTHEAST = make_xyz_unit( tripoint::below + tripoint::north_east ),

    ABOVEWEST      = make_xyz_unit( tripoint::above + tripoint::west ),
    WEST           = make_xyz_unit( tripoint::west ),
    BELOWWEST      = make_xyz_unit( tripoint::below + tripoint::west ),
    ABOVECENTER    = make_xyz_unit( tripoint::above ),
    CENTER         = make_xyz_unit( tripoint::zero ),
    BELOWCENTER    = make_xyz_unit( tripoint::below ),
    ABOVEEAST      = make_xyz_unit( tripoint::above + tripoint::east ),
    EAST           = make_xyz_unit( tripoint::east ),
    BELOWEAST      = make_xyz_unit( tripoint::below + tripoint::east ),

    ABOVESOUTHWEST = make_xyz_unit( tripoint::above + tripoint::south_west ),
    SOUTHWEST      = make_xyz_unit( tripoint::south_west ),
    BELOWSOUTHWEST = make_xyz_unit( tripoint::below + tripoint::south_west ),
    ABOVESOUTH     = make_xyz_unit( tripoint::above + tripoint::south ),
    SOUTH          = make_xyz_unit( tripoint::south ),
    BELOWSOUTH     = make_xyz_unit( tripoint::below + tripoint::south ),
    ABOVESOUTHEAST = make_xyz_unit( tripoint::above + tripoint::south_east ),
    SOUTHEAST      = make_xyz_unit( tripoint::south_east ),
    BELOWSOUTHEAST = make_xyz_unit( tripoint::below + tripoint::south_east ),

    last = 27
};

template<>
struct enum_traits<direction> {
    static constexpr direction last = direction::last;
};

namespace std
{
template <> struct hash<direction> {
    std::size_t operator()( const direction &d ) const {
        return static_cast<std::size_t>( d );
    }
};
} // namespace std

template< class T >
constexpr inline direction operator%( const direction &lhs, const T &rhs )
{
    return static_cast<direction>( static_cast<T>( lhs ) % rhs );
}

template< class T >
constexpr inline T operator+( const direction &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) + rhs;
}

template< class T >
constexpr inline bool operator==( const direction &lhs, const T &rhs )
{
    return static_cast<T>( lhs ) == rhs;
}

template< class T >
constexpr inline bool operator==( const T &lhs, const direction &rhs )
{
    return operator==( rhs, lhs );
}

template< class T >
constexpr inline bool operator!=( const T &lhs, const direction &rhs )
{
    return !operator==( rhs, lhs );
}

template< class T >
constexpr inline bool operator!=( const direction &lhs, const T &rhs )
{
    return !operator==( lhs, rhs );
}

direction direction_from( const point &p ) noexcept;
direction direction_from( const tripoint &p ) noexcept;
direction direction_from( const point &p1, const point &p2 ) noexcept;
direction direction_from( const tripoint &p, const tripoint &q );

tripoint displace( direction dir );
point displace_XY( direction dir );
std::string direction_name( direction dir );
std::string direction_name_short( direction dir );
std::string direction_arrow( direction dir );

/* Get suffix describing vector from p to q (e.g. 1NW, 2SE) or empty string if p == q */
std::string direction_suffix( const tripoint_bub_ms &p, const tripoint_bub_ms &q );
std::string direction_suffix( const tripoint_abs_ms &p, const tripoint_abs_ms &q );

/**
 * The actual Bresenham algorithm in 2D and 3D, everything else should call these
 * and pass in an interact functor to iterate across a line between two points.
 */
void bresenham( const point_bub_ms &p1, const point_bub_ms &p2, int t,
                const std::function<bool( const point_bub_ms & )> &interact );
void bresenham( const tripoint_bub_ms &loc1, const tripoint_bub_ms &loc2, int t, int t2,
                const std::function<bool( const tripoint_bub_ms & )> &interact );

tripoint move_along_line( const tripoint &loc, const std::vector<tripoint> &line,
                          int distance );
tripoint_bub_ms move_along_line( const tripoint_bub_ms &loc,
                                 const std::vector<tripoint_bub_ms> &line, int distance );
// line from p1 to p2, including p2 but not p1, using Bresenham's algorithm
// The "t" value decides WHICH Bresenham line is used.
std::vector<point> line_to( const point &p1, const point &p2, int t = 0 );
// line from p1 to p2, including p2 but not p1, using Bresenham's algorithm
// t and t2 decide which Bresenham line is used.
std::vector<tripoint> line_to( const tripoint &loc1, const tripoint &loc2, int t = 0, int t2 = 0 );
// sqrt(dX^2 + dY^2)

inline float trig_dist( const tripoint &loc1, const tripoint &loc2 )
{
    return std::sqrt( static_cast<double>( ( loc1.x - loc2.x ) * ( loc1.x - loc2.x ) ) +
                      ( ( loc1.y - loc2.y ) * ( loc1.y - loc2.y ) ) +
                      ( ( loc1.z - loc2.z ) * ( loc1.z - loc2.z ) ) );
}
float trig_dist( const tripoint_bub_ms &loc1, const tripoint_bub_ms &loc2 );
inline float trig_dist( const point &loc1, const point &loc2 )
{
    return trig_dist( tripoint( loc1, 0 ), tripoint( loc2, 0 ) );
}
float trig_dist( const point_bub_ms &loc1, const point_bub_ms &loc2 );

// Roguelike distance; maximum of dX and dY
inline int square_dist( const tripoint &loc1, const tripoint &loc2 )
{
    const tripoint d = ( loc1 - loc2 ).abs();
    return std::max( { d.x, d.y, d.z } );
}
inline int square_dist( const point &loc1, const point &loc2 )
{
    const point d = ( loc1 - loc2 ).abs();
    return std::max( d.x, d.y );
}

// Choose between the above two according to the "circular distances" option
inline int rl_dist( const tripoint &loc1, const tripoint &loc2 )
{
    if( trigdist ) {
        return trig_dist( loc1, loc2 );
    }
    return square_dist( loc1, loc2 );
}
inline int rl_dist( const point &a, const point &b )
{
    return rl_dist( tripoint( a, 0 ), tripoint( b, 0 ) );
}

/**
 * Helper type for the return value of dist_fast().
 *
 * This lets us delay the sqrt() call of trigdist until the actual length is needed.
 */
struct FastDistanceApproximation {
    private:
        int value;
    public:
        explicit inline FastDistanceApproximation( int value ) : value( value ) { }
        template<typename T>
        inline bool operator<=( const T &rhs ) const {
            if( trigdist ) {
                return value <= rhs * rhs;
            }
            return value <= rhs;
        }
        template<typename T>
        inline bool operator>=( const T &rhs ) const {
            if( trigdist ) {
                return value >= rhs * rhs;
            }
            return value >= rhs;
        }
        inline explicit operator int() const {
            if( trigdist ) {
                return std::sqrt( value );
            }
            return value;
        }
};

FastDistanceApproximation trig_dist_fast( const tripoint_bub_ms &loc1,
        const tripoint_bub_ms &loc2 );
FastDistanceApproximation square_dist_fast( const tripoint_bub_ms &loc1,
        const tripoint_bub_ms &loc2 );
inline FastDistanceApproximation rl_dist_fast( const tripoint_bub_ms &loc1,
        const tripoint_bub_ms &loc2 )
{
    if( trigdist ) {
        return trig_dist_fast( loc1, loc2 );
    }
    return square_dist_fast( loc1, loc2 );
}
FastDistanceApproximation rl_dist_fast( const point_bub_ms &a, const point_bub_ms &b );

float rl_dist_exact( const tripoint &loc1, const tripoint &loc2 );
// Sum of distance in both axes
int manhattan_dist( const point &loc1, const point &loc2 );

// Travel distance between 2 points on a square grid, assuming diagonal moves
// cost sqrt(2) and cardinal moves cost 1.
int octile_dist( const point &loc1, const point &loc2, int multiplier = 1 );
float octile_dist_exact( const point &loc1, const point &loc2 );

// get angle of direction represented by point
units::angle atan2( const point & );
units::angle atan2( const rl_vec2d & );

// Get the magnitude of the slope ranging from 0.0 to 1.0
float get_normalized_angle( const point &start, const point &end );
std::vector<tripoint> continue_line( const std::vector<tripoint> &line, int distance );
std::vector<tripoint_bub_ms> continue_line( const std::vector<tripoint_bub_ms> &line,
        int distance );
std::vector<point_bub_ms> squares_in_direction( const point_bub_ms &p1, const point_bub_ms &p2 );
std::vector<point_omt_ms> squares_in_direction( const point_omt_ms &p1, const point_omt_ms &p2 );
// Returns a vector of squares adjacent to @from that are closer to @to than @from is.
// Currently limited to the same z-level as @from.
std::vector<tripoint_bub_ms> squares_closer_to( const tripoint_bub_ms &from,
        const tripoint_bub_ms &to );
void calc_ray_end( units::angle, int range, const tripoint &p, tripoint &out );
template<typename Point, coords::origin Origin, coords::scale Scale>
void calc_ray_end( units::angle angle, int range,
                   const coords::coord_point_ob<Point, Origin, Scale> &p,
                   coords::coord_point_ob<Point, Origin, Scale> &out )
{
    calc_ray_end( angle, range, p.raw(), out.raw() );
}
/**
 * Calculates the horizontal angle between the lines from (0,0,0) to @p a and
 * the line from (0,0,0) to @p b.
 * Returned value is in degree and in the range 0....360.
 * Example: if @p a is (0,1) and @p b is (1,0), the result will 90 degree
 * The function currently ignores the z component.
 */
units::angle coord_to_angle( const tripoint &a, const tripoint &b );
template<typename Point, coords::origin Origin, coords::scale Scale>
units::angle coord_to_angle( const coords::coord_point_ob<Point, Origin, Scale> &a,
                             const coords::coord_point_ob<Point, Origin, Scale> &b )
{
    return coord_to_angle( a.raw(), b.raw() );
}

// weird class for 2d vectors where dist is derived from rl_dist
struct rl_vec2d {
    float x;
    float y;

    // vec2d(){}
    explicit rl_vec2d( float x = 0, float y = 0 ) : x( x ), y( y ) {}
    explicit rl_vec2d( const point &p ) : x( p.x ), y( p.y ) {}

    float magnitude() const;
    rl_vec2d normalized() const;
    rl_vec2d rotated( float angle ) const;
    float dot_product( const rl_vec2d &v ) const;
    bool is_null() const;

    point as_point() const;

    // scale.
    rl_vec2d operator* ( float rhs ) const;
    rl_vec2d operator/ ( float rhs ) const;
    // subtract
    rl_vec2d operator- ( const rl_vec2d &rhs ) const;
    // unary negation
    rl_vec2d operator- () const;
    rl_vec2d operator+ ( const rl_vec2d &rhs ) const;
};

struct rl_vec3d {
    float x;
    float y;
    float z;

    explicit rl_vec3d( float x = 0, float y = 0, float z = 0 ) : x( x ), y( y ), z( z ) {}
    explicit rl_vec3d( const rl_vec2d &xy, float z = 0 ) : x( xy.x ), y( xy.y ), z( z ) {}
    explicit rl_vec3d( const tripoint &p ) : x( p.x ), y( p.y ), z( p.z ) {}

    rl_vec2d xy() const;

    float magnitude() const;
    rl_vec3d normalized() const;
    rl_vec3d rotated( float angle ) const;
    float dot_product( const rl_vec3d &v ) const;
    bool is_null() const;

    tripoint as_point() const;

    // scale.
    rl_vec3d &operator*=( float rhs );
    rl_vec3d &operator/=( float rhs );
    rl_vec3d operator*( float rhs ) const;
    rl_vec3d operator/( float rhs ) const;
    // subtract
    rl_vec3d operator-( const rl_vec3d &rhs ) const;
    // unary negation
    rl_vec3d operator-() const;
    rl_vec3d operator+( const rl_vec3d &rhs ) const;
};

#endif // CATA_SRC_LINE_H
