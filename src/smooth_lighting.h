#pragma once
#ifndef CATA_SRC_SMOOTH_LIGHTING_H
#define CATA_SRC_SMOOTH_LIGHTING_H

#include <cstdint>

#include "cuboid_rectangle.h"
#include "point.h"

enum class lit_level : uint8_t;

namespace smooth_lighting
{

// what smooth lighting shows of one tile, from the classification classic
// tiles draw by
struct light_cell {
    // 0 at vision threshold to 1 at LIGHT_AMBIENT_LIT and above
    float light = 0.0f;
    // seen in detail: map::get_visibility gives visibility_type::CLEAR
    bool detail = false;
};

// whether classify_light_cell reads apparent light for `ll`
bool needs_apparent_light( lit_level ll );

// `apparent_light` from map::apparent_light_helper, read only when
// needs_apparent_light( ll ); `vision_threshold` from visibility_variables
light_cell classify_light_cell( lit_level ll, float apparent_light, float vision_threshold );

// apparent light: 0 at vision threshold, 1 at LIGHT_AMBIENT_LIT
float light_above_threshold( float apparent_light, float vision_threshold );

// sprite rotations the tiles use: SDL turns about the sprite's center
enum class quarter_turn : uint8_t {
    none,
    clockwise,
    counterclockwise,
};

// support of the cubic B-spline in lit_sample.glsl, in cells each side
constexpr int filter_reach = 2;

// how far above its tile's ground line, in tile widths, a sprite's opaque top
// must reach for the sprite to stand on the tile rather than lie on it
constexpr float standing_rise = 0.125f;

struct sprite_footprint {
    // sprite size and opaque box, in source sprite pixels; empty box is a fully
    // transparent sprite
    point size;
    half_open_rectangle<point> opaque;
    // flips apply in source space, before the turn
    bool flip_horizontal = false;
    bool flip_vertical = false;
    quarter_turn turn = quarter_turn::none;
    // tile_type::pixelscale
    float pixelscale = 1.0f;
    // sprite top relative to the z level's tile top edge, in tileset pixels:
    // tile offset y plus extra offset, minus what the sprite is stacked on
    int top = 0;
};

// tileset tile size in tileset pixels, and projection
struct tile_geometry {
    int width = 0;
    int height = 0;
    bool iso = false;
};

// whether the sprite's opaque pixels, flipped and turned as drawn, rise above
// its tile's ground line by standing_rise of a tile; zoom plays no part
bool sprite_stands( const sprite_footprint &f, const tile_geometry &g );

} // namespace smooth_lighting

#endif // CATA_SRC_SMOOTH_LIGHTING_H
