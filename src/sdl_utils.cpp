#if defined(TILES)

#include "sdl_utils.h"

#include <array>
#include <utility>

#include "color.h"
#include "color_loader.h"
#include "cursesport.h"
#include "debug.h"
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

std::vector<SDL_Color> color_linear_interpolate( const SDL_Color &start_color,
        const SDL_Color &end_color, const unsigned additional_steps )
{
    const unsigned steps = additional_steps + 2;
    std::vector<SDL_Color> colors;
    colors.reserve( steps );
    colors.push_back( start_color );

    const float step = 1.0f / ( steps - 1 );
    for( unsigned i = 0; i < steps - 2; i++ ) {
        const float percent = ( i + 1 ) * step;
        const Uint8 r = static_cast<Uint8>( start_color.r + static_cast<Uint8>( percent *
                                            ( end_color.r - start_color.r ) ) );
        const Uint8 g = static_cast<Uint8>( start_color.g + static_cast<Uint8>( percent *
                                            ( end_color.g - start_color.g ) ) );
        const Uint8 b = static_cast<Uint8>( start_color.b + static_cast<Uint8>( percent *
                                            ( end_color.b - start_color.b ) ) );
        colors.push_back( { r, g, b, 255 } );
    }
    colors.push_back( end_color );
    return colors;
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
