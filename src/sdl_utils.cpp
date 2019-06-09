#if defined(TILES)

#include "sdl_utils.h"

#include "color.h"
#include "color_loader.h"
#include "cursesport.h"
#include "sdltiles.h"

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

std::vector<SDL_Color> color_linear_interpolate( const SDL_Color &start_color, SDL_Color &end_color,
        const unsigned additional_steps )
{
    const unsigned steps = additional_steps + 2;
    std::vector<SDL_Color> colors;
    colors.reserve( steps );
    colors.push_back( start_color );

    const float step = 1.0f / ( steps - 1 );
    for( unsigned i = 0; i < steps - 2; i++ ) {
        const float percent = ( i + 1 ) * step;
        const auto r = static_cast<Uint8>( start_color.r + static_cast<Uint8>( percent *
                                           ( end_color.r - start_color.r ) ) );
        const auto g = static_cast<Uint8>( start_color.g + static_cast<Uint8>( percent *
                                           ( end_color.g - start_color.g ) ) );
        const auto b = static_cast<Uint8>( start_color.b + static_cast<Uint8>( percent *
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
        RenderCopy( renderer, alt_rect_tex, NULL, &rect );
    } else {
        SetRenderDrawColor( renderer, r, g, b, 255 );
        RenderFillRect( renderer, &rect );
    }
}

#endif // SDL_TILES
