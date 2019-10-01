#include "line.h"

#include <cstdlib>
#include <cassert>
#include <algorithm>
#include <array>
#include <memory>
#include <tuple>
#include <utility>

#include "math_defines.h"
#include "translations.h"
#include "string_formatter.h"
#include "output.h"
#include "enums.h"

extern bool trigdist;

void bresenham( const point &p1, const point &p2, int t,
                const std::function<bool( const point & )> &interact )
{
    // The slope components.
    const point d = p2 - p1;
    // Signs of slope values.
    const int sx = ( d.x == 0 ) ? 0 : sgn( d.x );
    const int sy = ( d.y == 0 ) ? 0 : sgn( d.y );
    // Absolute values of slopes x2 to avoid rounding errors.
    const point a = abs( d ) * 2;

    point cur = p1;

    if( a.x == a.y ) {
        while( cur.x != p2.x ) {
            cur.y += sy;
            cur.x += sx;
            if( !interact( cur ) ) {
                break;
            }
        }
    } else if( a.x > a.y ) {
        while( cur.x != p2.x ) {
            if( t > 0 ) {
                cur.y += sy;
                t -= a.x;
            }
            cur.x += sx;
            t += a.y;
            if( !interact( cur ) ) {
                break;
            }
        }
    } else {
        while( cur.y != p2.y ) {
            if( t > 0 ) {
                cur.x += sx;
                t -= a.y;
            }
            cur.y += sy;
            t += a.x;
            if( !interact( cur ) ) {
                break;
            }
        }
    }
}

