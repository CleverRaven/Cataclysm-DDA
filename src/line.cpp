#include "line.h"
#include "game.h"
#include "translations.h"
#include <stdlib.h>

#define SGN(a) (((a)<0) ? -1 : 1)

//Trying to pull points out of a tripoint vector is messy and
//probably slow, so leaving two full functions for now
std::vector <point> line_to(const int x1, const int y1, const int x2, const int y2, int t)
{
    std::vector<point> ret;
    // Preallocate the number of cells we need instead of allocating them piecewise.
    const int numCells = square_dist(tripoint(x1, y1, 0), tripoint(x2, y2, 0));
    ret.reserve(numCells);
    const int dx = x2 - x1;
    const int dy = y2 - y1;

    point cur;
    cur.x = x1;
    cur.y = y1;

    // Draw point
    if (dx == 0 && dy == 0) {
        ret.push_back(cur);
        // Should exit here
        return ret;
    }

    // Any ideas why we're multiplying the abs distance by two here?
    const int ax = abs(dx) * 2;
    const int ay = abs(dy) * 2;
    const int sx = (dx == 0 ? 0 : SGN(dx)), sy = (dy == 0 ? 0 : SGN(dy));

    // The old version of this algorithm would generate points on the line and check min/max for each point
    // to determine whether or not to continue generating the line. Since we already know how many points
    // we need, this method saves us a half-dozen variables and a few calculations.
    if (ax == ay) {
        for (int i = 0; i < numCells; i++) {
            cur.y += sy;
            cur.x += sx;
            ret.push_back(cur);
        } ;
    } else if (ax > ay) {
        for (int i = 0; i < numCells; i++) {
            if (t > 0) {
                cur.y += sy;
                t -= ax;
            }
            cur.x += sx;
            t += ay;
            ret.push_back(cur);
        } ;
    } else {
        for (int i = 0; i < numCells; i++) {
            if (t > 0) {
                cur.x += sx;
                t -= ay;
            }
            cur.y += sy;
            t += ax;
            ret.push_back(cur);
        } ;
    }
    return ret;
}

std::vector<point> line_to( const point &p1, const point &p2, const int t )
{
    return line_to( p1.x, p1.y, p2.x, p2.y, t );
}

std::vector <tripoint> line_to(const tripoint &loc1, const tripoint &loc2, int t, int t2)
{
    std::vector<tripoint> ret;
    // Preallocate the number of cells we need instead of allocating them piecewise.
    const int numCells = square_dist(loc1, loc2);
    ret.reserve(numCells);
    tripoint cur( loc1 );
    const int dx = loc2.x - loc1.x;
    const int dy = loc2.y - loc1.y;
    const int dz = loc2.z - loc1.z;
    // Any ideas why we're multiplying the abs distance by two here?
    const int ax = abs(dx) * 2;
    const int ay = abs(dy) * 2;
    const int az = abs(dz) * 2;
    const int sx = (dx == 0 ? 0 : SGN(dx));
    const int sy = (dy == 0 ? 0 : SGN(dy));
    const int sz = (dz == 0 ? 0 : SGN(dz));
    if (az == 0) {
        if (ax == ay) {
            for (int i = 0; i < numCells; i++) {
                cur.y += sy;
                cur.x += sx;
                ret.push_back(cur);
            } ;
        } else if (ax > ay) {
            for (int i = 0; i < numCells; i++) {
                if (t > 0) {
                    cur.y += sy;
                    t -= ax;
                }
                cur.x += sx;
                t += ay;
                ret.push_back(cur);
            } ;
        } else {
            for (int i = 0; i < numCells; i++) {
                if (t > 0) {
                    cur.x += sx;
                    t -= ay;
                }
                cur.y += sy;
                t += ax;
                ret.push_back(cur);
            } ;
        }
    } else {
        if (ax == ay && ay == az) {
            for (int i = 0; i < numCells; i++) {
                cur.z += sz;
                cur.y += sy;
                cur.x += sx;
                ret.push_back(cur);
            } ;
        } else if ((az > ax) && (az > ay)) {
            for (int i = 0; i < numCells; i++) {
                if (t > 0) {
                    cur.x += sx;
                    t -= az;
                }
                if (t2 > 0) {
                    cur.z += sz;
                    t2 -= ax;
                }
                cur.z += sz;
                t += ax;
                t2 += ay;
                ret.push_back(cur);
            } ;
        } else if (ax == ay) {
            for (int i = 0; i < numCells; i++) {
                if (t > 0) {
                    cur.z += sz;
                    t -= ax; // to clarify, ax and az are equivalent in this case
                }
                cur.y += sy;
                cur.x += sx;
                t += az;
                ret.push_back(cur);
            } ;
        } else if (ax > ay) {
            for (int i = 0; i < numCells; i++) {
                if (t > 0) {
                    cur.y += sy;
                    t -= ax;
                }
                if (t2 > 0) {
                    cur.z += sz;
                    t2 -= ax;
                }
                cur.x += sx;
                t += ay;
                t2 += az;
                ret.push_back(cur);
            } ;
        } else { //dy > dx >= dz
            for (int i = 0; i < numCells; i++) {
                if (t > 0) {
                    cur.x += sx;
                    t -= ay;
                }
                if (t2 > 0) {
                    cur.z += sz;
                    t2 -= ay;
                }
                cur.y += sy;
                t += ax;
                t2 += az;
                ret.push_back(cur);
            } ;
        }
    }
    return ret;
}

