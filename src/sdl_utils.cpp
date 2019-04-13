#include "sdl_utils.h"

#include "color.h"
#include "color_loader.h"
#include "cursesport.h"

extern SDL_Texture_Ptr alt_rect_tex;
extern bool alt_rect_tex_enabled;
extern void draw_alt_rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
                           Uint32 r, Uint32 g, Uint32 b );

extern std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> windowsPalette;


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
        RenderCopy( renderer, alt_rect_tex, NULL, &rect );
    } else {
        SetRenderDrawColor( renderer, r, g, b, 255 );
        RenderFillRect( renderer, &rect );
    }
}
