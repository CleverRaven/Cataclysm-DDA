#if defined(TILES)
#include <cstdint>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>

#include "atlas_bake_plan.h"
#include "cata_catch.h"
#include "cata_shader.h"
#include "cata_tiles.h"
#include "options.h"
#include "options_helpers.h"
#include "sdl_renderer_recovery.h"
#include "sdl_wrappers.h"
#include "sdltiles.h"

TEST_CASE( "atlas_bake_plan_full_when_shaders_unavailable", "[tiles][gpu]" )
{
    const atlas_bake_plan plan = compute_atlas_bake_plan( false, std::nullopt, false );
    CHECK( plan.normal );
    CHECK( plan.shadow );
    CHECK( plan.night );
    CHECK( plan.overexposed );
    CHECK( plan.memory );
    CHECK( plan.silhouette );
    CHECK( plan.all_baked() );
}

TEST_CASE( "atlas_bake_plan_skips_shader_variants_when_available", "[tiles][gpu]" )
{
    using cata_shader::memory_preset;
    SECTION( "named memory preset skips memory too" ) {
        const atlas_bake_plan plan =
            compute_atlas_bake_plan( true, memory_preset::SEPIA_LIGHT, false );
        CHECK( plan.normal );
        CHECK_FALSE( plan.shadow );
        CHECK_FALSE( plan.night );
        CHECK_FALSE( plan.overexposed );
        CHECK_FALSE( plan.memory );
        CHECK( plan.silhouette );
        CHECK_FALSE( plan.all_baked() );
    }
    SECTION( "custom memory preset keeps the memory bake" ) {
        const atlas_bake_plan plan = compute_atlas_bake_plan( true, std::nullopt, false );
        CHECK( plan.normal );
        CHECK_FALSE( plan.shadow );
        CHECK( plan.memory );
        CHECK( plan.silhouette );
    }
    SECTION( "tint shader drops silhouette bake" ) {
        const atlas_bake_plan plan =
            compute_atlas_bake_plan( true, memory_preset::DARKEN, true );
        CHECK_FALSE( plan.silhouette );
    }
    SECTION( "tint shader without variant shaders is not a valid input and keeps everything" ) {
        const atlas_bake_plan plan = compute_atlas_bake_plan( false, std::nullopt, true );
        CHECK( plan.all_baked() );
    }
}

TEST_CASE( "atlas_bake_plan_needs_rebake_when_memory_preset_leaves_shader_set", "[tiles][gpu]" )
{
    using cata_shader::memory_preset;
    const atlas_bake_plan skipped =
        compute_atlas_bake_plan( true, memory_preset::DARKEN, false );
    CHECK( skipped.needs_rebake_for( false, std::nullopt ) );
    CHECK( skipped.needs_rebake_for( true, std::nullopt ) );
    CHECK_FALSE( skipped.needs_rebake_for( true, memory_preset::BLUE_DARK ) );
    CHECK_FALSE( skipped.needs_rebake_for( true, memory_preset::DARKEN ) );
    const atlas_bake_plan full = compute_atlas_bake_plan( false, std::nullopt, false );
    CHECK_FALSE( full.needs_rebake_for( false, std::nullopt ) );
    CHECK_FALSE( full.needs_rebake_for( true, memory_preset::DARKEN ) );
}

