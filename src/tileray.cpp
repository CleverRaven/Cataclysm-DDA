#include "tileray.h"

#include <cmath>
#include <cstdlib>
#include <string>

#include "line.h"
#include "units.h"
#include "units_utility.h"

static const int sx[4] = { 1, -1, -1, 1 };
static const int sy[4] = { 1, 1, -1, -1 };

tileray::tileray() = default;

tileray::tileray( const point &ad )
{
    init( ad );
}

tileray::tileray( units::angle adir ): direction( adir )
{
    init( adir );
}

void tileray::init( const point &ad )
{
    delta = ad;
    abs_d = delta.abs();
    if( delta == point_zero ) {
        direction = 0_degrees;
    } else {
        direction = atan2( delta );
        if( direction < 0_degrees ) {
            direction += 360_degrees;
        }
    }
    last_d = point_zero;
    steps = 0;
    infinite = false;
}

void tileray::init( const units::angle &adir )
{
    leftover = 0;
    // Clamp adir to the range [0, 360)
    direction = normalize( adir );
    last_d = point_zero;
    rl_vec2d delta_f( units::cos( direction ), units::sin( direction ) );
    delta = ( delta_f * 100 ).as_point();
    abs_d = delta.abs();
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

units::angle tileray::dir() const
{
    return direction;
}

int tileray::quadrant() const
{
    return static_cast<int>( std::floor( direction / 90_degrees ) ) % 4;
}

int tileray::dir4() const
{
    if( direction >= 45_degrees && direction <= 135_degrees ) {
        return 1;
    } else if( direction > 135_degrees && direction < 225_degrees ) {
        return 2;
    } else if( direction >= 225_degrees && direction <= 315_degrees ) {
        return 3;
    } else {
        return 0;
    }
}

int tileray::dir8() const
{
    int oct = 0;
    units::angle dir = direction;
    if( dir < 23_degrees || dir > 337_degrees ) {
        return 0;
    }
    while( dir > 22_degrees ) {
        dir -= 45_degrees;
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

std::string tileray::to_string_azimuth_from_north() const
{
    return std::to_string( std::lround( to_degrees( dir() + 90_degrees ) ) % 360 ) + "°";
}

int tileray::ortho_dx( int od ) const
{
    od *= -sy[quadrant()];
    return mostly_vertical() ? od : 0;
}

int tileray::ortho_dy( int od ) const
{
    od *= sx[quadrant()];
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
    steps += anum;
    const bool vertical = mostly_vertical();
    if( abs_d.x && abs_d.y ) {
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
    int quadr = quadrant();
    last_d.x *= sx[quadr];
    last_d.y *= sy[quadr];
    if( num < 0 ) {
        last_d = -last_d;
    }
}

int tileray::get_steps() const
{
    return steps;
}
