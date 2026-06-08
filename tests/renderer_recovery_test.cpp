#if defined(TILES)

#include <algorithm>
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <string>

#include "cata_catch.h"
#include "cata_imgui.h"
#include "cata_tiles.h"
#include "options_helpers.h"
#include "point.h"
#include "sdl_renderer_recovery.h"

namespace
{
using ak = recovery_drain_planner::action_kind;
using sev = renderer_recovery_severity;

uint64_t epoch( uint64_t seq, lifecycle_state s )
{
    return recovery_drain_planner::make_lifecycle_epoch( seq, s );
}
} // namespace

// Pure drain-planner coverage: feed the SDL-free state machine synthetic inbox
// snapshots and recipe results, assert the next action it asks for. Exercises
// coalesce, seq-CAS, restart, resize-after-recovery, and retry, no renderer.

TEST_CASE( "drain_planner_epoch_pack_unpack_roundtrips", "[tiles][renderer_recovery]" )
{
    using P = recovery_drain_planner;
    const uint64_t e = P::make_lifecycle_epoch( 12345, lifecycle_state::resumed_pending_rebuild );
    CHECK( P::lifecycle_seq_of( e ) == 12345 );
    CHECK( P::lifecycle_state_of( e ) == lifecycle_state::resumed_pending_rebuild );
    // severity_max keeps the higher level regardless of argument order.
    CHECK( P::severity_max( sev::targets_reset, sev::device_lost ) == sev::device_lost );
    CHECK( P::severity_max( sev::device_lost, sev::targets_reset ) == sev::device_lost );
    CHECK( P::severity_max( sev::device_reset, sev::none ) == sev::device_reset );
}

TEST_CASE( "drain_planner_paused_entry_finishes_without_claiming", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    REQUIRE( p.begin().kind == ak::load_entry_epoch );
    // A paused entry epoch returns ready without ever reaching the inbox claim,
    // so the coordinator's queued severity is left for the next foreground.
    const recovery_drain_planner::action a =
        p.advance_entry_epoch( epoch( 3, lifecycle_state::paused ) );
    CHECK( a.kind == ak::finish_ready );
    CHECK( p.state() == renderer_recovery_state::ready );
    CHECK_FALSE( p.abort_latch() );
}

TEST_CASE( "drain_planner_foreground_epoch_alone_claims_device_reset",
           "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    REQUIRE( p.begin().kind == ak::load_entry_epoch );
    // A resumed foreground with no inbox severity still implies a device reset.
    recovery_drain_planner::action a =
        p.advance_entry_epoch( epoch( 1, lifecycle_state::resumed_pending_rebuild ) );
    REQUIRE( a.kind == ak::claim_initial_inbox );
    a = p.advance_initial_inbox( sev::none, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    CHECK( a.serviced_seq == 1 );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );
    // The seq-CAS cleared the serviced epoch to active; nothing else pending.
    a = p.advance_after_success( sev::none, epoch( 1, lifecycle_state::active ) );
    REQUIRE( a.kind == ak::load_post_resize );
    CHECK( p.advance_post_resize( 0 ).kind == ak::finish_ready );
    CHECK( p.state() == renderer_recovery_state::ready );
}

TEST_CASE( "drain_planner_coalesces_severity_queued_during_recipe",
           "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::targets_reset, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::targets_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );
    // A device reset became pending during the targets-reset recipe; the
    // coordinator exchanges it now and the planner reclaims it.
    a = p.advance_after_success( sev::device_reset, epoch( 0, lifecycle_state::active ) );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );
    a = p.advance_after_success( sev::none, epoch( 0, lifecycle_state::active ) );
    REQUIRE( a.kind == ak::load_post_resize );
    CHECK( p.advance_post_resize( 0 ).kind == ak::finish_ready );
}

TEST_CASE( "drain_planner_seq_cas_preserves_newer_foreground", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a =
        p.advance_entry_epoch( epoch( 5, lifecycle_state::resumed_pending_rebuild ) );
    a = p.advance_initial_inbox( sev::none, 0, false );
    REQUIRE( a.severity == sev::device_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    CHECK( a.serviced_seq == 5 );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );
    // A newer foreground (seq 6) raced in, so the CAS did not clear it and the
    // live epoch the coordinator loads is still resumed at the newer sequence.
    a = p.advance_after_success( sev::none, epoch( 6, lifecycle_state::resumed_pending_rebuild ) );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );
    // The planner adopts the newer sequence as the one it now services.
    CHECK( p.serviced_lifecycle_seq() == 6 );
}

