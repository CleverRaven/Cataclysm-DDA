#include <cstdlib>
#include <memory>

#include "calendar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "point.h"
#include "weather.h"

static bool is_nearly( float value, float expected )
{
    // Rounding errors make the values change around a bit
    // Lets just check that they are within 1% of expected

    bool ret_val = std::abs( value - expected ) / expected < 0.01;
    return ret_val;
}

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

        item normal_item( "meat_cooked" );

        item freeze_item( "offal_canned" );

        item sealed_item( "offal_canned" );
        sealed_item = sealed_item.in_its_container();

        set_map_temperature( 65 ); // 18,3 C

        normal_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        sealed_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        freeze_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );

        // Item should exist with no rot when it is brand new
        CHECK( to_turns<int>( normal_item.get_rot() ) == 0 );
        CHECK( to_turns<int>( sealed_item.get_rot() ) == 0 );
        CHECK( to_turns<int>( freeze_item.get_rot() ) == 0 );

        calendar::turn = to_turn<int>( calendar::turn + 20_minutes );
        normal_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        sealed_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        freeze_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_FREEZER );

        // After 20 minutes the normal item should have 20 minutes of rot
        CHECK( is_nearly( to_turns<int>( normal_item.get_rot() ), to_turns<int>( 20_minutes ) ) );
        // Item in freezer and in preserving container should have no rot
        CHECK( to_turns<int>( sealed_item.get_rot() ) == 0 );
        CHECK( to_turns<int>( freeze_item.get_rot() ) == 0 );

        // Move time 110 minutes
        calendar::turn = to_turn<int>( calendar::turn + 110_minutes );
        sealed_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_NORMAL );
        freeze_item.process( nullptr, tripoint_zero, false, 1, temperature_flag::TEMP_FREEZER );
        // In freezer and in preserving container still should be no rot
        CHECK( to_turns<int>( sealed_item.get_rot() ) == 0 );
        CHECK( to_turns<int>( freeze_item.get_rot() ) == 0 );

        // The item in freezer should still not be frozen
        CHECK( !freeze_item.item_tags.count( "FROZEN" ) );
    }
}
