#pragma once
#ifndef CATA_SRC_SDL_RENDERER_RECOVERY_H
#define CATA_SRC_SDL_RENDERER_RECOVERY_H

#if defined(TILES)

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>

#include "cata_tiles.h"

// Severity of a renderer-resource recovery, ordered so the inbox can keep
// the maximum pending level via a monotonic-max. Android foreground maps to
// device_reset through the lifecycle epoch rather than a distinct severity.
enum class renderer_recovery_severity : int {
    none = 0,
    targets_reset = 1,
    device_reset = 2,
    device_lost = 3,
};

enum class renderer_recovery_state {
    bootstrapping,
    ready,
    recovering,
    retry_needed,
};

// Packed into the lifecycle epoch alongside a sequence number so a
// background-foreground-background ordering cannot be lost between the two
// separate stores a watcher makes.
enum class lifecycle_state : uint64_t {
    active = 0,
    paused = 1,
    resumed_pending_rebuild = 2,
};

// The two generations sampled together at tileset fetch time so the cache
// can reject a bundle uploaded against a since-invalidated renderer or
// device-texture epoch.
struct renderer_texture_generations {
    uint64_t instance;
    uint64_t textures;
};

// Outcome of a recovery recipe. restart_required means the recipe bailed
// before any destructive teardown (e.g. an unsafe unbind) and the drain
// should re-claim work at restart_severity without bumping generations.
enum class recipe_outcome {
    success,
    failure,
    restart_required,
};
struct recipe_result {
    recipe_outcome outcome;
    renderer_recovery_severity restart_severity = renderer_recovery_severity::none;
};

// SDL-free state machine that owns the drain transition policy: lifecycle epoch
// pack/unpack, severity coalesce, the resize-after-recovery ordering, the
// post-success seq-CAS decision, and retry persistence. The coordinator owns the
// atomic inbox, the SDL recipes, and the generations; drain_pending() is a thin
// driver that asks the planner for the next action, performs that one side
// effect (recipe, resize, inbox read, seq-CAS), and feeds the result back. The
// policy lives here so the coalesce / seq-CAS / restart / resize branches are
// unit-testable with plain values and no renderer.
class recovery_drain_planner
{
    public:
        // Lifecycle epoch packs a sequence number with the state so a
        // background-foreground-background ordering survives the two separate
        // stores a watcher makes: [63..2] seq, [1..0] state.
        static constexpr uint64_t make_lifecycle_epoch( uint64_t seq, lifecycle_state s ) {
            return ( seq << 2 ) | static_cast<uint64_t>( s );
        }
        static constexpr uint64_t lifecycle_seq_of( uint64_t epoch ) {
            return epoch >> 2;
        }
        static constexpr lifecycle_state lifecycle_state_of( uint64_t epoch ) {
            return static_cast<lifecycle_state>( epoch & 0x3 );
        }
        static constexpr renderer_recovery_severity severity_max(
            renderer_recovery_severity a, renderer_recovery_severity b ) {
            return static_cast<int>( a ) >= static_cast<int>( b ) ? a : b;
        }

        // The next side effect drain_pending must perform. For the read /
        // exchange / recipe / resize kinds the driver performs it and feeds the
        // result back through the matching advance_*; finish_ready and
        // finish_retry are terminal.
        enum class action_kind {
            load_entry_epoch,     // load lifecycle epoch -> advance_entry_epoch
            claim_initial_inbox,  // exchange severity, load resize, read latch -> advance_initial_inbox
            run_recipe,           // run_recipe_for(action.severity) -> advance_recipe
            attempt_seq_cas,      // clear the serviced foreground epoch -> advance_seq_cas_done
            claim_after_success,  // exchange severity, load lifecycle -> advance_after_success
            claim_after_restart,  // load lifecycle, exchange severity -> advance_after_restart
            load_post_resize,     // load resize epoch -> advance_post_resize
            apply_resize,         // apply_resize_only(action.resize_epoch) -> advance_resize
            finish_ready,
            finish_retry,
        };
        struct action {
            action_kind kind;
            renderer_recovery_severity severity = renderer_recovery_severity::none;
            uint32_t resize_epoch = 0;
            uint64_t serviced_seq = 0;
        };