TEST_CASE( "drain_planner_restart_required_reruns_escalated", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::device_reset, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    a = p.advance_recipe( { recipe_outcome::restart_required, sev::device_lost } );
    REQUIRE( a.kind == ak::claim_after_restart );

    SECTION( "an active live epoch reruns at the escalated severity" ) {
        a = p.advance_after_restart( epoch( 0, lifecycle_state::active ), sev::none );
        REQUIRE( a.kind == ak::run_recipe );
        CHECK( a.severity == sev::device_lost );
        CHECK( p.serviced_lifecycle_seq() == 0 );
    }

    SECTION( "a newer foreground at restart is adopted as the serviced sequence" ) {
        a = p.advance_after_restart( epoch( 7, lifecycle_state::resumed_pending_rebuild ), sev::none );
        REQUIRE( a.kind == ak::run_recipe );
        CHECK( a.severity == sev::device_lost );
        CHECK( p.serviced_lifecycle_seq() == 7 );
    }
}

TEST_CASE( "drain_planner_failure_persists_retry_for_next_drain", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::targets_reset, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    a = p.advance_recipe( { recipe_outcome::failure } );
    CHECK( a.kind == ak::finish_retry );
    CHECK( p.state() == renderer_recovery_state::retry_needed );

    // A fresh drain with an empty inbox reclaims the persisted severity.
    p.begin();
    a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::none, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::targets_reset );
}

TEST_CASE( "drain_planner_resize_fast_path", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();

    SECTION( "an applied entry resize finishes ready" ) {
        p.begin();
        recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
        a = p.advance_initial_inbox( sev::none, 1, false );
        REQUIRE( a.kind == ak::apply_resize );
        CHECK( a.resize_epoch == 1 );
        a = p.advance_resize( true );
        CHECK( a.kind == ak::finish_ready );
        CHECK( p.state() == renderer_recovery_state::ready );
    }

    SECTION( "a failed entry resize needs retry and persists no recipe severity" ) {
        p.begin();
        recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
        a = p.advance_initial_inbox( sev::none, 1, false );
        REQUIRE( a.kind == ak::apply_resize );
        a = p.advance_resize( false );
        CHECK( a.kind == ak::finish_retry );
        CHECK( p.state() == renderer_recovery_state::retry_needed );

        // The next drain claims nothing: a resize-only failure leaves no recipe
        // severity behind.
        p.begin();
        a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
        a = p.advance_initial_inbox( sev::none, 0, false );
        CHECK( a.kind == ak::load_post_resize );
    }
}

TEST_CASE( "drain_planner_defers_resize_until_after_recovery", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    // Both a device reset and a resize are pending: the recipe runs first.
    a = p.advance_initial_inbox( sev::device_reset, 1, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );

    a = p.advance_recipe( { recipe_outcome::success } );
    REQUIRE( a.kind == ak::attempt_seq_cas );
    REQUIRE( p.advance_seq_cas_done().kind == ak::claim_after_success );
    a = p.advance_after_success( sev::none, epoch( 0, lifecycle_state::active ) );
    REQUIRE( a.kind == ak::load_post_resize );
    // Only now, after recovery, is the deferred resize applied.
    a = p.advance_post_resize( 1 );
    REQUIRE( a.kind == ak::apply_resize );
    CHECK( a.resize_epoch == 1 );
    CHECK( p.advance_resize( true ).kind == ak::finish_ready );
}

