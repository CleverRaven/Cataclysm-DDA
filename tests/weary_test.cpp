#include "activity_scheduling_helper.h"
#include "player_helpers.h"
#include "map_helpers.h"

#include "activity_actor_definitions.h"
#include "avatar.h"
#include "catch/catch.hpp"

// Set up our scenarios ahead of time
static const int moves_for_25h = to_seconds<int>( 25_hours ) * 100;
static const dig_activity_actor dig_actor( moves_for_25h, tripoint_zero, "t_pit", tripoint_zero, 0,
        "" );
static const activity_schedule task_dig( dig_actor, 5_minutes );
static const activity_schedule task_wait( activity_id( "ACT_WAIT" ), 5_minutes );
static const activity_schedule task_firstaid( activity_id( "ACT_FIRSTAID" ), 5_minutes );
static const activity_schedule task_plant( activity_id( "ACT_PLANT_SEED" ), 5_minutes );
static const activity_schedule task_weld( activity_id( "ACT_VEHICLE" ), 5_minutes );
static const activity_schedule task_read( activity_id( "ACT_READ" ), 5_minutes );

static const meal_schedule sausage( itype_id( "sausage" ) );
static const meal_schedule milk( itype_id( "milk" ) );

static const sleep_schedule sched_sleep{};

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
        CHECK( info.transition_minutes( 0, 1, 365_minutes ) == Approx( 365 ).margin( 5 ) );
        CHECK( guy.weariness_level() == 1 );
    }

    SECTION( "Heavy tasks" ) {
        INFO( "\nDigging Pits 8 hours:" );
        weariness_events info = do_activity( soldier_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 120_minutes ) == Approx( 120 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 2, 250_minutes ) == Approx( 250 ).margin( 5 ) );
        CHECK( info.transition_minutes( 2, 3, 360_minutes ) == Approx( 360 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4, 465_minutes ) == Approx( 465 ).margin( 5 ) );
        CHECK( guy.weariness_level() == 4 );

        INFO( "\nDigging Pits 12 hours:" );
        info = do_activity( soldier_12h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 125_minutes ) == Approx( 125 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 2, 250_minutes ) == Approx( 250 ).margin( 5 ) );
        CHECK( info.transition_minutes( 2, 3, 355_minutes ) == Approx( 355 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4, 460_minutes ) == Approx( 460 ).margin( 5 ) );
        CHECK( info.transition_minutes( 4, 5, 590_minutes ) == Approx( 590 ).margin( 5 ) );
        CHECK( guy.weariness_level() == 6 );
    }
}

TEST_CASE( "weary_recovery", "[weary][activities]" )
{
    const avatar &guy = get_avatar();

    tasklist soldier_8h;
    soldier_8h.enschedule( task_dig, 8_hours );
    soldier_8h.enschedule( task_wait, 8_hours );

    tasklist mechanic_day;
    // Clear the guts, we're providing our own food
    mechanic_day.enschedule( sched_clear_guts, 0_turns );
    // Have a nice meal, get to work, eat more, read some, sleep
    mechanic_day.enschedule( sausage, 0_turns );
    mechanic_day.enschedule( sausage, 0_turns );
    mechanic_day.enschedule( milk, 0_turns );
    mechanic_day.enschedule( milk, 0_turns );
    mechanic_day.enschedule( task_weld, 5_hours );
    mechanic_day.enschedule( sausage, 0_turns );
    mechanic_day.enschedule( sausage, 0_turns );
    mechanic_day.enschedule( milk, 0_turns );
    mechanic_day.enschedule( milk, 0_turns );
    mechanic_day.enschedule( task_weld, 5_hours );
    mechanic_day.enschedule( sausage, 0_turns );
    mechanic_day.enschedule( sausage, 0_turns );
    mechanic_day.enschedule( milk, 0_turns );
    mechanic_day.enschedule( milk, 0_turns );
    mechanic_day.enschedule( task_read, 4_hours );
    mechanic_day.enschedule( sched_sleep, 10_hours );
    mechanic_day.enschedule( task_wait, 16_hours );

    SECTION( "Heavy tasks" ) {
        INFO( "\nDigging Pits 8 hours, then waiting 8:" );
        weariness_events info = do_activity( soldier_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 4, 3, 550_minutes ) == Approx( 550 ).margin( 0 ) );
        CHECK( info.transition_minutes( 3, 2, 670_minutes ) == Approx( 670 ).margin( 0 ) );
        CHECK( guy.weariness_level() == 1 );
    }

    SECTION( "1 day vehicle work" ) {
        INFO( "\n3 meals, 10h vehicle work, 4h reading, 10h sleep, 16h waiting" );
        weariness_events info = do_activity( mechanic_day );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 325_minutes ) == Approx( 325 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 2, 625_minutes ) == Approx( 625 ).margin( 5 ) );
        CHECK( info.transition_minutes( 2, 1, 740_minutes ) == Approx( 740 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 0, 990_minutes ) == Approx( 990 ).margin( 5 ) );
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
        CHECK( info.transition_minutes( 0, 1, 130_minutes ) == Approx( 130 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 2, 255_minutes ) == Approx( 255 ).margin( 5 ) );
        CHECK( info.transition_minutes( 2, 3, 360_minutes ) == Approx( 360 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4, 465_minutes ) == Approx( 465 ).margin( 5 ) );
        CHECK( info.transition_minutes( 4, 5, 585_minutes ) == Approx( 590 ).margin( 5 ) );
        CHECK( info.transition_minutes( 5, 6, 725_minutes ) == Approx( 725 ).margin( 10 ) );
        CHECK( info.transition_minutes( 6, 7, 810_minutes ) == Approx( 810 ).margin( 10 ) );
        CHECK( info.transition_minutes( 7, 8, 890_minutes ) == Approx( 890 ).margin( 10 ) );
        // TODO: You should collapse from this - currently we
        // just get really high levels of weariness
        CHECK( guy.weariness_level() > 8 );
    }
}
