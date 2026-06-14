#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "enums.h"
#include "flag.h"
#include "game_constants.h"
#include "item.h"
#include "map.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const itype_id itype_meat_cooked( "meat_cooked" );
static const itype_id itype_water( "water" );

static void set_map_temperature( units::temperature new_temperature )
{
    get_weather().temperature = new_temperature;
    get_weather().clear_temp_cache();
}

TEST_CASE( "Item_spawns_with_right_thermal_attributes", "[temperature]" )
{
    item D( itype_meat_cooked );

    CHECK( D.get_specific_heat_liquid() == 3.7f );
    CHECK( D.get_specific_heat_solid() == 2.15f );
    CHECK( D.get_latent_heat() == 260 );

    CHECK( units::to_kelvin( D.temperature ) == 0 );
    CHECK( units::to_joule_per_gram( D.specific_energy ) == -10 );

    set_map_temperature( units::from_fahrenheit( 122 ) );
    D.process_temperature_rot( 1, tripoint_bub_ms::zero, get_map(), nullptr );

    CHECK( units::to_kelvin( D.temperature ) == Approx( 323.15 ) );
}

TEST_CASE( "Rate_of_temperature_change", "[temperature]" )
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

    map &here = get_map();
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

        item water1( itype_water );
        item water2( itype_water );

        set_map_temperature( units::from_fahrenheit( 131 ) ); // 55 C

        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        water2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // 55 C
        CHECK( units::to_kelvin( water1.temperature ) == Approx( 328.15 ) );

        set_map_temperature( units::from_fahrenheit( 68 ) ); // 20C

        calendar::turn += 11_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 20_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 29_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        water2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 15_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        water2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // about 29.6 C
        CHECK( units::to_kelvin( water1.temperature ) == Approx( 302.71802 ) );
        CHECK( units::to_kelvin( water1.temperature ) == Approx( units::to_kelvin( water2.temperature ) ) );
    }

    SECTION( "Hot liquid to frozen" ) {
        // 2x cooked meat (50 C) cooling in -20 C environment for several hours
        // 1) Both at 50C and hot
        // 2) Wait a short time then Meat 1 at about 34.6 C and not hot
        // 3) Wait an hour at different intervals then Meat 1 and 2 at 0 C not frozen
        // 4) Wait two hours then Meat 1 and 2 at 0 C frozen
        // 5) Wait a bit over hour then Meat 1 and 2 at about -5.2 C

        item meat1( itype_meat_cooked );
        item meat2( itype_meat_cooked );

        set_map_temperature( units::from_fahrenheit( 122 ) ); //50 C

        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // 50 C
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 323.15 ) );
        CHECK( meat1.has_own_flag( flag_HOT ) );

        set_map_temperature( units::from_fahrenheit( -4 ) ); // -20 C

        calendar::turn += 15_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // about 34.6 C
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 307.78338 ) );
        CHECK( !meat1.has_own_flag( flag_HOT ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 30_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // 0C
        // not frozen
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 273.15 ) );
        CHECK( units::to_kelvin( meat2.temperature ) == Approx( 273.15 ) );
        CHECK( !meat1.has_own_flag( flag_FROZEN ) );
        CHECK( !meat2.has_own_flag( flag_FROZEN ) );

        calendar::turn += 60_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 60_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // 0C
        // frozen
        // same energy as meat 2
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 273.15 ) );;
        CHECK( meat1.has_own_flag( flag_FROZEN ) );
        CHECK( meat2.has_own_flag( flag_FROZEN ) );
        CHECK( units::to_joule_per_gram( meat1.specific_energy ) == Approx( units::to_joule_per_gram(
                    meat2.specific_energy ) ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 50_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // about -5.2 C
        // frozen
        // same temp as meat 2
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 267.98050 ) );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( units::to_kelvin( meat2.temperature ) ) );
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

        item meat1( itype_meat_cooked );
        item meat2( itype_meat_cooked );

        set_map_temperature( units::from_fahrenheit( -4 ) ); // -20 C

        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // -20 C
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 253.15 ) );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );

        set_map_temperature( units::from_fahrenheit( 68 ) ); // 20 C

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        // about -9.3 C
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 263.89390 ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // 0C
        // same temp
        // frozen
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 273.15 ) );
        CHECK( units::to_kelvin( meat2.temperature ) == units::to_kelvin( meat1.temperature ) );
        CHECK( meat1.has_own_flag( flag_FROZEN ) );
        CHECK( meat2.has_own_flag( flag_FROZEN ) );

        calendar::turn += 45_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 45_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // 0C
        // same temp
        // not frozen
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 273.15 ) );
        CHECK( units::to_kelvin( meat2.temperature ) == units::to_kelvin( meat1.temperature ) );
        CHECK( !meat1.has_own_flag( flag_FROZEN ) );

        calendar::turn += 11_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        calendar::turn += 20_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        calendar::turn += 50_minutes;
        meat1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );
        meat2.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr );

        // about 2.2 C
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( 275.32468 ) );
        CHECK( units::to_kelvin( meat1.temperature ) == Approx( units::to_kelvin( meat2.temperature ) ) );
    }
}

TEST_CASE( "Temperature_controlled_location", "[temperature]" )
{
    SECTION( "Heater test" ) {
        // Spawn water
        // Process immediately in heater. Sets temperature to temperatures::normal.
        // Process water 15 min later. Should still be temperatures::normal.
        // Process water 2h 3m later. Should still be temperatures::normal.
        item water1( itype_water );

        set_map_temperature( units::from_fahrenheit( 0 ) ); // -17 C

        map &here = get_map();
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr,
                                        temperature_flag::HEATER );

        CHECK( units::to_kelvin( water1.temperature ) == Approx( units::to_kelvin(
                    temperatures::normal ) ) );

        calendar::turn += 15_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr,
                                        temperature_flag::HEATER );

        CHECK( units::to_kelvin( water1.temperature ) == Approx( units::to_kelvin(
                    temperatures::normal ) ) );

        calendar::turn += 2_hours + 3_minutes;
        water1.process_temperature_rot( 1, tripoint_bub_ms::zero, here, nullptr,
                                        temperature_flag::HEATER );

        CHECK( units::to_kelvin( water1.temperature ) == Approx( units::to_kelvin(
                    temperatures::normal ) ) );
    }
}