TEST_CASE( "drain_planner_pause_during_recipe_aborts_to_retry", "[tiles][renderer_recovery]" )
{
    recovery_drain_planner p;
    p.finish_bootstrap();
    p.begin();
    recovery_drain_planner::action a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::device_reset, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    // The coordinator's check_pause_abort observed a pause mid-recipe.
    p.note_pause_abort();
    // Even though the recipe reports success, the drain finishes as retry.
    a = p.advance_recipe( { recipe_outcome::success } );
    CHECK( a.kind == ak::finish_retry );
    CHECK( p.state() == renderer_recovery_state::retry_needed );

    // The interrupted device reset persists as the retry severity.
    p.begin();
    a = p.advance_entry_epoch( epoch( 0, lifecycle_state::active ) );
    a = p.advance_initial_inbox( sev::none, 0, false );
    REQUIRE( a.kind == ak::run_recipe );
    CHECK( a.severity == sev::device_reset );
}

// Inbox/state-machine coverage with no live renderer: each case drives a local
// coordinator so the atomic inbox, render embargo, paused-drain preservation,
// and lifecycle-epoch ordering run through the public surface, no SDL.

TEST_CASE( "renderer_coordinator_inbox_keeps_max_severity", "[tiles][renderer_recovery]" )
{
    renderer_resource_coordinator coord;
    coord.finish_bootstrap();
    REQUIRE( coord.pending() == renderer_recovery_severity::none );

    coord.request_recovery( renderer_recovery_severity::targets_reset );
    CHECK( coord.pending() == renderer_recovery_severity::targets_reset );

    coord.request_recovery( renderer_recovery_severity::device_lost );
    CHECK( coord.pending() == renderer_recovery_severity::device_lost );

    // A lower severity arriving afterwards must not lower the pending level.
    coord.request_recovery( renderer_recovery_severity::device_reset );
    CHECK( coord.pending() == renderer_recovery_severity::device_lost );
}

TEST_CASE( "renderer_coordinator_embargoes_render_on_inbox_change", "[tiles][renderer_recovery]" )
{
    renderer_resource_coordinator coord;
    coord.finish_bootstrap();
    REQUIRE( coord.is_render_allowed() );
    REQUIRE_FALSE( coord.should_abort_frame() );

    SECTION( "queued severity embargoes rendering" ) {
        coord.request_recovery( renderer_recovery_severity::targets_reset );
        CHECK_FALSE( coord.is_render_allowed() );
        CHECK( coord.should_abort_frame() );
    }

    SECTION( "pending resize embargoes rendering" ) {
        coord.notify_resize();
        CHECK_FALSE( coord.is_render_allowed() );
        CHECK( coord.should_abort_frame() );
    }

    SECTION( "paused lifecycle embargoes rendering" ) {
        coord.notify_lifecycle( lifecycle_state::paused );
        CHECK_FALSE( coord.is_render_allowed() );
        CHECK( coord.should_abort_frame() );
    }
}

TEST_CASE( "renderer_coordinator_paused_drain_preserves_severity", "[tiles][renderer_recovery]" )
{
    renderer_resource_coordinator coord;
    coord.finish_bootstrap();

    coord.request_recovery( renderer_recovery_severity::device_reset );
    coord.notify_lifecycle( lifecycle_state::paused );

    // A drain while paused performs no rendering and leaves the queued severity
    // for the next foreground; the embargo must hold.
    coord.drain_pending();
    CHECK( coord.pending() == renderer_recovery_severity::device_reset );
    CHECK_FALSE( coord.is_render_allowed() );
    CHECK( coord.should_abort_frame() );
}

TEST_CASE( "renderer_coordinator_lifecycle_survives_background_ordering",
           "[tiles][renderer_recovery]" )
{
    renderer_resource_coordinator coord;
    coord.finish_bootstrap();
    REQUIRE( coord.is_render_allowed() );

    // A background, foreground, then background in quick succession must settle
    // on suspended: the packed epoch records the latest state, so the trailing
    // background is not lost behind the intervening foreground.
    coord.notify_lifecycle( lifecycle_state::paused );
    coord.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    coord.notify_lifecycle( lifecycle_state::paused );

    CHECK_FALSE( coord.is_render_allowed() );

    // A drain in this state stays suspended: no rendering, embargo retained.
    coord.drain_pending();
    CHECK_FALSE( coord.is_render_allowed() );
    CHECK( coord.should_abort_frame() );
}

// End-to-end coverage driving the process-lifetime renderer_coordinator through
// its real recipes against a headless software renderer. Each case stands up the
// fixture and skips when the dummy SDL video backend is unavailable.

