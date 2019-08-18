#pragma once
#ifndef LINE_H
#define LINE_H

#include <cmath>
#include <functional>
#include <string>
#include <vector>

#include "math_defines.h"
#include "point.h"

/** Converts degrees to radians */
constexpr double DEGREES( double v )
{
    return v * M_PI / 180;
}

/** Converts arc minutes to radians */
constexpr double ARCMIN( double v )
{
    return DEGREES( v ) / 60;
}

/**
 * Calculate base of an isosceles triangle
 * @param distance one of the equal lengths
 * @param vertex the unequal angle expressed in MoA
 * @returns base in equivalent units to distance
 */
inline double iso_tangent( double distance, double vertex )
{
    // we can use the cosine formula (a² = b² + c² - 2bc⋅cosθ) to calculate the tangent
    return sqrt( 2 * pow( distance, 2 ) * ( 1 - cos( ARCMIN( vertex ) ) ) );
}

//! This compile-time usable function combines the sign of each (x, y, z) component into a single integer
//! to allow simple runtime and compile-time mapping of (x, y, z) tuples to @ref direction enumerators.
//! Specifically, (0, -, +) => (0, 1, 2); a base-3 number.
//! This only works correctly for inputs between -1,-1,-1 and 1,1,1.
//! For numbers outside that range, use make_xyz().
inline constexpr unsigned make_xyz_unit( const int x, const int y, const int z ) noexcept
{
    return ( ( x > 0 ) ? 2u : ( x < 0 ) ? 1u : 0u ) * 1u +
           ( ( y > 0 ) ? 2u : ( y < 0 ) ? 1u : 0u ) * 3u +
           ( ( z > 0 ) ? 2u : ( z < 0 ) ? 1u : 0u ) * 9u;
}

// This more general version of this function gives correct values for larger inputs.
unsigned make_xyz( const tripoint & );

enum direction : unsigned {
    ABOVENORTHWEST = make_xyz_unit( -1, -1, -1 ),
    NORTHWEST      = make_xyz_unit( -1, -1,  0 ),
    BELOWNORTHWEST = make_xyz_unit( -1, -1,  1 ),
    ABOVENORTH     = make_xyz_unit( 0, -1, -1 ),
    NORTH          = make_xyz_unit( 0, -1,  0 ),
    BELOWNORTH     = make_xyz_unit( 0, -1,  1 ),
    ABOVENORTHEAST = make_xyz_unit( 1, -1, -1 ),
    NORTHEAST      = make_xyz_unit( 1, -1,  0 ),
    BELOWNORTHEAST = make_xyz_unit( 1, -1,  1 ),

    ABOVEWEST      = make_xyz_unit( -1,  0, -1 ),
    WEST           = make_xyz_unit( -1,  0,  0 ),
    BELOWWEST      = make_xyz_unit( -1,  0,  1 ),
    ABOVECENTER    = make_xyz_unit( 0,  0, -1 ),
    CENTER         = make_xyz_unit( 0,  0,  0 ),
    BELOWCENTER    = make_xyz_unit( 0,  0,  1 ),
    ABOVEEAST      = make_xyz_unit( 1,  0, -1 ),
    EAST           = make_xyz_unit( 1,  0,  0 ),
    BELOWEAST      = make_xyz_unit( 1,  0,  1 ),

    ABOVESOUTHWEST = make_xyz_unit( -1,  1, -1 ),
    SOUTHWEST      = make_xyz_unit( -1,  1,  0 ),
    BELOWSOUTHWEST = make_xyz_unit( -1,  1,  1 ),
    ABOVESOUTH     = make_xyz_unit( 0,  1, -1 ),
    SOUTH          = make_xyz_unit( 0,  1,  0 ),
    BELOWSOUTH     = make_xyz_unit( 0,  1,  1 ),
    ABOVESOUTHEAST = make_xyz_unit( 1,  1, -1 ),
    SOUTHEAST      = make_xyz_unit( 1,  1,  0 ),
    BELOWSOUTHEAST = make_xyz_unit( 1,  1,  1 ),
};

direction direction_from( const point &p ) noexcept;
direction direction_from( const tripoint &p ) noexcept;
direction direction_from( int x1, int y1, int x2, int y2 ) noexcept;
direction direction_from( const point &p1, int x2, int y2 ) noexcept;
direction direction_from( const point &p1, const point &p2 ) noexcept;
direction direction_from( const tripoint &p, const tripoint &q );

