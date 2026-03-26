#include <memory> // IWYU pragma: keep
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "level_cache.h"
#include "lightmap.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "mdarray.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "shadowcasting.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"
#include "weather_type.h"

static const field_type_str_id field_fd_test_green_glow( "fd_test_green_glow" );
static const ter_str_id ter_t_brick_wall( "t_brick_wall" );
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_test_blue_light( "t_test_blue_light" );
static const ter_str_id ter_t_test_red_light( "t_test_red_light" );
static const ter_str_id ter_test_t_utility_light( "test_t_utility_light" );
static const vpart_id vpart_test_red_headlight( "test_red_headlight" );
static const vproto_id vehicle_prototype_car( "car" );

static const time_point midnight = calendar::turn_zero + 0_hours;

// Helper: build full lightmap cache at a given z-level
static void rebuild_lightmap( int zlev )
{
    map &here = get_map();
    for( int z = -2; z <= OVERMAP_HEIGHT; z++ ) {
        here.invalidate_map_cache( z );
    }
    here.invalidate_visibility_cache();
    here.build_map_cache( zlev );
    here.invalidate_visibility_cache();
    here.update_visibility_cache( zlev );
    here.build_map_cache( zlev );
}

// Helper: standard dark-map test setup
static void setup_dark_map()
{
    clear_avatar();
    clear_map_without_vision( -2, OVERMAP_HEIGHT );
    g->reset_light_level();
    calendar::turn = midnight;
    get_player_character().recalc_sight_limits();
}

// Helper: get light color at a map position
static light_color_rgb get_light_color_at( const tripoint_bub_ms &pos )
{
    const map &here = get_map();
    const level_cache &cache = here.access_cache( pos.z() );
    return cache.light_color_cache[pos.x()][pos.y()];
}

// Helper: place terrain at a position with a roof above
static void place_ter_roofed( const tripoint_bub_ms &pos, const ter_str_id &ter )
{
    map &here = get_map();
    here.ter_set( pos, ter.id() );
    here.ter_set( pos + tripoint::above, ter_t_flat_roof.id() );
}

TEST_CASE( "colored_light_source_populates_cache", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_t_test_red_light );

    rebuild_lightmap( 0 );

    const light_color_rgb color = get_light_color_at( src );
    CHECK( color.r > 0.0f );
    CHECK( color.g == 0.0f );
    CHECK( color.b == 0.0f );
}

TEST_CASE( "colored_light_attenuates_with_distance", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 5;
    place_ter_roofed( src, ter_t_test_red_light );

    // Surround with floor so transparency is uniform
    for( int dx = -4; dx <= 4; dx++ ) {
        for( int dy = -4; dy <= 4; dy++ ) {
            tripoint_bub_ms p = src + tripoint( dx, dy, 0 );
            if( p != src ) {
                place_ter_roofed( p, ter_t_floor );
            }
        }
    }

    rebuild_lightmap( 0 );

    // Skip the source tile itself (octant overlap makes it complicated).
    // Check attenuation at increasing distances from the source.
    const float r_at_1 = get_light_color_at( src + tripoint::east ).r;
    const float r_at_2 = get_light_color_at( src + tripoint::east * 2 ).r;
    const float r_at_3 = get_light_color_at( src + tripoint::east * 3 ).r;

    CHECK( r_at_1 > 0.0f );
    CHECK( r_at_1 > r_at_2 );
    CHECK( r_at_2 > r_at_3 );
    CHECK( r_at_3 > 0.0f );
}

TEST_CASE( "wall_blocks_colored_light", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_t_test_red_light );

    // Wall one tile east of the source
    const tripoint_bub_ms wall_pos = src + tripoint::east;
    place_ter_roofed( wall_pos, ter_t_brick_wall );

    // Target is behind the wall
    const tripoint_bub_ms target = src + tripoint::east * 2;
    place_ter_roofed( target, ter_t_floor );

    rebuild_lightmap( 0 );

    const light_color_rgb behind_wall = get_light_color_at( target );
    CHECK( behind_wall.r == 0.0f );
    CHECK( behind_wall.g == 0.0f );
    CHECK( behind_wall.b == 0.0f );
}