int trig_dist(const int x1, const int y1, const int x2, const int y2)
{
    return trig_dist(tripoint(x1, y1, 0), tripoint(x2, y2, 0));
}

int trig_dist(const tripoint &loc1, const tripoint &loc2)
{
    return int (sqrt(double((loc1.x - loc2.x) * (loc1.x - loc2.x)) +
                     ((loc1.y - loc2.y) * (loc1.y - loc2.y)) +
                     ((loc1.z - loc2.z) * (loc1.z - loc2.z))));
}

int square_dist(const int x1, const int y1, const int x2, const int y2)
{
    return square_dist(tripoint(x1, y1, 0), tripoint(x2, y2, 0));
}

int square_dist(const tripoint &loc1, const tripoint &loc2)
{
    const int dx = abs(loc1.x - loc2.x);
    const int dy = abs(loc1.y - loc2.y);
    const int dz = abs(loc1.z - loc2.z);
    int maxDxDy = (dx > dy ? dx : dy); // Sloppy, but should be quick.
    return (maxDxDy > dz ? maxDxDy : dz); // Too bad it doesn't scale.
}

int rl_dist(const int x1, const int y1, const int x2, const int y2)
{
    return rl_dist(tripoint(x1, y1, 0), tripoint (x2, y2, 0));
}

int rl_dist(const point &a, const point &b)
{
    return rl_dist(tripoint(a.x, a.y, 0), tripoint(b.x, b.y, 0));
}

int rl_dist(const tripoint &loc1, const tripoint &loc2)
{
    if(trigdist) {
        return trig_dist(loc1, loc2);
    }
    return square_dist(loc1, loc2);
}

// returns normalized dx and dy for the current line vector.
std::pair<double, double> slope_of(const std::vector<point> &line)
{
    const double len = line.size();
    double normDx = (line.back().x - line.front().x) / len;
    double normDy = (line.back().y - line.front().y) / len;
    std::pair<double, double> ret = std::make_pair(normDx, normDy); // slope of x, y
    return ret;
}

// returns the normalized dx, dy, dz for the current line vector.
// ret.second contains z and can be ignored if unused.
std::pair<std::pair<double, double>, double> slope_of(const std::vector<tripoint> &line)
{
    const double len = line.size();
    double normDx = (line.back().x - line.front().x) / len;
    double normDy = (line.back().y - line.front().y) / len;
    double normDz = (line.back().z - line.front().z) / len;
    std::pair<double, double> retXY = std::make_pair(normDx, normDy);
    // slope of <x, y> z
    std::pair<std::pair<double, double>, double> ret = std::make_pair(retXY, normDz);
    return ret;
}

std::vector<point> continue_line(const std::vector<point> &line, const int distance)
{
    const point start = line.back();
    point end = line.back();
    const std::pair<double, double> slope = slope_of(line);
    end.x += distance * slope.first;
    end.y += distance * slope.second;
    return line_to( start, end, 0 );
}

std::vector<tripoint> continue_line(const std::vector<tripoint> &line, const int distance)
{
    // May want to optimize this, but it's called fairly infrequently as part of specific attack
    // routines, erring on the side of readability.
    tripoint start( line.back() );
    tripoint end( line.back() );
    // slope <<x,y>,z>
    std::pair<std::pair<double, double>, double> slope;
    slope = slope_of(line);
    end.x += int(distance * slope.first.first);
    end.y += int(distance * slope.first.second);
    end.z += int(distance * slope.second);
    return line_to(start, end, 0, 0);
}