point direction_XY( direction dir );
std::string direction_name( direction dir );
std::string direction_name_short( direction dir );

/* Get suffix describing vector from p to q (e.g. 1NW, 2SE) or empty string if p == q */
std::string direction_suffix( const tripoint &p, const tripoint &q );

/**
 * The actual Bresenham algorithm in 2D and 3D, everything else should call these
 * and pass in an interact functor to iterate across a line between two points.
 */
void bresenham( int x1, int y1, int x2, int y2, int t,
                const std::function<bool( const point & )> &interact );
void bresenham( const point &p1, int x2, int y2, int t,
                const std::function<bool( const point & )> &interact );
void bresenham( const point &p1, const point &p2, int t,
                const std::function<bool( const point & )> &interact );
void bresenham( const tripoint &loc1, const tripoint &loc2, int t, int t2,
                const std::function<bool( const tripoint & )> &interact );

tripoint move_along_line( const tripoint &loc, const std::vector<tripoint> &line,
                          int distance );
// The "t" value decides WHICH Bresenham line is used.
std::vector<point> line_to( int x1, int y1, int x2, int y2, int t = 0 );
std::vector<point> line_to( const point &p1, int x2, int y2, int t = 0 );
std::vector<point> line_to( const point &p1, const point &p2, int t = 0 );
// t and t2 decide which Bresenham line is used.
std::vector<tripoint> line_to( const tripoint &loc1, const tripoint &loc2, int t = 0, int t2 = 0 );
// sqrt(dX^2 + dY^2)
float trig_dist( int x1, int y1, int x2, int y2 );
float trig_dist( const point &p1, int x2, int y2 );
float trig_dist( const point &loc1, const point &loc2 );
float trig_dist( const tripoint &loc1, const tripoint &loc2 );
// Roguelike distance; minimum of dX and dY
int square_dist( int x1, int y1, int x2, int y2 );
int square_dist( const point &p1, int x2, int y2 );
int square_dist( const point &loc1, const point &loc2 );
int square_dist( const tripoint &loc1, const tripoint &loc2 );
int rl_dist( int x1, int y1, int x2, int y2 );
int rl_dist( const point &p1, int x2, int y2 );
int rl_dist( const tripoint &loc1, const tripoint &loc2 );
int rl_dist( const point &a, const point &b );
// Sum of distance in both axes
int manhattan_dist( const point &loc1, const point &loc2 );

// get angle of direction represented by point (in radians or degrees)
double atan2( const point & );
double atan2_degrees( const point & );

// Get the magnitude of the slope ranging from 0.0 to 1.0
float get_normalized_angle( const point &start, const point &end );
std::vector<tripoint> continue_line( const std::vector<tripoint> &line, int distance );
std::vector<point> squares_in_direction( int x1, int y1, int x2, int y2 );
std::vector<point> squares_in_direction( const point &p1, int x2, int y2 );
std::vector<point> squares_in_direction( const point &p1, const point &p2 );
// Returns a vector of squares adjacent to @from that are closer to @to than @from is.
// Currently limited to the same z-level as @from.
std::vector<tripoint> squares_closer_to( const tripoint &from, const tripoint &to );
void calc_ray_end( int angle, int range, const tripoint &p, tripoint &out );
/**
 * Calculates the horizontal angle between the lines from (0,0,0) to @p a and
 * the line from (0,0,0) to @p b.
 * Returned value is in degree and in the range 0....360.
 * Example: if @p a is (0,1) and @p b is (1,0), the result will 90 degree
 * The function currently ignores the z component.
 */
double coord_to_angle( const tripoint &a, const tripoint &b );

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
    explicit rl_vec3d( const tripoint &p ) : x( p.x ), y( p.y ), z( p.z ) {}

    float magnitude() const;
    rl_vec3d normalized() const;
    rl_vec3d rotated( float angle ) const;
    float dot_product( const rl_vec3d &v ) const;
    bool is_null() const;

    tripoint as_point() const;

    // scale.
    rl_vec3d operator* ( float rhs ) const;
    rl_vec3d operator/ ( float rhs ) const;
    // subtract
    rl_vec3d operator- ( const rl_vec3d &rhs ) const;
    // unary negation
    rl_vec3d operator- () const;
    rl_vec3d operator+ ( const rl_vec3d &rhs ) const;
};

#endif
