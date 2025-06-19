#pragma once
#ifndef CATA_SRC_CURSESDEF_H
#define CATA_SRC_CURSESDEF_H

#include <memory>
#include <string>
#include <utility>

#include "string_formatter.h"

class nc_color;
struct point;

/**
 * Contains the curses interface used by the whole game.
 *
 * All code (except platform/OS/build type specific code) should use functions/
 * types from this namespace only. The @ref input_manager and @ref input_context
 * should be used for user input.
 *
 * There are currently (Nov 2017) two implementations for most of this interface:
 * - ncurses (mostly in ncurses_def.cpp). The interface originates from there,
 *   so it's mostly just forwarding to ncurses functions of the same name.
 * - our own curses library @ref cata_cursesport (mostly in cursesport.cpp),
 *   cursesport.h contains the structures and some functions specific to that.
 *
 * A few (system specific) functions are implemented in three versions:
 * - ncurses (ncurses_def.cpp),
 * - Windows curses (no SDL, using @ref cata_cursesport, see wincurses.cpp), and
 * - SDL curses (using @ref cata_cursesport, see sdltiles.cpp).
 *
 * As this interface is derived from ncurses, refer to documentation of that.
 *
 * The interface is in a separate namespace we can link with the ncurses library,
 * which exports its functions globally.
 */
namespace catacurses
{

/// @throws std::exception upon any errors. The caller should display / log it
/// and abort the program. Only continue the program when this returned normally.
void init_interface();

/**
 * A wrapper over a pointer to a curses window.
 * All curses functions here receive/return and operate on such a @ref window.
 * The objects have shared ownership of the contained pointer (behaves like a
 * shared pointer).
 *
 * Use @ref newwin to get a new instance. Use the default constructor to get
 * an invalid window pointer (test for this via @ref operator bool()).
 * Use the @ref operator= to reset the pointer.
 */
class window
{
    private:
        std::shared_ptr<void> native_window;

    public:
        window() = default;
        explicit window( std::shared_ptr<void> ptr ) : native_window( std::move( ptr ) ) {
        }
        template<typename T = void>
        T * get() const {
            return static_cast<T *>( native_window.get() );
        }
        explicit operator bool() const {
            return native_window.operator bool();
        }
        bool operator==( const window &rhs ) const {
            return native_window.get() == rhs.native_window.get();
        }
        std::weak_ptr<void> weak_ptr() const {
            return native_window;
        }
};

#if defined(USE_PDCURSES) && !defined(PDC_RGB)
enum base_color : short {
    black = 0x00,    // BGR{0, 0, 0}
    red = 0x04,      // BGR{0, 0, 192}
    green = 0x02,    // BGR{0, 196, 0}
    yellow = 0x06,   // BGR{30, 180, 196}
    blue = 0x01,     // BGR{196, 0, 0}
    magenta = 0x05,  // BGR{180, 0, 196}
    cyan = 0x03,     // BGR{200, 170, 0}
    white = 0x07,    // BGR{196, 196, 196}

    // 256 Color Support
    dark_gray = 237,
};
#else
enum base_color : short {
    black = 0x00,    // RGB{0, 0, 0}
    red = 0x01,      // RGB{196, 0, 0}
    green = 0x02,    // RGB{0, 196, 0}
    yellow = 0x03,   // RGB{196, 180, 30}
    blue = 0x04,     // RGB{0, 0, 196}
    magenta = 0x05,  // RGB{196, 0, 180}
    cyan = 0x06,     // RGB{0, 170, 200}
    white = 0x07,    // RGB{196, 196, 196}

    // 256 Color Support
    dark_gray = 237,
};
#endif

using chtype = int;
using attr_t = unsigned short;

extern window stdscr;
#if defined(USE_PDCURSES)
inline constexpr window &newscr = stdscr;
#else
extern window newscr;
#endif

window newwin( int nlines, int ncols, const point &begin );
void wborder( const window &win, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl, chtype tr,
              chtype bl, chtype br );
void mvwhline( const window &win, const point &p, chtype ch, int n );
void mvwvline( const window &win, const point &p, chtype ch, int n );
void wnoutrefresh( const window &win );
void wrefresh( const window &win );
void refresh();
void doupdate();
void wredrawln( const window &win, int beg_line, int num_lines );
void mvwprintw( const window &win, const point &p, const std::string &text );
template<typename ...Args>
inline void mvwprintw( const window &win, const point &p, const char *const fmt,
                       Args &&... args )
{
    return mvwprintw( win, p, string_format( fmt, std::forward<Args>( args )... ) );
}

void wprintw( const window &win, const std::string &text );
template<typename ...Args>
inline void wprintw( const window &win, const char *const fmt, Args &&... args )
{
    return wprintw( win, string_format( fmt, std::forward<Args>( args )... ) );
}

void resizeterm();
void werase( const window &win );
void init_pair( short pair, base_color f, base_color b );
void wmove( const window &win, const point &p );
void clear();
void erase();
void endwin();
void mvwaddch( const window &win, const point &p, chtype ch );
void wclear( const window &win );
void curs_set( int visibility );
// Set specified color, possibly including bold/blink attributes, for the window
void wattron( const window &win, const nc_color &attrs );
// Reset window color to white on black, no bold, no blink
void wattroff( const window &win, nc_color attrs );
void waddch( const window &win, chtype ch );
int getmaxy( const window &win );
int getmaxx( const window &win );
int getbegx( const window &win );
int getbegy( const window &win );
int getcurx( const window &win );
int getcury( const window &win );
bool supports_256_colors();
} // namespace catacurses

#endif // CATA_SRC_CURSESDEF_H
