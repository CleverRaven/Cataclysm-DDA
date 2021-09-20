#include "activity_actor_definitions.h"
#include "activity_scheduling_helper.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

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
        clear_avatar();
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( desk_8h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( info.empty() );
        CHECK( guy.weariness_level() == 0 );
    }

    SECTION( "Moderate tasks" ) {
        INFO( "\nPlanting 8 hours:" );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( moderate_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 370_minutes ) == Approx( 370 ).margin( 5 ) );
        CHECK( !info.have_weary_decrease() );
        CHECK( guy.weariness_level() == 1 );
    }

    SECTION( "Heavy tasks - Digging Pits 8 hours" ) {
        clear_avatar();
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( soldier_8h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 120_minutes ) == Approx( 120 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 2, 250_minutes ) == Approx( 250 ).margin( 5 ) );
        CHECK( info.transition_minutes( 2, 3, 360_minutes ) == Approx( 360 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4, 465_minutes ) == Approx( 465 ).margin( 5 ) );
        // CHECK( !info.have_weary_decrease() );
        CHECK( guy.weariness_level() == 4 );
    }

    SECTION( "Heavy tasks - Digging Pits 12 hours" ) {
        clear_avatar();
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( soldier_12h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 120_minutes ) == Approx( 120 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 2, 250_minutes ) == Approx( 250 ).margin( 5 ) );
        CHECK( info.transition_minutes( 2, 3, 360_minutes ) == Approx( 360 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4, 465_minutes ) == Approx( 465 ).margin( 5 ) );
        CHECK( info.transition_minutes( 4, 5, 600_minutes ) == Approx( 600 ).margin( 5 ) );
        // CHECK( !info.have_weary_decrease() );
        CHECK( guy.weariness_level() == 5 );
    }
}

