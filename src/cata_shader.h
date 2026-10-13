#pragma once
#ifndef CATA_SRC_CATA_SHADER_H
#define CATA_SRC_CATA_SHADER_H

#if defined(TILES)

#include <array>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <vector>

#include "point.h"
#include "sdl_wrappers.h"
#include "smooth_lighting.h"

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
// (see select_memory_preset); the custom MEMORY_MAP_MODE preset has no classic
// shader and falls back to the memory atlas, though the lit shaders blend into
// it through memory_look.
enum class variant_kind : int {
    NORMAL = 0,
    SHADOW,       // lit_level::LOW without nightvision
    NIGHT,        // nightvision active at lit_level::LOW
    OVEREXPOSED,  // nightvision active at lit_level above LOW
    MEMORY,       // lit_level::MEMORIZED
    count
};

// Whether a variant keeps the sprite's own colors, and so reads as lit by the
// tile's light. Night vision and the memory overlay rewrite the sprite into a
// palette of their own, which the colored-light tint has nothing to say about.
bool variant_takes_tint( variant_kind v );

// Memory map overlay presets that have a baked shader. Mirrors the four
// named MEMORY_MAP_MODE values; the custom preset has no classic shader and
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
        // as above, with `bindings` bound after the draw's own texture, from
        // fragment sampler slot 1 on. SDL copies them at creation.
        static render_state create( SDL_Renderer *renderer, const shader &fragment_shader,
                                    const SDL_GPUTextureSamplerBinding *bindings, int num_bindings );

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

// memory overlay look for the lit shaders: a named preset, else the custom
// MEMORY_MAP_MODE mixer
struct memory_look {
    std::optional<memory_preset> preset;
    // custom dark and light colors in 0 to 1, and gamma
    std::array<float, 3> custom_dark = {};
    std::array<float, 3> custom_light = {};
    float custom_gamma = 1.0f;
};

// lit_params id of the custom look, after the named presets
constexpr int32_t custom_memory_look = static_cast<int32_t>( memory_preset::count );
static_assert( custom_memory_look == smooth_lighting::custom_look,
               "lit shaders and their reference agree on the custom look id" );

struct lit_frame {
    SDL_Texture *lightmap = nullptr;
    // what sprites read when not per tile: the light lit_prefilter.frag
    // stored at `layout`'s grid points
    SDL_Texture *prefiltered = nullptr;
    smooth_lighting::prefilter_layout layout;
    smooth_lighting::lookup lookup = smooth_lighting::lookup::hardware;
    memory_look memory;
    // fade out-of-sight light into the memory look rather than darkness
    bool blend_memory = false;
    bool per_tile = false;
    bool iso = false;
    bool night_vision = false;
};

// what begin_lit did; an unsafe boundary is not a fallback
enum class lit_begin_outcome : uint8_t {
    // lit states bound for this frame
    active,
    // no lit path here: draw classic
    classic,
    // making lit states failed: draw classic, and `failure` says why
    failed,
    // renderer may hold a bind this pass could not release: abort the frame
    abort_frame,
};

struct lit_begin_result {
    lit_begin_outcome outcome = lit_begin_outcome::classic;
    std::optional<smooth_lighting::lit_failure> failure;
};

smooth_lighting::lit_frame_action action_for( lit_begin_outcome o );

// what prepare_lit did; `capability` holds only when the outcome is active
struct lit_prepare_result {
    lit_begin_outcome outcome = lit_begin_outcome::classic;
    std::optional<smooth_lighting::lit_failure> failure;
    smooth_lighting::lit_capability capability;
};

// what a prefilter pass did; failed leaves the renderer safe and the
// target's texels undefined
enum class prefilter_outcome : uint8_t {
    ok,
    failed,
    unsafe,
};

// a float render target for the prefiltered light; its colorspace is sRGB
// so SDL passes the pass's coordinate vertex colors through unconverted
SDL_Texture *create_prefilter_target( SDL_Renderer *renderer, const point &size );

