#include <string>
#include <array>
#include <list>
#include <memory>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "player.h"
#include "weather.h"
#include "bodypart.h"
#include "calendar.h"
#include "item.h"

// Set the stage for a particular ambient and target temperature and run update_bodytemp() until
// core body temperature settles.
static void temperature_check( player *p, const int ambient_temp, const int target_temp )
{
    g->weather.temperature = ambient_temp;
    for( int i = 0 ; i < num_bp; i++ ) {
        p->temp_cur[i] = BODYTEMP_NORM;
    }
    for( int i = 0 ; i < num_bp; i++ ) {
        p->temp_conv[i] = BODYTEMP_NORM;
    }

    int prev_temp = 0;
    int prev_diff = 0;
    for( int i = 0; i < 10000; i++ ) {
        if( prev_diff != prev_temp - p->temp_cur[0] ) {
            prev_diff = prev_temp - p->temp_cur[0];
        } else if( prev_temp == p->temp_cur[0] ) {
            break;
        }
        prev_temp = p->temp_cur[0];
        p->update_bodytemp();
    }
    int high = target_temp + 100;
    int low = target_temp - 100;
    CHECK( low < p->temp_cur[0] );
    CHECK( high > p->temp_cur[0] );
}

static void equip_clothing( player *p, const std::string &clothing )
{
    const item article( clothing, 0 );
    p->wear_item( article );
}

// Run the tests for each of the temperature setpoints.
// ambient_temps MUST have 7 values or we'll segfault.
static void test_temperature_spread( player *p, const std::array<int, 7> &ambient_temps )
{
    temperature_check( p, ambient_temps[0], BODYTEMP_FREEZING );
    temperature_check( p, ambient_temps[1], BODYTEMP_VERY_COLD );
    temperature_check( p, ambient_temps[2], BODYTEMP_COLD );
    temperature_check( p, ambient_temps[3], BODYTEMP_NORM );
    temperature_check( p, ambient_temps[4], BODYTEMP_HOT );
    temperature_check( p, ambient_temps[5], BODYTEMP_VERY_HOT );
    temperature_check( p, ambient_temps[6], BODYTEMP_SCORCHING );
}

TEST_CASE( "Player body temperatures converge on expected values.", "[.bodytemp]" )
{

    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) ) {}

    // See http://personal.cityu.edu.hk/~bsapplec/heat.htm for temperature basis.
    // As we aren't modeling metabolic rate, assume 2 METS when not sleeping.
    // Obviously though 0.7 METS when sleeping is called for.

    // I'm not sure how to apply +1 METS as a temperature offset,
    // treating it as a 12C/54F boost across the board

    // The commented out tests are the ideals calculated from the above site.
    // The enabled tests are the current status quo just to check for regressions.

    SECTION( "Nude target temperatures." ) {
        //test_temperature_spread( &dummy, { 19, 34, 49, 64, 79, 94, 109 } );
        test_temperature_spread( &dummy, {{ -12, 15, 40, 64, 78, 90, 101 }} );
    }

    SECTION( "Lightly clothed target temperatures" ) {
        equip_clothing( &dummy, "hat_ball" );
        equip_clothing( &dummy, "bandana" );
        equip_clothing( &dummy, "tshirt" );
        equip_clothing( &dummy, "gloves_fingerless" );
        equip_clothing( &dummy, "jeans" );
        equip_clothing( &dummy, "socks" );
        equip_clothing( &dummy, "sneakers" );

        //test_temperature_spread( &dummy, { -3, 12, 27, 42, 57, 72, 87 } );
        test_temperature_spread( &dummy, {{ -17, 10, 35, 62, 77, 90, 100 }} );
    }

    SECTION( "Heavily clothed target temperatures" ) {
        equip_clothing( &dummy, "hat_knit" );
        equip_clothing( &dummy, "tshirt" );
        equip_clothing( &dummy, "vest" );
        equip_clothing( &dummy, "trenchcoat" );
        equip_clothing( &dummy, "gloves_wool" );
        equip_clothing( &dummy, "long_underpants" );
        equip_clothing( &dummy, "pants_army" );
        equip_clothing( &dummy, "socks_wool" );
        equip_clothing( &dummy, "boots" );

        //test_temperature_spread( &dummy, { -25, -10, 5, 20, 35, 50, 65 } );
        test_temperature_spread( &dummy, {{ -39, -14, 17, 46, 70, 84, 96 }} );
    }

    SECTION( "Arctic gear target temperatures" ) {
        equip_clothing( &dummy, "balclava" );
        equip_clothing( &dummy, "goggles_ski" );
        equip_clothing( &dummy, "hat_hunting" );
        equip_clothing( &dummy, "under_armor" );
        equip_clothing( &dummy, "vest" );
        equip_clothing( &dummy, "coat_winter" );
        equip_clothing( &dummy, "gloves_liner" );
        equip_clothing( &dummy, "gloves_winter" );
        equip_clothing( &dummy, "long_underpants" );
        equip_clothing( &dummy, "pants_fur" );
        equip_clothing( &dummy, "socks_wool" );
        equip_clothing( &dummy, "boots_winter" );

        //test_temperature_spread( &dummy, { -47, -32, -17, -2, 13, 28, 43 } );
        test_temperature_spread( &dummy, {{ -115, -87, -54, -6, 36, 64, 80 }} );
    }
}
