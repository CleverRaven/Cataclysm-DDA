#pragma once
#ifndef CATA_SRC_SMOOTH_LIGHTING_H
#define CATA_SRC_SMOOTH_LIGHTING_H

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "map_scale_constants.h"
#include "point.h"

class JsonObject;
class map;
enum class lit_level : uint8_t;
struct ter_t;
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

// support of the cubic B-spline in lit_filter.glsl, in cells each side
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
// one strip of MAPSIZE_Y rows per z level; see lit_common.glsl
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

// a colored light's color at its strength w, as 1 - w + w * color with the
// color's brightest channel 1: w is the colored share of the light.
// `scalar_light` is the tile's light, not what the avatar sees of it, so
// special vision does not change the hue. the tint fades to white as the
// colored light itself falls from LIGHT_AMBIENT_LIT to LIGHT_AMBIENT_LOW
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

// one corner of a lit sprite's quad: where it goes on screen, its vertex
// color, and its texture coordinates
struct lit_vertex {
    float x = 0.0f;
    float y = 0.0f;
    lit_coords light;
    float u = 0.0f;
    float v = 0.0f;
};

// lit sprite as drawn: its screen box and texture box before any turn or flip,
// the tile it stands for and the tile's ground on screen
struct lit_quad_params {
    std::array<float, 4> screen = {};
    std::array<float, 4> uv = {};
    bool flip_horizontal = false;
    bool flip_vertical = false;
    quarter_turn turn = quarter_turn::none;
    bool iso = false;
    float tile_width = 0.0f;
    float tile_height = 0.0f;
    // screen position of the tile's top left corner on its z level's ground
    float ground_x = 0.0f;
    float ground_y = 0.0f;
    tripoint_bub_ms pos;
    bool standing = false;
};

// the four corners of a lit sprite, top left first and clockwise as drawn
// before the turn; SDL turns about the box's center
std::array<lit_vertex, 4> lit_quad( const lit_quad_params &p );

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

// smooth_filtered's filter before it turns into a lit_sample; the prefilter
// stores this, so the GPU and the reference interpolate the same values
struct filter_result {
    float light = 0.0f;
    // weighted chroma shares before chroma_hue
    std::array<float, 3> chroma = { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f };
    // before the sight edge smoothstep
    float in_sight = 0.0f;
    // only CPU checks read it: the prefilter texture has no channel for it
    float weight = 0.0f;
};
// lit_filter.glsl's filter_light: light of light map cell `own` on `level`
// at ( lx, ly ) inside it, each 0 to 1
filter_result reference_filter( const lightmap_view &view, const point &own, int level, float lx,
                                float ly );
lit_sample finish_filter( const filter_result &r );

// grid steps per cell side of the prefiltered light; even, so the change of
// anchors at a cell's middle falls on a grid line rather than between two
constexpr int prefilter_grid = 10;
// max per channel difference from the point filter, in 0..255 steps
constexpr float prefilter_error_budget = 2.0f;

// where the prefiltered light keeps each cell: a block of ( grid + 1 ) texels
// a side, light at local i / grid, blocks of a level row by row, levels
// stacked below each other
struct prefilter_layout {
    // cells, x, y inside their level
    half_open_rectangle<point> area;
    // light map level of the first block row
    int first_level = 0;
    int levels = 0;
    int grid = prefilter_grid;

    point size() const;
    point block_origin( const point &cell, int level ) const;
    // whether `cell` on `level` has a block; any other cell's light is dark
    bool holds( const point &cell, int level ) const;
    bool operator==( const prefilter_layout &other ) const;
};
// an empty layout always fits: it has nothing to store
bool prefilter_fits( const prefilter_layout &layout, int max_texture_size, bool format_supported );
// texture size for `requested` prefiltered texels: a quarter more as headroom,
// never past `max_texture_size`, never under `current` or one texel
point prefilter_texture_size( const point &requested, const point &current, int max_texture_size );

