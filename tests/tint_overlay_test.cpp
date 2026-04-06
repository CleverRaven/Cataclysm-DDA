#include "cata_catch.h"
#include "map.h"

// sprite_screen_bounds is defined in map.h, no TILES dependency.

TEST_CASE( "sprite_screen_bounds_first_expand_sets_valid", "[tint_overlay]" )
{
    sprite_screen_bounds b;
    CHECK_FALSE( b.valid );
    b.expand( 10, 20, 30, 40 );
    CHECK( b.valid );
    CHECK( b.x == 10 );
    CHECK( b.y == 20 );
    CHECK( b.w == 30 );
    CHECK( b.h == 40 );
}

TEST_CASE( "sprite_screen_bounds_disjoint_union", "[tint_overlay]" )
{
    sprite_screen_bounds b;
    b.expand( 0, 0, 10, 10 );
    b.expand( 20, 20, 10, 10 );
    // Union should span (0,0) to (30,30)
    CHECK( b.x == 0 );
    CHECK( b.y == 0 );
    CHECK( b.w == 30 );
    CHECK( b.h == 30 );
}

TEST_CASE( "sprite_screen_bounds_overlapping_union", "[tint_overlay]" )
{
    sprite_screen_bounds b;
    b.expand( 0, 0, 20, 20 );
    b.expand( 10, 10, 20, 20 );
    CHECK( b.x == 0 );
    CHECK( b.y == 0 );
    CHECK( b.w == 30 );
    CHECK( b.h == 30 );
}

TEST_CASE( "sprite_screen_bounds_nested_rect", "[tint_overlay]" )
{
    sprite_screen_bounds b;
    b.expand( 0, 0, 100, 100 );
    b.expand( 10, 10, 20, 20 );
    // Inner rect doesn't change the outer
    CHECK( b.x == 0 );
    CHECK( b.y == 0 );
    CHECK( b.w == 100 );
    CHECK( b.h == 100 );
}

TEST_CASE( "sprite_screen_bounds_negative_coords", "[tint_overlay]" )
{
    sprite_screen_bounds b;
    b.expand( -10, -20, 30, 40 );
    CHECK( b.x == -10 );
    CHECK( b.y == -20 );
    b.expand( 5, 5, 10, 10 );
    // Union: (-10,-20) to (20,20) -> w=30, h=40
    CHECK( b.x == -10 );
    CHECK( b.y == -20 );
    CHECK( b.w == 30 );
    CHECK( b.h == 40 );
}

TEST_CASE( "sprite_screen_bounds_idempotent", "[tint_overlay]" )
{
    sprite_screen_bounds b;
    b.expand( 5, 10, 20, 30 );
    b.expand( 5, 10, 20, 30 );
    CHECK( b.x == 5 );
    CHECK( b.y == 10 );
    CHECK( b.w == 20 );
    CHECK( b.h == 30 );
}

#if defined(TILES)

#include "cata_tiles.h"

// Helper: simulate the opaque-bounds-to-destination transform from draw_sprite_at,
// including flip handling. Returns the bounds that would be accumulated.
static sprite_screen_bounds simulate_opaque_bounds_transform(
    const SDL_Rect &destination, const SDL_Rect &opq,
    int src_w, int src_h, bool flip_h, bool flip_v )
{
    SDL_Rect adj = opq;
    if( flip_h ) {
        adj.x = src_w - opq.x - opq.w;
    }
    if( flip_v ) {
        adj.y = src_h - opq.y - opq.h;
    }
    sprite_screen_bounds b;
    if( adj.w > 0 && adj.h > 0 ) {
        b.expand(
            destination.x + adj.x * destination.w / src_w,
            destination.y + adj.y * destination.h / src_h,
            adj.w * destination.w / src_w,
            adj.h * destination.h / src_h );
    }
    return b;
}

TEST_CASE( "texture_opaque_rect_defaults_to_full_srcrect", "[tint_overlay]" )
{
    // 2-arg constructor: opaque rect should default to full sprite dimensions.
    texture tex( nullptr, SDL_Rect{ 0, 0, 32, 64 } );
    const SDL_Rect &opq = tex.get_opaque_rect();
    CHECK( opq.x == 0 );
    CHECK( opq.y == 0 );
    CHECK( opq.w == 32 );
    CHECK( opq.h == 64 );
}

