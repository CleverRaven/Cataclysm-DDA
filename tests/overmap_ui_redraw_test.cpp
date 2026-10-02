#include <string>
#include <vector>

#include "cata_catch.h"
#include "coordinates.h"
#include "overmap_ui.h"
#include "point.h"

#if defined(TILES)
#include <memory>

#include "avatar.h"
#include "cata_tiles.h"
#include "sdl_renderer_recovery.h"
#endif

TEST_CASE( "overmap_map_redraw_needed_only_when_view_changes", "[overmap]" )
{
    const overmap_ui::map_view_state drawn{ tripoint_abs_omt( 10, 20, 0 ), true };
    GIVEN( "TIMEOUT pass" ) {
        WHEN( "cursor and blink phase are unchanged, nothing animates" ) {
            THEN( "the map keeps its pixels" ) {
                CHECK_FALSE( overmap_ui::map_redraw_needed( "TIMEOUT", drawn, drawn, false ) );
            }
        }
        WHEN( "edge scroll or path replay moved the cursor" ) {
            const overmap_ui::map_view_state now{ tripoint_abs_omt( 11, 20, 0 ), true };
            THEN( "the map is redrawn" ) {
                CHECK( overmap_ui::map_redraw_needed( "TIMEOUT", drawn, now, false ) );
            }
        }
        WHEN( "blink phase flipped" ) {
            const overmap_ui::map_view_state now{ drawn.cursor, false };
            THEN( "the map is redrawn" ) {
                CHECK( overmap_ui::map_redraw_needed( "TIMEOUT", drawn, now, false ) );
            }
        }
        WHEN( "overmap tiles are animated" ) {
            THEN( "the map is redrawn" ) {
                CHECK( overmap_ui::map_redraw_needed( "TIMEOUT", drawn, drawn, true ) );
            }
        }
    }
    GIVEN( "any other action with an unchanged view" ) {
        const std::vector<std::string> actions = {
            "MOUSE_MOVE", "CONFIRM", "ANY_INPUT", "zoom_in", "TOGGLE_BLINKING", "SEARCH"
        };
        for( const std::string &action : actions ) {
            CAPTURE( action );
            CHECK( overmap_ui::map_redraw_needed( action, drawn, drawn, false ) );
        }
    }
}

#if defined(TILES)
TEST_CASE( "draw_om_clears_the_animated_flag_on_a_static_view", "[tiles][overmap]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const std::shared_ptr<const tileset> ts =
        renderer_recovery_test_support::install_synthetic_bundle_with_highlight(
            "overmap_anim_ts", "color_pixel_darken", 1, 1 );
    REQUIRE( ts );
    const std::unique_ptr<cata_tiles> om = renderer_recovery_test_support::make_test_tiles( ts );
    GIVEN( "the overmap context drew an animated tile last frame" ) {
        renderer_recovery_test_support::set_has_animated_tiles( *om, true );
        WHEN( "draws a view with no animated tile" ) {
            renderer_recovery_test_support::draw_test_overmap( *om, get_avatar().pos_abs_omt() );
            THEN( "it doesn't report animation anymore" ) {
                CHECK_FALSE( om->has_animated_tiles() );
            }
        }
    }
}
#endif // TILES
