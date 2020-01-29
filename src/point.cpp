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
    std::vector<tripoint> points;
    int X = radius * 2 + 1;
    int Y = radius * 2 + 1;
    int x = 0;
    int y = 0;
    int dx = 0;
    int dy = -1;
    int t = std::max( X, Y );
    int maxI = t * t;
    for( int i = 0; i < maxI; i++ ) {
        if( -X / 2 <= x && x <= X / 2 && -Y / 2 <= y && y <= Y / 2 ) {
            points.push_back( center + point( x, y ) );
        }
        if( x == y || ( x < 0 && x == -y ) || ( x > 0 && x == 1 - y ) ) {
            t = dx;
            dx = -dy;
            dy = t;
        }
        x += dx;
        y += dy;
    }
    return points;
}

std::vector<point> closest_points_first( const point &center, size_t radius )
{
    const std::vector<tripoint> tripoints = closest_tripoints_first( tripoint( center, 0 ), radius );
    std::vector<point> points;
    for( const tripoint &p : tripoints ) {
        points.push_back( p.xy() );
    }
    return points;
}
