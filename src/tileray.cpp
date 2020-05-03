#include "tileray.h"

#include <cmath>
#include <cstdlib>

#include "line.h"
#include "math_defines.h"

static const int sx[4] = { 1, -1, -1, 1 };
static const int sy[4] = { 1, 1, -1, -1 };

tileray::tileray(): leftover( 0 ), direction( 0 ), steps( 0 ), infinite( false )
{
}

tileray::tileray( const point &ad )
{
    init( ad );
}

tileray::tileray( int adir ): direction( adir )
{
    init( adir );
}

void tileray::init( const point &ad )
{
    delta = ad;
    abs_d = abs( delta );
    if( delta == point_zero ) {
        direction = 0;
    } else {
        direction = static_cast<int>( atan2_degrees( delta ) );
        if( direction < 0 ) {
            direction += 360;
        }
    }
    last_d = point_zero;
    steps = 0;
    infinite = false;
}

void tileray::init( int adir )
{
    leftover = 0;
    // Clamp adir to the range [0, 359]
    direction = ( adir < 0 ? 360 - ( ( -adir ) % 360 ) : adir % 360 );
    last_d = point_zero;
    float direction_radians = static_cast<float>( direction ) * M_PI / 180.0;
    rl_vec2d delta_f( std::cos( direction_radians ), std::sin( direction_radians ) );
    delta = ( delta_f * 100 ).as_point();
    abs_d = abs( delta );
    steps = 0;
    infinite = true;
}

void tileray::clear_advance()
{
    leftover = 0;
    last_d = point_zero;
    steps = 0;
}

int tileray::dx() const
{
    return last_d.x;
}

int tileray::dy() const
{
    return last_d.y;
}

int tileray::dir() const
{
    return direction;
}

int tileray::dir4() const
{
    if( direction >= 45 && direction <= 135 ) {
        return 1;
    } else if( direction > 135 && direction < 225 ) {
        return 2;
    } else if( direction >= 225 && direction <= 315 ) {
        return 3;
    } else {
        return 0;
    }
}

int tileray::dir8() const
{
    int oct = 0;
    int dir = direction;
    if( dir < 23 || dir > 337 ) {
        return 0;
    }
    while( dir > 22 ) {
        dir -= 45;
        oct += 1;
    }
    return oct;
}

// This function assumes a vehicle is being drawn.
// It assumes horizontal lines are never skewed, vertical lines often skewed.
int tileray::dir_symbol( int sym ) const
{
    switch( sym ) {
        // output.cpp special_symbol() converts yubn to corners, hj to lines, c to cross
        case 'j':
            // vertical line
            return "h\\j/h\\j/"[dir8()];
        case 'h':
            // horizontal line
            return "jhjh"[dir4()];
        case 'y':
            // top left corner
            return "unby"[dir4()];
        case 'u':
            // top right corner
            return "nbyu"[dir4()];
        case 'n':
            // bottom right corner
            return "byun"[dir4()];
        case 'b':
            // bottom left corner
            return "yunb"[dir4()];
        case '^':
            return ">v<^"[dir4()];
        case '>':
            return "v<^>"[dir4()];
        case 'v':
            return "<^>v"[dir4()];
        case '<':
            return "^>v<"[dir4()];
        case 'c':
            // +
            return "cXcXcXcX"[dir8()];
        case 'X':
            return "XcXcXcXc"[dir8()];

        case '[':
            // [ not rotated to ] because they might represent different items
            return "-\\[/-\\[/"[dir8()];
        case ']':
            return "-\\]/-\\]/"[dir8()];
        case '|':
            return "-\\|/-\\|/"[dir8()];
        case '-':
            return "|/-\\|/-\\"[dir8()];
        case '=':
            return "H=H="[dir4()];
        case 'H':
            return "=H=H"[dir4()];
        case '\\':
            return "/-\\|/-\\|"[dir8()];
        case '/':
            return "\\|/-\\|/-"[dir8()];
        default:
            ;
    }
    return sym;
}

int tileray::ortho_dx( int od ) const
{
    int quadr = ( direction / 90 ) % 4;
    od *= -sy[quadr];
    return mostly_vertical() ? od : 0;
}

int tileray::ortho_dy( int od ) const
{
    int quadr = ( direction / 90 ) % 4;
    od *= sx[quadr];
    return mostly_vertical() ? 0 : od;
}

bool tileray::mostly_vertical() const
{
    return abs_d.x <= abs_d.y;
}

void tileray::advance( int num )
{
    last_d = point_zero;
    if( num == 0 ) {
        return;
    }
    int anum = std::abs( num );
    steps = anum;
    const bool vertical = mostly_vertical();
    if( direction % 90 ) {
        for( int i = 0; i < anum; i++ ) {
            if( vertical ) {
                // mostly vertical line
                leftover += abs_d.x;
                if( leftover >= abs_d.y ) {
                    last_d.x++;
                    leftover -= abs_d.y;
                }
            } else {
                // mostly horizontal line
                leftover += abs_d.y;
                if( leftover >= abs_d.x ) {
                    last_d.y++;
                    leftover -= abs_d.x;
                }
            }
        }
    }
    if( vertical ) {
        last_d.y = anum;
    } else {
        last_d.x = anum;
    }

    // offset calculated for 0-90 deg quadrant, we need to adjust if direction is other
    int quadr = ( direction / 90 ) % 4;
    last_d.x *= sx[quadr];
    last_d.y *= sy[quadr];
    if( num < 0 ) {
        last_d = -last_d;
    }
}

bool tileray::end()
{
    if( infinite ) {
        return true;
    }
    return mostly_vertical() ? steps >= abs_d.y - 1 : steps >= abs_d.x - 1;
}
