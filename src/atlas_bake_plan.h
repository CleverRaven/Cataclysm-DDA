#pragma once
#ifndef CATA_SRC_ATLAS_BAKE_PLAN_H
#define CATA_SRC_ATLAS_BAKE_PLAN_H

#if defined(TILES)

#include <cstdint>
#include <optional>
#include <string>

class tileset;
namespace cata_shader
{
enum class memory_preset : int;
} // namespace cata_shader

// Which of the six atlas variants an upload must pre-bake. The normal atlas
// is always baked; the others are skipped only when the SDL3 shader path can
// generate them at draw time.
struct atlas_bake_plan {
    bool normal = true;
    bool shadow = true;
    bool night = true;
    bool overexposed = true;
    bool memory = true;
    bool silhouette = true;

    bool all_baked() const {
        return normal && shadow && night && overexposed && memory && silhouette;
    }
    // True if the bundle uploaded under this plan can no longer be drawn
    // correctly (a skipped shader variant is now missing, or memory preset
    // changed from one with shader to one without shader while memory was
    // skipped)
    bool needs_rebake_for( bool shader_variants_available,
                           std::optional<cata_shader::memory_preset> preset ) const {
        const bool skipped_variants = !shadow || !night || !overexposed || !silhouette;
        if( skipped_variants && !shader_variants_available ) {
            return true;
        }
        return !memory && ( !shader_variants_available || !preset.has_value() );
    }
};

// tint_shader_available is only honored with shader_variants_available
atlas_bake_plan compute_atlas_bake_plan( bool shader_variants_available,
        std::optional<cata_shader::memory_preset> preset,
        bool tint_shader_available );

// six letters in fixed order (n s v o m i), '-' for a skipped variant
std::string bake_plan_summary( const atlas_bake_plan &plan );

// What a tileset pointer holds: nothing, a precheck's metadata (no id, no
// atlases), or an uploaded atlas set.
enum class bundle_state {
    none,
    metadata_only,
    uploaded,
};
bundle_state classify_bundle( const tileset *ts );

// True if live bundle uploaded under `plan` with (mode_at_upload,
// fingerprint_at_upload) can't be drawn correctly under (applied_mode,
// applied_fingerprint) and current shader availability: atlases have stale
// fingerprint, baked memory atlas has another mode, or a variant it skipped has
// no shader now.
bool bundle_needs_repair( const atlas_bake_plan &plan,
                          const std::string &mode_at_upload,
                          uint64_t fingerprint_at_upload,
                          const std::string &applied_mode,
                          uint64_t applied_fingerprint,
                          bool shader_variants_available );

// why the mask replay found no silhouette for a recorded sprite
enum class silhouette_miss {
    // upload skipped silhouettes for the tint shader; present gate refuses the
    // frame until the replay bakes them
    skipped_by_plan,
    // synthetic item highlight has normal texture only
    normal_only_highlight,
    // missing expected bundle silhouette
    invalid,
};
silhouette_miss classify_silhouette_miss( const atlas_bake_plan &plan,
        std::optional<int> synthetic_highlight_index, int sprite_index );

#endif // TILES
#endif // CATA_SRC_ATLAS_BAKE_PLAN_H
