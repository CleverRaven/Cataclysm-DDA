#include "catch/catch.hpp"

#include "calendar.h"
#include "rng.h"
#include "stringmaker.h"

TEST_CASE( "moon_phases_take_28_days", "[calendar]" )
{
    CAPTURE( calendar::season_length() );
    // This test only makes sense if the seasons are set to the default length
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    const int num_days = GENERATE( take( 100, random( 0, 1000 ) ) );
    const time_point first_time = calendar::turn_zero + time_duration::from_days( num_days );
    const time_point later_14_days = first_time + 14_days;
    const time_point later_29_days = first_time + 29_days;
    const time_point later_30_days = first_time + 30_days;

    CAPTURE( num_days );
    CHECK( get_moon_phase( first_time ) != get_moon_phase( later_14_days ) );
    // Phase should match either 29 or 30 days later
    CHECK( ( get_moon_phase( first_time ) == get_moon_phase( later_29_days ) ||
             get_moon_phase( first_time ) == get_moon_phase( later_30_days ) ) );
}

TEST_CASE( "moon_phase_changes_at_noon", "[calendar]" )
{
    // This test only makes sense if the seasons are set to the default length
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );

    const int num_days = GENERATE( take( 100, random( 0, 1000 ) ) );
    const time_point midnight = calendar::turn_zero + time_duration::from_days( num_days );
    const time_point earlier_11_hours = midnight - 11_hours;
    const time_point later_11_hours = midnight + 11_hours;

    CAPTURE( num_days );
    CHECK( get_moon_phase( earlier_11_hours ) == get_moon_phase( later_11_hours ) );
}