TEST_CASE( "renderer_coordinator_targets_reset_bumps_only_resource_generation",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t inst0 = renderer_coordinator.instance_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    renderer_coordinator.request_recovery( renderer_recovery_severity::targets_reset );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.resource_generation() == res0 + 1 );
    CHECK( renderer_coordinator.instance_generation() == inst0 );
    CHECK( renderer_coordinator.textures_generation() == tex0 );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_device_reset_bumps_texture_generation",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t inst0 = renderer_coordinator.instance_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.resource_generation() == res0 + 1 );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
    // The renderer object survives a device reset, so its instance generation
    // is unchanged.
    CHECK( renderer_coordinator.instance_generation() == inst0 );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_device_lost_bumps_all_generations",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t inst0 = renderer_coordinator.instance_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    std::shared_ptr<const tileset> bundle =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_lost_ts", "color_pixel_sepia_light", inst0, tex0 );
    REQUIRE( bundle );

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_lost );
    renderer_coordinator.drain_pending();

    // A lost device recreates the renderer, so all three generations advance.
    CHECK( renderer_coordinator.resource_generation() == res0 + 1 );
    CHECK( renderer_coordinator.instance_generation() == inst0 + 1 );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
    // Replay re-uploaded the live bundle against the recreated renderer, so it
    // carries both the new renderer-instance and texture generations.
    CHECK( bundle->get_renderer_instance_generation_at_upload()
           == renderer_coordinator.instance_generation() );
    CHECK( bundle->get_gpu_textures_generation_at_upload()
           == renderer_coordinator.textures_generation() );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_foreground_epoch_alone_forces_rebuild",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst0 = renderer_coordinator.instance_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    // No request_recovery: the resumed lifecycle epoch alone implies a device
    // reset, so the drain still rebuilds and clears the epoch back to active.
    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
    CHECK( renderer_coordinator.instance_generation() == inst0 );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_device_reset_replays_cached_bundle",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();

    std::shared_ptr<const tileset> bundle =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_replay_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( bundle );
    REQUIRE( bundle->get_gpu_textures_generation_at_upload() == tex );

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.textures_generation() == tex + 1 );
    // Replay re-uploaded the live bundle and stamped it with the new texture
    // generation, so a later fetch no longer treats it as stale.
    CHECK( bundle->get_gpu_textures_generation_at_upload()
           == renderer_coordinator.textures_generation() );
}

TEST_CASE( "renderer_coordinator_cache_returns_bundle_on_fresh_generations",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();

    std::shared_ptr<const tileset> installed =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_hit_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( installed );

    // Matching generations are a fresh hit: the same cached bundle comes back
    // without a reload.
    std::shared_ptr<const tileset> fetched =
        renderer_recovery_test_support::fetch_cached_bundle(
            "synthetic_hit_ts", "color_pixel_sepia_light", inst, tex );
    CHECK( fetched == installed );
}

TEST_CASE( "renderer_coordinator_cache_freshness_predicate", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    const std::string id = "synthetic_fresh_ts";
    const std::string mode = "color_pixel_sepia_light";

    std::shared_ptr<const tileset> bundle;
    {
        // Install under a fixed scaling filter so the recorded fingerprint
        // matches a same-filter lookup.
        override_option scaling( "SCALING_MODE", "nearest" );
        bundle = renderer_recovery_test_support::install_synthetic_bundle( id, mode, inst, tex );
        REQUIRE( bundle );

        // Matching generations and fingerprint are a fresh hit.
        CHECK( renderer_recovery_test_support::cache_lookup_is_fresh( id, mode, inst, tex ) );
        // A stale texture generation is rejected without a reload.
        CHECK_FALSE( renderer_recovery_test_support::cache_lookup_is_fresh( id, mode, inst, tex + 1 ) );
        // A stale renderer-instance generation is rejected without a reload.
        CHECK_FALSE( renderer_recovery_test_support::cache_lookup_is_fresh( id, mode, inst + 1, tex ) );
    }
    // A different scaling filter changes the fingerprint, so the key misses.
    {
        override_option scaling( "SCALING_MODE", "linear" );
        CHECK_FALSE( renderer_recovery_test_support::cache_lookup_is_fresh( id, mode, inst, tex ) );
    }
}

