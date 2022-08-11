#include "catch/catch.hpp"

#include "game.h"
#include "scenario.h"
#include "options.h"
#include "options_helpers.h"

static const string_id<scenario> scenario_ambushed( "ambushed" );
static const string_id<scenario> scenario_test_random_day( "test_random_day" );
static const string_id<scenario> scenario_test_random_hour( "test_random_hour" );
static const string_id<scenario> scenario_test_random_year( "test_random_year" );

TEST_CASE( "Test start dates" )
{
    int default_initial_day = 60;
    int default_initial_time = 8;  // Note that the default for scenario time is same
    int default_spawn_delay = 0;
    int default_season_length = 91;

    SECTION( "Default start date with no scenario" ) {
        set_scenario( scenario::generic() );
        REQUIRE( get_option<int>( "INITIAL_DAY" ) == default_initial_day );
        REQUIRE( get_option<int>( "INITIAL_TIME" ) == default_initial_time );
        REQUIRE( get_option<int>( "SPAWN_DELAY" ) == default_spawn_delay );

        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero + 1_days * default_initial_day );
        CHECK( calendar::start_of_game == calendar::turn_zero + 1_days * default_initial_day + 1_hours *
               default_initial_time );
    }

    SECTION( "Customized start date" ) {
        override_option initial_day( "INITIAL_DAY", "100" );
        override_option initial_time( "INITIAL_TIME", "13" );
        override_option spawn_delay( "SPAWN_DELAY", "10" );

        set_scenario( scenario::generic() );
        REQUIRE( get_option<int>( "INITIAL_DAY" ) == 100 );
        REQUIRE( get_option<int>( "INITIAL_TIME" ) == 13 );
        REQUIRE( get_option<int>( "SPAWN_DELAY" ) == 10 );

        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero + 1_days * 100 );
        CHECK( calendar::start_of_game == calendar::turn_zero + 1_days * ( 100 + 10 ) + 1_hours * 13 );
    }

    SECTION( "Scenario with start date" ) {
        set_scenario( &scenario_ambushed.obj() );
        // Ambushed scenario starts on winter day 1 (day 273 with default season length)

        // Initial time and spawn delay should not affect scenario dates
        override_option initial_time( "INITIAL_TIME", "13" );
        override_option spawn_delay( "SPAWN_DELAY", "10" );

        REQUIRE( get_option<int>( "INITIAL_DAY" ) == default_initial_day );
        REQUIRE( get_option<int>( "SEASON_LENGTH" ) == default_season_length );

        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero + 1_days * default_initial_day );
        CHECK( calendar::start_of_game == calendar::turn_zero + 1_days * 273 + 1_hours *
               default_initial_time );
    }

    SECTION( "Scenario with start date tries to start before cataclysm" ) {
        set_scenario( &scenario_ambushed.obj() );
        // Ambushed scenario starts on winter day 1 (day 273 with default season length)

        override_option initial_day( "INITIAL_DAY", "350" );

        REQUIRE( get_option<int>( "INITIAL_DAY" ) == 350 );
        REQUIRE( get_option<int>( "INITIAL_TIME" ) == default_initial_time );
        REQUIRE( get_option<int>( "SPAWN_DELAY" ) == default_spawn_delay );
        REQUIRE( get_option<int>( "SEASON_LENGTH" ) == default_season_length );

        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero + 1_days * 350 );

        // Start should be moved forwards by one year from normal to avoid too early start
        CHECK( calendar::start_of_game == calendar::turn_zero + 1_days * 273 + 1_hours *
               default_initial_time + calendar::year_length() );
    }

    // Reset dates so other tests won't fail
    calendar::start_of_game = calendar::turn_zero;
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::turn = calendar::turn_zero;
}

