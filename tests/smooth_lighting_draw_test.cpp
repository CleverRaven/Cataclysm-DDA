#if defined(TILES)

#include <array>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "atlas_bake_plan.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_shader.h"
#include "cata_tiles.h"
#include "coordinates.h"
#include "creature.h"
#include "cursesdef.h"
#include "game.h"
#include "map.h"
#include "map_helpers_tests.h"
#include "options_helpers.h"
#include "path_info.h"
#include "player_helpers.h"
#include "point.h"
#include "sdl_renderer_recovery.h"
#include "sdltiles.h"
#include "smooth_lighting.h"
#include "type_id.h"
#include "weighted_list.h"

static const mtype_id mon_zombie( "mon_zombie" );

static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_wall( "t_wall" );

namespace
{

const tripoint_bub_ms player_pos( 65, 65, 0 );
constexpr int view_tiles = 9;

using light_log = std::vector<drawn_sprite_record>;

tile_type atlas_sprite_tile()
{
    tile_type tile;
    tile.fg.add( std::vector<int> { 0 }, 1 );
    return tile;
}

std::vector<std::pair<std::string, tile_type>> plain_tiles( const std::vector<std::string> &ids )
{
    std::vector<std::pair<std::string, tile_type>> tiles;
    tiles.reserve( ids.size() );
    for( const std::string &id : ids ) {
        tiles.emplace_back( id, atlas_sprite_tile() );
    }
    return tiles;
}

// `center` and its eight neighbours; the small test view draws only some of them
std::vector<tripoint_bub_ms> around( const tripoint_bub_ms &center )
{
    std::vector<tripoint_bub_ms> tiles;
    for( int dx = -1; dx <= 1; ++dx ) {
        for( int dy = -1; dy <= 1; ++dy ) {
            tiles.push_back( center + tripoint( dx, dy, 0 ) );
        }
    }
    return tiles;
}

// draws a grass map on the software renderer, logging each sprite's light policy
struct policy_fixture {
    explicit policy_fixture( const std::vector<std::pair<std::string, tile_type>> &tiles =
    plain_tiles( {
        ter_t_grass.str(), ter_t_wall.str(), mon_zombie.str(), "lighting_hidden"
    } ) ) : available_( fx_.available() ) {
        if( !available_ ) {
            return;
        }
        clear_avatar();
        g->place_player( player_pos );
        build_test_map( ter_t_grass.id() );
        set_time_to_day();
        refresh_caches();
        const std::shared_ptr<const tileset> ts = renderer_recovery_test_support::install_tiles_bundle(
                    "policy_ts", "color_pixel_darken", 1, 1, atlas_bake_plan{}, true, tiles );
        REQUIRE( ts );
        tiles_ = renderer_recovery_test_support::make_test_tiles( ts );
        renderer_recovery_test_support::log_draw_light( *tiles_, &log_ );
    }
    ~policy_fixture() {
        if( tiles_ ) {
            renderer_recovery_test_support::log_draw_light( *tiles_, nullptr );
        }
    }
    policy_fixture( const policy_fixture & ) = delete;
    policy_fixture &operator=( const policy_fixture & ) = delete;

    static void refresh_caches() {
        map &here = get_map();
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0 );
        here.invalidate_visibility_cache();
        here.update_visibility_cache( 0 );
    }
    void draw() {
        log_.clear();
        tiles_->set_draw_cache_dirty();
        renderer_recovery_test_support::draw_test_map( *tiles_, player_pos, view_tiles, view_tiles );
    }
    bool drew( const std::string &id, const draw_light light ) const {
        for( const drawn_sprite_record &e : log_ ) {
            if( e.id == id && e.light == light ) {
                return true;
            }
        }
        return false;
    }

    software_render_fixture fx_;
    bool available_ = false;
    std::unique_ptr<cata_tiles> tiles_;
    light_log log_;
};

} // namespace

