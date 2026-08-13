#if defined(TILES)

#include <algorithm>

#include "cata_catch.h"
#include "pixel_minimap_geometry.h"
#include "point.h"
#include "sdl_renderer_recovery.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"

TEST_CASE( "render_read_pixels_honors_requested_format", "[tiles][pixel_minimap]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    GIVEN( "the render target cleared to pure red" ) {
        SetRenderDrawColor( get_sdl_renderer(), 255, 0, 0, 255 );
        RenderClear( get_sdl_renderer() );
        WHEN( "one pixel is read back as ARGB8888 and as ABGR8888" ) {
            // One of the two requested formats always differs from the
            // backend's native format, so at least one read exercises a
            // real conversion.
            const SDL_Rect probe = { 0, 0, 1, 1 };
            Uint32 argb = 0;
            Uint32 abgr = 0;
            REQUIRE( RenderReadPixels( get_sdl_renderer(), &probe,
                                       SDL_PIXELFORMAT_ARGB8888, &argb, 4 ) );
            REQUIRE( RenderReadPixels( get_sdl_renderer(), &probe,
                                       SDL_PIXELFORMAT_ABGR8888, &abgr, 4 ) );
            THEN( "red occupies each format's own red channel position" ) {
                // Packed-32 formats are defined on the Uint32 value, so
                // shifts are endian-safe.
                CHECK( ( ( argb >> 16 ) & 0xFF ) == 255 );
                CHECK( ( argb & 0xFF ) == 0 );
                CHECK( ( abgr & 0xFF ) == 255 );
                CHECK( ( ( abgr >> 16 ) & 0xFF ) == 0 );
            }
        }
    }
}

TEST_CASE( "minimap_batch_quad_layout", "[tiles][pixel_minimap]" )
{
    GIVEN( "an empty batch" ) {
        minimap_vertex_batch batch;
        WHEN( "one quad is appended" ) {
            batch.append_quad( 2.0f, 3.0f, 4.0f, 5.0f, SDL_FColor{ 1.0f, 0.5f, 0.25f, 1.0f } );
            THEN( "it holds 4 vertices and 6 indices" ) {
                CHECK( batch.vertex_count() == 4 );
                CHECK( batch.index_count() == 6 );
            }
            THEN( "corners are in CW fan order with the shared color" ) {
                // (x,y) (x+w,y) (x+w,y+h) (x,y+h)
                const float *xy = batch.xy_data();
                CHECK( xy[0] == 2.0f );
                CHECK( xy[1] == 3.0f );
                CHECK( xy[2] == 6.0f );
                CHECK( xy[3] == 3.0f );
                CHECK( xy[4] == 6.0f );
                CHECK( xy[5] == 8.0f );
                CHECK( xy[6] == 2.0f );
                CHECK( xy[7] == 8.0f );
                const Uint32 *idx = batch.index_data();
                const Uint32 expected[6] = { 0, 1, 2, 0, 2, 3 };
                CHECK( std::equal( idx, idx + 6, expected ) );
                CHECK( batch.color_data()[0].g == 0.5f );
            }
        }
    }
}

TEST_CASE( "minimap_batch_index_invariants", "[tiles][pixel_minimap]" )
{
    // SDL_RenderGeometryRaw rejects count % 3 != 0 and OOB indices at the
    // API boundary; enforce the contract on our side.
    GIVEN( "a batch of several quads" ) {
        minimap_vertex_batch batch;
        for( int i = 0; i < 7; ++i ) {
            batch.append_quad( i * 2.0f, 0.0f, 1.0f, 1.0f, SDL_FColor{ 0, 0, 0, 1.0f } );
        }
        THEN( "index count is a triangle multiple and indices stay in range" ) {
            REQUIRE( batch.index_count() % 3 == 0 );
            const Uint32 *idx = batch.index_data();
            const Uint32 max_index = *std::max_element( idx, idx + batch.index_count() );
            CHECK( max_index == static_cast<Uint32>( batch.vertex_count() - 1 ) );
        }
    }
    GIVEN( "a batch pushed past the 16-bit vertex limit" ) {
        // 16500 quads is 66000 vertices, past the 16-bit index range.
        minimap_vertex_batch batch;
        for( int i = 0; i < 16500; ++i ) {
            const int col = i % 256;
            const int row = i / 256;
            batch.append_quad( static_cast<float>( col ), static_cast<float>( row ),
                               1.0f, 1.0f, SDL_FColor{ 0, 0, 0, 1.0f } );
        }
        THEN( "the last indices exceed 65535 and still address their vertices" ) {
            REQUIRE( batch.vertex_count() == 66000 );
            const Uint32 *idx = batch.index_data();
            const Uint32 max_index = *std::max_element( idx, idx + batch.index_count() );
            CHECK( max_index == 65999 );
        }
    }
}

TEST_CASE( "minimap_to_fcolor_converts_channels", "[tiles][pixel_minimap]" )
{
    GIVEN( "an 8-bit color with distinct channel values" ) {
        WHEN( "it is converted to SDL_FColor" ) {
            const SDL_FColor c = to_fcolor( SDL_Color{ 255, 128, 0, 255 } );
            THEN( "every channel maps onto [0, 1] by dividing by 255" ) {
                CHECK( c.r == 1.0f );
                CHECK( c.g == Approx( 128.0f / 255.0f ) );
                CHECK( c.b == 0.0f );
                CHECK( c.a == 1.0f );
            }
        }
    }
}

