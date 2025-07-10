#include <numeric>
#include <string>
#include <vector>

#include "activity_tracker.h"
#include "calendar.h"
#include "cata_catch.h"
#include "game_constants.h"
#include "rng.h"

static void test_activity_tracker( const std::vector<float> &values )
{
    activity_tracker tracker;
    for( float i : values ) {
        calendar::turn += 1_turns;
        tracker.new_turn( false );
        // If we're on a "new turn", we should have nominal activity.
        CHECK( tracker.activity( false ) == 1.0 );
        // Smaller values inserted before and after the highest value should be irrelevant.
        tracker.log_activity( rng_float( 0.0f, i - 0.01f ) );
        tracker.log_activity( i );
        tracker.log_activity( rng_float( 0.0f, i - 0.01f ) );
        // Verify the highest value inserted is the current value.
        CHECK( tracker.activity( false ) == i );
    }
    int end_value = values.back();
    // activity() still returns most recently logged activity.
    CHECK( tracker.activity( false ) == end_value );
    const float expected_activity = std::accumulate( values.begin(), values.end(), 0.0f ) /
                                    static_cast<float>( values.size() );
    // average_activity() returns average of the most recent period.
    CHECK( tracker.average_activity() == expected_activity );
    tracker.reset_activity_level();
    // activity() should be unchanged after a reset. (it's still the same turn)
    CHECK( tracker.activity( false ) == end_value );
    // average_activity() also continues to return the previous value.
    CHECK( tracker.average_activity() == expected_activity );
    calendar::turn += 1_turns;
    // activity() returns 1.0 now that it's a new turn.
    CHECK( tracker.activity( false ) == 1.0f );
    tracker.new_turn( false );
    tracker.log_activity( 5.0f );
    // After starting a new recording cycle, activity() and average_activity() return the new data.
    CHECK( tracker.activity( false ) == 5.0f );
    CHECK( tracker.average_activity() == 5.0f );
    calendar::turn += 1_turns;
    tracker.new_turn( false );
    tracker.log_activity( 7.0f );
    // And the behavior continues.
    CHECK( tracker.activity( false ) == 7.0f );
    CHECK( tracker.average_activity() == 6.0f );
    calendar::turn = calendar::turn_zero;
}

TEST_CASE( "activity_counter_from_1_to_300", "[activity_tracker]" )
{
    std::vector<float> values;
    for( int i = 1; i <= 300; ++i ) {
        values.push_back( i );
    }
    calendar::turn += 50000_turns;
    test_activity_tracker( values );
    calendar::turn = calendar::turn_zero;
}

TEST_CASE( "activity_tracker_from_300_to_1", "[activity_tracker]" )
{
    std::vector<float> values;
    for( int i = 300; i > 0; --i ) {
        values.push_back( i );
    }
    test_activity_tracker( values );
}

TEST_CASE( "activity_tracker_constant_value", "[activity_tracker]" )
{
    std::vector<float> values( 300, 1.0 );
    test_activity_tracker( values );
}

TEST_CASE( "activity_tracker_intermittent_values", "[activity_tracker]" )
{
    std::vector<float> values( 300, 1.0 );
    for( int i = 0; i < 300; i += 30 ) {
        values[ i ] = 10.0f;
    }
    test_activity_tracker( values );
}

TEST_CASE( "activity_tracker_string_representation", "[activity_tracker]" )
{
    activity_tracker tracker;
    // Start at the lowest level
    tracker.reset_activity_level();
    REQUIRE( tracker.activity_level_str() == "SLEEP_EXERCISE" );

    // Increase level a couple times
    tracker.log_activity( LIGHT_EXERCISE );
    CHECK( tracker.activity_level_str() == "LIGHT_EXERCISE" );
    tracker.log_activity( MODERATE_EXERCISE );
    CHECK( tracker.activity_level_str() == "MODERATE_EXERCISE" );
    // Cannot 'increase' to lower level
    tracker.log_activity( LIGHT_EXERCISE );
    CHECK( tracker.activity_level_str() == "MODERATE_EXERCISE" );
    tracker.log_activity( NO_EXERCISE );
    CHECK( tracker.activity_level_str() == "MODERATE_EXERCISE" );
    // Increase to highest level
    tracker.log_activity( ACTIVE_EXERCISE );
    CHECK( tracker.activity_level_str() == "ACTIVE_EXERCISE" );
    tracker.log_activity( EXTRA_EXERCISE );
    CHECK( tracker.activity_level_str() == "EXTRA_EXERCISE" );
    // Cannot increase beyond the highest
    tracker.log_activity( EXTRA_EXERCISE );
    CHECK( tracker.activity_level_str() == "EXTRA_EXERCISE" );
}
