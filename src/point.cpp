#include "point.h"

#include <algorithm>
#include <cmath>
#include <locale>
#include <sstream>
#include <string>

#include "cata_assert.h"
#include "debug.h"

point point::from_string( const std::string &s )
{
    std::istringstream is( s );
    is.imbue( std::locale::classic() );
    point result;
    is >> result;
    if( !is ) {
        debugmsg( "Could not convert string '" + s + "' to point" );
        return point::zero;
    }
    return result;
}

point point::rotate( int turns, const point &dim ) const
{
    cata_assert( turns >= 0 );
    cata_assert( turns <= 4 );

    switch( turns ) {
        case 1:
            return { dim.y - y - 1, x };
        case 2:
            return { dim.x - x - 1, dim.y - y - 1 };
        case 3:
            return { y, dim.x - x - 1 };
    }

    return *this;
}

float point::distance( const point &rhs ) const
{
    return std::sqrt( static_cast<float>( std::pow( x - rhs.x, 2 ) + std::pow( y - rhs.y, 2 ) ) );
}

int point::distance_manhattan( const point &rhs ) const
{
    return std::abs( x - rhs.x ) + std::abs( y - rhs.y );
}

std::string point::to_string() const
{
    std::ostringstream os;
    os.imbue( std::locale::classic() );
    os << *this;
    return os.str();
}

std::string point::to_string_writable() const
{
    // This is supposed to be a non-translated version of to_string, but we can
    // just use regular to_string
    return to_string();
}

tripoint tripoint::from_string( const std::string &s )
{
    std::istringstream is( s );
    is.imbue( std::locale::classic() );
    tripoint result;
    is >> result;
    if( !is ) {
        debugmsg( "Could not convert string '" + s + "' to tripoint" );
        return tripoint::zero;
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

std::string tripoint::to_string_writable() const
{
    // This is supposed to be a non-translated version of to_string, but we can
    // just use regular to_string
    return to_string();
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
    // silence -Wunused-value
    static_cast<void>( is.get( c ) && c == '(' && is >> pos.x && is.get( c ) && c == ',' &&
                       is >> pos.y && is.get( c ) && c == ')' );
    return is;
}

std::istream &operator>>( std::istream &is, tripoint &pos )
{
    char c;
    static_cast<void>( is.get( c ) && c == '(' && is >> pos.x && is.get( c ) && c == ',' &&
                       is >> pos.y && is.get( c ) && c == ',' && is >> pos.z && is.get( c ) && c == ')' );
    return is;
}

std::optional<int> rectangle_size( int min_dist, int max_dist )
{
    min_dist = std::max( min_dist, 0 );
    max_dist = std::max( max_dist, 0 );

    if( min_dist > max_dist ) {
        return std::nullopt;
    }

    const int min_edge = min_dist * 2 + 1;
    const int max_edge = max_dist * 2 + 1;

    const int n = max_edge * max_edge - ( min_edge - 2 ) * ( min_edge - 2 ) + ( min_dist == 0 ? 1 : 0 );
    return n;
}

std::vector<tripoint> closest_points_first( const tripoint &center, int max_dist )
{
    return closest_points_first( center, 0, max_dist );
}

std::vector<tripoint> closest_points_first( const tripoint &center, int min_dist, int max_dist )
{
    std::optional<int> n = rectangle_size( min_dist, max_dist );

    if( n == std::nullopt ) {
        return {};
    }

    std::vector<tripoint> result;
    result.reserve( *n );

    find_point_closest_first( center, min_dist, max_dist, [&result]( const tripoint & p ) {
        result.push_back( p );
        return false;
    } );

    return result;
}

std::vector<point> closest_points_first( const point &center, int max_dist )
{
    return closest_points_first( center, 0, max_dist );
}

std::vector<point> closest_points_first( const point &center, int min_dist, int max_dist )
{
    std::optional<int> n = rectangle_size( min_dist, max_dist );

    if( n == std::nullopt ) {
        return {};
    }

    std::vector<point> result;
    result.reserve( *n );

    find_point_closest_first( center, min_dist, max_dist, [&result]( const point & p ) {
        result.push_back( p );
        return false;
    } );

    return result;
}

template <typename PredicateFn, typename Point>
std::optional<Point> find_point_closest_first( const Point &center, int max_dist,
        PredicateFn &&fn )
{
    return find_point_closest_first( center, 0, max_dist, fn );
}
