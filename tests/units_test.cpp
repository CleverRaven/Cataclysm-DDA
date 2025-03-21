#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "cata_utility.h"
#include "flexbuffer_json.h"
#include "json_loader.h"
#include "math_defines.h"
#include "options_helpers.h"
#include "units.h"
#include "units_utility.h"

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

    CHECK( M_PI * 1_radians == 1_pi_radians );
    CHECK( 2_pi_radians == 360_degrees );
    CHECK( 60_arcmin == 1_degrees );

    CHECK( 1_mW == units::from_milliwatt( 1 ) );
    CHECK( 1_W == units::from_watt( 1 ) );
    CHECK( 1_kW == units::from_kilowatt( 1 ) );

    CHECK( 1_W * 1_seconds == 1_J );
    CHECK( 1_seconds * 1_W == 1_J );
    CHECK( 1_J / 1_seconds == 1_W );
    CHECK( 1_J / 1_W == 1_seconds );
    CHECK( 5_W * 5_minutes == 1.5_kJ );
    CHECK( -5_W * 5_minutes == -1.5_kJ );
    CHECK( ( 5_kJ / 5_W ) == ( 16_minutes + 40_seconds ) );
    CHECK( ( -5_kJ / 5_W ) == -( 16_minutes + 40_seconds ) );
    CHECK( ( 5_kJ / -5_W ) == -( 16_minutes + 40_seconds ) );
}

static units::energy parse_energy_quantity( const std::string &json )
{
    JsonValue jsin = json_loader::from_string( json );
    return read_from_json_string<units::energy>( jsin, units::energy_units );
}

TEST_CASE( "energy_parsing_from_JSON", "[units]" )
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

static units::power parse_power_quantity( const std::string &json )
{
    JsonValue jsin = json_loader::from_string( json );
    return read_from_json_string<units::power>( jsin, units::power_units );
}

TEST_CASE( "power_parsing_from_JSON", "[units]" )
{
    CHECK_THROWS( parse_power_quantity( "\"\"" ) ); // empty string
    CHECK_THROWS( parse_power_quantity( "27" ) ); // not a string at all
    CHECK_THROWS( parse_power_quantity( "\"    \"" ) ); // only spaces
    CHECK_THROWS( parse_power_quantity( "\"27\"" ) ); // no energy unit

    CHECK( parse_power_quantity( "\"1 mW\"" ) == 1_mW );
    CHECK( parse_power_quantity( "\"1 W\"" ) == 1_W );
    CHECK( parse_power_quantity( "\"1 kW\"" ) == 1_kW );
    CHECK( parse_power_quantity( "\"+1 mW\"" ) == 1_mW );
    CHECK( parse_power_quantity( "\"+1 W\"" ) == 1_W );
    CHECK( parse_power_quantity( "\"+1 kW\"" ) == 1_kW );

    CHECK( parse_power_quantity( "\"1 mW 1 W 1 kW\"" ) == 1_mW + 1_W + 1_kW );
    CHECK( parse_power_quantity( "\"1 mW -4 W 1 kW\"" ) == 1_mW - 4_W + 1_kW );
}

static time_duration parse_time_duration( const std::string &json )
{
    JsonValue jsin = json_loader::from_string( json );
    return read_from_json_string<time_duration>( jsin, time_duration::units );
}

TEST_CASE( "time_duration_parsing_from_JSON", "[units]" )
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