        // Pure state the coordinator predicates and recipes consult.
        renderer_recovery_state state() const {
            return state_;
        }
        bool abort_latch() const {
            return abort_latch_;
        }
        uint32_t observed_resize_epoch() const {
            return observed_resize_epoch_;
        }
        uint64_t serviced_lifecycle_seq() const {
            return serviced_lifecycle_seq_;
        }
        renderer_recovery_severity current_claimed() const {
            return current_claimed_;
        }

        // Bootstrapping -> ready, once the base UI is up.
        void finish_bootstrap() {
            state_ = renderer_recovery_state::ready;
        }
        // Back to the initial bootstrapping state for a fresh test fixture.
        void reset() {
            *this = recovery_drain_planner{};
        }

        // Driver protocol. begin() opens a drain (recovering + abort latch) and
        // returns the first action. Each advance_* consumes the result of the
        // side effect the previous action named and returns the next action.
        action begin();
        action advance_entry_epoch( uint64_t entry_epoch );
        action advance_initial_inbox( renderer_recovery_severity pulled, uint32_t resize_epoch,
                                      bool boundary_recovery_pending );
        action advance_recipe( recipe_result r );
        action advance_seq_cas_done();
        action advance_after_success( renderer_recovery_severity pulled, uint64_t live_epoch );
        action advance_after_restart( uint64_t live_epoch, renderer_recovery_severity pulled );
        action advance_post_resize( uint32_t resize_epoch );
        action advance_resize( bool applied );

        // Called from inside an SDL recipe (or apply_resize_only) when a pause is
        // observed: persists the in-flight severity as the retry and arms the
        // mid-recipe abort so the next advance_recipe finishes as retry_needed.
        void note_pause_abort();
        // Called from inside apply_resize_only at the point the serviced resize
        // epoch is committed, before the post-success blank, so the blank
        // predicate sees no phantom pending resize.
        void acknowledge_resize( uint32_t serviced_resize_epoch ) {
            observed_resize_epoch_ = serviced_resize_epoch;
        }

    private:
        // Merge a freshly exchanged inbox severity with any persisted retry, then
        // clear the retry slot. The exchange itself is the coordinator's atomic
        // side effect; only the merge half is pure policy.
        renderer_recovery_severity merge_retry( renderer_recovery_severity pulled );
        action emit_run_recipe( renderer_recovery_severity sev );
        action emit_apply_resize( uint32_t resize_epoch, bool fast_path );
        action finish_ready_action();
        action finish_retry_action();

        renderer_recovery_state state_ = renderer_recovery_state::bootstrapping;
        renderer_recovery_severity retry_severity_ = renderer_recovery_severity::none;
        // Severity of the recipe currently in flight, so a mid-recipe pause abort
        // can persist it as the retry severity.
        renderer_recovery_severity current_claimed_ = renderer_recovery_severity::none;
        bool abort_latch_ = false;
        uint32_t observed_resize_epoch_ = 0;
        // Lifecycle sequence the current drain is servicing. A resumed epoch with
        // this sequence is the foreground rebuild in progress and does not
        // self-interrupt; a different sequence is a newer foreground.
        uint64_t serviced_lifecycle_seq_ = 0;
        // epoch_implied severity derived from the entry lifecycle state, carried
        // from advance_entry_epoch to advance_initial_inbox.
        renderer_recovery_severity entry_epoch_implied_ = renderer_recovery_severity::none;
        // restart_severity from the recipe result, carried from advance_recipe to
        // advance_after_restart.
        renderer_recovery_severity restart_severity_pending_ = renderer_recovery_severity::none;
        // Set by note_pause_abort so the following advance_recipe finishes the
        // drain as retry_needed regardless of the recipe's own outcome.
        bool recipe_pause_aborted_ = false;
        // Whether the apply_resize in flight is the entry fast path (clears retry
        // on failure) rather than the post-recovery step (leaves it).
        bool resize_fast_path_ = false;
};

// Test-only seam, befriended below; full declaration follows the coordinator.
struct renderer_recovery_test_support;

