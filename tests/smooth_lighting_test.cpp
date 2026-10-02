#include <algorithm>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

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
