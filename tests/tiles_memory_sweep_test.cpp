#if defined(TILES)

#include <memory>
#include <string>

#include "avatar.h"
#include "cata_catch.h"
#include "cata_tiles.h"
#include "coordinates.h"
#include "game.h"
#include "map.h"
#include "map_helpers_tests.h"
#include "map_memory.h"
#include "player_helpers.h"
#include "point.h"
#include "sdl_renderer_recovery.h"
#include "type_id.h"

static const furn_str_id furn_f_chair( "f_chair" );

static const ter_str_id ter_t_grass( "t_grass" );
static const ter_str_id ter_t_wall( "t_wall" );

namespace
{

// avatar is mid-bubble, viewport a few tiles wide, so `far` is inside the
// bubble, in plain view, and off screen
constexpr int view_tiles = 5;
const tripoint_bub_ms player_pos( 65, 65, 0 );
const tripoint_bub_ms far( 85, 65, 0 );

struct sweep_fixture {
    sweep_fixture() : available_( fx_.available() ) {
        if( !available_ ) {
            return;
        }
        clear_avatar();
        g->place_player( player_pos );
        build_test_map( ter_t_grass.id() );
        set_time_to_day();
        map &here = get_map();
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, true );
        // set_time_to_day refreshes visibility cache before map cache is built,
        // leaving it blank on a fresh map
        here.invalidate_visibility_cache();
        here.update_visibility_cache( 0 );
        const std::shared_ptr<const tileset> ts =
            renderer_recovery_test_support::install_synthetic_bundle_with_highlight(
                "sweep_ts", "color_pixel_darken", 1, 1 );
        REQUIRE( ts );
        tiles_ = renderer_recovery_test_support::make_test_tiles( ts );
    }

    bool available() const {
        return available_;
    }

    void draw() const {
        renderer_recovery_test_support::draw_test_map( *tiles_, get_avatar().pos_bub(), view_tiles,
                view_tiles );
    }

    static const memorized_tile &memory_at( const tripoint_bub_ms &p ) {
        return get_avatar().get_memorized_tile( get_map().get_abs( p ) );
    }

    software_render_fixture fx_;
    bool available_ = false;
    std::unique_ptr<cata_tiles> tiles_;
};

} // namespace

TEST_CASE( "tiles_idle_redraw_memorizes_an_offscreen_terrain_change", "[tiles][map_memory]" )
{
    sweep_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    GIVEN( "drawn frame" ) {
        fx.draw();
        WHEN( "off-screen tile becomes a wall, map redraws with no turn" ) {
            get_map().ter_set( far, ter_t_wall.id() );
            fx.draw();
            THEN( "the wall is memorized" ) {
                CHECK( sweep_fixture::memory_at( far ).get_ter_id() == ter_t_wall.str() );
            }
        }
    }
}

TEST_CASE( "tiles_idle_redraw_memorizes_an_offscreen_furniture_change", "[tiles][map_memory]" )
{
    sweep_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    GIVEN( "drawn frame" ) {
        fx.draw();
        WHEN( "off-screen tile gets a chair, map redraws with no turn" ) {
            get_map().furn_set( far, furn_f_chair.id() );
            fx.draw();
            THEN( "chair is memorized" ) {
                CHECK( sweep_fixture::memory_at( far ).get_dec_id() == furn_f_chair.str() );
            }
        }
    }
}

TEST_CASE( "tiles_idle_redraw_skips_the_memorize_sweep", "[tiles][map_memory]" )
{
    sweep_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const tripoint_abs_ms far_abs = get_map().get_abs( far );
    GIVEN( "a drawn frame and a probe id written into an off-screen tile's memory" ) {
        fx.draw();
        REQUIRE( sweep_fixture::memory_at( far ).get_ter_id() == ter_t_grass.str() );
        get_avatar().memorize_terrain( far_abs, "t_sweep_probe", 0, 0 );
        WHEN( "map redraws with nothing changed" ) {
            fx.draw();
            THEN( "probe survives, so no sweep ran" ) {
                CHECK( sweep_fixture::memory_at( far ).get_ter_id() == "t_sweep_probe" );
            }
            AND_WHEN( "draw points marked dirty, as a new turn does, and it redraws" ) {
                fx.tiles_->set_draw_cache_dirty();
                fx.draw();
                THEN( "the sweep restores the real terrain" ) {
                    CHECK( sweep_fixture::memory_at( far ).get_ter_id() == ter_t_grass.str() );
                }
            }
        }
        WHEN( "avatar moves one tile, map redraws" ) {
            g->place_player( player_pos + tripoint_rel_ms::east );
            fx.draw();
            THEN( "the moved bubble is swept" ) {
                CHECK( sweep_fixture::memory_at( far ).get_ter_id() == ter_t_grass.str() );
            }
        }
    }
}

#endif // TILES
