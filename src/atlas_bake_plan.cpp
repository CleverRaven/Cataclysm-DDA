#include "atlas_bake_plan.h"

#if defined(TILES)

#include "cata_shader.h"
#include "cata_tiles.h"

atlas_bake_plan compute_atlas_bake_plan( const bool shader_variants_available,
        const std::optional<cata_shader::memory_preset> preset,
        const bool tint_shader_available )
{
    atlas_bake_plan plan;
    if( !shader_variants_available ) {
        return plan;
    }
    plan.shadow = false;
    plan.night = false;
    plan.overexposed = false;
    // color_pixel_custom has no memory shader, so it keeps the bake
    plan.memory = !preset.has_value();
    plan.silhouette = !tint_shader_available;
    return plan;
}

std::string bake_plan_summary( const atlas_bake_plan &plan )
{
    std::string out;
    out += plan.normal ? 'n' : '-';
    out += plan.shadow ? 's' : '-';
    out += plan.night ? 'v' : '-';
    out += plan.overexposed ? 'o' : '-';
    out += plan.memory ? 'm' : '-';
    out += plan.silhouette ? 'i' : '-';
    return out;
}

bundle_state classify_bundle( const tileset *ts )
{
    if( !ts ) {
        return bundle_state::none;
    }
    // precheck returns before loader assigns id or uploads any atlas;
    // default-constructed tileset looks the same
    if( ts->get_tileset_id().empty() || ts->get_tile( 0 ) == nullptr ) {
        return bundle_state::metadata_only;
    }
    return bundle_state::uploaded;
}

bool bundle_needs_repair( const atlas_bake_plan &plan,
                          const std::string &mode_at_upload,
                          const uint64_t fingerprint_at_upload,
                          const std::string &applied_mode,
                          const uint64_t applied_fingerprint,
                          const bool shader_variants_available )
{
    // Every atlas texture carries the fingerprint's scale mode, so a changed
    // fingerprint makes the whole bundle stale.
    if( fingerprint_at_upload != applied_fingerprint ) {
        return true;
    }
    if( plan.memory && mode_at_upload != applied_mode ) {
        return true;
    }
    return plan.needs_rebake_for( shader_variants_available,
                                  cata_shader::memory_preset_from_option_value( applied_mode ) );
}

silhouette_miss classify_silhouette_miss( const atlas_bake_plan &plan,
        const std::optional<int> synthetic_highlight_index, const int sprite_index )
{
    if( !plan.silhouette ) {
        return silhouette_miss::skipped_by_plan;
    }
    if( synthetic_highlight_index && *synthetic_highlight_index == sprite_index ) {
        return silhouette_miss::normal_only_highlight;
    }
    return silhouette_miss::invalid;
}

#endif // TILES