// cells of one level's light texels, `stride` texels a row, seen in detail
// inside `area`
std::optional<half_open_rectangle<point>> seen_box( const lightmap_texel *level, int stride,
                                       const half_open_rectangle<point> &area );
// layout over `seen` grown by one cell, clipped to `clip`: a pixel's 2x2
// anchors lie within one cell of its own, so past that its light is dark
prefilter_layout prefilter_layout_for( const std::optional<half_open_rectangle<point>> &seen,
                                       const half_open_rectangle<point> &clip, int first_level, int levels );

// `x` as the nearest half float stores it, ties to even
float half_round( float x );

// texels a lookup reads, ( 0, 0 ) ( 1, 0 ) ( 0, 1 ) ( 1, 1 ), and its weights;
// the second texel is clamped to the block, so local 1 never reads past it
struct prefilter_lookup {
    std::array<point, 4> texels;
    float wx = 0.0f;
    float wy = 0.0f;
};
prefilter_lookup prefilter_lookup_texels( const prefilter_layout &layout, const point &cell,
        int level, float lx, float ly );

// prefiltered light as the GPU stores it: light, two chroma shares and
// in_sight, each a half float
struct prefilter_table {
    prefilter_layout layout;
    std::vector<std::array<float, 4>> texels;
    // round lookup weights to 1/16, the coarsest a GPU's linear filter may
    bool quantize_weights = false;
};
prefilter_table build_prefilter_table( const lightmap_view &view, const prefilter_layout &layout );
// reference_sample over the prefiltered light; per tile it reads the light map
lit_sample reference_prefiltered_sample( const prefilter_table &table, const lightmap_view &view,
        const sample_params &params, const lit_coords &coords );

// light level from which lit sprites keep their full color
constexpr float full_color_light = 0.75f;
// how far a fully colored light mixes a lit pixel toward its own color, at
// the pixel's brightness or as near it as the screen shows
constexpr float tint_mix = 0.4f;
// brightness of lit sprites at the vision threshold: the classic shadow
// variant's, color_pixel_grayscale's 5/8
constexpr float shadow_shade = 0.625f;
// share of night vision's look that low light keeps
constexpr float night_floor = 0.55f;
// light level over which night vision hands over to the overexposed look; at
// full light classic tiles show the overexposed variant
constexpr float overexpose_start = 0.85f;

// what lit shaders' looks depend on besides the light sample
struct look_params {
    // memory_presets.glsl preset number, or custom_look
    int memory_look = 0;
    bool blend_memory = false;
    std::array<float, 3> custom_dark = {};
    std::array<float, 3> custom_light = {};
    float custom_gamma = 1.0f;
};
// memory_look of the custom MEMORY_MAP_MODE mixer, after the named presets
constexpr int custom_look = 4;

// lit.frag, nightvision_lit.frag and the preset looks they include, run on
// the CPU: `rgb` is the sprite pixel, 0 to 1
std::array<float, 3> reference_memory_rgb( const look_params &look,
        const std::array<float, 3> &rgb );
std::array<float, 3> reference_lit_rgb( const look_params &look, const std::array<float, 3> &rgb,
                                        const lit_sample &s );
std::array<float, 3> reference_night_rgb( const look_params &look,
        const std::array<float, 3> &rgb, const lit_sample &s );

// what kept smooth lighting off this frame
enum class lit_failure : uint8_t {
    texture_create,
    upload,
    sampler_create,
    shader_load,
    state_create,
    uniform_upload,
    probe_mismatch,
    // prefilter pass failed but left the renderer safe
    prefilter,
};
const char *to_string( lit_failure f );

// which lighting a failure turns off
enum class failure_scope : uint8_t {
    all,
    filtered,
};
// while a frame filters, failing to make, bind or fill what only filtered
// light reads turns off filtered light alone
failure_scope scope_of( lit_failure f, bool frame_filtered );

