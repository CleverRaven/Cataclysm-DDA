#if defined(TILES)

#include <memory>

#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "cached_options.h"
#include "character.h"
#include "coordinates.h"
#include "game.h"
#include "map.h"
#include "map_helpers_tests.h"
#include "pixel_minimap.h"
#include "player_helpers.h"
#include "point.h"
#include "sdl_geometry.h"
#include "sdl_renderer_recovery.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"
#include "type_id.h"

static const ter_str_id ter_t_grass( "t_grass" );

namespace
{

// Characterization suite: pins where pixel_minimap::draw paints.

struct minimap_probe_fixture {
    minimap_probe_fixture() : available_( fx_.available() ) {
        if( !available_ ) {
            return;
        }
        // The fixture window is 64x64 by default; grow it and verify, or
        // the probes read outside the render output. The resize is
        // deferred until the coordinator drains.
        renderer_recovery_test_support::set_scaling_and_resize_window( 1, 256, 256 );
        renderer_coordinator.drain_pending();
        int out_w = 0;
        int out_h = 0;
        GetRendererOutputSize( get_sdl_renderer(), &out_w, &out_h );
        available_ = out_w >= 242 && out_h >= 242;

        // Left at zero the background is transparent and never reaches the
        // target, so a probe cannot tell it from an unpainted pixel.
        pixel_minimap_r = 0x00;
        pixel_minimap_g = 0x00;
        pixel_minimap_b = 0xFF;
        pixel_minimap_a = 0xFF;

        clear_avatar();
        g->place_player( tripoint_bub_ms( 65, 65, 0 ) );
        build_test_map( ter_t_grass.id() );
        set_time_to_day();
        map &here = get_map();
        here.invalidate_map_cache( 0 );
        here.build_map_cache( 0, true );
        // set_time_to_day refreshes the visibility cache before the map
        // cache is built, so on a fresh map it is BLANK everywhere but the
        // player tile and the minimap skips those tiles.
        here.invalidate_visibility_cache();
        here.update_visibility_cache( 0 );
    }

    bool available() const {
        return available_;
    }

    // Alpha is masked off: target alpha semantics vary by backend and the
    // characterization only pins RGB.
    Uint32 probe_rgb( const point &px ) const {
        const SDL_Rect rect = { px.x, px.y, 1, 1 };
        Uint32 value = 0;
        REQUIRE( RenderReadPixels( get_sdl_renderer(), &rect,
                                   SDL_PIXELFORMAT_ARGB8888, &value, 4 ) );
        return value & 0x00FFFFFF;
    }

    static Uint32 rgb_of( const SDL_Color &c ) {
        return ( static_cast<Uint32>( c.r ) << 16 )
               | ( static_cast<Uint32>( c.g ) << 8 )
               | static_cast<Uint32>( c.b );
    }

    static SDL_Color clear_to_sentinel() {
        const SDL_Color sentinel = { 0xFF, 0x00, 0xFF, 0xFF };
        SetRenderDrawColor( get_sdl_renderer(), sentinel.r, sentinel.g, sentinel.b, sentinel.a );
        RenderClear( get_sdl_renderer() );
        return sentinel;
    }

    software_render_fixture fx_;
    GeometryRenderer_Ptr geometry_ = std::make_unique<DefaultGeometryRenderer>();
    bool available_ = false;
    restore_on_out_of_scope<int> restore_bg_r_{ pixel_minimap_r };
    restore_on_out_of_scope<int> restore_bg_g_{ pixel_minimap_g };
    restore_on_out_of_scope<int> restore_bg_b_{ pixel_minimap_b };
    restore_on_out_of_scope<int> restore_bg_a_{ pixel_minimap_a };
};

// 242x242 rect over the 121-tile window: tile size 2, no fit scaling, no
// centering offset.
constexpr SDL_Rect minimap_rect = { 0, 0, 242, 242 };

} // namespace

TEST_CASE( "pixel_minimap_respects_caller_clip", "[tiles][pixel_minimap]" )
{
    minimap_probe_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    GIVEN( "a caller clip covering part of the drawn area" ) {
        const SDL_Color sentinel = minimap_probe_fixture::clear_to_sentinel();
        const SDL_Rect caller_clip = { 116, 116, 6, 10 };
        RenderSetClipRect( get_sdl_renderer(), &caller_clip );

        pixel_minimap minimap( get_sdl_renderer(), fx.geometry_ );
        minimap.set_settings( pixel_minimap_settings() );

        WHEN( "the minimap draws" ) {
            minimap.draw( minimap_rect, get_player_character().pos_bub() );
            THEN( "a pixel inside the clip is painted" ) {
                CHECK( fx.probe_rgb( point( 118, 116 ) )
                       != minimap_probe_fixture::rgb_of( sentinel ) );
            }
            THEN( "a pixel outside the clip is untouched" ) {
                CHECK( fx.probe_rgb( point( 124, 116 ) )
                       == minimap_probe_fixture::rgb_of( sentinel ) );
            }
            THEN( "the caller clip is still in effect afterwards" ) {
                REQUIRE( RenderIsClipEnabled( get_sdl_renderer() ) );
                SDL_Rect after = { 0, 0, 0, 0 };
                RenderGetClipRect( get_sdl_renderer(), &after );
                CHECK( after.x == caller_clip.x );
                CHECK( after.y == caller_clip.y );
                CHECK( after.w == caller_clip.w );
                CHECK( after.h == caller_clip.h );
            }
        }
        RenderSetClipRect( get_sdl_renderer(), nullptr );
    }
}

TEST_CASE( "pixel_minimap_scale_to_fit_stays_inside_dest", "[tiles][pixel_minimap]" )
{
    minimap_probe_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    GIVEN( "scale_to_fit into a wide 240x80 rect over a sentinel-colored canvas" ) {
        const SDL_Color sentinel = minimap_probe_fixture::clear_to_sentinel();

        pixel_minimap minimap( get_sdl_renderer(), fx.geometry_ );
        pixel_minimap_settings settings;
        settings.scale_to_fit = true;
        minimap.set_settings( settings );

        WHEN( "the minimap draws" ) {
            minimap.draw( SDL_Rect{ 0, 0, 240, 80 }, get_player_character().pos_bub() );
            // 121x121 native fitted into 240x80 -> 80x80 centered at x 80.
            THEN( "pixels outside the fitted rect keep the sentinel color" ) {
                CHECK( fx.probe_rgb( point( 10, 40 ) )
                       == minimap_probe_fixture::rgb_of( sentinel ) );
                CHECK( fx.probe_rgb( point( 230, 40 ) )
                       == minimap_probe_fixture::rgb_of( sentinel ) );
            }
            THEN( "pixels inside the fitted rect do not" ) {
                CHECK( fx.probe_rgb( point( 120, 40 ) )
                       != minimap_probe_fixture::rgb_of( sentinel ) );
            }
        }
    }
}

#endif // TILES
