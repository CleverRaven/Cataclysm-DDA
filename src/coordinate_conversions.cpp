#include "coordinate_conversions.h"

#include "map_scale_constants.h"

static int divide( int v, int m )
{
    if( v >= 0 ) {
        return v / m;
    }
    return ( v - m + 1 ) / m;
}

static int divide( int v, int m, int &r )
{
    const int result = divide( v, m );
    r = v - result * m;
    return result;
}

point sm_to_mmr_remain( int &x, int &y )
{
    return point( divide( x, MM_REG_SIZE, x ), divide( y, MM_REG_SIZE, y ) );
}