// which smooth lighting failures are retried next frame, and which keep it
// off until the GPU resources are rebuilt
class failure_policy
{
    public:
        // `retried` gets upload_retries consecutive frames before it latches;
        // every other failure latches at once
        explicit failure_policy( lit_failure retried = lit_failure::upload ) : retried_( retried ) {}
        // consecutive frames the retried failure may fail before it latches
        static constexpr int upload_retries = 3;
        // record a failure; true when this one latched
        bool fail( lit_failure f );
        // a frame drew lit
        void succeed();
        // resources now carry `generation`; a new one clears a latch
        void rebuilt( uint32_t generation );
        void reset();
        bool latched() const {
            return latched_.has_value();
        }
        std::optional<lit_failure> latched_by() const {
            return latched_;
        }
    private:
        lit_failure retried_;
        std::optional<lit_failure> latched_;
        int upload_streak_ = 0;
        uint32_t generation_ = 0;
};

// what lighting the map draws with, and why it is not smooth
enum class lighting_status : uint8_t {
    classic_by_option,
    classic_no_shader_path,
    classic_failed,
    classic_this_frame,
    smooth,
    smooth_filtered,
    // smooth_filtered asked for, per tile light drawn: the GPU cannot store or
    // read the prefiltered light, or the prefilter failed
    smooth_filtered_unavailable,
};
const char *to_string( lighting_status s );

// how sprites read the prefiltered light: the GPU's linear filter, or four
// texel fetches with float weights for GPUs whose filter is too coarse
enum class lookup : uint8_t {
    hardware,
    manual,
};

// what a group of activation probe cases found
enum class probe_group_result : uint8_t {
    passed,
    mismatch,
    // the renderer may hold a bind or a target the probe left
    unsafe,
    skipped,
};

// which smooth lighting the probed shaders can draw
struct lit_capability {
    // a probe left the renderer unsafe: abort the frame
    bool unsafe = false;
    bool per_tile = false;
    std::optional<lookup> filtered;
};
// `probed` as the current samplers allow: the hardware lookup reads through
// the linear sampler, so without it filtered light is unavailable
lit_capability with_linear_sampler( lit_capability probed, bool linear_sampler );
// filtered cases are tried with the hardware lookup, then the manual one
lit_capability decide_lit_capability( probe_group_result per_tile,
                                      probe_group_result filtered_hardware, probe_group_result filtered_manual );

// how a frame lights its sprites, and the status it shows
struct lit_mode {
    bool per_tile = true;
    lighting_status status = lighting_status::smooth;
};
lit_mode choose_lit_mode( bool want_filtered, const lit_capability &capability,
                          bool filtered_latched, bool prefilter_fits );

// what the prefiltered light was made from
struct prefilter_inputs {
    uint64_t upload_generation = 0;
    prefilter_layout layout;
    lookup mode = lookup::hardware;
    uint32_t resource_generation = 0;
    bool operator==( const prefilter_inputs &other ) const;
};

// whether the prefiltered light is current; a pass forgets the old inputs
// before it writes and publishes the new ones only once it finished cleanly
class prefilter_cache
{
    public:
        bool needs_run( const prefilter_inputs &in ) const;
        void begin_write();
        void publish( const prefilter_inputs &in );
    private:
        std::optional<prefilter_inputs> published_;
};

// what a frame does after asking for lit states
enum class lit_frame_action : uint8_t {
    draw_lit,
    draw_classic,
    abort_frame,
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

// where a sprite takes its smooth light from
enum class light_anchor : uint8_t {
    // per pixel, from the ground under it
    ground,
    // along base line, the same all the way up
    base,
};

// a tile entry's "light_anchor", nullopt when absent; throws JsonError on an
// unknown value
std::optional<light_anchor> read_light_anchor( const JsonObject &entry );

// base for terrain that rises out of the ground: walls, trees, shrubs, doors
// and windows; ground for the rest
light_anchor terrain_light_anchor( const ter_t &t );

// tileset's anchor for the tile, else the default for what it shows; nullopt
// leaves it to sprite_stands
std::optional<light_anchor> chosen_light_anchor( std::optional<light_anchor> tile_anchor,
        std::optional<light_anchor> default_anchor );

} // namespace smooth_lighting

#endif // CATA_SRC_SMOOTH_LIGHTING_H