// The cases the activation probe draws through the lit shaders before they
// are used, and the readbacks their C++ references expect.
namespace lit_probe
{
struct rgb {
    int r = 0;
    int g = 0;
    int b = 0;
};

// per channel; the same allowance as shader_tint_self_test
constexpr int channel_tolerance = 4;

struct probe_case {
    std::string name;
    // `levels` stacked z levels of light texels left of `columns`, reach
    // masks right of it
    std::vector<smooth_lighting::lightmap_texel> texels;
    int columns = 0;
    int rows_per_level = 0;
    int levels = 3;
    // probe sprite's single texel, RGBA; drawn blended over black
    std::array<uint8_t, 4> source = { 128, 128, 128, 255 };
    lit_frame frame;
    bool night = false;
    // target size and the quad's vertex colors at its corners: top left, top
    // right, bottom right, bottom left
    point size;
    std::array<smooth_lighting::lit_coords, 4> corners;
};

smooth_lighting::lightmap_view view_of( const probe_case &c );
// prefilter layout of a filtered case: its own cell and the cells round it,
// on its own level
smooth_lighting::prefilter_layout layout_of( const probe_case &c );
std::vector<probe_case> cases();
// per target pixel, row by row
std::vector<rgb> expected( const probe_case &c );
bool matches( const std::vector<rgb> &expected, const std::vector<rgb> &readback );
// what a shader that ignores the light map would show: the source as drawn
std::vector<rgb> readback_if_ignored( const probe_case &c );
// what a shader that shows everything in sight and fully lit would show
std::vector<rgb> readback_if_full_light( const probe_case &c );
} // namespace lit_probe