TEST_CASE( "map_sprites_take_scene_light_and_overlays_fixed_light", "[smooth_lighting][tiles]" )
{
    policy_fixture fx;
    if( !fx.available_ ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    WHEN( "plain terrain is drawn" ) {
        fx.draw();
        THEN( "it takes the scene's light" ) {
            CHECK( fx.drew( ter_t_grass.str(), draw_light::scene ) );
        }
    }
    WHEN( "tiles around avatar show terrain override" ) {
        for( const tripoint_bub_ms &p : around( player_pos ) ) {
            fx.tiles_->init_draw_terrain_override( p, ter_t_wall.id() );
        }
        fx.draw();
        fx.tiles_->void_terrain_override();
        THEN( "override keeps its look whatever the light" ) {
            CHECK( fx.drew( ter_t_wall.str(), draw_light::fixed ) );
        }
    }
    WHEN( "tiles around avatar show monster override" ) {
        for( const tripoint_bub_ms &p : around( player_pos ) ) {
            fx.tiles_->init_draw_monster_override( p, mon_zombie, 1, false, Creature::Attitude::HOSTILE );
        }
        fx.draw();
        fx.tiles_->void_monster_override();
        THEN( "override keeps its look whatever the light" ) {
            CHECK( fx.drew( mon_zombie.str(), draw_light::fixed ) );
        }
    }
    WHEN( "night on unexplored ground hides the tiles round the avatar" ) {
        // no memory and out of sight, so the tile draws the hidden vision effect
        get_avatar().clear_map_memory();
        calendar::turn = calendar::turn_zero;
        g->reset_light_level();
        policy_fixture::refresh_caches();
        fx.draw();
        THEN( "vision effect over a hidden tile keeps its look" ) {
            CHECK( fx.drew( "lighting_hidden", draw_light::fixed ) );
        }
    }
}

TEST_CASE( "failed_shader_unbind_stops_the_minimap_before_it_paints",
           "[smooth_lighting][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    REQUIRE( renderer_recovery_test_support::install_test_font( PATH_INFO::fontdir() + "unifont.ttf",
             8, 16, 16, false ) );
    const catacurses::window w = catacurses::newwin( 4, 8, point::zero );
    int paints = 0;
    WHEN( "minimap window is drawn" ) {
        CHECK( renderer_recovery_test_support::draw_test_minimap_window( w, false, paints ) );
        THEN( "minimap paints" ) {
            CHECK( paints == 1 );
            CHECK_FALSE( display_buffer_scope_recovery_pending() );
        }
    }
    WHEN( "shader unbind before its text fails" ) {
        CHECK_FALSE( renderer_recovery_test_support::draw_test_minimap_window( w, true, paints ) );
        THEN( "recovery is pending and minimap never paints" ) {
            CHECK( paints == 0 );
            CHECK( display_buffer_scope_recovery_pending() );
        }
        renderer_coordinator.drain_pending();
        CHECK( renderer_coordinator.state() == renderer_recovery_state::ready );
    }
}

// grass, and a wall drawn through subtiles that take `subtile_anchor`
static std::vector<std::pair<std::string, tile_type>> wall_tiles(
            const std::optional<smooth_lighting::light_anchor> &subtile_anchor )
{
    std::vector<std::pair<std::string, tile_type>> tiles = plain_tiles( { ter_t_grass.str() } );
    tile_type wall = atlas_sprite_tile();
    wall.multitile = true;
    wall.rotates = true;
    for( const std::string key : {
             "center", "corner", "edge", "t_connection", "end_piece", "unconnected"
         } ) {
        wall.available_subtiles.push_back( key );
        tile_type subtile = atlas_sprite_tile();
        subtile.rotates = true;
        subtile.light_anchor = subtile_anchor;
        tiles.emplace_back( ter_t_wall.str() + "_" + key, subtile );
    }
    tiles.emplace_back( ter_t_wall.str(), wall );
    return tiles;
}

static std::vector<std::optional<smooth_lighting::light_anchor>> wall_subtile_anchors(
            const light_log &log )
{
    std::vector<std::optional<smooth_lighting::light_anchor>> anchors;
    for( const drawn_sprite_record &e : log ) {
        if( e.id.rfind( ter_t_wall.str() + "_", 0 ) == 0 ) {
            anchors.push_back( e.anchor );
        }
    }
    return anchors;
}

TEST_CASE( "connected_walls_keep_their_anchor_through_subtiles", "[smooth_lighting][tiles]" )
{
    const auto draw_walls = []( policy_fixture & fx ) {
        // a ring round the avatar, in the part of the map the small view draws
        map &here = get_map();
        for( const tripoint_bub_ms &p : around( player_pos ) ) {
            if( p != player_pos ) {
                here.ter_set( p, ter_t_wall.id() );
            }
        }
        policy_fixture::refresh_caches();
        fx.draw();
    };
    GIVEN( "a wall whose subtiles say nothing of their anchor" ) {
        policy_fixture fx( wall_tiles( std::nullopt ) );
        if( !fx.available_ ) {
            WARN( "dummy SDL video backend unavailable; skipping" );
            return;
        }
        draw_walls( fx );
        THEN( "its subtiles take the base anchor of walls" ) {
            const std::vector<std::optional<smooth_lighting::light_anchor>> anchors =
                        wall_subtile_anchors( fx.log_ );
            REQUIRE_FALSE( anchors.empty() );
            for( const std::optional<smooth_lighting::light_anchor> &a : anchors ) {
                CHECK( a == smooth_lighting::light_anchor::base );
            }
        }
    }
    GIVEN( "the wall whose subtiles the tileset lights as ground" ) {
        policy_fixture fx( wall_tiles( smooth_lighting::light_anchor::ground ) );
        if( !fx.available_ ) {
            WARN( "dummy SDL video backend unavailable; skipping" );
            return;
        }
        draw_walls( fx );
        THEN( "tileset's anchor wins" ) {
            const std::vector<std::optional<smooth_lighting::light_anchor>> anchors =
                        wall_subtile_anchors( fx.log_ );
            REQUIRE_FALSE( anchors.empty() );
            for( const std::optional<smooth_lighting::light_anchor> &a : anchors ) {
                CHECK( a == smooth_lighting::light_anchor::ground );
            }
        }
    }
}