TEST_CASE( "Random dates" )
{
    int default_initial_time = 8;

    SECTION( "Random start date" ) {
        override_option initial_day( "INITIAL_DAY", "-1" );
        override_option spawn_delay( "SPAWN_DELAY", "1" );

        set_scenario( scenario::generic() );
        REQUIRE( get_option<int>( "INITIAL_DAY" ) == -1 );
        REQUIRE( get_option<int>( "INITIAL_TIME" ) == default_initial_time );
        REQUIRE( get_option<int>( "SPAWN_DELAY" ) == 1 );

        g->start_calendar();

        time_point start_of_cataclysm_1 = calendar::start_of_cataclysm;

        // Even with random time of cataclysm the game starts set amount of time later
        CHECK( calendar::start_of_game == calendar::start_of_cataclysm + 1_hours * default_initial_time +
               1_days );

        // Generate up to 10 worlds to check that randomness makes different dates.
        // If even one of them is different everything is fine.
        bool all_same = true;
        for( int i = 0; i < 10; i++ ) {
            g->start_calendar();
            all_same = start_of_cataclysm_1 == calendar::start_of_cataclysm;
            if( !all_same ) {
                break;
            }
        }

        // There is astronomically low chance for this to fail even if everything is working right.
        CHECK_FALSE( all_same );
    }

    // Reset dates so other tests won't fail
    calendar::start_of_game = calendar::turn_zero;
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::turn = calendar::turn_zero;
}

TEST_CASE( "Random scenario dates" )
{
    // Days counted from start of year
    time_duration first_day_of_summer = calendar::season_length();
    time_duration last_day_of_summer = 2 * calendar::season_length() - 1_days;
    time_duration default_start_hour = 1_hours * 8;

    SECTION( "Random hour" ) {
        set_scenario( &scenario_test_random_hour.obj() );

        g->start_calendar();

        // Random hour should result in something between day default_first_day_of_summer hour 0 - day first_day_of_summer hour 23
        INFO( "Game started on turn " << to_turn<int>( calendar::start_of_game ) );
        CHECK( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer );
        CHECK( calendar::start_of_game <= calendar::turn_zero + first_day_of_summer + 1_hours * 23 );

        time_point start_of_game_1 = calendar::start_of_game;

        // Generate up to 10 worlds to check that randomness makes different dates.
        // If even one of them is different everything is fine.
        bool all_same = true;
        for( int i = 0; i < 10; i++ ) {
            g->start_calendar();
            all_same = start_of_game_1 == calendar::start_of_game;
            if( !all_same ) {
                break;
            }
        }

        // There is astronomically low chance for this to fail even if everything is working right.
        CHECK_FALSE( all_same );
    }

    SECTION( "Random day" ) {
        set_scenario( &scenario_test_random_day.obj() );

        g->start_calendar();

        // Random day should result in something between day first_day_of_summer hour 8 - day last_day_of_summer hour 8
        INFO( "Game started on turn " << to_turn<int>( calendar::start_of_game ) );
        CHECK( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer +
               default_start_hour );
        CHECK( calendar::start_of_game <= calendar::turn_zero + last_day_of_summer + default_start_hour );

        time_point start_of_game_1 = calendar::start_of_game;

        // Generate up to 10 worlds to check that randomness makes different dates.
        // If even one of them is different everything is fine.
        bool all_same = true;
        for( int i = 0; i < 10; i++ ) {
            g->start_calendar();
            all_same = start_of_game_1 == calendar::start_of_game;
            if( !all_same ) {
                break;
            }
        }

        // There is astronomically low chance for this to fail even if everything is working right.
        CHECK_FALSE( all_same );
    }

    SECTION( "Random year" ) {
        set_scenario( &scenario_test_random_year.obj() );

        g->start_calendar();

        // Random year should result in something between year 0 day first_day_of_summer hour 8 - year 11 day first_day_of_summer hour 8
        INFO( "Game started on turn " << to_turn<int>( calendar::start_of_game ) );
        CHECK( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer +
               default_start_hour );
        CHECK( calendar::start_of_game <= calendar::turn_zero + first_day_of_summer +
               calendar::year_length() * 11 + default_start_hour );

        time_point start_of_game_1 = calendar::start_of_game;

        // Generate up to 10 worlds to check that randomness makes different dates.
        // If even one of them is different everything is fine.
        bool all_same = true;
        for( int i = 0; i < 10; i++ ) {
            g->start_calendar();
            all_same = start_of_game_1 == calendar::start_of_game;
            if( !all_same ) {
                break;
            }
        }

        // There is astronomically low chance for this to fail even if everything is working right.
        CHECK_FALSE( all_same );
    }

    // Reset dates so other tests won't fail
    calendar::start_of_game = calendar::turn_zero;
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::turn = calendar::turn_zero;
}
