#include "catch/catch.hpp"

#include "cata_scope_helpers.h"
#include "game.h"
#include "scenario.h"
#include "options.h"
#include "options_helpers.h"

static const string_id<scenario> scenario_test_custom_both( "test_custom_both" );
static const string_id<scenario> scenario_test_custom_cataclysm( "test_custom_cataclysm" );
static const string_id<scenario> scenario_test_custom_game( "test_custom_game" );
static const string_id<scenario> scenario_test_custom_game_invalid( "test_custom_game_invalid" );
static const string_id<scenario> scenario_test_random_day( "test_random_day" );
static const string_id<scenario> scenario_test_random_hour( "test_random_hour" );
static const string_id<scenario> scenario_test_random_year( "test_random_year" );

TEST_CASE( "Test_start_dates" )
{
    int default_initial_day = 0;
    int default_season_length = 91;
    int default_year_length = default_season_length * 4;

    on_out_of_scope guard{ []()
    {
        set_scenario( scenario::generic() );
    } };

    SECTION( "Default game start date with no scenario" ) {
        scenario scen = *scenario::generic();

        set_scenario( &scen );
        scen.rerandomize();
        g->start_calendar();

        CHECK( calendar::start_of_game >= calendar::turn_zero + 1_days * default_initial_day + 0_hours );
        CHECK( calendar::start_of_game <= calendar::turn_zero + 1_days * default_initial_day + 23_hours );
    }

    SECTION( "Scenario with custom game start date" ) {
        scenario scen = scenario_test_custom_game.obj();

        set_scenario( &scen );
        scen.rerandomize();
        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero );
        CHECK( calendar::start_of_game == calendar::turn_zero +
               1_hours * 18 +
               1_days * 7 +
               1_days * default_season_length * season_type::SUMMER +
               1_days * default_year_length * 3
             );
    }

    SECTION( "Scenario has game start date before cataclysm start date" ) {
        scenario scen = scenario_test_custom_game_invalid.obj();

        set_scenario( &scen );
        scen.rerandomize();
        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero );
        CHECK( calendar::start_of_game == calendar::turn_zero );
    }

    SECTION( "Scenario with custom cataclysm start date" ) {
        scenario scen = scenario_test_custom_cataclysm.obj();

        set_scenario( &scen );
        scen.rerandomize();
        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero +
               1_hours * 1 +
               1_days * 9 +
               1_days * default_season_length * season_type::AUTUMN +
               1_days * default_year_length * 6
             );
    }

    SECTION( "Scenario with custom cataclysm start date and game start date" ) {
        scenario scen = scenario_test_custom_both.obj();

        set_scenario( &scen );
        scen.rerandomize();
        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero +
               1_hours * 6 +
               1_days * 4 +
               1_days * default_season_length * season_type::WINTER +
               1_days * default_year_length * 5
             );
        CHECK( calendar::start_of_game == calendar::turn_zero +
               1_hours * 13 +
               1_days * 19 +
               1_days * default_season_length * season_type::AUTUMN +
               1_days * default_year_length * 8
             );
    }

    // Reset dates so other tests won't fail
    calendar::start_of_game = calendar::turn_zero;
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::turn = calendar::turn_zero;
}

TEST_CASE( "Random_scenario_dates" )
{
    // Days counted from start of year
    time_duration first_day_of_summer = calendar::season_length();
    time_duration last_day_of_summer = 2 * calendar::season_length() - 1_days;
    time_duration default_start_hour = 1_hours * 8;

    on_out_of_scope guard{ []()
    {
        set_scenario( scenario::generic() );
    } };

    SECTION( "Random hour" ) {
        scenario scen = scenario_test_random_hour.obj();

        set_scenario( &scen );
        scen.rerandomize();
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
            set_scenario( &scen );
            scen.rerandomize();
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
        scenario scen = scenario_test_random_day.obj();

        set_scenario( &scen );
        scen.rerandomize();
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
            set_scenario( &scen );
            scen.rerandomize();
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
        scenario scen = scenario_test_random_year.obj();

        set_scenario( &scen );
        scen.rerandomize();
        g->start_calendar();

        // Random year should result in something between year 0 day first_day_of_summer hour 8 - year 11 day first_day_of_summer hour 8
        INFO( "Game started on turn " << to_turn<int>( calendar::start_of_game ) );
        CHECK( calendar::start_of_game >= calendar::turn_zero + first_day_of_summer +
               default_start_hour );
        CHECK( calendar::start_of_game <= calendar::turn_zero + first_day_of_summer +
               calendar::year_length() * 10 + default_start_hour );

        time_point start_of_game_1 = calendar::start_of_game;

        // Generate up to 10 worlds to check that randomness makes different dates.
        // If even one of them is different everything is fine.
        bool all_same = true;
        for( int i = 0; i < 10; i++ ) {
            set_scenario( &scen );
            scen.rerandomize();
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