TEST_CASE( "texture_opaque_rect_stores_custom_bounds", "[tint_overlay]" )
{
    // 3-arg constructor: opaque rect should reflect the alpha-trimmed box.
    texture tex( nullptr, SDL_Rect{ 0, 0, 32, 64 }, SDL_Rect{ 4, 8, 24, 48 } );
    const SDL_Rect &opq = tex.get_opaque_rect();
    CHECK( opq.x == 4 );
    CHECK( opq.y == 8 );
    CHECK( opq.w == 24 );
    CHECK( opq.h == 48 );
}

TEST_CASE( "texture_opaque_rect_empty_for_transparent_sprite", "[tint_overlay]" )
{
    texture tex( nullptr, SDL_Rect{ 0, 0, 32, 32 }, SDL_Rect{ 0, 0, 0, 0 } );
    const SDL_Rect &opq = tex.get_opaque_rect();
    CHECK( opq.w == 0 );
    CHECK( opq.h == 0 );
}

TEST_CASE( "opaque_bounds_transform_no_flip", "[tint_overlay]" )
{
    // 32x64 sprite, opaque region is the bottom 32x32 (character body, no head padding).
    const SDL_Rect dest = { 100, 50, 64, 128 }; // 2x scaled
    const SDL_Rect opq = { 0, 32, 32, 32 };     // bottom half in source
    const sprite_screen_bounds b = simulate_opaque_bounds_transform( dest, opq, 32, 64, false, false );
    CHECK( b.valid );
    CHECK( b.x == 100 );       // no x offset (opq.x == 0)
    CHECK( b.y == 50 + 64 );   // opq.y=32 scaled by 128/64 = 64
    CHECK( b.w == 64 );        // opq.w=32 scaled by 64/32 = 64
    CHECK( b.h == 64 );        // opq.h=32 scaled by 128/64 = 64
}

TEST_CASE( "opaque_bounds_transform_horizontal_flip", "[tint_overlay]" )
{
    // Asymmetric sprite: opaque pixels on the left side only.
    const SDL_Rect dest = { 100, 50, 64, 64 };
    const SDL_Rect opq = { 0, 0, 16, 32 };  // left quarter in source
    const sprite_screen_bounds b = simulate_opaque_bounds_transform( dest, opq, 32, 32, true, false );
    CHECK( b.valid );
    // After H-flip: opq.x = 32 - 0 - 16 = 16, scaled: 16*64/32 = 32
    CHECK( b.x == 100 + 32 );
    CHECK( b.y == 50 );
    CHECK( b.w == 32 );  // 16*64/32
    CHECK( b.h == 64 );  // 32*64/32
}

TEST_CASE( "opaque_bounds_transform_both_flips", "[tint_overlay]" )
{
    // Asymmetric sprite: opaque pixels in top-left corner.
    const SDL_Rect dest = { 0, 0, 64, 128 };
    const SDL_Rect opq = { 0, 0, 16, 32 };  // top-left in source (32x64)
    const sprite_screen_bounds b = simulate_opaque_bounds_transform( dest, opq, 32, 64, true, true );
    CHECK( b.valid );
    // After H-flip: opq.x = 32 - 0 - 16 = 16
    // After V-flip: opq.y = 64 - 0 - 32 = 32
    CHECK( b.x == 0 + 16 * 64 / 32 );  // 32
    CHECK( b.y == 0 + 32 * 128 / 64 ); // 64
    CHECK( b.w == 16 * 64 / 32 );       // 32
    CHECK( b.h == 32 * 128 / 64 );      // 64
}

TEST_CASE( "opaque_bounds_transform_empty_opaque_rect", "[tint_overlay]" )
{
    const SDL_Rect dest = { 100, 50, 64, 64 };
    const SDL_Rect opq = { 0, 0, 0, 0 };
    const sprite_screen_bounds b = simulate_opaque_bounds_transform( dest, opq, 32, 32, false, false );
    CHECK_FALSE( b.valid );
}

#endif // TILES
