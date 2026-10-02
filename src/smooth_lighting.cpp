#include "smooth_lighting.h"

#include <algorithm>

#include "cata_assert.h"
#include "coordinates.h"
#include "lightmap.h"

namespace smooth_lighting
{

// Character::get_vision_threshold caps the threshold at LIGHT_AMBIENT_LOW
static_assert( LIGHT_AMBIENT_LOW < LIGHT_AMBIENT_LIT );

bool needs_apparent_light( const lit_level ll )
{
    return ll == lit_level::LOW || ll == lit_level::LIT;
}

float light_above_threshold( const float apparent_light, const float vision_threshold )
{
    cata_assert( vision_threshold < LIGHT_AMBIENT_LIT );
    return std::clamp( ( apparent_light - vision_threshold ) /
                       ( LIGHT_AMBIENT_LIT - vision_threshold ), 0.0f, 1.0f );
}

light_cell classify_light_cell( const lit_level ll, const float apparent_light,
                                const float vision_threshold )
{
    switch( ll ) {
        case lit_level::BRIGHT:
            // clairvoyance, a light source's own tile, or light past
            // LIGHT_SOURCE_BRIGHT: full light, as classic tiles show them
            return { 1.0f, true };
        case lit_level::LOW:
        case lit_level::LIT:
            return { light_above_threshold( apparent_light, vision_threshold ), true };
        case lit_level::DARK:
        case lit_level::BRIGHT_ONLY:
        case lit_level::MEMORIZED:
        case lit_level::BLANK:
            return {};
    }
    return {};
}

bool sprite_stands( const sprite_footprint &f, const tile_geometry &g )
{
    int x0 = f.opaque.p_min.x;
    int x1 = f.opaque.p_max.x;
    int y0 = f.opaque.p_min.y;
    const int y1 = f.opaque.p_max.y;
    if( x1 <= x0 || y1 <= y0 ) {
        return false;
    }
    if( f.flip_horizontal ) {
        const int left = x0;
        x0 = f.size.x - x1;
        x1 = f.size.x - left;
    }
    if( f.flip_vertical ) {
        y0 = f.size.y - y1;
    }
    // opaque top after the turn, doubled to stay whole
    int top2 = 2 * y0;
    switch( f.turn ) {
        case quarter_turn::none:
            break;
        case quarter_turn::clockwise:
            top2 = 2 * x0 + f.size.y - f.size.x;
            break;
        case quarter_turn::counterclockwise:
            top2 = f.size.x + f.size.y - 2 * x1;
            break;
    }
    const float opaque_top = static_cast<float>( f.top ) + 0.5f * static_cast<float>( top2 ) *
                             f.pixelscale;
    const float ground = g.iso ? static_cast<float>( g.height ) - static_cast<float>( g.width ) / 2.0f
                         : 0.0f;
    return opaque_top < ground - standing_rise * static_cast<float>( g.width );
}

half_open_rectangle<point> lightmap_fill_area( const point &view_min, const point &view_max,
        const point &screen_min, const point &screen_max )
{
    return half_open_rectangle<point>(
               point( std::max( view_min.x, screen_min.x - filter_reach ),
                      std::max( view_min.y, screen_min.y - filter_reach ) ),
               point( std::min( view_max.x, screen_max.x + filter_reach ) + 1,
                      std::min( view_max.y, screen_max.y + filter_reach ) + 1 ) );
}

bool lightmap_extent::covers( const tripoint_bub_ms &p ) const
{
    return p.z() >= min_z && p.z() <= max_z && area.contains( p.xy().raw() );
}

bool lit_path_for( const bool lighting_active, const bool wants_scene_light,
                   const bool cell_covered )
{
    return lighting_active && wants_scene_light && cell_covered;
}

void lightmap_keys::begin_frame( const lightmap_fill_settings &settings )
{
    if( !settings_ || !( *settings_ == settings ) ) {
        forget_all();
        settings_ = settings;
    }
}

bool lightmap_keys::needs_fill( const int z, const layer_inputs &inputs ) const
{
    const std::optional<layer_inputs> &filled = filled_[z + OVERMAP_DEPTH];
    return !filled || !( *filled == inputs );
}

void lightmap_keys::mark_filled( const int z, const layer_inputs &inputs )
{
    filled_[z + OVERMAP_DEPTH] = inputs;
}

void lightmap_keys::forget( const int z )
{
    filled_[z + OVERMAP_DEPTH].reset();
}

void lightmap_keys::forget_all()
{
    filled_ = {};
}

} // namespace smooth_lighting