TEST_CASE( "bundle_needs_repair_decision_per_bundle", "[tiles][gpu]" )
{
    using cata_shader::memory_preset;
    const atlas_bake_plan skipped = compute_atlas_bake_plan( true, memory_preset::DARKEN, false );
    const atlas_bake_plan full = compute_atlas_bake_plan( false, std::nullopt, false );
    SECTION( "named to named keeps a skipped bundle while the shaders run" ) {
        CHECK_FALSE( bundle_needs_repair( skipped, "color_pixel_darken", 1,
                                          "color_pixel_sepia_light", 1, true ) );
    }
    SECTION( "named to custom repairs a skipped bundle" ) {
        CHECK( bundle_needs_repair( skipped, "color_pixel_darken", 1,
                                    "color_pixel_custom", 1, true ) );
    }
    SECTION( "a fully baked bundle repairs on any mode change" ) {
        CHECK( bundle_needs_repair( full, "color_pixel_darken", 1,
                                    "color_pixel_sepia_light", 1, false ) );
        CHECK( bundle_needs_repair( full, "color_pixel_darken", 1,
                                    "color_pixel_custom", 2, false ) );
    }
    SECTION( "any fingerprint change repairs, whatever the plan" ) {
        CHECK( bundle_needs_repair( full, "color_pixel_custom", 1, "color_pixel_custom", 2, false ) );
        CHECK( bundle_needs_repair( skipped, "color_pixel_darken", 1, "color_pixel_darken", 2, true ) );
    }
    SECTION( "a skipped bundle repairs when the shaders are gone" ) {
        CHECK( bundle_needs_repair( skipped, "color_pixel_darken", 1, "color_pixel_darken", 1, false ) );
    }
    SECTION( "a matching bundle never repairs" ) {
        CHECK_FALSE( bundle_needs_repair( skipped, "color_pixel_darken", 1,
                                          "color_pixel_darken", 1, true ) );
        CHECK_FALSE( bundle_needs_repair( full, "color_pixel_custom", 1,
                                          "color_pixel_custom", 1, false ) );
        CHECK_FALSE( bundle_needs_repair( full, "color_pixel_darken", 1,
                                          "color_pixel_darken", 1, true ) );
    }
}

TEST_CASE( "classify_bundle_distinguishes_metadata_only_from_uploaded", "[tiles][gpu]" )
{
    CHECK( classify_bundle( nullptr ) == bundle_state::none );
    // stands in for a precheck bundle: no id, no atlas
    tileset metadata_only;
    CHECK( classify_bundle( &metadata_only ) == bundle_state::metadata_only );
}

TEST_CASE( "variant_pass_ensure_probed_is_unavailable_on_software_renderer", "[tiles][gpu]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    REQUIRE( vp );
    const int probes_before = renderer_recovery_test_support::variant_probe_count();
    CHECK( vp->ensure_probed() == cata_shader::probe_state::unavailable );
    // idempotent: second call doesn't re-run the probe
    CHECK( vp->ensure_probed() == cata_shader::probe_state::unavailable );
    CHECK( renderer_recovery_test_support::variant_probe_count() == probes_before + 1 );
}

TEST_CASE( "variant_pass_memory_preset_is_selected_before_any_upload", "[tiles][gpu]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    // fixture mirrors WinCreate, selects preset right after creating pass,
    // before any tileset is touched
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    REQUIRE( vp );
    CHECK( vp->active_memory_preset() ==
           cata_shader::memory_preset_from_option_value(
               get_option<std::string>( "MEMORY_MAP_MODE" ) ) );
}

TEST_CASE( "failed_reprobe_flush_reports_unsafe_without_a_second_probe", "[tiles][gpu]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    REQUIRE( vp );
    REQUIRE( vp->ensure_probed() == cata_shader::probe_state::unavailable );
    const int probes_before = renderer_recovery_test_support::variant_probe_count();
    renderer_recovery_test_support::arm_flush_failure();
    cata_shader::request_reprobe();
    CHECK( vp->ensure_probed() == cata_shader::probe_state::unsafe );
    CHECK( vp->boundary_lost() );
    CHECK( vp->shader_fault() );
    // failed reset returned before probing
    CHECK( renderer_recovery_test_support::variant_probe_count() == probes_before );
    // second call still unsafe and doesn't probe
    CHECK( vp->ensure_probed() == cata_shader::probe_state::unsafe );
    CHECK( renderer_recovery_test_support::variant_probe_count() == probes_before );
}

