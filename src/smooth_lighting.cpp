#include "smooth_lighting.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "cata_assert.h"
#include "coordinates.h"
#include "level_cache.h"
#include "lightmap.h"
#include "map.h"
#include "mdarray.h"
#include "shadowcasting.h"
#include "tile_tint.h"

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

// a colored light's tint shows this much stronger as a multiplier than as the
// overlay compute_tile_tint sizes it for, which caps near 1/3
static constexpr float tint_strength_gain = 1.5f;
// sight edge fades over this band of the in-sight fraction
static constexpr float sight_edge_start = 0.3f;
static constexpr float sight_edge_end = 0.7f;
// filter weights below this count as nothing admitted
static constexpr float min_weight = 1.0e-4f;

size_t light_index( const point &p )
{
    return static_cast<size_t>( p.y ) * lightmap_width + p.x;
}

size_t reach_index( const point &p )
{
    return light_index( p ) + reach_column;
}

lightmap_texel encode_light_texel( const light_cell &cell, const std::array<float, 3> &hue,
                                   const bool barrier )
{
    lightmap_texel t;
    if( cell.detail ) {
        const float sum = hue[0] + hue[1] + hue[2];
        t.r = static_cast<uint8_t>( std::lround( 255.0f * cell.light ) );
        t.g = static_cast<uint8_t>( std::lround( 255.0f * hue[0] / sum ) );
        t.b = static_cast<uint8_t>( std::lround( 255.0f * hue[1] / sum ) );
        t.a = texel_detail;
    }
    if( barrier ) {
        t.a |= texel_barrier;
    }
    return t;
}

lightmap_texel encode_reach_texel( const uint32_t mask )
{
    lightmap_texel t;
    t.r = static_cast<uint8_t>( mask & 0xff );
    t.g = static_cast<uint8_t>( ( mask >> 8 ) & 0xff );
    t.b = static_cast<uint8_t>( ( mask >> 16 ) & 0xff );
    t.a = static_cast<uint8_t>( mask >> 24 );
    return t;
}

uint32_t texel_reach( const lightmap_texel &t )
{
    return static_cast<uint32_t>( t.r ) | ( static_cast<uint32_t>( t.g ) << 8 ) |
           ( static_cast<uint32_t>( t.b ) << 16 ) | ( static_cast<uint32_t>( t.a ) << 24 );
}

uint32_t reach_bit( const point &d )
{
    cata_assert( std::abs( d.x ) <= filter_reach && std::abs( d.y ) <= filter_reach );
    const int half = filter_window / 2;
    return 1u << ( ( d.y + half ) * filter_window + d.x + half );
}

std::array<float, 3> illumination_hue( const light_color_rgb &lc, const float scalar_light )
{
    std::array<float, 3> hue = { 1.0f, 1.0f, 1.0f };
    if( !lc.is_colored() ) {
        return hue;
    }
    const std::optional<tile_tint> tint = compute_tile_tint( lc, scalar_light );
    if( !tint ) {
        return hue;
    }
    // by the colored light's own strength, its brightest channel: its reach
    // ends where that falls to LIGHT_AMBIENT_LOW, and other light can keep the
    // tile lit past that
    const float colored = std::max( { lc.r, lc.g, lc.b } );
    const float t = std::clamp( ( colored - LIGHT_AMBIENT_LOW ) /
                                ( LIGHT_AMBIENT_LIT - LIGHT_AMBIENT_LOW ), 0.0f, 1.0f );
    const float fade = t * t * ( 3.0f - 2.0f * t );
    const float s = fade * std::min( 1.0f, tint_strength_gain * tint->a / 255.0f );
    hue[0] = 1.0f + s * ( tint->r / 255.0f - 1.0f );
    hue[1] = 1.0f + s * ( tint->g / 255.0f - 1.0f );
    hue[2] = 1.0f + s * ( tint->b / 255.0f - 1.0f );
    return hue;
}

bool barrier_grid::inside( const point &p ) const
{
    return p.x >= 0 && p.y >= 0 && p.x < width && p.y < height;
}

bool barrier_grid::barrier( const point &p ) const
{
    cata_assert( inside( p ) );
    return ( texels[static_cast<size_t>( p.y ) * stride + p.x].a & texel_barrier ) != 0;
}

// one step from `v` toward 0
static int toward_own( const int v )
{
    return v > 0 ? v - 1 : v < 0 ? v + 1 : 0;
}

