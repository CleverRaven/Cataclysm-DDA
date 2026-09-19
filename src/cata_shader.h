#pragma once
#ifndef CATA_SRC_CATA_SHADER_H
#define CATA_SRC_CATA_SHADER_H

#if defined(TILES)

#include <array>
#include <memory>
#include <optional>
#include <string>

#include "sdl_wrappers.h"

struct renderer_recovery_test_support;

namespace cata_shader
{

// Forces the next variant_pass::try_begin to drop cached shader artifacts
// and re-run the activation probe. Used to pick up freshly rebuilt
// .spv/.dxil/.msl without restarting. No-op on a non-GPU renderer.
void request_reprobe();
bool reprobe_requested();
void clear_reprobe();

// How the variant pass can serve an atlas upload; see
// variant_pass::ensure_probed
enum class probe_state {
    available,
    unavailable,
    // renderer boundary is lost, see boundary_lost()
    unsafe,
};

// test seams driven by renderer_recovery_test_support only, inert until armed.
// armed faults replace the named SDL outcome and log D_INFO, because the real
// failure's D_ERROR would fail the test run
void test_arm_probe_unsafe( int count );
int test_probe_unsafe_remaining();
void test_arm_flush_failure();
int test_probe_runs();
void test_reset_seams();

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
// created the state. SDL destroys it via SDL_DestroyGPURenderState(state)
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
// is not used: per-call uploads leak host memory on the GPU renderer
// without recycling, so each variant gets its own state and the dispatch
// is a state switch rather than a uniform mutation.
//
// try_begin holds the bound state across runs of sprites that select the same
// state, only calling SDL_SetGPURenderState when the state changes. flush() at
// the end of the frame clears the held state.
//
// Lifecycle:
//   - construct with renderer (no work).
//   - probe() lazily on first use; loads shaders and creates states for
//     SHADOW/NIGHT/OVEREXPOSED, one per named memory_preset, and tint.frag for
//     tinted NORMAL sprites. either all variants succeed or all are marked
//     unavailable (single decision, no per-variant gating). untinted NORMAL has
//     no shader
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
        // probe loads tint.frag with every other variant, so the tint path is
        // available exactly when the pass is
        bool tint_available() const {
            return available();
        }

        // classify pass for upload decision, run activation probe if not done
        // yet. check in order: embargo, lost boundary, pending reprobe, sticky
        // fault, then the probe.
        probe_state ensure_probed();
        // renderer may still have a probe target or shader bind that this pass
        // couldn't release; flush() refuses while set; cleared only by
        // rebind_renderer
        bool boundary_lost() const {
            return boundary_lost_;
        }
        // Sticky: unsafe probe, failed flush, or draw-time bind failure happened.
        // Survives rebind_renderer; cleared only by successful explicit reset
        // (request_reprobe).
        bool shader_fault() const {
            return shader_fault_;
        }
        std::optional<memory_preset> active_memory_preset() const {
            return active_memory_preset_;
        }
        // raise the flags for failed draw-time SDL_SetGPURenderState, log_error
        // false skips the D_ERROR line for the test seam
        void note_draw_bind_failure( bool log_error = true );

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

        // tinted selects tint.frag for NORMAL; every other variant shader reads
        // the tint from the vertex color
        begin_result try_begin( variant_kind v, bool tinted = false );
        bool end();

        // returns false on lost boundary, under embargo, or on
        // SDL_SetGPURenderState(NULL) failure; callers then refuse to cross a
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
        friend struct ::renderer_recovery_test_support;

        void probe();
        void reset();
        void mark_probe_unsafe();
        void mark_flush_failed();
        // Drop shader + render-state slots. abandon_handles=true skips SDL
        // destroy on each (renderer undefined or about to die); false runs the
        // destructors normally to release the SDL handles.
        void clear_state_arrays( bool abandon_handles );
        SDL_GPURenderState *state_for( variant_kind v, bool tinted ) const;

        SDL_Renderer *renderer_ = nullptr;
        std::array<shader, static_cast<size_t>( variant_kind::count )> shaders_;
        std::array<render_state, static_cast<size_t>( variant_kind::count )> states_;
        std::array<shader, static_cast<size_t>( memory_preset::count )> memory_shaders_;
        std::array<render_state, static_cast<size_t>( memory_preset::count )>
        memory_states_;
        shader tint_shader_;
        render_state tint_state_;
        std::optional<memory_preset> active_memory_preset_;
        SDL_GPURenderState *bound_state_ = nullptr;
        // Set after an unsafe bind transition (failed SDL_SetGPURenderState or
        // a probe boundary loss): next flush() must call null-state regardless
        // of bound_state_ to clear whatever the renderer holds.
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
        bool boundary_lost_ = false;
        bool shader_fault_ = false;
};

} // namespace cata_shader

#endif // TILES

#endif // CATA_SRC_CATA_SHADER_H
