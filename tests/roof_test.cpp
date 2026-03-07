#include <string>

#include "cata_catch.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "point.h"
#include "type_id.h"

static const furn_str_id furn_f_canvas_wall( "f_canvas_wall" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_open_air( "t_open_air" );
static const ter_str_id ter_t_ramp_down_high( "t_ramp_down_high" );

TEST_CASE( "is_roofed", "[map]" )
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

    SECTION( "underground is always roofed" ) {
        const tripoint_bub_ms underground( 60, 60, -1 );
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
}
