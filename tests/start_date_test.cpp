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

TEST_CASE( "Test_start_dates" )
{
    int default_season_length = 91;
    int default_year_length = default_season_length * 4;

    on_out_of_scope guard{ []()
    {
        set_scenario( scenario::generic() );
        g->start_calendar();
    } };

    SECTION( "Scenario with custom game start date" ) {
        scenario scen = scenario_test_custom_game.obj();
        set_scenario( &scen );
        g->start_calendar();

        CHECK( calendar::start_of_game == calendar::turn_zero +
               1_hours * 18 +
               1_days * 6 +
               1_days * default_season_length * season_type::SUMMER +
               1_days * default_year_length * 3
             );
    }

    SECTION( "Scenario has game start date before cataclysm start date" ) {
        scenario scen = scenario_test_custom_game_invalid.obj();
        set_scenario( &scen );
        g->start_calendar();

        CHECK( calendar::start_of_game == calendar::start_of_cataclysm );
    }

    SECTION( "Scenario with custom cataclysm start date" ) {
        scenario scen = scenario_test_custom_cataclysm.obj();
        set_scenario( &scen );
        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero +
               1_hours * 1 +
               1_days * 8 +
               1_days * default_season_length * season_type::AUTUMN +
               1_days * default_year_length * 6
             );
    }

    SECTION( "Scenario with custom cataclysm start date and game start date" ) {
        scenario scen = scenario_test_custom_both.obj();
        set_scenario( &scen );
        g->start_calendar();

        CHECK( calendar::start_of_cataclysm == calendar::turn_zero +
               1_hours * 6 +
               1_days * 3 +
               1_days * default_season_length * season_type::WINTER +
               1_days * default_year_length * 5
             );
        CHECK( calendar::start_of_game == calendar::turn_zero +
               1_hours * 13 +
               1_days * 18 +
               1_days * default_season_length * season_type::AUTUMN +
               1_days * default_year_length * 8
             );
    }

    // Reset dates so other tests won't fail
    calendar::start_of_game = calendar::turn_zero;
    calendar::start_of_cataclysm = calendar::turn_zero;
    calendar::turn = calendar::turn_zero;
}
