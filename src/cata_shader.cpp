#if defined(TILES)

#include "cata_shader.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include "cuboid_rectangle.h"
#include "debug.h"
#include "map_scale_constants.h"
#include "path_info.h"
#include "smooth_lighting.h"
#include "tile_tint.h"

namespace cata_shader
{

namespace
{
bool g_reprobe_requested = false;
int test_probe_unsafe_countdown = 0;
bool test_flush_failure_armed = false;
int test_probe_run_count = 0;
} // namespace

void request_reprobe()
{
    g_reprobe_requested = true;
}

bool reprobe_requested()
{
    return g_reprobe_requested;
}

void clear_reprobe()
{
    g_reprobe_requested = false;
}

void test_arm_probe_unsafe( const int count )
{
    test_probe_unsafe_countdown = count;
}

int test_probe_unsafe_remaining()
{
    return test_probe_unsafe_countdown;
}

void test_arm_flush_failure()
{
    test_flush_failure_armed = true;
}

int test_probe_runs()
{
    return test_probe_run_count;
}

void test_reset_seams()
{
    test_probe_unsafe_countdown = 0;
    test_flush_failure_armed = false;
    test_probe_run_count = 0;
}

namespace
{

// SDL_GPU shader format selection. SDL_GetGPUShaderFormats reports a bitmask
// of supported formats; we ship one artifact per format and pick the matching
// one. The order below mirrors common backend preference: Vulkan (SPIR-V),
// D3D12 (DXIL), Metal (MSL).
struct format_artifact {
    SDL_GPUShaderFormat sdl_flag;
    const char *suffix;
    SDL_GPUShaderFormat shader_format;
};

constexpr std::array<format_artifact, 3> SUPPORTED_FORMATS{ {
        { SDL_GPU_SHADERFORMAT_SPIRV, ".spv", SDL_GPU_SHADERFORMAT_SPIRV },
        { SDL_GPU_SHADERFORMAT_DXIL,  ".dxil", SDL_GPU_SHADERFORMAT_DXIL },
        { SDL_GPU_SHADERFORMAT_MSL,   ".msl", SDL_GPU_SHADERFORMAT_MSL },
    }
};

std::vector<unsigned char> read_file_bytes( const std::string &path )
{
    std::ifstream in( path, std::ios::binary );
    if( !in.is_open() ) {
        return {};
    }
    return { std::istreambuf_iterator<char>( in ),
             std::istreambuf_iterator<char>() };
}

} // namespace

shader shader::load_fragment( SDL_GPUDevice *device, const std::string &basename,
                              unsigned int num_samplers, unsigned int num_uniform_buffers )
{
    if( !device ) {
        return shader{};
    }

    const SDL_GPUShaderFormat available = SDL_GetGPUShaderFormats( device );
    if( available == SDL_GPU_SHADERFORMAT_INVALID ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader: SDL_GetGPUShaderFormats returned INVALID; "
                "no shader formats supported by GPU device";
        return shader{};
    }

    // Pick the first format we have an artifact for that the device reports.
    const format_artifact *chosen = nullptr;
    for( const format_artifact &candidate : SUPPORTED_FORMATS ) {
        if( ( available & candidate.sdl_flag ) != 0 ) {
            chosen = &candidate;
            break;
        }
    }
    if( !chosen ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader: no shipped shader artifact matches GPU "
                "device's supported formats (mask=" << static_cast<unsigned>( available ) << ")";
        return shader{};
    }

    const std::string artifact_path = ( PATH_INFO::datadir() + "shaders/" + basename )
                                      + chosen->suffix;
    std::vector<unsigned char> bytes = read_file_bytes( artifact_path );
    if( bytes.empty() ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader: failed to read shader artifact at " << artifact_path;
        return shader{};
    }

    SDL_GPUShaderCreateInfo info{};
    info.code_size = bytes.size();
    info.code = bytes.data();
    info.entrypoint = nullptr; // Let SDL supply backend default; do not hardcode "main".
    info.format = chosen->shader_format;
    info.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
    info.num_samplers = num_samplers;
    info.num_storage_textures = 0;
    info.num_storage_buffers = 0;
    info.num_uniform_buffers = num_uniform_buffers;

    SDL_GPUShader *raw = SDL_CreateGPUShader( device, &info );
    if( !raw ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader: SDL_CreateGPUShader failed: " << SDL_GetError();
        return shader{};
    }
    return shader( device, raw );
}

shader::~shader()
{
    if( ptr_ && device_ ) {
        SDL_ReleaseGPUShader( device_, ptr_ );
    }
}

shader::shader( shader &&other ) noexcept
    : device_( other.device_ ), ptr_( other.ptr_ )
{
    other.device_ = nullptr;
    other.ptr_ = nullptr;
}

shader &shader::operator=( shader &&other ) noexcept
{
    if( this != &other ) {
        if( ptr_ && device_ ) {
            SDL_ReleaseGPUShader( device_, ptr_ );
        }
        device_ = other.device_;
        ptr_ = other.ptr_;
        other.device_ = nullptr;
        other.ptr_ = nullptr;
    }
    return *this;
}

render_state render_state::create( SDL_Renderer *renderer, const shader &fragment_shader )
{
    // No additional sampler/storage bindings: the atlas sampler comes from the
    // renderer's normal textured-draw path
    return create( renderer, fragment_shader, nullptr, 0 );
}

render_state render_state::create( SDL_Renderer *renderer, const shader &fragment_shader,
                                   const SDL_GPUTextureSamplerBinding *bindings,
                                   const int num_bindings )
{
    if( !renderer || !fragment_shader.is_valid() ) {
        return render_state{};
    }

    SDL_GPURenderStateCreateInfo info{};
    info.fragment_shader = fragment_shader.get();
    info.num_sampler_bindings = num_bindings;
    info.sampler_bindings = bindings;
    info.num_storage_textures = 0;
    info.num_storage_buffers = 0;

    SDL_GPURenderState *raw = SDL_CreateGPURenderState( renderer, &info );
    if( !raw ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader: SDL_CreateGPURenderState failed: " << SDL_GetError();
        return render_state{};
    }
    return render_state( raw );
}

render_state::~render_state()
{
    if( ptr_ ) {
        SDL_DestroyGPURenderState( ptr_ );
    }
}

render_state::render_state( render_state &&other ) noexcept
    : ptr_( other.ptr_ )
{
    other.ptr_ = nullptr;
}

render_state &render_state::operator=( render_state &&other ) noexcept
{
    if( this != &other ) {
        if( ptr_ ) {
            SDL_DestroyGPURenderState( ptr_ );
        }
        ptr_ = other.ptr_;
        other.ptr_ = nullptr;
    }
    return *this;
}

namespace
{

// Disposal slot for probe-owned textures left undestroyable by a failed
// SDL_SetGPURenderState(NULL). Recovery replaces the renderer, so the gate
// keeps teardown from destroying them on a dead renderer.
gpu_handle_graveyard &probe_texture_graveyard()
{
    static gpu_handle_graveyard g;
    return g;
}


const char *shader_basename_for( variant_kind v )
{
    switch( v ) {
        case variant_kind::SHADOW:
            return "grayscale.frag";
        case variant_kind::NIGHT:
            return "nightvision.frag";
        case variant_kind::OVEREXPOSED:
            return "overexposed.frag";
        case variant_kind::NORMAL:
        case variant_kind::MEMORY:
        case variant_kind::count:
            return nullptr;
    }
    return nullptr;
}

const char *shader_basename_for( memory_preset p )
{
    switch( p ) {
        case memory_preset::DARKEN:
            return "memory_darken.frag";
        case memory_preset::SEPIA_LIGHT:
            return "memory_sepia_light.frag";
        case memory_preset::SEPIA_DARK:
            return "memory_sepia_dark.frag";
        case memory_preset::BLUE_DARK:
            return "memory_blue_dark.frag";
        case memory_preset::count:
            return nullptr;
    }
    return nullptr;
}

using probe_predicate = bool ( * )( int r, int g, int b );

bool grayscale_predicate( int r, int g, int b )
{
    // Gray-toned (R~=G~=B within 8 LSB) AND strictly darker than the
    // mid-gray (128) input.
    return std::abs( r - g ) < 8 && std::abs( g - b ) < 8 && r < 120;
}

bool nightvision_predicate( int r, int g, int b )
{
    // Green-tinted: G clearly greater than R and B. Margin 20 covers driver
    // rounding without false-positiving the unmodulated gray (R==G==B).
    return g > r + 20 && g > b + 20;
}

bool darken_predicate( int r, int g, int b )
{
    // Memory darken multiplies by 85/256 (~1/3) of input mid-gray, so the
    // expected output is ~42 on each channel. Allow some driver slack.
    return std::abs( r - g ) < 8 && std::abs( g - b ) < 8 && r < 80;
}

bool warm_predicate( int r, int /*g*/, int b )
{
    // Sepia variants emit warm-tinted output: R clearly greater than B.
    return r > b + 5;
}

bool cool_predicate( int r, int /*g*/, int b )
{
    // Blue dark emits cool-tinted output: B clearly greater than R.
    return b > r + 5;
}

// red at strength 128/255, applied to the probe's mid-gray source
constexpr tint_texture_mod tint_probe_mod{ 255, 0, 0, 127 };

bool tint_predicate( int r, int g, int b )
{
    // identity shader returns source gray and fails
    const auto expected = []( const uint8_t mod_channel ) {
        return static_cast<int>( std::lround( tint_mix_channel( 128.0f, mod_channel,
                                              tint_probe_mod.a ) ) );
    };
    return std::abs( r - expected( tint_probe_mod.r ) ) <= 4 &&
           std::abs( g - expected( tint_probe_mod.g ) ) <= 4 &&
           std::abs( b - expected( tint_probe_mod.b ) ) <= 4;
}

probe_predicate predicate_for( variant_kind v )
{
    switch( v ) {
        case variant_kind::SHADOW:
            return grayscale_predicate;
        case variant_kind::NIGHT:
        case variant_kind::OVEREXPOSED:
            return nightvision_predicate;
        case variant_kind::NORMAL:
        case variant_kind::MEMORY:
        case variant_kind::count:
            return nullptr;
    }
    return nullptr;
}

probe_predicate predicate_for( memory_preset p )
{
    switch( p ) {
        case memory_preset::DARKEN:
            return darken_predicate;
        case memory_preset::SEPIA_LIGHT:
        case memory_preset::SEPIA_DARK:
            return warm_predicate;
        case memory_preset::BLUE_DARK:
            return cool_predicate;
        case memory_preset::count:
            return nullptr;
    }
    return nullptr;
}

} // namespace

bool variant_takes_tint( const variant_kind v )
{
    return v == variant_kind::NORMAL || v == variant_kind::SHADOW;
}

std::optional<memory_preset> memory_preset_from_option_value(
    const std::string &mode )
{
    if( mode == "color_pixel_darken" ) {
        return memory_preset::DARKEN;
    }
    if( mode == "color_pixel_sepia_light" ) {
        return memory_preset::SEPIA_LIGHT;
    }
    if( mode == "color_pixel_sepia_dark" ) {
        return memory_preset::SEPIA_DARK;
    }
    if( mode == "color_pixel_blue_dark" ) {
        return memory_preset::BLUE_DARK;
    }
    return std::nullopt;
}

// one count shared by all passes, so no two passes report the same generation
static uint32_t next_resource_generation()
{
    static uint32_t last = 0;
    return ++last;
}

variant_pass::variant_pass( SDL_Renderer *renderer )
    : renderer_( renderer ), resource_generation_( next_resource_generation() )
{
}

namespace lit_probe
{

static constexpr int probe_columns = 8;
static constexpr int probe_rows = 8;

// light texel of a probe map character: 'o' full, 'h' half, 'f' a fifth and
// 'z' no light, all white; 'r' and 'g' full red and green light; '#' a wall
// lit green; '1' to '5' white at that many fifths; ' ' out of sight
static smooth_lighting::lightmap_texel probe_texel( const char c )
{
    const std::array<float, 3> white = { 1.0f, 1.0f, 1.0f };
    const std::array<float, 3> red = { 1.0f, 0.2f, 0.2f };
    const std::array<float, 3> green = { 0.2f, 1.0f, 0.2f };
    switch( c ) {
        case 'o':
            return smooth_lighting::encode_light_texel( { 1.0f, true }, white, false );
        case 'h':
            return smooth_lighting::encode_light_texel( { 0.5f, true }, white, false );
        case 'f':
            return smooth_lighting::encode_light_texel( { 0.2f, true }, white, false );
        case 'z':
            return smooth_lighting::encode_light_texel( { 0.0f, true }, white, false );
        case 'r':
            return smooth_lighting::encode_light_texel( { 1.0f, true }, red, false );
        case 'g':
            return smooth_lighting::encode_light_texel( { 1.0f, true }, green, false );
        case '#':
            return smooth_lighting::encode_light_texel( { 1.0f, true }, green, true );
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
            return smooth_lighting::encode_light_texel( { ( c - '0' ) / 5.0f, true }, white, false );
        default:
            return smooth_lighting::lightmap_texel();
    }
}

namespace
{
// light map shape of a probe case: `levels` z levels of `columns` by `rows`
// cells; `own` is the level the case's cell is on
struct probe_layout {
    int columns = probe_columns;
    int rows = probe_rows;
    int levels = 3;
    int own = 1;
};
} // namespace

// probe case: `rows` fill level layout.own, reach masks come from reach_mask,
// and every quad corner has own cell `cell` and ground point `at`; `standing`
// marks a standing sprite
static probe_case make_case( const std::string &name, const std::vector<std::string> &rows,
                             const point &cell, const std::array<float, 2> &at, const bool standing,
                             const probe_layout &layout = probe_layout() )
{
    probe_case c;
    c.name = name;
    c.columns = layout.columns;
    c.rows_per_level = layout.rows;
    c.levels = layout.levels;
    const int width = 2 * layout.columns;
    const int top = layout.own * layout.rows;
    c.texels.assign( static_cast<size_t>( width ) * layout.rows * layout.levels,
                     smooth_lighting::lightmap_texel() );
    for( size_t y = 0; y < rows.size(); ++y ) {
        for( size_t x = 0; x < rows[y].size(); ++x ) {
            c.texels[( top + y ) * width + x] = probe_texel( rows[y][x] );
        }
    }
    const smooth_lighting::barrier_grid grid{ c.texels.data() + static_cast<size_t>( top ) *width,
            width, layout.columns, layout.rows };
    for( int y = 0; y < layout.rows; ++y ) {
        for( int x = 0; x < layout.columns; ++x ) {
            c.texels[( top + y ) * width + layout.columns + x] =
                smooth_lighting::encode_reach_texel( smooth_lighting::reach_mask( grid, point( x, y ) ) );
        }
    }
    c.frame.memory.preset = memory_preset::DARKEN;
    c.size = point::south_east;
    const smooth_lighting::lit_coords corner{ at[0], top + at[1],
            cell.x + ( standing ? smooth_lighting::standing_marker : 0.0f ),
            static_cast<float>( top + cell.y ) };
    c.corners = { corner, corner, corner, corner };
    return c;
}

smooth_lighting::lightmap_view view_of( const probe_case &c )
{
    return { c.texels.data(), 2 * c.columns, c.rows_per_level * c.levels, c.rows_per_level, c.columns };
}

smooth_lighting::prefilter_layout layout_of( const probe_case &c )
{
    const smooth_lighting::lit_coords &k = c.corners[0];
    const int row = static_cast<int>( std::floor( k.row + 0.5f ) );
    const int level = row / c.rows_per_level;
    const point own( static_cast<int>( std::floor( k.column + 0.5f *
                                       smooth_lighting::standing_marker ) ),
                     row - level * c.rows_per_level );
    return smooth_lighting::prefilter_layout_for( half_open_rectangle<point>( own,
            own + point::south_east ),
            half_open_rectangle<point>( point::zero, point( c.columns, c.rows_per_level ) ), level, 1 );
}

std::vector<probe_case> cases()
{
    const std::vector<std::string> half( probe_rows, "hhhhhhhh" );
    std::vector<probe_case> out;
    out.push_back( make_case( "per tile, half light", half, point( 3, 3 ), { 3.5f, 3.5f }, false ) );
    out.back().frame.per_tile = true;

    out.push_back( make_case( "per tile, no light", std::vector<std::string>( probe_rows,
                              "zzzzzzzz" ), point( 3, 3 ), { 3.5f, 3.5f }, false ) );
    out.back().frame.per_tile = true;
    out.back().frame.blend_memory = true;

    out.push_back( make_case( "per tile, out of sight, memory", {}, point( 3, 3 ), { 3.5f, 3.5f },
                              false ) );
    out.back().frame.per_tile = true;
    out.back().frame.blend_memory = true;

    out.push_back( make_case( "per tile, out of sight, custom memory", {}, point( 3, 3 ), { 3.5f, 3.5f },
                              false ) );
    out.back().frame.per_tile = true;
    out.back().frame.blend_memory = true;
    out.back().frame.memory.preset = std::nullopt;
    out.back().frame.memory.custom_dark = { 0.1f, 0.2f, 0.6f };
    out.back().frame.memory.custom_light = { 0.9f, 0.5f, 0.1f };
    out.back().frame.memory.custom_gamma = 1.6f;

    // standing ortho sprite takes its light from the middle of its own row,
    // red; at its top edge, as a ground sprite, half green
    out.push_back( make_case( "ortho standing", {
        "gggggggg", "gggggggg", "gggggggg", "rrrrrrrr", "rrrrrrrr", "rrrrrrrr", "rrrrrrrr", "rrrrrrrr"
    }, point( 3, 3 ), { 3.5f, 3.0f }, true ) );

    // standing iso sprite at its cell's bottom left corner takes the light of
    // its own red cell's center; ground sampling there would read the green
    // south and west
    out.push_back( make_case( "iso standing", {
        "gggrrrrr", "gggrrrrr", "gggrrrrr", "gggrrrrr", "gggggggg", "gggggggg", "gggggggg", "gggggggg"
    }, point( 3, 3 ), { 3.0f, 4.0f }, true ) );
    out.back().frame.iso = true;

    // a wall between the cell and the green beyond it: red only
    out.push_back( make_case( "barrier", std::vector<std::string>( probe_rows, "gg#rrrrr" ),
                              point( 3, 3 ), { 3.0f, 3.5f }, false ) );

    // both sides of a floor edge near a wall agree
    const std::vector<std::string> seam = { "ffffffff", "frffffff", "f#ffffff", "ffffffff",
                                            "ffffffff", "ffffffff", "ffffffff", "ffffffff"
                                          };
    out.push_back( make_case( "seam, left cell", seam, point( 1, 3 ), { 2.0f, 3.5f }, false ) );
    out.push_back( make_case( "seam, right cell", seam, point( 2, 3 ), { 2.0f, 3.5f }, false ) );

    // last light column; the reach masks beside it are not light
    out.push_back( make_case( "edge of the light columns", half, point( 7, 3 ), { 7.9f, 3.5f },
                              false ) );

    // light gradient across a quad four pixels wide
    probe_case gradient = make_case( "coordinates across a quad", std::vector<std::string>( probe_rows,
                                     "12345555" ), point( 2, 3 ), { 2.0f, 3.5f }, false );
    gradient.size = point( 4, 1 );
    gradient.corners[1].x = 3.0f;
    gradient.corners[2].x = 3.0f;
    out.push_back( gradient );

    // diagonal edge from dark to full light across a 4x4 quad
    probe_case grid = make_case( "coordinates across a square", {
        "zzzzzooo", "zzzzoooo", "zzzooooo", "zzoooooo", "zooooooo", "oooooooo", "oooooooo", "oooooooo"
    }, point( 2, 2 ), { 2.0f, 2.0f }, false );
    grid.size = point( 4, 4 );
    grid.corners[1].x = 3.0f;
    grid.corners[2].x = 3.0f;
    grid.corners[2].y += 1.0f;
    grid.corners[3].y += 1.0f;
    out.push_back( grid );

    // quad running into the sight edge, part way into the memory look
    probe_case edge = make_case( "into the edge of sight", std::vector<std::string>( probe_rows,
                                 "oooo    " ), point( 3, 3 ), { 3.0f, 3.5f }, false );
    edge.size = point( 4, 1 );
    edge.corners[1].x = 4.0f;
    edge.corners[2].x = 4.0f;
    edge.frame.blend_memory = true;
    out.push_back( edge );

    // colored artwork drained toward gray by half light
    out.push_back( make_case( "colored sprite, half light", half, point( 3, 3 ), { 3.5f, 3.5f },
                              false ) );
    out.back().source = { 220, 120, 30, 255 };

    // a translucent sprite edge blends over what is under it
    out.push_back( make_case( "translucent sprite", half, point( 3, 3 ), { 3.5f, 3.5f }, false ) );
    out.back().source = { 220, 120, 30, 128 };

    // corner of a full size level in the real stack, red full light on the
    // levels above and below it
    const probe_layout packed{ MAPSIZE_X, MAPSIZE_Y, OVERMAP_LAYERS, OVERMAP_LAYERS - 2 };
    probe_case corner = make_case( "corner of a packed level", std::vector<std::string>( MAPSIZE_Y,
                                   std::string( MAPSIZE_X, 'f' ) ), point( MAPSIZE_X - 1, MAPSIZE_Y - 1 ),
    { MAPSIZE_X - 0.02f, MAPSIZE_Y - 0.02f }, false, packed );
    const int packed_width = 2 * MAPSIZE_X;
    for( const int level : {
             packed.own - 1, packed.own + 1
         } ) {
        for( int y = 0; y < MAPSIZE_Y; ++y ) {
            for( int x = 0; x < MAPSIZE_X; ++x ) {
                corner.texels[( level * MAPSIZE_Y + y ) * packed_width + x] = probe_texel( 'r' );
            }
        }
    }
    out.push_back( corner );

    // a cell joined to an unseen neighbour and, past a wall, to a seen
    // diagonal: its sight fraction is a ratio that changes fastest here
    probe_case steep = make_case( "steep sight ratio", {
        "o o o o ", "#o#o#o#o", "o o o o ", "#o#o#o#o", "o o o o ", "#o#o#o#o", "o o o o ", "#o#o#o#o"
    }, point( 2, 2 ), { 2.0f, 2.0f }, false );
    steep.size = point( 4, 4 );
    steep.corners[1].x = 3.0f;
    steep.corners[2].x = 3.0f;
    steep.corners[2].y += 1.0f;
    steep.corners[3].y += 1.0f;
    out.push_back( steep );

    // filtered night vision from full light, overexposed, out of sight
    probe_case night_edge = make_case( "night, filtered, edge of sight",
                                       std::vector<std::string>( probe_rows, "oooo    " ), point( 3, 3 ), { 3.0f, 3.5f }, false );
    night_edge.size = point( 4, 1 );
    night_edge.corners[1].x = 4.0f;
    night_edge.corners[2].x = 4.0f;
    night_edge.night = true;
    out.push_back( night_edge );

    const std::vector<std::string> full( probe_rows, "oooooooo" );
    out.push_back( make_case( "night, full light", full, point( 3, 3 ), { 3.5f, 3.5f }, false ) );
    out.back().frame.per_tile = true;
    out.back().night = true;

    out.push_back( make_case( "night, half light", half, point( 3, 3 ), { 3.5f, 3.5f }, false ) );
    out.back().frame.per_tile = true;
    out.back().night = true;
    return out;
}

static std::array<float, 3> source_rgb( const probe_case &c )
{
    return { c.source[0] / 255.0f, c.source[1] / 255.0f, c.source[2] / 255.0f };
}

// shader output blended over the black target
static std::array<float, 3> over_black( const probe_case &c, const std::array<float, 3> &rgb )
{
    const float a = c.source[3] / 255.0f;
    return { rgb[0] *a, rgb[1] *a, rgb[2] *a };
}

static smooth_lighting::look_params look_of( const lit_frame &frame )
{
    smooth_lighting::look_params look;
    look.memory_look = frame.memory.preset ? static_cast<int>( *frame.memory.preset ) :
                       smooth_lighting::custom_look;
    look.blend_memory = frame.blend_memory;
    look.custom_dark = frame.memory.custom_dark;
    look.custom_light = frame.memory.custom_light;
    look.custom_gamma = frame.memory.custom_gamma;
    return look;
}

static rgb to_rgb( const std::array<float, 3> &c )
{
    const auto byte = []( const float v ) {
        return static_cast<int>( std::lround( 255.0f * std::clamp( v, 0.0f, 1.0f ) ) );
    };
    return { byte( c[0] ), byte( c[1] ), byte( c[2] ) };
}

// the vertex color the rasterizer gives the center of pixel ( i, j )
static smooth_lighting::lit_coords at_pixel( const probe_case &c, const int i, const int j )
{
    const float u = ( i + 0.5f ) / c.size.x;
    const float v = ( j + 0.5f ) / c.size.y;
    const auto lerp = [&]( const float tl, const float tr, const float br, const float bl ) {
        return ( tl + ( tr - tl ) * u ) * ( 1.0f - v ) + ( bl + ( br - bl ) * u ) * v;
    };
    const std::array<smooth_lighting::lit_coords, 4> &k = c.corners;
    return { lerp( k[0].x, k[1].x, k[2].x, k[3].x ), lerp( k[0].y, k[1].y, k[2].y, k[3].y ),
             lerp( k[0].column, k[1].column, k[2].column, k[3].column ),
             lerp( k[0].row, k[1].row, k[2].row, k[3].row ) };
}

static std::vector<rgb> readback_of( const probe_case &c,
                                     const std::function<smooth_lighting::lit_sample( const smooth_lighting::lit_coords & )> &sample )
{
    std::vector<rgb> out;
    const smooth_lighting::look_params look = look_of( c.frame );
    for( int j = 0; j < c.size.y; ++j ) {
        for( int i = 0; i < c.size.x; ++i ) {
            const smooth_lighting::lit_sample s = sample( at_pixel( c, i, j ) );
            const std::array<float, 3> rgb = source_rgb( c );
            out.push_back( to_rgb( over_black( c, c.night ? smooth_lighting::reference_night_rgb( look, rgb,
                                               s ) : smooth_lighting::reference_lit_rgb( look, rgb, s ) ) ) );
        }
    }
    return out;
}

std::vector<rgb> expected( const probe_case &c )
{
    const smooth_lighting::lightmap_view view = view_of( c );
    const smooth_lighting::sample_params params{ c.frame.per_tile, c.frame.iso };
    if( c.frame.per_tile ) {
        return readback_of( c, [&]( const smooth_lighting::lit_coords & coords ) {
            return smooth_lighting::reference_sample( view, params, coords );
        } );
    }
    const smooth_lighting::prefilter_table table = smooth_lighting::build_prefilter_table( view,
            layout_of( c ) );
    return readback_of( c, [&]( const smooth_lighting::lit_coords & coords ) {
        return smooth_lighting::reference_prefiltered_sample( table, view, params, coords );
    } );
}

std::vector<rgb> readback_if_full_light( const probe_case &c )
{
    return readback_of( c, []( const smooth_lighting::lit_coords & ) {
        smooth_lighting::lit_sample s;
        s.light = 1.0f;
        s.visible = 1.0f;
        return s;
    } );
}

std::vector<rgb> readback_if_ignored( const probe_case &c )
{
    return std::vector<rgb>( static_cast<size_t>( c.size.x ) * c.size.y, to_rgb( over_black( c,
                             source_rgb( c ) ) ) );
}

bool matches( const std::vector<rgb> &expected, const std::vector<rgb> &readback )
{
    if( expected.size() != readback.size() ) {
        return false;
    }
    for( size_t i = 0; i < expected.size(); ++i ) {
        if( std::abs( expected[i].r - readback[i].r ) > channel_tolerance ||
            std::abs( expected[i].g - readback[i].g ) > channel_tolerance ||
            std::abs( expected[i].b - readback[i].b ) > channel_tolerance ) {
            return false;
        }
    }
    return true;
}

} // namespace lit_probe

variant_pass::~variant_pass()
{
    const bool flushed = flush();
    // On flush failure the handles may still be referenced; abandon rather
    // than have RAII teardown call SDL on a still-bound resource.
    clear_state_arrays( !flushed );
}

namespace
{

struct probe_result {
    bool ok = false;
    // False when the renderer was left with an undefined shader-state bind
    // or with rt still active as the target. Caller must NOT destroy any
    // resources that the renderer might still reference.
    bool boundary_safe = true;
};

struct target_readback {
    // draw call ran and every pixel came back
    bool read = false;
    // as probe_result::boundary_safe
    bool boundary_safe = true;
    // RGBA per pixel, row by row
    std::vector<std::array<Uint8, 4>> pixels;
};

// Binds a `size` render target, draws a 1x1 `source` texel through `state`
// with `draw`, and reads the target back.
target_readback draw_into_probe_target( SDL_Renderer *renderer, SDL_GPURenderState *state,
                                        const point &size, const std::array<Uint8, 4> &source,
                                        const std::optional<tint_texture_mod> &mod,
                                        const std::function<bool( SDL_Texture * )> &draw )
{
    target_readback res;
    SDL_Surface *src_surf = SDL_CreateSurface( 1, 1, SDL_PIXELFORMAT_RGBA32 );
    if( !src_surf ) {
        return res;
    }
    SDL_FillSurfaceRect( src_surf, nullptr, SDL_MapSurfaceRGBA( src_surf, source[0], source[1],
                         source[2], source[3] ) );
    SDL_Texture *src = SDL_CreateTextureFromSurface( renderer, src_surf );
    SDL_DestroySurface( src_surf );
    if( !src ) {
        return res;
    }
    // blended as sprites are, over the cleared black target
    SDL_SetTextureBlendMode( src, SDL_BLENDMODE_BLEND );
    if( mod ) {
        // src is local to the probe, so nothing else sees the mod
        SDL_SetTextureColorMod( src, mod->r, mod->g, mod->b );
        SDL_SetTextureAlphaMod( src, mod->a );
    }
    SDL_Texture *rt = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_TARGET, size.x, size.y );
    if( !rt ) {
        SDL_DestroyTexture( src );
        return res;
    }
    SDL_Texture *prior_target = SDL_GetRenderTarget( renderer );
    const bool target_bound = SDL_SetRenderTarget( renderer, rt );
    if( !target_bound ) {
        // SDL may mutate target state before failing, so this is not a clean
        // no-op: mark the boundary unsafe and graveyard the probe textures.
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::draw_probe: SDL_SetRenderTarget(rt) failed: "
                << SDL_GetError();
        res.boundary_safe = false;
    }
    if( target_bound ) {
        SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
        SDL_RenderClear( renderer );
        if( SDL_SetGPURenderState( renderer, state ) ) {
            const bool drew = draw( src );
            // Unbind shader state before any further switch/readback. On
            // failure the bind is still held, so skip rt + restore.
            const bool unbound = SDL_SetGPURenderState( renderer, nullptr );
            if( drew && unbound ) {
                const SDL_Rect rect{ 0, 0, size.x, size.y };
                SDL_Surface *out = SDL_RenderReadPixels( renderer, &rect );
                if( out ) {
                    SDL_Surface *rgba = out->format == SDL_PIXELFORMAT_RGBA32
                                        ? out
                                        : SDL_ConvertSurface( out, SDL_PIXELFORMAT_RGBA32 );
                    if( rgba != out ) {
                        SDL_DestroySurface( out );
                    }
                    if( rgba ) {
                        for( int y = 0; y < size.y; ++y ) {
                            const Uint8 *row = static_cast<const Uint8 *>( rgba->pixels ) + y * rgba->pitch;
                            for( int x = 0; x < size.x; ++x ) {
                                res.pixels.push_back( { row[x * 4], row[x * 4 + 1], row[x * 4 + 2], row[x * 4 + 3] } );
                            }
                        }
                        res.read = true;
                        SDL_DestroySurface( rgba );
                    }
                }
            }
            if( !unbound ) {
                DebugLog( D_ERROR, DC_ALL )
                        << "cata_shader::draw_probe: SDL_SetGPURenderState(NULL) failed: "
                        << SDL_GetError();
                res.read = false;
                res.boundary_safe = false;
            }
        } else {
            // Bind state undefined after a failed SDL_SetGPURenderState:
            // assume still held, so do not ship rt/probe through teardown.
            DebugLog( D_ERROR, DC_ALL )
                    << "cata_shader::draw_probe: SDL_SetGPURenderState failed: "
                    << SDL_GetError();
            res.boundary_safe = false;
        }
        if( res.boundary_safe && !SDL_SetRenderTarget( renderer, prior_target ) ) {
            DebugLog( D_ERROR, DC_ALL )
                    << "cata_shader::draw_probe: SDL_SetRenderTarget restore failed: "
                    << SDL_GetError();
            res.read = false;
            // rt is still the active target: mark unsafe so neither rt nor
            // src is destroyed and the caller refuses the still-bound state.
            res.boundary_safe = false;
        }
    }
    // Preserve rt and src when the renderer state is undefined: destroying
    // them here would invalidate resources the renderer still references.
    if( res.boundary_safe ) {
        SDL_DestroyTexture( rt );
        SDL_DestroyTexture( src );
    } else {
        probe_texture_graveyard().add( rt );
        probe_texture_graveyard().add( src );
    }
    return res;
}

// Renders a 1x1 mid-gray sprite via state and checks readback via pred.
probe_result draw_probe( SDL_Renderer *renderer, SDL_GPURenderState *state,
                         probe_predicate pred, const std::optional<tint_texture_mod> &mod )
{
    // mid gray gives each variant a distinctive output, so a passing check
    // tells "shader ran" from "bind silently ignored"
    static constexpr std::array<Uint8, 4> mid_gray = { 128, 128, 128, 255 };
    const target_readback rb = draw_into_probe_target( renderer, state, point::south_east, mid_gray,
                               mod,
    [renderer]( SDL_Texture * src ) {
        const SDL_FRect dst{ 0.0f, 0.0f, 1.0f, 1.0f };
        return SDL_RenderTexture( renderer, src, nullptr, &dst );
    } );
    probe_result res;
    res.boundary_safe = rb.boundary_safe;
    if( rb.read && !rb.pixels.empty() ) {
        const std::array<Uint8, 4> &p = rb.pixels.front();
        res.ok = p[3] >= 250 && pred && pred( p[0], p[1], p[2] );
    }
    return res;
}

} // namespace

