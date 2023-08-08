#include "tileray.h"

#include <cmath>
#include <cstdlib>
#include <string>

#include "line.h"
#include "units.h"
#include "units_utility.h"

static constexpr std::array<int, 4> sx = { 1, -1, -1, 1 };
static constexpr std::array<int, 4> sy = { 1, 1, -1, -1 };

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
