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
        static const string_id<scenario> ambushed_scenario( "ambushed" );
        set_scenario( &ambushed_scenario.obj() );
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
        static const string_id<scenario> ambushed_scenario( "ambushed" );
        set_scenario( &ambushed_scenario.obj() );
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
               default_initial_time + 1_days * default_season_length * 4 );
    }

    // Reset dates so other tests won't fail
    calendar::start_of_game = calendar::turn_zero;
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::turn = calendar::turn_zero;
}

TEST_CASE( "Random dates", "[!mayfail]" )
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
               1_days * 1 );


        g->start_calendar();

        time_point start_of_cataclysm_2 = calendar::start_of_cataclysm;

        // Two random starts should result in different dates
        // There is about 1/364 chance for this to fail when everything is working fine
        // Don't worry about it unless it starts failing often.
        CHECK( start_of_cataclysm_1 != start_of_cataclysm_2 );
    }

    // Reset dates so other tests won't fail
    calendar::start_of_game = calendar::turn_zero;
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::turn = calendar::turn_zero;
}

TEST_CASE( "Random scenario dates", "[!mayfail]" )
{
    // Days counted from start of year
    time_duration first_day_of_summer = calendar::season_length();
    time_duration last_day_of_summer = 2 * calendar::season_length() - 1_days;

    SECTION( "Random hour" ) {
        static const string_id<scenario> random_hour_scenario( "test_random_hour" );
        set_scenario( &random_hour_scenario.obj() );

        g->start_calendar();

        // Random hour should result in something between day default_first_day_of_summer hour 0 - day first_day_of_summer hour 23
        CHECK( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer );
        CHECK( calendar::start_of_game <= calendar::turn_zero + first_day_of_summer + 1_hours * 24 );

        time_point start_of_game_1 = calendar::start_of_game;

        // Second random start should have different time
        g->start_calendar();

        // This test has about 1/24 chance to fail when everything is working fine.
        CHECK( start_of_game_1 != calendar::start_of_game );
    }

    SECTION( "Random day" ) {
        static const string_id<scenario> random_day_scenario( "test_random_day" );
        set_scenario( &random_day_scenario.obj() );

        g->start_calendar();

        // Random day should result in something between day first_day_of_summer hour 0 - day last_day_of_summer hour 0
        CHECK( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer );
        CHECK( calendar::start_of_game <= calendar::turn_zero + last_day_of_summer );

        time_point start_of_game_1 = calendar::start_of_game;

        // Second random start should have different time
        g->start_calendar();

        // This test has about 1/91 chance to fail when everything is working fine.
        CHECK( start_of_game_1 != calendar::start_of_game );
    }

    SECTION( "Random year" ) {
        static const string_id<scenario> random_year_scenario( "test_random_year" );
        set_scenario( &random_year_scenario.obj() );

        g->start_calendar();

        // Random year should result in something between year 0 day first_day_of_summer hour 0 - year 11 day first_day_of_summer hour 0
        CHECK( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer );
        CHECK( calendar::start_of_game <= calendar::turn_zero + first_day_of_summer + calendar::year_length() * 11 );

        time_point start_of_game_1 = calendar::start_of_game;

        // Second random start should have different time
        g->start_calendar();

        // This test has about 1/11 chance to fail when everything is working fine.
        CHECK( start_of_game_1 != calendar::start_of_game );
    }

    SECTION( "Fully random date" ) {
        static const string_id<scenario> random_date_scenario( "test_random" );
        set_scenario( &random_date_scenario.obj() );

        g->start_calendar();

        // Random year should result in something between year 0 day first_day_of_summer hour 0 - year 11 day last_day_of_summer hour 23
        CHECK( calendar::start_of_game >= calendar::turn_zero );
        CHECK( calendar::start_of_game <= calendar::turn_zero + last_day_of_summer + calendar::year_length() * 11 + 1_hours
               * 23 );

        time_point start_of_game_1 = calendar::start_of_game;

        // Second random start should have different time
        g->start_calendar();

        // This test has about 1/364 chance to fail when everything is working fine.
        CHECK( start_of_game_1 != calendar::start_of_game );
    }

    // Reset dates so other tests won't fail
    calendar::start_of_game = calendar::turn_zero;
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::turn = calendar::turn_zero;
}



