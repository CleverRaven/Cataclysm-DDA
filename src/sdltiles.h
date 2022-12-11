#pragma once
#ifndef CATA_SRC_SDLTILES_H
#define CATA_SRC_SDLTILES_H

#include "point.h" // IWYU pragma: keep

namespace catacurses
{
class window;
} // namespace catacurses

#if defined(TILES)

#include <memory>
#include <string>

#include "color_loader.h"
#include "coordinates.h"
#include "sdl_wrappers.h"
#include "string_id.h"

#if defined(__APPLE__)
// For TARGET_OS_IPHONE macro to test if is on iOS
#include <TargetConditionals.h>
#endif

class cata_tiles;

struct weather_type;

using weather_type_id = string_id<weather_type>;

namespace catacurses
{
class window;
} // namespace catacurses

extern std::shared_ptr<cata_tiles> tilecontext;
extern std::shared_ptr<cata_tiles> closetilecontext;
extern std::shared_ptr<cata_tiles> fartilecontext;
extern std::unique_ptr<cata_tiles> overmap_tilecontext;
extern std::array<SDL_Color, color_loader<SDL_Color>::COLOR_NAMES_COUNT> windowsPalette;
extern int fontheight;
extern int fontwidth;

// This function may refresh the screen, so it should not be used where tiles
// may be displayed. Actually, this is supposed to be called from init.cpp,
// and only from there.
void load_tileset();
void rescale_tileset( int size );
bool save_screenshot( const std::string &file_path );
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

const SDL_Renderer_Ptr &get_sdl_renderer();

#endif // TILES

// Text level, valid only for a point relative to the window, not a point in overall space.
bool window_contains_point_relative( const catacurses::window &win, const point &p );
#endif // CATA_SRC_SDLTILES_H
