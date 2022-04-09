#include "calendar.h"
#include "cata_utility.h"
#include "cata_catch.h"
#include "enums.h"
#include "flag.h"
#include "game_constants.h"
#include "item.h"
#include "point.h"
#include "weather.h"

static void set_map_temperature( int new_temperature )
{
    get_weather().temperature = new_temperature;
    get_weather().clear_temp_cache();
}

TEST_CASE( "Item spawns with right thermal attributes" )
{
    item D( "meat_cooked" );

    CHECK( D.get_specific_heat_liquid() == 3.7f );
    CHECK( D.get_specific_heat_solid() == 2.15f );
    CHECK( D.get_latent_heat() == 260 );

    CHECK( D.temperature == 0 );
    CHECK( D.specific_energy == -10 );

    set_map_temperature( 122 );
    D.process_temperature_rot( 1, tripoint_zero, nullptr );

    CHECK( D.temperature == Approx( 323.15 * 100000 ).margin( 1 ) );
}

TEST_CASE( "Rate of temperature change" )
{
    // Fahrenheits and kelvins get used and converted around
    // So there are small rounding errors everywhere. Use margins.
    // The calculations are done once every 10 minutes.
    // Don't bother with times shorter than that.

    // Note: If process interval is longer than 1 hour the calculations will be done using the environment temperature
    // IMPORTANT: Processing intervals should be kept below 1 hour to avoid this.

    // Sections:
    // Water bottle (realisticity check)
    // Cool down test
    // heat up test

    SECTION( "Water bottle test (ralisticity)" ) {
        // Water at 55 C
        // Environment at 20 C
        // 75 minutes
        // Water 1 and 2 processed at slightly different intervals
        // Temperature after should be approx 30 C for realistic values
        // Lower than 30 C means faster temperature changes
        // Based on the water bottle cooling measurements on this paper
        // https://www.researchgate.net/publication/282841499_Study_on_heat_transfer_coefficients_during_cooling_of_PET_bottles_for_food_beverages
        // Checked with incremental updates and whole time at once

        item water1( "water" );
        item water2( "water" );

        set_map_temperature( 131 ); // 55 C

        water1.process_temperature_rot( 1, tripoint_zero, nullptr );
        water2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 55 C
        CHECK( water1.temperature == Approx( 328.15 * 100000 ).margin( 1 ) );

        set_map_temperature( 68 ); // 20C

        calendar::turn += 11_minutes;
        water1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn += 20_minutes;
        water1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn += 29_minutes;
        water1.process_temperature_rot( 1, tripoint_zero, nullptr );
        water2.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn += 15_minutes;
        water1.process_temperature_rot( 1, tripoint_zero, nullptr );
        water2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // about 29.6 C
        CHECK( water1.temperature == Approx( 30271802 ).margin( 1 ) );
        CHECK( water1.temperature == Approx( water2.temperature ).margin( 1 ) );
    }

    SECTION( "Hot liquid to frozen" ) {
        // 2x cooked meat (50 C) cooling in -20 C environment for several hours
        // 1) Both at 50C and hot
        // 2) Wait a short time then Meat 1 at about 34.6 C and not hot
        // 3) Wait an hour at different intervals then Meat 1 and 2 at 0 C not frozen
        // 4) Wait two hours then Meat 1 and 2 at 0 C frozen
        // 5) Wait a bit over hour then Meat 1 and 2 at about -5.2 C

        item meat1( "meat_cooked" );
        item meat2( "meat_cooked" );

        set_map_temperature( 122 ); //50 C

        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 50 C
        CHECK( meat1.temperature == Approx( 323.15 * 100000 ).margin( 1 ) );
        CHECK( meat1.has_own_flag( flag_HOT ) );

        set_map_temperature( -4 ); // -20 C

        calendar::turn += 15_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // about 34.6 C
        CHECK( meat1.temperature == Approx( 30778338 ).margin( 1 ) );
        CHECK( !meat1.has_own_flag( flag_HOT ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn += 30_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 0C
        // not frozen
        CHECK( meat1.temperature == 27315000 );
        CHECK( meat2.temperature == 27315000 );
        CHECK( !meat1.has_own_flag( flag_FROZEN ) );
        CHECK( !meat2.has_own_flag( flag_FROZEN ) );

        calendar::turn += 60_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn += 60_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 0C
        // frozen
        // same energy as meat 2
        CHECK( meat1.temperature == 27315000 );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );
        CHECK( meat2.has_own_flag( flag_FROZEN ) );
        CHECK( meat1.specific_energy == Approx( meat2.specific_energy ).margin( 1 ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn += 50_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // about -5.2 C
        // frozen
        // same temp as meat 2
        CHECK( meat1.temperature == Approx( 26798050 ).margin( 1 ) );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );
        CHECK( meat1.temperature == Approx( meat2.temperature ).margin( 1 ) );
    }

    SECTION( "Cold solid to liquid" ) {
        // 2x cooked meat (-20 C) warming in 20 C environment
        // Start: both at -20 C and frozen
        // 11 min: meat 1 at about -9.3 C
        // Process 32 min in different steps
        // Both meats frozen at 0 C
        // Process 90 min
        // Both meats not frozen at 9 C
        // Process 100 min in different steps
        // Both meats at about 2.2 C

        item meat1( "meat_cooked" );
        item meat2( "meat_cooked" );

        set_map_temperature( -4 ); // -20 C

        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // -20 C
        CHECK( meat1.temperature == Approx( 253.15 * 100000 ).margin( 1 ) );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );

        set_map_temperature( 68 ); // 20 C

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        // about -9.3 C
        CHECK( meat1.temperature == Approx( 26389390 ).margin( 1 ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 0C
        // same temp
        // frozen
        CHECK( meat1.temperature == 27315000 );
        CHECK( meat2.temperature == meat1.temperature );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );
        CHECK( meat2.has_own_flag( flag_FROZEN ) );

        calendar::turn += 45_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn += 45_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 0C
        // same temp
        // not frozen
        CHECK( meat1.temperature == 27315000 );
        CHECK( meat2.temperature == meat1.temperature );
        CHECK( !meat1.has_own_flag( flag_FROZEN ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn += 50_minutes;
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // about 2.2 C
        CHECK( meat1.temperature == Approx( 27532468 ).margin( 1 ) );
        CHECK( meat1.temperature == Approx( meat2.temperature ).margin( 1 ) );
    }
}

TEST_CASE( "Temperature controlled location" )
{
    SECTION( "Heater test" ) {
        // Spawn water
        // Process immediately in heater. Sets temperature to temperatures::normal.
        // Process water 15 min later. Should still be temperatures::normal.
        // Process water 2h 3m later. Should still be temperatures::normal.
        item water1( "water" );

        set_map_temperature( 0 ); // -17 C

        water1.process_temperature_rot( 1, tripoint_zero, nullptr,
                                        temperature_flag::HEATER );

        CHECK( water1.temperature == Approx( 100000 * temp_to_kelvin( temperatures::normal ) ).margin(
                   1 ) );

        calendar::turn += 15_minutes;
        water1.process_temperature_rot( 1, tripoint_zero, nullptr,
                                        temperature_flag::HEATER );

        CHECK( water1.temperature == Approx( 100000 * temp_to_kelvin( temperatures::normal ) ).margin(
                   1 ) );

        calendar::turn += 2_hours + 3_minutes;
        water1.process_temperature_rot( 1, tripoint_zero, nullptr,
                                        temperature_flag::HEATER );

        CHECK( water1.temperature == Approx( 100000 * temp_to_kelvin( temperatures::normal ) ).margin(
                   1 ) );
    }
}