// convert_length and length_units work in tandem to give a succinct expression of length in
// imperial or metric units, based on whether the length is evenly divisible by the natural
// units in each system (millimeter|centimeter|meter|kilometer, or inch|foot|yard|mile)
//
// convert_length takes a units::length, and returns an integer number of the most appropriate
// measurement unit for the configured DISTANCE_UNITS option, based on whether the length is evenly
// divisible by that unit.
//
// length_units returns the corresponding unit string for that measurement ("mm", "in.", "ft" etc.)
//
// Thus, together they prefer (say) "15 cm" over "150 mm", and "2 m" over "200 cm" for metric units,
// or "2 ft" over "24 in.", and "1 yd" over "36 in.", and "1 mi" over "5280 ft" for imperial units.
//
TEST_CASE( "convert_length", "[units][convert][length]" )
{
    SECTION( "imperial length" ) {
        override_option opt_imperial( "DISTANCE_UNITS", "imperial" );

        // Converting metric to imperial is always gonna be a bit messy.
        // Different compilers may have slightly different edge conditions.
        // For instance 254 mm is 10 inches on clang, but only 9 inches on MinGW.
        // Values are fudged slightly (by 1 mm) to make the tests less likely to fail.
        SECTION( "non-multiples of 12 inches expressed in inches" ) {
            // 1 inch == 25.4 mm
            CHECK( convert_length( 26_mm ) == 1 );
            CHECK( length_units( 26_mm ) == "in." );
            CHECK( convert_length( 255_mm ) == 10 );
            CHECK( length_units( 255_mm ) == "in." );
            CHECK( convert_length( 2541_mm ) == 100 );
            CHECK( length_units( 2541_mm ) == "in." );
        }
        SECTION( "multiples of 12 inches expressed in feet" ) {
            // 12 inches == 304.8 mm
            CHECK( convert_length( 305_mm ) == 1 );
            CHECK( length_units( 305_mm ) == "ft" );
            CHECK( convert_length( 610_mm ) == 2 );
            CHECK( length_units( 610_mm ) == "ft" );
        }
        SECTION( "multiples of 36 inches expressed in yards" ) {
            // 1 yard == 914.4 mm
            CHECK( convert_length( 915_mm ) == 1 );
            CHECK( length_units( 915_mm ) == "yd" );
            CHECK( convert_length( 1830_mm ) == 2 );
            CHECK( length_units( 1830_mm ) == "yd" );
        }
        SECTION( "multiples of 63360 inches expressed in miles" ) {
            // 1 mile == 1,609,344 mm
            CHECK( convert_length( 1609345_mm ) == 1 );
            CHECK( length_units( 1609345_mm ) == "mi" );
        }
    }

    SECTION( "metric length" ) {
        override_option opt_metric( "DISTANCE_UNITS", "metric" );

        SECTION( "non-multiples of 10 mm expressed in millimeters" ) {
            CHECK( convert_length( 1_mm ) == 1 );
            CHECK( length_units( 1_mm ) == "mm" );
            CHECK( convert_length( 51_mm ) == 51 );
            CHECK( length_units( 51_mm ) == "mm" );
            CHECK( convert_length( 99_mm ) == 99 );
            CHECK( length_units( 99_mm ) == "mm" );
        }
        SECTION( "multiples of 10 mm in centimeters" ) {
            CHECK( convert_length( 10_mm ) == 1 );
            CHECK( length_units( 10_mm ) == "cm" );
            CHECK( convert_length( 50_mm ) == 5 );
            CHECK( length_units( 50_mm ) == "cm" );
            CHECK( convert_length( 100_mm ) == 10 );
            CHECK( length_units( 100_mm ) == "cm" );
            CHECK( convert_length( 150_mm ) == 15 );
            CHECK( length_units( 150_mm ) == "cm" );
        }
        SECTION( "multiples of 1,000 mm expressed in meters" ) {
            CHECK( convert_length( 1000_mm ) == 1 );
            CHECK( length_units( 1000_mm ) == "m" );
            CHECK( convert_length( 5000_mm ) == 5 );
            CHECK( length_units( 5000_mm ) == "m" );
            CHECK( convert_length( 25000_mm ) == 25 );
            CHECK( length_units( 25000_mm ) == "m" );
        }
        SECTION( "multiples of 1,000,000 mm expressed in kilometers" ) {
            CHECK( convert_length( 1000000_mm ) == 1 );
            CHECK( length_units( 1000000_mm ) == "km" );
            CHECK( convert_length( 5000000_mm ) == 5 );
            CHECK( length_units( 5000000_mm ) == "km" );
        }
    }
}