TEST_CASE( "failed_flush_sets_the_sticky_shader_fault", "[tiles][gpu]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    REQUIRE( vp );
    REQUIRE_FALSE( vp->shader_fault() );
    renderer_recovery_test_support::arm_flush_failure();
    // plain flush, not the one inside a reprobe reset
    CHECK_FALSE( vp->flush() );
    CHECK( vp->boundary_lost() );
    CHECK( vp->shader_fault() );
}

TEST_CASE( "explicit_reprobe_clears_the_sticky_shader_fault", "[tiles][gpu]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    REQUIRE( vp );
    renderer_recovery_test_support::mark_shader_fault();
    REQUIRE( vp->shader_fault() );
    REQUIRE_FALSE( vp->boundary_lost() );
    CHECK( vp->ensure_probed() == cata_shader::probe_state::unavailable );
    cata_shader::request_reprobe();
    // Software has no GPU device, so re-probe is still unavailable, but reset
    // was successful and cleared the fault.
    CHECK( vp->ensure_probed() == cata_shader::probe_state::unavailable );
    CHECK_FALSE( vp->shader_fault() );
}

TEST_CASE( "lost_shader_boundary_refuses_target_binds_until_rebind",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    cata_shader::variant_pass *vp = get_shared_variant_pass();
    REQUIRE( vp );
    SDL_Texture_Ptr target = CreateTexture( get_sdl_renderer(), SDL_PIXELFORMAT_ARGB8888,
                                            SDL_TEXTUREACCESS_TARGET, 4, 4 );
    REQUIRE( target );
    // SetupRenderTarget leaves the renderer on the window target
    REQUIRE( GetRenderTarget( get_sdl_renderer() ) == nullptr );

    renderer_recovery_test_support::arm_flush_failure();
    cata_shader::request_reprobe();
    REQUIRE( vp->ensure_probed() == cata_shader::probe_state::unsafe );

    // nothing bound, no unbind pending, so only lost boundary can stop flush()
    // taking its no-op path and switching targets
    CHECK( permanent_render_target_bind( get_sdl_renderer(), target.get(), vp )
           == bind_result::failed_in_switch );
    CHECK( GetRenderTarget( get_sdl_renderer() ) == nullptr );
    CHECK( renderer_boundary_recovery_pending() );

    // The texture belongs to the renderer the recovery destroys
    target.reset();
    renderer_coordinator.drain_pending();
    REQUIRE( renderer_coordinator.state() == renderer_recovery_state::ready );
    CHECK_FALSE( vp->boundary_lost() );
    CHECK( vp->shader_fault() );
    CHECK( permanent_render_target_bind( get_sdl_renderer(), nullptr, vp ) == bind_result::ok );
}

