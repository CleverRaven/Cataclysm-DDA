#ifndef LINE_H
#define LINE_H

#include <vector>
#include <string>
#include "enums.h"
#include <math.h>

enum direction {
    NORTH = 0,
    NORTHEAST,
    EAST,
    SOUTHEAST,
    SOUTH,
    SOUTHWEST,
    WEST,
    NORTHWEST,
    ABOVENORTH,
    ABOVENORTHEAST,
    ABOVEEAST,
    ABOVESOUTHEAST,
    ABOVESOUTH,
    ABOVESOUTHWEST,
    ABOVEWEST,
    ABOVENORTHWEST,
    BELOWNORTH,
    BELOWNORTHEAST,
    BELOWEAST,
    BELOWSOUTHEAST,
    BELOWSOUTH,
    BELOWSOUTHWEST,
    BELOWWEST,
    BELOWNORTHWEST,
    CENTER,
};

// The "t" value decides WHICH Bresenham line is used.
std::vector <point> line_to(const int x1, const int y1, const int x2, const int y2, int t);
std::vector <point> line_to( point p1, point p2, int t );
// t and t2 decide which Bresenham line is used.
std::vector <tripoint> line_to(const tripoint loc1, const tripoint loc2, int t, int t2);
// sqrt(dX^2 + dY^2)
int trig_dist(const int x1, const int y1, const int x2, const int y2);
int trig_dist(const tripoint loc1, const tripoint loc2);
// Roguelike distance; minimum of dX and dY
int square_dist(const int x1, const int y1, const int x2, const int y2);
int square_dist(const tripoint loc1, const tripoint loc2);
int rl_dist(const int x1, const int y1, const int x2, const int y2);
int rl_dist(const tripoint loc1, const tripoint loc2);
int rl_dist(point a, point b);
std::pair<double, double> slope_of(const std::vector<point> &line);
std::pair<std::pair<double, double>, double> slope_of(const std::vector<tripoint> &line);
std::vector<point> continue_line(const std::vector<point> &line, const int distance);
std::vector<tripoint> continue_line(const std::vector<tripoint> &line, const int distance);
direction direction_from(const int x1, const int y1, const int x2, const int y2);
direction direction_from(const tripoint loc1, const tripoint loc2);
point direction_XY(direction dir);
std::string direction_name(direction dir);
std::string direction_name_short(direction dir);
std::vector<point> squares_in_direction( const int x1, const int y1, const int x2, const int y2 );

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
    rl_vec2d operator* (const float rhs);
    rl_vec2d operator/ (const float rhs);
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
    rl_vec3d operator* (const float rhs);
    rl_vec3d operator/ (const float rhs);
    // subtract
    rl_vec3d operator- (const rl_vec3d &rhs);
    // unary negation
    rl_vec3d operator- ();
    rl_vec3d operator+ (const rl_vec3d &rhs);
};

#endif
