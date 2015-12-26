#include "line.h"
#include "game.h"
#include "translations.h"
#include <cstdlib>

#define SGN(a) (((a)<0) ? -1 : 1)

void bresenham( const int x1, const int y1, const int x2, const int y2, int t,
                const std::function<bool(const point &)> &interact )
{
    // The slope components.
    const int dx = x2 - x1;
    const int dy = y2 - y1;
    // Signs of slope values.
    const int sx = (dx == 0) ? 0 : SGN(dx);
    const int sy = (dy == 0) ? 0 : SGN(dy);
    // Absolute values of slopes x2 to avoid rounding errors.
    const int ax = abs(dx) * 2;
    const int ay = abs(dy) * 2;

    point cur(x1, y1);

    if( ax == ay ) {
        while( cur.x != x2 ) {
            cur.y += sy;
            cur.x += sx;
            if( !interact( cur ) ) {
                break;
            }
        }
    } else if( ax > ay ) {
        while( cur.x != x2 ) {
            if( t > 0 ) {
                cur.y += sy;
                t -= ax;
            }
            cur.x += sx;
            t += ay;
            if( !interact( cur ) ) {
                break;
            }
        }
    } else {
        while( cur.y != y2 ) {
            if( t > 0 ) {
                cur.x += sx;
                t -= ay;
            }
            cur.y += sy;
            t += ax;
            if( !interact( cur ) ) {
                break;
            }
        }
    }
}

void bresenham( const tripoint &loc1, const tripoint &loc2, int t, int t2,
                const std::function<bool(const tripoint &)> &interact )
{
    // The slope components.
    const int dx = loc2.x - loc1.x;
    const int dy = loc2.y - loc1.y;
    const int dz = loc2.z - loc1.z;
    // The signs of the slopes.
    const int sx = (dx == 0 ? 0 : SGN(dx));
    const int sy = (dy == 0 ? 0 : SGN(dy));
    const int sz = (dz == 0 ? 0 : SGN(dz));
    // Absolute values of slope components, x2 to avoid rounding errors.
    const int ax = abs(dx) * 2;
    const int ay = abs(dy) * 2;
    const int az = abs(dz) * 2;

    tripoint cur( loc1 );

    if( az == 0 ) {
        if( ax == ay ) {
          while( cur.x != loc2.x ) {
                cur.y += sy;
                cur.x += sx;
                if( !interact( cur ) ) {
                    break;
                }
            }
        } else if (ax > ay) {
            while( cur.x != loc2.x ) {
                if( t > 0 ) {
                    cur.y += sy;
                    t -= ax;
                }
                cur.x += sx;
                t += ay;
                if( !interact( cur ) ) {
                    break;
                }
            }
        } else {
            while( cur.y != loc2.y ) {
                if( t > 0 ) {
                    cur.x += sx;
                    t -= ay;
                }
                cur.y += sy;
                t += ax;
                if( !interact( cur ) ) {
                    break;
                }
            }
        }
    } else {
        if( ax == ay && ay == az ) {
            while( cur.x != loc2.x ) {
                cur.z += sz;
                cur.y += sy;
                cur.x += sx;
                if( !interact( cur ) ) {
                    break;
                }
            }
        } else if( (az > ax) && (az > ay) ) {
            while( cur.z != loc2.z ) {
                if( t > 0 ) {
                    cur.x += sx;
                    t -= az;
                }
                if( t2 > 0 ) {
                    cur.y += sy;
                    t2 -= az;
                }
                cur.z += sz;
                t += ax;
                t2 += ay;
                if( !interact( cur ) ) {
                    break;
                }
            }
        } else if( ax == ay ) {
            while( cur.x != loc2.x ) {
                if( t > 0 ) {
                    cur.z += sz;
                    t -= ax;
                }
                cur.y += sy;
                cur.x += sx;
                t += az;
                if( !interact( cur ) ) {
                    break;
                }
            }
        } else if( ax > ay ) {
            while( cur.x != loc2.x ) {
                if( t > 0 ) {
                    cur.y += sy;
                    t -= ax;
                }
                if( t2 > 0 ) {
                    cur.z += sz;
                    t2 -= ax;
                }
                cur.x += sx;
                t += ay;
                t2 += az;
                if( !interact( cur ) ) {
                    break;
                }
            }
        } else { //dy > dx >= dz
            while( cur.y != loc2.y ) {
                if( t > 0 ) {
                    cur.x += sx;
                    t -= ay;
                }
                if( t2 > 0 ) {
                    cur.z += sz;
                    t2 -= ay;
                }
                cur.y += sy;
                t += ax;
                t2 += az;
                if( !interact( cur ) ) {
                    break;
                }
            }
        }
    }
}

//Trying to pull points out of a tripoint vector is messy and
//probably slow, so leaving two full functions for now
std::vector<point> line_to(const int x1, const int y1, const int x2, const int y2, int t)
{
    std::vector<point> line;
    // Preallocate the number of cells we need instead of allocating them piecewise.
    const int numCells = square_dist(tripoint(x1, y1, 0), tripoint(x2, y2, 0));
    if( numCells == 0 ) {
        line.push_back( {x1, y1} );
    } else {
        line.reserve(numCells);
        bresenham( x1, y1, x2, y2, t, [&line]( const point &new_point ) {
            line.push_back(new_point);
            return true;
        } );
    }
    return line;
}

std::vector<point> line_to( const point &p1, const point &p2, const int t )
{
    return line_to( p1.x, p1.y, p2.x, p2.y, t );
}

