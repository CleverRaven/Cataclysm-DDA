#if defined(TILES)

#include "pixel_minimap_geometry.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <iterator>

#include "point.h"
#include "sdl_utils.h"

void minimap_vertex_batch::append_quad( const float x, const float y,
                                        const float w, const float h,
                                        const SDL_FColor &color )
{
    const Uint32 base = static_cast<Uint32>( vertex_count() );
    const float xs[4] = { x, x + w, x + w, x };
    const float ys[4] = { y, y, y + h, y + h };
    for( int i = 0; i < 4; ++i ) {
        xy_.push_back( xs[i] );
        xy_.push_back( ys[i] );
        colors_.push_back( color );
    }
    const Uint32 quad_indices[6] = { base, base + 1, base + 2, base, base + 2, base + 3 };
    indices_.insert( indices_.end(), std::begin( quad_indices ), std::end( quad_indices ) );
}

void minimap_vertex_batch::clear()
{
    xy_.clear();
    colors_.clear();
    indices_.clear();
}

bool minimap_vertex_batch::empty() const
{
    return indices_.empty();
}

int minimap_vertex_batch::vertex_count() const
{
    return static_cast<int>( colors_.size() );
}

int minimap_vertex_batch::index_count() const
{
    return static_cast<int>( indices_.size() );
}

const float *minimap_vertex_batch::xy_data() const
{
    return xy_.data();
}

const SDL_FColor *minimap_vertex_batch::color_data() const
{
    return colors_.data();
}

const Uint32 *minimap_vertex_batch::index_data() const
{
    return indices_.data();
}

void minimap_vertex_batch::reserve_quads( const int quads )
{
    xy_.reserve( 8 * static_cast<size_t>( quads ) );
    colors_.reserve( 4 * static_cast<size_t>( quads ) );
    indices_.reserve( 6 * static_cast<size_t>( quads ) );
}

SDL_FColor to_fcolor( const SDL_Color &color )
{
    return SDL_FColor{ color.r / 255.0f, color.g / 255.0f,
                       color.b / 255.0f, color.a / 255.0f };
}

minimap_transform compute_minimap_transform( const point &native,
        const SDL_Rect &screen_rect, const bool scale_to_fit )
{
    minimap_transform tf;
    if( scale_to_fit ) {
        tf.dest_rect = fit_rect_inside( SDL_Rect{ 0, 0, native.x, native.y }, screen_rect );
        tf.origin_x = tf.dest_rect.x;
        tf.origin_y = tf.dest_rect.y;
        tf.scale_x = static_cast<float>( tf.dest_rect.w ) / native.x;
        tf.scale_y = static_cast<float>( tf.dest_rect.h ) / native.y;
    } else {
        const point d( ( native.x - screen_rect.w ) / 2,
                       ( native.y - screen_rect.h ) / 2 );
        // Positive d crops the native image; negative d centers it on the
        // screen rect.
        tf.dest_rect = SDL_Rect{
            screen_rect.x - std::min( d.x, 0 ),
            screen_rect.y - std::min( d.y, 0 ),
            native.x - 2 * std::max( d.x, 0 ),
            native.y - 2 * std::max( d.y, 0 )
        };
        tf.origin_x = tf.dest_rect.x - std::max( d.x, 0 );
        tf.origin_y = tf.dest_rect.y - std::max( d.y, 0 );
    }
    return tf;
}

int snap_to_pixel( const float origin, const float scale, const int native )
{
    return static_cast<int>( std::lround( origin + scale * native ) );
}

void append_beacon( minimap_vertex_batch &batch, const SDL_Rect &rect,
                    const SDL_Color &color, const int edge_divisor )
{
    for( int x = -rect.w; x <= rect.w; ++x ) {
        const int y_range = rect.h - std::abs( x );
        for( int y = -y_range; y <= y_range; ++y ) {
            const bool on_edge = std::abs( y ) == y_range;
            const int divisor = on_edge ? edge_divisor : 1;
            const SDL_FColor c = to_fcolor( SDL_Color{
                static_cast<Uint8>( color.r / divisor ),
                static_cast<Uint8>( color.g / divisor ),
                static_cast<Uint8>( color.b / divisor ),
                0xFF } );
            batch.append_quad( static_cast<float>( rect.x + x ),
                               static_cast<float>( rect.y + y ),
                               1.0f, 1.0f, c );
        }
    }
}

void render_batch( const SDL_Renderer_Ptr &renderer, const minimap_vertex_batch &batch )
{
    RenderGeometryRaw( renderer,
                       batch.xy_data(), 2 * static_cast<int>( sizeof( float ) ),
                       batch.color_data(), static_cast<int>( sizeof( SDL_FColor ) ),
                       batch.vertex_count(),
                       batch.index_data(), batch.index_count() );
}

#endif // TILES