void bresenham( const tripoint &loc1, const tripoint &loc2, int t, int t2,
                const std::function<bool( const tripoint & )> &interact )
{
    // The slope components.
    const int dx = loc2.x - loc1.x;
    const int dy = loc2.y - loc1.y;
    const int dz = loc2.z - loc1.z;
    // The signs of the slopes.
    const int sx = ( dx == 0 ? 0 : sgn( dx ) );
    const int sy = ( dy == 0 ? 0 : sgn( dy ) );
    const int sz = ( dz == 0 ? 0 : sgn( dz ) );
    // Absolute values of slope components, x2 to avoid rounding errors.
    const int ax = abs( dx ) * 2;
    const int ay = abs( dy ) * 2;
    const int az = abs( dz ) * 2;

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
        } else if( ax > ay ) {
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
        } else if( ( az > ax ) && ( az > ay ) ) {
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
std::vector<point> line_to( const point &p1, const point &p2, int t )
{
    std::vector<point> line;
    // Preallocate the number of cells we need instead of allocating them piecewise.
    const int numCells = square_dist( p1, p2 );
    if( numCells == 0 ) {
        line.push_back( p1 );
    } else {
        line.reserve( numCells );
        bresenham( p1, p2, t, [&line]( const point & new_point ) {
            line.push_back( new_point );
            return true;
        } );
    }
    return line;
}

std::vector <tripoint> line_to( const tripoint &loc1, const tripoint &loc2, int t, int t2 )
{
    std::vector<tripoint> line;
    // Preallocate the number of cells we need instead of allocating them piecewise.
    const int numCells = square_dist( loc1, loc2 );
    if( numCells == 0 ) {
        line.push_back( loc1 );
    } else {
        line.reserve( numCells );
        bresenham( loc1, loc2, t, t2, [&line]( const tripoint & new_point ) {
            line.push_back( new_point );
            return true;
        } );
    }
    return line;
}

float trig_dist( const point &loc1, const point &loc2 )
{
    return trig_dist( tripoint( loc1, 0 ), tripoint( loc2, 0 ) );
}

float trig_dist( const tripoint &loc1, const tripoint &loc2 )
{
    return sqrt( static_cast<double>( ( loc1.x - loc2.x ) * ( loc1.x - loc2.x ) ) +
                 ( ( loc1.y - loc2.y ) * ( loc1.y - loc2.y ) ) +
                 ( ( loc1.z - loc2.z ) * ( loc1.z - loc2.z ) ) );
}

int square_dist( const point &loc1, const point &loc2 )
{
    const point d = abs( loc1 - loc2 );
    return std::max( d.x, d.y );
}

int square_dist( const tripoint &loc1, const tripoint &loc2 )
{
    const tripoint d = abs( loc1 - loc2 );
    return std::max( { d.x, d.y, d.z } );
}

int rl_dist( const point &a, const point &b )
{
    return rl_dist( tripoint( a, 0 ), tripoint( b, 0 ) );
}

int rl_dist( const tripoint &loc1, const tripoint &loc2 )
{
    if( trigdist ) {
        return trig_dist( loc1, loc2 );
    }
    return square_dist( loc1, loc2 );
}

int manhattan_dist( const point &loc1, const point &loc2 )
{
    const point d = abs( loc1 - loc2 );
    return d.x + d.y;
}

double atan2( const point &p )
{
    return atan2( static_cast<double>( p.y ), static_cast<double>( p.x ) );
}

double atan2_degrees( const point &p )
{
    return atan2( p ) * 180.0 / M_PI;
}

// This more general version of this function gives correct values for larger values.
unsigned make_xyz( const tripoint &p )
{
    static constexpr double sixteenth_arc = M_PI / 8;
    int vertical_position = ( ( p.z > 0 ) ? 2u : ( p.z < 0 ) ? 1u : 0u ) * 9u;
    if( p.xy() == point_zero ) {
        return vertical_position;
    }
    // Get the arctan of the angle and divide by approximately 22.5 deg to get the octant.
    // the angle is in, then truncate it and map to the right direction.
    // You can read 'octant' as being "number of 22.5 degree sections away from due south".
    // FIXME: atan2 normally takes arguments in ( y, x ) order.  This is
    // passing ( x, y ).
    int octant = atan2( p.x, p.y ) / sixteenth_arc;
    switch( octant ) {
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

// returns the normalized dx, dy, dz for the current line vector.
static std::tuple<double, double, double> slope_of( const std::vector<tripoint> &line )
{
    assert( !line.empty() && line.front() != line.back() );
    const double len = trig_dist( line.front(), line.back() );
    double normDx = ( line.back().x - line.front().x ) / len;
    double normDy = ( line.back().y - line.front().y ) / len;
    double normDz = ( line.back().z - line.front().z ) / len;
    // slope of <x, y, z>
    return std::make_tuple( normDx, normDy, normDz );
}

float get_normalized_angle( const point &start, const point &end )
{
    // Taking the abs value of the difference puts the values in the first quadrant.
    const float absx = std::abs( std::max( start.x, end.x ) - std::min( start.x, end.x ) );
    const float absy = std::abs( std::max( start.y, end.y ) - std::min( start.y, end.y ) );
    const float max = std::max( absx, absy );
    if( max == 0 ) {
        return 0;
    }
    const float min = std::min( absx, absy );
    return min / max;
}

tripoint move_along_line( const tripoint &loc, const std::vector<tripoint> &line,
                          const int distance )
{
    // May want to optimize this, but it's called fairly infrequently as part of specific attack
    // routines, erring on the side of readability.
    tripoint res( loc );
    const auto slope = slope_of( line );
    res.x += distance * std::get<0>( slope );
    res.y += distance * std::get<1>( slope );
    res.z += distance * std::get<2>( slope );
    return res;
}

std::vector<tripoint> continue_line( const std::vector<tripoint> &line, const int distance )
{
    return line_to( line.back(), move_along_line( line.back(), line, distance ) );
}

direction direction_from( const point &p ) noexcept
{
    return static_cast<direction>( make_xyz( tripoint( p, 0 ) ) );
}

direction direction_from( const tripoint &p ) noexcept
{
    return static_cast<direction>( make_xyz( p ) );
}

direction direction_from( const point &p1, const point &p2 ) noexcept
{
    return direction_from( p2 - p1 );
}

direction direction_from( const tripoint &p, const tripoint &q )
{
    // Note: Z-coordinate has to be inverted either here or in direction definitions
    return direction_from( tripoint( q.x - p.x, q.y - p.y, -( q.z - p.z ) ) );
}

point direction_XY( const direction dir )
{
    switch( dir % 9 ) {
        case NORTHWEST:
            return point_north_west;
        case NORTH:
            return point_north;
        case NORTHEAST:
            return point_north_east;
        case WEST:
            return point_west;
        case CENTER:
            return point_zero;
        case EAST:
            return point_east;
        case SOUTHWEST:
            return point_south_west;
        case SOUTH:
            return point_south;
        case SOUTHEAST:
            return point_south_east;
    }

    return point_zero;
}

namespace
{
std::string direction_name_impl( const direction dir, const bool short_name )
{
    enum : int { size = 3 * 3 * 3 };
    static const auto names = [] {
        using pair_t = std::pair<std::string, std::string>;
        std::array < pair_t, size + 1 > result;

        //~ abbreviated direction names and long direction names
        result[NORTH]          = pair_t {translate_marker( "N    " ), translate_marker( "north" )};
        result[NORTHEAST]      = pair_t {translate_marker( "NE   " ), translate_marker( "northeast" )};
        result[EAST]           = pair_t {translate_marker( "E    " ), translate_marker( "east" )};
        result[SOUTHEAST]      = pair_t {translate_marker( "SE   " ), translate_marker( "southeast" )};
        result[SOUTH]          = pair_t {translate_marker( "S    " ), translate_marker( "south" )};
        result[SOUTHWEST]      = pair_t {translate_marker( "SW   " ), translate_marker( "southwest" )};
        result[WEST]           = pair_t {translate_marker( "W    " ), translate_marker( "west" )};
        result[NORTHWEST]      = pair_t {translate_marker( "NW   " ), translate_marker( "northwest" )};
        result[ABOVENORTH]     = pair_t {translate_marker( "UP_N " ), translate_marker( "north and above" )};
        result[ABOVENORTHEAST] = pair_t {translate_marker( "UP_NE" ), translate_marker( "northeast and above" )};
        result[ABOVEEAST]      = pair_t {translate_marker( "UP_E " ), translate_marker( "east and above" )};
        result[ABOVESOUTHEAST] = pair_t {translate_marker( "UP_SE" ), translate_marker( "southeast and above" )};
        result[ABOVESOUTH]     = pair_t {translate_marker( "UP_S " ), translate_marker( "south and above" )};
        result[ABOVESOUTHWEST] = pair_t {translate_marker( "UP_SW" ), translate_marker( "southwest and above" )};
        result[ABOVEWEST]      = pair_t {translate_marker( "UP_W " ), translate_marker( "west and above" )};
        result[ABOVENORTHWEST] = pair_t {translate_marker( "UP_NW" ), translate_marker( "northwest and above" )};
        result[BELOWNORTH]     = pair_t {translate_marker( "DN_N " ), translate_marker( "north and below" )};
        result[BELOWNORTHEAST] = pair_t {translate_marker( "DN_NE" ), translate_marker( "northeast and below" )};
        result[BELOWEAST]      = pair_t {translate_marker( "DN_E " ), translate_marker( "east and below" )};
        result[BELOWSOUTHEAST] = pair_t {translate_marker( "DN_SE" ), translate_marker( "southeast and below" )};
        result[BELOWSOUTH]     = pair_t {translate_marker( "DN_S " ), translate_marker( "south and below" )};
        result[BELOWSOUTHWEST] = pair_t {translate_marker( "DN_SW" ), translate_marker( "southwest and below" )};
        result[BELOWWEST]      = pair_t {translate_marker( "DN_W " ), translate_marker( "west and below" )};
        result[BELOWNORTHWEST] = pair_t {translate_marker( "DN_NW" ), translate_marker( "northwest and below" )};
        result[ABOVECENTER]    = pair_t {translate_marker( "UP_CE" ), translate_marker( "above" )};
        result[CENTER]         = pair_t {translate_marker( "CE   " ), translate_marker( "center" )};
        result[BELOWCENTER]    = pair_t {translate_marker( "DN_CE" ), translate_marker( "below" )};

        result[size] = pair_t {"BUG. (line.cpp:direction_name)", "BUG. (line.cpp:direction_name)"};
        return result;
    }();

    auto i = static_cast<int>( dir );
    if( i < 0 || i >= size ) {
        i = size;
    }

    return short_name ? _( names[i].first ) : _( names[i].second );
}
} //namespace

std::string direction_name( const direction dir )
{
    return direction_name_impl( dir, false );
}

std::string direction_name_short( const direction dir )
{
    return direction_name_impl( dir, true );
}

std::string direction_suffix( const tripoint &p, const tripoint &q )
{
    int dist = square_dist( p, q );
    if( dist <= 0 ) {
        return std::string();
    }
    return string_format( "%d%s", dist, trim( direction_name_short( direction_from( p, q ) ) ) );
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
        adjacent_closer_squares.push_back( from + tripoint( sgn( dx ), sgn( dy ), sgn( dz ) ) );
    }
    if( ax > ay ) {
        // X dominant.
        adjacent_closer_squares.push_back( from + point( sgn( dx ), 0 ) );
        adjacent_closer_squares.push_back( from + point( sgn( dx ), 1 ) );
        adjacent_closer_squares.push_back( from + point( sgn( dx ), -1 ) );
        if( dy != 0 ) {
            adjacent_closer_squares.push_back( from + point( 0, sgn( dy ) ) );
        }
    } else if( ax < ay ) {
        // Y dominant.
        adjacent_closer_squares.push_back( from + point( 0, sgn( dy ) ) );
        adjacent_closer_squares.push_back( from + point( 1, sgn( dy ) ) );
        adjacent_closer_squares.push_back( from + point( -1, sgn( dy ) ) );
        if( dx != 0 ) {
            adjacent_closer_squares.push_back( from + point( sgn( dx ), 0 ) );
        }
    } else if( dx != 0 ) {
        // Pure diagonal.
        adjacent_closer_squares.push_back( from + point( sgn( dx ), sgn( dy ) ) );
        adjacent_closer_squares.push_back( from + point( sgn( dx ), 0 ) );
        adjacent_closer_squares.push_back( from + point( 0, sgn( dy ) ) );
    }

    return adjacent_closer_squares;
}

// Returns a vector of the adjacent square in the direction of the target,
// and the two squares flanking it.
std::vector<point> squares_in_direction( const point &p1, const point &p2 )
{
    int junk = 0;
    point center_square = line_to( p1, p2, junk )[0];
    std::vector<point> adjacent_squares;
    adjacent_squares.push_back( center_square );
    if( p1.x == center_square.x ) {
        // Horizontally adjacent.
        adjacent_squares.push_back( point( p1.x + 1, center_square.y ) );
        adjacent_squares.push_back( point( p1.x - 1, center_square.y ) );
    } else if( p1.y == center_square.y ) {
        // Vertically adjacent.
        adjacent_squares.push_back( point( center_square.x, p1.y + 1 ) );
        adjacent_squares.push_back( point( center_square.x, p1.y - 1 ) );
    } else {
        // Diagonally adjacent.
        adjacent_squares.push_back( point( p1.x, center_square.y ) );
        adjacent_squares.push_back( point( center_square.x, p1.y ) );
    }
    return adjacent_squares;
}

float rl_vec2d::magnitude() const
{
    return sqrt( x * x + y * y );
}

float rl_vec3d::magnitude() const
{
    return sqrt( x * x + y * y + z * z );
}

rl_vec2d rl_vec2d::normalized() const
{
    rl_vec2d ret;
    if( is_null() ) { // shouldn't happen?
        ret.x = ret.y = 1;
        return ret;
    }
    const float m = magnitude();
    ret.x = x / m;
    ret.y = y / m;
    return ret;
}

rl_vec3d rl_vec3d::normalized() const
{
    rl_vec3d ret;
    if( is_null() ) { // shouldn't happen?
        ret.x = ret.y = ret.z = 0;
        return ret;
    }
    const float m = magnitude();
    ret.x = x / m;
    ret.y = y / m;
    ret.z = z / m;
    return ret;
}

rl_vec2d rl_vec2d::rotated( float angle ) const
{
    return rl_vec2d(
               x * cos( angle ) - y * sin( angle ),
               x * sin( angle ) + y * cos( angle )
           );
}

rl_vec3d rl_vec3d::rotated( float angle ) const
{
    return rl_vec3d(
               x * cos( angle ) - y * sin( angle ),
               x * sin( angle ) + y * cos( angle )
           );
}

float rl_vec2d::dot_product( const rl_vec2d &v ) const
{
    return x * v.x + y * v.y;
}

float rl_vec3d::dot_product( const rl_vec3d &v ) const
{
    return x * v.x + y * v.y + y * v.z;
}

bool rl_vec2d::is_null() const
{
    return !( x || y );
}

point rl_vec2d::as_point() const
{
    return point(
               round( x ),
               round( y )
           );
}

bool rl_vec3d::is_null() const
{
    return !( x || y || z );
}

tripoint rl_vec3d::as_point() const
{
    return tripoint(
               round( x ),
               round( y ),
               round( z )
           );
}

// scale.
rl_vec2d rl_vec2d::operator*( const float rhs ) const
{
    rl_vec2d ret;
    ret.x = x * rhs;
    ret.y = y * rhs;
    return ret;
}

rl_vec3d rl_vec3d::operator*( const float rhs ) const
{
    rl_vec3d ret;
    ret.x = x * rhs;
    ret.y = y * rhs;
    ret.z = z * rhs;
    return ret;
}

// subtract
rl_vec2d rl_vec2d::operator-( const rl_vec2d &rhs ) const
{
    rl_vec2d ret;
    ret.x = x - rhs.x;
    ret.y = y - rhs.y;
    return ret;
}

rl_vec3d rl_vec3d::operator-( const rl_vec3d &rhs ) const
{
    rl_vec3d ret;
    ret.x = x - rhs.x;
    ret.y = y - rhs.y;
    ret.z = z - rhs.z;
    return ret;
}

// unary negation
rl_vec2d rl_vec2d::operator-() const
{
    rl_vec2d ret;
    ret.x = -x;
    ret.y = -y;
    return ret;
}

rl_vec3d rl_vec3d::operator-() const
{
    rl_vec3d ret;
    ret.x = -x;
    ret.y = -y;
    ret.z = -z;
    return ret;
}

rl_vec2d rl_vec2d::operator+( const rl_vec2d &rhs ) const
{
    rl_vec2d ret;
    ret.x = x + rhs.x;
    ret.y = y + rhs.y;
    return ret;
}

rl_vec3d rl_vec3d::operator+( const rl_vec3d &rhs ) const
{
    rl_vec3d ret;
    ret.x = x + rhs.x;
    ret.y = y + rhs.y;
    ret.z = z + rhs.z;
    return ret;
}

rl_vec2d rl_vec2d::operator/( const float rhs ) const
{
    rl_vec2d ret;
    ret.x = x / rhs;
    ret.y = y / rhs;
    return ret;
}

rl_vec3d rl_vec3d::operator/( const float rhs ) const
{
    rl_vec3d ret;
    ret.x = x / rhs;
    ret.y = y / rhs;
    ret.z = z / rhs;
    return ret;
}

void calc_ray_end( int angle, const int range, const tripoint &p, tripoint &out )
{
    // forces input angle to be between 0 and 360, calculated from actual input
    angle %= 360;
    if( angle < 0 ) {
        angle += 360;
    }
    const double rad = DEGREES( angle );
    out.z = p.z;
    if( trigdist ) {
        out.x = p.x + range * cos( rad );
        out.y = p.y + range * sin( rad );
    } else {
        int mult = 0;
        if( angle >= 135 && angle <= 315 ) {
            mult = -1;
        } else {
            mult = 1;
        }

        if( angle <= 45 || ( 135 <= angle && angle <= 215 ) || 315 < angle ) {
            out.x = p.x + range * mult;
            out.y = p.y + range * tan( rad ) * mult;
        } else {
            out.x = p.x + range * 1 / tan( rad ) * mult;
            out.y = p.y + range * mult;
        }
    }
}

double coord_to_angle( const tripoint &a, const tripoint &b )
{
    double rad = atan2( b.y - a.y, b.x - a.x );
    if( rad < 0 ) {
        rad += 2 * M_PI;
    }
    return rad * 180 / M_PI;
}
