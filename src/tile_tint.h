#pragma once
#ifndef CATA_SRC_TILE_TINT_H
#define CATA_SRC_TILE_TINT_H

#include <cstdint>
#include <optional>

struct light_color_rgb;

// per-tile colored-light tint: normalized hue + strength in [0, 80]
struct tile_tint {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
};

// chromatic residual of the light color, normalized to full brightness, with
// strength = min(1, saturation / scalar) * 80. nullopt when the light is white,
// too weak, or the scalar light is at or below 0.1
std::optional<tile_tint> compute_tile_tint( const light_color_rgb &lc, float scalar_light );

// Texture color and alpha modulation that encodes a tint: rgb = tint hue,
// alpha = 255 - strength, so the untouched default (255, 255, 255, 255) means
// no tint
struct tint_texture_mod {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
};
tint_texture_mod tint_texture_mod_for( const tile_tint &t );
tint_texture_mod tint_texture_mod_none();

// one channel of the sprite shader tint, unrounded: mix(src, mod, 1 - mod_a / 255)
float tint_mix_channel( float src, float mod, uint8_t mod_a );

#endif // CATA_SRC_TILE_TINT_H