// Deterministic fault-injection coverage. Hooks armed through the test support
// fire at real coordinator branch points (a phase gate, a replay poll, the
// pre-CAS window) so retry, coalesce, quarantine, and seq-CAS run without threads.

TEST_CASE( "renderer_coordinator_second_reset_mid_drain_coalesces", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    renderer_coordinator.request_recovery( renderer_recovery_severity::targets_reset );
    // Queue a device reset partway through the targets-reset recipe; the drain
    // loop reclaims it after that recipe succeeds.
    renderer_recovery_test_support::arm_phase_queue_device_reset( 2 );
    renderer_coordinator.drain_pending();

    // targets_reset then the coalesced device_reset: resource generation bumps
    // twice, textures once (only the device reset invalidates textures).
    CHECK( renderer_coordinator.resource_generation() == res0 + 2 );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
    CHECK( renderer_coordinator.is_render_allowed() );
}

// The seq-CAS-preserves-newer-foreground branch is covered by
// drain_planner_seq_cas_preserves_newer_foreground above, which feeds the newer
// epoch directly into the planner.

TEST_CASE( "renderer_coordinator_resize_failure_persists_retry", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    renderer_coordinator.notify_resize();
    REQUIRE_FALSE( renderer_coordinator.is_render_allowed() );

    // Fail the resize at its first checkpoint; the epoch stays unacked and the
    // embargo holds.
    renderer_recovery_test_support::arm_phase_fail_retry( 1 );
    renderer_coordinator.drain_pending();
    CHECK_FALSE( renderer_coordinator.is_render_allowed() );

    // The next drain retries the resize and clears the embargo.
    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_deferred_scaled_resize_rebuilds_buffer",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    // Odd window dimensions at scaling > 1: the display buffer must follow the
    // terminal-sized dims (floor(window / font / scaling) cells times font), not
    // the raw window-over-scaling pixels.
    renderer_recovery_test_support::set_scaling_and_resize_window( 2, 802, 602 );
    REQUIRE_FALSE( renderer_coordinator.is_render_allowed() );

    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );

    int win_w = 0;
    int win_h = 0;
    int font_w = 0;
    int font_h = 0;
    int scaling = 0;
    int min_term_w = 0;
    int min_term_h = 0;
    renderer_recovery_test_support::current_window_metrics( win_w, win_h, font_w, font_h, scaling,
            min_term_w, min_term_h );
    int buf_w = 0;
    int buf_h = 0;
    renderer_recovery_test_support::current_display_buffer_dims( buf_w, buf_h );
    CAPTURE( win_w, win_h, font_w, font_h, scaling, min_term_w, min_term_h, buf_w, buf_h );
    REQUIRE( scaling == 2 );
    REQUIRE( font_w > 0 );
    REQUIRE( font_h > 0 );
    // Buffer follows the terminal-sized dims: floor(window / font / scaling)
    // cells (clamped to the minimum terminal size) times the font.
    const int term_w = std::max( win_w / font_w / scaling, min_term_w );
    const int term_h = std::max( win_h / font_h / scaling, min_term_h );
    CHECK( buf_w == term_w * font_w );
    CHECK( buf_h == term_h * font_h );
    // At least one axis must exercise the genuine scaled rounding (not min-clamped)
    // so the buffer differs from the raw window-over-scaling pixels.
    const bool height_scaled = win_h / font_h / scaling > min_term_h;
    REQUIRE( height_scaled );
    CHECK( buf_h != win_h / scaling );
}