namespace
{

enum class load_outcome {
    ok,
    failed_clean,
    failed_unsafe,
};

// Loads shader + render_state for basename and validates via draw_probe.
// On a probe boundary failure the local shader/state are abandoned so their
// destructors do not release SDL handles the renderer may still reference.
load_outcome load_and_probe( SDL_GPUDevice *device, SDL_Renderer *renderer,
                             const char *basename, probe_predicate pred,
                             shader &out_shader, render_state &out_state,
                             const std::optional<tint_texture_mod> &mod = std::nullopt )
{
    shader frag = shader::load_fragment( device, basename, 1, 0 );
    if( !frag.is_valid() ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: shader load failed for "
                << basename << "; shader variant path disabled";
        return load_outcome::failed_clean;
    }
    render_state state = render_state::create( renderer, frag );
    if( !state.is_valid() ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: render_state::create failed for "
                << basename << "; shader variant path disabled";
        return load_outcome::failed_clean;
    }
    const probe_result res = draw_probe( renderer, state.get(), pred, mod );
    if( !res.boundary_safe ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: probe left renderer in undefined "
                "state for " << basename << "; abandoning shader + render_state "
                "to avoid destroying a bound resource";
        state.abandon();
        frag.abandon();
        return load_outcome::failed_unsafe;
    }
    if( !res.ok ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: textured-draw probe failed for "
                << basename << " (silent miswire?  readback did not match "
                "expected variant transform); shader variant path disabled";
        return load_outcome::failed_clean;
    }
    out_shader = std::move( frag );
    out_state = std::move( state );
    return load_outcome::ok;
}

} // namespace