TEST_CASE( "upload_bakes_only_the_variants_the_plan_names", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    using cata_shader::memory_preset;
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();

    GIVEN( "a plan that relies on the variant shaders under a named preset" ) {
        const atlas_bake_plan plan =
            compute_atlas_bake_plan( true, memory_preset::SEPIA_LIGHT, false );
        const std::shared_ptr<const tileset> bundle =
            renderer_recovery_test_support::install_synthetic_bundle(
                "synthetic_skip_ts", "color_pixel_sepia_light", inst, tex, plan );
        REQUIRE( bundle );
        THEN( "only the normal and silhouette atlases exist" ) {
            CHECK( classify_bundle( bundle.get() ) == bundle_state::uploaded );
            CHECK( bundle->get_tile( 0 ) != nullptr );
            CHECK( bundle->get_silhouette_tile( 0 ) != nullptr );
            CHECK( bundle->get_shadow_tile( 0 ) == nullptr );
            CHECK( bundle->get_night_tile( 0 ) == nullptr );
            CHECK( bundle->get_overexposed_tile( 0 ) == nullptr );
            CHECK( bundle->get_memory_tile( 0 ) == nullptr );
            CHECK( bake_plan_summary( bundle->get_bake_plan_at_upload() ) == "n----i" );
        }
    }
    GIVEN( "same reliance under the custom preset" ) {
        const atlas_bake_plan plan = compute_atlas_bake_plan( true, std::nullopt, false );
        const std::shared_ptr<const tileset> bundle =
            renderer_recovery_test_support::install_synthetic_bundle(
                "synthetic_custom_ts", "color_pixel_custom", inst, tex, plan );
        REQUIRE( bundle );
        THEN( "memory atlas is still baked" ) {
            CHECK( bundle->get_memory_tile( 0 ) != nullptr );
            CHECK( bundle->get_shadow_tile( 0 ) == nullptr );
        }
    }
    GIVEN( "default plan" ) {
        const std::shared_ptr<const tileset> bundle =
            renderer_recovery_test_support::install_synthetic_bundle(
                "synthetic_full_ts", "color_pixel_sepia_light", inst, tex );
        REQUIRE( bundle );
        THEN( "all six variants exist and the upload configuration is recorded" ) {
            CHECK( bundle->get_shadow_tile( 0 ) != nullptr );
            CHECK( bundle->get_night_tile( 0 ) != nullptr );
            CHECK( bundle->get_overexposed_tile( 0 ) != nullptr );
            CHECK( bundle->get_memory_tile( 0 ) != nullptr );
            CHECK( bundle->get_silhouette_tile( 0 ) != nullptr );
            CHECK( bundle->get_bake_plan_at_upload().all_baked() );
            CHECK( bundle->get_filter_fingerprint_at_upload()
                   == compute_tileset_filter_fingerprint( "color_pixel_sepia_light" ) );
        }
    }
}

TEST_CASE( "mode2_injected_shader_boundary_loss_queues_device_lost",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    renderer_recovery_test_support::arm_mode2_interrupt(
        1, atlas_upload_interrupt::shader_boundary_lost );
    CHECK( renderer_coordinator.mode2_upload_poll() == atlas_upload_interrupt::shader_boundary_lost );
    CHECK( renderer_coordinator.pending() == renderer_recovery_severity::device_lost );
    renderer_coordinator.drain_pending();
    CHECK( renderer_coordinator.state() == renderer_recovery_state::ready );
}

TEST_CASE( "saved_mode_change_replays_under_the_applied_mode_and_rekeys_in_place",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    override_option darken( "MEMORY_MAP_MODE", "color_pixel_darken" );
    on_tiles_options_changed();
    const std::shared_ptr<const tileset> bundle =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_rekey_ts", "color_pixel_darken", inst, tex );
    REQUIRE( bundle );
    REQUIRE( renderer_coordinator.pending() == renderer_recovery_severity::none );

    override_option blue( "MEMORY_MAP_MODE", "color_pixel_blue_dark" );
    on_tiles_options_changed();
    CHECK( renderer_coordinator.pending() == renderer_recovery_severity::device_reset );
    renderer_coordinator.drain_pending();

    const uint64_t inst2 = renderer_coordinator.instance_generation();
    const uint64_t tex2 = renderer_coordinator.textures_generation();
    CHECK( bundle->get_memory_map_mode_at_upload() == "color_pixel_blue_dark" );
    CHECK( bundle->get_filter_fingerprint_at_upload() == applied_tile_atlas_config().fingerprint );
    CHECK( bundle->get_memory_tile( 0 ) != nullptr );
    CHECK_FALSE( renderer_recovery_test_support::cache_lookup_is_fresh(
                     "synthetic_rekey_ts", "color_pixel_darken", inst2, tex2 ) );
    // A miss in fetch_cached_bundle would run a real JSON load; confirm the hit first.
    REQUIRE( renderer_recovery_test_support::cache_lookup_is_fresh(
                 "synthetic_rekey_ts", "color_pixel_blue_dark", inst2, tex2 ) );
    CHECK( renderer_recovery_test_support::fetch_cached_bundle(
               "synthetic_rekey_ts", "color_pixel_blue_dark", inst2, tex2 ) == bundle );
}