uint32_t reach_mask( const barrier_grid &grid, const point &at )
{
    const bool own = grid.barrier( at );
    uint32_t mask = reach_bit( point::zero );
    // rings of growing distance in steps: a cell is reached when a reached
    // cell one step closer, along x or along y, joins it in the same class
    for( int d = 1; d <= 2 * filter_reach; ++d ) {
        for( int dy = -filter_reach; dy <= filter_reach; ++dy ) {
            const int rest = d - std::abs( dy );
            if( rest < 0 || rest > filter_reach ) {
                continue;
            }
            for( int dx = -rest; dx <= rest; dx += std::max( 1, 2 * rest ) ) {
                const point cell = at + point( dx, dy );
                // outside the map is no cell at all, whatever the own class
                if( !grid.inside( cell ) || grid.barrier( cell ) != own ) {
                    continue;
                }
                const bool from_x = dx != 0 && ( mask & reach_bit( point( toward_own( dx ), dy ) ) ) != 0;
                const bool from_y = dy != 0 && ( mask & reach_bit( point( dx, toward_own( dy ) ) ) ) != 0;
                if( from_x || from_y ) {
                    mask |= reach_bit( point( dx, dy ) );
                }
            }
        }
    }
    return mask;
}

bool encode_lightmap_layer( const map &here, const int z, const lightmap_fill_settings &settings,
                            std::vector<lightmap_texel> &out )
{
    out.assign( static_cast<size_t>( lightmap_width ) * MAPSIZE_Y, lightmap_texel() );
    const level_cache &ch = here.access_cache( z );
    const half_open_rectangle<point> &area = settings.area;
    static const std::array<float, 3> white = { 1.0f, 1.0f, 1.0f };
    bool any_detail = false;
    for( int y = area.p_min.y; y < area.p_max.y; ++y ) {
        for( int x = area.p_min.x; x < area.p_max.x; ++x ) {
            const lit_level ll = ch.visibility_cache[x][y];
            const float apparent = needs_apparent_light( ll )
                                   ? map::apparent_light_helper( ch, tripoint_bub_ms( x, y, z ) ).apparent_light
                                   : 0.0f;
            const light_cell cell = classify_light_cell( ll, apparent, settings.vision_threshold );
            any_detail |= cell.detail;
            const std::array<float, 3> hue = settings.tint && cell.detail
                                             ? illumination_hue( ch.light_color_cache[x][y], ch.lm[x][y].max() ) : white;
            // light barrier, so windows stay open to light
            out[light_index( point( x, y ) )] = encode_light_texel( cell, hue,
                                                ch.transparency_cache[x][y] <= LIGHT_TRANSPARENCY_SOLID );
        }
    }
    if( !any_detail ) {
        return false;
    }
    // per tile sampling never reads the masks
    if( !settings.masks ) {
        return true;
    }
    const barrier_grid grid{ out.data(), lightmap_width, MAPSIZE_X, MAPSIZE_Y };
    for( int y = area.p_min.y; y < area.p_max.y; ++y ) {
        for( int x = area.p_min.x; x < area.p_max.x; ++x ) {
            out[reach_index( point( x, y ) )] = encode_reach_texel( reach_mask( grid, point( x, y ) ) );
        }
    }
    return true;
}

static float bspline( float x )
{
    x = std::abs( x );
    if( x >= 2.0f ) {
        return 0.0f;
    }
    if( x >= 1.0f ) {
        const float a = 2.0f - x;
        return a * a * a / 6.0f;
    }
    return ( 4.0f - 6.0f * x * x + 3.0f * x * x * x ) / 6.0f;
}

static float smooth_step( const float lo, const float hi, const float x )
{
    const float t = std::clamp( ( x - lo ) / ( hi - lo ), 0.0f, 1.0f );
    return t * t * ( 3.0f - 2.0f * t );
}

namespace
{
struct decoded_texel {
    bool valid = false;
    float light = 0.0f;
    std::array<float, 3> chroma = {};
    bool detail = false;
    bool barrier = false;
};
} // namespace

// light texel at `cell`, empty outside the light columns or the rows of
// `level`: a sample never leaves its own level
static decoded_texel fetch_light( const lightmap_view &view, const point &cell, const int level )
{
    decoded_texel o;
    const int top = level * view.rows_per_level;
    if( cell.x < 0 || cell.x >= view.reach_column || cell.y < top ||
        cell.y >= top + view.rows_per_level ) {
        return o;
    }
    const lightmap_texel &t = view.texels[static_cast<size_t>( cell.y ) * view.width + cell.x];
    o.valid = true;
    o.light = t.r / 255.0f;
    o.chroma = { t.g / 255.0f, t.b / 255.0f, std::max( 1.0f - t.g / 255.0f - t.b / 255.0f, 0.0f ) };
    o.detail = ( t.a & texel_detail ) != 0;
    o.barrier = ( t.a & texel_barrier ) != 0;
    return o;
}

// reach mask of a cell fetch_light found valid
static uint32_t fetch_reach( const lightmap_view &view, const point &cell )
{
    return texel_reach( view.texels[static_cast<size_t>( cell.y ) * view.width + view.reach_column +
                                                         cell.x] );
}

