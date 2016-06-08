#include "scent_map.h"
#include "calendar.h"
#include "color.h"
#include "map.h"
#include "output.h"

nc_color sev( const int a )
{
    switch( a ) {
        case 0:
            return c_cyan;
        case 1:
            return c_ltcyan;
        case 2:
            return c_ltblue;
        case 3:
            return c_blue;
        case 4:
            return c_ltgreen;
        case 5:
            return c_green;
        case 6:
            return c_yellow;
        case 7:
            return c_pink;
        case 8:
            return c_ltred;
        case 9:
            return c_red;
        case 10:
            return c_magenta;
        case 11:
            return c_brown;
        case 12:
            return c_cyan_red;
        case 13:
            return c_ltcyan_red;
        case 14:
            return c_ltblue_red;
        case 15:
            return c_blue_red;
        case 16:
            return c_ltgreen_red;
        case 17:
            return c_green_red;
        case 18:
            return c_yellow_red;
        case 19:
            return c_pink_red;
        case 20:
            return c_magenta_red;
        case 21:
            return c_brown_red;
    }
    return c_dkgray;
}

scent_map::scent_map() = default;

void scent_map::reset()
{
    for( auto &elem : grscent ) {
        for( auto &val : elem ) {
            val = 0;
        }
    }
}

void scent_map::decay()
{
    for( auto &elem : grscent ) {
        for( auto &val : elem ) {
            val = std::max( 0, val - 1 );
        }
    }
}

void scent_map::draw( WINDOW *const win, const int div, const tripoint &center ) const
{
    const int maxx = getmaxx( win );
    const int maxy = getmaxy( win );
    for( int x = 0; x < maxx; ++x ) {
        for( int y = 0; y < maxy; ++y ) {
            const int sn = operator()( x + center.x - maxx / 2, y + center.y - maxy / 2 ) / div;
            mvwprintz( win, y, x, sev( sn / 10 ), "%d", sn % 10 );
        }
    }
}

void scent_map::shift( const int sm_shift_x, const int sm_shift_y )
{
    scent_array<int> new_scent;
    for( size_t x = 0; x < SEEX * MAPSIZE; ++x ) {
        for( size_t y = 0; y < SEEY * MAPSIZE; ++y ) {
            // operator() does bound checking and returns 0 upon invalid coordinates
            new_scent[x][y] = operator()( x + sm_shift_x, y + sm_shift_y );
        }
    }
    grscent = new_scent;
}

int scent_map::operator()( const size_t x, const size_t y ) const
{
    if( inbounds( x, y ) ) {
        return grscent[x][y];
    }
    return 0;
}

int &scent_map::operator()( const size_t x, const size_t y )
{
    if( inbounds( x, y ) ) {
        return grscent[x][y];
    }
    null_scent = 0;
    return null_scent;
}
