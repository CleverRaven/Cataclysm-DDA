#include "activity_actor_definitions.h"
#include "activity_scheduling_helper.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"

static const activity_id ACT_FIRSTAID( "ACT_FIRSTAID" );
static const activity_id ACT_PLANT_SEED( "ACT_PLANT_SEED" );
static const activity_id ACT_READ( "ACT_READ" );
static const activity_id ACT_VEHICLE( "ACT_VEHICLE" );
static const activity_id ACT_WAIT( "ACT_WAIT" );

static const itype_id itype_milk( "milk" );
static const itype_id itype_sausage( "sausage" );

// Set up our scenarios ahead of time
static const int moves_for_25h = to_seconds<int>( 25_hours ) * 100;
static const clear_rubble_activity_actor dig_actor( moves_for_25h );
static const activity_schedule task_dig( dig_actor, 5_minutes );
static const activity_schedule task_wait( ACT_WAIT, 5_minutes );
static const activity_schedule task_firstaid( ACT_FIRSTAID, 5_minutes );
static const activity_schedule task_plant( ACT_PLANT_SEED, 5_minutes );
static const activity_schedule task_weld( ACT_VEHICLE, 5_minutes );
static const activity_schedule task_read( ACT_READ, 5_minutes );

static const meal_schedule sausage( itype_sausage );
static const meal_schedule milk( itype_milk );

static const sleep_schedule sched_sleep{};

TEST_CASE( "weary_assorted_tasks", "[weary][activities]" )
{
    avatar &guy = get_avatar();

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
        guy.activity_history.set_intake( ( guy.base_bmr() * 1000 * 19 ) / 24 );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( desk_8h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( info.empty() );
        CHECK( guy.weariness_level() == 0 );
    }

    SECTION( "Moderate tasks" ) {
        INFO( "\nPlanting 8 hours:" );
        guy.activity_history.set_intake( ( guy.base_bmr() * 1000 * 19 ) / 24 );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( moderate_8h );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 370_minutes ) == Approx( 370 ).margin( 10 ) );
        CHECK( !info.have_weary_decrease() );
        CHECK( guy.weariness_level() == 1 );
    }

    SECTION( "Heavy tasks - Digging Pits 8 hours" ) {
        clear_avatar();
        guy.activity_history.set_intake( ( guy.base_bmr() * 1000 * 19 ) / 24 );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( soldier_8h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 165_minutes ) == Approx( 165 ).margin( 10 ) );
        CHECK( info.transition_minutes( 1, 2, 295_minutes ) == Approx( 295 ).margin( 10 ) );
        CHECK( info.transition_minutes( 2, 3, 390_minutes ) == Approx( 390 ).margin( 10 ) );
        CHECK( !info.have_weary_decrease() );
        CHECK( guy.weariness_level() == 3 );
    }

    SECTION( "Heavy tasks - Digging Pits 12 hours" ) {
        clear_avatar();
        guy.activity_history.set_intake( ( guy.base_bmr() * 1000 * 19 ) / 24 );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( soldier_12h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 165_minutes ) == Approx( 165 ).margin( 10 ) );
        CHECK( info.transition_minutes( 1, 2, 295_minutes ) == Approx( 295 ).margin( 10 ) );
        CHECK( info.transition_minutes( 2, 3, 390_minutes ) == Approx( 390 ).margin( 10 ) );
        CHECK( info.transition_minutes( 3, 4, 485_minutes ) == Approx( 485 ).margin( 10 ) );
        CHECK( info.transition_minutes( 4, 5, 600_minutes ) == Approx( 600 ).margin( 10 ) );
        CHECK( info.transition_minutes( 5, 6, 710_minutes ) == Approx( 710 ).margin( 10 ) );
        CHECK( !info.have_weary_decrease() );
        CHECK( guy.weariness_level() == 6 );
    }
}

