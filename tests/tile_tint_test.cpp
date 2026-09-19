#include <optional>
#include <string>

#include "cata_catch.h"
#include "lightmap.h"
#include "tile_tint.h"

TEST_CASE( "compute_tile_tint_white_light_has_no_tint", "[tint_overlay]" )
{
    const light_color_rgb white{ 1.0f, 1.0f, 1.0f };
    CHECK_FALSE( compute_tile_tint( white, 1.0f ).has_value() );
}

TEST_CASE( "compute_tile_tint_saturated_light_in_darkness_caps_at_80", "[tint_overlay]" )
{
    const light_color_rgb red{ 2.0f, 0.0f, 0.0f };
    const std::optional<tile_tint> t = compute_tile_tint( red, 1.0f );
    REQUIRE( t );
    CHECK( t->r == 255 );
    CHECK( t->g == 0 );
    CHECK( t->b == 0 );
    CHECK( t->a == 80 );
}

TEST_CASE( "compute_tile_tint_bright_ambient_washes_out", "[tint_overlay]" )
{
    const light_color_rgb red{ 0.5f, 0.0f, 0.0f };
    // ratio = 0.5 / 50 = 0.01. alpha rounds down to 0, no tint.
    CHECK_FALSE( compute_tile_tint( red, 50.0f ).has_value() );
}

TEST_CASE( "compute_tile_tint_no_scalar_light_has_no_tint", "[tint_overlay]" )
{
    const light_color_rgb red{ 1.0f, 0.0f, 0.0f };
    CHECK_FALSE( compute_tile_tint( red, 0.05f ).has_value() );
}

TEST_CASE( "tint_texture_mod_encodes_strength_in_inverted_alpha", "[tint_overlay]" )
{
    const tile_tint t{ 255, 0, 0, 80 };
    const tint_texture_mod m = tint_texture_mod_for( t );
    CHECK( m.r == 255 );
    CHECK( m.g == 0 );
    CHECK( m.b == 0 );
    CHECK( m.a == 175 );
    const tint_texture_mod none = tint_texture_mod_none();
    CHECK( none.r == 255 );
    CHECK( none.g == 255 );
    CHECK( none.b == 255 );
    CHECK( none.a == 255 );
}

#if defined(TILES)

#include "cata_shader.h"
#include "point.h"
#include "sdl_renderer_recovery.h"
#include "sdl_utils.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"

TEST_CASE( "variant_pass_tinted_normal_uses_the_atlas_on_software_renderer", "[tiles][gpu]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    REQUIRE( vp );
    CHECK( vp->try_begin( cata_shader::variant_kind::NORMAL, true ) ==
           cata_shader::variant_pass::begin_result::use_atlas );
    CHECK_FALSE( vp->tint_available() );
    CHECK( vp->flush() );
}

// 2x1 strip: opaque white on the left, transparent on the right. asymmetric, so
// a flip or rotation moves the opaque texel to a different destination
static SDL_Texture_Ptr make_left_opaque_2x1_texture()
{
    SDL_Surface_Ptr surf = create_surface_32( 2, 1 );
    REQUIRE( surf );
    const SDL_Rect left = { 0, 0, 1, 1 };
    const SDL_Rect right = { 1, 0, 1, 1 };
    REQUIRE( FillRect( surf, &left, MapRGBA( surf, 255, 255, 255, 255 ) ) == 0 );
    REQUIRE( FillRect( surf, &right, MapRGBA( surf, 255, 255, 255, 0 ) ) == 0 );
    SDL_Texture_Ptr tex = CreateTextureFromSurface( get_sdl_renderer(), surf );
    REQUIRE( tex );
    return tex;
}

static Uint32 probe_argb( const point &px )
{
    const SDL_Rect probe = { px.x, px.y, 1, 1 };
    Uint32 argb = 0;
    REQUIRE( RenderReadPixels( get_sdl_renderer(), &probe, SDL_PIXELFORMAT_ARGB8888, &argb, 4 ) );
    return argb & 0x00FFFFFF;
}

TEST_CASE( "software_renderer_color_mod_reaches_render_texture", "[tiles][tint_overlay]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    SDL_Texture_Ptr tex = make_left_opaque_2x1_texture();
    SetRenderDrawColor( get_sdl_renderer(), 0, 0, 0, 255 );
    RenderClear( get_sdl_renderer() );
    SetTextureColorMod( tex, 255, 0, 0 );
    const SDL_Rect dst = { 0, 0, 2, 1 };

    SECTION( "unrotated draw" ) {
        RenderCopyEx( get_sdl_renderer(), tex.get(), nullptr, &dst, 0, nullptr, SDL_FLIP_NONE );
        CHECK( probe_argb( point::zero ) == 0x00FF0000 );
        // transparent source pixel leaves the clear color
        CHECK( probe_argb( point::east ) == 0x00000000 );
    }
    SECTION( "horizontal flip takes the rotated path" ) {
        RenderCopyEx( get_sdl_renderer(), tex.get(), nullptr, &dst, 0, nullptr, SDL_FLIP_HORIZONTAL );
        CHECK( probe_argb( point::east ) == 0x00FF0000 );
        CHECK( probe_argb( point::zero ) == 0x00000000 );
    }
    SECTION( "ninety degree rotation takes the rotated path" ) {
        // over a 2x2 rect the opaque texel first covers the left column.
        // positive angle rotates clockwise about the destination center (1, 1),
        // so the left column becomes the top row.
        const SDL_Rect square = { 0, 0, 2, 2 };
        RenderCopyEx( get_sdl_renderer(), tex.get(), nullptr, &square, 90, nullptr, SDL_FLIP_NONE );
        CHECK( probe_argb( point::zero ) == 0x00FF0000 );
        CHECK( probe_argb( point::east ) == 0x00FF0000 );
        CHECK( probe_argb( point::south ) == 0x00000000 );
        CHECK( probe_argb( point::south_east ) == 0x00000000 );
    }
    SECTION( "mod is captured per draw" ) {
        RenderCopyEx( get_sdl_renderer(), tex.get(), nullptr, &dst, 0, nullptr, SDL_FLIP_NONE );
        SetTextureColorMod( tex, 0, 255, 0 );
        const SDL_Rect dst2 = { 0, 1, 2, 1 };
        RenderCopyEx( get_sdl_renderer(), tex.get(), nullptr, &dst2, 0, nullptr, SDL_FLIP_NONE );
        CHECK( probe_argb( point::zero ) == 0x00FF0000 );
        CHECK( probe_argb( point::south ) == 0x0000FF00 );
    }
}

#endif // TILES
