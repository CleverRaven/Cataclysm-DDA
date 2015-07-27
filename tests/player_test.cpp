#include "catch/catch.hpp"

#include "morale.h"
#include "player.h"
#include "game.h"
#include "overmapbuffer.h"
#include "item_factory.h"
#include "start_location.h"
#include "path_info.h"
#include "mapsharing.h"
#include "options.h"
#include "map.h"

#include <string>

// Set the stage for a particular ambient and target temperature and run update_bodytemp() until
// core body temperature settles.
void temperature_check( player *p, int ambient_temp, int target_temp )
{
    g->temperature = ambient_temp;
    for (int i = 0 ; i < num_bp; i++) {
        p->temp_cur[i] = BODYTEMP_NORM;
    }
    for (int i = 0 ; i < num_bp; i++) {
        p->temp_conv[i] = BODYTEMP_NORM;
    }

    int prev_temp = 0;
    int prev_diff = 0;
    for( int i = 0; i < 10000; i++ ) {
        if( prev_diff != prev_temp - p->temp_cur[0] ) {
            prev_diff = prev_temp - p->temp_cur[0];
        } else if ( prev_temp == p->temp_cur[0] ) {
            break;
        }
        prev_temp = p->temp_cur[0];
        p->update_bodytemp();
    }
    int high = target_temp + 100;
    int low = target_temp - 100;
    REQUIRE( low < p->temp_cur[0] );
    REQUIRE( high > p->temp_cur[0] );
}

void equip_clothing( player *p, std::string clothing )
{
    item article(clothing, 0);
    p->wear_item( &article );
}

// Run the tests for each of the temperature setpoints.
// ambient_temps MUST have 7 values or we'll segfault.
void test_temperature_spread( player *p, int ambient_temps[] )
{
    temperature_check( p, ambient_temps[0], BODYTEMP_FREEZING );
    temperature_check( p, ambient_temps[1], BODYTEMP_VERY_COLD );
    temperature_check( p, ambient_temps[2], BODYTEMP_COLD );
    temperature_check( p, ambient_temps[3], BODYTEMP_NORM );
    temperature_check( p, ambient_temps[4], BODYTEMP_HOT );
    temperature_check( p, ambient_temps[5], BODYTEMP_VERY_HOT );
    temperature_check( p, ambient_temps[6], BODYTEMP_SCORCHING );
}

TEST_CASE("Player body temperatures converge on expected values.") {

    player dummy;

    PATH_INFO::init_base_path("");
    PATH_INFO::init_user_dir("./");
    PATH_INFO::set_standard_filenames();

    MAP_SHARING::setDefaults();

    // Assume curses
    initOptions();
    load_options();
    initscr();

    g = new game;
    g->load_static_data();
//    g->check_all_mod_data();
//    world_generator->set_active_world( world_generator->pick_world( true ) );
    g->setup();

    dummy.normalize( );

    dummy.name = "dummy";
    g->u = dummy;
//    g->cur_om = &overmap_buffer.get( 0, 0 );
    g->m.load( g->get_levx(), g->get_levy(), g->get_levz(), false );


    // See http://personal.cityu.edu.hk/~bsapplec/heat.htm for temperature basis.
    // As we aren't modelling metabolic rate, assume 2 METS when not sleeping.
    // Obviously though 0.7 METS when sleeping is called for.

    // I'm not sure how to apply +1 METS as a temperature offset,
    // treating it as a 12C/54F boost across the board

    SECTION("Nude target temperatures.") {
        int temp_spread[] = { 19, 34, 49, 64, 79, 94, 109 };
        test_temperature_spread( &dummy, temp_spread );
    }

    SECTION("Lightly clothed target temperatures") {
        equip_clothing( &dummy, "hat_ball");
        equip_clothing( &dummy, "bandana");
        equip_clothing( &dummy, "tshirt");
        equip_clothing( &dummy, "gloves_fingerless");
        equip_clothing( &dummy, "jeans");
        equip_clothing( &dummy, "socks");
        equip_clothing( &dummy, "sneakers");

        int temp_spread[] = { -3, 12, 27, 42, 57, 72, 87 };
        test_temperature_spread( &dummy, temp_spread );
    }

    SECTION("Heavily clothed target temperatures" ) {
        equip_clothing( &dummy, "hat_knit");
        equip_clothing( &dummy, "tshirt");
        equip_clothing( &dummy, "vest");
        equip_clothing( &dummy, "trenchcoat");
        equip_clothing( &dummy, "gloves_wool");
        equip_clothing( &dummy, "long_underpants");
        equip_clothing( &dummy, "pants_army");
        equip_clothing( &dummy, "wool_socks");
        equip_clothing( &dummy, "boots");

        int temp_spread[] = { -25, -10, 5, 20, 35, 50, 65 };
        test_temperature_spread( &dummy, temp_spread );
    }

    SECTION("Artic gear target temperatures") {
        equip_clothing( &dummy, "balclava");
        equip_clothing( &dummy, "goggles_ski");
        equip_clothing( &dummy, "hat_hunting");
        equip_clothing( &dummy, "under_armor");
        equip_clothing( &dummy, "vest");
        equip_clothing( &dummy, "coat_winter");
        equip_clothing( &dummy, "gloves_liner");
        equip_clothing( &dummy, "gloves_winter");
        equip_clothing( &dummy, "long_underpants");
        equip_clothing( &dummy, "pants_fur");
        equip_clothing( &dummy, "wool_socks");
        equip_clothing( &dummy, "boots_winter");

        int temp_spread[] = { -47, -32, -17, -2, 13, 28, 43 };
        test_temperature_spread( &dummy, temp_spread );
    }
}