// Owns one SDL_GPUShader + SDL_GPURenderState per supported variant and
// brackets bind/unbind around per-sprite draws. classic variants each get their
// own state, so dispatch is a state switch, not a uniform change. only lit
// states carry fragment uniforms, set when their parameters change.
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
        // changes whenever GPU resources are dropped or rebuilt, and differs
        // between passes
        uint32_t resource_generation() const {
            return resource_generation_;
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

        // tinted selects tint.frag for NORMAL. SHADOW reads the tint from the
        // vertex color in its own shader, and the rest take no tint at all
        begin_result try_begin( variant_kind v, bool tinted = false );
        bool end();

        // returns false on lost boundary, under embargo, or on
        // SDL_SetGPURenderState(NULL) failure; callers then refuse to cross a
        // render-target boundary.
        bool flush();

        void select_memory_preset( std::optional<memory_preset> preset );

        // load lit shaders and run activation probe once per resources: per
        // tile cases, filtered ones with each lookup
        lit_prepare_result prepare_lit();
        // smooth lighting. while active, NORMAL and SHADOW draws run lit.frag and
        // NIGHT and OVEREXPOSED run nightvision_lit.frag, shading each pixel at
        // the light map texel coordinates in the vertex colors (see
        // lit_sample.glsl): from `frame.lightmap` with `per_tile`, else from
        // `frame.prefiltered`. with `frame.blend_memory`, MEMORY draws run
        // lit.frag too and every lit draw fades out-of-sight light into
        // `frame.memory`. `iso` picks the base line standing sprites take their
        // light from; `night_vision` makes MEMORY draws blend toward the night
        // vision look. the result says whether to draw lit, draw classic, or
        // abort the frame.
        lit_begin_result begin_lit( const lit_frame &frame );
        // fill `frame.prefiltered` from `frame.lightmap` over `frame.layout`
        prefilter_outcome prefilter_lit( const lit_frame &frame );
        // back to classic variants, lit states stay for the next frame
        void end_lit();
        // drop lit states before a texture they read is destroyed: the states
        // hold it, and destroying them flushes queued draws. false when the
        // flush failed: the renderer may still read the texture
        bool drop_lit();
        // drop only the lit states that read the bound texture; shaders,
        // samplers and probed capability stay. false as drop_lit
        bool drop_lit_states();
        bool lit_active() const {
            return lit_active_;
        }
        // whether a draw of `v` runs a lit state now, and so wants map
        // coordinates in its vertex colors
        bool lit_takes( variant_kind v ) const;
        // while set, draws use the classic variants even though lighting is
        // active; for sprites that show whatever the light at their tile
        void set_lit_suspended( bool suspended ) {
            lit_suspended_ = suspended;
        }

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
        void release_lit( bool abandon_handles );

        SDL_Renderer *renderer_ = nullptr;
        std::array<shader, static_cast<size_t>( variant_kind::count )> shaders_;
        std::array<render_state, static_cast<size_t>( variant_kind::count )> states_;
        std::array<shader, static_cast<size_t>( memory_preset::count )> memory_shaders_;
        std::array<render_state, static_cast<size_t>( memory_preset::count )>
        memory_states_;
        shader tint_shader_;
        render_state tint_state_;
        std::optional<memory_preset> active_memory_preset_;
        shader lit_shader_;
        shader nv_lit_shader_;
        render_state lit_state_;
        render_state nv_lit_state_;
        // reads light map as the draw's own texture, no extra binding
        shader prefilter_shader_;
        render_state prefilter_state_;
        SDL_GPUDevice *lit_device_ = nullptr;
        SDL_GPUSampler *lit_sampler_ = nullptr;
        SDL_GPUSampler *lit_linear_sampler_ = nullptr;
        // what lit_state_ and nv_lit_state_ read
        SDL_Texture *lit_texture_ = nullptr;
        SDL_GPUSampler *lit_bound_sampler_ = nullptr;
        // lit_params block of lit_common.glsl, std140: nine vec4
        struct lit_params {
            // light map width, height, rows per z level, reach mask column
            std::array<int32_t, 4> size = {};
            // memory look, per tile, blend out of sight into memory, iso
            std::array<int32_t, 4> mode = { -1, 0, 0, 0 };
            // detail flag bit, barrier flag bit, custom memory look id, unused
            std::array<int32_t, 4> flags = {};
            // shadow shade, standing marker, full color light, night floor
            std::array<float, 4> tone = {};
            // custom memory look: dark rgb and gamma; light rgb
            std::array<float, 4> custom_dark = {};
            std::array<float, 4> custom_light = {};
            // overexpose start, tint mix, unused, unused
            std::array<float, 4> look = {};
            // prefilter layout: first cell x and y, cells per level, first level
            std::array<int32_t, 4> prefilter = {};
            // grid steps per cell side, the layout's width in cells and its
            // level count, 1 for the manual lookup
            std::array<int32_t, 4> prefilter_grid = {};
            bool operator==( const lit_params &o ) const {
                return size == o.size && mode == o.mode && flags == o.flags && tone == o.tone &&
                       custom_dark == o.custom_dark && custom_light == o.custom_light && look == o.look &&
                       prefilter == o.prefilter && prefilter_grid == o.prefilter_grid;
            }
        };
        static_assert( sizeof( lit_params ) == 9 * 16, "lit_params is a std140 block of nine vec4" );
        static_assert( std::is_trivially_copyable_v<lit_params> );
        // lit_params for `frame`
        static lit_params make_lit_params( const lit_frame &frame );
        // compares readback of every `filtered` lit_probe case with its
        // reference
        smooth_lighting::probe_group_result probe_lit_group( bool filtered,
                smooth_lighting::lookup lookup );
        smooth_lighting::lit_capability probe_lit();
        // draw prefilter of `lightmap` into `target`, `size` pixels
        prefilter_outcome run_prefilter( SDL_Texture *lightmap, SDL_Texture *target,
                                         const lit_params &params, const point &size );
        lit_params lit_params_;
        bool lit_active_ = false;
        bool lit_suspended_ = false;
        // memory blends toward the night vision look, not the lit look
        bool lit_night_vision_ = false;
        // what the lit shaders' activation probe found for these resources
        std::optional<smooth_lighting::lit_capability> lit_capability_;
        uint32_t resource_generation_ = 0;
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
