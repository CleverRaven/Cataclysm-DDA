#include "calendar.h"

#include "catch/catch.hpp"

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

TEST_CASE( "time_point_to_string", "[calendar]" )
{
    CHECK( to_string( calendar::turn_zero ) == "Year 1, Spring, day 1 12:00:00AM" );
}
