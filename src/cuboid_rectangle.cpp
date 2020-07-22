#include "cuboid_rectangle.h"

#include "cata_utility.h"

point clamp( const point &p, const half_open_rectangle &r )
{
    return point( clamp( p.x, r.p_min.x, r.p_max.x - 1 ), clamp( p.y, r.p_min.y, r.p_max.y - 1 ) );
}

point clamp( const point &p, const inclusive_rectangle &r )
{
    return point( clamp( p.x, r.p_min.x, r.p_max.x ), clamp( p.y, r.p_min.y, r.p_max.y ) );
}
