#ifndef LINE_H
#define LINE_H

#include <vector>
#include <string>
#include "enums.h"
#include <math.h>

//! This compile-time useable function combines the sign of each (x, y, z) component into a single integer
//! to allow simple runtime and compiletime mapping of (x, y, z) tuples to @ref direction enumerators.
//! Specifically, (0, -, +) => (0, 1, 2); a base-3 number.
//! This only works correctly for inputs between -1,-1,-1 and 1,1,1.
//! For numbers outside that range, use make_xyz().
inline constexpr unsigned make_xyz_unit(int const x, int const y, int const z) noexcept
{
  return ((x > 0) ? 2u : (x < 0) ? 1u : 0u) * 1u +
         ((y > 0) ? 2u : (y < 0) ? 1u : 0u) * 3u +
         ((z > 0) ? 2u : (z < 0) ? 1u : 0u) * 9u;
}

// This more general version of this function gives correct values for larger inputs.
unsigned make_xyz(int const x, int const y, int const z);

enum direction : int {
    ABOVENORTHWEST = make_xyz_unit(-1, -1, -1),
    NORTHWEST      = make_xyz_unit(-1, -1,  0),
    BELOWNORTHWEST = make_xyz_unit(-1, -1,  1),
    ABOVENORTH     = make_xyz_unit( 0, -1, -1),
    NORTH          = make_xyz_unit( 0, -1,  0),
    BELOWNORTH     = make_xyz_unit( 0, -1,  1),
    ABOVENORTHEAST = make_xyz_unit( 1, -1, -1),
    NORTHEAST      = make_xyz_unit( 1, -1,  0),
    BELOWNORTHEAST = make_xyz_unit( 1, -1,  1),

    ABOVEWEST      = make_xyz_unit(-1,  0, -1),
    WEST           = make_xyz_unit(-1,  0,  0),
    BELOWWEST      = make_xyz_unit(-1,  0,  1),
    ABOVECENTER    = make_xyz_unit( 0,  0, -1),
    CENTER         = make_xyz_unit( 0,  0,  0),
    BELOWCENTER    = make_xyz_unit( 0,  0,  1),
    ABOVEEAST      = make_xyz_unit( 1,  0, -1),
    EAST           = make_xyz_unit( 1,  0,  0),
    BELOWEAST      = make_xyz_unit( 1,  0,  1),

    ABOVESOUTHWEST = make_xyz_unit(-1,  1, -1),
    SOUTHWEST      = make_xyz_unit(-1,  1,  0),
    BELOWSOUTHWEST = make_xyz_unit(-1,  1,  1),
    ABOVESOUTH     = make_xyz_unit( 0,  1, -1),
    SOUTH          = make_xyz_unit( 0,  1,  0),
    BELOWSOUTH     = make_xyz_unit( 0,  1,  1),
    ABOVESOUTHEAST = make_xyz_unit( 1,  1, -1),
    SOUTHEAST      = make_xyz_unit( 1,  1,  0),
    BELOWSOUTHEAST = make_xyz_unit( 1,  1,  1),
};

direction direction_from(int x, int y, int z = 0) noexcept;
direction direction_from(int x1, int y1, int x2, int y2) noexcept;
direction direction_from(tripoint const &p, tripoint const &q);

point direction_XY(direction dir);
std::string const& direction_name(direction dir);
std::string const& direction_name_short(direction dir);

// The "t" value decides WHICH Bresenham line is used.
std::vector<point> line_to(int x1, int y1, int x2, int y2, int t);
std::vector<point> line_to( const point &p1, const point &p2, int t );
// t and t2 decide which Bresenham line is used.
std::vector<tripoint> line_to(const tripoint &loc1, const tripoint &loc2, int t, int t2);
// sqrt(dX^2 + dY^2)
int trig_dist(int x1, int y1, int x2, int y2);
int trig_dist(const tripoint &loc1, const tripoint &loc2);
// Roguelike distance; minimum of dX and dY
int square_dist(int x1, int y1, int x2, int y2);
int square_dist(const tripoint &loc1, const tripoint &loc2);
int rl_dist(int x1, int y1, int x2, int y2);
int rl_dist(const tripoint &loc1, const tripoint &loc2);
int rl_dist(const point &a, const point &b);
std::pair<double, double> slope_of(const std::vector<point> &line);
std::pair<std::pair<double, double>, double> slope_of(const std::vector<tripoint> &line);
std::vector<point> continue_line(const std::vector<point> &line, int distance);
std::vector<tripoint> continue_line(const std::vector<tripoint> &line, int distance);
std::vector<point> squares_in_direction( int x1, int y1, int x2, int y2 );

// weird class for 2d vectors where dist is derived from rl_dist
struct rl_vec2d {
    float x;
    float y;

    // vec2d(){}
    rl_vec2d(float X = 0, float Y = 0) : x (X), y (Y) {}
    rl_vec2d(const rl_vec2d &v) : x (v.x), y (v.y) {}
    ~rl_vec2d() {}

    float norm();
    rl_vec2d normalized();
    rl_vec2d get_vertical();
    float dot_product (rl_vec2d &v);
    bool is_null();
    // scale.
    rl_vec2d operator* (float rhs);
    rl_vec2d operator/ (float rhs);
    // subtract
    rl_vec2d operator- (const rl_vec2d &rhs);
    // unary negation
    rl_vec2d operator- ();
    rl_vec2d operator+ (const rl_vec2d &rhs);
};

struct rl_vec3d {
    float x;
    float y;
    float z;

    rl_vec3d(float X = 0, float Y = 0, float Z = 0) : x (X), y (Y), z (Z) {}
    rl_vec3d(const rl_vec3d &v) : x (v.x), y (v.y), z (v.z) {}
    ~rl_vec3d() {}

    float norm();
    rl_vec3d normalized();
    rl_vec3d get_vertical();
    float dot_product (rl_vec3d &v);
    bool is_null();
    // scale.
    rl_vec3d operator* (float rhs);
    rl_vec3d operator/ (float rhs);
    // subtract
    rl_vec3d operator- (const rl_vec3d &rhs);
    // unary negation
    rl_vec3d operator- ();
    rl_vec3d operator+ (const rl_vec3d &rhs);
};

#endif