probe_state variant_pass::ensure_probed()
{
    if( abandoned_pending_rebind_ || boundary_lost_ ) {
        return probe_state::unsafe;
    }
    if( reprobe_requested() ) {
        reset();
        clear_reprobe();
        if( boundary_lost_ ) {
            return probe_state::unsafe;
        }
    }
    if( shader_fault_ ) {
        return probe_state::unavailable;
    }
    if( !probe_attempted_ ) {
        probe();
        if( boundary_lost_ ) {
            return probe_state::unsafe;
        }
    }
    return available() ? probe_state::available : probe_state::unavailable;
}

void variant_pass::reset()
{
    const bool flushed = flush();
    clear_state_arrays( !flushed );
    if( !flushed ) {
        // renderer might still hold the bind, keep probe_attempted_ so nothing
        // probes against it
        boundary_lost_ = true;
        shader_fault_ = true;
        return;
    }
    probe_attempted_ = false;
    probed_ok_ = false;
    session_disabled_ = false;
    unbind_required_ = false;
    shader_fault_ = false;
}

void variant_pass::mark_probe_unsafe()
{
    unbind_required_ = true;
    session_disabled_ = true;
    boundary_lost_ = true;
    shader_fault_ = true;
}

void variant_pass::mark_flush_failed()
{
    session_disabled_ = true;
    boundary_lost_ = true;
    shader_fault_ = true;
}

