#pragma once
#ifndef CATACURSE_H
#define CATACURSE_H

#include <utility>
#if defined(TILES) || defined(_WIN32)

#include <array>
#include <string>
#include <vector>

#include "point.h"

namespace catacurses
{
class window;

enum base_color : short;
} // namespace catacurses

/**
 * Contains our own curses implementation.
 * Don't use it in the game itself, use only function of @ref catacurses.
 * Functions declared here are *not* defined in ncurses builds.
 */
namespace cata_cursesport
{
using base_color = catacurses::base_color;

//a pair of colors[] indexes, foreground and background
struct pairs {
    base_color FG;
    base_color BG;
};

//Individual lines, so that we can track changed lines
struct cursecell {
    std::string ch;
    base_color FG = static_cast<base_color>( 0 );
    base_color BG = static_cast<base_color>( 0 );

    cursecell( std::string ch ) : ch( std::move( ch ) ) { }
    cursecell() : cursecell( std::string( 1, ' ' ) ) { }

    bool operator==( const cursecell &b ) const {
        return FG == b.FG && BG == b.BG && ch == b.ch;
    }
};

struct curseline {
    bool touched;
    std::vector<cursecell> chars;
};

// The curses window struct
struct WINDOW {
    // Top-left corner of window
    point pos;
    int width;
    int height;
    // Current foreground color from attron
    base_color FG;
    // Current background color from attron
    base_color BG;
    // Does this window actually exist?
    bool inuse;
    // Tracks if the window text has been changed
    bool draw;
    point cursor;
    std::vector<curseline> line;
};

extern std::array<pairs, 100> colorpairs;
void curses_drawwindow( const catacurses::window &win );

// Allow extra logic for framebuffer clears
extern void handle_additional_window_clear( WINDOW *win );

} // namespace cata_cursesport

// TODO: move into cata_cursesport
// Used only in SDL mode for clearing windows using rendering
void clear_window_area( const catacurses::window &win );
int projected_window_width();
int projected_window_height();
bool handle_resize( int w, int h );
int get_scaling_factor();

#endif
#endif

