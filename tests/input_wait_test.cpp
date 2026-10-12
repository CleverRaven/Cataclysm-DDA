#include <cstdint>
#include <initializer_list>
#include <optional>

#include "cata_catch.h"
#include "input_wait.h"

TEST_CASE( "ms_until_elapsed_mirrors_unsigned_tick_predicates", "[input_wait]" )
{
    GIVEN( "started 100 ms ago" ) {
        THEN( "strict 300 ms predicate fires in 201 ms, non-strict in 200 ms" ) {
            CHECK( ms_until_elapsed_exceeds( 1000, 1100, 300 ) == 201u );
            CHECK( ms_until_elapsed_reaches( 1000, 1100, 300 ) == 200u );
        }
    }
    GIVEN( "a start exactly one period ago" ) {
        THEN( "non-strict predicate holds, strict one waits 1 ms" ) {
            CHECK( ms_until_elapsed_reaches( 1000, 1300, 300 ) == 0u );
            CHECK( ms_until_elapsed_exceeds( 1000, 1300, 300 ) == 1u );
        }
    }
    GIVEN( "a start of 0, as finger_repeat_time holds before the first repeat" ) {
        for( const uint32_t now : {
                 5000u, 0x7FFFFFFFu, 0x80000000u, 0x80000001u, 0xFFFFFFFFu
             } ) {
            CAPTURE( now );
            CHECK( ms_until_elapsed_exceeds( 0, now, 500 ) == 0u );
        }
    }
    GIVEN( "a start just before the tick counter wraps" ) {
        THEN( "elapsed time wraps with it" ) {
            CHECK( ms_until_elapsed_exceeds( 0xFFFFFFF0u, 0x10u, 300 ) == 300u - 32u + 1u );
            CHECK( ms_until_elapsed_reaches( 0xFFFFFFF0u, 0x10u, 25 ) == 0u );
        }
    }
}

TEST_CASE( "input_wait_blocks_until_the_earliest_wake", "[input_wait]" )
{
    GIVEN( "nothing pending, no input timeout" ) {
        const input_wait_state s;
        THEN( "it waits without a timeout" ) {
            CHECK( input_wait_timeout_ms( s ) == -1 );
        }
    }
    GIVEN( "an input timeout" ) {
        for( const uint32_t left : {
                 0u, 1u, 30u, 125u
             } ) {
            CAPTURE( left );
            input_wait_state s;
            s.input_ms = left;
            CHECK( input_wait_timeout_ms( s ) == static_cast<int>( left ) );
        }
    }
    GIVEN( "throttled present due before the input timeout" ) {
        input_wait_state s;
        s.input_ms = 100;
        s.present_ms = 15;
        THEN( "it wakes for the present" ) {
            CHECK( input_wait_timeout_ms( s ) == 15 );
        }
    }
    GIVEN( "a platform timer earliest and no present pending" ) {
        input_wait_state s;
        s.input_ms = 100;
        s.platform_ms = 7;
        THEN( "it wakes for the timer" ) {
            CHECK( input_wait_timeout_ms( s ) == 7 );
        }
    }
    GIVEN( "an overdue platform timer while a throttled present is pending" ) {
        // CheckMessages skips touch timers until the present clears needupdate,
        // so waking for the timer before then would spin
        input_wait_state s;
        s.platform_ms = 0;
        s.present_ms = 15;
        THEN( "it waits for the present instead of polling" ) {
            CHECK( input_wait_timeout_ms( s ) == 15 );
        }
    }
}

TEST_CASE( "touch_wait_matches_checkmessages_timers", "[input_wait]" )
{
    touch_timers t;
    t.initial_delay = 300;
    t.finger_repeat_delay = 100;
    t.present_interval = 25;
    GIVEN( "nothing touched" ) {
        t.now = 1000;
        THEN( "no timer is armed" ) {
            CHECK_FALSE( touch_wait_ms( t ) );
        }
    }
    GIVEN( "back button held 100 ms" ) {
        t.back_down_time = 1000;
        t.now = 1100;
        THEN( "toggle fires just past the delay" ) {
            CHECK( touch_wait_ms( t ) == std::optional<uint32_t>( 201 ) );
        }
        WHEN( "toggle was handled" ) {
            t.back_toggle_handled = true;
            THEN( "no timer is armed" ) {
                CHECK_FALSE( touch_wait_ms( t ) );
            }
        }
    }
    GIVEN( "one finger down 100 ms with no repeat yet, at any uptime" ) {
        for( const uint32_t base : {
                 0u, 0x7FFFFF00u, 0x80000100u, 0xFFFFFF00u
             } ) {
            CAPTURE( base );
            t.finger_repeat_time = 0;
            t.finger_down_time = base + 1000;
            t.now = base + 1100;
            CHECK( touch_wait_ms( t ) == std::optional<uint32_t>( 201 ) );
        }
    }
    GIVEN( "one finger repeating, last repeat 40 ms ago" ) {
        t.finger_down_time = 1000;
        t.finger_repeat_time = 1400;
        t.now = 1440;
        THEN( "next repeat follows the repeat delay" ) {
            CHECK( touch_wait_ms( t ) == std::optional<uint32_t>( 61 ) );
        }
        WHEN( "a second finger joined" ) {
            t.multi_finger_touch = true;
            THEN( "no repeat is armed" ) {
                CHECK_FALSE( touch_wait_ms( t ) );
            }
        }
    }
    GIVEN( "first tap 100 ms ago waiting for a second" ) {
        t.last_tap_time = 2000;
        t.now = 2100;
        THEN( "single tap fires at the delay" ) {
            CHECK( touch_wait_ms( t ) == std::optional<uint32_t>( 200 ) );
        }
    }
    GIVEN( "a quick shortcut held past the hint delay" ) {
        t.quick_shortcut_touch = true;
        t.finger_down_time = 1000;
        t.last_present = 1500;
        t.now = 1510;
        THEN( "it repaints at the present rate, not in a spin" ) {
            CHECK( touch_wait_ms( t ) == std::optional<uint32_t>( 15 ) );
        }
    }
}