TEST_CASE( "interrupted_replay_keeps_every_bundle_tracked", "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    override_option darken( "MEMORY_MAP_MODE", "color_pixel_darken" );
    on_tiles_options_changed();
    const std::shared_ptr<const tileset> first =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_track_a_ts", "color_pixel_darken", inst, tex );
    const std::shared_ptr<const tileset> second =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_track_b_ts", "color_pixel_darken", inst, tex );
    REQUIRE( first );
    REQUIRE( second );

    const auto check_both_current = [&]() {
        const uint64_t inst_now = renderer_coordinator.instance_generation();
        const uint64_t tex_now = renderer_coordinator.textures_generation();
        for( const std::shared_ptr<const tileset> &b : {
                 first, second
             } ) {
            CAPTURE( b->get_tileset_id() );
            CHECK( b->get_renderer_instance_generation_at_upload() == inst_now );
            CHECK( b->get_gpu_textures_generation_at_upload() == tex_now );
            CHECK( b->get_memory_map_mode_at_upload() == "color_pixel_blue_dark" );
        }
    };

    override_option blue( "MEMORY_MAP_MODE", "color_pixel_blue_dark" );
    on_tiles_options_changed();
    REQUIRE( renderer_coordinator.pending() == renderer_recovery_severity::device_reset );
    // 1x1 one-descriptor bundle costs 5 replay polls (entry, pre-descriptor, sub-rect,
    // post-loop, pre-publish), so the 6th is the second bundle's entry poll.
    renderer_recovery_test_support::arm_replay_pause( 6 );
    renderer_coordinator.drain_pending();
    // Pin where the pause landed: the first bundle committed and was re-keyed,
    // the second was never uploaded.
    REQUIRE( first->get_gpu_textures_generation_at_upload()
             == renderer_coordinator.textures_generation() );
    REQUIRE( first->get_memory_map_mode_at_upload() == "color_pixel_blue_dark" );
    REQUIRE( second->get_gpu_textures_generation_at_upload() == tex );
    REQUIRE( second->get_memory_map_mode_at_upload() == "color_pixel_darken" );

    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();
    REQUIRE( renderer_coordinator.is_render_allowed() );
    check_both_current();

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_reset );
    renderer_coordinator.drain_pending();
    check_both_current();

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_lost );
    renderer_coordinator.drain_pending();
    REQUIRE( renderer_coordinator.instance_generation() == inst + 1 );
    check_both_current();
}

TEST_CASE( "replay_keeps_same_id_bundles_from_different_modes_tracked",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    // Two contexts drew the same tileset under different historical modes.
    const std::shared_ptr<const tileset> darken =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_twin_ts", "color_pixel_darken", inst, tex );
    const std::shared_ptr<const tileset> sepia =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_twin_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( darken );
    REQUIRE( sepia );
    override_option blue( "MEMORY_MAP_MODE", "color_pixel_blue_dark" );

    const auto check_both_current = [&]() {
        const uint64_t inst_now = renderer_coordinator.instance_generation();
        const uint64_t tex_now = renderer_coordinator.textures_generation();
        for( const std::shared_ptr<const tileset> &b : {
                 darken, sepia
             } ) {
            CHECK( b->get_renderer_instance_generation_at_upload() == inst_now );
            CHECK( b->get_gpu_textures_generation_at_upload() == tex_now );
            CHECK( b->get_memory_map_mode_at_upload() == "color_pixel_blue_dark" );
        }
    };

    // both entries re-key onto one key
    on_tiles_options_changed();
    renderer_coordinator.drain_pending();
    check_both_current();
    const uint64_t inst_now = renderer_coordinator.instance_generation();
    const uint64_t tex_now = renderer_coordinator.textures_generation();
    REQUIRE( renderer_recovery_test_support::cache_lookup_is_fresh(
                 "synthetic_twin_ts", "color_pixel_blue_dark", inst_now, tex_now ) );
    CHECK( renderer_recovery_test_support::fetch_cached_bundle(
               "synthetic_twin_ts", "color_pixel_blue_dark", inst_now, tex_now ) == sepia );

    renderer_coordinator.request_recovery( renderer_recovery_severity::device_lost );
    renderer_coordinator.drain_pending();
    check_both_current();
}