static void check_weary_mutation_nosleep( const std::string &trait_name, float sleepiness_mod )
{
    tasklist soldier_8h;
    avatar &guy = get_avatar();

    float multiplier = 1.0f + sleepiness_mod;

    std::stringstream section_name;
    section_name << "Non-sleep effects of " << trait_name;
    section_name << " (sleepiness_mod: " << sleepiness_mod << ")";

    SECTION( section_name.str() ) {
        clear_avatar();
        set_single_trait( guy, trait_name );
        guy.set_stored_kcal( guy.get_healthy_kcal() );
        // How do we make sure they don't sleep? Set sleepiness to -1000?
        // Doesn't seem to be a problem, fortunately.

        soldier_8h.enschedule( task_dig, 8_hours );
        soldier_8h.enschedule( task_wait, 8_hours );

        INFO( "\nDigging Pits 8 hours, then waiting 8:" );
        guy.activity_history.set_intake( ( guy.base_bmr() * 1000 * 19 ) / 24 );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( soldier_8h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        if( multiplier >= 1.0f ) { // sleepiness alterations from mutations themselves affect thresholds...
            CHECK( info.transition_minutes( 0, 1, 165_minutes ) <= 170 );
            CHECK( info.transition_minutes( 0, 1, 165_minutes ) >= ( 160.0f / multiplier ) );
            CHECK( info.transition_minutes( 1, 2, 295_minutes ) <= 300 );
            CHECK( info.transition_minutes( 1, 2, 295_minutes ) >= ( 290.0f / multiplier ) );
            CHECK( info.transition_minutes( 2, 3, 390_minutes ) <= 395 );
            CHECK( info.transition_minutes( 2, 3, 390_minutes ) >= ( 385.0f / multiplier ) );
            CHECK( info.transition_minutes( 3, 4, 485_minutes ) >= ( 480.0f / multiplier ) );
        } else {
            CHECK( info.transition_minutes( 0, 1, 165_minutes ) >= 160 );
            CHECK( info.transition_minutes( 0, 1, 165_minutes ) <= ( 170.0f / multiplier ) );
            CHECK( info.transition_minutes( 1, 2, 295_minutes ) >= 290 );
            CHECK( info.transition_minutes( 1, 2, 295_minutes ) <= ( 300.0f / multiplier ) );
            CHECK( info.transition_minutes( 2, 3, 390_minutes ) >= 385 );
            CHECK( info.transition_minutes( 2, 3, 390_minutes ) <= ( 395.0f / multiplier ) );
            // CHECK( info.transition_minutes( 3, 4, 485_minutes ) >= 480 );
            if( ( 490.0f / multiplier ) < ( 8 * 60 ) ) {
                CHECK( info.transition_minutes( 3, 4, 485_minutes ) <= ( 490.0f / multiplier ) );
            }
        }
        time_duration time1 = ( ( 500_minutes - 8_hours ) * multiplier ) + 8_hours;
        time_duration time2 = ( ( 620_minutes - 8_hours ) * multiplier ) + 8_hours;
        // Increased below margin for floats to 9.5 (from 5) to account for roundoff vs 5-minute weariness cycle
        if( time1 < 16_hours ) {
            if( multiplier >= 1.0f ) {
                CHECK( info.transition_minutes( 4, 3,
                                                time1 ) >= to_minutes<float>( time1 ) - 9.5f );
            } else if( ( 490.0f / multiplier ) < ( 8 * 60 ) ) {
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

        if( multiplier >= 1.0f ) {
            CHECK( info.transition_minutes( 1, 0, 0_minutes ) > ( 16 * 60 ) ); // should be INT_MAX
        }
        CHECK( info.transition_minutes( 2, 1, 0_minutes ) > to_minutes<float>( time2 ) - 9.5f );
    }
}

static void check_weary_mutation_sleep( const std::string &trait_name, float sleepiness_mod,
                                        float sleepiness_regen_mod )
{
    tasklist soldier_8h;
    avatar &guy = get_avatar();

    float multiplier = 1.0f + sleepiness_mod;
    float multiplier2 = multiplier / ( 2.0f + sleepiness_regen_mod );

    std::stringstream section_name;
    section_name << "Sleep effects of " << trait_name;
    section_name << " (sleepiness_mod: " << sleepiness_mod;
    section_name << "; sleepiness_regen_mod: " << sleepiness_regen_mod << ")";

    SECTION( section_name.str() ) {
        clear_avatar();
        set_single_trait( guy, trait_name );
        guy.set_stored_kcal( guy.get_healthy_kcal() );

        soldier_8h.enschedule( task_dig, 8_hours );
        soldier_8h.enschedule( sched_sleep, 8_hours );

        INFO( "\nDigging Pits 8 hours, then sleeping 8:" );
        guy.activity_history.set_intake( ( guy.base_bmr() * 1000 * 19 ) / 24 );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( soldier_8h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        // Appears to take about 5 minutes to sleep, from messages.
        time_duration time1 = ( ( 495_minutes - 8_hours ) * multiplier2 ) + ( 5_minutes * multiplier ) +
                              8_hours;
        time_duration time2 = ( ( 615_minutes - 8_hours ) * multiplier2 ) + ( 5_minutes * multiplier ) +
                              8_hours;
        // Increased below margin for floats to 13 due to sleep uncertainty re 5-minute weary timer.
        if( time1 < 16_hours ) {
            if( multiplier2 >= 1.0f ) {
                CHECK( info.transition_minutes( 4, 3,
                                                time1 ) >= to_minutes<float>( time1 ) - 13.0f );
            } else if( ( 490.0f / multiplier ) < ( 8 * 60 ) ) {
                CHECK( info.transition_minutes( 4, 3,
                                                time1 ) <= to_minutes<float>( time1 ) + 13.0f );
            }
            if( time2 < 16_hours ) {
                if( multiplier2 >= 1.0f ) {
                    CHECK( info.transition_minutes( 3, 2,
                                                    time2 ) >= to_minutes<float>( time2 ) - 13.0f );
                } else {
                    CHECK( info.transition_minutes( 3, 2,
                                                    time2 ) <= to_minutes<float>( time2 ) + 13.0f );
                }
            } else {
                CHECK( info.transition_minutes( 3, 2, time2 ) >= ( 16 * 60 ) - 5 );
            }
        } else {
            CHECK( info.transition_minutes( 4, 3, time1 ) >= ( 16 * 60 ) - 5 );
            CHECK( info.transition_minutes( 3, 2, time2 ) >= ( 16 * 60 ) );
        }

        if( multiplier2 >= 1.0f ) {
            CHECK( info.transition_minutes( 1, 0, 0_minutes ) > ( 16 * 60 ) ); // should be INT_MAX
        }
        CHECK( info.transition_minutes( 2, 1, 0_minutes ) > to_minutes<float>( time2 ) - 13.0f );
    }
}

static void check_weary_mutation( const std::string &trait_name, float sleepiness_mod,
                                  float sleepiness_regen_mod )
{
    check_weary_mutation_nosleep( trait_name, sleepiness_mod );
    check_weary_mutation_sleep( trait_name, sleepiness_mod, sleepiness_regen_mod );
}

TEST_CASE( "weary_recovery_mutations", "[weary][activities][mutations]" )
{
    // WAKEFUL: sleepiness_mod -0.15
    // SLEEPY: sleepiness_mod 0.33, sleepiness_regen_mod 0.33
    // WAKEFUL2: sleepiness_mod -0.25
    // WAKEFUL3: sleepiness_mod -0.5, sleepiness_regen_mod 0.5
    // HUGE: sleepiness_mod 0.15 (HUGE_OK - does it remove this? Should it?)
    // PERSISTENCE_HUNTER: sleepiness_mod -0.1
    // PERSISTENCE_HUNGER2: sleepiness_mod -0.2
    // MET_RAT: sleepiness_mod 0.5, sleepiness_regen_mod 0.33
    // SLEEPY2: sleepiness_mod 1.0 (does this include SLEEPY's sleepiness_regen_mod? looks like it?)

    check_weary_mutation( "WAKEFUL", -0.15f, 0.0f );
    check_weary_mutation( "SLEEPY", 0.33f, 0.33f );
    check_weary_mutation( "WAKEFUL2", -0.25f, 0.0f );
    check_weary_mutation( "WAKEFUL3", -0.5f, 0.5f );
    check_weary_mutation( "HUGE", 0.15f, 0.0f );
    check_weary_mutation( "PERSISTENCE_HUNTER", -0.1f, 0.0f );
    check_weary_mutation( "PERSISTENCE_HUNTER2", -0.2f, 0.0f );
    check_weary_mutation( "MET_RAT", 0.5f, 0.33f );
    check_weary_mutation( "SLEEPY2", 1.0f, 0.33f );
}

TEST_CASE( "weary_recovery", "[weary][activities]" )
{
    calendar::turn = calendar::start_of_game;
    clear_avatar();
    avatar &guy = get_avatar();

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
        guy.activity_history.set_intake( ( guy.base_bmr() * 1000 * 19 ) / 24 );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( soldier_8h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 4, 3, 500_minutes ) == Approx( 500 ).margin( 10 ) );
        CHECK( info.transition_minutes( 3, 2, 620_minutes ) == Approx( 620 ).margin( 10 ) );
        CHECK( info.transition_minutes( 1, 0, 0_minutes ) > ( 8 * 60 ) ); // should be INT_MAX
        CHECK( info.transition_minutes( 2, 1, 0_minutes ) > ( 8 * 60 ) );
        CHECK( info.transition_minutes( 1, 2, 16_hours ) <= ( 8 * 60 ) );
        CHECK( info.transition_minutes( 2, 3, 16_hours ) <= ( 8 * 60 ) );
        // CHECK( info.transition_minutes( 3, 4, 16_hours ) <= ( 8 * 60 ) );
        CHECK( guy.weariness_level() == 1 );
    }

    SECTION( "1 day vehicle work" ) {
        INFO( "\n3 meals, 10h vehicle work, 4h reading, 10h sleep, 16h waiting" );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( mechanic_day );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 325_minutes ) == Approx( 325 ).margin( 10 ) );
        CHECK( info.transition_minutes( 1, 2, 625_minutes ) == Approx( 625 ).margin( 10 ) );
        CHECK( info.transition_minutes( 2, 1, 735_minutes ) == Approx( 735 ).margin( 10 ) );
        CHECK( info.transition_minutes( 1, 0, 985_minutes ) == Approx( 985 ).margin( 10 ) );
    }
}

