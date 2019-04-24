#pragma once
#ifndef SDL_UTILS_H
#define SDL_UTILS_H

#include "sdl_wrappers.h"

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

#endif // SDL_UTILS_H