direction direction_from(int const x, int const y, int const z) noexcept
{
    return static_cast<direction>(make_xyz(x, y, z));
}

direction direction_from(int const x1, int const y1, int const x2, int const y2) noexcept
{
    return direction_from(x2 - x1, y2 - y1);
}

direction direction_from(tripoint const &p, tripoint const &q)
{
    return direction_from(q.x - p.x, q.y - p.y, q.z - p.z);
}

point direction_XY(direction const dir)
{
    switch (dir % 9) {
    case NORTHWEST:  return point(-1, -1);
    case NORTH:      return point( 0, -1);
    case NORTHEAST:  return point( 1, -1);
    case WEST:       return point(-1,  0);
    case CENTER:     return point( 0,  0);
    case EAST:       return point( 1,  0);
    case SOUTHWEST:  return point(-1,  1);
    case SOUTH:      return point( 0,  1);
    case SOUTHEAST:  return point( 1,  1);
    }

    return point(0, 0);
}

namespace {
std::string const& direction_name_impl(direction const dir, bool const short_name)
{
    enum : int { size = 3*3*3 };
    static auto const names = [] {
        using pair_t = std::pair<std::string, std::string>;
        std::array<pair_t, size + 1> result;

        //~ abbreviated direction names and long direction names
        result[NORTH]          = pair_t {_("N    "), _("north")};
        result[NORTHEAST]      = pair_t {_("NE   "), _("northeast")};
        result[EAST]           = pair_t {_("E    "), _("east")};
        result[SOUTHEAST]      = pair_t {_("SE   "), _("southeast")};
        result[SOUTH]          = pair_t {_("S    "), _("south")};
        result[SOUTHWEST]      = pair_t {_("SW   "), _("southwest")};
        result[WEST]           = pair_t {_("W    "), _("west")};
        result[NORTHWEST]      = pair_t {_("NW   "), _("northwest")};
        result[ABOVENORTH]     = pair_t {_("UP_N "), _("north and above")};
        result[ABOVENORTHEAST] = pair_t {_("UP_NE"), _("northeast and above")};
        result[ABOVEEAST]      = pair_t {_("UP_E "), _("east and above")};
        result[ABOVESOUTHEAST] = pair_t {_("UP_SE"), _("southeast and above")};
        result[ABOVESOUTH]     = pair_t {_("UP_S "), _("south and above")};
        result[ABOVESOUTHWEST] = pair_t {_("UP_SW"), _("southwest and above")};
        result[ABOVEWEST]      = pair_t {_("UP_W "), _("west and above")};
        result[ABOVENORTHWEST] = pair_t {_("UP_NW"), _("northwest and above")};
        result[BELOWNORTH]     = pair_t {_("DN_N "), _("north and below")};
        result[BELOWNORTHEAST] = pair_t {_("DN_NE"), _("northeast and below")};
        result[BELOWEAST]      = pair_t {_("DN_E "), _("east and below")};
        result[BELOWSOUTHEAST] = pair_t {_("DN_SE"), _("southeast and below")};
        result[BELOWSOUTH]     = pair_t {_("DN_S "), _("south and below")};
        result[BELOWSOUTHWEST] = pair_t {_("DN_SW"), _("southwest and below")};
        result[BELOWWEST]      = pair_t {_("DN_W "), _("west and below")};
        result[BELOWNORTHWEST] = pair_t {_("DN_NW"), _("northwest and below")};
        result[ABOVECENTER]    = pair_t {_("UP_CE"), _("center and above")};
        result[CENTER]         = pair_t {_("CE   "), _("center")};
        result[BELOWCENTER]    = pair_t {_("DN_CE"), _("center and below")};

        result[size] = pair_t {"BUG. (line.cpp:direction_name)", "BUG. (line.cpp:direction_name)"};
        return result;
    }();

    auto i = static_cast<int>(dir);
    if (i < 0 || i >= size) {
        i = size;
    }

    return short_name ? names[i].first : names[i].second;
}
} //namespace

std::string const& direction_name(direction const dir)
{
    return direction_name_impl(dir, false);
}

std::string const& direction_name_short(direction const dir)
{
    return direction_name_impl(dir, true);
}