static std::array<float, 3> chroma_hue( const std::array<float, 3> &c )
{
    const float m = std::max( { c[0], c[1], c[2], min_weight } );
    return { c[0] / m, c[1] / m, c[2] / m };
}

lit_sample reference_sample( const lightmap_view &view, const sample_params &params,
                             const lit_coords &coords )
{
    const float marker_half = 0.5f * standing_marker;
    const float column = coords.column + marker_half;
    const point own( static_cast<int>( std::floor( column ) ),
                     static_cast<int>( std::floor( coords.row + 0.5f ) ) );
    const bool standing = column - std::floor( column ) >= standing_marker;
    const int level = own.y / view.rows_per_level;
    float lx = coords.x - own.x;
    float ly = coords.y - own.y;
    if( standing ) {
        // light along the sprite's base line, the same all the way up
        if( params.iso ) {
            lx = ( lx + ly ) * 0.5f;
            ly = lx;
        } else {
            ly = 0.5f;
        }
    }
    lx = std::clamp( lx, 0.0f, 1.0f );
    ly = std::clamp( ly, 0.0f, 1.0f );
    const decoded_texel own_texel = fetch_light( view, own, level );
    lit_sample s;
    if( params.per_tile ) {
        s.light = own_texel.light;
        s.hue = chroma_hue( own_texel.chroma );
        s.in_sight = own_texel.detail ? 1.0f : 0.0f;
        s.visible = s.in_sight;
        s.weight = s.in_sight;
        return s;
    }
    // texel space: centers at whole numbers
    const float stx = own.x + lx - 0.5f;
    const float sty = own.y + ly - 0.5f;
    const point base( static_cast<int>( std::floor( stx ) ), static_cast<int>( std::floor( sty ) ) );
    const float fx = stx - base.x;
    const float fy = sty - base.y;
    float light_sum = 0.0f;
    std::array<float, 3> chroma_sum = {};
    float blend_weight = 0.0f;
    float sight_sum = 0.0f;
    float sight_weight = 0.0f;
    // 2x2 cells around the sample point that join the own cell through cells of
    // its class: a diagonal walled on both sides is not joined
    std::array<bool, 4> joined = {};
    for( int k = 0; k < 4; ++k ) {
        const decoded_texel ct = fetch_light( view, base + point( k % 2, k / 2 ), level );
        joined[k] = ct.valid && ct.barrier == own_texel.barrier;
    }
    const int own_k = ( own.y - base.y ) * 2 + own.x - base.x;
    // diagonal joins through either side cell
    joined[3 - own_k] = joined[3 - own_k] && ( joined[own_k ^ 1] || joined[own_k ^ 2] );
    // each joined cell filters with its own mask and their results blend
    // bilinearly, so neighbours of one class agree
    for( int cj = 0; cj <= 1; ++cj ) {
        for( int ci = 0; ci <= 1; ++ci ) {
            const point c = base + point( ci, cj );
            if( !joined[cj * 2 + ci] ) {
                continue;
            }
            const decoded_texel ct = fetch_light( view, c, level );
            const float bw = ( ci ? fx : 1.0f - fx ) * ( cj ? fy : 1.0f - fy );
            sight_weight += bw;
            if( !ct.detail ) {
                continue;
            }
            sight_sum += bw;
            const uint32_t mask = fetch_reach( view, c );
            float w_sum = 0.0f;
            float l_sum = 0.0f;
            std::array<float, 3> ch_sum = {};
            for( int j = -1; j <= 2; ++j ) {
                for( int i = -1; i <= 2; ++i ) {
                    const point tap = base + point( i, j );
                    const point d = tap - c;
                    if( ( mask & reach_bit( d ) ) == 0 ) {
                        continue;
                    }
                    const decoded_texel t = fetch_light( view, tap, level );
                    if( !t.detail ) {
                        continue;
                    }
                    const float w = bspline( stx - tap.x ) * bspline( sty - tap.y );
                    w_sum += w;
                    l_sum += w * t.light;
                    // chroma by filter weight alone: weighting it by light
                    // would give a faint colored cell its full hue
                    for( int k = 0; k < 3; ++k ) {
                        ch_sum[k] += w * t.chroma[k];
                    }
                }
            }
            // a detail cell admits itself, at most one cell from the point
            cata_assert( w_sum > 0.0f );
            light_sum += bw * l_sum / w_sum;
            for( int k = 0; k < 3; ++k ) {
                chroma_sum[k] += bw * ch_sum[k] / w_sum;
            }
            blend_weight += bw;
            s.weight += bw * w_sum;
        }
    }
    s.light = blend_weight > min_weight ? light_sum / blend_weight : 0.0f;
    std::array<float, 3> chroma = { 1.0f / 3.0f, 1.0f / 3.0f, 1.0f / 3.0f };
    if( blend_weight > min_weight ) {
        for( int k = 0; k < 3; ++k ) {
            chroma[k] = chroma_sum[k] / blend_weight;
        }
    }
    s.hue = chroma_hue( chroma );
    s.in_sight = sight_weight > min_weight ? sight_sum / sight_weight : 0.0f;
    s.visible = smooth_step( sight_edge_start, sight_edge_end, s.in_sight );
    return s;
}