// Main-thread executor that recovers every renderer-owned GPU resource
// across SDL render-target reset, device reset, device loss, and android
// background/foreground. The event watch writes only the atomic inbox;
// drain_pending() runs the ordered recovery recipes at the named outer
// boundaries.
class renderer_resource_coordinator
{
        friend struct renderer_recovery_test_support;
    public:
        // Inbox writers. Safe to call from the SDL event-watch thread: they
        // touch only the atomic inbox.
        void request_recovery( renderer_recovery_severity sev );
        void notify_resize();
        void notify_lifecycle( lifecycle_state ev );

        // Seed the renderer creation policy from the live renderer (backend
        // name plus actual software/accelerated mode). Stays in
        // bootstrapping; rendering is not yet allowed.
        void seed_renderer_policy( const std::string &renderer_name );
        // Move bootstrapping -> ready once the base renderer, display buffer,
        // shader pass, ImGui, and fonts are up. Rendering and recovery are
        // allowed from here, before any world-load atlas upload.
        void finish_bootstrap();

        // Enter/leave a nestable atlas-upload scope; see atlas_upload_scope below.
        // end releases depth only -- drain explicitly after the outermost scope,
        // since recovery can throw and must not run from a destructor.
        void begin_atlas_upload();
        void end_atlas_upload();

        // Main-thread surface.
        void drain_pending();
        bool is_render_allowed() const;
        bool should_abort_frame() const;

        renderer_recovery_state state() const {
            return planner_.state();
        }
        renderer_recovery_severity pending() const {
            return pending_severity_.load();
        }
        uint64_t resource_generation() const {
            return renderer_resource_generation_;
        }
        uint64_t instance_generation() const {
            return renderer_instance_generation_;
        }
        uint64_t textures_generation() const {
            return gpu_textures_generation_;
        }
        // Sampled together by the tileset fetch path for the cache staleness
        // check.
        renderer_texture_generations texture_generations() const {
            return { renderer_instance_generation_, gpu_textures_generation_ };
        }

    private:
        // should_abort_frame without the recovery-owned abort latch: true when
        // the external inbox (lifecycle, severity, resize, boundary latch)
        // forbids rendering.
        bool external_inbox_blocks_render() const;
        // Like external_inbox_blocks_render but permits the foreground epoch
        // this drain is servicing (resumed with serviced_lifecycle_seq_), so
        // the recovery-owned post-success blank may run for it; a pause, a
        // newer resumed sequence, or other inbox work still blocks.
        bool recovery_blank_blocked() const;
        // Clear the foreground epoch the drain serviced so the next coalesce
        // iteration re-derives epoch_implied from the current lifecycle state.
        void attempt_serviced_seq_cas( uint64_t serviced_seq );
        recipe_result run_recipe_for( renderer_recovery_severity sev );
        // Debug-only invariant check run after each recipe success: usable
        // renderer + display buffer, boundary latch cleared, generations not
        // rolled backward from the pre-dispatch snapshot.
        void assert_recipe_postcondition( uint64_t resource_gen_before,
                                          renderer_texture_generations gens_before ) const;
        // serviced_resize_epoch is the pending_resize_epoch value sampled by
        // the drain; it is acknowledged into observed_resize_epoch_ on success
        // so the post-success blank predicate sees no phantom pending resize.
        bool apply_resize_only( uint32_t serviced_resize_epoch );
        void run_post_success_full_invalidation();
        bool install_renderer_with_fallback();
        bool safe_pre_teardown_unbind( recipe_result &out );
        bool check_pause_abort();
        // Drop every live cached tileset's atlas textures against the still-
        // live renderer, before any renderer destruction. Idempotent.
        void release_live_atlases() const;
        // Re-upload atlases over every live tileset, polling for pause/loss
        // between chunks and routing interrupted candidates into the replay
        // quarantine. Returns the interrupt reason, or none on full success.
        atlas_upload_interrupt replay_live_atlases();
        // Map an atlas-replay interrupt to a recipe outcome: paused persists
        // retry and yields retry_needed; the *_invalidated reasons restart at
        // the matching severity.
        recipe_result map_replay_interrupt( atlas_upload_interrupt reason );
        // Poll consulted during atlas replay: reports pause or a higher
        // pending severity so the upload stops and quarantines.
        atlas_upload_interrupt replay_poll();
        // Rebuild display_buffer, recording its dims on success. A failure
        // with the sticky boundary latch raised escalates to device-loss in
        // the same drain rather than a plain retry.
        recipe_result setup_target_phase();
        recipe_result recipe_targets_reset();
        recipe_result recipe_device_reset();
        recipe_result recipe_device_lost();
        void record_display_buffer_dims();