TEST_CASE( "two_colored_sources_both_contribute", "[light_color]" )
{
    // Two differently colored sources illuminate their own areas independently.
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms center = get_player_character().pos_bub() + tripoint::east * 5;

    // Red source 3 tiles west of center
    const tripoint_bub_ms red_pos = center + tripoint::west * 3;
    place_ter_roofed( red_pos, ter_t_test_red_light );

    // Blue source 3 tiles east of center
    const tripoint_bub_ms blue_pos = center + tripoint::east * 3;
    place_ter_roofed( blue_pos, ter_t_test_blue_light );

    // Fill area with floor
    for( int dx = -5; dx <= 5; dx++ ) {
        for( int dy = -2; dy <= 2; dy++ ) {
            tripoint_bub_ms p = center + tripoint( dx, dy, 0 );
            if( p != red_pos && p != blue_pos ) {
                place_ter_roofed( p, ter_t_floor );
            }
        }
    }

    rebuild_lightmap( 0 );

    const light_color_rgb mid = get_light_color_at( center );
    CHECK( mid.r > 0.0f );
    CHECK( mid.b > 0.0f );
}

TEST_CASE( "overlapping_same_color_sources_accumulate", "[light_color]" )
{
    // Two same-colored sources on opposite sides of a point produce more
    // color at the midpoint than one source alone: each source reaches
    // center from different octants with different attenuation paths.
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms center = get_player_character().pos_bub() + tripoint::east * 5;

    // First: measure color from a single red source
    place_ter_roofed( center + tripoint::west * 2, ter_t_test_red_light );
    for( int dx = -3; dx <= 3; dx++ ) {
        for( int dy = -3; dy <= 3; dy++ ) {
            tripoint_bub_ms p = center + tripoint( dx, dy, 0 );
            if( get_map().ter( p ) != ter_t_test_red_light.id() ) {
                place_ter_roofed( p, ter_t_floor );
            }
        }
    }
    rebuild_lightmap( 0 );
    const float single_r = get_light_color_at( center ).r;
    CHECK( single_r > 0.0f );

    // Second: add another red source on the other side
    place_ter_roofed( center + tripoint::east * 2, ter_t_test_red_light );
    rebuild_lightmap( 0 );
    const float double_r = get_light_color_at( center ).r;

    // Two sources should produce strictly more color than one
    CHECK( double_r > single_r );
}

TEST_CASE( "uncolored_source_produces_no_color", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_test_t_utility_light );

    rebuild_lightmap( 0 );

    const light_color_rgb color = get_light_color_at( src );
    CHECK( color.r == 0.0f );
    CHECK( color.g == 0.0f );
    CHECK( color.b == 0.0f );
}

TEST_CASE( "colored_light_footprint_matches_scalar", "[light_color]" )
{
    // Invariant: wherever the scalar lightmap shows source contribution above
    // ambient, color should be present. Checked by comparing against a control
    // tile with NO source (purely ambient). Single isolated source, dark map.
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 5;
    place_ter_roofed( src, ter_t_test_red_light );

    // Control tile far from source, same z-level, gives us the ambient lm value
    const tripoint_bub_ms control = get_player_character().pos_bub() + tripoint::west * 10;
    place_ter_roofed( control, ter_t_floor );

    rebuild_lightmap( 0 );

    const map &here = get_map();
    const level_cache &cache = here.access_cache( 0 );
    const float ambient = cache.lm[control.x()][control.y()].max();

    // Check tiles near the source: if lm is meaningfully above ambient, color
    // should be present. And if lm equals ambient, color may or may not be
    // present (color castLight can reach tiles where scalar is ambient-masked).
    int tiles_with_both = 0;
    for( int dx = -4; dx <= 4; dx++ ) {
        for( int dy = -4; dy <= 4; dy++ ) {
            const point p( src.x() + dx, src.y() + dy );
            if( p.x < 0 || p.x >= MAPSIZE_X || p.y < 0 || p.y >= MAPSIZE_Y ) {
                continue;
            }
            const float scalar = cache.lm[p.x][p.y].max();
            const bool has_color = cache.light_color_cache[p.x][p.y].is_colored();

            // If scalar is well above ambient, color must be present
            if( scalar > ambient + 1.0f ) {
                CAPTURE( dx, dy, scalar, ambient, cache.light_color_cache[p.x][p.y].r );
                CHECK( has_color );
                tiles_with_both++;
            }
        }
    }
    // Sanity: we found at least some tiles with both scalar and color
    CHECK( tiles_with_both > 10 );
}

