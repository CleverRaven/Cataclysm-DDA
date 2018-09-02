#include "coordinate_conversions.h"

#include "game_constants.h"

#include "options.h"

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

point omt_to_om_copy( int x, int y )
{
    point return_value = point( divide( x, OMAPX ), divide( y, OMAPY ) );
    limit_and_loop_om_coordinates( return_value );
    return return_value;
}

tripoint omt_to_om_copy( const tripoint &p )
{
    tripoint return_value = tripoint( divide( p.x, OMAPX ), divide( p.y, OMAPY ), p.z );
    limit_and_loop_om_coordinates( return_value );
    return return_value;
}

void omt_to_om( int &x, int &y )
{
    x = divide( x, OMAPX );
    y = divide( y, OMAPY );

    limit_and_loop_om_coordinates( x, y );
}

point omt_to_om_remain( int &x, int &y )
{
    point return_value = point( divide( x, OMAPX, x ), divide( y, OMAPY, y ) );
    limit_and_loop_om_coordinates( return_value );
    return return_value;
}


point sm_to_omt_copy( int x, int y )
{
    return point( divide( x, 2 ), divide( y, 2 ) );
}

tripoint sm_to_omt_copy( const tripoint &p )
{
    return tripoint( divide( p.x, 2 ), divide( p.y, 2 ), p.z );
}

void sm_to_omt( int &x, int &y )
{
    x = divide( x, 2 );
    y = divide( y, 2 );
}

point sm_to_omt_remain( int &x, int &y )
{
    return point( divide( x, 2, x ), divide( y, 2, y ) );
}


point sm_to_om_copy( int x, int y )
{
    point return_value = point( divide( x, 2 * OMAPX ), divide( y, 2 * OMAPY ) );
    limit_and_loop_om_coordinates( return_value );
    return return_value;
}

tripoint sm_to_om_copy( const tripoint &p )
{
    tripoint return_value = tripoint( divide( p.x, 2 * OMAPX ), divide( p.y, 2 * OMAPY ), p.z );
    limit_and_loop_om_coordinates( return_value );
    return return_value;
}

void sm_to_om( int &x, int &y )
{
    x = divide( x, 2 * OMAPX );
    y = divide( y, 2 * OMAPY );
    limit_and_loop_om_coordinates( x, y );
}

point sm_to_om_remain( int &x, int &y )
{
    point return_value = point( divide( x, 2 * OMAPX, x ), divide( y, 2 * OMAPY, y ) );
    limit_and_loop_om_coordinates( return_value );
    return return_value;
}


point omt_to_sm_copy( int x, int y )
{
    return point( x * 2, y * 2 );
}

tripoint omt_to_sm_copy( const tripoint &p )
{
    return tripoint( p.x * 2, p.y * 2, p.z );
}

void omt_to_sm( int &x, int &y )
{
    x *= 2;
    y *= 2;
}


point om_to_sm_copy( int x, int y )
{
    return point( x * 2 * OMAPX, y * 2 * OMAPX );
}

tripoint om_to_sm_copy( const tripoint &p )
{
    return tripoint( p.x * 2 * OMAPX, p.y * 2 * OMAPX, p.z );
}

void om_to_sm( int &x, int &y )
{
    x *= 2 * OMAPX;
    y *= 2 * OMAPY;
}


point ms_to_sm_copy( int x, int y )
{
    return point( divide( x, SEEX ), divide( y, SEEY ) );
}

tripoint ms_to_sm_copy( const tripoint &p )
{
    return tripoint( divide( p.x, SEEX ), divide( p.y, SEEY ), p.z );
}

void ms_to_sm( int &x, int &y )
{
    x = divide( x, SEEX );
    y = divide( y, SEEY );
}

point ms_to_sm_remain( int &x, int &y )
{
    return point( divide( x, SEEX, x ), divide( y, SEEY, y ) );
}


point sm_to_ms_copy( int x, int y )
{
    return point( x * SEEX, y * SEEY );
}

tripoint sm_to_ms_copy( const tripoint &p )
{
    return tripoint( p.x * SEEX, p.y * SEEY, p.z );
}

void sm_to_ms( int &x, int &y )
{
    x *= SEEX;
    y *= SEEY;
}


point ms_to_omt_copy( int x, int y )
{
    return point( divide( x, SEEX * 2 ), divide( y, SEEY * 2 ) );
}

tripoint ms_to_omt_copy( const tripoint &p )
{
    return tripoint( divide( p.x, SEEX * 2 ), divide( p.y, SEEY * 2 ), p.z );
}

void ms_to_omt( int &x, int &y )
{
    x = divide( x, SEEX * 2 );
    y = divide( y, SEEY * 2 );
}

point ms_to_omt_remain( int &x, int &y )
{
    return point( divide( x, SEEX * 2, x ), divide( y, SEEY * 2, y ) );
}

tripoint omt_to_seg_copy( const tripoint &p )
{
    return tripoint( divide( p.x, SEG_SIZE ), divide( p.y, SEG_SIZE ), p.z );
}

void limit_and_loop_om_coordinates( int &x, int &y )
{
    if( get_option<int>( "WORLD_LIMIT_X" ) > 0 ) {
        x = x % get_option<int>( "WORLD_LIMIT_X" );
    }
    if( get_option<int>( "WORLD_LIMIT_Y" ) > 0 ) {
        y = y % get_option<int>( "WORLD_LIMIT_Y" );
    }
}