std::vector <tripoint> line_to(const tripoint &loc1, const tripoint &loc2, int t, int t2)
{
    std::vector<tripoint> line;
    // Preallocate the number of cells we need instead of allocating them piecewise.
    const int numCells = square_dist(loc1, loc2);
    if( numCells == 0 ) {
        line.push_back( loc1 );
    } else {
        line.reserve(numCells);
        bresenham( loc1, loc2, t, t2, [&line]( const tripoint &new_point ) {
            line.push_back(new_point);
            return true;
        } );
    }
    return line;
}

float trig_dist(const int x1, const int y1, const int x2, const int y2)
{
    return trig_dist(tripoint(x1, y1, 0), tripoint(x2, y2, 0));
}

float trig_dist(const tripoint &loc1, const tripoint &loc2)
{
    return sqrt(double((loc1.x - loc2.x) * (loc1.x - loc2.x)) +
                ((loc1.y - loc2.y) * (loc1.y - loc2.y)) +
                ((loc1.z - loc2.z) * (loc1.z - loc2.z)));
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

// This more general version of this function gives correct values for larger values.
unsigned make_xyz(int const x, int const y, int const z)
{
    static const double sixteenth_arc = 0.392699082;
    int vertical_position = ((z > 0) ? 2u : (z < 0) ? 1u : 0u) * 9u;
    if( x == 0 && y == 0 ) {
        return vertical_position;
    }
    // Get the arctan of the angle and divide by approximately 22.5 deg to get the octant.
    // the angle is in, then truncate it and map to the right direction.
    // You can read 'octant' as being "number of 22.5 degree sections away from due south".
    int octant = atan2( x, y ) / sixteenth_arc;
    switch(octant) {
    case 0:
      return SOUTH + vertical_position;
    case 1:
    case 2:
      return SOUTHEAST + vertical_position;
    case 3:
    case 4:
      return EAST + vertical_position;
    case 5:
    case 6:
      return NORTHEAST + vertical_position;
    case -1:
    case -2:
      return SOUTHWEST + vertical_position;
    case -3:
    case -4:
      return WEST + vertical_position;
    case -5:
    case -6:
      return NORTHWEST + vertical_position;
    case 7:
    case 8:
    case -7:
    case -8:
    default:
      return NORTH + vertical_position;
   }
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

float get_normalized_angle( const point &start, const point &end )
{
    // Taking the abs value of the difference puts the values in the first quadrant.
    const float absx = std::abs( std::max(start.x, end.x) - std::min(start.x, end.x) );
    const float absy = std::abs( std::max(start.y, end.y) - std::min(start.y, end.y) );
    const float max = std::max( absx, absy );
    if( max == 0 ) {
        return 0;
    }
    const float min = std::min( absx, absy );
    return min / max;
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
    // Note: Z coord has to be inverted either here or in direction defintions
    return direction_from(q.x - p.x, q.y - p.y, -(q.z - p.z) );
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
        result[ABOVECENTER]    = pair_t {_("UP_CE"), _("above")};
        result[CENTER]         = pair_t {_("CE   "), _("center")};
        result[BELOWCENTER]    = pair_t {_("DN_CE"), _("below")};

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

// Cardinals are cardinals. Result is cardinal and adjacent sub-cardinals.
// Sub-Cardinals are sub-cardinals && abs(x) == abs(y). Result is sub-cardinal and adjacent cardinals.
// Sub-sub-cardinals are direction && abs(x) > abs(y) or vice versa.
// Result is adjacent cardinal and sub-cardinals, plus the nearest other cardinal.
// e.g. if the direction is NNE, also include E.
std::vector<tripoint> squares_closer_to( const tripoint &from, const tripoint &to )
{
    std::vector<tripoint> adjacent_closer_squares;
    const int dx = to.x - from.x;
    const int dy = to.y - from.y;
    const int dz = to.z - from.z;
    const int ax = std::abs( dx );
    const int ay = std::abs( dy );
    if( dz != 0 ) {
        adjacent_closer_squares.push_back( { from.x + SGN(dx), from.y + SGN(dy), from.z + SGN(dz) } );
    }
    if( ax > ay ) {
        // X dominant.
        adjacent_closer_squares.push_back( { from.x + SGN(dx), from.y, from.z } );
        adjacent_closer_squares.push_back( { from.x + SGN(dx), from.y + 1, from.z } );
        adjacent_closer_squares.push_back( { from.x + SGN(dx), from.y - 1, from.z } );
        if( dy != 0 ) {
            adjacent_closer_squares.push_back( { from.x, from.y + SGN(dy), from.z } );
        }
    } else if( ax < ay ) {
        // Y dominant.
        adjacent_closer_squares.push_back( { from.x, from.y + SGN(dy), from.z } );
        adjacent_closer_squares.push_back( { from.x + 1, from.y + SGN(dy), from.z } );
        adjacent_closer_squares.push_back( { from.x - 1, from.y + SGN(dy), from.z } );
        if( dx != 0 ) {
            adjacent_closer_squares.push_back( { from.x + SGN(dx), from.y, from.z } );
        }
    } else {
        // Pure diagonal.
        adjacent_closer_squares.push_back( { from.x + SGN(dx), from.y + SGN(dy), from.z } );
        adjacent_closer_squares.push_back( { from.x + SGN(dx), from.y, from.z } );
        adjacent_closer_squares.push_back( { from.x, from.y + SGN(dy), from.z } );
    }

    return adjacent_closer_squares;
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