void variant_pass::note_draw_bind_failure( const bool log_error )
{
    if( log_error ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: SDL_SetGPURenderState failed: "
                << SDL_GetError();
    }
    session_disabled_ = true;
    unbind_required_ = true;
    boundary_lost_ = true;
    shader_fault_ = true;
}

void variant_pass::clear_state_arrays( bool abandon_handles )
{
    if( abandon_handles ) {
        for( shader &s : shaders_ ) {
            s.abandon();
        }
        for( render_state &s : states_ ) {
            s.abandon();
        }
        for( shader &s : memory_shaders_ ) {
            s.abandon();
        }
        for( render_state &s : memory_states_ ) {
            s.abandon();
        }
        tint_shader_.abandon();
        tint_state_.abandon();
    }
    release_lit( abandon_handles );
    resource_generation_ = next_resource_generation();
    // rebuilt resources are probed again
    lit_capability_.reset();
    // Render states reference their fragment shader; clear states before
    // shaders so SDL does not see a dangling reference on the clean path.
    states_ = {};
    memory_states_ = {};
    tint_state_ = {};
    shaders_ = {};
    memory_shaders_ = {};
    tint_shader_ = {};
}

void variant_pass::probe()
{
    probe_attempted_ = true;
    ++test_probe_run_count;
    if( !renderer_ ) {
        return;
    }
    if( std::getenv( "CATA_DISABLE_SPRITE_SHADERS" ) ) {
        DebugLog( D_INFO, DC_ALL )
                << "cata_shader::variant_pass: CATA_DISABLE_SPRITE_SHADERS set; "
                "shader variant path disabled";
        return;
    }
    if( test_probe_unsafe_countdown > 0 ) {
        --test_probe_unsafe_countdown;
        DebugLog( D_INFO, DC_ALL ) << "cata_shader::variant_pass: armed unsafe probe";
        mark_probe_unsafe();
        return;
    }
    SDL_GPUDevice *const device = SDL_GetGPURendererDevice( renderer_ );
    if( !device ) {
        DebugLog( D_INFO, DC_ALL )
                << "cata_shader::variant_pass: SDL_GetGPURendererDevice "
                "returned NULL (renderer is not gpu); shader variant path disabled";
        return;
    }
    for( int i = 0; i < static_cast<int>( variant_kind::count ); ++i ) {
        const variant_kind v = static_cast<variant_kind>( i );
        const char *basename = shader_basename_for( v );
        if( !basename ) {
            continue;
        }
        const load_outcome o = load_and_probe( device, renderer_, basename,
                                               predicate_for( v ), shaders_[i], states_[i] );
        if( o == load_outcome::failed_unsafe ) {
            mark_probe_unsafe();
            return;
        }
        if( o == load_outcome::failed_clean ) {
            return;
        }
    }
    for( int i = 0; i < static_cast<int>( memory_preset::count ); ++i ) {
        const memory_preset p = static_cast<memory_preset>( i );
        const char *basename = shader_basename_for( p );
        if( !basename ) {
            continue;
        }
        const load_outcome o = load_and_probe( device, renderer_, basename,
                                               predicate_for( p ), memory_shaders_[i], memory_states_[i] );
        if( o == load_outcome::failed_unsafe ) {
            mark_probe_unsafe();
            return;
        }
        if( o == load_outcome::failed_clean ) {
            return;
        }
    }
    const load_outcome tint = load_and_probe( device, renderer_, "tint.frag", tint_predicate,
                              tint_shader_, tint_state_, tint_probe_mod );
    if( tint == load_outcome::failed_unsafe ) {
        mark_probe_unsafe();
        return;
    }
    if( tint == load_outcome::failed_clean ) {
        return;
    }
    probed_ok_ = true;
}

