#pragma once
#ifndef CATA_SRC_TERMINAL_LAYOUT_H
#define CATA_SRC_TERMINAL_LAYOUT_H

#include <optional>
#include <string>
#include <vector>

#include "point.h"

// Terminal grid sizing for windows that change orientation (Android)
namespace terminal_layout
{

// window taller than it is wide. a square window counts as landscape
bool is_portrait( const point &window );

// pixels kept free along the bottom of a portrait window: the given share of
// the height, or the shortcut strip if that's taller
int portrait_reserved_bottom( int window_height, int reserve_percent, int shortcut_strip );

// cell sizes the bundled Terminus draws pixel-exact, smallest first. each is one
// of its bitmap strikes, used at a point size equal to the cell height
const std::vector<point> &text_size_presets();
// option id of a cell size, like "8x16"
std::string text_size_id( const point &cell );
// preset cell with this id, nullopt for anything else
std::optional<point> parse_text_size( const std::string &id );
// cell for a custom height: half as wide, rounded up
point custom_text_size( int height );
// shorter side of an area, the same in either orientation
int narrow_side( const point &area );
// pixels the text size is chosen against: the short side of the outer window, which
// keeps its size when bars inset the content, else of the SDL window
int sizing_reference( const point &window, const point &outer );
// largest preset whose 80 columns fit across `narrow` pixels, else the smallest
point auto_text_size( int narrow );
// cell for text size choice: "auto", "custom" or a preset id. unknown is auto
point resolve_text_size( const std::string &choice, int custom_height, int narrow );
// cells of `cell` fitting `bounds`, each axis lowered to its cap when the cap is
// above 0, then held within the supported terminal size. a grid held up to the
// minimum is bigger than the bounds
point fit_grid( const point &bounds, const point &cell, const point &caps );
// whole zoom of a bitmap cell `cell_width` wide so 80 columns fit `narrow`, at least 1
int bitmap_scale( int narrow, int cell_width );
// size a buffer is drawn at within bounds: `scale` times, or the largest smaller
// whole multiple that fits. a buffer bigger than bounds shrinks, keeping its aspect
point present_size( const point &buffer, const point &bounds, int scale );

} // namespace terminal_layout

#endif // CATA_SRC_TERMINAL_LAYOUT_H
