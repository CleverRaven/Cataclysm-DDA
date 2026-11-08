#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "enums.h"
#include "flexbuffer_json.h"
#include "game.h"
#include "json_loader.h"
#include "level_cache.h"
#include "lightmap.h"
#include "map.h"
#include "map_helpers.h"
#include "map_scale_constants.h"
#include "mdarray.h"
#include "options.h"
#include "options_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "smooth_lighting.h"
#include "type_id.h"
#include "weather_type.h"

static const field_type_str_id field_fd_clairvoyant( "fd_clairvoyant" );

static const mtype_id mon_zombie_electric( "mon_zombie_electric" );

static const ter_str_id ter_t_brick_wall( "t_brick_wall" );
static const ter_str_id ter_t_dirt( "t_dirt" );
static const ter_str_id ter_t_door_c( "t_door_c" );
static const ter_str_id ter_t_door_o( "t_door_o" );
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_shrub( "t_shrub" );
static const ter_str_id ter_t_tree( "t_tree" );
static const ter_str_id ter_t_utility_light( "t_utility_light" );
static const ter_str_id ter_t_window( "t_window" );

static const trait_id trait_MYOPIC( "MYOPIC" );

static const time_point midnight = calendar::turn_zero + 0_hours;
static const time_point day_time = calendar::turn_zero + 9_hours + 30_minutes;

// avatar at `center` with nothing that changes its vision, map cleared
static void reset_avatar_and_map( const tripoint_bub_ms &center )
{
    clear_avatar();
    Character &you = get_player_character();
    you.clear_mutations();
    g->place_player( center );
    // outside the middle submaps the map shifts and avatar lands elsewhere
    REQUIRE( get_avatar().pos_bub() == center );
    clear_map_without_vision( -2, OVERMAP_HEIGHT );
    g->reset_light_level();
}

// dark roofed room of floor walled at `radius`, avatar at its center
static void build_dark_room( const tripoint_bub_ms &center, const int radius )
{
    reset_avatar_and_map( center );
    calendar::turn = midnight;
    map &here = get_map();
    for( int dx = -radius; dx <= radius; ++dx ) {
        for( int dy = -radius; dy <= radius; ++dy ) {
            const tripoint_bub_ms p = center + tripoint( dx, dy, 0 );
            const bool edge = std::abs( dx ) == radius || std::abs( dy ) == radius;
            here.ter_set( p, edge ? ter_t_brick_wall.id() : ter_t_floor.id() );
            here.ter_set( p + tripoint::above, ter_t_flat_roof.id() );
        }
    }
    get_player_character().recalc_sight_limits();
}

// caches as a turn leaves them, run twice so vision threshold follows new light
static void settle_caches( const int z )
{
    map &here = get_map();
    for( int pass = 0; pass < 2; ++pass ) {
        for( int lz = -2; lz <= OVERMAP_HEIGHT; ++lz ) {
            here.invalidate_map_cache( lz );
        }
        here.build_map_cache( z );
        here.invalidate_visibility_cache();
        here.update_visibility_cache( z );
    }
}

TEST_CASE( "smooth_light_follows_apparent_light_of_the_classifier", "[smooth_lighting][vision]" )
{
    scoped_weather_override weather( WEATHER_CLEAR );
    const tripoint_bub_ms center( 60, 60, 0 );
    GIVEN( "a utility light three tiles from the avatar in a dark room" ) {
        build_dark_room( center, 14 );
        get_map().ter_set( center + tripoint::east * 3, ter_t_utility_light.id() );
        settle_caches( 0 );
        const map &here = get_map();
        const level_cache &ch = here.access_cache( 0 );
        const float threshold = here.get_visibility_variables_cache().vision_threshold;
        WHEN( "every tile in the room is classified" ) {
            std::vector<std::pair<float, float>> low_tiles;
            for( int dx = -13; dx <= 13; ++dx ) {
                for( int dy = -13; dy <= 13; ++dy ) {
                    const tripoint_bub_ms p = center + tripoint( dx, dy, 0 );
                    const lit_level ll = ch.visibility_cache[p.x()][p.y()];
                    const float apparent = map::apparent_light_helper( ch, p ).apparent_light;
                    const smooth_lighting::light_cell cell =
                        smooth_lighting::classify_light_cell( ll, apparent, threshold );
                    CAPTURE( p, ll, apparent, threshold, cell.light );
                    if( ll == lit_level::BRIGHT ) {
                        CHECK( cell.detail );
                        CHECK( cell.light == 1.0f );
                    } else if( ll == lit_level::LOW ) {
                        CHECK( cell.detail );
                        CHECK( cell.light >= 0.0f );
                        CHECK( cell.light < 1.0f );
                        low_tiles.emplace_back( apparent, cell.light );
                    } else {
                        CHECK_FALSE( cell.detail );
                    }
                }
            }
            THEN( "dim tiles grade up from no light at the vision threshold" ) {
                REQUIRE( low_tiles.size() > 2 );
                std::sort( low_tiles.begin(), low_tiles.end() );
                for( size_t i = 1; i < low_tiles.size(); ++i ) {
                    CAPTURE( low_tiles[i - 1].first, low_tiles[i].first );
                    CHECK( low_tiles[i - 1].second <= low_tiles[i].second );
                }
                CAPTURE( low_tiles.front().first );
                CHECK( low_tiles.front().second < 0.35f );
            }
        }
    }
}

TEST_CASE( "smooth_light_detail_matches_clear_visibility", "[smooth_lighting][vision]" )
{
    const map &here = get_map();
    const visibility_variables &cache = here.get_visibility_variables_cache();
    for( const lit_level ll : {
             lit_level::DARK, lit_level::LOW, lit_level::BRIGHT_ONLY, lit_level::LIT,
             lit_level::BRIGHT, lit_level::MEMORIZED, lit_level::BLANK
         } ) {
        CAPTURE( ll );
        CHECK( smooth_lighting::classify_light_cell( ll, LIGHT_AMBIENT_LIT, 1.0f ).detail ==
               ( here.get_visibility( ll, cache ) == visibility_type::CLEAR ) );
    }
}

TEST_CASE( "light_source_seen_from_beyond_unimpaired_range_shows_no_detail",
           "[smooth_lighting][vision]" )
{
    scoped_weather_override weather( WEATHER_CLEAR );
    const tripoint_bub_ms center( 60, 60, 0 );
    reset_avatar_and_map( center );
    calendar::turn = day_time;
    Character &you = get_player_character();
    you.set_mutation( trait_MYOPIC );
    you.recalc_sight_limits();
    const tripoint_bub_ms far_point = center + tripoint::east * 14;
    REQUIRE( g->place_critter_at( mon_zombie_electric, far_point ) );
    settle_caches( 0 );
    const map &here = get_map();
    const level_cache &ch = here.access_cache( 0 );
    const lit_level ll = ch.visibility_cache[far_point.x()][far_point.y()];
    REQUIRE( ll == lit_level::BRIGHT_ONLY );
    const float apparent = map::apparent_light_helper( ch, far_point ).apparent_light;
    CHECK_FALSE( smooth_lighting::classify_light_cell( ll, apparent,
                 here.get_visibility_variables_cache().vision_threshold ).detail );
}

TEST_CASE( "clairvoyance_shows_full_light_like_classic_tiles", "[smooth_lighting][vision]" )
{
    scoped_weather_override weather( WEATHER_CLEAR );
    const tripoint_bub_ms center( 60, 60, 0 );
    build_dark_room( center, 6 );
    const tripoint_bub_ms behind_wall = center + tripoint::east * 8;
    get_map().add_field( behind_wall, field_fd_clairvoyant.id(), 1 );
    settle_caches( 0 );
    const map &here = get_map();
    const level_cache &ch = here.access_cache( 0 );
    const lit_level ll = ch.visibility_cache[behind_wall.x()][behind_wall.y()];
    REQUIRE( ll == lit_level::BRIGHT );
    const smooth_lighting::light_cell cell = smooth_lighting::classify_light_cell( ll, 0.0f,
            here.get_visibility_variables_cache().vision_threshold );
    CHECK( cell.detail );
    CHECK( cell.light == 1.0f );
}

TEST_CASE( "light_above_threshold_spans_threshold_to_lit", "[smooth_lighting]" )
{
    for( const float threshold : {
             0.0f, 1.0f, LIGHT_AMBIENT_LOW
         } ) {
        CAPTURE( threshold );
        CHECK( smooth_lighting::light_above_threshold( threshold, threshold ) == 0.0f );
        CHECK( smooth_lighting::light_above_threshold( LIGHT_AMBIENT_LIT, threshold ) == 1.0f );
        CHECK( smooth_lighting::light_above_threshold( LIGHT_AMBIENT_LIT * 3.0f, threshold ) == 1.0f );
    }
}

static smooth_lighting::sprite_footprint footprint( const point &size, const point &opq_min,
        const point &opq_max, const int top )
{
    smooth_lighting::sprite_footprint f;
    f.size = size;
    f.opaque = half_open_rectangle<point>( opq_min, opq_max );
    f.top = top;
    return f;
}

TEST_CASE( "standing_sprite_classification_follows_flip_and_turn", "[smooth_lighting][tiles]" )
{
    const smooth_lighting::tile_geometry ortho{ 32, 32, false };
    GIVEN( "32x64 sprite with opaque bottom half, drawn one tile up" ) {
        smooth_lighting::sprite_footprint f = footprint( { 32, 64 }, { 0, 32 }, { 32, 64 }, -32 );
        THEN( "it lies on the ground unflipped" ) {
            CHECK_FALSE( smooth_lighting::sprite_stands( f, ortho ) );
        }
        WHEN( "it is flipped vertically" ) {
            f.flip_vertical = true;
            THEN( "opaque half rises above the tile and it stands" ) {
                CHECK( smooth_lighting::sprite_stands( f, ortho ) );
            }
        }
    }
    GIVEN( "64x32 sprite opaque in its bottom left quarter" ) {
        smooth_lighting::sprite_footprint f = footprint( { 64, 32 }, { 0, 16 }, { 16, 32 }, 0 );
        WHEN( "unturned" ) {
            THEN( "it lies on the ground" ) {
                CHECK_FALSE( smooth_lighting::sprite_stands( f, ortho ) );
            }
        }
        WHEN( "turned clockwise" ) {
            f.turn = smooth_lighting::quarter_turn::clockwise;
            THEN( "opaque part turns up above the tile and it stands" ) {
                CHECK( smooth_lighting::sprite_stands( f, ortho ) );
            }
        }
        WHEN( "turned counterclockwise" ) {
            f.turn = smooth_lighting::quarter_turn::counterclockwise;
            THEN( "opaque part turns down and it lies on the ground" ) {
                CHECK_FALSE( smooth_lighting::sprite_stands( f, ortho ) );
            }
        }
        WHEN( "flipped horizontally, turned clockwise" ) {
            f.flip_horizontal = true;
            f.turn = smooth_lighting::quarter_turn::clockwise;
            THEN( "opaque part turns down and it lies on the ground" ) {
                CHECK_FALSE( smooth_lighting::sprite_stands( f, ortho ) );
            }
        }
    }
}

