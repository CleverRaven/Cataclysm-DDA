#include "catch/catch.hpp"
#include "calendar.h"
#include "itype.h"
#include "item.h"
#include "map.h"

bool is_nearly( float value, float expected )
{
    // Rounding errors make the values change around a bit
    // Inside reality bubble and outside reality bubble also get slightly different results
    // Lets just check that they are within 5% of expected

    bool ret_val = std::abs( value - expected ) / expected < 0.05;
    return ret_val;
}

TEST_CASE( "Item spawns with right thermal attributes" )
{
    item D( "meat_cooked" );

    CHECK( D.type->comestible->specific_heat_liquid == 3.7f );
    CHECK( D.type->comestible->specific_heat_solid == 2.15f );
    CHECK( D.type->comestible->latent_heat == 260 );

    CHECK( D.temperature == 0 );
    CHECK( D.specific_energy == -10 );

    D.process_temperature_rot( 122, 1, tripoint( 0, 0, 0 ), nullptr );

    CHECK( is_nearly( D.temperature, 323.15 * 100000 ) );
}

TEST_CASE( "Rate of temperature change" )
{
    // Farenheits and kelvins get used and converted around
    // So there are small rounding errors everywhere
    // Check ranges instead of values when rounding errors
    // The calculations are done once every 101 turns (10 min)
    // Don't bother with times shorter than this

    // Sections:
    // Water bottle (realisticity check)
    // Cool down test
    // heat up test

    SECTION( "Water bottle test (ralisticity)" ) {
        // Water at 55 C
        // Enviroment at 20 C
        // 75 minutes
        // Temperature after should be approx 30 C for realistic values
        // Lower than 30 C means faster temperature changes
        // Based on the water bottle cooling measurements on this paper
        // https://www.researchgate.net/publication/282841499_Study_on_heat_transfer_coefficients_during_cooling_of_PET_bottles_for_food_beverages
        // Checked with incremental updates and whole time at once

        item water1( "water" );
        item water2( "water" );

        water1.process_temperature_rot( 131, 1, tripoint( 0, 0, 0 ), nullptr );
        water2.process_temperature_rot( 131, 1, tripoint( 0, 0, 0 ), nullptr );

        // 55 C
        CHECK( is_nearly( water1.temperature, 328.15 * 100000 ) );


        calendar::turn = to_turn<int>( calendar::turn + 101_turns );
        water1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 200_turns );
        water1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 300_turns );
        water1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 150_turns );
        water1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );
        water2.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );

        // 29.4 C
        CHECK( is_nearly( water1.temperature, 30259330 ) );
        CHECK( is_nearly( water1.temperature, water2.temperature ) );
    }

    SECTION( "Hot liquid to frozen" ) {
        // 2x cooked meat (50 C) cooling in -20 C enviroment for ~190 minutes
        // 0 min: All 3 at 50C and hot
        // 10 min: meat 1 at 33.5 C and not hot
        // 20 min: meat 1 step
        // 30 min: meat 1 step
        // 50 min: Meat 1 at 0 C not frozen
        // 140 min: Meat 1 and 2 at 0 C fozen
        // 150 min: meat 1 step
        // 170 min: meat 1 step
        // 190 min: Meat 1 and 2 at -x C

        item meat1( "meat_cooked" );
        item meat2( "meat_cooked" );

        meat1.process_temperature_rot( 122, 1, tripoint( 0, 0, 0 ), nullptr );
        meat2.process_temperature_rot( 122, 1, tripoint( 0, 0, 0 ), nullptr );

        // 50 C
        CHECK( std::abs( meat1.temperature - 323.15 * 100000 ) < 1 );
        CHECK( meat1.item_tags.count( "HOT" ) );


        calendar::turn = to_turn<int>( calendar::turn + 101_turns );
        meat1.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );

        // 33.5 C
        CHECK( is_nearly( meat1.temperature, 30673432 ) );
        CHECK( !meat1.item_tags.count( "HOT" ) );

        calendar::turn = to_turn<int>( calendar::turn + 101_turns );
        meat1.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );
        calendar::turn = to_turn<int>( calendar::turn + 101_turns );
        meat1.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 197_turns );
        meat1.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );
        // 0C
        // not frozen
        CHECK( is_nearly( meat1.temperature, 27315000 ) );
        CHECK( !meat1.item_tags.count( "FROZEN" ) );

        calendar::turn = to_turn<int>( calendar::turn + 900_turns );
        meat1.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );
        meat2.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );
        // 0C
        // frozen
        // same energy as meat 2
        CHECK( is_nearly( meat1.temperature, 27315000 ) );
        CHECK( meat1.item_tags.count( "FROZEN" ) );
        CHECK( is_nearly( meat1.specific_energy, meat2.specific_energy ) );

        calendar::turn = to_turn<int>( calendar::turn + 101_turns );
        meat1.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );
        calendar::turn = to_turn<int>( calendar::turn + 199_turns );
        meat1.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 200_turns );
        meat1.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );
        meat2.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );

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
        // 10 min: meat 1 at -5.2 C
        // 20 min: meat 1 step
        // 30 min: meat 1 step
        // 50 min: Meat 1 at 0 C an frozen
        // 140 min: Meat 1 and 2 at 0 C not fozen.
        //          Both have same energy (within rounding error)
        // 150 min: meat 1 step
        // 170 min: meat 1 step
        // 190 min: Meat 1 and 2 at 13.3 C

        item meat1( "meat_cooked" );
        item meat2( "meat_cooked" );

        meat1.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );
        meat2.process_temperature_rot( -4, 1, tripoint( 0, 0, 0 ), nullptr );

        // -20 C
        CHECK( is_nearly( meat1.temperature, 253.15 * 100000 ) );
        CHECK( meat1.item_tags.count( "FROZEN" ) );


        calendar::turn = to_turn<int>( calendar::turn + 101_turns );
        meat1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );
        // -5.2 C
        CHECK( is_nearly( meat1.temperature, 26789608 ) );

        calendar::turn = to_turn<int>( calendar::turn + 101_turns );
        meat1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 101_turns );
        meat1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 197_turns );
        meat1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );
        // 0C
        // frozen
        CHECK( is_nearly( meat1.temperature, 27315000 ) );
        CHECK( meat1.item_tags.count( "FROZEN" ) );

        calendar::turn = to_turn<int>( calendar::turn + 900_turns );
        meat1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );
        meat2.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 201_turns );
        meat1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );
        meat2.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );
        // 0C
        // not frozen
        CHECK( is_nearly( meat1.temperature, 27315000 ) );
        CHECK( is_nearly( meat2.temperature, 27315000 ) );
        CHECK( !meat1.item_tags.count( "FROZEN" ) );

        calendar::turn = to_turn<int>( calendar::turn + 101_turns );
        meat1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );
        calendar::turn = to_turn<int>( calendar::turn + 199_turns );
        meat1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );

        calendar::turn = to_turn<int>( calendar::turn + 200_turns );
        meat1.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );
        meat2.process_temperature_rot( 68, 1, tripoint( 0, 0, 0 ), nullptr );
        // 13.3 C
        // same temp as meat 2
        CHECK( is_nearly( meat1.temperature, 28654986 ) );
        CHECK( is_nearly( meat1.temperature, meat2.temperature ) );
    }
}
