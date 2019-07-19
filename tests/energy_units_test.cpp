#include <sstream>
#include <string>

#include "catch/catch.hpp"
#include "json.h"
#include "units.h"

static units::energy parse_energy_quantity( const std::string &json )
{
    std::istringstream buffer( json );
    JsonIn jsin( buffer );
    return read_from_json_string<units::energy>( jsin, units::energy_units );
}

TEST_CASE( "energy parsing from JSON" )
{
    CHECK_THROWS( parse_energy_quantity( "\"\"" ) ); // empty string
    CHECK_THROWS( parse_energy_quantity( "27" ) ); // not a string at all
    CHECK_THROWS( parse_energy_quantity( "\"    \"" ) ); // only spaces
    CHECK_THROWS( parse_energy_quantity( "\"27\"" ) ); // no energy unit

    REQUIRE( parse_energy_quantity( "\"1 mJ\"" ) == 1_mJ );
    REQUIRE( parse_energy_quantity( "\"1 J\"" ) == 1_J );
    REQUIRE( parse_energy_quantity( "\"1 kJ\"" ) == 1_kJ );
    REQUIRE( parse_energy_quantity( "\"+1 mJ\"" ) == 1_mJ );
    REQUIRE( parse_energy_quantity( "\"+1 J\"" ) == 1_J );
    REQUIRE( parse_energy_quantity( "\"+1 kJ\"" ) == 1_kJ );

    REQUIRE( parse_energy_quantity( "\"1 mJ 1 J 1 kJ\"" ) == 1_mJ + 1_J + 1_kJ );
    REQUIRE( parse_energy_quantity( "\"1 mJ -4 J 1 kJ\"" ) == 1_mJ - 4_J + 1_kJ );

    REQUIRE( 1_mJ * 1000 == units::from_joule( 1 ) );
    REQUIRE( 1_J * 1000 == units::from_kilojoule( 1 ) );
}