TEST_CASE( "standing_threshold_is_an_eighth_of_a_tile_in_tileset_pixels",
           "[smooth_lighting][tiles]" )
{
    const smooth_lighting::tile_geometry ortho{ 32, 32, false };
    const smooth_lighting::tile_geometry iso{ 32, 32, true };
    CHECK_FALSE( smooth_lighting::sprite_stands( footprint( { 32, 36 }, point::zero, { 32, 36 }, -4 ),
                 ortho ) );
    CHECK( smooth_lighting::sprite_stands( footprint( { 32, 37 }, point::zero, { 32, 37 }, -5 ),
                                           ortho ) );
    // iso ground line is the diamond's top corner, 32 - 16
    CHECK_FALSE( smooth_lighting::sprite_stands( footprint( { 32, 32 }, { 0, 12 }, { 32, 32 }, 0 ),
                 iso ) );
    CHECK( smooth_lighting::sprite_stands( footprint( { 32, 32 }, { 0, 11 }, { 32, 32 }, 0 ),
                                           iso ) );
    GIVEN( "a fully transparent sprite" ) {
        THEN( "it doesn't stand" ) {
            CHECK_FALSE( smooth_lighting::sprite_stands( footprint( { 32, 64 }, point::zero, point::zero, -32 ),
                         ortho ) );
        }
    }
}

TEST_CASE( "lighting_mode_defaults_to_smooth", "[smooth_lighting]" )
{
    CHECK( get_options().get_option( "LIGHTING_MODE" ).getDefaultValue() == "smooth" );
}

TEST_CASE( "lightmap_extent_covers_the_view_range_on_screen", "[smooth_lighting]" )
{
    // avatar at bubble ( 65, 65 ): view range 5 to 125, screen wider than the
    // bubble, as at far zoom
    const point view_min( 5, 5 );
    const point view_max( 125, 125 );
    const smooth_lighting::lightmap_extent extent{
        smooth_lighting::lightmap_fill_area( view_min, view_max, point( -40, -40 ), point( 170, 170 ) ),
        -1, 0 };
    for( const tripoint_bub_ms &p : {
             tripoint_bub_ms( 5, 60, 0 ), tripoint_bub_ms( 125, 60, 0 ),
             tripoint_bub_ms( 60, 5, 0 ), tripoint_bub_ms( 60, 125, -1 )
         } ) {
        CAPTURE( p );
        CHECK( extent.covers( p ) );
    }
    for( const tripoint_bub_ms &p : {
             tripoint_bub_ms( 4, 60, 0 ), tripoint_bub_ms( 126, 60, 0 ),
             tripoint_bub_ms( 60, 4, 0 ), tripoint_bub_ms( 60, 126, 0 ),
             tripoint_bub_ms( -1, 60, 0 ), tripoint_bub_ms( 140, 60, 0 ),
             tripoint_bub_ms( 60, 60, 1 ), tripoint_bub_ms( 60, 60, -2 )
         } ) {
        CAPTURE( p );
        CHECK_FALSE( extent.covers( p ) );
    }
}

TEST_CASE( "lightmap_fill_area_keeps_filter_margin_round_the_screen", "[smooth_lighting]" )
{
    const half_open_rectangle<point> area = smooth_lighting::lightmap_fill_area(
            point( 5, 5 ), point( 125, 125 ), point( 40, 50 ), point( 80, 90 ) );
    CHECK( area.p_min == point( 40 - smooth_lighting::filter_reach,
                                50 - smooth_lighting::filter_reach ) );
    CHECK( area.p_max == point( 80 + smooth_lighting::filter_reach + 1,
                                90 + smooth_lighting::filter_reach + 1 ) );
}

TEST_CASE( "only_scene_light_on_a_covered_cell_takes_the_lit_path", "[smooth_lighting]" )
{
    CHECK( smooth_lighting::lit_path_for( true, true, true ) );
    CHECK_FALSE( smooth_lighting::lit_path_for( true, false, true ) );
    CHECK_FALSE( smooth_lighting::lit_path_for( true, true, false ) );
    CHECK_FALSE( smooth_lighting::lit_path_for( false, true, true ) );
}

TEST_CASE( "lightmap_keys_refill_on_each_texel_input", "[smooth_lighting]" )
{
    smooth_lighting::lightmap_fill_settings settings;
    settings.area = half_open_rectangle<point>( point( 5, 5 ), point( 126, 126 ) );
    settings.vision_threshold = 1.0f;
    settings.tint = true;
    const smooth_lighting::layer_inputs inputs{ 3, 7, 5, 2, false };
    smooth_lighting::lightmap_keys keys;
    keys.begin_frame( settings );
    REQUIRE( keys.needs_fill( 0, inputs ) );
    keys.mark_filled( 0, inputs );
    keys.mark_filled( -1, inputs );
    WHEN( "nothing changes" ) {
        keys.begin_frame( settings );
        THEN( "no level refills" ) {
            CHECK_FALSE( keys.needs_fill( 0, inputs ) );
            CHECK_FALSE( keys.needs_fill( -1, inputs ) );
        }
    }
    WHEN( "recompute visibility without light change" ) {
        THEN( "that level refills" ) {
            CHECK( keys.needs_fill( 0, smooth_lighting::layer_inputs{ 3, 8, 5, 2, false } ) );
            CHECK_FALSE( keys.needs_fill( -1, inputs ) );
        }
    }
    WHEN( "seen cache is rebuilt" ) {
        THEN( "the level refills" ) {
            CHECK( keys.needs_fill( 0, smooth_lighting::layer_inputs{ 3, 7, 6, 2, false } ) );
        }
    }
    WHEN( "light changes" ) {
        THEN( "the level refills" ) {
            CHECK( keys.needs_fill( 0, smooth_lighting::layer_inputs{ 4, 7, 5, 2, false } ) );
        }
    }
    WHEN( "aim cache is dirtied" ) {
        THEN( "the level refills" ) {
            CHECK( keys.needs_fill( 0, smooth_lighting::layer_inputs{ 3, 7, 5, 3, false } ) );
        }
    }
    WHEN( "aim cone starts to apply" ) {
        THEN( "the level refills" ) {
            CHECK( keys.needs_fill( 0, smooth_lighting::layer_inputs{ 3, 7, 5, 2, true } ) );
        }
    }
    WHEN( "tint overlay is toggled" ) {
        settings.tint = false;
        keys.begin_frame( settings );
        THEN( "every level refills" ) {
            CHECK( keys.needs_fill( 0, inputs ) );
            CHECK( keys.needs_fill( -1, inputs ) );
        }
    }
    WHEN( "the screen shows another part of the bubble" ) {
        settings.area = half_open_rectangle<point>( point( 20, 5 ), point( 126, 126 ) );
        keys.begin_frame( settings );
        THEN( "the level refills" ) {
            CHECK( keys.needs_fill( 0, inputs ) );
        }
    }
    WHEN( "vision threshold moves" ) {
        settings.vision_threshold = 2.0f;
        keys.begin_frame( settings );
        THEN( "the level refills" ) {
            CHECK( keys.needs_fill( 0, inputs ) );
        }
    }
    WHEN( "fill switches between filtered and per tile" ) {
        settings.masks = !settings.masks;
        keys.begin_frame( settings );
        THEN( "the level refills" ) {
            CHECK( keys.needs_fill( 0, inputs ) );
        }
    }
    WHEN( "one level's upload failed" ) {
        keys.forget( 0 );
        THEN( "only that level refills" ) {
            CHECK( keys.needs_fill( 0, inputs ) );
            CHECK_FALSE( keys.needs_fill( -1, inputs ) );
        }
    }
}

TEST_CASE( "visibility_recompute_bumps_the_visibility_generation_only",
           "[smooth_lighting][vision]" )
{
    const tripoint_bub_ms center( 60, 60, 0 );
    build_dark_room( center, 6 );
    settle_caches( 0 );
    map &here = get_map();
    const uint64_t light_before = here.access_cache( 0 ).lightmap_generation;
    const uint64_t visibility_before = here.access_cache( 0 ).visibility_generation;
    here.invalidate_visibility_cache();
    here.update_visibility_cache( 0 );
    CHECK( here.access_cache( 0 ).visibility_generation != visibility_before );
    CHECK( here.access_cache( 0 ).lightmap_generation == light_before );
}

TEST_CASE( "avatar_move_on_foot_bumps_the_seen_generation", "[smooth_lighting][vision]" )
{
    const tripoint_bub_ms center( 60, 60, 0 );
    build_dark_room( center, 6 );
    settle_caches( 0 );
    map &here = get_map();
    // on foot: build_seen_cache returns before its vehicle mirror pass
    REQUIRE_FALSE( here.veh_at( center ) );
    const uint64_t seen_before = here.seen_generation();
    g->place_player( center + tripoint::east );
    here.build_map_cache( 0 );
    CHECK( here.seen_generation() != seen_before );
}

TEST_CASE( "cache_generations_never_repeat_across_map_rebuilds", "[smooth_lighting][vision]" )
{
    const tripoint_bub_ms center( 60, 60, 0 );
    build_dark_room( center, 6 );
    settle_caches( 0 );
    const level_cache &first = get_map().access_cache( 0 );
    const uint64_t first_light = first.lightmap_generation;
    const uint64_t first_visibility = first.visibility_generation;
    const uint64_t first_seen = get_map().seen_generation();
    WHEN( "map is cleared and the same room built again" ) {
        build_dark_room( center, 6 );
        settle_caches( 0 );
        const level_cache &again = get_map().access_cache( 0 );
        THEN( "no generation comes back with an earlier value" ) {
            CHECK( again.lightmap_generation > first_light );
            CHECK( again.visibility_generation > first_visibility );
            CHECK( get_map().seen_generation() > first_seen );
        }
    }
}

TEST_CASE( "dirtying_the_aim_cache_bumps_the_aim_generation", "[smooth_lighting]" )
{
    avatar &u = get_avatar();
    const uint64_t before = u.aim_generation();
    u.mark_aim_cache_dirty();
    CHECK( u.aim_generation() > before );
}

