#include <optional>

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
