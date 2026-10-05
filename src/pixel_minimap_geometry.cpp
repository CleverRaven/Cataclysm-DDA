#if defined(TILES)

#include "pixel_minimap_geometry.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "point.h"
#include "sdl_quad_batch.h"
#include "sdl_utils.h"

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

void append_beacon( quad_batch &batch, const SDL_Rect &rect,
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

#endif // TILES
