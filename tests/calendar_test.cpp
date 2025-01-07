#include <iosfwd>
#include <string>

#include "calendar.h"
#include "cata_catch.h"

TEST_CASE( "time_duration_to_string", "[calendar][nogame]" )
{
    calendar::set_season_length( 91 );
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

TEST_CASE( "time_duration_to_string_eternal_season", "[calendar][nogame]" )
{
    calendar::set_season_length( 91 );
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
    REQUIRE( calendar::season_from_default_ratio() == Approx( 1.0f ) );
}
