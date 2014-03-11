#include "line.h"
#include "game.h"
#include <stdlib.h>
#include <tuple>

#define SGN(a) (((a)<0) ? -1 : 1)

//Trying to pull points out of a tripoint vector is messy and 
//probably slow, so leaving two full functions for now
//note that there is not currently a notion of "which" B-line
//is used anymore, line_to will return the same line every time.
//90+% of callers default to zero anyways.  Haven't yet tested this.
std::vector <point> line_to(const int x1, const int y1, const int x2, const int y2, int t)
{
    std::vector<point> ret;
    // Preallocate the number of cells we need instead of allocating them piecewise.
    const int j = square_dist(tripoint(x1, y1, 0),tripoint(x2,y2,0)); //j = points in line
    ret.reserve(j);
	point cur= (x1,y1);
    int dx = x2 - x1, dy = y2 - y1;
    for (int i = 1; i != (j + 1); i++) {
        double k = i/j; //This is how far along the vector we are
        cur.x = x1 + (ceil(dx*k) * sgn(dx)); // these operations may be affected by
        cur.y = y1 + (ceil(dy*k) * sgn(dx)); // floating-point representational errors
        // (1 != 1 in dbl-land), haven't checked.
        ret.push_back(cur);
    }
    return ret;
}

std::vector <tripoint> line_to(const tripoint loc1, const tripoint loc2, int t)
{
    std::vector<tripoint> ret;
    // Preallocate the number of cells we need instead of allocating them piecewise.
	const int j = square_dist(loc1, loc2); //j = points in line
    ret.reserve(j);
	tripoint cur= loc1;
    int dx = loc2.x - loc1.x, dy = loc2.y - loc1.y, dz = loc2.z - loc1.z;
    for (int i = 1; i != (j + 1); i++) {
        double k = i/j; //This is how far along the vector we are
        cur.x = loc1.x + ceil(dx*k) * sgn(dx); // these operations may be affected by
        cur.y = loc1.y + ceil(dy*k) * sgn(dx); // floating-point representational errors
        cur.z = loc1.z + ceil(dz*k) * sgn(dx); // (1 != 1 in dbl-land), haven't checked.
        ret.push_back(cur);
    }
    return ret;
}

int trig_dist(const int x1, const int y1, const int x2, const int y2)
{
    return trig_dist(tripoint(x1,y1,0),tripoint(x2,y2,0));
}

int trig_dist(const tripoint loc1, const tripoint loc2)
{
    return int( sqrt( double( pow((loc1.x - loc2.x), 2) + pow((loc1.y - loc2.y), 2) + pow((loc1.z - loc2.z), 2))));
}

int square_dist(const int x1, const int y1, const int x2, const int y2)
{
    return square_dist(tripoint(x1, y1, 0), tripoint(x2, y2, 0));
}

int square_dist(const tripoint loc1, const tripoint loc2)
{
    const int dx = abs(loc1.x - loc2.x), dy = abs(loc1.y - loc2.y), dz = abs(loc1.z - loc2.z);
    int maxDxDy = (dx > dy ? dx : dy); // Sloppy, but should be quick.
    return (maxDxDy > dz ? maxDxDy : dz); // Too bad it doesn't scale.
}

int rl_dist(const int x1, const int y1, const int x2, const int y2)
{
    return rl_dist(tripoint(x1,x2,0), tripoint (x2,y2,0));
}

int rl_dist(const tripoint loc1, const tripoint loc2)
{
    if(trigdist) {
        return trig_dist(loc1, loc2);
    }
    return square_dist(loc1, loc2);
}

int rl_dist(point a, point b)
{
    return rl_dist(tripoint(a.x,a.y,0),tripoint(b.x, b.y, 0));
}

double slope_of(const std::vector<point> &line)
{
	double dX = line.back().x - line.front().x,
           dY = line.back().y - line.front().y;
    return (dX == 0 ? SLOPE_VERTICAL : (dY / dX));
}

// returns the normalized dx, dy, dz for the current line vector
std::tuple<double, double, double> slope_of(const std::vector<tripoint> &line)
{
    int len = line.size();
	double normDx = (line.back().x - line.front().x) / len,
           normDy = (line.back().y - line.front().y) / len,
           normDz = (line.back().z - line.front().z) / len;	
	auto ret = std::make_tuple(normDx, normDy, normDz); // slope in x, y, z
    return ret;
}

