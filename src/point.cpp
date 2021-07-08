#include "point.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <type_traits>

#include "debug.h"

point point::from_string( const std::string &s )
{
    std::istringstream is( s );
    is.imbue( std::locale::classic() );
    point result;
    is >> result;
    if( !is ) {
        debugmsg( "Could not convert string '" + s + "' to point" );
        return point_zero;
    }
    return result;
}

std::string point::to_string() const
{
    std::ostringstream os;
    os.imbue( std::locale::classic() );
    os << *this;
    return os.str();
}

tripoint tripoint::from_string( const std::string &s )
{
    std::istringstream is( s );
    is.imbue( std::locale::classic() );
    tripoint result;
    is >> result;
    if( !is ) {
        debugmsg( "Could not convert string '" + s + "' to tripoint" );
        return tripoint_zero;
    }
    return result;
}

std::string tripoint::to_string() const
{
    std::ostringstream os;
    os.imbue( std::locale::classic() );
    os << *this;
    return os.str();
}

std::ostream &operator<<( std::ostream &os, const point &pos )
{
    return os << "(" << pos.x << "," << pos.y << ")";
}

std::ostream &operator<<( std::ostream &os, const tripoint &pos )
{
    return os << "(" << pos.x << "," << pos.y << "," << pos.z << ")";
}

std::istream &operator>>( std::istream &is, point &pos )
{
    char c;
    is.get( c ) &&c == '(' &&is >> pos.x &&is.get( c ) &&c == ',' &&is >> pos.y &&
                                is.get( c ) &&c == ')';
    return is;
}

std::istream &operator>>( std::istream &is, tripoint &pos )
{
    char c;
    is.get( c ) &&c == '(' &&is >> pos.x &&is.get( c ) &&c == ',' &&is >> pos.y &&
                                is.get( c ) &&c == ',' &&is >> pos.z &&is.get( c ) &&c == ')';
    return is;
}

std::vector<tripoint> closest_points_first( const tripoint &center, int max_dist )
{
    return closest_points_first( center, 0, max_dist );
}

std::vector<tripoint> closest_points_first( const tripoint &center, int min_dist, int max_dist )
{
    const std::vector<point> points = closest_points_first( center.xy(), min_dist, max_dist );

    std::vector<tripoint> result;
    result.reserve( points.size() );

    for( const point &p : points ) {
        result.emplace_back( p, center.z );
    }

    return result;
}

std::vector<point> closest_points_first( const point &center, int max_dist )
{
    return closest_points_first( center, 0, max_dist );
}

std::vector<point> closest_points_first( const point &center, int min_dist, int max_dist )
{
    min_dist = std::max( min_dist, 0 );
    max_dist = std::max( max_dist, 0 );

    if( min_dist > max_dist ) {
        return {};
    }

    const int min_edge = min_dist * 2 + 1;
    const int max_edge = max_dist * 2 + 1;

    const int n = max_edge * max_edge - ( min_edge - 2 ) * ( min_edge - 2 );
    const bool is_center_included = min_dist == 0;

    std::vector<point> result;
    result.reserve( n + ( is_center_included ? 1 : 0 ) );

    if( is_center_included ) {
        result.push_back( center );
    }

    int x_init = std::max( min_dist, 1 );
    point p( x_init, 1 - x_init );

    point d( point_east );

    for( int i = 0; i < n; i++ ) {
        result.push_back( center + p );

        if( p.x == p.y || ( p.x < 0 && p.x == -p.y ) || ( p.x > 0 && p.x == 1 - p.y ) ) {
            std::swap( d.x, d.y );
            d.x = -d.x;
        }

        p.x += d.x;
        p.y += d.y;
    }

    return result;
}
