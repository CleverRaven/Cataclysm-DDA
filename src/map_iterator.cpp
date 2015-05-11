#include "map_iterator.h"

tripoint_range::point_generator &tripoint_range::point_generator::operator++()
{
    p.x++;
    if( p.x <= range.maxx ) {
        return *this;
    }
    
    p.y++;
    p.x = range.minx;
    if( p.y <= range.maxy ) {
        return *this;
    }

    p.z++;
    p.y = range.miny;
    return *this;
}

const tripoint &tripoint_range::point_generator::operator*() const
{
    return p;
}

bool tripoint_range::point_generator::operator!=( const point_generator &other ) const
{
    // Reverse coord order, because it will usually only be compared with endpoint
    // which will always differ in z, except for the very last comparison
    const tripoint &pt = other.p;
    return p.z != pt.z || p.y != pt.y || p.x != pt.x;
}


tripoint_range::point_generator tripoint_range::begin() const
{
    return point_generator( tripoint( minx, miny, minz ), *this );
}

tripoint_range::point_generator tripoint_range::end() const
{
    return point_generator( tripoint( maxx, maxy, maxz ), *this );
}

tripoint_range::tripoint_range( int _minx, int _miny, int _minz, int _maxx, int _maxy, int _maxz )
    : minx( _minx ), miny( _miny ), minz( _minz ),
      maxx( _maxx ), maxy( _maxy ), maxz( _maxz )
{
}
