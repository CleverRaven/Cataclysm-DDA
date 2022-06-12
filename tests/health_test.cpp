#include "cata_catch.h"

#include <array>
#include <cstddef>
#include <sstream>
#include <vector>

#include "calendar.h"
#include "enums.h"
#include "npc.h"

static void daily_routine( npc &dude, int numb_stam_burn, int vitamin_amount )
{
    for( size_t i = 0; i < numb_stam_burn; i++ ) {
        //Cardio: burn more than half stamina
        dude.mod_stamina( -0.6 * dude.get_stamina_max() );
        dude.set_stamina( dude.get_stamina_max() );
    }

    //RDA: get some vitamins
    dude.vitamin_mod( vitamin_id( "iron" ), vitamin_amount );
    dude.vitamin_mod( vitamin_id( "vitC" ), vitamin_amount );
    dude.vitamin_mod( vitamin_id( "calcium" ), vitamin_amount );

    // set to midnight
    calendar::turn = calendar::turn_zero;
    dude.update_body();
}

// Healthy lifetyle makes health go up
TEST_CASE( "healthy_lifestyle", "[health]" )
{
    standard_npc dude( "healthy lifestyle" );

    int init_lifestyle = dude.get_lifestyle();
    int init_daily_health = dude.get_daily_health();
    for( size_t i = 0; i < 7; i++ ) {
        daily_routine( dude, 5, 2000 );
    }

    INFO( "Lifestyle value: " << dude.get_lifestyle() );
    CHECK( dude.get_lifestyle() > init_lifestyle );

    INFO( "Health tally: " << dude.get_daily_health() );
    CHECK( dude.get_daily_health() > init_daily_health );
}

// Unhealthy lifestyle makes health go down
TEST_CASE( "unhealthy_lifestyle", "[health]" )
{
    standard_npc dude( "unhealthy lifestyle" );

    int init_lifestyle = dude.get_lifestyle();
    int init_daily_health = dude.get_daily_health();
    for( size_t i = 0; i < 7; i++ ) {
        daily_routine( dude, 0, 0 );
    }

    INFO( "Lifestyle value: " << dude.get_lifestyle() );
    CHECK( dude.get_lifestyle() < init_lifestyle );

    INFO( "Health tally: " << dude.get_daily_health() );
    CHECK( dude.get_daily_health() < init_daily_health );
}