TEST_CASE( "light_texel_keeps_light_apart_from_hue", "[smooth_lighting]" )
{
    const smooth_lighting::light_cell half{ 0.5f, true };
    const smooth_lighting::lightmap_texel red =
        smooth_lighting::encode_light_texel( half, { 1.0f, 0.2f, 0.2f }, false );
    const smooth_lighting::lightmap_texel green =
        smooth_lighting::encode_light_texel( half, { 0.2f, 1.0f, 0.2f }, false );
    THEN( "light sits in its own channel whatever the hue" ) {
        CHECK( red.r == green.r );
        CHECK( red.r == 128 );
    }
    THEN( "hue shows in the chroma shares" ) {
        CHECK( red.g > green.g );
        CHECK( green.b > red.b );
        CHECK( red.g + red.b <= 255 );
    }
    THEN( "the detail flag is set and the barrier flag is not" ) {
        CHECK( ( red.a & smooth_lighting::texel_detail ) != 0 );
        CHECK( ( red.a & smooth_lighting::texel_barrier ) == 0 );
    }
    WHEN( "out of sight cell is a barrier" ) {
        const smooth_lighting::lightmap_texel wall = smooth_lighting::encode_light_texel(
                    smooth_lighting::light_cell(), { 1.0f, 1.0f, 1.0f }, true );
        THEN( "it has the barrier flag and no light" ) {
            CHECK( wall.r == 0 );
            CHECK( wall.a == smooth_lighting::texel_barrier );
        }
    }
}

TEST_CASE( "reach_texel_round_trips_every_mask_bit", "[smooth_lighting]" )
{
    for( const uint32_t mask : {
             0u, 1u, 0x1000000u, 0x1ffffffu, 0x0aaaaaau
         } ) {
        CAPTURE( mask );
        CHECK( smooth_lighting::texel_reach( smooth_lighting::encode_reach_texel( mask ) ) == mask );
    }
}

TEST_CASE( "colored_light_fades_to_white_as_it_dims", "[smooth_lighting][light_color]" )
{
    light_color_rgb red;
    red.r = 12.0f;
    red.g = 0.0f;
    red.b = 0.0f;
    const std::array<float, 3> bright = smooth_lighting::illumination_hue( red, 12.0f );
    CHECK( bright[0] > bright[1] );
    light_color_rgb dim = red;
    dim.r = LIGHT_AMBIENT_LOW;
    const std::array<float, 3> faded = smooth_lighting::illumination_hue( dim, LIGHT_AMBIENT_LOW );
    CHECK( faded[0] == Approx( 1.0f ) );
    CHECK( faded[1] == Approx( 1.0f ) );
    CHECK( faded[2] == Approx( 1.0f ) );
}

TEST_CASE( "colored_light_fades_out_at_its_reach_under_other_light",
           "[smooth_lighting][light_color]" )
{
    // magenta at its last tile, where white light still lights the floor
    light_color_rgb magenta;
    magenta.r = LIGHT_AMBIENT_LOW;
    magenta.g = 0.0f;
    magenta.b = LIGHT_AMBIENT_LOW;
    const float white_light = 6.0f;
    const std::array<float, 3> edge = smooth_lighting::illumination_hue( magenta, white_light );
    for( const float h : edge ) {
        CHECK( h == Approx( 1.0f ).margin( 0.01 ) );
    }
    WHEN( "magenta light is strong" ) {
        magenta.r = magenta.b = LIGHT_AMBIENT_LIT;
        const std::array<float, 3> strong = smooth_lighting::illumination_hue( magenta, white_light );
        THEN( "it still tints" ) {
            CHECK( strong[1] < strong[0] - 0.1f );
        }
    }
}

// lit floor round ( 60, 60 ) at noon, avatar on it
static void build_lit_ground()
{
    reset_avatar_and_map( tripoint_bub_ms( 60, 60, 0 ) );
    calendar::turn = calendar::turn_zero + 12_hours;
    g->reset_light_level();
    map &here = get_map();
    for( int x = 50; x <= 70; ++x ) {
        for( int y = 50; y <= 70; ++y ) {
            here.ter_set( tripoint_bub_ms( x, y, 0 ), ter_t_floor.id() );
        }
    }
}

static std::vector<smooth_lighting::lightmap_texel> encode_lit_ground( const bool masks )
{
    settle_caches( 0 );
    const map &here = get_map();
    smooth_lighting::lightmap_fill_settings settings;
    settings.area = half_open_rectangle<point>( point( 50, 50 ), point( 71, 71 ) );
    settings.vision_threshold = here.get_visibility_variables_cache().vision_threshold;
    settings.tint = true;
    settings.masks = masks;
    std::vector<smooth_lighting::lightmap_texel> layer;
    REQUIRE( smooth_lighting::encode_lightmap_layer( here, 0, settings, layer ) );
    return layer;
}

static uint32_t reach_at( const point &p )
{
    const std::vector<smooth_lighting::lightmap_texel> layer = encode_lit_ground( true );
    // only a seen cell has a mask
    CAPTURE( get_map().access_cache( 0 ).visibility_cache[p.x][p.y] );
    REQUIRE( ( layer[smooth_lighting::light_index( p )].a & smooth_lighting::texel_detail ) != 0 );
    return smooth_lighting::texel_reach( layer[smooth_lighting::reach_index( p )] );
}

TEST_CASE( "light_filter_reach_stops_at_barriers", "[smooth_lighting][vision]" )
{
    scoped_weather_override weather( WEATHER_CLEAR );
    build_lit_ground();
    map &here = get_map();
    GIVEN( "thin wall east of the cell" ) {
        for( int y = 55; y <= 65; ++y ) {
            here.ter_set( tripoint_bub_ms( 61, y, 0 ), ter_t_brick_wall.id() );
        }
        WHEN( "the wall is whole" ) {
            const uint32_t mask = reach_at( point( 60, 60 ) );
            THEN( "light reaches west but not into or past the wall" ) {
                CHECK( ( mask & smooth_lighting::reach_bit( point::east ) ) == 0 );
                CHECK( ( mask & smooth_lighting::reach_bit( point( 2, 0 ) ) ) == 0 );
                CHECK( ( mask & smooth_lighting::reach_bit( point::west ) ) != 0 );
            }
        }
        WHEN( "wall has an open door" ) {
            here.ter_set( tripoint_bub_ms( 61, 60, 0 ), ter_t_door_o.id() );
            THEN( "light reaches through the doorway" ) {
                CHECK( ( reach_at( point( 60, 60 ) ) & smooth_lighting::reach_bit( point( 2, 0 ) ) ) != 0 );
            }
        }
        WHEN( "door closed" ) {
            here.ter_set( tripoint_bub_ms( 61, 60, 0 ), ter_t_door_c.id() );
            THEN( "light stays on this side" ) {
                CHECK( ( reach_at( point( 60, 60 ) ) & smooth_lighting::reach_bit( point( 2, 0 ) ) ) == 0 );
            }
        }
    }
    GIVEN( "outside corner of walls east and south" ) {
        here.ter_set( tripoint_bub_ms( 61, 60, 0 ), ter_t_brick_wall.id() );
        here.ter_set( tripoint_bub_ms( 60, 61, 0 ), ter_t_brick_wall.id() );
        THEN( "light doesn't wrap to the diagonal cell" ) {
            CHECK( ( reach_at( point( 60, 60 ) ) & smooth_lighting::reach_bit( point::south_east ) ) == 0 );
        }
    }
    GIVEN( "a corridor one tile wide running east" ) {
        for( int x = 55; x <= 65; ++x ) {
            here.ter_set( tripoint_bub_ms( x, 59, 0 ), ter_t_brick_wall.id() );
            here.ter_set( tripoint_bub_ms( x, 61, 0 ), ter_t_brick_wall.id() );
        }
        const uint32_t mask = reach_at( point( 60, 60 ) );
        THEN( "light runs along it, not past its walls" ) {
            CHECK( ( mask & smooth_lighting::reach_bit( point( 2, 0 ) ) ) != 0 );
            CHECK( ( mask & smooth_lighting::reach_bit( point( 0, 2 ) ) ) == 0 );
            CHECK( ( mask & smooth_lighting::reach_bit( point( 0, -2 ) ) ) == 0 );
        }
    }
}

TEST_CASE( "per_tile_fill_builds_no_reach_masks", "[smooth_lighting][vision]" )
{
    scoped_weather_override weather( WEATHER_CLEAR );
    build_lit_ground();
    get_map().ter_set( tripoint_bub_ms( 61, 60, 0 ), ter_t_brick_wall.id() );
    const std::vector<smooth_lighting::lightmap_texel> layer = encode_lit_ground( false );
    for( int y = 0; y < MAPSIZE_Y; ++y ) {
        for( int x = 0; x < MAPSIZE_X; ++x ) {
            if( !( layer[smooth_lighting::reach_index( point( x,
                                                       y ) )] == smooth_lighting::lightmap_texel() ) ) {
                CAPTURE( x, y );
                FAIL( "per tile fill wrote a reach mask" );
            }
        }
    }
}

namespace
{
struct test_lightmap {
    std::vector<smooth_lighting::lightmap_texel> texels;
    smooth_lighting::lightmap_view view;
};
} // namespace

static constexpr int test_columns = 8;
static constexpr int test_rows = 8;

// middle of three z levels from text rows: 'o' lit floor, 'T' floor at
// t_light with red hue, '#' seen wall, 'X' both, ' ' floor out of sight
static test_lightmap build_test_lightmap( const std::vector<std::string> &rows,
        const float floor_light, const float t_light )
{
    test_lightmap m;
    const int width = 2 * test_columns;
    m.texels.assign( static_cast<size_t>( width ) * test_rows * 3, smooth_lighting::lightmap_texel() );
    const int top = test_rows;
    for( size_t y = 0; y < rows.size(); ++y ) {
        for( size_t x = 0; x < rows[y].size(); ++x ) {
            const char c = rows[y][x];
            const bool hot = c == 'T' || c == 'X';
            const smooth_lighting::light_cell cell{ hot ? t_light : floor_light, c != ' ' };
            const std::array<float, 3> hue = hot ? std::array<float, 3> { 1.0f, 0.2f, 0.2f } :
                                             std::array<float, 3> { 1.0f, 1.0f, 1.0f };
            m.texels[( top + y ) * width + x] = smooth_lighting::encode_light_texel( cell, hue,
                                                c == '#' || c == 'X' );
        }
    }
    const smooth_lighting::barrier_grid grid{ m.texels.data() + static_cast<size_t>( top ) *width,
            width, test_columns, test_rows };
    for( int y = 0; y < test_rows; ++y ) {
        for( int x = 0; x < test_columns; ++x ) {
            m.texels[( top + y ) * width + test_columns + x] =
                smooth_lighting::encode_reach_texel( smooth_lighting::reach_mask( grid, point( x, y ) ) );
        }
    }
    m.view = { m.texels.data(), width, test_rows * 3, test_rows, test_columns };
    return m;
}

// a ground pixel at map position ( x, y ) of the middle level, own cell `cell`
static smooth_lighting::lit_coords ground_at( const point &cell, const float x, const float y )
{
    return { x, test_rows + y, static_cast<float>( cell.x ), static_cast<float>( test_rows + cell.y ) };
}