bool variant_pass::lit_takes( const variant_kind v ) const
{
    if( !lit_active_ || lit_suspended_ ) {
        return false;
    }
    // memory blends back toward the lit look near the edge of sight
    return v == variant_kind::NORMAL || v == variant_kind::SHADOW || v == variant_kind::NIGHT ||
           v == variant_kind::OVEREXPOSED ||
           ( v == variant_kind::MEMORY && lit_params_.mode[2] != 0 );
}

SDL_GPURenderState *variant_pass::state_for( variant_kind v, const bool tinted ) const
{
    if( lit_takes( v ) ) {
        // light carries the tint, so tinted doesn't matter
        const bool night = v == variant_kind::NIGHT || v == variant_kind::OVEREXPOSED ||
                           ( v == variant_kind::MEMORY && lit_night_vision_ );
        return night ? nv_lit_state_.get() :
               lit_state_.get();
    }
    if( v == variant_kind::NORMAL && tinted ) {
        return tint_state_.get();
    }
    if( v == variant_kind::MEMORY ) {
        if( !active_memory_preset_ ) {
            return nullptr;
        }
        return memory_states_[static_cast<size_t>( *active_memory_preset_ )].get();
    }
    return states_[static_cast<size_t>( v )].get();
}

variant_pass::begin_result variant_pass::try_begin( variant_kind v, const bool tinted )
{
    if( abandoned_pending_rebind_ || boundary_lost_ ) {
        // embargo or lost boundary: refuse without calling SDL until rebind
        return begin_result::abort_frame;
    }
    if( reprobe_requested() ) {
        reset();
        clear_reprobe();
        if( boundary_lost_ ) {
            return begin_result::abort_frame;
        }
    }
    if( shader_fault_ || session_disabled_ ) {
        // Drop held bind so that a faulted or disabled session doesn't leave
        // any shader state on later draws. A failed flush leaves the renderer
        // undefined.
        if( !flush() ) {
            return begin_result::abort_frame;
        }
        return begin_result::use_atlas;
    }
    if( !probe_attempted_ ) {
        probe();
        if( boundary_lost_ ) {
            return begin_result::abort_frame;
        }
    }
    if( !probed_ok_ ) {
        return begin_result::use_atlas;
    }
    SDL_GPURenderState *target = state_for( v, tinted );
    if( target == bound_state_ ) {
        return target != nullptr ? begin_result::bound : begin_result::use_atlas;
    }
    if( !SDL_SetGPURenderState( renderer_, target ) ) {
        // bind state is undefined after failure, treat as held
        note_draw_bind_failure();
        return begin_result::abort_frame;
    }
    bound_state_ = target;
    unbind_required_ = false;
    return target != nullptr ? begin_result::bound : begin_result::use_atlas;
}

