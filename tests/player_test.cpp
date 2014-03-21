#include <tap++/tap++.h>
using namespace TAP;

#include <string>

#include "player.h"
#include "game.h"
#include "overmapbuffer.h"
#include "item_factory.h"

// Set the stage for a particular ambient and target temperature and run update_bodytemp() until
// core body temperature settles.
void temperature_check( player *p, int ambient_temp, int target_temp, std::string clothing )
{
    g->temperature = ambient_temp;
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
        p->update_bodytemp();
    }
    char error_string[100];
    sprintf( error_string, "Testing with %s clothes at %dF, Expect %d, got %d",
             clothing.c_str(), ambient_temp, target_temp, p->temp_cur[0] );
    ok( target_temp - 100 < p->temp_cur[0] && target_temp + 100 > p->temp_cur[0],
        error_string );
}

void equip_clothing( player *p, std::string clothing )
{
    item article = item_controller->create(clothing, 0);
    p->wear_item( &article );
}

// Run the tests for each of the temperature setpoints.
// ambient_temps MUST have 7 values or we'll segfault.
void test_temperature_spread( player *p, int ambient_temps[], std::string clothing)
{
    temperature_check( p, ambient_temps[0], BODYTEMP_FREEZING, clothing );
    temperature_check( p, ambient_temps[1], BODYTEMP_VERY_COLD, clothing );
    temperature_check( p, ambient_temps[2], BODYTEMP_COLD, clothing );
    temperature_check( p, ambient_temps[3], BODYTEMP_NORM, clothing );
    temperature_check( p, ambient_temps[4], BODYTEMP_HOT, clothing );
    temperature_check( p, ambient_temps[5], BODYTEMP_VERY_HOT, clothing );
    temperature_check( p, ambient_temps[6], BODYTEMP_SCORCHING, clothing );
}

int main(int argc, char *argv[])
{
 plan(29);

 player dummy;

 g = new game;
 g->load_static_data();
 g->check_all_mod_data();

 dummy.normalize( );

 // Assume curses
 initOptions();
 load_options();
 initscr();

 dummy.name = "dummy";
 g->m = map( &g->traps );
 g->u = dummy;
 g->cur_om = &overmap_buffer.get( 0, 0 );
 g->m.load( g->levx, g->levy, g->levz );


 // See http://personal.cityu.edu.hk/~bsapplec/heat.htm for temperature basis.
 // As we aren't modelling metabolic rate, assume 2 METS when not sleeping.
 // Obviously though 0.7 METS when sleeping is called for.

 // I'm not sure how to apply +1 METS as a temperature offset,
 // treating it as a 12C/54F boost across the board

 // Nude target temperatures
 {
     int temp_spread[] = { 19, 34, 49, 64, 79, 94, 109 };
     test_temperature_spread( &dummy, temp_spread, "No" );
 }

 // Lightly clothed target temperatures
 equip_clothing( &dummy, "hat_ball");
 equip_clothing( &dummy, "bandana");
 equip_clothing( &dummy, "tshirt");
 equip_clothing( &dummy, "gloves_fingerless");
 equip_clothing( &dummy, "jeans");
 equip_clothing( &dummy, "socks");
 equip_clothing( &dummy, "sneakers");

 {
     int temp_spread[] = { -3, 12, 27, 42, 57, 72, 87 };
     test_temperature_spread( &dummy, temp_spread, "Light" );
 }

 dummy.worn.clear();

 // Heavily clothed target temperatures
 equip_clothing( &dummy, "hat_knit");
 equip_clothing( &dummy, "tshirt");
 equip_clothing( &dummy, "vest");
 equip_clothing( &dummy, "trenchcoat");
 equip_clothing( &dummy, "gloves_wool");
 equip_clothing( &dummy, "long_underpants");
 equip_clothing( &dummy, "pants_army");
 equip_clothing( &dummy, "wool_socks");
 equip_clothing( &dummy, "boots");

 {
     int temp_spread[] = { -25, -10, 5, 20, 35, 50, 65 };
     test_temperature_spread( &dummy, temp_spread, "Heavy" );
 }

 dummy.worn.clear();

 // Artic gear target temperatures
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

 {
     int temp_spread[] = { -47, -32, -17, -2, 13, 28, 43 };
     test_temperature_spread( &dummy, temp_spread, "Artic" );
 }

 dummy.worn.clear();

 ok( dummy.health == 0 );

 return exit_status();
}
