#if defined(TILES)

#include <array>
#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <type_traits>

#include "cata_catch.h"
#include "cata_scope_helpers.h"
#include "path_info.h"
#include "point.h"
#include "sdl_font.h"
#include "sdl_renderer_recovery.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"

namespace
{

using palette_t = std::remove_reference_t<decltype( windowsPalette )>;

Uint32 rgb_of( const SDL_Color &c )
{
    return ( static_cast<Uint32>( c.r ) << 16 ) | ( static_cast<Uint32>( c.g ) << 8 ) | c.b;
}

} // namespace

TEST_CASE( "software_renderer_draws_text_without_the_atlas", "[tiles][sdl_font]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    REQUIRE( renderer_recovery_test_support::install_test_font(
                 PATH_INFO::fontdir() + "unifont.ttf", 8, 16, 16, false ) );
    Font &font = *renderer_recovery_test_support::test_font();
    const CachedTTFFont *ttf = dynamic_cast<const CachedTTFFont *>( &font );
    REQUIRE( ttf != nullptr );
    GIVEN( "glyph drawn in two colors on the software renderer" ) {
        font.OutputChar( get_sdl_renderer(), "A", point::zero, 3 );
        font.OutputChar( get_sdl_renderer(), "A", point::zero, 12 );
        THEN( "each color gets its own texture, atlas stays empty" ) {
            CHECK( ttf->immediate_glyph_count() == 2 );
            CHECK( ttf->atlas_glyph_count() == 0 );
        }
    }
}

TEST_CASE( "ttf_atlas_holds_one_entry_per_codepoint", "[tiles][sdl_font]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    override_text_atlas( true );
    on_out_of_scope restore_atlas( []() {
        override_text_atlas( std::nullopt );
    } );
    restore_on_out_of_scope<palette_t> restore_palette( windowsPalette );
    for( size_t i = 0; i < windowsPalette.size(); ++i ) {
        windowsPalette[i] = SDL_Color{ static_cast<Uint8>( 10 * i ), static_cast<Uint8>( 200 - 9 * i ),
                                       static_cast<Uint8>( 40 + 13 * i ), 0xFF };
    }
    REQUIRE( renderer_recovery_test_support::install_test_font(
                 PATH_INFO::fontdir() + "unifont.ttf", 8, 16, 16, false ) );
    Font &font = *renderer_recovery_test_support::test_font();
    const CachedTTFFont *ttf = dynamic_cast<const CachedTTFFont *>( &font );
    REQUIRE( ttf != nullptr );

    GIVEN( "one glyph drawn in all 16 colors" ) {
        for( int color = 0; color < 16; ++color ) {
            CAPTURE( color );
            font.OutputChar( get_sdl_renderer(), "A", point::zero, static_cast<unsigned char>( color ) );
        }
        THEN( "atlas has only one entry" ) {
            CHECK( ttf->atlas_glyph_count() == 1 );
        }
        WHEN( "release gpu resources" ) {
            font.release_gpu_resources();
            THEN( "atlas empty" ) {
                CHECK( ttf->atlas_glyph_count() == 0 );
            }
            AND_WHEN( "the glyph is drawn in color 12 over a sentinel" ) {
                const SDL_Color sentinel = { 0xFF, 0x00, 0xFF, 0xFF };
                SetRenderDrawColor( get_sdl_renderer(), sentinel.r, sentinel.g, sentinel.b, sentinel.a );
                RenderClear( get_sdl_renderer() );
                font.OutputChar( get_sdl_renderer(), "A", point::zero, 12 );
                Uint32 px[8 * 16] = {};
                const SDL_Rect cell = { 0, 0, 8, 16 };
                REQUIRE( RenderReadPixels( get_sdl_renderer(), &cell, SDL_PIXELFORMAT_ARGB8888, px, 8 * 4 ) );
                int painted = 0;
                int foreign = 0;
                for( const Uint32 p : px ) {
                    const Uint32 rgb = p & 0x00FFFFFF;
                    painted += rgb == rgb_of( windowsPalette[12] ) ? 1 : 0;
                    foreign += rgb != rgb_of( windowsPalette[12] ) && rgb != rgb_of( sentinel ) ? 1 : 0;
                }
                THEN( "repopulates lazily, paints only palette color 12" ) {
                    CHECK( ttf->atlas_glyph_count() == 1 );
                    CHECK( painted > 0 );
                    CHECK( foreign == 0 );
                }
            }
        }
    }
}

TEST_CASE( "ttf_atlas_retries_a_page_that_failed_to_create", "[tiles][sdl_font]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    REQUIRE( renderer_recovery_test_support::install_test_font(
                 PATH_INFO::fontdir() + "unifont.ttf", 8, 16, 16, false ) );
    Font &font = *renderer_recovery_test_support::test_font();
    const CachedTTFFont *ttf = dynamic_cast<const CachedTTFFont *>( &font );
    REQUIRE( ttf != nullptr );
    override_text_atlas( true );
    on_out_of_scope disarm( []() {
        arm_glyph_page_create_failures( 0 );
        override_text_atlas( std::nullopt );
    } );

    GIVEN( "first atlas page creation fails" ) {
        arm_glyph_page_create_failures( 1 );
        font.OutputChar( get_sdl_renderer(), "B", point::zero, 7 );
        THEN( "nothing cached, no page left behind" ) {
            CHECK( ttf->atlas_glyph_count() == 0 );
            CHECK( ttf->atlas_page_count() == 0 );
        }
        WHEN( "the glyph is drawn again" ) {
            font.OutputChar( get_sdl_renderer(), "B", point::zero, 7 );
            THEN( "page created and glyph cached" ) {
                CHECK( ttf->atlas_glyph_count() == 1 );
                CHECK( ttf->atlas_page_count() == 1 );
            }
        }
    }
}

#endif // TILES