TEST_CASE( "filtered_light_is_continuous_across_floor_cells_near_a_wall", "[smooth_lighting]" )
{
    // A at ( 1, 3 ) and B at ( 2, 3 ); A cannot reach T by a path of single
    // steps toward it, B can
    const test_lightmap m = build_test_lightmap( {
        "oooooooo",
        "oToooooo",
        "o#oooooo",
        "oooooooo",
    }, 0.2f, 1.0f );
    const uint32_t mask_a = smooth_lighting::texel_reach( m.texels[( test_rows + 3 ) * 2 * test_columns
                                              +
                                              test_columns + 1] );
    const uint32_t mask_b = smooth_lighting::texel_reach( m.texels[( test_rows + 3 ) * 2 * test_columns
                                              +
                                              test_columns + 2] );
    REQUIRE( ( mask_a & smooth_lighting::reach_bit( point( 0, -2 ) ) ) == 0 );
    REQUIRE( ( mask_b & smooth_lighting::reach_bit( point( -1, -2 ) ) ) != 0 );
    const smooth_lighting::sample_params filtered{ false, false };
    for( const float y : {
             3.05f, 3.3f, 3.5f, 3.7f, 3.95f
         } ) {
        CAPTURE( y );
        const smooth_lighting::lit_sample from_a =
            smooth_lighting::reference_sample( m.view, filtered, ground_at( point( 1, 3 ), 2.0f, y ) );
        const smooth_lighting::lit_sample from_b =
            smooth_lighting::reference_sample( m.view, filtered, ground_at( point( 2, 3 ), 2.0f, y ) );
        CHECK( from_a.light == Approx( from_b.light ).margin( 1e-5 ) );
        CHECK( from_a.visible == Approx( from_b.visible ).margin( 1e-5 ) );
        for( int k = 0; k < 3; ++k ) {
            CHECK( from_a.hue[k] == Approx( from_b.hue[k] ).margin( 1e-5 ) );
        }
    }
}

TEST_CASE( "filtered_light_never_reads_outside_its_level", "[smooth_lighting]" )
{
    test_lightmap m = build_test_lightmap( std::vector<std::string>( test_rows, "oooooooo" ), 0.2f,
                                           1.0f );
    // full red light on the levels above and below, mask texels beside; a
    // fetch outside this level would read either as light
    const smooth_lighting::lightmap_texel bright = smooth_lighting::encode_light_texel( { 1.0f, true }, {
        1.0f, 0.2f, 0.2f
    }, false );
    const int width = 2 * test_columns;
    for( int y = 0; y < test_rows; ++y ) {
        for( int x = 0; x < test_columns; ++x ) {
            m.texels[y * width + x] = bright;
            m.texels[( 2 * test_rows + y ) * width + x] = bright;
        }
    }
    const smooth_lighting::sample_params filtered{ false, false };
    for( const point &cell : {
             point::zero, point( 7, 0 ), point::south, point( 7, 1 ), point( 0, 7 ), point( 7, 7 )
         } ) {
        for( const float d : {
                 0.02f, 0.98f
             } ) {
            CAPTURE( cell, d );
            const smooth_lighting::lit_sample s = smooth_lighting::reference_sample( m.view, filtered,
                                                  ground_at( cell, cell.x + d, cell.y + d ) );
            CHECK( s.light == Approx( 0.2f ).margin( 0.01 ) );
            for( const float h : s.hue ) {
                CHECK( h == Approx( 1.0f ).margin( 0.01 ) );
            }
        }
    }
}

TEST_CASE( "seen_pixels_always_carry_admitted_light", "[smooth_lighting]" )
{
    const test_lightmap m = build_test_lightmap( {
        "ooo#    ",
        "oTo#    ",
        "ooo#    ",
    }, 0.4f, 1.0f );
    const smooth_lighting::sample_params filtered{ false, false };
    for( int cy = 0; cy < 3; ++cy ) {
        for( int cx = 0; cx < test_columns; ++cx ) {
            for( const float d : {
                     0.01f, 0.5f, 0.99f
                 } ) {
                const smooth_lighting::lit_sample s = smooth_lighting::reference_sample( m.view, filtered,
                                                      ground_at( point( cx, cy ), cx + d, cy + d ) );
                CAPTURE( cx, cy, d, s.visible, s.weight );
                if( s.visible > 0.0f ) {
                    CHECK( s.weight > 0.0f );
                }
            }
        }
    }
}

TEST_CASE( "floor_pixels_ignore_the_wall_beside_them", "[smooth_lighting]" )
{
    // seen wall in bright red light beside dim white floor
    const test_lightmap lit_wall = build_test_lightmap( {
        "ooXooooo",
    }, 0.3f, 1.0f );
    const int width = 2 * test_columns;
    REQUIRE_FALSE( lit_wall.texels[test_rows * width + 2] == lit_wall.texels[test_rows * width + 1] );
    const smooth_lighting::sample_params filtered{ false, false };
    const smooth_lighting::lit_sample s = smooth_lighting::reference_sample( lit_wall.view, filtered,
                                          ground_at( point::east, 1.98f, 0.5f ) );
    CHECK( s.light == Approx( 0.3f ).margin( 0.01 ) );
    for( const float h : s.hue ) {
        CHECK( h == Approx( 1.0f ).margin( 0.01 ) );
    }
    CHECK( s.visible == Approx( 1.0f ) );
}

TEST_CASE( "filtered_light_does_not_cross_a_blocked_diagonal", "[smooth_lighting]" )
{
    // A at ( 0, 0 ) and T at ( 1, 1 ) touch only at a corner walled on both sides
    const test_lightmap m = build_test_lightmap( {
        "o#",
        "#T",
    }, 0.2f, 1.0f );
    const uint32_t mask_a = smooth_lighting::texel_reach( m.texels[test_rows * 2 * test_columns +
                                      test_columns] );
    REQUIRE( ( mask_a & smooth_lighting::reach_bit( point::south_east ) ) == 0 );
    const smooth_lighting::sample_params filtered{ false, false };
    for( const float d : {
             0.6f, 0.8f, 0.9f, 0.99f
         } ) {
        CAPTURE( d );
        const smooth_lighting::lit_sample s = smooth_lighting::reference_sample( m.view, filtered,
                                              ground_at( point::zero, d, d ) );
        CHECK( s.light == Approx( 0.2f ).margin( 1e-4 ) );
        for( const float h : s.hue ) {
            CHECK( h == Approx( 1.0f ).margin( 1e-4 ) );
        }
        CHECK( s.visible == Approx( 1.0f ) );
    }
}

TEST_CASE( "colored_light_fades_in_without_a_jump_as_its_weight_grows", "[smooth_lighting]" )
{
    // a dim red source among seen cells whose light is zero
    const test_lightmap m = build_test_lightmap( {
        "oooooooo",
        "oToooooo",
        "oooooooo",
    }, 0.0f, 0.05f );
    const smooth_lighting::sample_params filtered{ false, false };
    // light and hue must change continuously
    std::array<float, 4> last = {};
    bool first = true;
    for( int i = 0; i < 900; ++i ) {
        const float x = 1.5f + 0.005f * static_cast<float>( i );
        const smooth_lighting::lit_sample s = smooth_lighting::reference_sample( m.view, filtered,
                                              ground_at( point( static_cast<int>( x ), 1 ), x, 1.5f ) );
        const std::array<float, 4> got = { s.light, s.hue[0], s.hue[1], s.hue[2] };
        if( !first ) {
            for( int k = 0; k < 4; ++k ) {
                CAPTURE( x, k, last[k], got[k] );
                CHECK( std::abs( got[k] - last[k] ) < 0.02f );
            }
        }
        last = got;
        first = false;
    }
}

namespace
{
struct filter_fixture {
    std::string name;
    test_lightmap map;
};
} // namespace

// light maps with every kind of filter edge: seam beside a wall, walled
// diagonal, sight edge with walls, steep sight ratio, colored fade from
// darkness, lit wall beside dim floor
static std::vector<filter_fixture> filter_fixtures()
{
    std::vector<filter_fixture> out;
    out.push_back( { "seam", build_test_lightmap( {
            "oooooooo",
            "oToooooo",
            "o#oooooo",
            "oooooooo",
        }, 0.2f, 1.0f )
    } );
    out.push_back( { "walled diagonal", build_test_lightmap( {
            "o#oooooo",
            "#Toooooo",
            "oooooooo",
        }, 0.2f, 1.0f )
    } );
    out.push_back( { "sight edge", build_test_lightmap( {
            "ooo#    ",
            "oTo#    ",
            "ooo#    ",
            "oooo    ",
        }, 0.4f, 1.0f )
    } );
    out.push_back( { "steep sight ratio", build_test_lightmap( {
            "o o o o ",
            "#o#o#o#o",
            "To To To",
            " o o o o",
        }, 0.6f, 0.9f )
    } );
    out.push_back( { "colored fade", build_test_lightmap( {
            "oooooooo",
            "oToooooo",
            "oooooooo",
        }, 0.0f, 0.05f )
    } );
    out.push_back( { "lit wall", build_test_lightmap( {
            "ooXooooo",
            "ooXooooo",
            "oooooooo",
        }, 0.3f, 1.0f )
    } );
    return out;
}

// layout of the middle level's whole test area
static smooth_lighting::prefilter_layout test_layout( const int grid )
{
    smooth_lighting::prefilter_layout layout;
    layout.area = half_open_rectangle<point>( point::zero, point( test_columns, test_rows ) );
    layout.first_level = 1;
    layout.levels = 1;
    layout.grid = grid;
    return layout;
}

// layout the game would derive for the middle level of `m`: its seen cells
// grown by one
static smooth_lighting::prefilter_layout seen_layout( const test_lightmap &m, const int grid )
{
    const half_open_rectangle<point> whole( point::zero, point( test_columns, test_rows ) );
    smooth_lighting::prefilter_layout layout = smooth_lighting::prefilter_layout_for(
                smooth_lighting::seen_box( m.texels.data() + static_cast<size_t>( test_rows ) * 2 * test_columns,
                                           2 * test_columns, whole ), whole, 1, 1 );
    layout.grid = grid;
    return layout;
}

