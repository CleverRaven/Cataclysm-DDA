#include "activity_scheduling_helper.h"
#include "player_helpers.h"
#include "map_helpers.h"

#include "avatar.h"
#include "catch/catch.hpp"

// Set up our scenarios ahead of time
static const int moves_for_25h = to_seconds<int>( 25_hours ) * 100;
const dig_activity_actor dig_actor( moves_for_25h, tripoint_zero, "t_pit", tripoint_zero, 0, "" );
const activity_schedule task_dig( dig_actor, 5_minutes );
const activity_schedule task_wait( activity_id( "ACT_WAIT" ), 5_minutes );
const activity_schedule task_firstaid( activity_id( "ACT_FIRSTAID" ), 5_minutes );
const activity_schedule task_plant( activity_id( "ACT_PLANT_SEED" ), 5_minutes );

TEST_CASE( "weary_assorted_tasks", "[weary][activities]" )
{
    const avatar &guy = get_avatar();

    tasklist desk_8h;
    desk_8h.enschedule( task_firstaid, 8_hours );

    tasklist moderate_8h;
    moderate_8h.enschedule( task_plant, 8_hours );

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
        CHECK( info.transition_minutes( 0, 1, 370_minutes ) == Approx( 370 ).margin( 5 ) );
        CHECK( guy.weariness_level() == 1 );
    }

    SECTION( "Heavy tasks" ) {
        INFO( "\nDigging Pits 8 hours:" );
        weariness_events info = do_activity( soldier_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 115_minutes ) == Approx( 115 ).margin( 0 ) );
        CHECK( info.transition_minutes( 1, 2, 255_minutes ) == Approx( 255 ).margin( 0 ) );
        CHECK( info.transition_minutes( 2, 3, 360_minutes ) == Approx( 360 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4, 465_minutes ) == Approx( 465 ).margin( 5 ) );
        CHECK( guy.weariness_level() == 4 );

        INFO( "\nDigging Pits 12 hours:" );
        info = do_activity( soldier_12h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 110_minutes ) == Approx( 110 ).margin( 0 ) );
        CHECK( info.transition_minutes( 1, 2, 250_minutes ) == Approx( 250 ).margin( 0 ) );
        CHECK( info.transition_minutes( 2, 3, 355_minutes ) == Approx( 355 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4, 460_minutes ) == Approx( 460 ).margin( 5 ) );
        CHECK( info.transition_minutes( 4, 5, 580_minutes ) == Approx( 580 ).margin( 5 ) );
        CHECK( guy.weariness_level() == 5 );
    }
}

TEST_CASE( "weary_recovery", "[weary][activities]" )
{
    const avatar &guy = get_avatar();

    tasklist soldier_8h;
    soldier_8h.enschedule( task_dig, 8_hours );
    soldier_8h.enschedule( task_wait, 8_hours );

    SECTION( "Heavy tasks" ) {
        INFO( "\nDigging Pits 8 hours, then waiting 8:" );
        weariness_events info = do_activity( soldier_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 4, 3, 700_minutes ) == Approx( 700 ).margin( 0 ) );
        CHECK( info.transition_minutes( 3, 2, 820_minutes ) == Approx( 820 ).margin( 0 ) );
        CHECK( guy.weariness_level() == 2 );
    }
}

TEST_CASE( "weary_24h_tasks", "[weary][activities]" )
{
    const avatar &guy = get_avatar();

    tasklist waiting_24h;
    waiting_24h.enschedule( task_wait, 24_hours );

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
        CHECK( info.transition_minutes( 0, 1, 125_minutes ) == Approx( 125 ).margin( 0 ) );
        CHECK( info.transition_minutes( 1, 2, 260_minutes ) == Approx( 260 ).margin( 0 ) );
        CHECK( info.transition_minutes( 2, 3, 360_minutes ) == Approx( 360 ).margin( 0 ) );
        CHECK( info.transition_minutes( 3, 4, 460_minutes ) == Approx( 460 ).margin( 5 ) );
        CHECK( info.transition_minutes( 4, 5, 585_minutes ) == Approx( 585 ).margin( 5 ) );
        CHECK( info.transition_minutes( 5, 6, 725_minutes ) == Approx( 725 ).margin( 5 ) );
        CHECK( info.transition_minutes( 6, 7, 836_minutes ) == Approx( 835 ).margin( 5 ) );
        CHECK( info.transition_minutes( 7, 8, 905_minutes ) == Approx( 905 ).margin( 5 ) );
        // TODO: You should collapse from this - currently we
        // just get really high levels of weariness
        CHECK( guy.weariness_level() > 8 );
    }
}