bool variant_pass::end()
{
    return true;
}

bool variant_pass::flush()
{
    if( abandoned_pending_rebind_ || boundary_lost_ ) {
        // Refuse without SDL: false tells every caller not to cross a render
        // target boundary until rebind_renderer.
        return false;
    }
    if( test_flush_failure_armed ) {
        // stands in for a failed SDL_SetGPURenderState(NULL) even with nothing
        // bound; the software fixture never binds
        test_flush_failure_armed = false;
        DebugLog( D_INFO, DC_ALL ) << "cata_shader::variant_pass: armed flush failure";
        mark_flush_failed();
        return false;
    }
    if( !bound_state_ && !unbind_required_ ) {
        return true;
    }
    if( !SDL_SetGPURenderState( renderer_, nullptr ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: SDL_SetGPURenderState(NULL) failed: "
                << SDL_GetError();
        mark_flush_failed();
        return false;
    }
    bound_state_ = nullptr;
    unbind_required_ = false;
    return true;
}

void variant_pass::select_memory_preset( std::optional<memory_preset> preset )
{
    active_memory_preset_ = preset;
}

void variant_pass::release_lit( const bool abandon_handles )
{
    if( abandon_handles ) {
        lit_state_.abandon();
        nv_lit_state_.abandon();
        prefilter_state_.abandon();
        lit_shader_.abandon();
        nv_lit_shader_.abandon();
        prefilter_shader_.abandon();
    }
    lit_state_ = {};
    nv_lit_state_ = {};
    prefilter_state_ = {};
    lit_shader_ = {};
    nv_lit_shader_ = {};
    prefilter_shader_ = {};
    if( lit_device_ && !abandon_handles ) {
        for( SDL_GPUSampler *sampler : {
                 lit_sampler_, lit_linear_sampler_
             } ) {
            if( sampler ) {
                SDL_ReleaseGPUSampler( lit_device_, sampler );
            }
        }
    }
    lit_sampler_ = nullptr;
    lit_linear_sampler_ = nullptr;
    lit_device_ = nullptr;
    lit_texture_ = nullptr;
    lit_bound_sampler_ = nullptr;
    lit_params_ = lit_params();
    lit_active_ = false;
}

variant_pass::lit_params variant_pass::make_lit_params( const lit_frame &frame )
{
    lit_params params;
    params.size = { smooth_lighting::lightmap_width, smooth_lighting::lightmap_height, MAPSIZE_Y,
                    smooth_lighting::reach_column
                  };
    params.mode = { frame.memory.preset ? static_cast<int32_t>( *frame.memory.preset ) : custom_memory_look,
                    frame.per_tile ? 1 : 0, frame.blend_memory ? 1 : 0, frame.iso ? 1 : 0
                  };
    params.flags = { smooth_lighting::texel_detail, smooth_lighting::texel_barrier, custom_memory_look, 0 };
    params.tone = { smooth_lighting::shadow_shade, smooth_lighting::standing_marker, smooth_lighting::full_color_light,
                    smooth_lighting::night_floor
                  };
    params.look = { smooth_lighting::overexpose_start, smooth_lighting::tint_mix, 0.0f, 0.0f };
    params.custom_dark = { frame.memory.custom_dark[0], frame.memory.custom_dark[1],
                           frame.memory.custom_dark[2], frame.memory.custom_gamma
                         };
    params.custom_light = { frame.memory.custom_light[0], frame.memory.custom_light[1],
                            frame.memory.custom_light[2], 0.0f
                          };
    const smooth_lighting::prefilter_layout &layout = frame.layout;
    params.prefilter = { layout.area.p_min.x, layout.area.p_min.y,
                         layout.area.p_max.y - layout.area.p_min.y, layout.first_level
                       };
    params.prefilter_grid = { layout.grid, layout.area.p_max.x - layout.area.p_min.x, layout.levels,
                              frame.lookup == smooth_lighting::lookup::manual ? 1 : 0
                            };
    return params;
}

static SDL_GPUSampler *create_lit_sampler( SDL_GPUDevice *device, const SDL_GPUFilter filter )
{
    SDL_GPUSamplerCreateInfo info{};
    info.min_filter = filter;
    info.mag_filter = filter;
    info.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    info.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    info.address_mode_w = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    return SDL_CreateGPUSampler( device, &info );
}

static SDL_GPUTexture *gpu_texture_of( SDL_Texture *texture )
{
    return static_cast<SDL_GPUTexture *>( SDL_GetPointerProperty( SDL_GetTextureProperties( texture ),
                                          SDL_PROP_TEXTURE_GPU_TEXTURE_POINTER, nullptr ) );
}

SDL_Texture *create_prefilter_target( SDL_Renderer *renderer, const point &size )
{
    const SDL_PropertiesID props = SDL_CreateProperties();
    if( !props ) {
        return nullptr;
    }
    SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER, SDL_PIXELFORMAT_RGBA64_FLOAT );
    SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER, SDL_TEXTUREACCESS_TARGET );
    SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, size.x );
    SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, size.y );
    SDL_SetNumberProperty( props, SDL_PROP_TEXTURE_CREATE_COLORSPACE_NUMBER, SDL_COLORSPACE_SRGB );
    SDL_Texture *texture = SDL_CreateTextureWithProperties( renderer, props );
    SDL_DestroyProperties( props );
    if( texture ) {
        // texels are light values the sprites interpolate, never scaled
        SDL_SetTextureBlendMode( texture, SDL_BLENDMODE_NONE );
    }
    return texture;
}

lit_prepare_result variant_pass::prepare_lit()
{
    using smooth_lighting::lit_failure;
    const auto failed = []( const lit_failure f ) {
        return lit_prepare_result{ lit_begin_outcome::failed, f, {} };
    };
    const lit_prepare_result abort{ lit_begin_outcome::abort_frame, std::nullopt, {} };
    if( abandoned_pending_rebind_ || boundary_lost_ ) {
        return abort;
    }
    if( !available() ) {
        return {};
    }
    if( !lit_shader_.is_valid() ) {
        SDL_GPUDevice *const device = SDL_GetGPURendererDevice( renderer_ );
        if( !device ) {
            DebugLog( D_ERROR, DC_ALL ) << "cata_shader::variant_pass: renderer has no GPU device";
            return failed( lit_failure::state_create );
        }
        lit_device_ = device;
        // lit_sample.glsl reads light map texels whole with texelFetch; the
        // hardware lookup filters the prefiltered light
        lit_sampler_ = create_lit_sampler( device, SDL_GPU_FILTER_NEAREST );
        if( !lit_sampler_ ) {
            DebugLog( D_ERROR, DC_ALL )
                    << "cata_shader::variant_pass: SDL_CreateGPUSampler failed: " << SDL_GetError();
            release_lit( false );
            return failed( lit_failure::sampler_create );
        }
        // without it filtered light takes the manual lookup
        lit_linear_sampler_ = create_lit_sampler( device, SDL_GPU_FILTER_LINEAR );
        if( !lit_linear_sampler_ ) {
            DebugLog( D_WARNING, DC_ALL )
                    << "cata_shader::variant_pass: linear SDL_CreateGPUSampler failed: " << SDL_GetError();
        }
        lit_shader_ = shader::load_fragment( device, "lit.frag", 2, 1 );
        nv_lit_shader_ = shader::load_fragment( device, "nightvision_lit.frag", 2, 1 );
        if( !lit_shader_.is_valid() || !nv_lit_shader_.is_valid() ) {
            release_lit( false );
            return failed( lit_failure::shader_load );
        }
        // without it filtered light is unavailable, per tile light still draws
        prefilter_shader_ = shader::load_fragment( device, "lit_prefilter.frag", 1, 1 );
        if( prefilter_shader_.is_valid() ) {
            prefilter_state_ = render_state::create( renderer_, prefilter_shader_ );
        }
    }
    if( !lit_capability_ ) {
        const smooth_lighting::lit_capability capability = probe_lit();
        if( capability.unsafe ) {
            release_lit( true );
            mark_probe_unsafe();
            return abort;
        }
        if( !capability.per_tile ) {
            release_lit( false );
            return failed( lit_failure::probe_mismatch );
        }
        lit_capability_ = capability;
    }
    return { lit_begin_outcome::active, std::nullopt,
             smooth_lighting::with_linear_sampler( *lit_capability_, lit_linear_sampler_ != nullptr ) };
}

