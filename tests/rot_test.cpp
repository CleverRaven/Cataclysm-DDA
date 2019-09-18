#include <stdlib.h>
#include <memory>
#include <string>

#include "catch/catch.hpp"
#include "calendar.h"
#include "item.h"
#include "cata_utility.h"
#include "game.h"
#include "flat_set.h"
#include "point.h"


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
    SECTION( "65 F" ) {
        // Item rot is a time duration.
        // At 65 F (18,3 C) item rots at rate of 1h/1h
        // So the level of rot should be about same as the item age

        item test_item( "meat_cooked" );

        set_map_temperature( 65 ); // 18,3 C

        test_item.process_temperature_rot( 1, tripoint_zero, nullptr );

        // Item should exist with no rot when it is brand new
        CHECK( to_turns<int>( test_item.get_rot() ) == 0 );


        calendar::turn = to_turn<int>( calendar::turn + 20_minutes );
        test_item.process_temperature_rot( 1, tripoint_zero, nullptr );

        // After 20 minutes the item should have 20 minutes of rot
        CHECK( is_nearly( to_turns<int>( test_item.get_rot() ), to_turns<int>( 20_minutes ) ) );
    }
}
