#include <cmath>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

#include "cata_catch.h"
#include "math_defines.h"
#include "touch_joystick.h"

using touch_joystick::direction;

// offset of a touch at the given angle, clockwise from +x with y down
static void offset_at( const float degrees, const float radius, float &dx, float &dy )
{
    const float radians = degrees * static_cast<float>( M_PI ) / 180.0f;
    dx = radius * std::cos( radians );
    dy = radius * std::sin( radians );
}

// straight slice widths: the slope 0.5 and 2 split, a wider one, and four directions only
static constexpr float slope_split_width = 53.13f;
static constexpr float wide_width = 55.0f;
static constexpr float four_way = 90.0f;

static direction classify_at( const float degrees, const float straight_width )
{
    float dx = 0.0f;
    float dy = 0.0f;
    offset_at( degrees, 100.0f, dx, dy );
    return touch_joystick::classify( dx, dy, 10.0f, straight_width );
}

TEST_CASE( "touch_joystick_deadzone", "[touch_joystick]" )
{
    CHECK( touch_joystick::classify( 0.0f, 0.0f, 10.0f, wide_width ) == direction::none );
    // edge is still inside the deadzone
    CHECK( touch_joystick::classify( 6.0f, 8.0f, 10.0f, wide_width ) == direction::none );
    CHECK( touch_joystick::classify( 6.0f, 8.1f, 10.0f, wide_width ) != direction::none );
}

TEST_CASE( "touch_joystick_eight_directions", "[touch_joystick]" )
{
    GIVEN( "a touch at the center of each slice" ) {
        THEN( "maps to that slice's direction" ) {
            CHECK( classify_at( 0.0f, wide_width ) == direction::right );
            CHECK( classify_at( 45.0f, wide_width ) == direction::down_right );
            CHECK( classify_at( 90.0f, wide_width ) == direction::down );
            CHECK( classify_at( 135.0f, wide_width ) == direction::down_left );
            CHECK( classify_at( 180.0f, wide_width ) == direction::left );
            CHECK( classify_at( 225.0f, wide_width ) == direction::up_left );
            CHECK( classify_at( 270.0f, wide_width ) == direction::up );
            CHECK( classify_at( 315.0f, wide_width ) == direction::up_right );
        }
    }
    GIVEN( "slope split of 0.5 and 2" ) {
        THEN( "boundary at atan 0.5" ) {
            CHECK( classify_at( 26.4f, slope_split_width ) == direction::right );
            CHECK( classify_at( 26.7f, slope_split_width ) == direction::down_right );
            CHECK( classify_at( 63.3f, slope_split_width ) == direction::down_right );
            CHECK( classify_at( 63.6f, slope_split_width ) == direction::down );
        }
    }
    GIVEN( "touches beside a slice boundary" ) {
        THEN( "straight slices span 55 degrees and diagonal slices 35" ) {
            CHECK( classify_at( 27.0f, wide_width ) == direction::right );
            CHECK( classify_at( 28.0f, wide_width ) == direction::down_right );
            CHECK( classify_at( 62.0f, wide_width ) == direction::down_right );
            CHECK( classify_at( 63.0f, wide_width ) == direction::down );
            CHECK( classify_at( 333.0f, wide_width ) == direction::right );
            CHECK( classify_at( 332.0f, wide_width ) == direction::up_right );
        }
    }
}

TEST_CASE( "touch_joystick_four_directions", "[touch_joystick]" )
{
    CHECK( classify_at( 44.0f, four_way ) == direction::right );
    CHECK( classify_at( 46.0f, four_way ) == direction::down );
    CHECK( classify_at( 134.0f, four_way ) == direction::down );
    CHECK( classify_at( 136.0f, four_way ) == direction::left );
    CHECK( classify_at( 224.0f, four_way ) == direction::left );
    CHECK( classify_at( 226.0f, four_way ) == direction::up );
    CHECK( classify_at( 314.0f, four_way ) == direction::up );
    CHECK( classify_at( 316.0f, four_way ) == direction::right );
}

TEST_CASE( "touch_joystick_wedges", "[touch_joystick]" )
{
    CHECK( touch_joystick::wedge( direction::right, wide_width ).first == Approx( -27.5f ) );
    CHECK( touch_joystick::wedge( direction::right, wide_width ).second == Approx( 27.5f ) );
    CHECK( touch_joystick::wedge( direction::down_right, wide_width ).first == Approx( 27.5f ) );
    CHECK( touch_joystick::wedge( direction::down_right, wide_width ).second == Approx( 62.5f ) );
    CHECK( touch_joystick::wedge( direction::up_right, wide_width ).first == Approx( 297.5f ) );
    CHECK( touch_joystick::wedge( direction::up_right, wide_width ).second == Approx( 332.5f ) );
    CHECK( touch_joystick::wedge( direction::right, four_way ).first == Approx( -45.0f ) );
    CHECK( touch_joystick::wedge( direction::down, four_way ).second == Approx( 135.0f ) );
}

