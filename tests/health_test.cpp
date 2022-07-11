#include "cata_catch.h"

#include <array>
#include <cstddef>
#include <sstream>
#include <vector>

#include "calendar.h"
#include "enums.h"
#include "npc.h"

static const vitamin_id vitamin_calcium( "calcium" );
static const vitamin_id vitamin_iron( "iron" );
static const vitamin_id vitamin_vitC( "vitC" );

static void daily_routine( npc &dude, int numb_stam_burn, int vitamin_amount,
                           bool sleep_deprivation )
{
    // set to midnight
    calendar::turn = calendar::turn_zero;
    dude.update_body();

    for( int i = 0; i < numb_stam_burn; i++ ) {
        //Cardio: burn more than half stamina
        dude.mod_stamina( -dude.get_stamina_max() );
        dude.set_stamina( dude.get_stamina_max() );
    }

    //RDA: get some vitamins
    dude.vitamin_mod( vitamin_iron, vitamin_amount );
    dude.vitamin_mod( vitamin_vitC, vitamin_amount );
    dude.vitamin_mod( vitamin_calcium, vitamin_amount );

    if( sleep_deprivation ) {
        dude.set_sleep_deprivation( static_cast<int>( SLEEP_DEPRIVATION_MASSIVE ) - 160 );
    }


    // set to next day
    calendar::turn = calendar::turn_zero + 24_hours;
    dude.update_body();
    dude.update_health();
}

// Healthy lifetyle makes health go up
TEST_CASE( "healthy_lifestyle", "[health]" )
{
    standard_npc dude( "healthy lifestyle" );

    int init_lifestyle = dude.get_lifestyle();
    int init_daily_health = dude.get_daily_health();
    for( int i = 0; i < 7; i++ ) {
        daily_routine( dude, 5, 2000, false );
    }

    INFO( "Lifestyle value: " << dude.get_lifestyle() );
    CHECK( dude.get_lifestyle() > init_lifestyle );

    INFO( "Daily Health: " << dude.get_daily_health() );
    CHECK( dude.get_daily_health() > init_daily_health );
}

// Unhealthy lifestyle makes health go down
TEST_CASE( "unhealthy_lifestyle", "[health]" )
{
    standard_npc dude( "unhealthy lifestyle" );

    int init_lifestyle = dude.get_lifestyle();
    int init_daily_health = dude.get_daily_health();
    for( int i = 0; i < 7; i++ ) {
        daily_routine( dude, 0, 0, true );
    }

    INFO( "Lifestyle value: " << dude.get_lifestyle() );
    CHECK( dude.get_lifestyle() < init_lifestyle );

    INFO( "Daily Health: " << dude.get_daily_health() );
    CHECK( dude.get_daily_health() < init_daily_health );
}

