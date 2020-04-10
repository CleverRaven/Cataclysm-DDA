#include <sstream>
#include <string>
#include <vector>

#include "calendar.h"
#include "catch/catch.hpp"
#include "json.h"
#include "units.h"

TEST_CASE( "units_have_correct_ratios", "[units]" )
{
    CHECK( 1_liter == 1000_ml );
    CHECK( 1.0_liter == 1000.0_ml );
    CHECK( 1_gram == 1000_milligram );
    CHECK( 1.0_gram == 1000.0_milligram );
    CHECK( 1_kilogram == 1000_gram );
    CHECK( 1.0_kilogram == 1000.0_gram );
    CHECK( 1_J == 1000_mJ );
    CHECK( 1.0_J == 1000.0_mJ );
    CHECK( 1_kJ == 1000_J );
    CHECK( 1.0_kJ == 1000.0_J );
    CHECK( 1_USD == 100_cent );
    CHECK( 1.0_USD == 100.0_cent );
    CHECK( 1_kUSD == 1000_USD );
    CHECK( 1.0_kUSD == 1000.0_USD );
    CHECK( 1_days == 24_hours );
    CHECK( 1_hours == 60_minutes );
    CHECK( 1_minutes == 60_seconds );

    CHECK( 1_mJ == units::from_millijoule( 1 ) );
    CHECK( 1_J == units::from_joule( 1 ) );
    CHECK( 1_kJ == units::from_kilojoule( 1 ) );

    CHECK( 1_seconds == time_duration::from_seconds( 1 ) );
    CHECK( 1_minutes == time_duration::from_minutes( 1 ) );
    CHECK( 1_hours == time_duration::from_hours( 1 ) );
    CHECK( 1_days == time_duration::from_days( 1 ) );
}

static units::energy parse_energy_quantity( const std::string &json )
{
    std::istringstream buffer( json );
    JsonIn jsin( buffer );
    return read_from_json_string<units::energy>( jsin, units::energy_units );
}

TEST_CASE( "energy parsing from JSON", "[units]" )
{
    CHECK_THROWS( parse_energy_quantity( "\"\"" ) ); // empty string
    CHECK_THROWS( parse_energy_quantity( "27" ) ); // not a string at all
    CHECK_THROWS( parse_energy_quantity( "\"    \"" ) ); // only spaces
    CHECK_THROWS( parse_energy_quantity( "\"27\"" ) ); // no energy unit

    CHECK( parse_energy_quantity( "\"1 mJ\"" ) == 1_mJ );
    CHECK( parse_energy_quantity( "\"1 J\"" ) == 1_J );
    CHECK( parse_energy_quantity( "\"1 kJ\"" ) == 1_kJ );
    CHECK( parse_energy_quantity( "\"+1 mJ\"" ) == 1_mJ );
    CHECK( parse_energy_quantity( "\"+1 J\"" ) == 1_J );
    CHECK( parse_energy_quantity( "\"+1 kJ\"" ) == 1_kJ );

    CHECK( parse_energy_quantity( "\"1 mJ 1 J 1 kJ\"" ) == 1_mJ + 1_J + 1_kJ );
    CHECK( parse_energy_quantity( "\"1 mJ -4 J 1 kJ\"" ) == 1_mJ - 4_J + 1_kJ );
}

static time_duration parse_time_duration( const std::string &json )
{
    std::istringstream buffer( json );
    JsonIn jsin( buffer );
    return read_from_json_string<time_duration>( jsin, time_duration::units );
}

TEST_CASE( "time_duration parsing from JSON", "[units]" )
{
    CHECK_THROWS( parse_time_duration( "\"\"" ) ); // empty string
    CHECK_THROWS( parse_time_duration( "27" ) ); // not a string at all
    CHECK_THROWS( parse_time_duration( "\"    \"" ) ); // only spaces
    CHECK_THROWS( parse_time_duration( "\"27\"" ) ); // no time unit

    CHECK( parse_time_duration( "\"1 turns\"" ) == 1_turns );
    CHECK( parse_time_duration( "\"1 minutes\"" ) == 1_minutes );
    CHECK( parse_time_duration( "\"+1 hours\"" ) == 1_hours );
    CHECK( parse_time_duration( "\"+1 days\"" ) == 1_days );

    CHECK( parse_time_duration( "\"1 turns 1 minutes 1 hours 1 days\"" ) == 1_turns + 1_minutes +
           1_hours + 1_days );
    CHECK( parse_time_duration( "\"1 turns -4 minutes 1 hours -4 days\"" ) == 1_turns - 4_minutes +
           1_hours - 4_days );
}