lit_begin_result variant_pass::begin_lit( const lit_frame &frame )
{
    using smooth_lighting::lit_failure;
    const lit_begin_result abort{ lit_begin_outcome::abort_frame, std::nullopt };
    lit_active_ = false;
    const lit_prepare_result prepared = prepare_lit();
    if( prepared.outcome != lit_begin_outcome::active ) {
        return { prepared.outcome, prepared.failure };
    }
    SDL_Texture *const bound = frame.per_tile ? frame.lightmap : frame.prefiltered;
    SDL_GPUSampler *const sampler = frame.per_tile ||
                                    frame.lookup == smooth_lighting::lookup::manual ? lit_sampler_ : lit_linear_sampler_;
    if( !bound ) {
        return {};
    }
    if( !sampler ) {
        return { lit_begin_outcome::failed, lit_failure::sampler_create };
    }
    const bool fresh = !lit_state_.is_valid() || bound != lit_texture_ ||
                       sampler != lit_bound_sampler_;
    if( fresh ) {
        if( lit_state_.is_valid() ) {
            // queued draws still run the old states
            if( !flush() ) {
                return abort;
            }
            lit_state_ = {};
            nv_lit_state_ = {};
        }
        lit_texture_ = nullptr;
        lit_bound_sampler_ = nullptr;
        SDL_GPUTexture *const gpu_texture = gpu_texture_of( bound );
        if( !gpu_texture ) {
            DebugLog( D_ERROR, DC_ALL ) << "cata_shader::variant_pass: lit texture has no GPU texture";
            return { lit_begin_outcome::failed, lit_failure::state_create };
        }
        SDL_GPUTextureSamplerBinding binding{};
        binding.texture = gpu_texture;
        binding.sampler = sampler;
        lit_state_ = render_state::create( renderer_, lit_shader_, &binding, 1 );
        nv_lit_state_ = render_state::create( renderer_, nv_lit_shader_, &binding, 1 );
        if( !lit_state_.is_valid() || !nv_lit_state_.is_valid() ) {
            lit_state_ = {};
            nv_lit_state_ = {};
            return { lit_begin_outcome::failed, lit_failure::state_create };
        }
        lit_texture_ = bound;
        lit_bound_sampler_ = sampler;
    }
    const lit_params params = make_lit_params( frame );
    if( fresh || !( params == lit_params_ ) ) {
        if( !SDL_SetGPURenderStateFragmentUniforms( lit_state_.get(), 0, &params, sizeof( params ) ) ||
            !SDL_SetGPURenderStateFragmentUniforms( nv_lit_state_.get(), 0, &params,
                    sizeof( params ) ) ) {
            DebugLog( D_ERROR, DC_ALL )
                    << "cata_shader::variant_pass: lit uniforms failed: " << SDL_GetError();
            return { lit_begin_outcome::failed, lit_failure::uniform_upload };
        }
        lit_params_ = params;
    }
    lit_night_vision_ = frame.night_vision;
    lit_active_ = true;
    return { lit_begin_outcome::active, std::nullopt };
}

prefilter_outcome variant_pass::prefilter_lit( const lit_frame &frame )
{
    if( abandoned_pending_rebind_ || boundary_lost_ ) {
        return prefilter_outcome::unsafe;
    }
    if( !frame.lightmap || !frame.prefiltered || !prefilter_state_.is_valid() ) {
        return prefilter_outcome::failed;
    }
    const point size = frame.layout.size();
    if( size.x <= 0 || size.y <= 0 ) {
        // nothing seen: every sprite takes the dark constant
        return prefilter_outcome::ok;
    }
    // a held sprite state must not cross the target switch
    if( !flush() ) {
        return prefilter_outcome::unsafe;
    }
    return run_prefilter( frame.lightmap, frame.prefiltered, make_lit_params( frame ),
                          frame.layout.size() );
}

prefilter_outcome variant_pass::run_prefilter( SDL_Texture *lightmap, SDL_Texture *target,
        const lit_params &params, const point &size )
{
    SDL_Texture *const prior_target = SDL_GetRenderTarget( renderer_ );
    if( !SDL_SetRenderTarget( renderer_, target ) ) {
        DebugLog( D_ERROR, DC_ALL ) << "cata_shader::variant_pass: prefilter target bind failed: "
                                    << SDL_GetError();
        mark_probe_unsafe();
        return prefilter_outcome::unsafe;
    }
    bool clean = SDL_SetGPURenderStateFragmentUniforms( prefilter_state_.get(), 0, &params,
                 sizeof( params ) );
    clean = clean && SDL_SetRenderDrawColor( renderer_, 0, 0, 0, 0 ) && SDL_RenderClear( renderer_ );
    if( clean ) {
        if( !SDL_SetGPURenderState( renderer_, prefilter_state_.get() ) ) {
            // bind is undefined: leave target as is
            note_draw_bind_failure();
            return prefilter_outcome::unsafe;
        }
        // light map alpha is flags; drawn blended would scale the stored light
        SDL_SetTextureBlendMode( lightmap, SDL_BLENDMODE_NONE );
        const float w = static_cast<float>( size.x );
        const float h = static_cast<float>( size.y );
        // vertex colors carry target pixel coordinates
        const std::array<SDL_Vertex, 4> quad = { {
                { { 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f } },
                { { w, 0.0f }, { w, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f } },
                { { w, h }, { w, h, 0.0f, 0.0f }, { 1.0f, 1.0f } },
                { { 0.0f, h }, { 0.0f, h, 0.0f, 0.0f }, { 0.0f, 1.0f } }
            }
        };
        static constexpr std::array<int, 6> indices = { 0, 1, 2, 0, 2, 3 };
        clean = SDL_RenderGeometry( renderer_, lightmap, quad.data(), static_cast<int>( quad.size() ),
                                    indices.data(), static_cast<int>( indices.size() ) );
        if( !SDL_SetGPURenderState( renderer_, nullptr ) ) {
            note_draw_bind_failure();
            return prefilter_outcome::unsafe;
        }
    }
    if( !SDL_SetRenderTarget( renderer_, prior_target ) ) {
        DebugLog( D_ERROR, DC_ALL ) << "cata_shader::variant_pass: prefilter target restore failed: "
                                    << SDL_GetError();
        mark_probe_unsafe();
        return prefilter_outcome::unsafe;
    }
    if( !clean ) {
        DebugLog( D_ERROR, DC_ALL ) << "cata_shader::variant_pass: prefilter pass failed: "
                                    << SDL_GetError();
        return prefilter_outcome::failed;
    }
    return prefilter_outcome::ok;
}

smooth_lighting::lit_frame_action action_for( const lit_begin_outcome o )
{
    switch( o ) {
        case lit_begin_outcome::active:
            return smooth_lighting::lit_frame_action::draw_lit;
        case lit_begin_outcome::classic:
        case lit_begin_outcome::failed:
            return smooth_lighting::lit_frame_action::draw_classic;
        case lit_begin_outcome::abort_frame:
            return smooth_lighting::lit_frame_action::abort_frame;
    }
    return smooth_lighting::lit_frame_action::abort_frame;
}

