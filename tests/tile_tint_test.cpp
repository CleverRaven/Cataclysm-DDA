#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "lightmap.h"
#include "point.h"
#include "shader_tint_self_test.h"
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

TEST_CASE( "tint_mix_channel_matches_the_tint_shader_formula", "[tint_overlay]" )
{
    // Strength 128/255 moves the mid-gray channel halfway to 255
    CHECK( std::lround( tint_mix_channel( 128.0f, 255.0f, 127 ) ) == 192 );
    CHECK( tint_mix_channel( 40.0f, 200.0f, 255 ) == Approx( 40.0f ) );
    CHECK( tint_mix_channel( 40.0f, 200.0f, 0 ) == Approx( 200.0f ) );
}

static void check_coverage( const point &px, const size_t case_index, const point &texel )
{
    CAPTURE( px.to_string() );
    const std::optional<shader_tint_self_test::coverage> c = shader_tint_self_test::covering_case( px );
    REQUIRE( c );
    CHECK( c->case_index == case_index );
    CHECK( c->texel == texel );
}

TEST_CASE( "shader_tint_self_test_maps_target_pixels_to_source_texels", "[tint_overlay]" )
{
    SECTION( "unrotated case a" ) {
        check_coverage( point( 2, 2 ), 0, point::zero );
        check_coverage( point( 5, 3 ), 0, point( 3, 1 ) );
    }
    SECTION( "unrotated case e drawn second" ) {
        check_coverage( point( 3, 7 ), 1, point::south_east );
    }
    SECTION( "horizontal flip case b mirrors the columns" ) {
        check_coverage( point( 10, 2 ), 2, point( 3, 0 ) );
        check_coverage( point( 13, 3 ), 2, point::south );
    }
    SECTION( "rotated case c turns rows into columns" ) {
        check_coverage( point( 19, 2 ), 3, point::south );
        check_coverage( point( 20, 5 ), 3, point( 3, 0 ) );
    }
    SECTION( "identity case d" ) {
        check_coverage( point( 5, 11 ), 4, point( 3, 1 ) );
    }
    SECTION( "pixels outside every footprint are uncovered" ) {
        CHECK_FALSE( shader_tint_self_test::covering_case( point::zero ) );
        CHECK_FALSE( shader_tint_self_test::covering_case( point( 6, 2 ) ) );
        // inside c's unrotated rect, outside its rotated footprint
        CHECK_FALSE( shader_tint_self_test::covering_case( point( 18, 3 ) ) );
        CHECK_FALSE( shader_tint_self_test::covering_case( point( 21, 3 ) ) );
    }
}

static std::array<int, 3> expected_rgb( const point &px )
{
    const shader_tint_self_test::rgba c = shader_tint_self_test::expected_pixel( px );
    return { c.r, c.g, c.b };
}

TEST_CASE( "shader_tint_self_test_expects_the_tint_mix_blended_over_the_clear_color",
           "[tint_overlay]" )
{
    const std::array<int, 3> clear = { 40, 60, 80 };
    SECTION( "uncovered pixels keep the clear color" ) {
        CHECK( expected_rgb( point::zero ) == clear );
        CHECK( expected_rgb( point( 18, 3 ) ) == clear );
    }
    SECTION( "case a mixes toward red at strength 128/255" ) {
        CHECK( expected_rgb( point( 2, 2 ) ) == std::array<int, 3> { 255, 0, 0 } );
        CHECK( expected_rgb( point( 3, 2 ) ) == std::array<int, 3> { 128, 127, 0 } );
        // alpha-128 texel blends the mix half over the clear color
        CHECK( expected_rgb( point( 4, 3 ) ) == std::array<int, 3> { 148, 94, 104 } );
        // transparent texel leaves the clear color
        CHECK( expected_rgb( point( 5, 3 ) ) == clear );
    }
    SECTION( "case e mixes toward cyan at strength 191/255" ) {
        CHECK( expected_rgb( point( 3, 7 ) ) == std::array<int, 3> { 0, 191, 191 } );
    }
    SECTION( "case b samples mirrored texel" ) {
        CHECK( expected_rgb( point( 10, 2 ) ) == std::array<int, 3> { 96, 96, 160 } );
    }
    SECTION( "case c samples the rotated texel" ) {
        CHECK( expected_rgb( point( 19, 2 ) ) == std::array<int, 3> { 127, 255, 127 } );
        CHECK( expected_rgb( point( 20, 5 ) ) == std::array<int, 3> { 64, 192, 64 } );
    }
    SECTION( "case d draws source unchanged" ) {
        CHECK( expected_rgb( point( 3, 10 ) ) == std::array<int, 3> { 0, 255, 0 } );
    }
}

TEST_CASE( "shader_tint_self_test_reports_the_first_pixel_beyond_the_tolerance",
           "[tint_overlay]" )
{
    namespace st = shader_tint_self_test;
    std::vector<st::rgba> pixels;
    for( int y = 0; y < st::target_size; ++y ) {
        for( int x = 0; x < st::target_size; ++x ) {
            pixels.push_back( st::expected_pixel( point( x, y ) ) );
        }
    }
    const auto at = [&pixels]( const point & px ) -> st::rgba & {
        const int index = px.y * st::target_size + px.x;
        return pixels[static_cast<size_t>( index )];
    };
    // b covers this pixel
    const point case_b_px( 11, 3 );
    const size_t case_b_index = 2;

    SECTION( "expected image passes all checks" ) {
        for( const std::optional<st::mismatch> &m : st::compare_readback( pixels ) ) {
            CHECK_FALSE( m );
        }
    }
    SECTION( "channel off by the tolerance passes" ) {
        at( case_b_px ).g = static_cast<uint8_t>( at( case_b_px ).g + st::channel_tolerance );
        for( const std::optional<st::mismatch> &m : st::compare_readback( pixels ) ) {
            CHECK_FALSE( m );
        }
    }
    SECTION( "channel off by more fails only its case" ) {
        at( case_b_px ).g = static_cast<uint8_t>( at( case_b_px ).g + st::channel_tolerance + 1 );
        const std::array<std::optional<st::mismatch>, st::check_count> results =
            st::compare_readback( pixels );
        for( size_t i = 0; i < results.size(); ++i ) {
            CAPTURE( i );
            if( i == case_b_index ) {
                REQUIRE( results[i] );
                CHECK( results[i]->px == case_b_px );
            } else {
                CHECK_FALSE( results[i] );
            }
        }
    }
    SECTION( "changed uncovered pixel fails the background check" ) {
        const point corner( 31, 31 );
        at( corner ).r = static_cast<uint8_t>( at( corner ).r + 20 );
        const std::array<std::optional<st::mismatch>, st::check_count> results =
            st::compare_readback( pixels );
        REQUIRE( results[st::background_index] );
        CHECK( results[st::background_index]->px == corner );
    }
}

#if defined(TILES)

#include "cata_shader.h"
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
