#pragma once
#ifndef CATA_SRC_SDLTILES_H
#define CATA_SRC_SDLTILES_H

#include <array>
#if defined(TILES)

#include <string>
#include <memory>

#include "color_loader.h"
#include "point.h"
#include "sdl_wrappers.h"

class cata_tiles;

namespace catacurses
{
class window;
} // namespace catacurses

extern SDL_Texture_Ptr alt_rect_tex;
extern bool alt_rect_tex_enabled;
extern std::unique_ptr<cata_tiles> tilecontext;
extern std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> windowsPalette;

void draw_alt_rect( const SDL_Renderer_Ptr &renderer, const SDL_Rect &rect,
                    Uint32 r, Uint32 g, Uint32 b );
void load_tileset();
void rescale_tileset( int size );
bool save_screenshot( const std::string &file_path );
void resize_term( int cell_w, int cell_h );
void toggle_fullscreen_window();

struct window_dimensions {
    point scaled_font_size;
    point window_pos_cell;
    point window_size_cell;
    point window_pos_pixel;
    point window_size_pixel;
};
window_dimensions get_window_dimensions( const catacurses::window &win );
// Get dimensional info of an imaginary normal catacurses::window with the given
// position and size. Unlike real catacurses::window, size can be zero.
window_dimensions get_window_dimensions( const point &pos, const point &size );

#endif // TILES

#endif // CATA_SRC_SDLTILES_H
