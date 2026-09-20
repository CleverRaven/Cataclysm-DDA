#include "tile_tint.h"

#include <algorithm>

#include "lightmap.h"

std::optional<tile_tint> compute_tile_tint( const light_color_rgb &lc, const float scalar_light )
{
    // Isolate chromatic (saturated) component by subtracting achromatic floor
    // (min channel). Pure white light (equal RGB) produces zero saturation and
    // no tint
    const float min_ch = std::min( { lc.r, lc.g, lc.b } );
    const float sat_r = lc.r - min_ch;
    const float sat_g = lc.g - min_ch;
    const float sat_b = lc.b - min_ch;
    const float sat_mag = std::max( { sat_r, sat_g, sat_b } );
    if( sat_mag < 0.01f ) {
        return std::nullopt;
    }
    // Alpha: ratio of saturated energy to total scalar light. subtle under
    // bright ambient, vivid in darkness
    const float ratio = scalar_light > 0.1f ? std::min( 1.0f, sat_mag / scalar_light ) : 0.0f;
    const uint8_t alpha = static_cast<uint8_t>( ratio * 80.0f );
    if( alpha == 0 ) {
        return std::nullopt;
    }
    // Normalize saturated color to full brightness
    tile_tint t;
    t.r = static_cast<uint8_t>( sat_r / sat_mag * 255.0f );
    t.g = static_cast<uint8_t>( sat_g / sat_mag * 255.0f );
    t.b = static_cast<uint8_t>( sat_b / sat_mag * 255.0f );
    t.a = alpha;
    return t;
}

tint_texture_mod tint_texture_mod_for( const tile_tint &t )
{
    tint_texture_mod m;
    m.r = t.r;
    m.g = t.g;
    m.b = t.b;
    m.a = static_cast<uint8_t>( 255 - t.a );
    return m;
}

tint_texture_mod tint_texture_mod_none()
{
    return tint_texture_mod{};
}

float tint_mix_channel( const float src, const float mod, const uint8_t mod_a )
{
    return ( src * mod_a + mod * ( 255 - mod_a ) ) / 255.0f;
}
