/* Needs to come before tap lib for some reason? */
#include "player.h"
#include "game.h"
#include "overmapbuffer.h"
#include "item_factory.h"

/* libtap doesn't extern C their headers, so we do it for them. */
extern "C" {
 #include "tap.h"
}

#include <stdio.h>


// Set the stage for a particular ambient and target temperature and run update_bodytemp() until
// core body temperature settles.
void temperature_check( game *g, player *p, int ambient_temp, int target_temp, std::string clothing )
{
    g->get_temperature() = ambient_temp;
    for (int i = 0 ; i < num_bp; i++)
    {
        p->temp_cur[i] = BODYTEMP_NORM;
    }
    for (int i = 0 ; i < num_bp; i++)
    {
        p->temp_conv[i] = BODYTEMP_NORM;
    }

    int prev_temp = 0;
    int prev_diff = 0;
    for( int i = 0; i < 10000; i++ )
    {
        if( prev_diff != prev_temp - p->temp_cur[0] )
        {
            prev_diff = prev_temp - p->temp_cur[0];
        }
        else if ( prev_temp == p->temp_cur[0] )
        {
            break;
        }
        prev_temp = p->temp_cur[0];
        p->update_bodytemp( g );
    }
    ok( target_temp - 100 < p->temp_cur[0] && target_temp + 100 > p->temp_cur[0],
        "Testing with %s clothes at %dF, Expect %d, got %d",
        clothing.c_str(), ambient_temp, target_temp, p->temp_cur[0] );
}

void equip_clothing( game *g, player *p, std::string clothing )
{
    item article = item_controller->create(clothing, 0);
    p->wear_item( g, &article );
}

// Run the tests for each of the temperature setpoints.
// ambient_temps MUST have 7 values or we'll segfault.
void test_temperature_spread( game *g, player *p, int ambient_temps[], std::string clothing)
{
    temperature_check( g, p, ambient_temps[0], BODYTEMP_FREEZING, clothing );
    temperature_check( g, p, ambient_temps[1], BODYTEMP_VERY_COLD, clothing );
    temperature_check( g, p, ambient_temps[2], BODYTEMP_COLD, clothing );
    temperature_check( g, p, ambient_temps[3], BODYTEMP_NORM, clothing );
    temperature_check( g, p, ambient_temps[4], BODYTEMP_HOT, clothing );
    temperature_check( g, p, ambient_temps[5], BODYTEMP_VERY_HOT, clothing );
    temperature_check( g, p, ambient_temps[6], BODYTEMP_SCORCHING, clothing );
}


int main(int argc, char *argv[])
{
 plan_tests(29);

 game fakeworld;
 player dummy;

 dummy.normalize( &fakeworld );

 dummy.name = "dummy";
 fakeworld.m = map( &fakeworld.traps );
 fakeworld.u = dummy;
 fakeworld.cur_om = &overmap_buffer.get(&fakeworld, 0, 0);
 fakeworld.m.load( &fakeworld, fakeworld.levx, fakeworld.levy, fakeworld.levz );


 // See http://personal.cityu.edu.hk/~bsapplec/heat.htm for temperature basis.
 // As we aren't modelling metabolic rate, assume 2 METS when not sleeping.
 // Obviously though 0.7 METS when sleeping is called for.

 // I'm not sure how to apply +1 METS as a temperature offset,
 // treating it as a 12C/54F boost across the board

 // Nude target temperatures
 {
     int temp_spread[] = { 19, 34, 49, 64, 79, 94, 109 };
     test_temperature_spread( &fakeworld, &dummy, temp_spread, "No" );
 }

 // Lightly clothed target temperatures
 equip_clothing( &fakeworld, &dummy, "hat_ball");
 equip_clothing( &fakeworld, &dummy, "bandana");
 equip_clothing( &fakeworld, &dummy, "tshirt");
 equip_clothing( &fakeworld, &dummy, "gloves_fingerless");
 equip_clothing( &fakeworld, &dummy, "jeans");
 equip_clothing( &fakeworld, &dummy, "socks");
 equip_clothing( &fakeworld, &dummy, "sneakers");

 {
     int temp_spread[] = { -3, 12, 27, 42, 57, 72, 87 };
     test_temperature_spread( &fakeworld, &dummy, temp_spread, "Light" );
 }

 dummy.worn.clear();

 // Heavily clothed target temperatures
 equip_clothing( &fakeworld, &dummy, "hat_knit");
 equip_clothing( &fakeworld, &dummy, "tshirt");
 equip_clothing( &fakeworld, &dummy, "vest");
 equip_clothing( &fakeworld, &dummy, "trenchcoat");
 equip_clothing( &fakeworld, &dummy, "gloves_wool");
 equip_clothing( &fakeworld, &dummy, "long_underpants");
 equip_clothing( &fakeworld, &dummy, "pants_army");
 equip_clothing( &fakeworld, &dummy, "wool_socks");
 equip_clothing( &fakeworld, &dummy, "boots");

 {
     int temp_spread[] = { -25, -10, 5, 20, 35, 50, 65 };
     test_temperature_spread( &fakeworld, &dummy, temp_spread, "Heavy" );
 }

 dummy.worn.clear();

 // Artic gear target temperatures
 equip_clothing( &fakeworld, &dummy, "balclava");
 equip_clothing( &fakeworld, &dummy, "goggles_ski");
 equip_clothing( &fakeworld, &dummy, "hat_hunting");
 equip_clothing( &fakeworld, &dummy, "under_armor");
 equip_clothing( &fakeworld, &dummy, "vest");
 equip_clothing( &fakeworld, &dummy, "coat_winter");
 equip_clothing( &fakeworld, &dummy, "gloves_liner");
 equip_clothing( &fakeworld, &dummy, "gloves_winter");
 equip_clothing( &fakeworld, &dummy, "long_underpants");
 equip_clothing( &fakeworld, &dummy, "pants_fur");
 equip_clothing( &fakeworld, &dummy, "wool_socks");
 equip_clothing( &fakeworld, &dummy, "boots_winter");

 {
     int temp_spread[] = { -47, -32, -17, -2, 13, 28, 43 };
     test_temperature_spread( &fakeworld, &dummy, temp_spread, "Artic" );
 }

 dummy.worn.clear();

 ok1( dummy.health == 0 );

 return exit_status();
}
