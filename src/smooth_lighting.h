#pragma once
#ifndef CATA_SRC_SMOOTH_LIGHTING_H
#define CATA_SRC_SMOOTH_LIGHTING_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "coords_fwd.h"
#include "cuboid_rectangle.h"
#include "map_scale_constants.h"
#include "point.h"

class map;
enum class lit_level : uint8_t;
struct light_color_rgb;

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

// light map layout: light texels left of reach_column, reach masks from it on,
// one strip of MAPSIZE_Y rows per z level; see lit_sample.glsl
constexpr int reach_column = MAPSIZE_X;
constexpr int lightmap_width = 2 * MAPSIZE_X;
constexpr int lightmap_height = MAPSIZE_Y * OVERMAP_LAYERS;
// reach masks cover this square round a cell, whatever the filter's reach
constexpr int filter_window = 5;
static_assert( 2 * filter_reach + 1 <= filter_window, "reach mask covers the filter" );
static_assert( filter_window * filter_window <= 32, "reach mask holds one bit per cell" );
// texel flag bits, in the a channel of a light texel
constexpr uint8_t texel_detail = 1;
constexpr uint8_t texel_barrier = 2;
// added to a standing sprite's cell column in its vertex colors
constexpr float standing_marker = 0.5f;

struct lightmap_texel {
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 0;
    bool operator==( const lightmap_texel &o ) const {
        return r == o.r && g == o.g && b == o.b && a == o.a;
    }
};
static_assert( sizeof( lightmap_texel ) == 4, "lightmap_texel is one RGBA32 texel" );

size_t light_index( const point &p );
size_t reach_index( const point &p );

// r light, g and b red and green shares of `hue`, a flag bits
lightmap_texel encode_light_texel( const light_cell &cell, const std::array<float, 3> &hue,
                                   bool barrier );
// 25 bit reach mask, little endian
lightmap_texel encode_reach_texel( uint32_t mask );
uint32_t texel_reach( const lightmap_texel &t );
// bit of the cell `d` from the own cell, both coordinates within filter_reach
uint32_t reach_bit( const point &d );

// how a colored light tints what it shines on, as a channel multiplier with
// its brightest channel 1. `scalar_light` is the tile's light, not what the
// avatar sees of it, so special vision does not change the hue. the tint
// fades to white as the colored light itself falls from LIGHT_AMBIENT_LIT to
// LIGHT_AMBIENT_LOW
std::array<float, 3> illumination_hue( const light_color_rgb &lc, float scalar_light );

// barrier flags of one strip's light texels
struct barrier_grid {
    const lightmap_texel *texels = nullptr;
    int stride = 0;
    int width = 0;
    int height = 0;
    bool inside( const point &p ) const;
    // only for cells inside
    bool barrier( const point &p ) const;
};

// cells inside the grid and within filter_reach of `at` that `at`'s light
// filter may read: those a path of single steps toward `at` joins to it
// through cells of its own barrier class
uint32_t reach_mask( const barrier_grid &grid, const point &at );

// fill one z level's strip, light texels then reach masks, over
// settings.area; false, with no cell seen in detail
bool encode_lightmap_layer( const map &here, int z, const lightmap_fill_settings &settings,
                            std::vector<lightmap_texel> &out );

// light map as lit shaders read it: light texels left of reach_column, reach
// masks from it on, z levels stacked rows_per_level apart
struct lightmap_view {
    const lightmap_texel *texels = nullptr;
    int width = 0;
    int height = 0;
    int rows_per_level = 0;
    int reach_column = 0;
};

// a lit vertex color as the rasterizer interpolates it; see lit_sample.glsl
struct lit_coords {
    float x = 0.0f;
    float y = 0.0f;
    float column = 0.0f;
    float row = 0.0f;
};

struct sample_params {
    bool per_tile = false;
    bool iso = false;
};

// what lit_sample.glsl computes for one pixel
struct lit_sample {
    // 0 at vision threshold to 1 at full light
    float light = 0.0f;
    // illumination as a channel multiplier, brightest channel 1
    std::array<float, 3> hue = { 1.0f, 1.0f, 1.0f };
    // how far in sight, 0 to 1, before the sight edge smoothstep
    float in_sight = 0.0f;
    // in_sight after the sight edge smoothstep
    float visible = 0.0f;
    // total filter weight the admitted light came with
    float weight = 0.0f;
};

// lit_sample.glsl's sample_light, run on the CPU
lit_sample reference_sample( const lightmap_view &view, const sample_params &params,
                             const lit_coords &coords );

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
