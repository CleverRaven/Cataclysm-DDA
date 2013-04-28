/* Needs to come before tap lib for some reason? */
#include "player.h"
#include "game.h"
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

int main(int argc, char *argv[])
{
 plan_tests(29);

 game fakeworld;
 player dummy;

 dummy.normalize( &fakeworld );

 dummy.name = "dummy";
 fakeworld.m = map( &fakeworld.itypes, &fakeworld.mapitems, &fakeworld.traps );
 fakeworld.u = dummy;
 fakeworld.cur_om = overmap( &fakeworld, 0, 0 );
 fakeworld.m.load( &fakeworld, fakeworld.levx, fakeworld.levy, fakeworld.levz );


 // See http://personal.cityu.edu.hk/~bsapplec/heat.htm for temperature basis.
 // As we aren't modelling metabolic rate, assume 2 METS when not sleeping.
 // Obviously though 0.7 METS when sleeping is called for.

 // I'm not sure how to apply +1 METS as a temperature offset,
 // treating it as a 12C/54F boost across the board

 // Nude target temperatures
 temperature_check( &fakeworld, &dummy, 19, BODYTEMP_FREEZING, "No" );
 temperature_check( &fakeworld, &dummy, 34, BODYTEMP_VERY_COLD, "No" );
 temperature_check( &fakeworld, &dummy, 49, BODYTEMP_COLD, "No" );
 temperature_check( &fakeworld, &dummy, 64, BODYTEMP_NORM, "No" );
 temperature_check( &fakeworld, &dummy, 79, BODYTEMP_HOT, "No" );
 temperature_check( &fakeworld, &dummy, 94, BODYTEMP_VERY_HOT, "No" );
 temperature_check( &fakeworld, &dummy, 109, BODYTEMP_SCORCHING, "No" );

 // Lightly clothed target temperatures
 equip_clothing( &fakeworld, &dummy, "hat_ball");
 equip_clothing( &fakeworld, &dummy, "bandana");
 equip_clothing( &fakeworld, &dummy, "tshirt");
 equip_clothing( &fakeworld, &dummy, "gloves_fingerless");
 equip_clothing( &fakeworld, &dummy, "jeans");
 equip_clothing( &fakeworld, &dummy, "socks");
 equip_clothing( &fakeworld, &dummy, "sneakers");

 temperature_check( &fakeworld, &dummy, -3, BODYTEMP_FREEZING, "Light" );
 temperature_check( &fakeworld, &dummy, 12, BODYTEMP_VERY_COLD, "Light" );
 temperature_check( &fakeworld, &dummy, 27, BODYTEMP_COLD, "Light" );
 temperature_check( &fakeworld, &dummy, 42, BODYTEMP_NORM, "Light" );
 temperature_check( &fakeworld, &dummy, 57, BODYTEMP_HOT, "Light" );
 temperature_check( &fakeworld, &dummy, 72, BODYTEMP_VERY_HOT, "Light" );
 temperature_check( &fakeworld, &dummy, 87, BODYTEMP_SCORCHING, "Light" );

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

 temperature_check( &fakeworld, &dummy, -25, BODYTEMP_FREEZING, "Heavy" );
 temperature_check( &fakeworld, &dummy, -10, BODYTEMP_VERY_COLD, "Heavy" );
 temperature_check( &fakeworld, &dummy,  5, BODYTEMP_COLD, "Heavy" );
 temperature_check( &fakeworld, &dummy, 20, BODYTEMP_NORM, "Heavy" );
 temperature_check( &fakeworld, &dummy, 35, BODYTEMP_HOT, "Heavy" );
 temperature_check( &fakeworld, &dummy, 50, BODYTEMP_VERY_HOT, "Heavy" );
 temperature_check( &fakeworld, &dummy, 65, BODYTEMP_SCORCHING, "Heavy" );

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

 temperature_check( &fakeworld, &dummy, -47, BODYTEMP_FREEZING, "Artic" );
 temperature_check( &fakeworld, &dummy, -32, BODYTEMP_VERY_COLD, "Artic" );
 temperature_check( &fakeworld, &dummy, -17, BODYTEMP_COLD, "Artic" );
 temperature_check( &fakeworld, &dummy, -2, BODYTEMP_NORM, "Artic" );
 temperature_check( &fakeworld, &dummy, 13, BODYTEMP_HOT, "Artic" );
 temperature_check( &fakeworld, &dummy, 28, BODYTEMP_VERY_HOT, "Artic" );
 temperature_check( &fakeworld, &dummy, 43, BODYTEMP_SCORCHING, "Artic" );

 dummy.worn.clear();

 ok1( dummy.health == 0 );

 return exit_status();
}
