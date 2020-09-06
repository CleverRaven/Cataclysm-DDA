#include "weary_test.h"

#include "player_helpers.h"
#include "map_helpers.h"

#include "avatar.h"
#include "catch/catch.hpp"

weariness_events do_activity( tasklist tasks )
{
    // Clear stuff, ensure we're in the right place
    clear_avatar();
    clear_map();


    avatar &guy = get_avatar();
    // How long we've been doing activities for
    time_duration spent = 0_seconds;
    // How weary we are starting
    int weariness_lvl = guy.weariness_level();
    weariness_events activity_log;
    while( tasks.duration() > spent ) {
        // What we're working on now
        const schedule &task = tasks.next_task();

        // Start our task, for however long the schedule says.
        // This may be longer than the interval, which means that we
        // never finish this task
        if( task.actor ) {
            guy.assign_activity( *task.actor, false );
        } else {
            guy.assign_activity( player_activity( task.act, calendar::INDEFINITELY_LONG, -1, INT_MIN,
                                                  "" ), false );
        }

        // How many turn's we've been at it
        time_duration turns = 0_seconds;
        while( turns <= task.interval ) {
            // Start each turn with a fresh set of moves
            guy.moves = 100;
            // Advance a turn
            calendar::turn += 1_turns;
            turns += 1_seconds;
            // Do our activity
            guy.activity.do_turn( guy );
            // Ensure we never actually finish an activity
            if( guy.activity.moves_left < 1000 ) {
                guy.activity.moves_left = 4000;
            }
            // Consume food, become weary, etc
            guy.update_body();
        }
        // Cancel our activity, now that we're done
        guy.cancel_activity();
        // How weary we are after ending this
        int new_weariness = guy.weariness_level();
        spent += task.interval;
        tasks.advance( task.interval );
        // If we're more weary than we were when we started, report it
        if( new_weariness != weariness_lvl ) {
            activity_log.log( weariness_lvl, new_weariness, spent );
            weariness_lvl = new_weariness;
        }
    }
    return activity_log;
}

TEST_CASE( "weary_assorted_tasks", "[weary][activities]" )
{
    const avatar &guy = get_avatar();

    static const int moves_for_25h = to_seconds<int>( 25_hours ) * 100;
    // Set up our scenarios ahead of time
    const dig_activity_actor dig_actor( moves_for_25h, tripoint_zero, "t_pit", tripoint_zero, 0, "" );
    const schedule task_dig( dig_actor, 5_minutes );

    tasklist desk_8h;
    desk_8h.enschedule( { activity_id( "ACT_FIRSTAID" ), 5_minutes }, 8_hours );

    tasklist moderate_8h;
    moderate_8h.enschedule( { activity_id( "ACT_PLANT_SEED" ), 5_minutes }, 8_hours );

    tasklist soldier_8h;
    soldier_8h.enschedule( task_dig, 8_hours );

    tasklist soldier_12h;
    soldier_12h.enschedule( task_dig, 12_hours );


    SECTION( "Light tasks" ) {
        INFO( "\nFirst Aid 8 hours:" );
        weariness_events info = do_activity( desk_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( info.empty() );
        CHECK( guy.weariness_level() == 0 );
    }

    SECTION( "Moderate tasks" ) {
        INFO( "\nPlanting 8 hours:" );
        weariness_events info = do_activity( moderate_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1 ) == Approx( 370 ).margin( 5 ) );
        CHECK( guy.weariness_level() == 1 );
    }

    SECTION( "Heavy tasks" ) {
        INFO( "\nDigging Pits 8 hours:" );
        weariness_events info = do_activity( soldier_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1 ) == Approx( 115 ).margin( 0 ) );
        CHECK( info.transition_minutes( 1, 2 ) == Approx( 255 ).margin( 0 ) );
        CHECK( info.transition_minutes( 2, 3 ) == Approx( 360 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4 ) == Approx( 465 ).margin( 5 ) );
        CHECK( guy.weariness_level() == 4 );

        INFO( "\nDigging Pits 12 hours:" );
        info = do_activity( soldier_12h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1 ) == Approx( 110 ).margin( 0 ) );
        CHECK( info.transition_minutes( 1, 2 ) == Approx( 250 ).margin( 0 ) );
        CHECK( info.transition_minutes( 2, 3 ) == Approx( 355 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4 ) == Approx( 460 ).margin( 5 ) );
        CHECK( info.transition_minutes( 4, 5 ) == Approx( 580 ).margin( 5 ) );
        CHECK( guy.weariness_level() == 5 );
    }
}

TEST_CASE( "weary_recovery", "[weary][activities]" )
{
    const avatar &guy = get_avatar();

    static const int moves_for_25h = to_seconds<int>( 25_hours ) * 100;
    // Set up our scenarios ahead of time
    const dig_activity_actor dig_actor( moves_for_25h, tripoint_zero, "t_pit", tripoint_zero, 0, "" );
    const schedule task_dig( dig_actor, 5_minutes );
    const schedule task_wait( activity_id( "ACT_WAIT" ), 5_minutes );

    tasklist soldier_8h;
    soldier_8h.enschedule( task_dig, 8_hours );
    soldier_8h.enschedule( task_wait, 8_hours );

    SECTION( "Heavy tasks" ) {
        INFO( "\nDigging Pits 8 hours, then waiting 8:" );
        weariness_events info = do_activity( soldier_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 4, 3 ) == Approx( 700 ).margin( 0 ) );
        CHECK( info.transition_minutes( 3, 2 ) == Approx( 820 ).margin( 0 ) );
        CHECK( guy.weariness_level() == 2 );
    }
}

TEST_CASE( "weary_24h_tasks", "[weary][activities]" )
{
    const avatar &guy = get_avatar();

    static const int moves_for_25h = to_seconds<int>( 25_hours ) * 100;
    // Set up our scenarios ahead of time
    const dig_activity_actor dig_actor( moves_for_25h, tripoint_zero, "t_pit", tripoint_zero, 0, "" );
    const schedule task_dig( dig_actor, 5_minutes );

    tasklist waiting_24h;
    waiting_24h.enschedule( { activity_id( "ACT_WAIT" ), 5_minutes }, 24_hours );

    tasklist digging_24h;
    digging_24h.enschedule( task_dig, 24_hours );

    SECTION( "Waiting 24 hours" ) {
        weariness_events info = do_activity( waiting_24h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( info.empty() );
        CHECK( guy.weariness_level() == 0 );
    }

    SECTION( "Digging 24 hours" ) {
        weariness_events info = do_activity( digging_24h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1 ) == Approx( 125 ).margin( 0 ) );
        CHECK( info.transition_minutes( 1, 2 ) == Approx( 260 ).margin( 0 ) );
        CHECK( info.transition_minutes( 2, 3 ) == Approx( 360 ).margin( 0 ) );
        CHECK( info.transition_minutes( 3, 4 ) == Approx( 460 ).margin( 5 ) );
        CHECK( info.transition_minutes( 4, 5 ) == Approx( 585 ).margin( 5 ) );
        CHECK( info.transition_minutes( 5, 6 ) == Approx( 725 ).margin( 5 ) );
        CHECK( info.transition_minutes( 6, 7 ) == Approx( 835 ).margin( 5 ) );
        CHECK( info.transition_minutes( 7, 8 ) == Approx( 905 ).margin( 5 ) );
        // TODO: You should collapse from this - currently we
        // just get really high levels of weariness
        CHECK( guy.weariness_level() > 8 );
    }
}
