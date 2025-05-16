#include <string>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "npc.h"
#include "type_id.h"

static const efftype_id effect_lying_down( "lying_down" );
static const efftype_id effect_npc_suspend( "npc_suspend" );
static const efftype_id effect_sleep( "sleep" );

static const vitamin_id vitamin_calcium( "calcium" );
static const vitamin_id vitamin_iron( "iron" );
static const vitamin_id vitamin_vitC( "vitC" );

static void pass_time( Character &p, time_duration amt, time_duration tick )
{
    for( time_duration turns = 1_turns; turns < amt; turns += tick ) {
        calendar::turn += tick;
        p.update_body();
        p.update_health();
    }
}

static void daily_routine( npc &dude, int numb_stam_burn, int vitamin_amount,
                           bool sleep_deprivation )
{
    // set to midnight
    calendar::turn = calendar::turn_zero;
    dude.remove_effect( effect_sleep );
    dude.remove_effect( effect_lying_down );
    dude.remove_effect( effect_npc_suspend );
    dude.cancel_activity();
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

    // pass a day
    pass_time( dude, 24_hours, 1_hours );
}

// Healthy lifetyle makes health go up
TEST_CASE( "healthy_lifestyle", "[health]" )
{
    standard_npc dude( "healthy lifestyle" );
    dude.set_stored_kcal( dude.get_healthy_kcal() );

    int init_lifestyle = dude.get_lifestyle();

    for( int i = 0; i < 7; i++ ) {
        daily_routine( dude, 5, 2000, false );
    }

    INFO( "Lifestyle value: " << dude.get_lifestyle() );
    CHECK( dude.get_lifestyle() > init_lifestyle );
}

// Unhealthy lifestyle makes health go down
TEST_CASE( "unhealthy_lifestyle", "[health]" )
{
    standard_npc dude( "unhealthy lifestyle" );
    dude.set_stored_kcal( dude.get_healthy_kcal() );

    int init_lifestyle = dude.get_lifestyle();

    for( int i = 0; i < 7; i++ ) {
        daily_routine( dude, 0, 0, true );
    }

    INFO( "Lifestyle value: " << dude.get_lifestyle() );
    CHECK( dude.get_lifestyle() < init_lifestyle );

}