static void check_weary_mutations_nosleep( const std::string &trait_name, float fatigue_mod )
{
    tasklist soldier_8h;
    avatar &guy = get_avatar();

    float multiplier = 1.0f + fatigue_mod;

    std::stringstream section_name;
    section_name << "Non-sleep effects of " << trait_name;
    section_name << " (fatigue_mod: " << fatigue_mod << ")";

    SECTION( section_name.str() ) {
        clear_avatar();
        set_single_trait( guy, trait_name );
        // How do we make sure they don't sleep? Set fatigue to -1000?

        soldier_8h.enschedule( task_dig, 8_hours );
        soldier_8h.enschedule( task_wait, 8_hours );

        INFO( "\nDigging Pits 8 hours, then waiting 8:" );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( soldier_8h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        if( multiplier >= 1.0f ) { // Fatigue alterations from mutations themselves affect thresholds...
            CHECK( info.transition_minutes( 0, 1, 120_minutes ) <= 125 );
            CHECK( info.transition_minutes( 0, 1, 120_minutes ) >= ( 115.0f / multiplier ) );
            CHECK( info.transition_minutes( 1, 2, 250_minutes ) <= 255 );
            CHECK( info.transition_minutes( 1, 2, 250_minutes ) >= ( 245.0f / multiplier ) );
            CHECK( info.transition_minutes( 2, 3, 360_minutes ) <= 365 );
            CHECK( info.transition_minutes( 2, 3, 360_minutes ) >= ( 355.0f / multiplier ) );
            CHECK( info.transition_minutes( 3, 4, 465_minutes ) <= 470 );
            CHECK( info.transition_minutes( 3, 4, 465_minutes ) >= ( 460.0f / multiplier ) );
        } else {
            CHECK( info.transition_minutes( 0, 1, 120_minutes ) >= 115 );
            CHECK( info.transition_minutes( 0, 1, 120_minutes ) <= ( 125.0f / multiplier ) );
            CHECK( info.transition_minutes( 1, 2, 250_minutes ) >= 245 );
            CHECK( info.transition_minutes( 1, 2, 250_minutes ) <= ( 255.0f / multiplier ) );
            CHECK( info.transition_minutes( 2, 3, 360_minutes ) >= 355 );
            CHECK( info.transition_minutes( 2, 3, 360_minutes ) <= ( 365.0f / multiplier ) );
            CHECK( info.transition_minutes( 3, 4, 465_minutes ) >= 460 );
            if( ( 470.0f / multiplier ) < ( 8 * 60 ) ) {
                CHECK( info.transition_minutes( 3, 4, 465_minutes ) <= ( 470.0f / multiplier ) );
            }
        }
        time_duration time1 = ( ( 505_minutes - 8_hours ) * multiplier ) + 8_hours;
        time_duration time2 = ( ( 630_minutes - 8_hours ) * multiplier ) + 8_hours;
        // Increased below margin for floats to 9.5 (from 5) to account for roundoff vs 5-minute weariness cycle
        if( time1 < 16_hours ) {
            if( multiplier >= 1.0f ) {
                CHECK( info.transition_minutes( 4, 3,
                                                time1 ) >= to_minutes<float>( time1 ) - 9.5f );
            } else {
                CHECK( info.transition_minutes( 4, 3,
                                                time1 ) <= to_minutes<float>( time1 ) + 9.5f );
            }
            if( time2 < 16_hours ) {
                if( multiplier >= 1.0f ) {
                    CHECK( info.transition_minutes( 3, 2,
                                                    time2 ) >= to_minutes<float>( time2 ) - 9.5f );
                } else {
                    CHECK( info.transition_minutes( 3, 2,
                                                    time2 ) <= to_minutes<float>( time2 ) + 9.5f );
                }
            } else {
                CHECK( info.transition_minutes( 3, 2, time2 ) >= ( 16 * 60 ) - 5 );
            }
        } else {
            CHECK( info.transition_minutes( 4, 3, time1 ) >= ( 16 * 60 ) - 5 );
            CHECK( info.transition_minutes( 3, 2, time2 ) >= ( 16 * 60 ) );
        }

        if( multiplier >= 1.0f ) { // instability currently prevents use of first one... sigh.
            // CHECK( info.transition_minutes( 1, 0, 0_minutes ) > ( 8 * 60 ) ); // should be INT_MAX
            CHECK( info.transition_minutes( 2, 1, 0_minutes ) > ( 8 * 60 ) );
        }
    }
}

TEST_CASE( "weary_recovery_mutations", "[weary][activities][mutations]" )
{
    // WAKEFUL: fatigue_mod -0.15
    // SLEEPY: fatigue_mod 0.33, fatigue_regen_mod 0.33
    // WAKEFUL2: fatigue_mod -0.25
    // WAKEFUL3: fatigue_mod -0.5, fatigue_regen_mod 0.5
    // HUGE: fatigue_mod 0.15 (HUGE_OK - does it remove this?)
    // PERSISTENCE_HUNTER: fatigue_mod -0.1
    // PERSISTENCE_HUNGER2: fatigue_mod -0.2
    // MET_RAT: fatigue_mod 0.5, fatigue_regen_mod 0.33
    // SLEEPY2: fatigue_mod 1.0 (does this include SLEEPY's fatigue_regen_mod?)

    check_weary_mutations_nosleep( "WAKEFUL", -0.15f );
    check_weary_mutations_nosleep( "SLEEPY", 0.33f );
    check_weary_mutations_nosleep( "WAKEFUL2", -0.25f );
    check_weary_mutations_nosleep( "WAKEFUL3", -0.5f );
    check_weary_mutations_nosleep( "HUGE", 0.15f );
    check_weary_mutations_nosleep( "PERSISTENCE_HUNTER", -0.1f );
    check_weary_mutations_nosleep( "PERSISTENCE_HUNTER2", -0.2f );
    check_weary_mutations_nosleep( "MET_RAT", 0.5f );
    check_weary_mutations_nosleep( "SLEEPY2", 1.0f );
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
        clear_avatar();
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( soldier_8h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 4, 3, 505_minutes ) == Approx( 505 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 2, 630_minutes ) == Approx( 630 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 0, 0_minutes ) > ( 8 * 60 ) ); // should be INT_MAX
        CHECK( info.transition_minutes( 2, 1, 0_minutes ) > ( 8 * 60 ) );
        CHECK( info.transition_minutes( 1, 2, 16_hours ) <= ( 8 * 60 ) );
        CHECK( info.transition_minutes( 2, 3, 16_hours ) <= ( 8 * 60 ) );
        CHECK( info.transition_minutes( 3, 4, 16_hours ) <= ( 8 * 60 ) );
        CHECK( guy.weariness_level() == 1 );
    }

    SECTION( "1 day vehicle work" ) {
        INFO( "\n3 meals, 10h vehicle work, 4h reading, 10h sleep, 16h waiting" );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( mechanic_day );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 325_minutes ) == Approx( 325 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 2, 625_minutes ) == Approx( 625 ).margin( 5 ) );
        CHECK( info.transition_minutes( 2, 1, 735_minutes ) == Approx( 735 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 0, 985_minutes ) == Approx( 985 ).margin( 5 ) );
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
        clear_avatar();
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( waiting_24h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( info.empty() );
        CHECK( guy.weariness_level() == 0 );
    }

    SECTION( "Digging 24 hours" ) {
        clear_avatar();
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( digging_24h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 120_minutes ) == Approx( 120 ).margin( 5 ) );
        CHECK( info.transition_minutes( 1, 2, 250_minutes ) == Approx( 250 ).margin( 5 ) );
        CHECK( info.transition_minutes( 2, 3, 360_minutes ) == Approx( 360 ).margin( 5 ) );
        CHECK( info.transition_minutes( 3, 4, 465_minutes ) == Approx( 465 ).margin( 5 ) );
        CHECK( info.transition_minutes( 4, 5, 600_minutes ) == Approx( 600 ).margin( 5 ) );
        CHECK( info.transition_minutes( 5, 6, 740_minutes ) == Approx( 740 ).margin( 5 ) );
        CHECK( info.transition_minutes( 6, 7, 845_minutes ) == Approx( 845 ).margin( 5 ) );
        CHECK( info.transition_minutes( 7, 8, 925_minutes ) == Approx( 925 ).margin( 10 ) );
        CHECK( !info.have_weary_decrease() );
        // TODO: You should collapse from this - currently we
        // just get really high levels of weariness
        CHECK( guy.weariness_level() > 8 );
    }
}