TEST_CASE( "light_color_cache_clears_on_rebuild", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_t_test_red_light );

    rebuild_lightmap( 0 );
    CHECK( get_light_color_at( src ).r > 0.0f );

    // Remove the light source
    place_ter_roofed( src, ter_t_floor );

    rebuild_lightmap( 0 );
    CHECK( get_light_color_at( src ).r == 0.0f );
}

TEST_CASE( "field_colored_light_propagates", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    const tripoint_bub_ms src = get_player_character().pos_bub() + tripoint::east * 3;
    place_ter_roofed( src, ter_t_floor );

    // Place a green-glowing field
    map &here = get_map();
    here.add_field( src, field_fd_test_green_glow.id(), 1 );

    rebuild_lightmap( 0 );

    const light_color_rgb color = get_light_color_at( src );
    CHECK( color.g > 0.0f );
    CHECK( color.r == 0.0f );
    CHECK( color.b == 0.0f );

    // Check propagation to an adjacent tile
    const light_color_rgb adjacent = get_light_color_at( src + tripoint::east );
    CHECK( adjacent.g > 0.0f );
}

TEST_CASE( "vehicle_cone_light_carries_color", "[light_color]" )
{
    setup_dark_map();
    scoped_weather_override weather_clear( WEATHER_CLEAR );

    map &here = get_map();
    const tripoint_bub_ms veh_pos = get_player_character().pos_bub() + tripoint::east * 5;

    for( int dx = -10; dx <= 10; dx++ ) {
        for( int dy = -10; dy <= 10; dy++ ) {
            place_ter_roofed( veh_pos + tripoint( dx, dy, 0 ), ter_t_floor );
        }
    }

    vehicle *veh = here.add_vehicle( vehicle_prototype_car, veh_pos, 0_degrees, 100, 0 );
    REQUIRE( veh != nullptr );

    // Replace one stock headlight with our red test headlight
    std::vector<point_rel_ms> hl_mounts;
    for( const vpart_reference &vpr : veh->get_any_parts( VPFLAG_CONE_LIGHT ) ) {
        hl_mounts.push_back( vpr.part().mount );
        veh->remove_part( vpr.part() );
    }
    veh->part_removal_cleanup( here );
    REQUIRE( !hl_mounts.empty() );

    const int part_idx = veh->install_part( here, hl_mounts.front(), vpart_test_red_headlight );
    REQUIRE( part_idx >= 0 );

    vehicle_part &installed = veh->part( part_idx );
    installed.enabled = true;
    REQUIRE( installed.info().has_flag( VPFLAG_CONE_LIGHT ) );

    rebuild_lightmap( 0 );

    // Find the brightest colored tile near the headlight. The cone direction
    // depends on vehicle facing + part mount direction, so scan rather than
    // hardcoding an offset.
    const tripoint_bub_ms hl_pos = veh->bub_part_pos( here, installed );
    const level_cache &cache = here.access_cache( 0 );
    float best_r = 0.0f;
    tripoint best_offset;
    for( int dx = -6; dx <= 6; dx++ ) {
        for( int dy = -6; dy <= 6; dy++ ) {
            const tripoint_bub_ms p = hl_pos + tripoint( dx, dy, 0 );
            if( !here.inbounds( p ) ) {
                continue;
            }
            const float r = cache.light_color_cache[p.x()][p.y()].r;
            if( r > best_r ) {
                best_r = r;
                best_offset = tripoint( dx, dy, 0 );
            }
        }
    }
    REQUIRE( best_r > 0.0f );

    // The opposite direction should have less (or no) color
    const tripoint_bub_ms behind = hl_pos - best_offset;
    REQUIRE( here.inbounds( behind ) );
    const light_color_rgb behind_color = get_light_color_at( behind );
    CAPTURE( best_r, best_offset.x, best_offset.y );
    CAPTURE( behind_color.r, behind_color.g, behind_color.b );

    CHECK( best_r > behind_color.r );
}
