#if defined(TILES)

#include "sdl_utils.h"

#include <stddef.h>
#include <array>

#include "color.h"
#include "color_loader.h"
#include "cursesport.h"
#include "sdltiles.h"

color_pixel_function_map builtin_color_pixel_functions = {
    { "color_pixel_none", nullptr },
    { "color_pixel_darken", color_pixel_darken },
    { "color_pixel_sepia", color_pixel_sepia },
    { "color_pixel_grayscale", color_pixel_grayscale },
    { "color_pixel_nightvision", color_pixel_nightvision },
    { "color_pixel_overexposed", color_pixel_overexposed },
};

color_pixel_function_pointer get_color_pixel_function( const std::string &name )
{
    const auto iter = builtin_color_pixel_functions.find( name );
    if( iter == builtin_color_pixel_functions.end() ) {
        debugmsg( "no color pixel function with name %s", name );
        return nullptr;
    }
    return iter->second;
}

SDL_Color curses_color_to_SDL( const nc_color &color )
{
    const int pair_id = color.to_color_pair_index();
    const auto pair = cata_cursesport::colorpairs[pair_id];

    int palette_index = pair.FG != 0 ? pair.FG : pair.BG;

    if( color.is_bold() ) {
        palette_index += color_loader<SDL_Color>::COLOR_NAMES_COUNT / 2;
    }

    return windowsPalette[palette_index];
}

SDL_Surface_Ptr create_surface_32( int w, int h )
{
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    return CreateRGBSurface( 0, w, h, 32, 0xFF000000, 0x00FF0000, 0x0000FF00, 0x000000FF );
#else
    return CreateRGBSurface( 0, w, h, 32, 0x000000FF, 0x0000FF00, 0x00FF0000, 0xFF000000 );
#endif
}

void render_fill_rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
                       Uint32 r, Uint32 g, Uint32 b )
{
    if( alt_rect_tex_enabled ) {
        SetTextureColorMod( alt_rect_tex, r, g, b );
        RenderCopy( renderer, alt_rect_tex, nullptr, &rect );
    } else {
        SetRenderDrawColor( renderer, r, g, b, 255 );
        RenderFillRect( renderer, &rect );
    }
}

SDL_Rect fit_rect_inside( const SDL_Rect &inner, const SDL_Rect &outer )
{
    const float inner_ratio = static_cast<float>( inner.w ) / inner.h;
    const float outer_ratio = static_cast<float>( outer.w ) / outer.h;
    const float factor = inner_ratio > outer_ratio
                         ? static_cast<float>( outer.w ) / inner.w
                         : static_cast<float>( outer.h ) / inner.h;

    const int w = factor * inner.w;
    const int h = factor * inner.h;
    const int x = outer.x + ( outer.w - w ) / 2;
    const int y = outer.y + ( outer.h - h ) / 2;

    return SDL_Rect{ x, y, w, h };
}

#endif // SDL_TILES
