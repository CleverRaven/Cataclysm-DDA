#include "calendar.h"
#include "catch/catch.hpp"
#include "options_helpers.h"

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

TEST_CASE( "time_duration_to_string", "[calendar]" )
{
    CHECK( to_string( 10_seconds ) == "10 seconds" );
    CHECK( to_string( 60_seconds ) == "1 minute" );
    CHECK( to_string( 70_seconds ) == "1 minute and 10 seconds" );
    CHECK( to_string( 60_minutes ) == "1 hour" );
    CHECK( to_string( 70_minutes ) == "1 hour and 10 minutes" );
    CHECK( to_string( 24_hours ) == "1 day" );
    CHECK( to_string( 24_hours + 1_seconds ) == "1 day and 1 second" );
    CHECK( to_string( 25_hours ) == "1 day and 1 hour" );
    CHECK( to_string( 25_hours + 1_seconds ) == "1 day and 1 hour" );
    CHECK( to_string( 7_days ) == "1 week" );
    CHECK( to_string( 8_days ) == "1 week and 1 day" );
    CHECK( to_string( 91_days ) == "1 season" );
    CHECK( to_string( 92_days ) == "1 season and 1 day" );
    CHECK( to_string( 99_days ) == "1 season and 1 week" );
    CHECK( to_string( 364_days ) == "1 year" );
    CHECK( to_string( 365_days ) == "1 year and 1 day" );
    CHECK( to_string( 465_days ) == "1 year and 1 season" );
    CHECK( to_string( 3650_days ) == "10 years and 1 week" );
}

TEST_CASE( "time_duration_to_string_eternal_season", "[calendar]" )
{
    calendar::set_eternal_season( true );
    CHECK( to_string( 7_days ) == "1 week" );
    CHECK( to_string( 8_days ) == "1 week and 1 day" );
    CHECK( to_string( 91_days ) == "13 weeks" );
    CHECK( to_string( 92_days ) == "13 weeks and 1 day" );
    CHECK( to_string( 99_days ) == "14 weeks and 1 day" );
    CHECK( to_string( 364_days ) == "52 weeks" );
    CHECK( to_string( 365_days ) == "52 weeks and 1 day" );
    CHECK( to_string( 465_days ) == "66 weeks and 3 days" );
    CHECK( to_string( 3650_days ) == "521 weeks and 3 days" );
    calendar::set_eternal_season( false );
}