// convert_volume takes an integer number of milliliters, and returns equivalent floating-point
// number of cups, liters, or quarts, depending on the configured VOLUME_UNITS option (c, l, or qt)
TEST_CASE( "convert_volume", "[units][convert][volume]" )
{
    SECTION( "convert to metric cups" ) {
        override_option opt_cup( "VOLUME_UNITS", "c" );
        CHECK( convert_volume( 250 ) == Approx( 1.0f ) );
        CHECK( convert_volume( 1000 ) == Approx( 4.0f ) );
        CHECK( convert_volume( 1500 ) == Approx( 6.0f ) );
        CHECK( convert_volume( 2000 ) == Approx( 8.0f ) );
    }

    SECTION( "convert to liters" ) {
        override_option opt_liter( "VOLUME_UNITS", "l" );
        CHECK( convert_volume( 1000 ) == Approx( 1.0f ) );
        CHECK( convert_volume( 1500 ) == Approx( 1.5f ) );
        CHECK( convert_volume( 2000 ) == Approx( 2.0f ) );
    }
    SECTION( "convert to quarts" ) {
        override_option opt_quart( "VOLUME_UNITS", "qt" );
        CHECK( convert_volume( 1000 ) == Approx( 1.057f ).margin( 0.001f ) );
        CHECK( convert_volume( 2000 ) == Approx( 2.114f ).margin( 0.001f ) );
    }
}

// convert_weight takes a units::mass weight, and returns equivalent floating-point number of
// kilograms or pounds, depending on the configured USE_METRIC_WEIGHTS option (kg or lbs).
TEST_CASE( "convert_weight", "[units][convert][weight]" )
{
    // Kilograms (metric)
    override_option opt_kg( "USE_METRIC_WEIGHTS", "kg" );
    CHECK( convert_weight( 100_gram ) == Approx( 0.1f ) );
    CHECK( convert_weight( 500_gram ) == Approx( 0.5f ) );
    CHECK( convert_weight( 1000_gram ) == Approx( 1.0f ) );
    CHECK( convert_weight( 5000_gram ) == Approx( 5.0f ) );

    // Pounds (imperial)
    override_option opt_lbs( "USE_METRIC_WEIGHTS", "lbs" );
    const double g_per_lb = 453.6;
    CHECK( convert_weight( 100_gram ) == Approx( 100.0f / g_per_lb ) );
    CHECK( convert_weight( 1000_gram ) == Approx( 1000.0f / g_per_lb ) );
    CHECK( convert_weight( 5000_gram ) == Approx( 5000.0f / g_per_lb ) );
}

// convert_velocity takes an integer in "internal units", and returns a floating-point number based
// on configured USE_METRIC_SPEEDS (mph, km/h, or t/t).
//
// For "km/h", return value is in different units depending on whether it is VEHICLE or WIND speed.
TEST_CASE( "convert_velocity", "[units][convert][velocity]" )
{
    // 100 internal units == 1 mph, for both VEHICLE and WIND type
    override_option opt_mph( "USE_METRIC_SPEEDS", "mph" );
    CHECK( convert_velocity( 100, VU_VEHICLE ) == Approx( 1.0f ) );
    CHECK( convert_velocity( 100, VU_WIND ) == Approx( 1.0f ) );

    // Tiles-per-turn == 1/4 of mph for both VEHICLE and WIND type
    override_option opt_tpt( "USE_METRIC_SPEEDS", "t/t" );
    CHECK( convert_velocity( 100, VU_VEHICLE ) == Approx( 0.25f ) );
    CHECK( convert_velocity( 100, VU_WIND ) == Approx( 0.25f ) );

    // For metric velocity, VEHICLE speed uses km/h, while WIND speed uses meters/sec
    override_option opt_kph( "USE_METRIC_SPEEDS", "km/h" );
    CHECK( convert_velocity( 100, VU_VEHICLE ) == Approx( 1.609f ) );
    CHECK( convert_velocity( 100, VU_WIND ) == Approx( 0.447f ) );
}

