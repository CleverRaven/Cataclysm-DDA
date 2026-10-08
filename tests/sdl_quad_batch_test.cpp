#if defined(TILES)

#include "cata_catch.h"
#include "point.h"
#include "sdl_quad_batch.h"
#include "sdl_renderer_recovery.h"
#include "sdl_utils.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"

namespace
{

Uint32 probe_rgb( const point &px )
{
    const SDL_Rect rect = { px.x, px.y, 1, 1 };
    Uint32 value = 0;
    REQUIRE( RenderReadPixels( get_sdl_renderer(), &rect, SDL_PIXELFORMAT_ARGB8888, &value, 4 ) );
    return value & 0x00FFFFFF;
}

Uint32 rgb_of( const SDL_Color &c )
{
    return ( static_cast<Uint32>( c.r ) << 16 ) | ( static_cast<Uint32>( c.g ) << 8 ) | c.b;
}

// 8x8 texture: left half opaque white, right half clear.
SDL_Texture_Ptr half_white_texture()
{
    SDL_Surface_Ptr s = create_surface_32( 8, 8 );
    const SDL_Rect left = { 0, 0, 4, 8 };
    FillRect( s, &left, MapRGBA( s, 255, 255, 255, 255 ) );
    SDL_Texture_Ptr t = CreateTexture( get_sdl_renderer(), SDL_PIXELFORMAT_RGBA32,
                                       SDL_TEXTUREACCESS_STATIC, 8, 8 );
    REQUIRE( t );
    SetTextureBlendMode( t, SDL_BLENDMODE_BLEND );
    REQUIRE( UpdateTexture( t, SDL_Rect{ 0, 0, 8, 8 }, s ) );
    return t;
}

} // namespace

TEST_CASE( "quad_batch_interleaves_position_color_and_uv", "[tiles][sdl_font]" )
{
    GIVEN( "empty batch" ) {
        quad_batch batch;
        WHEN( "textured quad appended from texel rect 4,0 4x8 of an 8x8 texture" ) {
            batch.append_textured_quad( SDL_Rect{ 10, 20, 4, 8 }, SDL_Rect{ 4, 0, 4, 8 }, 8, 8,
                                        SDL_FColor{ 1.0f, 0.5f, 0.25f, 1.0f } );
            THEN( "corners run clockwise with exact uv edges" ) {
                REQUIRE( batch.vertex_count() == 4 );
                REQUIRE( batch.index_count() == 6 );
                const SDL_Vertex *v = batch.vertex_data();
                CHECK( v[0].position.x == 10.0f );
                CHECK( v[2].position.y == 28.0f );
                CHECK( v[0].tex_coord.x == 0.5f );
                CHECK( v[1].tex_coord.x == 1.0f );
                CHECK( v[2].tex_coord.y == 1.0f );
                CHECK( v[3].color.g == 0.5f );
            }
        }
    }
}

TEST_CASE( "quad_batch_clear_keeps_its_buffers", "[tiles][sdl_font]" )
{
    GIVEN( "batch that held 64 quads" ) {
        quad_batch batch;
        for( int i = 0; i < 64; ++i ) {
            batch.append_quad( 0.0f, 0.0f, 1.0f, 1.0f, SDL_FColor{ 1.0f, 1.0f, 1.0f, 1.0f } );
        }
        const SDL_Vertex *before = batch.vertex_data();
        WHEN( "it's cleared and refilled with as many quads" ) {
            batch.clear();
            for( int i = 0; i < 64; ++i ) {
                batch.append_quad( 0.0f, 0.0f, 1.0f, 1.0f, SDL_FColor{ 1.0f, 1.0f, 1.0f, 1.0f } );
            }
            THEN( "vertex storage was not reallocated" ) {
                CHECK( batch.vertex_data() == before );
            }
        }
    }
}

TEST_CASE( "text_batch_draws_rects_then_glyphs_modulated_by_vertex_color", "[tiles][sdl_font]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const SDL_Color glyph_color = { 196, 180, 30, 255 };
    const SDL_Color rect_color = { 0, 0, 196, 255 };
    GIVEN( "white texel glyph queued before a rect covering it" ) {
        SDL_Texture_Ptr tex = half_white_texture();
        text_batch batch;
        batch.add_glyph( tex.get(), 8, 8, SDL_Rect{ 0, 0, 8, 8 }, SDL_Rect{ 0, 0, 8, 8 },
                         to_fcolor( glyph_color ) );
        batch.add_rect( SDL_Rect{ 0, 0, 8, 8 }, rect_color );
        WHEN( "the batch flushes" ) {
            batch.flush( get_sdl_renderer() );
            THEN( "glyph lands on the rect in exactly the vertex color" ) {
                CHECK( probe_rgb( point::south_east ) == rgb_of( glyph_color ) );
            }
            THEN( "clear texels leave the rect showing" ) {
                CHECK( probe_rgb( point( 6, 1 ) ) == rgb_of( rect_color ) );
            }
            THEN( "batch is empty again" ) {
                CHECK( batch.empty() );
            }
        }
    }
    GIVEN( "glyph taken from clear right half" ) {
        SDL_Texture_Ptr tex = half_white_texture();
        SetRenderDrawColor( get_sdl_renderer(), 0, 0, 0, 255 );
        RenderClear( get_sdl_renderer() );
        text_batch batch;
        batch.add_glyph( tex.get(), 8, 8, SDL_Rect{ 0, 0, 4, 8 }, SDL_Rect{ 4, 0, 4, 8 },
                         to_fcolor( glyph_color ) );
        WHEN( "the batch flushes" ) {
            batch.flush( get_sdl_renderer() );
            THEN( "no texel of the left half bleeds in" ) {
                for( int x = 0; x < 4; ++x ) {
                    CAPTURE( x );
                    CHECK( probe_rgb( point( x, 3 ) ) == 0u );
                }
            }
        }
    }
}

#endif // TILES