TEST_CASE( "what_a_sprite_shows_sets_where_it_takes_its_light", "[smooth_lighting]" )
{
    using smooth_lighting::light_anchor;
    CHECK( cata_tiles::default_light_anchor( TILE_CATEGORY::TERRAIN, ter_t_wall.str() ) ==
           light_anchor::base );
    CHECK( cata_tiles::default_light_anchor( TILE_CATEGORY::TERRAIN, ter_t_grass.str() ) ==
           light_anchor::ground );
    CHECK( cata_tiles::default_light_anchor( TILE_CATEGORY::FURNITURE, "f_chair" ) ==
           light_anchor::base );
    CHECK( cata_tiles::default_light_anchor( TILE_CATEGORY::MONSTER, mon_zombie.str() ) ==
           light_anchor::base );
    CHECK( cata_tiles::default_light_anchor( TILE_CATEGORY::VEHICLE_PART, "vp_frame" ) ==
           light_anchor::base );
    // other categories and unknown ids have no default; the sprite's shape decides
    CHECK_FALSE( cata_tiles::default_light_anchor( TILE_CATEGORY::ITEM, "rock" ) );
    CHECK_FALSE( cata_tiles::default_light_anchor( TILE_CATEGORY::TERRAIN, "t_no_such_terrain" ) );
}

TEST_CASE( "custom_memory_overlay_reaches_the_lit_shader", "[smooth_lighting]" )
{
    override_option mode( "MEMORY_MAP_MODE", "color_pixel_custom" );
    override_option red( "MEMORY_RGB_DARK_RED", "51" );
    override_option gamma( "MEMORY_GAMMA", "2.0" );
    GIVEN( "no named preset is active" ) {
        const cata_shader::memory_look look = cata_tiles::memory_look_from_options( std::nullopt );
        THEN( "the look carries the custom colors and gamma" ) {
            CHECK_FALSE( look.preset );
            CHECK( look.custom_dark[0] == Approx( 0.2f ) );
            CHECK( look.custom_gamma == Approx( 2.0f ) );
        }
    }
    GIVEN( "a named preset is active" ) {
        THEN( "the look is that preset" ) {
            CHECK( cata_tiles::memory_look_from_options( cata_shader::memory_preset::SEPIA_DARK ).preset ==
                   cata_shader::memory_preset::SEPIA_DARK );
        }
    }
}

static const cata_shader::lit_probe::probe_case &probe_case_named(
    const std::vector<cata_shader::lit_probe::probe_case> &cases, const std::string &name )
{
    for( const cata_shader::lit_probe::probe_case &c : cases ) {
        if( c.name == name ) {
            return c;
        }
    }
    FAIL( "no probe case " << name );
    return cases.front();
}

TEST_CASE( "lit_probe_rejects_broken_shaders", "[smooth_lighting]" )
{
    using namespace cata_shader::lit_probe;
    bool ignored_passes = true;
    bool full_light_passes = true;
    for( const probe_case &c : cases() ) {
        CAPTURE( c.name );
        const std::vector<rgb> want = expected( c );
        CHECK( matches( want, want ) );
        ignored_passes = ignored_passes && matches( want, readback_if_ignored( c ) );
        full_light_passes = full_light_passes && matches( want, readback_if_full_light( c ) );
    }
    THEN( "a shader that ignores the light map fails" ) {
        CHECK_FALSE( ignored_passes );
    }
    THEN( "shader that shows everything fully lit fails" ) {
        CHECK_FALSE( full_light_passes );
    }
}

TEST_CASE( "lit_probe_tells_standing_sprites_from_ground", "[smooth_lighting]" )
{
    using namespace cata_shader::lit_probe;
    const std::vector<probe_case> all = cases();
    for( const std::string name : {
             "ortho standing", "iso standing"
         } ) {
        CAPTURE( name );
        const probe_case &standing = probe_case_named( all, name );
        probe_case ground = standing;
        for( smooth_lighting::lit_coords &k : ground.corners ) {
            k.column -= smooth_lighting::standing_marker;
        }
        CHECK_FALSE( matches( expected( standing ), expected( ground ) ) );
        if( standing.frame.iso ) {
            probe_case ortho = standing;
            ortho.frame.iso = false;
            CHECK_FALSE( matches( expected( standing ), expected( ortho ) ) );
        }
    }
}

