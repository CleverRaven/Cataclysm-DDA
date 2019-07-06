#include <sstream>
#include <string>

#include "catch/catch.hpp"
#include "calendar.h"
#include "json.h"
#include "units.h"

static time_duration parse_time_duration( const std::string &json )
{
    std::istringstream buffer( json );
    JsonIn jsin( buffer );
    return read_from_json_string<time_duration>( jsin, time_duration::units );
}

TEST_CASE( "time_duration parsing from JSON" )
{
    CHECK_THROWS( parse_time_duration( "\"\"" ) ); // empty string
    CHECK_THROWS( parse_time_duration( "27" ) ); // not a string at all
    CHECK_THROWS( parse_time_duration( "\"    \"" ) ); // only spaces
    CHECK_THROWS( parse_time_duration( "\"27\"" ) ); // no time unit

    REQUIRE( parse_time_duration( "\"1 turns\"" ) == 1_turns );
    REQUIRE( parse_time_duration( "\"1 minutes\"" ) == 1_minutes );
    REQUIRE( parse_time_duration( "\"+1 hours\"" ) == 1_hours );
    REQUIRE( parse_time_duration( "\"+1 days\"" ) == 1_days );

    REQUIRE( parse_time_duration( "\"1 turns 1 minutes 1 hours 1 days\"" ) == 1_turns + 1_minutes +
             1_hours + 1_days );
    REQUIRE( parse_time_duration( "\"1 turns -4 minutes 1 hours -4 days\"" ) == 1_turns - 4_minutes +
             1_hours - 4_days );

    REQUIRE( 1_turns * 60 == time_duration::from_minutes( 1 ) );
    REQUIRE( 1_minutes * 60 == time_duration::from_hours( 1 ) );
    REQUIRE( 1_hours * 24 == time_duration::from_days( 1 ) );
}
