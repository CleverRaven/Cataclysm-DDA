#if defined(TILES)
#include <optional>
#include <string>

#include "atlas_bake_plan.h"
#include "cata_catch.h"
#include "cata_shader.h"
#include "cata_tiles.h"

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
#endif // TILES
