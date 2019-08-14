#include <stdlib.h>
#include <memory>
#include <string>

#include "catch/catch.hpp"
#include "calendar.h"
#include "item.h"
#include "enums.h"
#include "cata_utility.h"
#include "game.h"
#include "flat_set.h"
#include "game_constants.h"
#include "point.h"
#include "weather.h"


static bool is_nearly( float value, float expected )
{
    // Rounding errors make the values change around a bit
    // Inside reality bubble and outside reality bubble also get slightly different results
    // Lets just check that they are within 5% of expected

    bool ret_val = std::abs( value - expected ) / expected < 0.05;
    return ret_val;
}

static void set_map_temperature( int new_temperature )
{
    g->weather.temperature = new_temperature;
    g->weather.clear_temp_cache();
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

    CHECK( is_nearly( D.temperature, 323.15 * 100000 ) );
}

TEST_CASE( "Rate of temperature change" )
{
    // Farenheits and kelvins get used and converted around
    // So there are small rounding errors everywhere
    // Check ranges instead of values when rounding errors
    // The calculations are done once every 101 turns (10 min 6 seconds)
    // Don't bother with times shorter than this

    // Note: If process interval is longer than 1 hour the calculations will be done using the enviroment temperature
    // Processing intervals should be kept below 1 hour to avoid this.

    // Sections:
    // Water bottle (realisticity check)
    // Cool down test
    // heat up test

    SECTION( "Water bottle test (ralisticity)" ) {
        // Water at 55 C
        // Enviroment at 20 C
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
        CHECK( is_nearly( water1.temperature, 328.15 * 100000 ) );

        set_map_temperature( 68 ); // 20C

        calendar::turn = to_turn<int>( calendar::turn + 11_minutes );
        water1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 20_minutes );
        water1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 29_minutes );
        water1.process_temperature_rot( 1, tripoint_zero, nullptr );
        water2.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 15_minutes );
        water1.process_temperature_rot( 1, tripoint_zero, nullptr );
        water2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 29.4 C
        CHECK( is_nearly( water1.temperature, 30259330 ) );
        CHECK( is_nearly( water1.temperature, water2.temperature ) );
    }

    SECTION( "Hot liquid to frozen" ) {
        // 2x cooked meat (50 C) cooling in -20 C enviroment for several hours
        // 1) Both at 50C and hot
        // 2) Wait a short time then Meat 1 at 33.5 C and not hot
        // 3) Wait less than an hour then Meat 1 and 2 at 0 C not frozen
        // 4) Wait a few hours then Meat 1 and 2 at 0 C frozen
        // 5) Wait a short time then Meat 1 and 2 at -x C

        item meat1( "meat_cooked" );
        item meat2( "meat_cooked" );

        set_map_temperature( 122 ); //50 C

        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 50 C
        CHECK( is_nearly( meat1.temperature, 323.15 * 100000 ) );
        CHECK( meat1.item_tags.count( "HOT" ) );

        set_map_temperature( -4 ); // -20 C

        calendar::turn = to_turn<int>( calendar::turn + 15_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 33.5 C
        CHECK( is_nearly( meat1.temperature, 30673432 ) );
        CHECK( !meat1.item_tags.count( "HOT" ) );

        calendar::turn = to_turn<int>( calendar::turn + 11_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn = to_turn<int>( calendar::turn + 11_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 30_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );
        // 0C
        // not frozen
        CHECK( is_nearly( meat1.temperature, 27315000 ) );
        CHECK( is_nearly( meat2.temperature, 27315000 ) );
        CHECK( !meat1.item_tags.count( "FROZEN" ) );
        CHECK( !meat2.item_tags.count( "FROZEN" ) );

        calendar::turn = to_turn<int>( calendar::turn + 60_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn = to_turn<int>( calendar::turn + 60_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 0C
        // frozen
        // same energy as meat 2
        CHECK( is_nearly( meat1.temperature, 27315000 ) );
        CHECK( meat1.item_tags.count( "FROZEN" ) );
        CHECK( meat2.item_tags.count( "FROZEN" ) );
        CHECK( is_nearly( meat1.specific_energy, meat2.specific_energy ) );

        calendar::turn = to_turn<int>( calendar::turn + 11_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn = to_turn<int>( calendar::turn + 20_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 20_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // -7.2 C
        // frozen
        // same temp as meat 2
        CHECK( is_nearly( meat1.temperature, 26595062 ) );
        CHECK( meat1.item_tags.count( "FROZEN" ) );
        CHECK( is_nearly( meat1.temperature, meat2.temperature ) );
    }

    SECTION( "Cold solid to liquid" ) {
        // 2x cooked meat (-20 C) warming in 20 C enviroment for ~190 minutes
        // 0 min: both at -20 C and frozen
        // 11 min: meat 1 at -5.2 C
        // 22 min: meat 1 step
        // 33 min: meat 1 step
        // 53 min: Meat 1, 2 at 0 C an frozen
        // 143 min: Meat 1 and 2 at 0 C not fozen.
        //          Both have same energy (within rounding error)
        // 154 min: meat 1 step
        // 174 min: meat 1 step
        // 194 min: Meat 1 and 2 at 13.3 C

        item meat1( "meat_cooked" );
        item meat2( "meat_cooked" );

        set_map_temperature( -4 ); // -20 C

        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // -20 C
        CHECK( is_nearly( meat1.temperature, 253.15 * 100000 ) );
        CHECK( meat1.item_tags.count( "FROZEN" ) );

        set_map_temperature( 68 ); // 20 C

        calendar::turn = to_turn<int>( calendar::turn + 11_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        // -5.2 C
        CHECK( is_nearly( meat1.temperature, 26789608 ) );

        calendar::turn = to_turn<int>( calendar::turn + 11_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 11_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 20_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 0C
        // same temp
        // frozen
        CHECK( is_nearly( meat1.temperature, 27315000 ) );
        CHECK( is_nearly( meat2.temperature, meat1.temperature ) );
        CHECK( meat1.item_tags.count( "FROZEN" ) );
        CHECK( meat2.item_tags.count( "FROZEN" ) );

        calendar::turn = to_turn<int>( calendar::turn + 45_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 45_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );

        // 0C
        // same tmep
        // not frozen
        CHECK( is_nearly( meat1.temperature, 27315000 ) );
        CHECK( is_nearly( meat2.temperature, meat1.temperature ) );
        CHECK( !meat1.item_tags.count( "FROZEN" ) );

        calendar::turn = to_turn<int>( calendar::turn + 11_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        calendar::turn = to_turn<int>( calendar::turn + 20_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 20_minutes );
        meat1.process_temperature_rot( 1, tripoint_zero, nullptr );
        meat2.process_temperature_rot( 1, tripoint_zero, nullptr );
        // 13.3 C
        // same temp
        CHECK( is_nearly( meat1.temperature, 28654986 ) );
        CHECK( is_nearly( meat1.temperature, meat2.temperature ) );
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

        water1.process_temperature_rot( 1, tripoint_zero, nullptr, temperature_flag::TEMP_HEATER );

        CHECK( is_nearly( water1.temperature, 100000 * temp_to_kelvin( temperatures::normal ) ) );

        calendar::turn = to_turn<int>( calendar::turn + 15_minutes );
        water1.process_temperature_rot( 1, tripoint_zero, nullptr, temperature_flag::TEMP_HEATER );

        CHECK( is_nearly( water1.temperature, 100000 * temp_to_kelvin( temperatures::normal ) ) );

        calendar::turn = to_turn<int>( calendar::turn + 2_hours + 3_minutes );
        water1.process_temperature_rot( 1, tripoint_zero, nullptr, temperature_flag::TEMP_HEATER );

        CHECK( is_nearly( water1.temperature, 100000 * temp_to_kelvin( temperatures::normal ) ) );
    }
}