static units::angle parse_angle( const std::string &json )
{
    JsonValue jsin = json_loader::from_string( json );
    return read_from_json_string<units::angle>( jsin, units::angle_units );
}

TEST_CASE( "angle_parsing_from_JSON", "[units]" )
{
    CHECK_THROWS( parse_angle( "\"\"" ) ); // empty string
    CHECK_THROWS( parse_angle( "27" ) ); // not a string at all
    CHECK_THROWS( parse_angle( "\"    \"" ) ); // only spaces
    CHECK_THROWS( parse_angle( "\"27\"" ) ); // no time unit

    CHECK( parse_angle( "\"1 rad\"" ) == 1_radians );
    CHECK( parse_angle( "\"1 °\"" ) == 1_degrees );
    CHECK( parse_angle( "\"+1 arcmin\"" ) == 1_arcmin );
}

TEST_CASE( "angles_to_trig_functions" )
{
    CHECK( sin( 0_radians ) == 0 );
    CHECK( sin( 0.5_pi_radians ) == Approx( 1 ) );
    CHECK( sin( 270_degrees ) == Approx( -1 ) );
    CHECK( cos( 1_pi_radians ) == Approx( -1 ) );
    CHECK( cos( 360_degrees ) == Approx( 1 ) );
    CHECK( units::atan2( 0, -1 ) == 1_pi_radians );
    CHECK( units::atan2( 0, 1 ) == 0_radians );
    CHECK( units::atan2( 1, 0 ) == 90_degrees );
    CHECK( units::atan2( -1, 0 ) == -90_degrees );
}

TEST_CASE( "rounding" )
{
    CHECK( round_to_multiple_of( 0_degrees, 15_degrees ) == 0_degrees );
    CHECK( round_to_multiple_of( 1_degrees, 15_degrees ) == 0_degrees );
    CHECK( round_to_multiple_of( 7_degrees, 15_degrees ) == 0_degrees );
    CHECK( round_to_multiple_of( 8_degrees, 15_degrees ) == 15_degrees );
    CHECK( round_to_multiple_of( 15_degrees, 15_degrees ) == 15_degrees );
    CHECK( round_to_multiple_of( 360_degrees, 15_degrees ) == 360_degrees );
    CHECK( round_to_multiple_of( -1_degrees, 15_degrees ) == 0_degrees );
    CHECK( round_to_multiple_of( -7_degrees, 15_degrees ) == 0_degrees );
    CHECK( round_to_multiple_of( -8_degrees, 15_degrees ) == -15_degrees );
    CHECK( round_to_multiple_of( -15_degrees, 15_degrees ) == -15_degrees );
    CHECK( round_to_multiple_of( -360_degrees, 15_degrees ) == -360_degrees );
}

TEST_CASE( "Temperatures", "[temperature]" )
{
    SECTION( "Different units match" ) {
        CHECK( units::to_kelvin( units::from_kelvin( 273.150 ) ) == Approx( 273.150 ) );
        CHECK( units::to_kelvin( units::from_celsius( 0.0 ) ) == Approx( 273.150 ) );
        CHECK( units::to_kelvin( units::from_fahrenheit( 32.0 ) ) == Approx( 273.150 ) );

        CHECK( units::to_kelvin( units::from_celsius( 0 ) ) == Approx( 273.150 ) );
        CHECK( units::to_kelvin( units::from_fahrenheit( 32 ) ) == Approx( 273.150 ) );

        CHECK( units::to_kelvin( 273.150_K ) == Approx( 273.150 ) );

        CHECK( units::to_fahrenheit( units::from_kelvin( 100 ) ) == Approx( -279.67 ) );
        CHECK( units::to_celsius( units::from_kelvin( 100 ) ) == Approx( -173.15 ) );
    }
}