std::vector<point> continue_line(const std::vector<point> &line, const int distance)
{
    point start = line.back(), end = line.back();
    double slope = slope_of(line);
    int sX = (line.front().x < line.back().x ? 1 : -1),
        sY = (line.front().y < line.back().y ? 1 : -1);
    if (abs(slope) == 1) {
        end.x += distance * sX;
        end.y += distance * sY;
    } else if (abs(slope) < 1) {
        end.x += distance * sX;
        end.y += int(distance * abs(slope) * sY);
    } else {
        end.y += distance * sY;
        if (slope != SLOPE_VERTICAL) {
            end.x += int(distance / abs(slope)) * sX;
        }
    }
    return line_to(start.x, start.y, end.x, end.y, 0);
} 

std::vector<tripoint> continue_line(const std::vector<tripoint> &line, const int distance)
{
    tripoint start = line.back(), end = line.back();
    std::tuple<double, double, double> slope = slope_of(line);
    end.x += distance * std::get<0>(slope);
	end.y += distance * std::get<1>(slope);
	end.z += distance * std::get<2>(slope);	
    return line_to(start, end, 0);
}

direction direction_from(int x1, int y1, int x2, int y2)
{
    return direction_from(tripoint(x1, y1, 0), tripoint(x2, y2, 0));
}

direction direction_from(const tripoint loc1, const tripoint loc2)
{
    int dx = loc2.x - loc1.x;
    int dy = loc2.y - loc1.y;
	int dz = loc2.z - loc1.z;
	int offset =  (dz = 0 ? 0 : (12 + 2 * sgn(dz))); // blergh.  one-liner to set us +8 or +16 in the enum
	direction offsetenum = direction(offset);
    if (dx < 0) {
        if (abs(dx) / 2 > abs(dy) || dy == 0) {
            return (WEST);
        } else if (abs(dy) / 2 > abs(dx)) {
            if (dy < 0) {
                return (NORTH);
            } else {
                return (SOUTH);
            }
        } else {
            if (dy < 0) {
                return (NORTHWEST);
            } else {
                return (SOUTHWEST);
            }
        }
    } else {
        if (dx / 2 > abs(dy) || dy == 0) {
            return EAST;
        } else if (abs(dy) / 2 > dx || dx == 0) {
            if (dy < 0) {
                return (NORTH);
            } else {
                return (SOUTH);
            }
        } else {
            if (dy < 0) {
                return (NORTHEAST);
            } else {
                return (SOUTHEAST);
            }
        }
    }
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
    }
    return "BUG. (line.cpp:direction_name)";
}

std::string direction_name_short(direction dir)
{
    switch (dir) {
        //~ abbreviated direction names
    case NORTH:
        return _("N ");
    case NORTHEAST:
        return _("NE");
    case EAST:
        return _("E ");
    case SOUTHEAST:
        return _("SE");
    case SOUTH:
        return _("S ");
    case SOUTHWEST:
        return _("SW");
    case WEST:
        return _("W ");
    case NORTHWEST:
        return _("NW");
    case ABOVENORTH:
        return _("UP_N");
    case ABOVENORTHEAST:
        return _("UP_NE");
    case ABOVEEAST:
        return _("UP_E");
    case ABOVESOUTHEAST:
        return _("UP_SE");
    case ABOVESOUTH:
        return _("UP_S");
    case ABOVESOUTHWEST:
        return _("UP_SW");
    case ABOVEWEST:
        return _("UP_W");
    case ABOVENORTHWEST:
        return _("DN_NW");
    case BELOWNORTH:
        return _("DN_N");
    case BELOWNORTHEAST:
        return _("DN_NE");
    case BELOWEAST:
        return _("DN_E");
    case BELOWSOUTHEAST:
        return _("DN_SE");
    case BELOWSOUTH:
        return _("DN_S");
    case BELOWSOUTHWEST:
        return _("DN_SW");
    case BELOWWEST:
        return _("DN_W");
    case BELOWNORTHWEST:
        return _("DN_NW");
    }
    return "Bug. (line.cpp:direction_name_short)";
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