static float mix1( const float a, const float b, const float t )
{
    return a + ( b - a ) * t;
}

static std::array<float, 3> mix3( const std::array<float, 3> &a, const std::array<float, 3> &b,
                                  const float t )
{
    return { mix1( a[0], b[0], t ), mix1( a[1], b[1], t ), mix1( a[2], b[2], t ) };
}

static float average( const std::array<float, 3> &rgb )
{
    return ( rgb[0] + rgb[1] + rgb[2] ) / 3.0f;
}

// memory_presets.glsl's memory_keep_black
static std::array<float, 3> keep_black( const std::array<float, 3> &rgb,
                                        const std::array<float, 3> &result )
{
    return rgb[0] + rgb[1] + rgb[2] <= 0.0f ? rgb : result;
}

static std::array<float, 3> memory_mixer( const std::array<float, 3> &rgb,
        const std::array<float, 3> &dark, const std::array<float, 3> &light, const float gamma )
{
    const float p = std::clamp( std::pow( average( rgb ), gamma ) * 1.5f, 0.0f, 1.0f );
    return keep_black( rgb, mix3( dark, light, p ) );
}

static std::array<float, 3> by255( const float r, const float g, const float b )
{
    return { r / 255.0f, g / 255.0f, b / 255.0f };
}

std::array<float, 3> reference_memory_rgb( const look_params &look,
        const std::array<float, 3> &rgb )
{
    switch( look.memory_look ) {
        case 0: {
            const float floor = 1.0f / 255.0f;
            return keep_black( rgb, {
                std::max( rgb[0] * 85.0f / 256.0f, floor ), std::max( rgb[1] * 85.0f / 256.0f, floor ),
                std::max( rgb[2] * 85.0f / 256.0f, floor )
            } );
        }
        case 1:
            return memory_mixer( rgb, by255( 39, 23, 19 ), by255( 241, 220, 163 ), 1.6f );
        case 2:
            return memory_mixer( rgb, by255( 39, 23, 19 ), by255( 70, 66, 60 ), 1.0f );
        case 3:
            return memory_mixer( rgb, by255( 19, 23, 39 ), by255( 60, 66, 70 ), 1.0f );
        default:
            return memory_mixer( rgb, look.custom_dark, look.custom_light, look.custom_gamma );
    }
}

static std::array<float, 3> nightvision_green( const float result )
{
    return { result * 0.25f, result, result * 0.125f };
}

std::array<float, 3> reference_lit_rgb( const look_params &look, const std::array<float, 3> &rgb,
                                        const lit_sample &s )
{
    const float gray = average( rgb );
    const std::array<float, 3> drained = mix3( { gray, gray, gray }, rgb,
                                         smooth_step( 0.0f, full_color_light, s.light ) );
    const std::array<float, 3> lit = { drained[0] *s.hue[0], drained[1] *s.hue[1], drained[2] *s.hue[2] };
    const float shade = mix1( shadow_shade, 1.0f, s.light );
    const std::array<float, 3> seen = { lit[0] *shade, lit[1] *shade, lit[2] *shade };
    const std::array<float, 3> unseen = look.blend_memory ? reference_memory_rgb( look, rgb ) :
                                        std::array<float, 3> { 0.0f, 0.0f, 0.0f };
    return mix3( unseen, seen, s.visible );
}

std::array<float, 3> reference_night_rgb( const look_params &look, const std::array<float, 3> &rgb,
        const lit_sample &s )
{
    const float av = average( rgb );
    const float night = std::min( av * ( av * 0.75f + 64.0f / 255.0f ) + 16.0f / 255.0f, 1.0f );
    const float over = std::min( 64.0f / 255.0f + av * ( av * 0.25f + 192.0f / 255.0f ), 1.0f );
    const std::array<float, 3> dim = nightvision_green( night * mix1( night_floor, 1.0f, s.light ) );
    const std::array<float, 3> nv = mix3( dim, nightvision_green( over ),
                                          smooth_step( overexpose_start, 1.0f, s.light ) );
    const std::array<float, 3> unseen = look.blend_memory ? reference_memory_rgb( look, rgb ) :
                                        std::array<float, 3> { 0.0f, 0.0f, 0.0f };
    return mix3( unseen, nv, s.visible );
}

} // namespace smooth_lighting
