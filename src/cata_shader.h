#pragma once
#ifndef CATA_SRC_CATA_SHADER_H
#define CATA_SRC_CATA_SHADER_H

#if defined(TILES)

#include "sdl_wrappers.h"

// SDL_SetGPURenderState and the SDL_GPU* surface this header wraps were added
// in SDL 3.4.0 and have no SDL2 counterpart. The class types below are
// SDL3-only.
#if SDL_MAJOR_VERSION >= 3

#include <array>
#include <memory>
#include <string>

namespace cata_shader
{

// Sprite-variant kinds for the GPU shader path. NORMAL has no shader and
// uses the atlas directly. MEMORY has no shader either; the memory atlas
// drives MEMORIZED draws.
enum class variant_kind : int {
    NORMAL = 0,
    SHADOW,       // lit_level::LOW without nightvision
    NIGHT,        // nightvision active at lit_level::LOW
    OVEREXPOSED,  // nightvision active at lit_level above LOW
    MEMORY,       // lit_level::MEMORIZED
    count
};

// RAII over SDL_GPUShader *. Lifetime is tied to the SDL_GPUDevice that
// created the shader (device-scoped).
class shader
{
    public:
        // Loads the matching per-backend shader artifact (.spv / .dxil / .msl)
        // for the format reported by SDL_GetGPUShaderFormats(device) and
        // calls SDL_CreateGPUShader. Returns an empty shader on failure;
        // callers must check is_valid() before use.
        //
        // basename - shader name without extension (e.g. "sprite_variant.frag");
        //            the loader appends ".spv" / ".dxil" / ".msl" based on the
        //            chosen format.
        // num_samplers / num_uniform_buffers - SDL_GPUShaderCreateInfo fields.
        static shader load_fragment( SDL_GPUDevice *device,
                                     const std::string &basename,
                                     unsigned int num_samplers,
                                     unsigned int num_uniform_buffers );

        shader() = default;
        ~shader();

        shader( const shader & ) = delete;
        shader &operator=( const shader & ) = delete;
        shader( shader &&other ) noexcept;
        shader &operator=( shader &&other ) noexcept;

        bool is_valid() const {
            return ptr_ != nullptr && device_ != nullptr;
        }
        SDL_GPUShader *get() const {
            return ptr_;
        }

    private:
        shader( SDL_GPUDevice *device, SDL_GPUShader *ptr )
            : device_( device ), ptr_( ptr ) {}

        SDL_GPUDevice *device_ = nullptr;
        SDL_GPUShader *ptr_ = nullptr;
};

// RAII over SDL_GPURenderState *. Lifetime is tied to the SDL_Renderer that
// created the state. SDL3 destroys it via SDL_DestroyGPURenderState(state)
// (single-arg, no renderer reference; the state retains its renderer link).
class render_state
{
    public:
        // Wraps the supplied fragment shader (which must outlive the
        // render_state) into an SDL_GPURenderState bound to renderer. Returns
        // an empty render_state on failure. The create-info declares only the
        // fragment shader; the atlas sampler and any uniform data come through
        // the renderer's normal textured-draw path and SetGPURenderStateFragmentUniforms.
        static render_state create( SDL_Renderer *renderer,
                                    const shader &fragment_shader );

        render_state() = default;
        ~render_state();

        render_state( const render_state & ) = delete;
        render_state &operator=( const render_state & ) = delete;
        render_state( render_state &&other ) noexcept;
        render_state &operator=( render_state &&other ) noexcept;

        bool is_valid() const {
            return ptr_ != nullptr;
        }
        SDL_GPURenderState *get() const {
            return ptr_;
        }

    private:
        explicit render_state( SDL_GPURenderState *ptr ) : ptr_( ptr ) {}

        SDL_GPURenderState *ptr_ = nullptr;
};

// RAII bracket around SDL_SetGPURenderState. Binds the supplied state on
// construction, clears it on destruction (or earlier via unbind()). ImGui's
// SDL3 renderer backend does not save/restore custom GPU state; any unpaired
// bind leaks into the next draw, so the bracket is mandatory.
class gpu_state_scope
{
    public:
        // Binds state on the renderer. is_valid() reflects whether the bind
        // succeeded; on failure end-of-scope is a no-op.
        gpu_state_scope( SDL_Renderer *renderer, SDL_GPURenderState *state );
        ~gpu_state_scope();

        gpu_state_scope( const gpu_state_scope & ) = delete;
        gpu_state_scope &operator=( const gpu_state_scope & ) = delete;
        gpu_state_scope( gpu_state_scope && ) = delete;
        gpu_state_scope &operator=( gpu_state_scope && ) = delete;

        bool is_valid() const {
            return active_;
        }

        // Clears the bind early. Returns true on success (or no-op when
        // already inactive), false if SDL_SetGPURenderState(NULL) failed.
        // Subsequent destruction is a no-op.
        bool unbind();

    private:
        SDL_Renderer *renderer_ = nullptr;
        bool active_ = false;
};

// Owns one SDL_GPUShader + SDL_GPURenderState per supported variant and
// brackets bind/unbind around per-sprite draws. SDL_SetGPURenderStateFragmentUniforms
// is not used: per-call uploads leak host memory on the SDL3 GPU renderer
// without recycling, so each variant gets its own state and the dispatch
// is a state switch rather than a uniform mutation.
//
// Lifecycle:
//   - construct with renderer (no work).
//   - probe() lazily on first use; loads shaders, creates states for
//     SHADOW/NIGHT/OVEREXPOSED. Either every variant succeeds or every
//     variant is marked unavailable (single decision, no per-variant
//     gating). NORMAL and MEMORY are never probed: neither has a shader.
//   - try_begin(v) returns true iff v has an active shader path AND bind
//     succeeded. Caller must call end() iff try_begin returned true.
//   - end() returns true on clean unbind. False sets session_disabled
//     (one-way) so subsequent try_begin returns false; recovers on
//     restart.
//   - destruction releases held shader/state slots.
class variant_pass
{
    public:
        explicit variant_pass( SDL_Renderer *renderer );
        ~variant_pass();

        variant_pass( const variant_pass & ) = delete;
        variant_pass &operator=( const variant_pass & ) = delete;
        variant_pass( variant_pass && ) = delete;
        variant_pass &operator=( variant_pass && ) = delete;

        // True iff probe ran successfully AND session not disabled by
        // late failure. Cheap, safe in a hot path.
        bool available() const {
            return probed_ok_ && !session_disabled_;
        }

        bool try_begin( variant_kind v );
        bool end();

    private:
        void probe();

        SDL_Renderer *renderer_ = nullptr;
        std::array<shader, static_cast<size_t>( variant_kind::count )> shaders_;
        std::array<render_state, static_cast<size_t>( variant_kind::count )> states_;
        bool probe_attempted_ = false;
        bool probed_ok_ = false;
        bool session_disabled_ = false;
        bool active_ = false;
};

} // namespace cata_shader

#endif // SDL_MAJOR_VERSION >= 3

#endif // TILES

#endif // CATA_SRC_CATA_SHADER_H
