#pragma once
#ifndef CATA_SRC_SDL_UTILS_H
#define CATA_SRC_SDL_UTILS_H

#include <string>
#include <unordered_map>
#include <vector>

#include "options.h"
#include "sdl_wrappers.h"

class nc_color;

using color_pixel_function_pointer = SDL_Color( * )( const SDL_Color &color );
using color_pixel_function_map = std::unordered_map<std::string, color_pixel_function_pointer>;

color_pixel_function_pointer get_color_pixel_function( const std::string &name );

SDL_Color adjust_color_brightness( const SDL_Color &color, int percent );
SDL_Color mix_colors( const SDL_Color &first, const SDL_Color &second,
                      int second_percent );

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

SDL_Color color_pixel_grayscale( const SDL_Color &color );
SDL_Color color_pixel_nightvision( const SDL_Color &color );
SDL_Color color_pixel_overexposed( const SDL_Color &color );
SDL_Color color_pixel_darken( const SDL_Color &color );
SDL_Color color_pixel_mixer( const SDL_Color &color, const float &gammav,
                             const SDL_Color &color_a, const SDL_Color &color_b );

inline SDL_Color color_pixel_sepia_light( const SDL_Color &color )
{
    const SDL_Color dark = { 39, 23, 19, color.a };
    const SDL_Color light = { 241, 220, 163, color.a };
    return color_pixel_mixer( color, 1.6f, dark, light );
}

inline SDL_Color color_pixel_sepia_dark( const SDL_Color &color )
{
    const SDL_Color dark = { 39, 23, 19, color.a };
    const SDL_Color light = { 70, 66, 60, color.a };
    return color_pixel_mixer( color, 1.0f, dark, light );
}

inline SDL_Color color_pixel_blue_dark( const SDL_Color &color )
{
    const SDL_Color dark = { 19, 23, 39, color.a };
    const SDL_Color light = { 60, 66, 70, color.a };
    return color_pixel_mixer( color, 1.0f, dark, light );
}

inline SDL_Color color_pixel_custom( const SDL_Color &color )
{
    const SDL_Color dark = { static_cast<Uint8>( get_option<int>( "MEMORY_RGB_DARK_RED" ) ), static_cast<Uint8>( get_option<int>( "MEMORY_RGB_DARK_GREEN" ) ), static_cast<Uint8>( get_option<int>( "MEMORY_RGB_DARK_BLUE" ) ), color.a };
    const SDL_Color light = { static_cast<Uint8>( get_option<int>( "MEMORY_RGB_BRIGHT_RED" ) ), static_cast<Uint8>( get_option<int>( "MEMORY_RGB_BRIGHT_GREEN" ) ), static_cast<Uint8>( get_option<int>( "MEMORY_RGB_BRIGHT_BLUE" ) ), color.a };
    return color_pixel_mixer( color, get_option<float>( "MEMORY_GAMMA" ), dark, light );
}

SDL_Color curses_color_to_SDL( const nc_color &color );

///@throws std::exception upon errors.
///@returns Always a valid pointer.
SDL_Surface_Ptr create_surface_32( int w, int h );

SDL_Rect fit_rect_inside( const SDL_Rect &inner, const SDL_Rect &outer );

/** Linearly interpolate intermediate colors between two given colors.
 * @param start_color: The color to start with.
 * @param end_color: The color that ends the interpolation.
 * @param additional_steps: Number of steps between the start and stop colors.
 * @return A vector of colors containing: the start color, all intermediate colors and finally the end color.
 * @note start with white (r=255, g=255, b=255) and end with blue (r=0, g=0, b=255) and use 2 additional steps:
 *     - The first intermediate color is: (r=170, g=170, b=255)
 *     - The second intermediate color is: (r=85, g=85, b=255)
 * Obviously the more intermediate steps there are, the harder it is to differentiate the intermediate colors.
 */
std::vector<SDL_Color> color_linear_interpolate( const SDL_Color &start_color,
        const SDL_Color &end_color, unsigned additional_steps );

#endif // CATA_SRC_SDL_UTILS_H