TEST_CASE( "present_gate_holds_frames_until_a_stale_bundle_is_repaired",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    override_option blue( "MEMORY_MAP_MODE", "color_pixel_blue_dark" );
    on_tiles_options_changed();
    REQUIRE( renderer_coordinator.pending() == renderer_recovery_severity::none );

    // published under previous mode after the change was applied, so nothing
    // queued a repair: gate itself must refuse and request it
    const std::shared_ptr<const tileset> bundle =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_repair_ts", "color_pixel_darken", inst, tex );
    REQUIRE( bundle );
    renderer_recovery_test_support::set_needupdate( false );
    CHECK_FALSE( renderer_recovery_test_support::run_present_gate() );
    CHECK( renderer_recovery_test_support::needupdate_armed() );
    CHECK( renderer_coordinator.pending() == renderer_recovery_severity::device_reset );

    // repair interrupted before bundle commit: frames stay refused
    renderer_recovery_test_support::arm_replay_pause( 4 );
    renderer_coordinator.drain_pending();
    REQUIRE( bundle->get_memory_map_mode_at_upload() == "color_pixel_darken" );
    renderer_recovery_test_support::set_needupdate( false );
    CHECK_FALSE( renderer_recovery_test_support::run_present_gate() );

    renderer_coordinator.notify_lifecycle( lifecycle_state::resumed_pending_rebuild );
    renderer_coordinator.drain_pending();
    CHECK( bundle->get_memory_map_mode_at_upload() == "color_pixel_blue_dark" );
    renderer_recovery_test_support::set_needupdate( false );
    CHECK( renderer_recovery_test_support::run_present_gate() );
    CHECK_FALSE( renderer_recovery_test_support::needupdate_armed() );
}

TEST_CASE( "saved_scaling_mode_change_replays_atlases_with_the_new_filter",
           "[tiles][renderer_recovery]" )
{
    software_render_fixture fx;
    if( !fx.available() ) {
        WARN( "dummy SDL video backend unavailable; skipping" );
        return;
    }
    const uint64_t inst = renderer_coordinator.instance_generation();
    const uint64_t tex = renderer_coordinator.textures_generation();
    override_option sepia( "MEMORY_MAP_MODE", "color_pixel_sepia_light" );
    override_option unscaled( "SCALING_MODE", "none" );
    on_tiles_options_changed();
    const std::shared_ptr<const tileset> bundle =
        renderer_recovery_test_support::install_synthetic_bundle(
            "synthetic_scale_ts", "color_pixel_sepia_light", inst, tex );
    REQUIRE( bundle );
    REQUIRE( bundle->get_tile( 0 ) != nullptr );
    REQUIRE( GetTextureScaleMode( bundle->get_tile( 0 )->get_texture_ptr() )
             == SDL_SCALEMODE_NEAREST );
    REQUIRE( renderer_coordinator.pending() == renderer_recovery_severity::none );

    override_option linear( "SCALING_MODE", "linear" );
    on_tiles_options_changed();
    CHECK( renderer_coordinator.pending() == renderer_recovery_severity::device_reset );
    renderer_coordinator.drain_pending();

    REQUIRE( bundle->get_tile( 0 ) != nullptr );
    CHECK( bundle->get_filter_fingerprint_at_upload() == applied_tile_atlas_config().fingerprint );
    // The recorded fingerprint names the linear filter, so the textures must carry it.
    CHECK( GetTextureScaleMode( bundle->get_tile( 0 )->get_texture_ptr() )
           == SDL_SCALEMODE_LINEAR );
}
#endif // TILES