TEST_CASE( "touch_joystick_highlight_matches_input", "[touch_joystick]" )
{
    for( const float straight_width : {
             20.0f, slope_split_width, wide_width, 75.0f, four_way
         } ) {
        for( int step = 0; step < 720; ++step ) {
            const float degrees = step * 0.5f + 0.25f;
            CAPTURE( straight_width, degrees );
            const direction dir = classify_at( degrees, straight_width );
            REQUIRE( dir != direction::none );
            const std::pair<float, float> slice = touch_joystick::wedge( dir, straight_width );
            // bring angle into slice's turn
            float angle = degrees;
            if( angle >= slice.second ) {
                angle -= 360.0f;
            }
            CHECK( angle >= slice.first );
            CHECK( angle < slice.second );
        }
    }
}

TEST_CASE( "touch_joystick_speed_fraction", "[touch_joystick]" )
{
    CHECK( touch_joystick::speed_fraction( 5.0f, 10.0f, 40.0f ) == Approx( 0.0f ) );
    CHECK( touch_joystick::speed_fraction( 10.0f, 10.0f, 40.0f ) == Approx( 0.0f ) );
    CHECK( touch_joystick::speed_fraction( 30.0f, 10.0f, 40.0f ) == Approx( 0.5f ) );
    CHECK( touch_joystick::speed_fraction( 50.0f, 10.0f, 40.0f ) == Approx( 1.0f ) );
    CHECK( touch_joystick::speed_fraction( 90.0f, 10.0f, 40.0f ) == Approx( 1.0f ) );
    CHECK( touch_joystick::speed_fraction( 30.0f, 10.0f, 0.0f ) == Approx( 1.0f ) );
}

TEST_CASE( "touch_joystick_octagon_geometry", "[touch_joystick]" )
{
    GIVEN( "octagon with flat sides facing straight directions" ) {
        THEN( "radius = apothem on a side, longer at a corner" ) {
            CHECK( touch_joystick::octagon_radius( 0.0f, 10.0f ) == Approx( 10.0f ) );
            CHECK( touch_joystick::octagon_radius( 90.0f, 10.0f ) == Approx( 10.0f ) );
            CHECK( touch_joystick::octagon_radius( 22.5f, 10.0f ) ==
                   Approx( 10.0f / std::cos( 22.5f * static_cast<float>( M_PI ) / 180.0f ) ) );
            CHECK( touch_joystick::octagon_radius( -22.5f, 10.0f ) ==
                   Approx( touch_joystick::octagon_radius( 22.5f, 10.0f ) ) );
            // between a corner and the next side the radius follows that side
            CHECK( touch_joystick::octagon_radius( 30.0f, 10.0f ) ==
                   Approx( 10.0f / std::cos( 15.0f * static_cast<float>( M_PI ) / 180.0f ) ) );
            CHECK( touch_joystick::octagon_radius( 40.0f, 10.0f ) ==
                   Approx( 10.0f / std::cos( 5.0f * static_cast<float>( M_PI ) / 180.0f ) ) );
        }
    }
    GIVEN( "a slice of the octagon" ) {
        THEN( "its outline samples the edges and every corner between them" ) {
            const std::vector<float> straight = touch_joystick::band_angles( -25.0f, 25.0f );
            REQUIRE( straight.size() == 4 );
            CHECK( straight[0] == Approx( -25.0f ) );
            CHECK( straight[1] == Approx( -22.5f ) );
            CHECK( straight[2] == Approx( 22.5f ) );
            CHECK( straight[3] == Approx( 25.0f ) );
            const std::vector<float> diagonal = touch_joystick::band_angles( 25.0f, 65.0f );
            REQUIRE( diagonal.size() == 2 );
            CHECK( diagonal[0] == Approx( 25.0f ) );
            CHECK( diagonal[1] == Approx( 65.0f ) );
        }
    }
}

TEST_CASE( "touch_joystick_bare_context", "[touch_joystick]" )
{
    GIVEN( "only the action every context registers on construction" ) {
        THEN( "it's bare" ) {
            CHECK( touch_joystick::is_bare_context( { "toggle_language_to_en" } ) );
            CHECK( touch_joystick::is_bare_context( {} ) );
        }
    }
    GIVEN( "actions of its own" ) {
        THEN( "it's not bare" ) {
            CHECK_FALSE( touch_joystick::is_bare_context( { "toggle_language_to_en", "LEFTUP" } ) );
            CHECK_FALSE( touch_joystick::is_bare_context( { "QUIT" } ) );
        }
    }
}
