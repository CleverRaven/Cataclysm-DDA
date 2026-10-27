#include "touch_joystick.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <string>

#include "math_defines.h"

namespace touch_joystick
{

namespace
{

constexpr std::array<direction, 4> straight_dirs = {
    direction::right, direction::down, direction::left, direction::up
};
constexpr std::array<direction, 4> diagonal_dirs = {
    direction::down_right, direction::down_left, direction::up_left, direction::up_right
};

// half a straight slice, clamped so a diagonal slice never goes negative
float half_width( const float straight_width )
{
    return std::clamp( straight_width, 0.0f, 90.0f ) * 0.5f;
}

} // namespace

direction classify( const float dx, const float dy, const float deadzone,
                    const float straight_width )
{
    if( dx * dx + dy * dy <= deadzone * deadzone ) {
        return direction::none;
    }
    const float degrees = std::atan2( dy, dx ) * 180.0f / static_cast<float>( M_PI );
    // each quarter turn starts at the first edge of its straight slice
    const float turned = std::fmod( degrees + half_width( straight_width ) + 360.0f, 360.0f );
    const int quarter = std::min( 3, static_cast<int>( turned / 90.0f ) );
    const float within = turned - quarter * 90.0f;
    if( within < 2.0f * half_width( straight_width ) ) {
        return straight_dirs[quarter];
    }
    return diagonal_dirs[quarter];
}

std::pair<float, float> wedge( const direction dir, const float straight_width )
{
    const float half = half_width( straight_width );
    for( int quarter = 0; quarter < 4; ++quarter ) {
        const float center = quarter * 90.0f;
        if( dir == straight_dirs[quarter] ) {
            return { center - half, center + half };
        }
        if( dir == diagonal_dirs[quarter] ) {
            return { center + half, center + 90.0f - half };
        }
    }
    return { 0.0f, 0.0f };
}

float speed_fraction( const float dist, const float deadzone, const float range )
{
    if( range <= 0.0f ) {
        return dist > deadzone ? 1.0f : 0.0f;
    }
    return std::clamp( ( dist - deadzone ) / range, 0.0f, 1.0f );
}

float octagon_radius( const float degrees, const float apothem )
{
    // angle from the center of the nearest flat side
    const float from_side = std::fmod( std::fmod( degrees, 45.0f ) + 45.0f + 22.5f, 45.0f ) - 22.5f;
    return apothem / std::cos( from_side * static_cast<float>( M_PI ) / 180.0f );
}

std::vector<float> band_angles( const float start, const float end )
{
    std::vector<float> angles = { start };
    // corners are halfway between straight and diagonal
    const int first_corner = static_cast<int>( std::ceil( ( start - 22.5f ) / 45.0f ) );
    for( int k = first_corner; k * 45.0f + 22.5f < end; ++k ) {
        const float corner = k * 45.0f + 22.5f;
        if( corner > start ) {
            angles.push_back( corner );
        }
    }
    angles.push_back( end );
    return angles;
}

bool is_bare_context( const std::vector<std::string> &actions )
{
    return std::all_of( actions.begin(), actions.end(), []( const std::string & action ) {
        return action == "toggle_language_to_en";
    } );
}

} // namespace touch_joystick
