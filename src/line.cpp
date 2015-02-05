#include "line.h"
#include "game.h"
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
    tripoint cur;
    cur = loc1;
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
    tripoint start;
    tripoint end;
    start = line.back();
    end = line.back();
    // slope <<x,y>,z>
    std::pair<std::pair<double, double>, double> slope;
    slope = slope_of(line);
    end.x += int(distance * slope.first.first);
    end.y += int(distance * slope.first.second);
    end.z += int(distance * slope.second);
    return line_to(start, end, 0, 0);
}

direction direction_from(int x1, int y1, int x2, int y2)
{
    return direction_from(tripoint(x1, y1, 0), tripoint(x2, y2, 0));
}

direction direction_from(const tripoint &loc1, const tripoint &loc2)
{
    int dx = loc2.x - loc1.x;
    int dy = loc2.y - loc1.y;
    int dz = loc2.z - loc1.z;
    // offset returns 0, 8, or 16 to put us in "above" or "below" range
    int offset =  (dz == 0 ? 0 : (12 + (sgn(dz) * 2)));
    if (dx < 0) {
        if (abs(dx) / 2 > abs(dy) || dy == 0) {
            return direction(6 + offset); //West
        } else if (abs(dy) / 2 > abs(dx)) {
            if (dy < 0) {
                return direction(0 + offset); //North
            } else {
                return direction(4 + offset); //South
            }
        } else {
            if (dy < 0) {
                return direction(7 + offset); //Northwest
            } else {
                return direction(5 + offset); //Southwest
            }
        }
    } else {
        if (dx / 2 > abs(dy) || dy == 0) {
            return direction(2 + offset); //East
        } else if (abs(dy) / 2 > dx || dx == 0) {
            if (dy < 0) {
                return direction(0 + offset); //North
            } else {
                return direction(4 + offset); //South
            }
        } else {
            if (dy < 0) {
                return direction(1 + offset); //Northeast
            } else {
                return direction(3 + offset); //Southeast
            }
        }
    }
}

point direction_XY(direction dir)
{
    switch((dir != CENTER) ? dir % 8 : dir) {
    case NORTH:
        return point(0, -1);

    case NORTHEAST:
        return point(1, -1);

    case EAST:
        return point(1, 0);

    case SOUTHEAST:
        return point(1, 1);

    case SOUTH:
        return point(0, 1);

    case SOUTHWEST:
        return point(-1, 1);

    case WEST:
        return point(-1, 0);

    case NORTHWEST:
        return point(-1, -1);

    case CENTER:
        return point(0, 0);

    default:
        break;
    }

    return point(999, 999);
}

std::string direction_name(direction dir)
{
    switch (dir) {
    //~ used for "to the north" etc
    case NORTH:
        return _("north");
    case NORTHEAST:
        return _("northeast");
    case EAST:
        return _("east");
    case SOUTHEAST:
        return _("southeast");
    case SOUTH:
        return _("south");
    case SOUTHWEST:
        return _("southwest");
    case WEST:
        return _("west");
    case NORTHWEST:
        return _("northwest");
    case ABOVENORTH:
        return _("north and above");
    case ABOVENORTHEAST:
        return _("northeast and above");
    case ABOVEEAST:
        return _("east and above");
    case ABOVESOUTHEAST:
        return _("southeast and above");
    case ABOVESOUTH:
        return _("south and above");
    case ABOVESOUTHWEST:
        return _("southwest and above");
    case ABOVEWEST:
        return _("west and above");
    case ABOVENORTHWEST:
        return _("northwest and above");
    case BELOWNORTH:
        return _("north and below");
    case BELOWNORTHEAST:
        return _("northeast and below");
    case BELOWEAST:
        return _("east and below");
    case BELOWSOUTHEAST:
        return _("southeast and below");
    case BELOWSOUTH:
        return _("south and below");
    case BELOWSOUTHWEST:
        return _("southwest and below");
    case BELOWWEST:
        return _("west and below");
    case BELOWNORTHWEST:
        return _("northwest and below");
    case CENTER:
        return _("center");
    }
    return "BUG. (line.cpp:direction_name)";
}

std::string direction_name_short(direction dir)
{
    switch (dir) {
    //~ abbreviated direction names
    case NORTH:
        return _("N    ");
    case NORTHEAST:
        return _("NE   ");
    case EAST:
        return _("E    ");
    case SOUTHEAST:
        return _("SE   ");
    case SOUTH:
        return _("S    ");
    case SOUTHWEST:
        return _("SW   ");
    case WEST:
        return _("W    ");
    case NORTHWEST:
        return _("NW   ");
    case ABOVENORTH:
        return _("UP_N ");
    case ABOVENORTHEAST:
        return _("UP_NE");
    case ABOVEEAST:
        return _("UP_E ");
    case ABOVESOUTHEAST:
        return _("UP_SE");
    case ABOVESOUTH:
        return _("UP_S ");
    case ABOVESOUTHWEST:
        return _("UP_SW");
    case ABOVEWEST:
        return _("UP_W ");
    case ABOVENORTHWEST:
        return _("UP_NW");
    case BELOWNORTH:
        return _("DN_N ");
    case BELOWNORTHEAST:
        return _("DN_NE");
    case BELOWEAST:
        return _("DN_E ");
    case BELOWSOUTHEAST:
        return _("DN_SE");
    case BELOWSOUTH:
        return _("DN_S ");
    case BELOWSOUTHWEST:
        return _("DN_SW");
    case BELOWWEST:
        return _("DN_W ");
    case BELOWNORTHWEST:
        return _("DN_NW");
    case CENTER:
        return _("CE");
    }
    return "Bug. (line.cpp:direction_name_short)";
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