smooth_lighting::probe_group_result variant_pass::probe_lit_group( const bool filtered,
        const smooth_lighting::lookup lookup )
{
    using smooth_lighting::probe_group_result;
    for( const lit_probe::probe_case &c : lit_probe::cases() ) {
        if( c.frame.per_tile == filtered ) {
            continue;
        }
        const smooth_lighting::lightmap_view view = lit_probe::view_of( c );
        SDL_Texture *tex = SDL_CreateTexture( renderer_, SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STATIC, view.width, view.height );
        if( !tex || !SDL_UpdateTexture( tex, nullptr, c.texels.data(), view.width * 4 ) ) {
            DebugLog( D_ERROR, DC_ALL ) << "cata_shader::variant_pass: lit probe texture failed: "
                                        << SDL_GetError();
            if( tex ) {
                SDL_DestroyTexture( tex );
            }
            return probe_group_result::mismatch;
        }
        lit_frame frame = c.frame;
        frame.lookup = lookup;
        lit_params params;
        SDL_Texture *prefiltered = nullptr;
        const auto destroy = [&]() {
            SDL_DestroyTexture( tex );
            if( prefiltered ) {
                SDL_DestroyTexture( prefiltered );
            }
        };
        const auto graveyard = [&]() {
            probe_texture_graveyard().add( tex );
            if( prefiltered ) {
                probe_texture_graveyard().add( prefiltered );
            }
        };
        if( filtered ) {
            frame.layout = lit_probe::layout_of( c );
            prefiltered = create_prefilter_target( renderer_, frame.layout.size() );
            if( !prefiltered ) {
                DebugLog( D_ERROR, DC_ALL ) << "cata_shader::variant_pass: prefilter target failed: "
                                            << SDL_GetError();
                destroy();
                return probe_group_result::mismatch;
            }
            params = make_lit_params( frame );
            params.size = { view.width, view.height, view.rows_per_level, view.reach_column };
            switch( run_prefilter( tex, prefiltered, params, frame.layout.size() ) ) {
                case prefilter_outcome::ok:
                    break;
                case prefilter_outcome::failed:
                    destroy();
                    return probe_group_result::mismatch;
                case prefilter_outcome::unsafe:
                    graveyard();
                    return probe_group_result::unsafe;
            }
        } else {
            params = make_lit_params( frame );
            params.size = { view.width, view.height, view.rows_per_level, view.reach_column };
        }
        SDL_GPUTexture *const gpu_texture = gpu_texture_of( filtered ? prefiltered : tex );
        SDL_GPUTextureSamplerBinding binding{};
        binding.texture = gpu_texture;
        binding.sampler = filtered &&
                          lookup == smooth_lighting::lookup::hardware ? lit_linear_sampler_ : lit_sampler_;
        render_state state = gpu_texture ? render_state::create( renderer_,
                             c.night ? nv_lit_shader_ : lit_shader_, &binding, 1 ) : render_state{};
        if( !state.is_valid() ||
            !SDL_SetGPURenderStateFragmentUniforms( state.get(), 0, &params, sizeof( params ) ) ) {
            DebugLog( D_ERROR, DC_ALL ) << "cata_shader::variant_pass: lit probe state failed for "
                                        << c.name << ": " << SDL_GetError();
            state = render_state{};
            destroy();
            return probe_group_result::mismatch;
        }
        // one quad over the whole target, its corners carrying the case's
        // vertex colors
        const std::array<SDL_FPoint, 4> at = { {
                { 0.0f, 0.0f }, { static_cast<float>( c.size.x ), 0.0f },
                { static_cast<float>( c.size.x ), static_cast<float>( c.size.y ) },
                { 0.0f, static_cast<float>( c.size.y ) }
            }
        };
        const std::array<SDL_FPoint, 4> uv = { { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } } };
        std::array<SDL_Vertex, 4> quad;
        for( size_t i = 0; i < quad.size(); ++i ) {
            const smooth_lighting::lit_coords &k = c.corners[i];
            quad[i] = { at[i], { k.x, k.y, k.column, k.row }, uv[i] };
        }
        static constexpr std::array<int, 6> indices = { 0, 1, 2, 0, 2, 3 };
        SDL_Renderer *const renderer = renderer_;
        const target_readback rb = draw_into_probe_target( renderer_, state.get(), c.size,
        c.source, std::nullopt, [renderer, &quad]( SDL_Texture * src ) {
            return SDL_RenderGeometry( renderer, src, quad.data(), static_cast<int>( quad.size() ),
                                       indices.data(), static_cast<int>( indices.size() ) );
        } );
        if( !rb.boundary_safe ) {
            state.abandon();
            graveyard();
            return probe_group_result::unsafe;
        }
        state = render_state{};
        destroy();
        std::vector<lit_probe::rgb> got;
        got.reserve( rb.pixels.size() );
        for( const std::array<Uint8, 4> &p : rb.pixels ) {
            got.push_back( { p[0], p[1], p[2] } );
        }
        const std::vector<lit_probe::rgb> want = lit_probe::expected( c );
        if( !rb.read || !lit_probe::matches( want, got ) ) {
            DebugLog( filtered ? D_WARNING : D_ERROR,
                      DC_ALL ) << "cata_shader::variant_pass: lit shader probe failed: "
                               << c.name << ( filtered ? lookup == smooth_lighting::lookup::hardware ? " (hardware lookup)" :
                                              " (manual lookup)" : "" );
            for( size_t i = 0; i < want.size() && i < got.size(); ++i ) {
                DebugLog( filtered ? D_WARNING : D_ERROR, DC_ALL ) << "  pixel " << i << " want " << want[i].r <<
                        "," << want[i].g << "," << want[i].b << " got " << got[i].r << "," << got[i].g << "," << got[i].b;
            }
            return probe_group_result::mismatch;
        }
    }
    return probe_group_result::passed;
}

smooth_lighting::lit_capability variant_pass::probe_lit()
{
    using smooth_lighting::probe_group_result;
    using smooth_lighting::lookup;
    const probe_group_result per_tile = probe_lit_group( false, lookup::hardware );
    probe_group_result hardware = probe_group_result::skipped;
    probe_group_result manual = probe_group_result::skipped;
    const bool storable = lit_device_ && prefilter_state_.is_valid() &&
                          SDL_GPUTextureSupportsFormat( lit_device_, SDL_GPU_TEXTUREFORMAT_R16G16B16A16_FLOAT,
                                  SDL_GPU_TEXTURETYPE_2D, SDL_GPU_TEXTUREUSAGE_SAMPLER | SDL_GPU_TEXTUREUSAGE_COLOR_TARGET );
    if( per_tile == probe_group_result::passed && storable ) {
        const char *forced = std::getenv( "CATA_PREFILTER_LOOKUP" );
        const bool force_manual = forced && std::string( forced ) == "manual";
        if( !force_manual && lit_linear_sampler_ ) {
            hardware = probe_lit_group( true, lookup::hardware );
        }
        if( force_manual || !lit_linear_sampler_ || hardware == probe_group_result::mismatch ) {
            manual = probe_lit_group( true, lookup::manual );
        }
    }
    const smooth_lighting::lit_capability capability = smooth_lighting::decide_lit_capability(
                per_tile, hardware, manual );
    if( !capability.unsafe ) {
        DebugLog( D_INFO, DC_ALL ) << "cata_shader::variant_pass: smooth lighting shaders probed: per tile "
                                   << ( capability.per_tile ? "ok" : "failed" ) << ", filtered " << ( !capability.filtered ?
                                           storable ? "failed" : "unavailable" : *capability.filtered == lookup::hardware ?
                                           "hardware lookup" : "manual lookup" );
    }
    return capability;
}

void variant_pass::end_lit()
{
    lit_active_ = false;
    lit_suspended_ = false;
}

bool variant_pass::drop_lit_states()
{
    if( !lit_state_.is_valid() && !nv_lit_state_.is_valid() ) {
        return true;
    }
    const bool flushed = flush();
    if( !flushed ) {
        // renderer might still hold them
        lit_state_.abandon();
        nv_lit_state_.abandon();
    }
    lit_state_ = {};
    nv_lit_state_ = {};
    lit_texture_ = nullptr;
    lit_bound_sampler_ = nullptr;
    lit_active_ = false;
    return flushed;
}

bool variant_pass::drop_lit()
{
    if( !lit_state_.is_valid() && !lit_sampler_ && !prefilter_state_.is_valid() ) {
        return true;
    }
    const bool flushed = flush();
    release_lit( !flushed );
    return flushed;
}

void variant_pass::release_gpu_resources()
{
    if( abandoned_pending_rebind_ ) {
        // Embargo already raised; nothing to do until rebind.
        return;
    }
    // flush() returns false on an undefined bind; abandon the handles in that
    // case rather than let their destructors touch a still-referenced resource.
    bool flushed = true;
    if( bound_state_ || unbind_required_ || boundary_lost_ ) {
        flushed = flush();
    }
    clear_state_arrays( !flushed );
    bound_state_ = nullptr;
    probe_attempted_ = false;
    probed_ok_ = false;
    // active_memory_preset_ is logical config, not a GPU handle; keep it.
    if( flushed ) {
        unbind_required_ = false;
        session_disabled_ = false;
    } else {
        // Abandoned: raise the embargo so later calls refuse SDL (see header).
        abandoned_pending_rebind_ = true;
    }
}

void variant_pass::force_abandon_gpu_resources()
{
    clear_state_arrays( true );
    bound_state_ = nullptr;
    probe_attempted_ = false;
    probed_ok_ = false;
    session_disabled_ = true;
    unbind_required_ = false;
    abandoned_pending_rebind_ = true;
}

void variant_pass::rebind_renderer( SDL_Renderer *renderer )
{
    // Do NOT call release_gpu_resources() here: its flush() would touch
    // renderer_, already destroyed on a LOST recovery. Callers release against
    // the OLD renderer first, so the arrays are already empty by now.
    clear_state_arrays( true );
    bound_state_ = nullptr;
    unbind_required_ = false;
    probe_attempted_ = false;
    probed_ok_ = false;
    session_disabled_ = false;
    abandoned_pending_rebind_ = false;
    boundary_lost_ = false;
    renderer_ = renderer;
}

} // namespace cata_shader

#endif // TILES