TEST_CASE( "renderer_coordinator_resize_without_buffer_change_keeps_generation",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    int buf_w0 = 0;
    int buf_h0 = 0;
    renderer_recovery_test_support::current_display_buffer_dims( buf_w0, buf_h0 );
    int draw_w0 = 0;
    int draw_h0 = 0;
    renderer_recovery_test_support::current_drawable_dims( draw_w0, draw_h0 );

    // DPI-only change: logical window untouched (buffer dims unchanged) but
    // backing pixels grow. The fast path follows the new drawable pixels and
    // re-arms presentation without recreating the buffer or bumping the generation.
    renderer_recovery_test_support::set_needupdate( false );
    renderer_recovery_test_support::override_drawable_pixels( draw_w0 * 2, draw_h0 * 2 );
    REQUIRE_FALSE( renderer_coordinator.is_render_allowed() );

    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.resource_generation() == res0 );

    // Buffer dims unchanged (no recreate on the fast path).
    int buf_w1 = 0;
    int buf_h1 = 0;
    renderer_recovery_test_support::current_display_buffer_dims( buf_w1, buf_h1 );
    CHECK( buf_w1 == buf_w0 );
    CHECK( buf_h1 == buf_h0 );

    // The drawable track followed the new backing pixels.
    int draw_w1 = 0;
    int draw_h1 = 0;
    renderer_recovery_test_support::current_drawable_dims( draw_w1, draw_h1 );
    CHECK( draw_w1 == draw_w0 * 2 );
    CHECK( draw_h1 == draw_h0 * 2 );

    // Presentation re-armed even though no renderer-owned resource changed.
    CHECK( renderer_recovery_test_support::needupdate_armed() );
}

TEST_CASE( "renderer_coordinator_buffer_changing_resize_bumps_resource_generation",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t res0 = renderer_coordinator.resource_generation();
    const uint64_t tex0 = renderer_coordinator.textures_generation();
    // A resize that changes the terminal-sized buffer recreates the display
    // buffer and bumps only the resource generation; a resize does not
    // invalidate textures, so the texture generation is unchanged.
    renderer_recovery_test_support::set_scaling_and_resize_window( 2, 802, 602 );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.resource_generation() == res0 + 1 );
    CHECK( renderer_coordinator.textures_generation() == tex0 );
}

TEST_CASE( "renderer_coordinator_background_then_foreground_rebuilds",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t tex0 = renderer_coordinator.textures_generation();
    // Backgrounded: a drain performs no rendering work and rendering stays
    // embargoed while the texture generation is untouched.
    renderer_coordinator.notify_lifecycle( lifecycle_state::paused );
    renderer_coordinator.drain_pending();
    CHECK_FALSE( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.textures_generation() == tex0 );

    // Foregrounded: the resumed epoch implies a device reset, so the next drain
    // rebuilds, bumps the texture generation, and clears the embargo.
    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
}

TEST_CASE( "imgui_frame_display_size_prefers_explicit_display_buffer_dims",
           "[tiles][renderer_recovery]" )
{
    // Idle-null contract: when the caller passes positive display-buffer dims,
    // ImGui sizes to them and never falls back to the renderer output (which
    // under the idle-null target reads the window). Zero dims fall back.
    CHECK( cataimgui::imgui_frame_display_size( 320, 240, 1920, 1080 ) == point{ 320, 240 } );
    CHECK( cataimgui::imgui_frame_display_size( 0, 0, 1920, 1080 ) == point{ 1920, 1080 } );
    // A one-axis-zero size is treated as unset and falls back, matching the
    // new_frame predicate.
    CHECK( cataimgui::imgui_frame_display_size( 320, 0, 1920, 1080 ) == point{ 1920, 1080 } );
}

TEST_CASE( "renderer_coordinator_pause_during_draw_scope_restores_target",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t tex0 = renderer_coordinator.textures_generation();

    // A pause that lands while a draw scope is on the stack must not stop the
    // scope dtor from restoring the render target to NULL (the idle-target
    // invariant), and a later foreground drain must still rebuild.
    bool bound_during = false;
    bool null_after = false;
    renderer_recovery_test_support::pause_during_draw_scope( bound_during, null_after );
    CHECK( bound_during );
    CHECK( null_after );

    // Paused: the drain performs no rendering work and the embargo holds.
    renderer_coordinator.drain_pending();
    CHECK_FALSE( renderer_coordinator.is_render_allowed() );

    // Foregrounded: the resumed epoch implies a device reset, so the next drain
    // rebuilds, bumps the texture generation, and clears the embargo.
    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.is_render_allowed() );
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
}

