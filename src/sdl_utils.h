#pragma once
#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include <algorithm>
#include <cmath>
#include <unordered_map>

#include "color.h"
#include "sdl_wrappers.h"

using color_pixel_function_pointer = SDL_Color( * )( const SDL_Color &color );
using color_pixel_function_map = std::unordered_map<std::string, color_pixel_function_pointer>;

color_pixel_function_pointer get_color_pixel_function( const std::string &name );

inline SDL_Color adjust_color_brightness( const SDL_Color &color, int percent )
{
    if( percent <= 0 ) {
        return { 0x00, 0x00, 0x00, color.a };
    }

    if( percent == 100 ) {
        return color;
    }

    return SDL_Color{
        static_cast<Uint8>( std::min( color.r *percent / 100, 0xFF ) ),
        static_cast<Uint8>( std::min( color.g *percent / 100, 0xFF ) ),
        static_cast<Uint8>( std::min( color.b *percent / 100, 0xFF ) ),
        color.a
    };
}

inline SDL_Color mix_colors( const SDL_Color &first, const SDL_Color &second, int second_percent )
{
    if( second_percent <= 0 ) {
        return first;
    }

    if( second_percent >= 100 ) {
        return second;
    }

    const int first_percent = 100 - second_percent;

    return SDL_Color{
        static_cast<Uint8>( std::min( ( first.r * first_percent + second.r * second_percent ) / 100, 0xFF ) ),
        static_cast<Uint8>( std::min( ( first.g * first_percent + second.g * second_percent ) / 100, 0xFF ) ),
        static_cast<Uint8>( std::min( ( first.b * first_percent + second.b * second_percent ) / 100, 0xFF ) ),
        static_cast<Uint8>( std::min( ( first.a * first_percent + second.a * second_percent ) / 100, 0xFF ) )
    };
}

inline bool is_black( const SDL_Color &color )
{
    return
        color.r == 0x00 &&
        color.g == 0x00 &&
        color.b == 0x00;
}

inline Uint8 average_pixel_color( const SDL_Color &color )
{
    return 85 * ( color.r + color.g + color.b ) >> 8; // 85/256 ~ 1/3
}

inline SDL_Color color_pixel_grayscale( const SDL_Color &color )
{
    if( is_black( color ) ) {
        return color;
    }

    const Uint8 av = average_pixel_color( color );
    const Uint8 result = std::max( av * 5 >> 3, 0x01 );

    return { result, result, result, color.a };
}

inline SDL_Color color_pixel_nightvision( const SDL_Color &color )
{
    const Uint8 av = average_pixel_color( color );
    const Uint8 result = std::min( ( av * ( ( av * 3 >> 2 ) + 64 ) >> 8 ) + 16, 0xFF );

    return {
        static_cast<Uint8>( result >> 2 ),
        static_cast<Uint8>( result ),
        static_cast<Uint8>( result >> 3 ),
        color.a
    };
}

inline SDL_Color color_pixel_overexposed( const SDL_Color &color )
{
    const Uint8 av = average_pixel_color( color );
    const Uint8 result = std::min( 64 + ( av * ( ( av >> 2 ) + 0xC0 ) >> 8 ), 0xFF );

    return {
        static_cast<Uint8>( result >> 2 ),
        static_cast<Uint8>( result ),
        static_cast<Uint8>( result >> 3 ),
        color.a
    };
}

inline SDL_Color color_pixel_darken( const SDL_Color &color )
{
    if( is_black( color ) ) {
        return color;
    }

    // 85/256 ~ 1/3
    return {
        std::max<Uint8>( 85 * color.r >> 8, 0x01 ),
        std::max<Uint8>( 85 * color.g >> 8, 0x01 ),
        std::max<Uint8>( 85 * color.b >> 8, 0x01 ),
        color.a
    };

}

inline SDL_Color color_pixel_sepia( const SDL_Color &color )
{
    if( is_black( color ) ) {
        return color;
    }

    /*
     *  Objective is to provide a gradient between two color points
     *  (sepia_dark and sepia_light) based on the grayscale value.
     *  This presents an effect intended to mimic a faded sepia photograph.
     */

    const SDL_Color sepia_dark = { 39, 23, 19, color.a};
    const SDL_Color sepia_light = { 241, 220, 163, color.a};

    const Uint8 av = average_pixel_color( color );
    const float gammav = 1.6;
    const float pv = av / 255.0;
    const Uint8 finalv = std::min( int( round( pow( pv, gammav ) * 150 ) ), 100 );

    return mix_colors( sepia_dark, sepia_light, finalv );
}

SDL_Color curses_color_to_SDL( const nc_color &color );

///@throws std::exception upon errors.
///@returns Always a valid pointer.
SDL_Surface_Ptr create_surface_32( int w, int h );

// SDL_RenderFillRect replacement handler
void render_fill_rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect, Uint32 r, Uint32 g,
                       Uint32 b );

#endif // SDL_UTILS_H
