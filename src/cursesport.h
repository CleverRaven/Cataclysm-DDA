#pragma once
#ifndef CATACURSE_H
#define CATACURSE_H

#include <vector>
#include <array>
#include <string>
#include <bitset>


class nc_color;

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
typedef struct {
    base_color FG;
    base_color BG;
} pairs;

enum font_style_flag : int {
    FS_BOLD = 0,
    FS_ITALIC,
    FS_STRIKETHROUGH,
    FS_UNDERLINE,
    FS_MAX,
};

class font_style : public std::bitset<FS_MAX>
{
    public:
        bool operator<( const font_style &fs ) const {
            return to_ulong() < fs.to_ulong();
        }
};

//Individual lines, so that we can track changed lines
struct cursecell {
    std::string ch;
    base_color FG = static_cast<base_color>( 0 );
    base_color BG = static_cast<base_color>( 0 );
    font_style FS;

    cursecell( std::string ch ) : ch( std::move( ch ) ) { }
    cursecell() : cursecell( std::string( 1, ' ' ) ) { }

    bool operator==( const cursecell &b ) const {
        return FG == b.FG && BG == b.BG && FS == b.FS && ch == b.ch;
    }
};

struct curseline {
    bool touched;
    std::vector<cursecell> chars;
};

//The curses window struct
struct WINDOW {
    int x;//left side of window
    int y;//top side of window
    int width;
    int height;
    base_color FG;//current foreground color from attron
    base_color BG;//current background color from attron
    font_style FS;//current font style from attron
    bool inuse;// Does this window actually exist?
    bool draw;//Tracks if the window text has been changed
    int cursorx;
    int cursory;
    std::vector<curseline> line;
};

extern std::array<pairs, 100> colorpairs;
void curses_drawwindow( const catacurses::window &win );

// allow extra logic for framebuffer clears
extern void handle_additional_window_clear( WINDOW *win );

} // namespace cata_cursesport

//@todo: move into cata_cursesport
//used only in SDL mode for clearing windows using rendering
void clear_window_area( const catacurses::window &win );
int projected_window_width();
int projected_window_height();
bool handle_resize( int w, int h );

#endif
