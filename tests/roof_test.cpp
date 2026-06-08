#include <algorithm>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_catch.h"
#include "coordinates.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "type_id.h"
#include "weather.h"

static const furn_str_id furn_f_canvas_wall( "f_canvas_wall" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_ramp_down_high( "t_ramp_down_high" );
static const ter_str_id ter_t_water_sh( "t_water_sh" );
static const weather_type_id weather_drizzle( "drizzle" );

TEST_CASE( "is_roofed", "[map][roof]" )
{
    clear_map();
    map &here = get_map();

    // Test point at z=0, roof checks z=1
    const tripoint_bub_ms test_pos( 60, 60, 0 );
    const tripoint_bub_ms above_pos = test_pos + tripoint::above;

    SECTION( "out of bounds returns false" ) {
        const tripoint_bub_ms oob( -1, -1, 0 );
        CHECK_FALSE( here.is_roofed( oob ) );
    }

    SECTION( "underground with solid floor above is roofed" ) {
        const tripoint_bub_ms underground( 60, 60, -1 );
        here.build_map_cache( -1 );
        here.build_map_cache( 0 );
        CHECK( here.is_roofed( underground ) );
    }

    SECTION( "top of world is never roofed" ) {
        const tripoint_bub_ms top( 60, 60, OVERMAP_HEIGHT );
        CHECK_FALSE( here.is_roofed( top ) );
    }

    SECTION( "solid floor above means roofed" ) {
        here.ter_set( above_pos, ter_t_floor );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        CHECK( here.is_roofed( test_pos ) );
    }

    SECTION( "open air above means not roofed" ) {
        here.ter_set( above_pos, ter_t_open_air );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        CHECK_FALSE( here.is_roofed( test_pos ) );
    }

    SECTION( "transparent floor (ramp) above is roofed" ) {
        // TRANSPARENT_FLOOR is marked no-floor in floor_cache but blocks precipitation
        here.ter_set( above_pos, ter_t_ramp_down_high );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        CHECK( here.is_roofed( test_pos ) );
    }

    SECTION( "SUN_ROOF_ABOVE furniture overrides no-floor above" ) {
        here.ter_set( above_pos, ter_t_open_air );
        here.furn_set( test_pos, furn_f_canvas_wall );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        CHECK( here.is_roofed( test_pos ) );
    }

    SECTION( "fallback path works when cache is dirty" ) {
        // Build cache with floor above, then dirty it without rebuilding
        here.ter_set( above_pos, ter_t_floor );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        // Manually dirty the z+1 floor cache to force fallback path
        here.set_floor_cache_dirty( above_pos.z() );
        CHECK( here.is_roofed( test_pos ) );
    }

    SECTION( "multi-level walk-up finds roof at z+2" ) {
        // Open air at z=1 but solid floor at z=2 still roofs the tile
        here.ter_set( above_pos, ter_t_open_air );
        const tripoint_bub_ms z2_pos( 60, 60, 2 );
        here.ter_set( z2_pos, ter_t_floor );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        here.build_map_cache( 2 );
        CHECK( here.is_roofed( test_pos ) );
    }

    SECTION( "underground open to sky is not roofed" ) {
        const tripoint_bub_ms underground( 60, 60, -1 );
        // Clear the column from z=0 upward to open air
        for( int z = 0; z <= OVERMAP_HEIGHT; ++z ) {
            here.ter_set( tripoint_bub_ms( 60, 60, z ), ter_t_open_air );
        }
        for( int z = -1; z <= std::min( 2, static_cast<int>( OVERMAP_HEIGHT ) ); ++z ) {
            here.build_map_cache( z );
        }
        CHECK_FALSE( here.is_roofed( underground ) );
        // Restore ground floor so gravity check in later tests doesn't
        // drop the avatar through open air at the default spawn point
        here.ter_set( tripoint_bub_ms( 60, 60, 0 ), ter_t_floor );
    }

    SECTION( "water surface above blocks precipitation" ) {
        here.ter_set( above_pos, ter_t_water_sh );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        CHECK( here.is_roofed( test_pos ) );
    }

    SECTION( "transparent floor blocks on fallback path" ) {
        // Validates the explicit TRANSPARENT_FLOOR check in the fallback path
        here.ter_set( above_pos, ter_t_ramp_down_high );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        here.set_floor_cache_dirty( above_pos.z() );
        CHECK( here.is_roofed( test_pos ) );
    }
}

TEST_CASE( "weather_wetting_respects_roof", "[roof]" )
{
    clear_map();
    clear_avatar();
    map &here = get_map();
    avatar &you = get_avatar();

    scoped_weather_override weather_override( weather_drizzle );

    // Reset turn to a known baseline aligned with once_every(6_seconds)
    calendar::turn = calendar::turn_zero;

    // Ensure avatar has no rain protection
    you.clear_worn();
    you.set_wielded_item( item() );

    const tripoint_bub_ms surface_pos( 60, 60, 0 );

    auto run_weather = [&]( int iterations ) {
        for( int i = 0; i < iterations; ++i ) {
            calendar::turn += 6_seconds;
            handle_weather_effects( weather_drizzle );
        }
    };

    auto reset_wetness = [&]() {
        for( const bodypart_id &bp : you.get_all_body_parts() ) {
            you.set_part_wetness( bp, 0 );
        }
    };

    SECTION( "roofed tile stays dry" ) {
        you.setpos( here, surface_pos );
        here.ter_set( surface_pos + tripoint::above, ter_t_floor );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        reset_wetness();

        run_weather( 60 );
        CHECK( you.get_part_wetness( body_part_torso ) == 0 );
    }

    SECTION( "unroofed tile gets wet" ) {
        you.setpos( here, surface_pos );
        here.ter_set( surface_pos + tripoint::above, ter_t_open_air );
        here.build_map_cache( 0 );
        here.build_map_cache( 1 );
        reset_wetness();

        run_weather( 60 );
        CHECK( you.get_part_wetness( body_part_torso ) > 0 );
    }

    SECTION( "underground roofed tile stays dry" ) {
        const tripoint_bub_ms underground( 60, 60, -1 );
        you.setpos( here, underground, false );
        // z=0 has default solid floor from clear_map
        here.build_map_cache( -1 );
        here.build_map_cache( 0 );
        reset_wetness();

        run_weather( 60 );
        CHECK( you.get_part_wetness( body_part_torso ) == 0 );
    }

    SECTION( "underground open to sky gets wet" ) {
        const tripoint_bub_ms underground( 60, 60, -1 );
        you.setpos( here, underground, false );
        // Clear column from z=0 upward
        for( int z = 0; z <= OVERMAP_HEIGHT; ++z ) {
            here.ter_set( tripoint_bub_ms( 60, 60, z ), ter_t_open_air );
        }
        for( int z = -1; z <= std::min( 2, static_cast<int>( OVERMAP_HEIGHT ) ); ++z ) {
            here.build_map_cache( z );
        }
        reset_wetness();

        run_weather( 60 );
        CHECK( you.get_part_wetness( body_part_torso ) > 0 );
        // Restore ground floor so gravity check in later tests doesn't
        // drop the avatar through open air at the default spawn point
        here.ter_set( tripoint_bub_ms( 60, 60, 0 ), ter_t_floor );
    }
}