        // Pure drain transition policy. drain_pending() drives it; the
        // coordinator supplies the atomic inbox reads and the SDL side effects.
        recovery_drain_planner planner_;
        // Nesting depth of active atlas-upload scopes; see begin_atlas_upload.
        // Distinct from bootstrapping, which only covers pre-base-UI startup.
        int atlas_upload_depth_ = 0;
        uint64_t renderer_resource_generation_ = 0;
        uint64_t renderer_instance_generation_ = 0;
        uint64_t gpu_textures_generation_ = 0;
        // Display-buffer dims the coordinator last built, so a resize can
        // skip the rebuild when only DPI or letterboxing changed.
        int display_buffer_w_ = 0;
        int display_buffer_h_ = 0;
        // Renderer creation policy, retained so a device-loss rebuild can
        // recreate with the same backend choice and track the actual
        // installed backend after any accelerated-to-software fallback.
        std::string renderer_name_;
        bool software_renderer_ = false;

        std::atomic<renderer_recovery_severity> pending_severity_{ renderer_recovery_severity::none };
        std::atomic<uint32_t> pending_resize_epoch_{ 0 };
        std::atomic<uint64_t> lifecycle_epoch_{ 0 };

        // Candidate atlas textures captured when a replay is interrupted by a
        // pause or loss; drained on the live renderer before retry or
        // abandoned before a renderer is destroyed.
        atlas_replay_quarantine replay_quarantine_;

        // Test-only fault-injection hooks, armed via renderer_recovery_test_support
        // and inert by default. Drive the retry, coalesce, replay-pause, and
        // seq-CAS branches without threads.
        enum class test_phase_action {
            none,
            fail_retry,
            queue_device_reset,
        };
        test_phase_action test_phase_action_ = test_phase_action::none;
        int test_phase_countdown_ = 0;
        bool test_phase_fault_fired_ = false;
        int test_replay_pause_countdown_ = 0;
};

// Process-lifetime coordinator; the event-watch userdata must outlive
// every callback.
extern renderer_resource_coordinator renderer_coordinator;

// RAII wrapper over begin_atlas_upload/end_atlas_upload, the canonical gate for
// atlas uploads: while any scope is open, a drain cannot rebuild the renderer
// under partially uploaded candidate textures. Enters on construction, releases
// on destruction so a throwing upload unwinds. Does not drain (recovery can
// throw, the dtor is noexcept); the caller drains after the outermost scope.
// active=false skips gating, e.g. a precheck load that does no GPU upload.
class atlas_upload_scope
{
    public:
        explicit atlas_upload_scope( bool active = true )
            : active_( active ) {
            if( active_ ) {
                renderer_coordinator.begin_atlas_upload();
            }
        }
        ~atlas_upload_scope() {
            if( active_ ) {
                renderer_coordinator.end_atlas_upload();
            }
        }
        atlas_upload_scope( const atlas_upload_scope & ) = delete;
        atlas_upload_scope &operator=( const atlas_upload_scope & ) = delete;
    private:
        bool active_;
};

// Test-only seam for the renderer-recovery suite. Methods are defined beside the
// file-static render globals in sdltiles.cpp; the struct is friended by the
// coordinator, tileset cache, and tileset so a Catch2 suite can stand up a
// headless renderer and synthetic bundle without those members going public.
struct renderer_recovery_test_support {
    // Stand up a hidden-window software renderer, display_buffer, variant_pass,
    // and geometry on the file-static globals, then seed and bootstrap the
    // coordinator. False (globals clean) if the dummy backend fails or a window is up.
    static bool setup_software_renderer();
    // Reverse setup: drain the quarantine and release atlases on the live
    // renderer, destroy variant_pass before the renderer, reset globals and the
    // coordinator, and quit video only if this fixture acquired it.
    static void teardown_software_renderer();

