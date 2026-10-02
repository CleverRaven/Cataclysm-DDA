#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <initializer_list>
#include <memory>
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
#include "game.h"
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
static const ter_str_id ter_t_flat_roof( "t_flat_roof" );
static const ter_str_id ter_t_floor( "t_floor" );
static const ter_str_id ter_t_utility_light( "t_utility_light" );

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
    const tripoint_bub_ms far = center + tripoint::east * 14;
    REQUIRE( g->place_critter_at( mon_zombie_electric, far ) );
    settle_caches( 0 );
    const map &here = get_map();
    const level_cache &ch = here.access_cache( 0 );
    const lit_level ll = ch.visibility_cache[far.x()][far.y()];
    REQUIRE( ll == lit_level::BRIGHT_ONLY );
    const float apparent = map::apparent_light_helper( ch, far ).apparent_light;
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