TEST_CASE( "weary_24h_tasks", "[weary][activities]" )
{
    avatar &guy = get_avatar();

    tasklist waiting_24h;
    waiting_24h.enschedule( task_wait, 24_hours );

    tasklist digging_24h;
    digging_24h.enschedule( task_dig, 24_hours );

    SECTION( "Waiting 24 hours" ) {
        clear_avatar();
        guy.activity_history.set_intake( ( guy.base_bmr() * 1000 * 19 ) / 24 );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( waiting_24h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( info.empty() );
        CHECK( guy.weariness_level() == 0 );
    }

    SECTION( "Digging 24 hours" ) {
        clear_avatar();
        guy.activity_history.set_intake( ( guy.base_bmr() * 1000 * 19 ) / 24 );
        INFO( guy.debug_weary_info() );
        weariness_events info = do_activity( digging_24h, false );
        INFO( info.summarize() );
        INFO( guy.debug_weary_info() );
        REQUIRE( !info.empty() );
        CHECK( info.transition_minutes( 0, 1, 165_minutes ) == Approx( 165 ).margin( 10 ) );
        CHECK( info.transition_minutes( 1, 2, 295_minutes ) == Approx( 295 ).margin( 10 ) );
        CHECK( info.transition_minutes( 2, 3, 390_minutes ) == Approx( 390 ).margin( 10 ) );
        CHECK( info.transition_minutes( 3, 4, 485_minutes ) == Approx( 485 ).margin( 10 ) );
        CHECK( info.transition_minutes( 4, 5, 600_minutes ) == Approx( 600 ).margin( 10 ) );
        CHECK( info.transition_minutes( 5, 6, 715_minutes ) == Approx( 715 ).margin( 10 ) );
        CHECK( info.transition_minutes( 6, 7, 800_minutes ) == Approx( 800 ).margin( 10 ) );
        CHECK( info.transition_minutes( 7, 8, 870_minutes ) == Approx( 870 ).margin( 10 ) );
        CHECK( !info.have_weary_decrease() );
        // TODO: You should collapse from this - currently we
        // just get really high levels of weariness
        CHECK( guy.weariness_level() > 8 );
    }
}
