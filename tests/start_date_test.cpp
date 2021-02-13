#include "catch/catch.hpp"

#include "game.h"
#include "scenario.h"
#include "options.h"
#include "options_helpers.h"



TEST_CASE( "Test start dates" )
{
    int default_initial_day = 60;
    int default_initial_time = 8;  // Note that the default for scenario time is same
    int default_spawn_delay = 0;
    int default_season_length = 91;

    SECTION( "Default start date with no scenario" ) {
        set_scenario( scenario::generic() );
        CHECK( get_option<int>( "INITIAL_DAY" ) == default_initial_day );
        CHECK( get_option<int>( "INITIAL_TIME" ) == default_initial_time );
        CHECK( get_option<int>( "SPAWN_DELAY" ) == default_spawn_delay );

        g->start_calendar();

        REQUIRE( calendar::start_of_cataclysm == calendar::turn_zero + 1_days * default_initial_day );
        REQUIRE( calendar::start_of_game == calendar::turn_zero + 1_days * default_initial_day + 1_hours *
                 default_initial_time );
    }

    SECTION( "Customized start date" ) {
        override_option initial_day( "INITIAL_DAY", "100" );
        override_option initial_time( "INITIAL_TIME", "13" );
        override_option spawn_delay( "SPAWN_DELAY", "10" );

        set_scenario( scenario::generic() );
        CHECK( get_option<int>( "INITIAL_DAY" ) == 100 );
        CHECK( get_option<int>( "INITIAL_TIME" ) == 13 );
        CHECK( get_option<int>( "SPAWN_DELAY" ) == 10 );

        g->start_calendar();

        REQUIRE( calendar::start_of_cataclysm == calendar::turn_zero + 1_days * 100 );
        REQUIRE( calendar::start_of_game == calendar::turn_zero + 1_days * ( 100 + 10 ) + 1_hours * 13 );
    }

    SECTION( "Scenario with start date" ) {
        static const string_id<scenario> ambushed_scenario( "ambushed" );
        set_scenario( &ambushed_scenario.obj() );
        // Ambushed scenario starts on winter day 1 (day 273 with default season length)

        // Initial time and spawn delay should not affect scenario dates
        override_option initial_time( "INITIAL_TIME", "13" );
        override_option spawn_delay( "SPAWN_DELAY", "10" );

        CHECK( get_option<int>( "INITIAL_DAY" ) == default_initial_day );
        CHECK( get_option<int>( "SEASON_LENGTH" ) == default_season_length );

        g->start_calendar();

        REQUIRE( calendar::start_of_cataclysm == calendar::turn_zero + 1_days * default_initial_day );
        REQUIRE( calendar::start_of_game == calendar::turn_zero + 1_days * 273 + 1_hours *
                 default_initial_time );
    }

    SECTION( "Scenario with start date tries to start before cataclysm" ) {
        static const string_id<scenario> ambushed_scenario( "ambushed" );
        set_scenario( &ambushed_scenario.obj() );
        // Ambushed scenario starts on winter day 1 (day 273 with default season length)

        override_option initial_day( "INITIAL_DAY", "350" );

        CHECK( get_option<int>( "INITIAL_DAY" ) == 350 );
        CHECK( get_option<int>( "INITIAL_TIME" ) == default_initial_time );
        CHECK( get_option<int>( "SPAWN_DELAY" ) == default_spawn_delay );
        CHECK( get_option<int>( "SEASON_LENGTH" ) == default_season_length );

        g->start_calendar();

        REQUIRE( calendar::start_of_cataclysm == calendar::turn_zero + 1_days * 350 );

        // Start should be moved forwards by one year from normal to avoid too early start
        REQUIRE( calendar::start_of_game == calendar::turn_zero + 1_days * 273 + 1_hours *
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
        CHECK( get_option<int>( "INITIAL_DAY" ) == -1 );
        CHECK( get_option<int>( "INITIAL_TIME" ) == default_initial_time );
        CHECK( get_option<int>( "SPAWN_DELAY" ) == 1 );

        g->start_calendar();

        time_point start_of_cataclysm_1 = calendar::start_of_cataclysm;

        // Even with random time of cataclysm the game starts set amount of time later
        REQUIRE( calendar::start_of_game == calendar::start_of_cataclysm + 1_hours * default_initial_time +
                 1_days );


        // Second and third random starts should have different time
        g->start_calendar();
        time_point start_of_cataclysm_2 = calendar::start_of_cataclysm;
        g->start_calendar();
        time_point start_of_cataclysm_3 = calendar::start_of_cataclysm;

        // There is decent chance that two of the three are on same date. It is enough that even one of them is different.
        // There is still small chance that all three dates are same so this test can fail even when everything is fine
        bool all_same = start_of_cataclysm_1 == start_of_cataclysm_2  &&
                        start_of_cataclysm_1 == start_of_cataclysm_3;
        REQUIRE_FALSE( all_same );
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
    time_duration default_start_hour = 1_hours * 8

    SECTION( "Random hour" ) {
        static const string_id<scenario> random_hour_scenario( "test_random_hour" );
        set_scenario( &random_hour_scenario.obj() );

        g->start_calendar();

        // Random hour should result in something between day default_first_day_of_summer hour 0 - day first_day_of_summer hour 23
        INFO( "Game started on turn " << to_turn<int>( calendar::start_of_game ) );
        REQUIRE( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer );
        REQUIRE( calendar::start_of_game <= calendar::turn_zero + first_day_of_summer + 1_hours * 23 );

        time_point start_of_game_1 = calendar::start_of_game;

        // Second and third random starts should have different time
        g->start_calendar();
        time_point start_of_game_2 = calendar::start_of_game;
        g->start_calendar();
        time_point start_of_game_3 = calendar::start_of_game;

        // There is decent chance that two of the three are on same date. It is enough that even one of them is different.
        // There is still small chance that all three dates are same so this test can fail even when everything is fine
        bool all_same = start_of_game_1 == start_of_game_2  && start_of_game_1 == start_of_game_3;
        REQUIRE_FALSE( all_same );
    }

    SECTION( "Random day" ) {
        static const string_id<scenario> random_day_scenario( "test_random_day" );
        set_scenario( &random_day_scenario.obj() );

        g->start_calendar();

        // Random day should result in something between day first_day_of_summer hour 8 - day last_day_of_summer hour 8
        INFO( "Game started on turn " << to_turn<int>( calendar::start_of_game ) );
        REQUIRE( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer +
                 default_start_hour );
        REQUIRE( calendar::start_of_game <= calendar::turn_zero + last_day_of_summer + default_start_hour );

        time_point start_of_game_1 = calendar::start_of_game;

        // Second and third random starts should have different time
        g->start_calendar();
        time_point start_of_game_2 = calendar::start_of_game;
        g->start_calendar();
        time_point start_of_game_3 = calendar::start_of_game;

        // There is decent chance that two of the three are on same date. It is enough that even one of them is different.
        // There is still small chance that all three dates are same so this test can fail even when everything is fine
        bool all_same = start_of_game_1 == start_of_game_2  && start_of_game_1 == start_of_game_3;
        REQUIRE_FALSE( all_same );
    }

    SECTION( "Random year" ) {
        static const string_id<scenario> random_year_scenario( "test_random_year" );
        set_scenario( &random_year_scenario.obj() );

        g->start_calendar();

        // Random year should result in something between year 0 day first_day_of_summer hour 8 - year 11 day first_day_of_summer hour 8
        INFO( "Game started on turn " << to_turn<int>( calendar::start_of_game ) );
        REQUIRE( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer +
                 default_start_hour );
        REQUIRE( calendar::start_of_game <= calendar::turn_zero + first_day_of_summer +
                 calendar::year_length() * 11 + default_start_hour );

        time_point start_of_game_1 = calendar::start_of_game;

        // Second and third random starts should have different time
        g->start_calendar();
        time_point start_of_game_2 = calendar::start_of_game;
        g->start_calendar();
        time_point start_of_game_3 = calendar::start_of_game;
        g->start_calendar();
        time_point start_of_game_4 = calendar::start_of_game;

        // There is decent chance that two of the three are on same date. It is enough that even one of them is different.
        bool all_same = start_of_game_1 == start_of_game_2  && start_of_game_1 == start_of_game_3 &&
                        start_of_game_1 == start_of_game_4;
        REQUIRE_FALSE( all_same );
    }

    // Reset dates so other tests won't fail
    calendar::start_of_game = calendar::turn_zero;
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::turn = calendar::turn_zero;
}



