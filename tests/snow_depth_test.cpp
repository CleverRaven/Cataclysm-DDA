#include "cata_catch.h"

#include <string>
#include <unordered_map>

#include "calendar.h"
#include "character.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "weather.h"

static const weather_type_id weather_snowing( "snowing" );
static const weather_type_id weather_sunny( "sunny" );

TEST_CASE( "snow_accumulation_and_melt", "[snow][weather]" )
{
    clear_map();
    weather_manager &weather = get_weather();
    weather.snow_depth_map.clear();

    // Align to minute boundary so once_every(1_minutes) in update_snow_depth fires
    calendar::turn -= ( calendar::turn - calendar::turn_zero ) % 1_minutes;

    Character &player = get_player_character();
    const tripoint_abs_omt player_omt = player.pos_abs_omt();

    SECTION( "snow accumulates during snowfall" ) {
        weather.weather_id = weather_snowing;
        weather.temperature = units::from_celsius( -5 );

        // Simulate 30 minutes of snowfall by advancing time in 1-minute steps
        for( int i = 0; i < 30; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }

        CHECK( weather.get_snow_depth_mm( player_omt ) > 0.0 );
    }

    SECTION( "snow melts above freezing" ) {
        // Pre-seed some snow
        weather.snow_depth_map[player_omt] = { 200.0, calendar::turn };

        weather.weather_id = weather_sunny;
        weather.temperature = 10_C;

        for( int i = 0; i < 30; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }

        CHECK( weather.get_snow_depth_mm( player_omt ) < 200.0 );
    }

    SECTION( "no snow in warm weather" ) {
        weather.weather_id = weather_sunny;
        weather.temperature = 20_C;

        for( int i = 0; i < 30; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }

        CHECK( weather.get_snow_depth_mm( player_omt ) < 0.001 );
    }

    SECTION( "unvisited OMT seeds from nearest neighbor" ) {
        weather.snow_depth_map[player_omt] = { 150.0, calendar::turn };

        // Query an adjacent OMT with no data
        tripoint_abs_omt neighbor = player_omt + tripoint::east;
        CHECK( weather.get_snow_depth_mm( neighbor ) == Approx( 150.0 ) );
    }

    SECTION( "depth is clamped to maximum" ) {
        weather.snow_depth_map[player_omt] = { 790.0, calendar::turn };
        weather.weather_id = weather_snowing;
        weather.temperature = units::from_celsius( -10 );

        // Accumulate past max
        for( int i = 0; i < 60; i++ ) {
            calendar::turn += 1_minutes;
            weather.update_snow_depth();
        }

        CHECK( weather.get_snow_depth_mm( player_omt ) <= 800.0 );
    }

    SECTION( "catch-up delta is clamped" ) {
        // Entry from 3 days ago during a blizzard
        weather.snow_depth_map[player_omt] = { 0.0, calendar::turn - 3_days };
        weather.weather_id = weather_snowing;
        weather.temperature = units::from_celsius( -5 );

        calendar::turn += 1_minutes;
        weather.update_snow_depth();

        // Should only accumulate for 1 hour (max catchup), not 3 days
        double depth = weather.get_snow_depth_mm( player_omt );
        CHECK( depth > 0.0 );
        CHECK( depth < 200.0 );
    }
}

TEST_CASE( "snow_movement_penalty", "[snow][movement]" )
{
    clear_map();
    clear_character( get_player_character() );
    weather_manager &weather = get_weather();
    weather.snow_depth_map.clear();

    Character &player = get_player_character();
    map &here = get_map();
    const tripoint_abs_omt player_omt = player.pos_abs_omt();

    // Make sure player is outside and unroofed
    here.ter_set( player.pos_bub(), ter_id( "t_grass" ) );
    here.ter_set( player.pos_bub() + tripoint::above, ter_id( "t_open_air" ) );
    here.build_map_cache( player.posz() );
    here.build_map_cache( player.posz() + 1 );

    SECTION( "heavy snow increases movement cost" ) {
        weather.snow_depth_map[player_omt] = { 600.0, calendar::turn };
        weather.weather_id = weather_sunny;
        weather.temperature = units::from_celsius( -5 );

        const int cost_with_snow = player.run_cost( 100 );

        weather.snow_depth_map[player_omt] = { 0.0, calendar::turn };
        const int cost_without_snow = player.run_cost( 100 );

        CHECK( cost_with_snow > cost_without_snow );
    }

    SECTION( "no penalty indoors" ) {
        weather.snow_depth_map[player_omt] = { 600.0, calendar::turn };

        // Put a floor above to make it roofed
        here.ter_set( player.pos_bub() + tripoint::above, ter_id( "t_floor" ) );
        here.build_map_cache( player.posz() );
        here.build_map_cache( player.posz() + 1 );

        // Get cost with roof
        const int cost_roofed = player.run_cost( 100 );

        // Remove roof
        here.ter_set( player.pos_bub() + tripoint::above, ter_id( "t_open_air" ) );
        here.build_map_cache( player.posz() );
        here.build_map_cache( player.posz() + 1 );
        const int cost_unroofed = player.run_cost( 100 );

        CHECK( cost_roofed < cost_unroofed );
    }
}
