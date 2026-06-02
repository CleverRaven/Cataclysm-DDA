#if defined(TILES)

#include <cstdint>
#include <memory>
#include <string>

#include "cata_catch.h"
#include "cata_tiles.h"
#include "options_helpers.h"
#include "sdl_renderer_recovery.h"

// Inbox/state-machine coverage that needs no live renderer. Each case drives a
// local coordinator so the atomic inbox, render embargo, paused-drain
// preservation, and lifecycle-epoch ordering are exercised through the public
// surface without touching the process-lifetime renderer_coordinator or any
// SDL resource.

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

// End-to-end coverage that drives the process-lifetime renderer_coordinator
// through its real recovery recipes against a headless software renderer. Each
// case stands up the fixture (which seeds and bootstraps the coordinator) and
// skips when the dummy SDL video backend is unavailable.

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

#endif // TILES