// Returns a vector of the adjacent square in the direction of the target,
// and the two squares flanking it.
std::vector<point> squares_in_direction( const int x1, const int y1, const int x2, const int y2 )
{
    int junk = 0;
    point center_square = line_to( x1, y1, x2, y2, junk )[0];
    std::vector<point> adjacent_squares;
    adjacent_squares.push_back( center_square );
    if( x1 == center_square.x ) {
        // Horizontally adjacent.
        adjacent_squares.push_back( point( x1 + 1, center_square.y ) );
        adjacent_squares.push_back( point( x1 - 1, center_square.y ) );
    } else if( y1 == center_square.y ) {
        // Vertically adjacent.
        adjacent_squares.push_back( point( center_square.x, y1 + 1 ) );
        adjacent_squares.push_back( point( center_square.x, y1 - 1 ) );
    } else {
        // Diagonally adjacent.
        adjacent_squares.push_back( point( x1, center_square.y ) );
        adjacent_squares.push_back( point( center_square.x, y1 ) );
    }
    return adjacent_squares;
}

float rl_vec2d::norm()
{
    return sqrt(x * x + y * y);
}

float rl_vec3d::norm()
{
    return sqrt(x * x + y * y + z * z);
}

rl_vec2d rl_vec2d::normalized()
{
    rl_vec2d ret;
    if (is_null()) { // shouldn't happen?
        ret.x = ret.y = 1;
        return ret;
    }
    float n = norm();
    ret.x = x / n;
    ret.y = y / n;
    return ret;
}

rl_vec3d rl_vec3d::normalized()
{
    rl_vec3d ret;
    if (is_null()) { // shouldn't happen?
        ret.x = ret.y = ret.z = 1;
        return ret;
    }
    float n = norm();
    ret.x = x / n;
    ret.y = y / n;
    ret.z = z / n;
    return ret;
}

rl_vec2d rl_vec2d::get_vertical()
{
    rl_vec2d ret;
    ret.x = -y;
    ret.y = x;
    return ret;
}

rl_vec3d rl_vec3d::get_vertical()
{
    rl_vec3d ret;
    ret.x = -y;
    ret.y = x;
    ret.z = z;
    return ret;
}

float rl_vec2d::dot_product (rl_vec2d &v)
{
    float dot = x * v.x + y * v.y;
    return dot;
}

float rl_vec3d::dot_product (rl_vec3d &v)
{
    float dot = x * v.x + y * v.y + y * v.z;
    return dot;
}

bool rl_vec2d::is_null()
{
    return !(x || y);
}

bool rl_vec3d::is_null()
{
    return !(x || y || z);
}

// scale.
rl_vec2d rl_vec2d::operator* (const float rhs)
{
    rl_vec2d ret;
    ret.x = x * rhs;
    ret.y = y * rhs;
    return ret;
}

rl_vec3d rl_vec3d::operator* (const float rhs)
{
    rl_vec3d ret;
    ret.x = x * rhs;
    ret.y = y * rhs;
    ret.z = z * rhs;
    return ret;
}

// subtract
rl_vec2d rl_vec2d::operator- (const rl_vec2d &rhs)
{
    rl_vec2d ret;
    ret.x = x - rhs.x;
    ret.y = y - rhs.y;
    return ret;
}

rl_vec3d rl_vec3d::operator- (const rl_vec3d &rhs)
{
    rl_vec3d ret;
    ret.x = x - rhs.x;
    ret.y = y - rhs.y;
    ret.z = z - rhs.z;
    return ret;
}

// unary negation
rl_vec2d rl_vec2d::operator- ()
{
    rl_vec2d ret;
    ret.x = -x;
    ret.y = -y;
    return ret;
}

rl_vec3d rl_vec3d::operator- ()
{
    rl_vec3d ret;
    ret.x = -x;
    ret.y = -y;
    ret.z = -z;
    return ret;
}

rl_vec2d rl_vec2d::operator+ (const rl_vec2d &rhs)
{
    rl_vec2d ret;
    ret.x = x + rhs.x;
    ret.y = y + rhs.y;
    return ret;
}

rl_vec3d rl_vec3d::operator+ (const rl_vec3d &rhs)
{
    rl_vec3d ret;
    ret.x = x + rhs.x;
    ret.y = y + rhs.y;
    ret.z = z + rhs.z;
    return ret;
}

rl_vec2d rl_vec2d::operator/ (const float rhs)
{
    rl_vec2d ret;
    ret.x = x / rhs;
    ret.y = y / rhs;
    return ret;
}

rl_vec3d rl_vec3d::operator/ (const float rhs)
{
    rl_vec3d ret;
    ret.x = x / rhs;
    ret.y = y / rhs;
    ret.z = z / rhs;
    return ret;
}
