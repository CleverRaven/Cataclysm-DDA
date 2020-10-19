#include <algorithm>
#include <array>
#include <cstddef>
#include <sstream>
#include <vector>

#include "calendar.h"
#include "catch/catch.hpp"
#include "enums.h"
#include "npc.h"


static const efftype_id effect_sleep( "sleep" );

static void update_body_for( Character &dude, time_duration time )
{
    const time_point start = calendar::turn_zero;
    const time_point end = start + time;
    for( time_point now = start; now < end; now += 5_minutes ) {
        dude.update_body( now, now + 5_minutes );
    }
}

TEST_CASE( "must_sleep_8_hours", "[fatigue]" )
{
    standard_npc dude( "sleeper", tripoint( -1, -1, 0 ) );
    dude.set_fatigue( 0 );
    WHEN( "The character is active for 16 hours" ) {
        update_body_for( dude, 16_hours );
        int fatigue_before_sleep = dude.get_fatigue();
        CAPTURE( fatigue_before_sleep );
        AND_WHEN( "The character sleeps for 7 hours and 30 minutes" ) {
            dude.add_effect( effect_sleep, 24_hours );
            update_body_for( dude, 7_hours + 30_minutes );
            THEN( "The character is not fully rested" ) {
                CHECK( dude.get_fatigue() > 0 );
            }
        }
        AND_WHEN( "The character sleeps for 8 hours" ) {
            dude.add_effect( effect_sleep, 24_hours );
            update_body_for( dude, 8_hours );
            THEN( "The character is fully rested" ) {
                CHECK( dude.get_fatigue() == 0 );
            }
        }
    }
}

TEST_CASE( "sleep_deprivation_rate", "[fatigue]" )
{
    standard_npc dude( "sleeper", tripoint( -1, -1, 0 ) );
    dude.set_fatigue( 0 );
    dude.set_sleep_deprivation( 0 );
    WHEN( "The character is active for 18 hours" ) {
        update_body_for( dude, 18_hours );
        int deprivation_before_sleep = dude.get_sleep_deprivation();
        CAPTURE( deprivation_before_sleep );
        AND_WHEN( "The character sleeps for 5 hours and 30 minutes" ) {
            dude.add_effect( effect_sleep, 24_hours );
            update_body_for( dude, 5_hours + 30_minutes );
            THEN( "The character has some sleep deprivation" ) {
                CHECK( dude.get_sleep_deprivation() > 0 );
            }
        }
        AND_WHEN( "The character sleeps for 6 hours" ) {
            dude.add_effect( effect_sleep, 24_hours );
            update_body_for( dude, 6_hours );
            THEN( "The character has no sleep deprivation" ) {
                CHECK( dude.get_sleep_deprivation() == 0 );
            }
        }
    }
}