TEST_CASE( "Temperature_delta", "[temperature]" )
{
    SECTION( "Different units match" ) {
        CHECK( units::to_kelvin_delta( units::from_kelvin_delta( 10 ) ) == 10 );
        CHECK( units::to_kelvin_delta( units::from_celsius_delta( 10 ) ) == 10 );
        CHECK( units::to_kelvin_delta( units::from_fahrenheit_delta( 18 ) ) == 10 );

        CHECK( units::to_kelvin_delta( units::from_kelvin_delta( 0 ) ) == 0 );
        CHECK( units::to_kelvin_delta( units::from_celsius_delta( 0 ) ) == 0 );
        CHECK( units::to_kelvin_delta( units::from_fahrenheit_delta( 0 ) ) == 0 );

        CHECK( units::to_kelvin_delta( units::from_kelvin_delta( -10 ) ) == -10 );
        CHECK( units::to_kelvin_delta( units::from_celsius_delta( -10 ) ) == -10 );
        CHECK( units::to_kelvin_delta( units::from_fahrenheit_delta( -18 ) ) == -10 );

        CHECK( units::to_kelvin_delta( units::from_kelvin_delta( 100.1 ) ) == 100.1f );
        CHECK( units::to_kelvin_delta( units::from_celsius_delta( 100.1 ) ) == 100.1f );
        CHECK( units::to_kelvin_delta( units::from_fahrenheit_delta( 180.18 ) ) == Approx( 100.1f ).margin(
                   0.001f ) );
    }

    SECTION( "Temperature subtraction" ) {
        CHECK( units::to_kelvin_delta( 100_K - 90_K ) == 10 );
        CHECK( units::to_kelvin_delta( units::from_kelvin_delta( 10 ) ) == 10 );
        CHECK( units::to_kelvin_delta( units::from_celsius_delta( 10 ) ) == 10 );
        CHECK( units::to_kelvin_delta( units::from_fahrenheit_delta( 18 ) ) == 10 );
    }

    SECTION( "Temperature plus delta" ) {
        CHECK( units::to_kelvin( 10_K + units::from_kelvin_delta( 10 ) ) == 20 );
        CHECK( units::to_kelvin( units::from_kelvin_delta( 10 ) + 10_K ) == 20 );
        CHECK( units::to_celsius( units::from_celsius( 10 ) + units::from_celsius_delta( 10 ) ) == 20 );
        CHECK( units::to_fahrenheit( units::from_fahrenheit( 10 ) + units::from_fahrenheit_delta(
                                         10 ) ) == Approx( 20.f ).margin( 0.0001f ) );

        units::temperature temp = units::from_kelvin( 22.5 );
        temp += units::from_kelvin_delta( 10.3 );
        CHECK( units::to_kelvin( temp ) == 32.8f );
    }

}

TEST_CASE( "Specific_energy", "[temperature]" )
{
    SECTION( "Different units match" ) {
        CHECK( units::to_joule_per_gram( units::from_joule_per_gram( 100 ) ) == 100 );
        CHECK( units::to_joule_per_gram( units::from_joule_per_gram( 100.1 ) ) == Approx( 100.1 ) );
    }
}

TEST_CASE( "energy_display", "[units][nogame]" )
{
    CHECK( units::display( units::from_millijoule( 1 ) ) == "1 mJ" );
    CHECK( units::display( units::from_millijoule( 1000 ) ) == "1 J" );
    CHECK( units::display( units::from_millijoule( 1001 ) ) == "1001 mJ" );
    CHECK( units::display( units::from_millijoule( 1000000 ) ) == "1 kJ" );
    CHECK( units::display( units::from_millijoule( 1000001 ) ) == "1 kJ" );
    CHECK( units::display( units::from_millijoule( 1001000 ) ) == "1001 J" );
    CHECK( units::display( units::from_millijoule( 1001001 ) ) == "1001001 mJ" );
    CHECK( units::display( units::from_millijoule( 2147483648LL ) ) == "2147483648 mJ" );
    CHECK( units::display( units::from_millijoule( 4294967296LL ) ) == "4294967296 mJ" );
}
