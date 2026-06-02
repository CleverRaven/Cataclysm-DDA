#pragma once
#ifndef CATA_SRC_CATA_SHADER_H
#define CATA_SRC_CATA_SHADER_H

#if defined(TILES)

#include "sdl_wrappers.h"

namespace cata_shader
{

// Forces the next variant_pass::try_begin to drop cached shader artifacts
// and re-run the activation probe. Used to pick up freshly rebuilt
// .spv/.dxil/.msl without restarting. No-op on SDL2 / non-GPU renderer.
void request_reprobe();
bool reprobe_requested();
void clear_reprobe();

} // namespace cata_shader

// SDL_SetGPURenderState and the SDL_GPU* surface this header wraps were added
// in SDL 3.4.0 and have no SDL2 counterpart. The class types below are
// SDL3-only.
#if SDL_MAJOR_VERSION >= 3

#include <array>
#include <memory>
#include <optional>
#include <string>

namespace cata_shader
{

// Sprite-variant kinds for the GPU shader path. NORMAL has no shader and
// uses the atlas directly. MEMORY dispatches by the selected memory_preset
// (see select_memory_preset); the custom MEMORY_MAP_MODE preset has no shader
// and falls back to the memory atlas.
enum class variant_kind : int {
    NORMAL = 0,
    SHADOW,       // lit_level::LOW without nightvision
    NIGHT,        // nightvision active at lit_level::LOW
    OVEREXPOSED,  // nightvision active at lit_level above LOW
    MEMORY,       // lit_level::MEMORIZED
    count
};

// Memory map overlay presets that have a baked shader. Mirrors the four
// named MEMORY_MAP_MODE values; the custom preset has no shader here and
// falls back to the memory atlas.
enum class memory_preset : int {
    DARKEN = 0,
    SEPIA_LIGHT,
    SEPIA_DARK,
    BLUE_DARK,
    count
};

// Maps a MEMORY_MAP_MODE option value to its memory_preset. Returns nullopt
// for color_pixel_custom or any unknown value.
std::optional<memory_preset> memory_preset_from_option_value(
    const std::string &mode );

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

        // Renounce ownership without SDL_ReleaseGPUShader, for when the
        // renderer still references this through a stale bind we could not
        // detach -- destroying it would invalidate live GPU pipeline state.
        void abandon() {
            ptr_ = nullptr;
            device_ = nullptr;
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

        // Renounce ownership without SDL_DestroyGPURenderState. Used when a
        // probe leaves the state bound on a renderer we cannot safely detach.
        void abandon() {
            ptr_ = nullptr;
        }

    private:
        explicit render_state( SDL_GPURenderState *ptr ) : ptr_( ptr ) {}

        SDL_GPURenderState *ptr_ = nullptr;
};

// Owns one SDL_GPUShader + SDL_GPURenderState per supported variant and
// brackets bind/unbind around per-sprite draws. SDL_SetGPURenderStateFragmentUniforms
// is not used: per-call uploads leak host memory on the SDL3 GPU renderer
// without recycling, so each variant gets its own state and the dispatch
// is a state switch rather than a uniform mutation.
//
// try_begin holds the bound state across same-variant runs of sprites and
// only calls SDL_SetGPURenderState when the variant changes. flush() at the
// end of the frame clears the held state.
//
// Lifecycle:
//   - construct with renderer (no work).
//   - probe() lazily on first use; loads shaders and creates states for
//     SHADOW/NIGHT/OVEREXPOSED plus one per named memory_preset. Either
//     every variant succeeds or every variant is marked unavailable
//     (single decision, no per-variant gating). NORMAL has no shader.
//   - select_memory_preset(p) picks which memory shader try_begin(MEMORY)
//     binds. Nullopt disables the MEMORY shader path so callers fall back
//     to the memory atlas (used for the custom MEMORY_MAP_MODE preset).
//   - try_begin(v) binds the shader for v and returns a begin_result (see
//     the enum doc). Atlas-fallback paths clear any prior bind first.
//   - end() is a no-op; bind persists for the next sprite.
//   - flush() unbinds any held state. Call once per frame after the last
//     sprite draw so ImGui or the next-frame draws see no leaked bind.
//   - destruction unbinds and releases held shader/state slots.
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

        // try_begin outcome. bound: shader path active, draw with it.
        // use_atlas: safe fallback (NORMAL, unsupported MEMORY preset, clean
        // session_disabled) -- renderer valid, fall through to the pre-baked
        // atlas. abort_frame: a failed SDL_SetGPURenderState left the renderer
        // undefined -- caller MUST stop rendering; the frame aborts and the
        // coordinator rebuilds.
        enum class begin_result {
            bound,
            use_atlas,
            abort_frame,
        };

        begin_result try_begin( variant_kind v );
        bool end();

        // Returns false on SDL_SetGPURenderState(NULL) failure; pass
        // stays flagged bound so callers can refuse to cross a
        // render-target boundary.
        bool flush();

        void select_memory_preset( std::optional<memory_preset> preset );

        // Drop all GPU resources, flushing held state first; idempotent. On
        // flush failure the handles are abandoned and the embargo raised (see
        // abandoned_pending_rebind_). Run before the owning renderer dies.
        void release_gpu_resources();

        // Abandon every handle without calling SDL: intentional leak for when
        // even flush() is unsafe (dangling renderer), reclaimed at process
        // exit. Raises the embargo. Prefer release_gpu_resources() otherwise.
        void force_abandon_gpu_resources();

        // Adopt a freshly created renderer after LOST/DEVICE_RESET, reset local
        // boundary state, and clear the embargo so the next try_begin re-probes.
        // Never skips on pointer equality: DEVICE_RESET keeps the pointer.
        void rebind_renderer( SDL_Renderer *renderer );

    private:
        void probe();
        void reset();
        // Drop shader + render-state slots. abandon_handles=true skips SDL
        // destroy on each (renderer undefined or about to die); false runs the
        // destructors normally to release the SDL handles.
        void clear_state_arrays( bool abandon_handles );
        SDL_GPURenderState *state_for( variant_kind v ) const;

        SDL_Renderer *renderer_ = nullptr;
        std::array<shader, static_cast<size_t>( variant_kind::count )> shaders_;
        std::array<render_state, static_cast<size_t>( variant_kind::count )> states_;
        std::array<shader, static_cast<size_t>( memory_preset::count )> memory_shaders_;
        std::array<render_state, static_cast<size_t>( memory_preset::count )>
        memory_states_;
        std::optional<memory_preset> active_memory_preset_;
        std::optional<variant_kind> currently_bound_;
        // Set after an unsafe bind transition (failed SDL_SetGPURenderState or
        // a probe boundary loss): next flush() must call null-state regardless
        // of currently_bound_ to clear whatever the renderer holds.
        bool unbind_required_ = false;
        // The "embargo": while true, every SDL-touching method (flush,
        // try_begin, release_gpu_resources, dtor) refuses without calling SDL,
        // the renderer being presumed dangling/unsafe. Raised by
        // force_abandon_gpu_resources() and by release_gpu_resources() on flush
        // failure; cleared by rebind_renderer().
        bool abandoned_pending_rebind_ = false;
        bool probe_attempted_ = false;
        bool probed_ok_ = false;
        bool session_disabled_ = false;
};

} // namespace cata_shader

#endif // SDL_MAJOR_VERSION >= 3

#endif // TILES

#endif // CATA_SRC_CATA_SHADER_H