    // Return the process-lifetime coordinator to its initial bootstrapping
    // state, draining any quarantine on the still-live renderer first.
    static void reset_coordinator();

    // Build a one-descriptor 1x1 bundle, upload it once at the given generations,
    // and insert it into the global cache. Returns the held bundle so the weak
    // cache entry stays live and the recorded generations are readable.
    static std::shared_ptr<const tileset> install_synthetic_bundle(
        const std::string &tileset_id, const std::string &memory_map_mode,
        uint64_t renderer_instance_generation, uint64_t gpu_textures_generation );

    // Fetch a bundle through the production cache lookup at the given current
    // generations; returns the cached bundle on a fresh hit. Used only for the
    // matching-generation hit path, which does not trigger a reload.
    static std::shared_ptr<const tileset> fetch_cached_bundle(
        const std::string &tileset_id, const std::string &memory_map_mode,
        uint64_t current_renderer_instance_gen, uint64_t current_gpu_textures_gen );

    // Whether the production fresh-cache predicate accepts the bundle at
    // (tileset_id, memory_map_mode) for the given generations: false on a
    // generation mismatch or a fingerprint miss, without a JSON reload.
    static bool cache_lookup_is_fresh(
        const std::string &tileset_id, const std::string &memory_map_mode,
        uint64_t current_renderer_instance_gen, uint64_t current_gpu_textures_gen );

    // Arm the coordinator's deterministic fault hooks (inert until armed): the
    // phase faults fire at the Nth check_pause_abort gate, the replay pause at
    // the Nth replay poll.
    static void arm_phase_fail_retry( int phase_countdown );
    static void arm_phase_queue_device_reset( int phase_countdown );
    static void arm_replay_pause( int poll_countdown );
    static bool replay_quarantine_empty();
    // Whether the most recently armed phase fault has fired since arming.
    static bool phase_fault_fired();

    // Set the scaling factor and resize the hidden fixture window, then notify
    // the coordinator. The deferred resize applies on the next drain.
    static void set_scaling_and_resize_window( int scaling, int window_w, int window_h );
    // Current display_buffer texture dimensions, for asserting a resize rebuild.
    static void current_display_buffer_dims( int &w, int &h );
    // The logical window size, font/scaling metrics, and minimum terminal size
    // the resize applied, so a test can derive the expected terminal-sized
    // buffer from the actual (possibly clamped) window.
    static void current_window_metrics( int &window_w, int &window_h, int &font_w,
                                        int &font_h, int &scaling, int &min_term_w,
                                        int &min_term_h );
    // Current backing-store (drawable) pixel dimensions.
    static void current_drawable_dims( int &w, int &h );
    // Inject divergent drawable pixels and notify a resize, standing in for a
    // DPI change the headless backend cannot produce. Cleared at teardown.
    static void override_drawable_pixels( int w, int h );
    // Set and read the present-needed flag, for asserting the resize fast path
    // re-arms presentation.
    static void set_needupdate( bool armed );
    static bool needupdate_armed();
    // Open an outermost draw scope, inject a paused lifecycle epoch while it is
    // on the stack, then close it. Reports whether the display buffer was bound
    // inside the scope and whether the target returned to NULL after it.
    static void pause_during_draw_scope( bool &bound_during, bool &null_after );
};

// RAII wrapper around setup/teardown for use as a Catch2 fixture local.
class software_render_fixture
{
    public:
        software_render_fixture()
            : available_( renderer_recovery_test_support::setup_software_renderer() ) {}
        ~software_render_fixture() {
            if( available_ ) {
                renderer_recovery_test_support::teardown_software_renderer();
            }
        }
        software_render_fixture( const software_render_fixture & ) = delete;
        software_render_fixture &operator=( const software_render_fixture & ) = delete;
        bool available() const {
            return available_;
        }
    private:
        bool available_ = false;
};

#endif // TILES

#endif // CATA_SRC_SDL_RENDERER_RECOVERY_H
