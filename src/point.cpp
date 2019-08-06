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