TEST_CASE( "minimap_beacon_pixel_pattern", "[tiles][pixel_minimap]" )
{
    GIVEN( "the beacon diamond for rect {10, 20, w=2, h=2}, edge divisor 3" ) {
        // 13 pixels total for w = h = 2, 8 of them on the edge.
        minimap_vertex_batch batch;
        append_beacon( batch, SDL_Rect{ 10, 20, 2, 2 },
                       SDL_Color{ 210, 90, 30, 255 }, 3 );

        THEN( "13 pixel quads are emitted" ) {
            CHECK( batch.vertex_count() == 13 * 4 );
            CHECK( batch.index_count() == 13 * 6 );
        }
        THEN( "the first pixel (x=-2, y=0) is on-edge: channels divided by 3" ) {
            const SDL_FColor first = batch.color_data()[0];
            CHECK( first.r == Approx( 70.0f / 255.0f ) );
            CHECK( first.g == Approx( 30.0f / 255.0f ) );
            CHECK( first.b == Approx( 10.0f / 255.0f ) );
            const float *xy = batch.xy_data();
            CHECK( xy[0] == 8.0f );   // rect.x + (-2)
            CHECK( xy[1] == 20.0f );  // rect.y + 0
        }
        THEN( "the center pixel (x=0, y=0) keeps the undivided color" ) {
            // Emission order is x-major, y ascending: x = -2 emits 1 pixel,
            // x = -1 emits 3, then x = 0 emits y = -2, -1 before (0,0).
            // Pixels before (0,0): 1 + 3 + 2 = 6.
            const SDL_FColor center = batch.color_data()[6 * 4];
            CHECK( center.r == Approx( 210.0f / 255.0f ) );
        }
    }
}

TEST_CASE( "minimap_transform_screen_mapping", "[tiles][pixel_minimap]" )
{
    GIVEN( "scale_to_fit with a width-constrained exact ratio" ) {
        // native 200x100 into 50x40: factor 0.25, fitted 50x25, centered.
        const minimap_transform tf =
            compute_minimap_transform( point( 200, 100 ), SDL_Rect{ 0, 0, 50, 40 }, true );
        THEN( "dest rect is the fitted centered box with matching scales" ) {
            CHECK( tf.dest_rect.x == 0 );
            CHECK( tf.dest_rect.y == 7 );
            CHECK( tf.dest_rect.w == 50 );
            CHECK( tf.dest_rect.h == 25 );
            CHECK( tf.scale_x == Approx( 0.25f ) );
            CHECK( tf.scale_y == Approx( 0.25f ) );
        }
    }
    GIVEN( "scale_to_fit with a truncating odd ratio" ) {
        // native 482x242 into 100x40: the truncated fit gives different
        // per-axis scales.
        const minimap_transform tf =
            compute_minimap_transform( point( 482, 242 ), SDL_Rect{ 5, 9, 100, 40 }, true );
        THEN( "each axis maps native extent exactly onto its dest extent" ) {
            CAPTURE( tf.dest_rect.w, tf.dest_rect.h );
            CHECK( tf.origin_x + tf.scale_x * 482 == Approx( tf.dest_rect.x + tf.dest_rect.w ) );
            CHECK( tf.origin_y + tf.scale_y * 242 == Approx( tf.dest_rect.y + tf.dest_rect.h ) );
            CHECK( tf.dest_rect.w <= 100 );
            CHECK( tf.dest_rect.h <= 40 );
        }
    }
    GIVEN( "no scale_to_fit, native larger than the screen rect" ) {
        // native 300x300 into 100x100 at offset (10, 20): crop d = 100,
        // dest = screen rect, origin shifted left/up by the crop.
        const minimap_transform tf =
            compute_minimap_transform( point( 300, 300 ), SDL_Rect{ 10, 20, 100, 100 }, false );
        THEN( "scales are 1 and the origin backs out the central crop" ) {
            CHECK( tf.scale_x == 1.0f );
            CHECK( tf.scale_y == 1.0f );
            CHECK( tf.dest_rect.x == 10 );
            CHECK( tf.dest_rect.y == 20 );
            CHECK( tf.dest_rect.w == 100 );
            CHECK( tf.dest_rect.h == 100 );
            CHECK( tf.origin_x == Approx( 10.0f - 100.0f ) );
            CHECK( tf.origin_y == Approx( 20.0f - 100.0f ) );
        }
    }
    GIVEN( "no scale_to_fit, native smaller than the screen rect" ) {
        // native 60x60 into 100x100 at origin: centered, no crop.
        const minimap_transform tf =
            compute_minimap_transform( point( 60, 60 ), SDL_Rect{ 0, 0, 100, 100 }, false );
        THEN( "the map is centered with unit scale" ) {
            CHECK( tf.scale_x == 1.0f );
            CHECK( tf.dest_rect.x == 20 );
            CHECK( tf.dest_rect.y == 20 );
            CHECK( tf.dest_rect.w == 60 );
            CHECK( tf.dest_rect.h == 60 );
            CHECK( tf.origin_x == Approx( 20.0f ) );
            CHECK( tf.origin_y == Approx( 20.0f ) );
        }
    }
}

TEST_CASE( "minimap_render_geometry_raw_smoke", "[tiles][pixel_minimap]" )
{
    // A logged SDL error fails the run under the renderer test policy, so
    // completing without error output is the assertion that matters.
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    minimap_vertex_batch batch;
    batch.append_quad( 0.0f, 0.0f, 8.0f, 8.0f, SDL_FColor{ 0.2f, 0.4f, 0.6f, 1.0f } );
    render_batch( get_sdl_renderer(), batch );
    CHECK( true );
}

#endif // TILES