TEST_CASE( "point_filter_is_the_finished_raw_filter", "[smooth_lighting]" )
{
    const smooth_lighting::sample_params filtered{ false, false };
    for( const filter_fixture &f : filter_fixtures() ) {
        for( int cy = 0; cy < 4; ++cy ) {
            for( int cx = 0; cx < test_columns; ++cx ) {
                // steps exact in binary, so the reference's local position is d
                for( const float d : {
                         0.0f, 0.25f, 0.5f, 0.75f, 1.0f
                     } ) {
                    CAPTURE( f.name, cx, cy, d );
                    const smooth_lighting::lit_sample point_sample = smooth_lighting::reference_sample(
                                f.map.view, filtered, ground_at( point( cx, cy ), cx + d, cy + 1.0f - d ) );
                    const smooth_lighting::lit_sample finished = smooth_lighting::finish_filter(
                                smooth_lighting::reference_filter( f.map.view, point( cx, test_rows + cy ), 1, d,
                                        1.0f - d ) );
                    CHECK( finished.light == point_sample.light );
                    CHECK( finished.hue == point_sample.hue );
                    CHECK( finished.in_sight == point_sample.in_sight );
                    CHECK( finished.visible == point_sample.visible );
                    CHECK( finished.weight == point_sample.weight );
                }
            }
        }
    }
}

TEST_CASE( "prefiltered_light_matches_the_filter_at_grid_points", "[smooth_lighting]" )
{
    const int grid = smooth_lighting::prefilter_grid;
    const smooth_lighting::sample_params filtered{ false, false };
    for( const filter_fixture &f : filter_fixtures() ) {
        const smooth_lighting::prefilter_table table = smooth_lighting::build_prefilter_table( f.map.view,
                test_layout( grid ) );
        for( const point &cell : {
                 point::south_east, point( 2, 2 ), point( 3, 1 ), point( 7, 3 )
             } ) {
            for( int j = 0; j <= grid; ++j ) {
                for( int i = 0; i <= grid; ++i ) {
                    const float lx = static_cast<float>( i ) / grid;
                    const float ly = static_cast<float>( j ) / grid;
                    CAPTURE( f.name, cell, i, j );
                    const smooth_lighting::filter_result raw = smooth_lighting::reference_filter( f.map.view,
                            cell + point( 0, test_rows ), 1, lx, ly );
                    smooth_lighting::filter_result stored = raw;
                    stored.light = smooth_lighting::half_round( raw.light );
                    stored.chroma = { smooth_lighting::half_round( raw.chroma[0] ),
                                      smooth_lighting::half_round( raw.chroma[1] ),
                                      std::max( 1.0f - smooth_lighting::half_round( raw.chroma[0] ) -
                                                smooth_lighting::half_round( raw.chroma[1] ), 0.0f )
                                    };
                    stored.in_sight = smooth_lighting::half_round( raw.in_sight );
                    const smooth_lighting::lit_sample want = smooth_lighting::finish_filter( stored );
                    const smooth_lighting::lit_sample got = smooth_lighting::reference_prefiltered_sample( table,
                                                            f.map.view, filtered, ground_at( cell, cell.x + lx, cell.y + ly ) );
                    CHECK( got.light == Approx( want.light ).margin( 1e-5 ) );
                    CHECK( got.in_sight == Approx( want.in_sight ).margin( 1e-5 ) );
                    for( int k = 0; k < 3; ++k ) {
                        CHECK( got.hue[k] == Approx( want.hue[k] ).margin( 1e-5 ) );
                    }
                }
            }
        }
    }
}

namespace
{
struct look_case {
    std::string name;
    smooth_lighting::look_params look;
    bool night = false;
};
} // namespace

static std::vector<look_case> look_cases()
{
    smooth_lighting::look_params plain;
    smooth_lighting::look_params blended;
    blended.blend_memory = true;
    blended.memory_look = 1;
    smooth_lighting::look_params custom;
    custom.blend_memory = true;
    custom.memory_look = smooth_lighting::custom_look;
    custom.custom_dark = { 0.26f, 0.09f, 0.07f };
    custom.custom_light = { 0.95f, 0.86f, 0.64f };
    custom.custom_gamma = 1.6f;
    return {
        { "plain", plain, false },
        { "memory preset", blended, false },
        { "custom memory", custom, false },
        { "night", plain, true },
        { "night, memory preset", blended, true },
    };
}

