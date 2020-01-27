#include "point.h"

#include <sstream>

#include "cata_utility.h"

std::string point::to_string() const
{
    std::ostringstream os;
    os << *this;
    return os.str();
}

std::string tripoint::to_string() const
{
    std::ostringstream os;
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

point clamp_half_open( const point &p, const rectangle &r )
{
    return point( clamp( p.x, r.p_min.x, r.p_max.x - 1 ), clamp( p.y, r.p_min.y, r.p_max.y - 1 ) );
}

point clamp_inclusive( const point &p, const rectangle &r )
{
    return point( clamp( p.x, r.p_min.x, r.p_max.x ), clamp( p.y, r.p_min.y, r.p_max.y ) );
}

std::vector<tripoint> closest_tripoints_first( const tripoint &center, size_t radius )
{
    const std::vector<point> points = closest_points_first( center.xy(), radius );

    std::vector<tripoint> result;
    result.reserve( points.size() );

    for( const point &p : points ) {
        result.emplace_back( p, center.z );
    }

    return result;
}

std::vector<point> closest_points_first( const point &center, size_t radius )
{
    const int edge = radius * 2 + 1;
    const int n = edge * edge;

    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = -1;

    std::vector<point> result;
    result.reserve( n );

    for( int i = 0; i < n; i++ ) {
        result.push_back( center + point{ x, y } );

        if( x == y || ( x < 0 && x == -y ) || ( x > 0 && x == 1 - y ) ) {
            std::swap( dx, dy );
            dx = -dx;
        }

        x += dx;
        y += dy;
    }

    return result;
}
