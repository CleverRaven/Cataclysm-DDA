#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "map.h"

TEST_CASE( "countdown_action_triggering", "[item]" )
{
    item grenade( "grenade_act" );
    grenade.active = true;

    SECTION( "countdown_point is in future" ) {
        grenade.countdown_point = calendar::turn + 10_seconds;
        // Grenade does not explode
        CHECK( grenade.process( get_map(), nullptr, tripoint_zero ) == false );
    }

    SECTION( "countdown_point is in past" ) {
        grenade.countdown_point = calendar::turn - 10_seconds;
        // Grenade explodes and is to be removed
        CHECK( grenade.process( get_map(), nullptr, tripoint_zero ) == true );
    }

    SECTION( "countdown_point is now" ) {
        grenade.countdown_point = calendar::turn;
        // Grenade explodes and is to be removed
        CHECK( grenade.process( get_map(), nullptr, tripoint_zero ) == true );
    }
}

TEST_CASE( "countdown_action_revert_to", "[item]" )
{
    SECTION( "revert to inert item" ) {
        item test_item( "arrow_flamming" );
        test_item.active = true;
        test_item.countdown_point = calendar::turn;

        // Is not deleted after coundown action
        CHECK( test_item.process( get_map(), nullptr, tripoint_zero ) == false );

        // Turns into normal arrow
        CHECK( test_item.typeId().str() == "arrow_field_point_fletched" );

        // Is not active anymore
        CHECK_FALSE( test_item.active );

        // Timer is gone
        CHECK( test_item.countdown_point == calendar::turn_max );
    }

    SECTION( "revert to item with new timer" ) {
        item test_item( "migo_plate_undergrown" );
        test_item.active = true;
        test_item.countdown_point = calendar::turn;

        // Is not deleted after coundown action
        CHECK( test_item.process( get_map(), nullptr, tripoint_zero ) == false );

        // Turns into new armor type
        CHECK( test_item.typeId().str() == "migo_plate" );

        // Is still active
        CHECK( test_item.active );

        // Has new timer
        CHECK( test_item.countdown_point == calendar::turn + 24_hours );
    }

    SECTION( "revert to item that requires processing" ) {
        item test_item( "test_rock_cheese" );
        test_item.active = true;
        test_item.countdown_point = calendar::turn;

        // Is not deleted after coundown action
        CHECK( test_item.process( get_map(), nullptr, tripoint_zero ) == false );

        // Turns into cheese
        CHECK( test_item.typeId().str() == "cheese_hard" );

        // Is still active
        CHECK( test_item.active );

        // Timer is gone
        CHECK( test_item.countdown_point == calendar::turn_max );
    }
}
