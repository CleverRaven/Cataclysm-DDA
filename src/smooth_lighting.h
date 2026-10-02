#pragma once
#ifndef CATA_SRC_SMOOTH_LIGHTING_H
#define CATA_SRC_SMOOTH_LIGHTING_H

#include <array>
#include <cstdint>
#include <optional>

#include "coords_fwd.h"
#include "cuboid_rectangle.h"
#include "map_scale_constants.h"
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

// map tiles the light map is filled for: the view range on screen, plus the
// filter's reach, from the view range's and the screen's corner tiles
half_open_rectangle<point> lightmap_fill_area( const point &view_min, const point &view_max,
        const point &screen_min, const point &screen_max );

// the cells a frame's light map holds. a lit sprite's own cell must be one:
// its vertex colors address the light map by it, and a cell outside would
// read another column, row or z level. every other sprite draws classic
struct lightmap_extent {
    half_open_rectangle<point> area;
    int min_z = 0;
    int max_z = -1;
    bool covers( const tripoint_bub_ms &p ) const;
};

// settings every level's texels are filled under
struct lightmap_fill_settings {
    half_open_rectangle<point> area;
    float vision_threshold = 0.0f;
    // colored light tints the light map
    bool tint = false;
    // build reach masks: only smooth_filtered reads them
    bool masks = false;
    bool operator==( const lightmap_fill_settings &o ) const {
        return area.p_min == o.area.p_min && area.p_max == o.area.p_max &&
               vision_threshold == o.vision_threshold && tint == o.tint && masks == o.masks;
    }
};

// cache generations a level's texels were filled from; every value comes from
// next_cache_generation, so equal values are the same mutation
struct layer_inputs {
    uint64_t lightmap_generation = 0;
    uint64_t visibility_generation = 0;
    uint64_t seen_generation = 0;
    uint64_t aim_generation = 0;
    // map::apparent_light_helper applies the aim cone
    bool aim_cone = false;
    bool operator==( const layer_inputs &o ) const {
        return lightmap_generation == o.lightmap_generation &&
               visibility_generation == o.visibility_generation &&
               seen_generation == o.seen_generation && aim_generation == o.aim_generation &&
               aim_cone == o.aim_cone;
    }
};

// which z levels of the light map hold texels for the current inputs. a
// level refills when its layer_inputs or the frame's fill settings change.
// a map shift invalidates every level's map cache, so it reaches
// lightmap_generation
class lightmap_keys
{
    public:
        // new fill settings forget every level
        void begin_frame( const lightmap_fill_settings &settings );
        bool needs_fill( int z, const layer_inputs &inputs ) const;
        void mark_filled( int z, const layer_inputs &inputs );
        // level's texels can't be trusted, e.g. after a failed upload
        void forget( int z );
        void forget_all();
    private:
        std::optional<lightmap_fill_settings> settings_;
        std::array<std::optional<layer_inputs>, OVERMAP_LAYERS> filled_;
};

// whether a sprite takes the scene's light from the light map: lighting is on,
// the sprite asks for scene light, and the light map holds its cell
bool lit_path_for( bool lighting_active, bool wants_scene_light, bool cell_covered );

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