TEST_CASE( "lit_probe_seam_cases_agree", "[smooth_lighting]" )
{
    using namespace cata_shader::lit_probe;
    const std::vector<probe_case> all = cases();
    CHECK( matches( expected( probe_case_named( all, "seam, left cell" ) ),
                    expected( probe_case_named( all, "seam, right cell" ) ) ) );
}

// `c` with every light texel of every level out of sight
static cata_shader::lit_probe::probe_case unseen( cata_shader::lit_probe::probe_case c )
{
    const int width = 2 * c.columns;
    for( int y = 0; y < c.rows_per_level * c.levels; ++y ) {
        for( int x = 0; x < c.columns; ++x ) {
            c.texels[static_cast<size_t>( y ) * width + x] = smooth_lighting::lightmap_texel();
        }
    }
    return c;
}

TEST_CASE( "lit_probe_cases_reach_each_visual_input", "[smooth_lighting]" )
{
    using namespace cata_shader::lit_probe;
    const std::vector<probe_case> all = cases();
    GIVEN( "light varying both ways across a square quad" ) {
        const probe_case &c = probe_case_named( all, "coordinates across a square" );
        const std::vector<rgb> want = expected( c );
        REQUIRE( want.size() == 16 );
        THEN( "pixels change down a column and along a row" ) {
            CHECK_FALSE( matches( { want[0] }, { want[12] } ) );
            CHECK_FALSE( matches( { want[0] }, { want[3] } ) );
        }
    }
    GIVEN( "quad running into the edge of sight" ) {
        const probe_case &c = probe_case_named( all, "into the edge of sight" );
        const std::vector<rgb> want = expected( c );
        const std::vector<rgb> memory = expected( unseen( c ) );
        REQUIRE( want.size() == 4 );
        THEN( "its far end is part way between seen and memory" ) {
            CHECK_FALSE( matches( { want[0] }, { want[3] } ) );
            CHECK_FALSE( matches( { memory[3] }, { want[3] } ) );
        }
    }
    GIVEN( "translucent colored sprite" ) {
        const probe_case &c = probe_case_named( all, "translucent sprite" );
        THEN( "it differs from the same sprite opaque" ) {
            CHECK_FALSE( matches( expected( c ), expected( probe_case_named( all,
                                  "colored sprite, half light" ) ) ) );
        }
    }
    GIVEN( "corner of a full size level between red levels" ) {
        const probe_case &c = probe_case_named( all, "corner of a packed level" );
        const std::vector<rgb> want = expected( c );
        REQUIRE( want.size() == 1 );
        THEN( "none of the red reaches it" ) {
            CHECK( std::abs( want[0].r - want[0].g ) <= 1 );
            CHECK( std::abs( want[0].r - want[0].b ) <= 1 );
        }
    }
}

TEST_CASE( "lit_begin_outcomes_map_to_frame_actions", "[smooth_lighting]" )
{
    using cata_shader::lit_begin_outcome;
    using smooth_lighting::lit_frame_action;
    CHECK( cata_shader::action_for( lit_begin_outcome::active ) == lit_frame_action::draw_lit );
    CHECK( cata_shader::action_for( lit_begin_outcome::classic ) == lit_frame_action::draw_classic );
    CHECK( cata_shader::action_for( lit_begin_outcome::failed ) == lit_frame_action::draw_classic );
    CHECK( cata_shader::action_for( lit_begin_outcome::abort_frame ) == lit_frame_action::abort_frame );
}

TEST_CASE( "smooth_lighting_frames_on_the_software_renderer", "[smooth_lighting][tiles]" )
{
    policy_fixture fx;
    if( !fx.available_ ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    WHEN( "smooth lighting requested" ) {
        override_option mode( "LIGHTING_MODE", "smooth" );
        fx.draw();
        THEN( "the frame draws classic, saying there is no shader path" ) {
            CHECK( fx.tiles_->effective_lighting() ==
                   smooth_lighting::lighting_status::classic_no_shader_path );
            CHECK_FALSE( display_buffer_scope_recovery_pending() );
        }
    }
    WHEN( "classic lighting requested" ) {
        override_option mode( "LIGHTING_MODE", "classic" );
        fx.draw();
        THEN( "the frame says so" ) {
            CHECK( fx.tiles_->effective_lighting() == smooth_lighting::lighting_status::classic_by_option );
        }
    }
}

#endif // TILES
