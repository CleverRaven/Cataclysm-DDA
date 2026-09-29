#include "shader_tint_self_test.h"

#include <cmath>
#include <cstdlib>

#include "cata_assert.h"

namespace shader_tint_self_test
{

namespace
{

bool rect_holds( const point &dst, const point &px )
{
    return px.x >= dst.x && px.x < dst.x + source_width &&
           px.y >= dst.y && px.y < dst.y + source_height;
}

std::optional<point> sampled_texel( const draw_case &c, const point &px )
{
    switch( c.xform ) {
        case transform::none:
            if( !rect_holds( c.dst, px ) ) {
                return std::nullopt;
            }
            return px - c.dst;
        case transform::flip_horizontal: {
            if( !rect_holds( c.dst, px ) ) {
                return std::nullopt;
            }
            point texel = px - c.dst;
            texel.x = source_width - 1 - texel.x;
            return texel;
        }
        case transform::rotate_90: {
            // doubled offsets from the rect center keep pixel centers on integers. a
            // clockwise turn moves texel offset (u, v) to screen offset (-v, u), so
            // screen offset (x, y) samples texel offset (y, -x)
            const int dx2 = 2 * ( px.x - c.dst.x ) + 1 - source_width;
            const int dy2 = 2 * ( px.y - c.dst.y ) + 1 - source_height;
            const int u2 = dy2 + source_width;
            const int v2 = -dx2 + source_height;
            if( u2 < 0 || u2 >= 2 * source_width || v2 < 0 || v2 >= 2 * source_height ) {
                return std::nullopt;
            }
            return point( u2 / 2, v2 / 2 );
        }
    }
    return std::nullopt;
}

} // namespace

const std::array<rgba, texel_count> &source_texels()
{
    static const std::array<rgba, texel_count> texels = {{
            { 255, 0, 0, 255 }, { 0, 255, 0, 255 }, { 0, 0, 255, 255 }, { 128, 128, 128, 255 },
            { 255, 255, 255, 255 }, { 0, 0, 0, 255 }, { 255, 255, 255, 128 }, { 0, 0, 0, 0 },
        }
    };
    return texels;
}

const std::array<draw_case, case_count> &draw_cases()
{
    // e follows a from the same texture with only the mod changed, so the pair
    // shows that each queued draw keeps its own mod
    static const std::array<draw_case, case_count> cases = {{
            { 'a', point( 2, 2 ), transform::none, { 255, 0, 0, 127 } },
            { 'e', point( 2, 6 ), transform::none, { 0, 255, 255, 64 } },
            { 'b', point( 10, 2 ), transform::flip_horizontal, { 0, 0, 255, 191 } },
            { 'c', point( 18, 3 ), transform::rotate_90, { 0, 255, 0, 127 } },
            { 'd', point( 2, 10 ), transform::none, { 255, 255, 255, 255 } },
        }
    };
    return cases;
}

rgba clear_color()
{
    return { 40, 60, 80, 255 };
}

std::optional<coverage> covering_case( const point &px )
{
    const std::array<draw_case, case_count> &cases = draw_cases();
    for( size_t i = 0; i < cases.size(); ++i ) {
        if( const std::optional<point> texel = sampled_texel( cases[i], px ) ) {
            return coverage{ i, *texel };
        }
    }
    return std::nullopt;
}

rgba expected_pixel( const point &px )
{
    const rgba clear = clear_color();
    const std::optional<coverage> c = covering_case( px );
    if( !c ) {
        return clear;
    }
    const int index = c->texel.y * source_width + c->texel.x;
    const rgba &src = source_texels()[static_cast<size_t>( index )];
    const tint_texture_mod &mod = draw_cases()[c->case_index].mod;
    const float alpha = src.a / 255.0f;
    const auto channel = [alpha, &mod]( const uint8_t s, const uint8_t m, const uint8_t dst ) {
        const float out = tint_mix_channel( s, m, mod.a );
        return static_cast<uint8_t>( std::lround( out * alpha + dst * ( 1.0f - alpha ) ) );
    };
    return { channel( src.r, mod.r, clear.r ), channel( src.g, mod.g, clear.g ),
             channel( src.b, mod.b, clear.b ), clear.a };
}

std::array<std::optional<mismatch>, check_count> compare_readback(
    const std::vector<rgba> &pixels )
{
    cata_assert( pixels.size() == static_cast<size_t>( target_size * target_size ) );
    const auto differs = []( const uint8_t expected, const uint8_t actual ) {
        return std::abs( expected - actual ) > channel_tolerance;
    };
    std::array<std::optional<mismatch>, check_count> result;
    for( int y = 0; y < target_size; ++y ) {
        for( int x = 0; x < target_size; ++x ) {
            const point px( x, y );
            const std::optional<coverage> c = covering_case( px );
            std::optional<mismatch> &slot = result[c ? c->case_index : background_index];
            if( slot ) {
                continue;
            }
            const rgba expected = expected_pixel( px );
            const int index = y * target_size + x;
            const rgba &actual = pixels[static_cast<size_t>( index )];
            if( differs( expected.r, actual.r ) || differs( expected.g, actual.g ) ||
                differs( expected.b, actual.b ) ) {
                slot = mismatch{ px, expected, actual };
            }
        }
    }
    return result;
}

std::string rgb_string( const rgba &c )
{
    return "(" + std::to_string( c.r ) + ", " + std::to_string( c.g ) + ", " +
           std::to_string( c.b ) + ")";
}

} // namespace shader_tint_self_test
