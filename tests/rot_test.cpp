#include <cstdlib>
#include <memory>

#include "calendar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "point.h"
#include "weather.h"

static void set_map_temperature( int new_temperature )
{
    g->weather.temperature = new_temperature;
    g->weather.clear_temp_cache();
}

TEST_CASE( "Rate of rotting" )
{
    SECTION( "Passage of time" ) {
        // Item rot is a time duration.
        // At 65 F (18,3 C) item rots at rate of 1h/1h
        // So the level of rot should be about same as the item age
        // In preserving containers and in freezer the item should not rot at all
        // Item in freezer should not be frozen.

        // Items created at turn zero are handled differently, so ensure we're
        // not there.
        if( calendar::turn <= calendar::start_of_cataclysm ) {
            calendar::turn = calendar::start_of_cataclysm + 1_minutes;
        }

        item normal_item( "meat_cooked" );

        item freeze_item( "offal_canned" );

        item sealed_item( "offal_canned" );
        sealed_item = sealed_item.in_its_container();

        set_map_temperature( 65 ); // 18,3 C

        normal_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::NORMAL );
        sealed_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::NORMAL );
        freeze_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::NORMAL );

        // Item should exist with no rot when it is brand new
        CHECK( normal_item.get_rot() == 0_turns );
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );

        INFO( "Initial turn: " << to_turn<int>( calendar::turn ) );

        calendar::turn += 20_minutes;
        normal_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::NORMAL );
        sealed_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::NORMAL );
        freeze_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::FREEZER );

        // After 20 minutes the normal item should have 20 minutes of rot
        CHECK( to_turns<int>( normal_item.get_rot() )
               == Approx( to_turns<int>( 20_minutes ) ).epsilon( 0.01 ) );
        // Item in freezer and in preserving container should have no rot
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );

        // Move time 110 minutes
        calendar::turn += 110_minutes;
        sealed_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::NORMAL );
        freeze_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::FREEZER );
        // In freezer and in preserving container still should be no rot
        CHECK( sealed_item.get_rot() == 0_turns );
        CHECK( freeze_item.get_rot() == 0_turns );

        // The item in freezer should still not be frozen
        CHECK( !freeze_item.item_tags.count( "FROZEN" ) );
    }
}

TEST_CASE( "Items rot away" )
{
    SECTION( "Item in reality bubble rots away" ) {
        // Item should rot away when it has 2x of its shelf life in rot.

        if( calendar::turn <= calendar::start_of_cataclysm ) {
            calendar::turn = calendar::start_of_cataclysm + 1_minutes;
        }

        item test_item( "meat_cooked" );

        // Process item once to set all of its values.
        test_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::HEATER );

        // Set rot to >2 days and process again. process_temperature_rot should return true.
        calendar::turn += 20_minutes;
        test_item.mod_rot( 2_days );

        CHECK( test_item.process_temperature_rot( 1, tripoint_zero, nullptr,
                temperature_flag::HEATER ) );
        INFO( "Rot: " << to_turns<int>( test_item.get_rot() ) );
    }
}
