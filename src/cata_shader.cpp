#if defined(TILES)

#include "cata_shader.h"

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

} // namespace

variant_pass::variant_pass( SDL_Renderer *renderer )
    : renderer_( renderer )
{
}

variant_pass::~variant_pass()
{
    if( active_ ) {
        ( void )SDL_SetGPURenderState( renderer_, nullptr );
    }
}

namespace
{

// Renders a single 1x1 mid-gray sprite via `state` to a 1x1 offscreen RT
// and reads the result back. Mid-gray (128,128,128,255) is chosen because
// every variant transform produces a distinctive output for it: grayscale
// stays gray-but-dimmer, nightvision/overexposed produce a green tint
// (G > R+20 and G > B+20). That distinguishes "shader ran" from "shader
// bind was silently ignored" (the latter returns the unmodulated gray).
bool variant_draw_probe( SDL_Renderer *renderer, SDL_GPURenderState *state,
                         variant_kind v )
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
                        if( a < 250 ) {
                            ok = false;
                        } else {
                            switch( v ) {
                                case variant_kind::SHADOW:
                                    // Grayscale: gray-toned (R~=G~=B within 8 LSB) AND
                                    // strictly darker than input mid-gray (128).
                                    ok = std::abs( r - g ) < 8 && std::abs( g - b ) < 8
                                         && r < 120;
                                    break;
                                case variant_kind::NIGHT:
                                case variant_kind::OVEREXPOSED:
                                    // Both variants emit green-tinted output: G clearly
                                    // greater than R and B. Margin 20 covers driver
                                    // rounding without false-positiving the unmodulated
                                    // gray (R==G==B).
                                    ok = g > r + 20 && g > b + 20;
                                    break;
                                default:
                                    // NORMAL/MEMORY are not probed (no shader path).
                                    ok = false;
                                    break;
                            }
                        }
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
        shader frag = shader::load_fragment( device, basename, 1, 0 );
        if( !frag.is_valid() ) {
            DebugLog( D_ERROR, DC_ALL )
                    << "cata_shader::variant_pass: shader load failed for "
                    << basename << "; shader variant path disabled";
            return;
        }
        render_state state = render_state::create( renderer_, frag );
        if( !state.is_valid() ) {
            DebugLog( D_ERROR, DC_ALL )
                    << "cata_shader::variant_pass: render_state::create failed for "
                    << basename << "; shader variant path disabled";
            return;
        }
        if( !variant_draw_probe( renderer_, state.get(), v ) ) {
            DebugLog( D_ERROR, DC_ALL )
                    << "cata_shader::variant_pass: textured-draw probe failed for "
                    << basename << " (silent miswire? readback did not match "
                    "expected variant transform); shader variant path disabled";
            return;
        }
        shaders_[i] = std::move( frag );
        states_[i] = std::move( state );
    }
    probed_ok_ = true;
}

bool variant_pass::try_begin( variant_kind v )
{
    if( session_disabled_ ) {
        return false;
    }
    if( !probe_attempted_ ) {
        probe();
    }
    if( !probed_ok_ ) {
        return false;
    }
    if( active_ ) {
        // try_begin called without preceding end(); programming error.
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: try_begin while bind active; "
                "session disabled to prevent state bleed";
        session_disabled_ = true;
        ( void )SDL_SetGPURenderState( renderer_, nullptr );
        active_ = false;
        return false;
    }
    SDL_GPURenderState *state = states_[static_cast<size_t>( v )].get();
    if( !state ) {
        // NORMAL/MEMORY/unhandled: no shader path for this variant.
        return false;
    }
    if( !SDL_SetGPURenderState( renderer_, state ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: SDL_SetGPURenderState failed: "
                << SDL_GetError();
        session_disabled_ = true;
        return false;
    }
    active_ = true;
    return true;
}

bool variant_pass::end()
{
    if( !active_ ) {
        return true;
    }
    active_ = false;
    if( !SDL_SetGPURenderState( renderer_, nullptr ) ) {
        DebugLog( D_ERROR, DC_ALL )
                << "cata_shader::variant_pass: SDL_SetGPURenderState(NULL) failed: "
                << SDL_GetError();
        session_disabled_ = true;
        return false;
    }
    return true;
}

} // namespace cata_shader

#endif // SDL_MAJOR_VERSION >= 3

#endif // TILES
