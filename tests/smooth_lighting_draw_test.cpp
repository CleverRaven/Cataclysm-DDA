#if defined(TILES)

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "atlas_bake_plan.h"
#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "cata_tiles.h"
#include "coordinates.h"
#include "creature.h"
#include "cursesdef.h"
#include "game.h"
#include "map.h"
#include "map_helpers_tests.h"
#include "path_info.h"
#include "player_helpers.h"
#include "point.h"
#include "sdl_renderer_recovery.h"
#include "sdltiles.h"
#include "type_id.h"

static const mtype_id mon_zombie( "mon_zombie" );

static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_wall( "t_wall" );

namespace
{

const tripoint_bub_ms player_pos( 65, 65, 0 );
constexpr int view_tiles = 9;

using light_log = std::vector<std::pair<std::string, draw_light>>;

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
    policy_fixture() : available_( fx_.available() ) {
        if( !available_ ) {
            return;
        }
        clear_avatar();
        g->place_player( player_pos );
        build_test_map( ter_t_grass.id() );
        set_time_to_day();
        refresh_caches();
        const std::shared_ptr<const tileset> ts =
            renderer_recovery_test_support::install_synthetic_bundle( "policy_ts", "color_pixel_darken",
        1, 1, atlas_bake_plan{}, true, {
            ter_t_grass.str(), ter_t_wall.str(), mon_zombie.str(), "lighting_hidden"
        } );
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
        for( const std::pair<std::string, draw_light> &e : log_ ) {
            if( e.first == id && e.second == light ) {
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

#endif // TILES