TEST_CASE( "renderer_coordinator_recovery_then_resize_in_one_drain",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t tex0 = renderer_coordinator.textures_generation();
    // Queue both a device reset and a buffer-changing resize, then drain once.
    // Recovery runs first; the deferred resize is consumed afterwards.
    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    renderer_recovery_test_support::set_scaling_and_resize_window( 2, 802, 602 );
    renderer_coordinator.drain_pending();

    CHECK( renderer_coordinator.is_render_allowed() );
    // The device reset ran: textures were invalidated and regenerated.
    CHECK( renderer_coordinator.textures_generation() == tex0 + 1 );
    // The resize was consumed after recovery: the display buffer ends at the
    // resized terminal-sized dims, not the pre-resize fixture size.
    int win_w = 0;
    int win_h = 0;
    int font_w = 0;
    int font_h = 0;
    int scaling = 0;
    int min_term_w = 0;
    int min_term_h = 0;
    renderer_recovery_test_support::current_window_metrics( win_w, win_h, font_w, font_h, scaling,
            min_term_w, min_term_h );
    int buf_w = 0;
    int buf_h = 0;
    renderer_recovery_test_support::current_display_buffer_dims( buf_w, buf_h );
    CHECK( buf_w == std::max( win_w / font_w / scaling, min_term_w ) * font_w );
    CHECK( buf_h == std::max( win_h / font_h / scaling, min_term_h ) * font_h );
}

TEST_CASE( "renderer_coordinator_replay_pause_quarantines_then_retries",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    std::shared_ptr<const tileset> bundle =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_pause_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( bundle );
    REQUIRE( renderer_recovery_test_support::replay_quarantine_empty() );

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    // The replay poll fires after the atlas candidate exists (cache-entry,
    // pre-descriptor, and sub-rect polls precede the post-loop poll), so the
    // candidate is quarantined rather than destroyed, and the hook pauses the
    // lifecycle as a real suspension would.
    renderer_recovery_test_support::arm_replay_pause( 4 );
    renderer_coordinator.drain_pending();

    CHECK_FALSE( renderer_recovery_test_support::replay_quarantine_empty() );
    CHECK_FALSE( renderer_coordinator.is_render_allowed() );

    // A drain while still suspended must not touch SDL: the quarantine stays
    // intact and no retry runs.
    renderer_coordinator.drain_pending();
    CHECK_FALSE( renderer_recovery_test_support::replay_quarantine_empty() );

    // Foreground drains the quarantine on the live renderer and completes.
    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();
    CHECK( renderer_recovery_test_support::replay_quarantine_empty() );
    CHECK( renderer_coordinator.is_render_allowed() );
}

TEST_CASE( "renderer_coordinator_retries_from_any_phase", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    // Bail the recipe at each successive phase gate; the next drain must
    // complete the rebuild from whatever partial teardown was left. A live
    // bundle keeps the atlas release/replay non-empty, and device_lost covers
    // retry after the renderer itself is destroyed and recreated.
    for( const renderer_recovery_severity sev : {
             renderer_recovery_severity::device_reset,
             renderer_recovery_severity::device_lost
         } ) {
        CAPTURE( static_cast<int>( sev ) );
        const uint64_t inst = renderer_coordinator.instance_generation();
        const uint64_t tex = renderer_coordinator.textures_generation();
        std::shared_ptr<const tileset> bundle =
            renderer_recovery_test_support::install_synthetic_bundle(
                "synthetic_sweep_ts", "color_pixel_sepia_light", inst, tex );
        REQUIRE( bundle );

        int gates_swept = 0;
        for( int phase = 1; phase <= 32; ++phase ) {
            CAPTURE( phase );
            renderer_coordinator.request_recovery( sev );
            renderer_recovery_test_support::arm_phase_fail_retry( phase );
            renderer_coordinator.drain_pending();
            if( !renderer_recovery_test_support::phase_fault_fired() ) {
                // Armed past the last gate: the drain completed without bailing.
                CHECK( renderer_coordinator.is_render_allowed() );
                break;
            }
            // The recipe bailed at this gate; the next drain completes from the
            // partial teardown.
            renderer_coordinator.drain_pending();
            CHECK( renderer_coordinator.is_render_allowed() );
            ++gates_swept;
        }
        CHECK( gates_swept >= 3 );
    }
}

#endif // TILES
