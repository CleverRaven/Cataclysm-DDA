#if defined(TILES)

#include "cata_shader.h"

namespace cata_shader
{

namespace
{
bool g_reprobe_requested = false;
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

} // namespace cata_shader

#if SDL_MAJOR_VERSION >= 3

#include <array>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <vector>

#include "debug.h"
#include "path_info.h"

namespace cata_shader
{

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
    if( !renderer || !fragment_shader.is_valid() ) {
        return render_state{};
    }

    SDL_GPURenderStateCreateInfo info{};
    info.fragment_shader = fragment_shader.get();
    // No additional sampler/storage bindings: the atlas sampler comes from the
    // renderer's normal textured-draw path, and uniforms (when present) are
    // uploaded per draw via SDL_SetGPURenderStateFragmentUniforms.
    info.num_sampler_bindings = 0;
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

gpu_state_scope::gpu_state_scope( SDL_Renderer *renderer, SDL_GPURenderState *state )
    : renderer_( renderer )
{
    if( !renderer_ || !state ) {
        return;
    }
    if( !SDL_SetGPURenderState( renderer_, state ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::gpu_state_scope: SDL_SetGPURenderState failed: "
                << SDL_GetError();
        return;
    }
    active_ = true;
}

gpu_state_scope::~gpu_state_scope()
{
    ( void )unbind();
}

bool gpu_state_scope::unbind()
{
    if( !active_ ) {
        return true;
    }
    active_ = false;
    if( !SDL_SetGPURenderState( renderer_, nullptr ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::gpu_state_scope: SDL_SetGPURenderState(NULL) failed: "
                << SDL_GetError();
        return false;
    }
    return true;
}

namespace
{

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

variant_pass::variant_pass( SDL_Renderer *renderer )
    : renderer_( renderer )
{
}

variant_pass::~variant_pass()
{
    flush();
}

namespace
{

// Renders a 1x1 mid-gray sprite via state and checks readback via pred.
// Mid-gray (128,128,128,255) gives each variant a distinctive output, so a
// passing predicate distinguishes "shader ran" from "bind silently ignored".
bool draw_probe( SDL_Renderer *renderer, SDL_GPURenderState *state,
                 probe_predicate pred )
{
    SDL_Surface *src_surf = SDL_CreateSurface( 1, 1, SDL_PIXELFORMAT_RGBA32 );
    if( !src_surf ) {
        return false;
    }
    // Mid-gray opaque: 0x80, 0x80, 0x80, 0xFF in RGBA32.
    SDL_FillSurfaceRect( src_surf, nullptr, 0xFF808080u );
    SDL_Texture *src = SDL_CreateTextureFromSurface( renderer, src_surf );
    SDL_DestroySurface( src_surf );
    if( !src ) {
        return false;
    }
    SDL_Texture *rt = SDL_CreateTexture( renderer, SDL_PIXELFORMAT_RGBA32,
                                         SDL_TEXTUREACCESS_TARGET, 1, 1 );
    if( !rt ) {
        SDL_DestroyTexture( src );
        return false;
    }
    SDL_Texture *prior_target = SDL_GetRenderTarget( renderer );
    bool ok = false;
    if( SDL_SetRenderTarget( renderer, rt ) ) {
        SDL_SetRenderDrawColor( renderer, 0, 0, 0, 255 );
        SDL_RenderClear( renderer );
        if( SDL_SetGPURenderState( renderer, state ) ) {
            const SDL_FRect dst{ 0.0f, 0.0f, 1.0f, 1.0f };
            if( SDL_RenderTexture( renderer, src, nullptr, &dst ) ) {
                SDL_SetGPURenderState( renderer, nullptr );
                const SDL_Rect rect{ 0, 0, 1, 1 };
                SDL_Surface *out = SDL_RenderReadPixels( renderer, &rect );
                if( out ) {
                    SDL_Surface *rgba = out->format == SDL_PIXELFORMAT_RGBA32
                                        ? out
                                        : SDL_ConvertSurface( out, SDL_PIXELFORMAT_RGBA32 );
                    if( rgba != out ) {
                        SDL_DestroySurface( out );
                    }
                    if( rgba ) {
                        const Uint8 *p = static_cast<const Uint8 *>( rgba->pixels );
                        const int r = p[0];
                        const int g = p[1];
                        const int b = p[2];
                        const int a = p[3];
                        ok = a >= 250 && pred && pred( r, g, b );
                        SDL_DestroySurface( rgba );
                    }
                }
            } else {
                SDL_SetGPURenderState( renderer, nullptr );
            }
        }
    }
    SDL_SetRenderTarget( renderer, prior_target );
    SDL_DestroyTexture( rt );
    SDL_DestroyTexture( src );
    return ok;
}

} // namespace

namespace
{

// Loads shader + render_state for basename and validates via draw_probe.
bool load_and_probe( SDL_GPUDevice *device, SDL_Renderer *renderer,
                     const char *basename, probe_predicate pred,
                     shader &out_shader, render_state &out_state )
{
    shader frag = shader::load_fragment( device, basename, 1, 0 );
    if( !frag.is_valid() ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: shader load failed for "
                << basename << "; shader variant path disabled";
        return false;
    }
    render_state state = render_state::create( renderer, frag );
    if( !state.is_valid() ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: render_state::create failed for "
                << basename << "; shader variant path disabled";
        return false;
    }
    if( !draw_probe( renderer, state.get(), pred ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: textured-draw probe failed for "
                << basename << " (silent miswire? readback did not match "
                "expected variant transform); shader variant path disabled";
        return false;
    }
    out_shader = std::move( frag );
    out_state = std::move( state );
    return true;
}

} // namespace

void variant_pass::reset()
{
    flush();
    for( size_t i = 0; i < shaders_.size(); ++i ) {
        states_[i] = render_state{};
        shaders_[i] = shader{};
    }
    for( size_t i = 0; i < memory_shaders_.size(); ++i ) {
        memory_states_[i] = render_state{};
        memory_shaders_[i] = shader{};
    }
    probe_attempted_ = false;
    probed_ok_ = false;
    session_disabled_ = false;
}

void variant_pass::probe()
{
    probe_attempted_ = true;
    if( !renderer_ ) {
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
        if( !load_and_probe( device, renderer_, basename, predicate_for( v ),
                             shaders_[i], states_[i] ) ) {
            return;
        }
    }
    for( int i = 0; i < static_cast<int>( memory_preset::count ); ++i ) {
        const memory_preset p = static_cast<memory_preset>( i );
        const char *basename = shader_basename_for( p );
        if( !basename ) {
            continue;
        }
        if( !load_and_probe( device, renderer_, basename, predicate_for( p ),
                             memory_shaders_[i], memory_states_[i] ) ) {
            return;
        }
    }
    probed_ok_ = true;
}

SDL_GPURenderState *variant_pass::state_for( variant_kind v ) const
{
    if( v == variant_kind::MEMORY ) {
        if( !active_memory_preset_ ) {
            return nullptr;
        }
        return memory_states_[static_cast<size_t>( *active_memory_preset_ )].get();
    }
    return states_[static_cast<size_t>( v )].get();
}

bool variant_pass::try_begin( variant_kind v )
{
    if( reprobe_requested() ) {
        reset();
        clear_reprobe();
    }
    if( session_disabled_ ) {
        // Drop any held bind so the disabled session does not leave shader
        // state on subsequent draws.
        flush();
        return false;
    }
    if( !probe_attempted_ ) {
        probe();
    }
    if( !probed_ok_ ) {
        return false;
    }
    SDL_GPURenderState *target = state_for( v );
    SDL_GPURenderState *current = currently_bound_
                                  ? state_for( *currently_bound_ )
                                  : nullptr;
    if( target == current ) {
        return target != nullptr;
    }
    if( !SDL_SetGPURenderState( renderer_, target ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: SDL_SetGPURenderState failed: "
                << SDL_GetError();
        session_disabled_ = true;
        currently_bound_.reset();
        return false;
    }
    if( target ) {
        currently_bound_ = v;
    } else {
        currently_bound_.reset();
    }
    return target != nullptr;
}

bool variant_pass::end()
{
    return true;
}

bool variant_pass::flush()
{
    if( !currently_bound_ ) {
        return true;
    }
    if( !SDL_SetGPURenderState( renderer_, nullptr ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: SDL_SetGPURenderState(NULL) failed: "
                << SDL_GetError();
        session_disabled_ = true;
        return false;
    }
    currently_bound_.reset();
    return true;
}

void variant_pass::select_memory_preset( std::optional<memory_preset> preset )
{
    active_memory_preset_ = preset;
}

render_target_scope::render_target_scope( SDL_Renderer *renderer, SDL_Texture *target,
        variant_pass *vp )
    : renderer_( renderer )
{
    if( !renderer_ ) {
        return;
    }
    prior_target_ = SDL_GetRenderTarget( renderer_ );
    if( vp && !vp->flush() ) {
        return;
    }
    if( !SDL_SetRenderTarget( renderer_, target ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::render_target_scope: SDL_SetRenderTarget failed: "
                << SDL_GetError();
        return;
    }
    valid_ = true;
}

bool render_target_scope::restore()
{
    if( restored_ ) {
        return true;
    }
    if( !renderer_ || !valid_ ) {
        return false;
    }
    if( !SDL_SetRenderTarget( renderer_, prior_target_ ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::render_target_scope::restore: SDL_SetRenderTarget failed: "
                << SDL_GetError();
        return false;
    }
    restored_ = true;
    return true;
}

render_target_scope::~render_target_scope()
{
    if( restored_ || !valid_ ) {
        return;
    }
    // Fallback for early exit. Mark restored regardless to avoid
    // retry loops; double failure means the invariant is lost.
    restored_ = true;
    if( !SDL_SetRenderTarget( renderer_, prior_target_ ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::~render_target_scope: SDL_SetRenderTarget failed: "
                << SDL_GetError();
    }
}

} // namespace cata_shader

#endif // SDL_MAJOR_VERSION >= 3

#endif // TILES