// largest per channel difference, in 0..255 steps, between the prefilter and
// point filter over every fixture, look, sprite color and own or standing
// pixel; `quantize_weights` rounds the bilinear weights to 1/16 as the coarsest
// hardware filter may
static float max_prefilter_error( const int grid, const bool quantize_weights )
{
    static const std::array<std::array<float, 3>, 4> colors = { {
            { 0.5f, 0.5f, 0.5f }, { 220.0f / 255, 40.0f / 255, 30.0f / 255 },
            { 60.0f / 255, 40.0f / 255, 20.0f / 255 }, { 1.0f, 1.0f, 1.0f }
        }
    };
    const std::vector<look_case> looks = look_cases();
    float worst = 0.0f;
    for( const filter_fixture &f : filter_fixtures() ) {
        smooth_lighting::prefilter_table table = smooth_lighting::build_prefilter_table( f.map.view,
                seen_layout( f.map, grid ) );
        table.quantize_weights = quantize_weights;
        for( const smooth_lighting::sample_params &params : {
                 smooth_lighting::sample_params{ false, false }, smooth_lighting::sample_params{ false, true }
             } ) {
            for( const bool standing : {
                     false, true
                 } ) {
                // every row, so cells outside the seen box take the dark
                // constant
                for( int cy = 0; cy < test_rows; ++cy ) {
                    for( int cx = 0; cx < test_columns; ++cx ) {
                        for( int j = 0; j <= 15; ++j ) {
                            for( int i = 0; i <= 15; ++i ) {
                                smooth_lighting::lit_coords at = ground_at( point( cx, cy ), cx + i / 15.0f,
                                                                 cy + j / 15.0f );
                                if( standing ) {
                                    at.column += smooth_lighting::standing_marker;
                                }
                                const smooth_lighting::lit_sample want = smooth_lighting::reference_sample(
                                            f.map.view, params, at );
                                const smooth_lighting::lit_sample got =
                                    smooth_lighting::reference_prefiltered_sample( table, f.map.view, params, at );
                                for( const look_case &l : looks ) {
                                    for( const std::array<float, 3> &rgb : colors ) {
                                        const std::array<float, 3> a = l.night ?
                                                                       smooth_lighting::reference_night_rgb( l.look, rgb, got ) :
                                                                       smooth_lighting::reference_lit_rgb( l.look, rgb, got );
                                        const std::array<float, 3> b = l.night ?
                                                                       smooth_lighting::reference_night_rgb( l.look, rgb, want ) :
                                                                       smooth_lighting::reference_lit_rgb( l.look, rgb, want );
                                        for( int k = 0; k < 3; ++k ) {
                                            worst = std::max( worst, 255.0f * std::abs( a[k] - b[k] ) );
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    return worst;
}

TEST_CASE( "prefiltered_light_stays_within_its_error_budget", "[smooth_lighting]" )
{
    const int grid = smooth_lighting::prefilter_grid;
    const float error = max_prefilter_error( grid, false );
    CAPTURE( grid, error );
    CHECK( error <= smooth_lighting::prefilter_error_budget );
    if( grid > 2 ) {
        // coarsest grid that fits the budget
        const float coarser = max_prefilter_error( grid - 2, false );
        CAPTURE( coarser );
        CHECK( coarser > smooth_lighting::prefilter_error_budget );
    }
}

// not run by default: prints the error of every candidate grid, exact and
// with the coarsest hardware filter weights
TEST_CASE( "prefilter_error_per_grid", "[.][smooth_lighting_report]" )
{
    for( const int grid : {
             2, 4, 6, 8, 10, 12, 16
         } ) {
        WARN( "grid " << grid << ": exact " << max_prefilter_error( grid, false ) <<
              ", 1/16 weights " << max_prefilter_error( grid, true ) );
    }
}

TEST_CASE( "light_is_dark_two_cells_from_anything_seen", "[smooth_lighting]" )
{
    for( const filter_fixture &f : filter_fixtures() ) {
        const smooth_lighting::lightmap_texel *level = f.map.texels.data() + static_cast<size_t>
                ( test_rows ) * 2 * test_columns;
        const auto seen = [&]( const point & p ) {
            return ( level[p.y * 2 * test_columns + p.x].a & smooth_lighting::texel_detail ) != 0;
        };
        for( int cy = 0; cy < test_rows; ++cy ) {
            for( int cx = 0; cx < test_columns; ++cx ) {
                bool is_near = false;
                for( int y = std::max( cy - 1, 0 ); y <= std::min( cy + 1, test_rows - 1 ); ++y ) {
                    for( int x = std::max( cx - 1, 0 ); x <= std::min( cx + 1, test_columns - 1 ); ++x ) {
                        is_near = is_near || seen( point( x, y ) );
                    }
                }
                if( is_near ) {
                    continue;
                }
                for( const float lx : {
                         0.0f, 0.5f, 1.0f
                     } ) {
                    for( const float ly : {
                             0.0f, 0.5f, 1.0f
                         } ) {
                        CAPTURE( f.name, cx, cy, lx, ly );
                        const smooth_lighting::filter_result r = smooth_lighting::reference_filter( f.map.view,
                                point( cx, test_rows + cy ), 1, lx, ly );
                        const smooth_lighting::filter_result dark;
                        CHECK( r.light == dark.light );
                        CHECK( r.in_sight == dark.in_sight );
                        CHECK( r.chroma == dark.chroma );
                    }
                }
            }
        }
    }
}

TEST_CASE( "prefilter_layout_covers_seen_cells_grown_by_one", "[smooth_lighting]" )
{
    const half_open_rectangle<point> clip( point( 2, 2 ), point( 12, 10 ) );
    GIVEN( "seen box inside the clip" ) {
        const smooth_lighting::prefilter_layout l = smooth_lighting::prefilter_layout_for(
                    half_open_rectangle<point>( point( 4, 5 ), point( 6, 7 ) ), clip, 3, 2 );
        THEN( "it grows by one cell each side" ) {
            CHECK( l.area.p_min == point( 3, 4 ) );
            CHECK( l.area.p_max == point( 7, 8 ) );
            CHECK( l.first_level == 3 );
            CHECK( l.levels == 2 );
        }
    }
    GIVEN( "seen box at the clip's edge" ) {
        const smooth_lighting::prefilter_layout l = smooth_lighting::prefilter_layout_for(
                    half_open_rectangle<point>( point( 2, 2 ), point( 12, 10 ) ), clip, 0, 1 );
        THEN( "growth stops at the clip" ) {
            CHECK( l.area.p_min == clip.p_min );
            CHECK( l.area.p_max == clip.p_max );
        }
    }
    GIVEN( "nothing seen" ) {
        const smooth_lighting::prefilter_layout l = smooth_lighting::prefilter_layout_for( std::nullopt,
                clip, 0, 1 );
        THEN( "layout is empty, fits any GPU, holds no cell" ) {
            CHECK( l.size() == point::zero );
            CHECK( smooth_lighting::prefilter_fits( l, 1, true ) );
            CHECK_FALSE( l.holds( point( 5, 5 ), 0 ) );
        }
    }
    GIVEN( "level strip with two seen cells" ) {
        std::vector<smooth_lighting::lightmap_texel> strip( static_cast<size_t>( 2 * test_columns ) *
                test_rows );
        strip[2 * 2 * test_columns + 1].a = smooth_lighting::texel_detail;
        strip[5 * 2 * test_columns + 6].a = smooth_lighting::texel_detail | smooth_lighting::texel_barrier;
        const std::optional<half_open_rectangle<point>> box = smooth_lighting::seen_box( strip.data(),
                2 * test_columns, half_open_rectangle<point>( point::zero, point( test_columns, test_rows ) ) );
        THEN( "the seen box spans them" ) {
            REQUIRE( box );
            CHECK( box->p_min == point( 1, 2 ) );
            CHECK( box->p_max == point( 7, 6 ) );
        }
    }
}

TEST_CASE( "prefilter_layout_puts_each_level_in_its_own_block", "[smooth_lighting]" )
{
    smooth_lighting::prefilter_layout layout;
    layout.area = half_open_rectangle<point>( point( 2, 1 ), point( 5, 4 ) );
    layout.first_level = 3;
    layout.levels = 2;
    layout.grid = 4;
    REQUIRE( layout.size() == point( 3 * 5, 3 * 5 * 2 ) );
    std::vector<int> owner( static_cast<size_t>( layout.size().x ) * layout.size().y, -1 );
    int block = 0;
    for( int level = 3; level < 5; ++level ) {
        for( int y = 1; y < 4; ++y ) {
            for( int x = 2; x < 5; ++x ) {
                const point origin = layout.block_origin( point( x, y ), level );
                CAPTURE( x, y, level, origin );
                for( int j = 0; j <= layout.grid; ++j ) {
                    for( int i = 0; i <= layout.grid; ++i ) {
                        const point t = origin + point( i, j );
                        REQUIRE( t.x >= 0 );
                        REQUIRE( t.y >= 0 );
                        REQUIRE( t.x < layout.size().x );
                        REQUIRE( t.y < layout.size().y );
                        int &o = owner[static_cast<size_t>( t.y ) * layout.size().x + t.x];
                        CHECK( o == -1 );
                        o = block;
                    }
                }
                ++block;
            }
        }
    }
    CHECK( std::count( owner.begin(), owner.end(), -1 ) == 0 );
}

TEST_CASE( "prefilter_fits_only_a_supported_format_within_the_size_limit", "[smooth_lighting]" )
{
    smooth_lighting::prefilter_layout layout;
    layout.area = half_open_rectangle<point>( point::zero, point( 10, 4 ) );
    layout.first_level = 0;
    layout.levels = 2;
    layout.grid = 4;
    REQUIRE( layout.size() == point( 50, 40 ) );
    CHECK( smooth_lighting::prefilter_fits( layout, 50, true ) );
    CHECK_FALSE( smooth_lighting::prefilter_fits( layout, 50, false ) );
    CHECK_FALSE( smooth_lighting::prefilter_fits( layout, 49, true ) );
    layout.levels = 3;
    CHECK_FALSE( smooth_lighting::prefilter_fits( layout, 50, true ) );
}

TEST_CASE( "prefilter_manual_lookup_stays_inside_the_last_block", "[smooth_lighting]" )
{
    smooth_lighting::prefilter_layout layout;
    layout.area = half_open_rectangle<point>( point( 3, 2 ), point( 6, 5 ) );
    layout.first_level = 0;
    layout.levels = 2;
    layout.grid = 4;
    const point last_cell( 5, 4 );
    const point origin = layout.block_origin( last_cell, 1 );
    for( const std::array<float, 2> &local : {
             std::array<float, 2> { 1.0f, 0.5f }, std::array<float, 2> { 0.5f, 1.0f },
             std::array<float, 2> { 1.0f, 1.0f }
         } ) {
        CAPTURE( local[0], local[1] );
        const smooth_lighting::prefilter_lookup l = smooth_lighting::prefilter_lookup_texels( layout,
                last_cell, 1, local[0], local[1] );
        for( const point &t : l.texels ) {
            CAPTURE( t );
            CHECK( t.x >= origin.x );
            CHECK( t.y >= origin.y );
            CHECK( t.x <= origin.x + layout.grid );
            CHECK( t.y <= origin.y + layout.grid );
            CHECK( t.x < layout.size().x );
            CHECK( t.y < layout.size().y );
        }
    }
}

TEST_CASE( "lit_capability_keeps_per_tile_when_filtered_fails", "[smooth_lighting]" )
{
    using smooth_lighting::probe_group_result;
    using smooth_lighting::lookup;
    const probe_group_result pass = probe_group_result::passed;
    const probe_group_result mismatch = probe_group_result::mismatch;
    const probe_group_result unsafe = probe_group_result::unsafe;
    const probe_group_result skipped = probe_group_result::skipped;
    GIVEN( "per tile cases mismatch" ) {
        const smooth_lighting::lit_capability c = smooth_lighting::decide_lit_capability( mismatch,
                skipped, skipped );
        THEN( "nothing available, frame safe" ) {
            CHECK_FALSE( c.unsafe );
            CHECK_FALSE( c.per_tile );
            CHECK_FALSE( c.filtered );
        }
    }
    GIVEN( "per tile cases pass, filtered ones were skipped" ) {
        const smooth_lighting::lit_capability c = smooth_lighting::decide_lit_capability( pass, skipped,
                skipped );
        THEN( "only per tile light is available" ) {
            CHECK( c.per_tile );
            CHECK_FALSE( c.filtered );
        }
    }
    GIVEN( "hardware lookup passes" ) {
        const smooth_lighting::lit_capability c = smooth_lighting::decide_lit_capability( pass, pass,
                skipped );
        THEN( "filtered light uses it" ) {
            CHECK( c.per_tile );
            CHECK( c.filtered == lookup::hardware );
        }
    }
    GIVEN( "hardware lookup mismatches, manual one passes" ) {
        const smooth_lighting::lit_capability c = smooth_lighting::decide_lit_capability( pass, mismatch,
                pass );
        THEN( "filtered light uses the manual lookup" ) {
            CHECK( c.filtered == lookup::manual );
        }
    }
    GIVEN( "both lookups mismatch" ) {
        const smooth_lighting::lit_capability c = smooth_lighting::decide_lit_capability( pass, mismatch,
                mismatch );
        THEN( "per tile light stays" ) {
            CHECK( c.per_tile );
            CHECK_FALSE( c.filtered );
            CHECK_FALSE( c.unsafe );
        }
    }
    GIVEN( "any group left the renderer unsafe" ) {
        for( const std::array<probe_group_result, 3> &g : {
                 std::array<probe_group_result, 3> { unsafe, skipped, skipped },
                 std::array<probe_group_result, 3> { pass, unsafe, skipped },
                 std::array<probe_group_result, 3> { pass, mismatch, unsafe }
             } ) {
            const smooth_lighting::lit_capability c = smooth_lighting::decide_lit_capability( g[0], g[1],
                    g[2] );
            CAPTURE( static_cast<int>( g[0] ), static_cast<int>( g[1] ), static_cast<int>( g[2] ) );
            CHECK( c.unsafe );
            CHECK_FALSE( c.per_tile );
            CHECK_FALSE( c.filtered );
        }
    }
}

TEST_CASE( "hardware_lookup_needs_the_linear_sampler", "[smooth_lighting]" )
{
    smooth_lighting::lit_capability probed;
    probed.per_tile = true;
    GIVEN( "a probe that chose the hardware lookup" ) {
        probed.filtered = smooth_lighting::lookup::hardware;
        WHEN( "linear sampler is gone" ) {
            const smooth_lighting::lit_capability c = smooth_lighting::with_linear_sampler( probed, false );
            THEN( "filtered light unavailable, per tile light stays" ) {
                CHECK( c.per_tile );
                CHECK_FALSE( c.filtered );
            }
        }
        WHEN( "linear sampler is there" ) {
            THEN( "hardware lookup stays" ) {
                CHECK( smooth_lighting::with_linear_sampler( probed, true ).filtered ==
                       smooth_lighting::lookup::hardware );
            }
        }
    }
    GIVEN( "a probe that chose the manual lookup" ) {
        probed.filtered = smooth_lighting::lookup::manual;
        THEN( "it needs no linear sampler" ) {
            CHECK( smooth_lighting::with_linear_sampler( probed, false ).filtered ==
                   smooth_lighting::lookup::manual );
        }
    }
}

TEST_CASE( "filtered_failures_latch_without_turning_off_per_tile_light", "[smooth_lighting]" )
{
    using smooth_lighting::lighting_status;
    smooth_lighting::failure_policy main;
    smooth_lighting::failure_policy filtered( smooth_lighting::lit_failure::prefilter );
    main.rebuilt( 1 );
    filtered.rebuilt( 1 );
    smooth_lighting::lit_capability capable;
    capable.per_tile = true;
    capable.filtered = smooth_lighting::lookup::hardware;
    GIVEN( "a prefilter that fails every frame" ) {
        for( int i = 1; i < smooth_lighting::failure_policy::upload_retries; ++i ) {
            CHECK_FALSE( filtered.fail( smooth_lighting::lit_failure::prefilter ) );
        }
        WHEN( "last retry fails too" ) {
            CHECK( filtered.fail( smooth_lighting::lit_failure::prefilter ) );
            const smooth_lighting::lit_mode m = smooth_lighting::choose_lit_mode( true, capable,
                                                filtered.latched(), true );
            THEN( "filtered light is off, per tile light stays on" ) {
                CHECK( m.per_tile );
                CHECK( m.status == lighting_status::smooth_filtered_unavailable );
                CHECK_FALSE( main.latched() );
            }
            AND_WHEN( "resources are rebuilt" ) {
                filtered.rebuilt( 2 );
                const smooth_lighting::lit_mode again = smooth_lighting::choose_lit_mode( true, capable,
                                                        filtered.latched(), true );
                THEN( "filtered light comes back" ) {
                    CHECK_FALSE( again.per_tile );
                    CHECK( again.status == lighting_status::smooth_filtered );
                }
            }
        }
    }
    GIVEN( "a prefilter too large for the GPU" ) {
        const smooth_lighting::lit_mode m = smooth_lighting::choose_lit_mode( true, capable, false,
                                            false );
        THEN( "frame draws per tile" ) {
            CHECK( m.per_tile );
            CHECK( m.status == lighting_status::smooth_filtered_unavailable );
        }
    }
    GIVEN( "smooth asked for" ) {
        const smooth_lighting::lit_mode m = smooth_lighting::choose_lit_mode( false, capable, false,
                                            true );
        THEN( "the frame draws per tile as smooth" ) {
            CHECK( m.per_tile );
            CHECK( m.status == lighting_status::smooth );
        }
    }
}

TEST_CASE( "prefilter_cache_publishes_only_after_a_clean_pass", "[smooth_lighting]" )
{
    smooth_lighting::prefilter_inputs in;
    in.upload_generation = 4;
    in.layout = test_layout( smooth_lighting::prefilter_grid );
    in.mode = smooth_lighting::lookup::hardware;
    in.resource_generation = 7;
    smooth_lighting::prefilter_cache cache;
    CHECK( cache.needs_run( in ) );
    GIVEN( "a pass that failed after it began writing" ) {
        cache.publish( in );
        cache.begin_write();
        THEN( "next frame runs it again" ) {
            CHECK( cache.needs_run( in ) );
        }
    }
    GIVEN( "a clean pass" ) {
        cache.begin_write();
        cache.publish( in );
        THEN( "equal inputs run no pass" ) {
            CHECK_FALSE( cache.needs_run( in ) );
        }
        THEN( "any input change runs it again" ) {
            smooth_lighting::prefilter_inputs uploaded = in;
            ++uploaded.upload_generation;
            CHECK( cache.needs_run( uploaded ) );
            smooth_lighting::prefilter_inputs moved = in;
            moved.layout.area = half_open_rectangle<point>( point::south_east, point( test_columns,
                                test_rows ) );
            CHECK( cache.needs_run( moved ) );
            smooth_lighting::prefilter_inputs manual = in;
            manual.mode = smooth_lighting::lookup::manual;
            CHECK( cache.needs_run( manual ) );
            smooth_lighting::prefilter_inputs rebuilt = in;
            ++rebuilt.resource_generation;
            CHECK( cache.needs_run( rebuilt ) );
        }
    }
}

TEST_CASE( "prefilter_texture_headroom_stays_within_the_gpu_limit", "[smooth_lighting]" )
{
    GIVEN( "a request well under the limit" ) {
        THEN( "a quarter more is allocated" ) {
            CHECK( smooth_lighting::prefilter_texture_size( point( 400, 200 ), point::zero,
                    4096 ) == point( 500, 250 ) );
        }
    }
    GIVEN( "a request whose headroom would pass the limit" ) {
        THEN( "headroom stops at the limit" ) {
            CHECK( smooth_lighting::prefilter_texture_size( point( 3432, 100 ), point::zero,
                    4096 ) == point( 4096, 125 ) );
        }
    }
    GIVEN( "a request exactly at the limit" ) {
        THEN( "it's allocated as asked" ) {
            CHECK( smooth_lighting::prefilter_texture_size( point( 4096, 4096 ), point::zero,
                    4096 ) == point( 4096, 4096 ) );
        }
    }
    GIVEN( "a texture already larger on one side" ) {
        THEN( "that side keeps its size" ) {
            CHECK( smooth_lighting::prefilter_texture_size( point( 100, 900 ), point( 800, 10 ),
                    4096 ) == point( 800, 1125 ) );
        }
    }
    GIVEN( "nothing to store" ) {
        THEN( "texture is still one texel" ) {
            CHECK( smooth_lighting::prefilter_texture_size( point::zero, point::zero,
                    4096 ) == point::south_east );
        }
    }
}

TEST_CASE( "failures_of_filtered_resources_turn_off_only_filtered_light", "[smooth_lighting]" )
{
    using smooth_lighting::failure_scope;
    using smooth_lighting::lit_failure;
    GIVEN( "frame that filters" ) {
        THEN( "failing to bind, upload to or store its filtered light turns off only that" ) {
            for( const lit_failure f : {
                     lit_failure::prefilter, lit_failure::state_create, lit_failure::uniform_upload,
                     lit_failure::texture_create, lit_failure::sampler_create
                 } ) {
                CAPTURE( smooth_lighting::to_string( f ) );
                CHECK( smooth_lighting::scope_of( f, true ) == failure_scope::filtered );
            }
        }
        THEN( "broken shaders or upload turn off all smooth light" ) {
            for( const lit_failure f : {
                     lit_failure::shader_load, lit_failure::probe_mismatch, lit_failure::upload
                 } ) {
                CAPTURE( smooth_lighting::to_string( f ) );
                CHECK( smooth_lighting::scope_of( f, true ) == failure_scope::all );
            }
        }
    }
    GIVEN( "frame per tile" ) {
        THEN( "all failures except prefilter turn off all smooth light" ) {
            CHECK( smooth_lighting::scope_of( lit_failure::state_create, false ) == failure_scope::all );
            CHECK( smooth_lighting::scope_of( lit_failure::texture_create, false ) == failure_scope::all );
            CHECK( smooth_lighting::scope_of( lit_failure::prefilter, false ) == failure_scope::filtered );
        }
    }
}

TEST_CASE( "smooth_lighting_failure_policy", "[smooth_lighting]" )
{
    SECTION( "uploads fail on consecutive frames until the last retry latches" ) {
        smooth_lighting::failure_policy policy;
        policy.rebuilt( 1 );
        for( int i = 1; i < smooth_lighting::failure_policy::upload_retries; ++i ) {
            CHECK_FALSE( policy.fail( smooth_lighting::lit_failure::upload ) );
        }
        CHECK( policy.fail( smooth_lighting::lit_failure::upload ) );
        CHECK( policy.latched_by() == smooth_lighting::lit_failure::upload );
    }
    SECTION( "a frame drawn lit resets the upload streak" ) {
        smooth_lighting::failure_policy policy;
        policy.rebuilt( 1 );
        CHECK_FALSE( policy.fail( smooth_lighting::lit_failure::upload ) );
        policy.succeed();
        for( int i = 1; i < smooth_lighting::failure_policy::upload_retries; ++i ) {
            CHECK_FALSE( policy.fail( smooth_lighting::lit_failure::upload ) );
        }
        CHECK_FALSE( policy.latched() );
    }
    SECTION( "other failures latch at once until the resources are rebuilt" ) {
        for( const smooth_lighting::lit_failure f : {
                 smooth_lighting::lit_failure::texture_create, smooth_lighting::lit_failure::sampler_create,
                 smooth_lighting::lit_failure::shader_load, smooth_lighting::lit_failure::state_create,
                 smooth_lighting::lit_failure::uniform_upload, smooth_lighting::lit_failure::probe_mismatch
             } ) {
            CAPTURE( smooth_lighting::to_string( f ) );
            smooth_lighting::failure_policy p;
            p.rebuilt( 1 );
            CHECK( p.fail( f ) );
            p.rebuilt( 1 );
            CHECK( p.latched() );
            p.rebuilt( 2 );
            CHECK_FALSE( p.latched() );
        }
    }
}

TEST_CASE( "levels_with_nothing_seen_fill_as_zero", "[smooth_lighting][vision]" )
{
    const tripoint_bub_ms center( 60, 60, 0 );
    build_dark_room( center, 6 );
    settle_caches( 0 );
    const map &here = get_map();
    smooth_lighting::lightmap_fill_settings settings;
    settings.area = half_open_rectangle<point>( point( 40, 40 ), point( 80, 80 ) );
    settings.vision_threshold = here.get_visibility_variables_cache().vision_threshold;
    settings.tint = true;
    settings.masks = true;
    std::vector<smooth_lighting::lightmap_texel> layer;
    // level below the room's floor is out of sight
    CHECK_FALSE( smooth_lighting::encode_lightmap_layer( here, -1, settings, layer ) );
    CHECK( std::all_of( layer.begin(), layer.end(), []( const smooth_lighting::lightmap_texel & t ) {
        return t == smooth_lighting::lightmap_texel();
    } ) );
}

TEST_CASE( "reach_masks_are_built_only_round_seen_cells", "[smooth_lighting][vision]" )
{
    const tripoint_bub_ms center( 60, 60, 0 );
    build_dark_room( center, 6 );
    settle_caches( 0 );
    const map &here = get_map();
    smooth_lighting::lightmap_fill_settings settings;
    settings.area = half_open_rectangle<point>( point( 40, 40 ), point( 80, 80 ) );
    settings.vision_threshold = here.get_visibility_variables_cache().vision_threshold;
    settings.masks = true;
    std::vector<smooth_lighting::lightmap_texel> layer;
    REQUIRE( smooth_lighting::encode_lightmap_layer( here, 0, settings, layer ) );
    // nothing's seen far outside the walled room
    CHECK( layer[smooth_lighting::reach_index( point( 45,
                                               45 ) )] == smooth_lighting::lightmap_texel() );
    // avatar's own cell is seen
    CHECK_FALSE( layer[smooth_lighting::reach_index( center.xy().raw() )] ==
                 smooth_lighting::lightmap_texel() );
}

TEST_CASE( "dim_seen_light_looks_like_the_classic_shadow_variant", "[smooth_lighting]" )
{
    const std::array<float, 3> rgb = { 0.8f, 0.4f, 0.2f };
    // color_pixel_grayscale
    const float shadow = ( rgb[0] + rgb[1] + rgb[2] ) / 3.0f * 5.0f / 8.0f;
    smooth_lighting::lit_sample s;
    s.visible = 1.0f;
    for( int memory_look = 0; memory_look <= smooth_lighting::custom_look; ++memory_look ) {
        for( const bool blend : {
                 false, true
             } ) {
            CAPTURE( memory_look, blend );
            smooth_lighting::look_params look;
            look.memory_look = memory_look;
            look.blend_memory = blend;
            look.custom_dark = { 0.1f, 0.5f, 0.1f };
            look.custom_light = { 0.9f, 0.1f, 0.9f };
            WHEN( "light is at the vision threshold" ) {
                s.light = 0.0f;
                const std::array<float, 3> got = smooth_lighting::reference_lit_rgb( look, rgb, s );
                THEN( "tile looks as classic tiles draw dim light, whatever the memory look" ) {
                    for( const float c : got ) {
                        CHECK( c == Approx( shadow ).margin( 1e-5 ) );
                    }
                }
            }
            WHEN( "light is full" ) {
                s.light = 1.0f;
                const std::array<float, 3> got = smooth_lighting::reference_lit_rgb( look, rgb, s );
                THEN( "sprite keeps its own color" ) {
                    for( size_t k = 0; k < 3; ++k ) {
                        CHECK( got[k] == Approx( rgb[k] ).margin( 1e-5 ) );
                    }
                }
            }
        }
    }
}

TEST_CASE( "terrain_that_rises_takes_light_along_its_base", "[smooth_lighting]" )
{
    for( const ter_str_id &t : {
             ter_t_brick_wall, ter_t_tree, ter_t_shrub, ter_t_door_c, ter_t_window
         } ) {
        CAPTURE( t );
        CHECK( smooth_lighting::terrain_light_anchor( t.obj() ) == smooth_lighting::light_anchor::base );
    }
    // data calls an open door flat, and so does the rule
    for( const ter_str_id &t : {
             ter_t_floor, ter_t_dirt, ter_t_grass, ter_t_door_o
         } ) {
        CAPTURE( t );
        CHECK( smooth_lighting::terrain_light_anchor( t.obj() ) == smooth_lighting::light_anchor::ground );
    }
}

TEST_CASE( "tile_entries_read_light_anchor", "[smooth_lighting]" )
{
    const auto read = []( const std::string & json ) {
        JsonObject entry = json_loader::from_string( json );
        entry.allow_omitted_members();
        return smooth_lighting::read_light_anchor( entry );
    };
    CHECK_FALSE( read( R"({ "id": "t_dirt" })" ) );
    CHECK( read( R"({ "id": "t_wall", "light_anchor": "base" })" ) ==
           smooth_lighting::light_anchor::base );
    CHECK( read( R"({ "id": "f_rug", "light_anchor": "ground" })" ) ==
           smooth_lighting::light_anchor::ground );
    CHECK_THROWS_AS( read( R"({ "id": "t_wall", "light_anchor": "up" })" ), JsonError );
}

TEST_CASE( "a_tilesets_own_anchor_beats_the_default", "[smooth_lighting]" )
{
    using smooth_lighting::light_anchor;
    CHECK( smooth_lighting::chosen_light_anchor( light_anchor::ground, light_anchor::base ) ==
           light_anchor::ground );
    CHECK( smooth_lighting::chosen_light_anchor( std::nullopt, light_anchor::base ) ==
           light_anchor::base );
    CHECK_FALSE( smooth_lighting::chosen_light_anchor( std::nullopt, std::nullopt ) );
}

TEST_CASE( "colored_light_mixes_its_hue_in_at_the_pixels_brightness",
           "[smooth_lighting][light_color]" )
{
    // brown wood, with almost no blue of its own, under full magenta light
    const std::array<float, 3> brown = { 0.5f, 0.3f, 0.1f };
    smooth_lighting::lit_sample s;
    s.light = 1.0f;
    s.visible = 1.0f;
    const smooth_lighting::look_params look;
    const std::array<float, 3> plain = smooth_lighting::reference_lit_rgb( look, brown, s );
    s.hue = { 1.0f, 0.0f, 1.0f };
    const std::array<float, 3> tinted = smooth_lighting::reference_lit_rgb( look, brown, s );
    CAPTURE( tinted[0], tinted[1], tinted[2] );
    THEN( "gains light's blue, loses green" ) {
        CHECK( tinted[2] > plain[2] + 0.1f );
        CHECK( tinted[1] < plain[1] );
    }
    THEN( "its brightness stays" ) {
        CHECK( tinted[0] + tinted[1] + tinted[2] == Approx( plain[0] + plain[1] + plain[2] ).margin(
                   1e-5 ) );
    }
    THEN( "mix goes tint_mix of the way to the light's color" ) {
        const float brightness = ( plain[0] + plain[1] + plain[2] ) / 3.0f;
        const float target_blue = brightness / ( 2.0f / 3.0f );
        CHECK( tinted[2] == Approx( plain[2] + ( target_blue - plain[2] ) *
                                    smooth_lighting::tint_mix ).margin(
                   1e-5 ) );
    }
}

TEST_CASE( "colored_light_mixing_stays_on_screen", "[smooth_lighting][light_color]" )
{
    smooth_lighting::lit_sample s;
    s.light = 1.0f;
    s.visible = 1.0f;
    const smooth_lighting::look_params look;
    GIVEN( "white pixel under full red light" ) {
        s.hue = { 1.0f, 0.0f, 0.0f };
        const std::array<float, 3> got = smooth_lighting::reference_lit_rgb( look, { 1.0f, 1.0f, 1.0f },
                                         s );
        CAPTURE( got[0], got[1], got[2] );
        THEN( "red stays at full, others drop, none past what the screen shows" ) {
            CHECK( got[0] == Approx( 1.0f ) );
            CHECK( got[1] == Approx( 1.0f - smooth_lighting::tint_mix ) );
            CHECK( got[2] == Approx( 1.0f - smooth_lighting::tint_mix ) );
        }
    }
    GIVEN( "pixels of every brightness under saturated lights" ) {
        for( const std::array<float, 3> &hue : {
                 std::array<float, 3> { 1.0f, 0.0f, 0.0f }, std::array<float, 3> { 0.0f, 1.0f, 0.0f },
                 std::array<float, 3> { 1.0f, 0.0f, 1.0f }
             } ) {
            for( int r = 0; r <= 4; ++r ) {
                for( int g = 0; g <= 4; ++g ) {
                    const std::array<float, 3> rgb = { r / 4.0f, g / 4.0f, 0.5f };
                    s.hue = hue;
                    const std::array<float, 3> got = smooth_lighting::reference_lit_rgb( look, rgb, s );
                    CAPTURE( hue[0], hue[1], hue[2], rgb[0], rgb[1], got[0], got[1], got[2] );
                    for( const float c : got ) {
                        CHECK( c <= 1.0f + 1e-6f );
                    }
                }
            }
        }
    }
}

TEST_CASE( "a_fully_colored_light_tints_at_full_strength", "[smooth_lighting][light_color]" )
{
    // pure magenta light as all the light there is, at full strength
    light_color_rgb magenta;
    magenta.r = LIGHT_AMBIENT_LIT;
    magenta.g = 0.0f;
    magenta.b = LIGHT_AMBIENT_LIT;
    const std::array<float, 3> hue = smooth_lighting::illumination_hue( magenta, LIGHT_AMBIENT_LIT );
    CHECK( hue[0] == Approx( 1.0f ) );
    CHECK( hue[1] == Approx( 0.0f ).margin( 0.01 ) );
    CHECK( hue[2] == Approx( 1.0f ) );
}

// 32x32 ortho tile at screen ( 100, 200 ), its sprite filling it
static smooth_lighting::lit_quad_params tile_quad( const tripoint_bub_ms &pos )
{
    smooth_lighting::lit_quad_params q;
    q.screen = { 100.0f, 200.0f, 132.0f, 232.0f };
    q.uv = { 0.25f, 0.5f, 0.5f, 0.75f };
    q.tile_width = 32.0f;
    q.tile_height = 32.0f;
    q.ground_x = 100.0f;
    q.ground_y = 200.0f;
    q.pos = pos;
    return q;
}

static float row_of( const tripoint_bub_ms &pos )
{
    return static_cast<float>( ( pos.z() + OVERMAP_DEPTH ) * MAPSIZE_Y + pos.y() );
}

TEST_CASE( "lit_sprite_corners_address_their_tile_in_the_light_map", "[smooth_lighting]" )
{
    const tripoint_bub_ms pos( 10, 20, 0 );
    GIVEN( "flat sprite filling its ortho tile" ) {
        const std::array<smooth_lighting::lit_vertex, 4> v = smooth_lighting::lit_quad( tile_quad( pos ) );
        THEN( "its corners fall on its cell's corners and carry its own cell" ) {
            CHECK( v[0].light.x == Approx( 10.0f ) );
            CHECK( v[0].light.y == Approx( row_of( pos ) ) );
            CHECK( v[2].light.x == Approx( 11.0f ) );
            CHECK( v[2].light.y == Approx( row_of( pos ) + 1.0f ) );
            for( const smooth_lighting::lit_vertex &c : v ) {
                CHECK( c.light.column == Approx( 10.0f ) );
                CHECK( c.light.row == Approx( row_of( pos ) ) );
            }
        }
    }
    GIVEN( "same sprite standing" ) {
        smooth_lighting::lit_quad_params q = tile_quad( pos );
        q.standing = true;
        THEN( "its column carries the standing marker" ) {
            CHECK( smooth_lighting::lit_quad( q )[0].light.column ==
                   Approx( 10.0f + smooth_lighting::standing_marker ) );
        }
    }
    GIVEN( "tile on the level below, drawn lower on screen" ) {
        const tripoint_bub_ms below = pos + tripoint::below;
        smooth_lighting::lit_quad_params q = tile_quad( below );
        q.screen[1] += 16.0f;
        q.screen[3] += 16.0f;
        q.ground_y += 16.0f;
        const std::array<smooth_lighting::lit_vertex, 4> v = smooth_lighting::lit_quad( q );
        THEN( "it addresses the same cell in that level's rows" ) {
            CHECK( v[0].light.x == Approx( 10.0f ) );
            CHECK( v[0].light.y == Approx( row_of( below ) ) );
            CHECK( v[0].light.row == Approx( row_of( below ) ) );
        }
    }
}

TEST_CASE( "lit_sprite_texture_follows_turns_and_flips", "[smooth_lighting]" )
{
    const tripoint_bub_ms pos( 10, 20, 0 );
    WHEN( "sprite turned clockwise" ) {
        smooth_lighting::lit_quad_params q = tile_quad( pos );
        q.turn = smooth_lighting::quarter_turn::clockwise;
        const std::array<smooth_lighting::lit_vertex, 4> v = smooth_lighting::lit_quad( q );
        THEN( "bottom-left texel lands at top-left of the tile" ) {
            CHECK( v[3].x == Approx( 100.0f ) );
            CHECK( v[3].y == Approx( 200.0f ) );
            CHECK( v[3].u == Approx( 0.25f ) );
            CHECK( v[3].v == Approx( 0.75f ) );
            // and takes the light of where it lands
            CHECK( v[3].light.x == Approx( 10.0f ) );
            CHECK( v[3].light.y == Approx( row_of( pos ) ) );
        }
    }
    WHEN( "sprite is flipped horizontally" ) {
        smooth_lighting::lit_quad_params q = tile_quad( pos );
        q.flip_horizontal = true;
        const std::array<smooth_lighting::lit_vertex, 4> v = smooth_lighting::lit_quad( q );
        THEN( "its left corners take the right texture edge" ) {
            CHECK( v[0].u == Approx( 0.5f ) );
            CHECK( v[1].u == Approx( 0.25f ) );
            CHECK( v[0].x == Approx( 100.0f ) );
        }
    }
}

TEST_CASE( "lit_sprite_corners_map_the_iso_diamond", "[smooth_lighting]" )
{
    const tripoint_bub_ms pos( 10, 20, 0 );
    // 32 wide, 32 tall iso tile: its diamond's left corner sits 24 down
    smooth_lighting::lit_quad_params q = tile_quad( pos );
    q.iso = true;
    q.screen = { 100.0f, 216.0f, 116.0f, 224.0f };
    const std::array<smooth_lighting::lit_vertex, 4> v = smooth_lighting::lit_quad( q );
    THEN( "the diamond's top corner is the cell's top right" ) {
        CHECK( v[1].light.x == Approx( 11.0f ) );
        CHECK( v[1].light.y == Approx( row_of( pos ) ) );
    }
    THEN( "the diamond's left corner is the cell's top left" ) {
        CHECK( v[3].light.x == Approx( 10.0f ) );
        CHECK( v[3].light.y == Approx( row_of( pos ) ) );
    }
}
