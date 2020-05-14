#if defined(TILES) || defined(_WIN32)
#include "cursesport.h"

#include <cstdint>
#include <memory>

#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "game_ui.h"
#include "output.h"
#include "wcwidth.h"

/**
 * Whoever cares, btw. not my base design, but this is how it works:
 * In absent of a native curses library, this is a simple implementation to
 * store the data that would be given to the curses system.
 * It is later displayed using code in sdltiles.cpp. This file only contains
 * the curses interface.
 *
 * The struct WINDOW is the base. It acts as the normal curses window, having
 * width/height, current cursor location, current coloring (background/foreground)
 * and the actual text.
 * The text is split into lines (curseline), which contains cells (cursecell).
 * Each cell has individual foreground and background, and a character. The
 * character is an UTF-8 encoded string. It should be one or two console cells
 * width. If it's two cells width, the next cell in the line must be completely
 * empty (the string must not contain anything). Also the last cell of a line
 * must not contain a two cell width string.
 */

//***********************************
//Globals                           *
//***********************************

catacurses::window catacurses::stdscr;
std::array<cata_cursesport::pairs, 100> cata_cursesport::colorpairs;   //storage for pair'ed colored

static bool wmove_internal( const catacurses::window &win_, const point &p )
{
    if( !win_ ) {
        return false;
    }
    cata_cursesport::WINDOW &win = *win_.get<cata_cursesport::WINDOW>();
    if( p.x >= win.width ) {
        return false;
    }
    if( p.y >= win.height ) {
        return false;
    }
    if( p.y < 0 ) {
        return false;
    }
    if( p.x < 0 ) {
        return false;
    }
    win.cursor = p;
    return true;
}

//***********************************
//Pseudo-Curses Functions           *
//***********************************

catacurses::window catacurses::newwin( int nlines, int ncols, const point &begin )
{
    if( begin.y < 0 || begin.x < 0 ) {
        return window(); //it's the caller's problem now (since they have logging functions declared)
    }

    // default values
    if( ncols == 0 ) {
        ncols = TERMX - begin.x;
    }
    if( nlines == 0 ) {
        nlines = TERMY - begin.y;
    }

    cata_cursesport::WINDOW *newwindow = new cata_cursesport::WINDOW();
    newwindow->pos = begin;
    newwindow->width = ncols;
    newwindow->height = nlines;
    newwindow->inuse = true;
    newwindow->draw = false;
    newwindow->BG = black;
    newwindow->FG = static_cast<base_color>( 8 );
    newwindow->cursor = point_zero;
    newwindow->line.resize( nlines );

    for( int j = 0; j < nlines; j++ ) {
        newwindow->line[j].chars.resize( ncols );
        newwindow->line[j].touched = true; //Touch them all !?
    }
    return std::shared_ptr<void>( newwindow, []( void *const w ) {
        delete static_cast<cata_cursesport::WINDOW *>( w );
    } );
}

inline int newline( cata_cursesport::WINDOW *win )
{
    if( win->cursor.y < win->height - 1 ) {
        win->cursor.y++;
        win->cursor.x = 0;
        return 1;
    }
    return 0;
}

// move the cursor a single cell, jumps to the next line if the
// end of a line has been reached, also sets the touched flag.
inline void addedchar( cata_cursesport::WINDOW *win )
{
    win->cursor.x++;
    win->line[win->cursor.y].touched = true;
    if( win->cursor.x >= win->width ) {
        newline( win );
    }
}

//Borders the window with fancy lines!
void catacurses::wborder( const window &win_, chtype ls, chtype rs, chtype ts, chtype bs, chtype tl,
                          chtype tr, chtype bl, chtype br )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    if( win == nullptr ) {
        // TODO: log this
        return;
    }
    int i = 0;
    int j = 0;
    point old = win->cursor; // methods below move the cursor, save the value!

    const chtype border_ls = ls ? ls : LINE_XOXO;
    const chtype border_rs = rs ? rs : LINE_XOXO;
    const chtype border_ts = ts ? ts : LINE_OXOX;
    const chtype border_bs = bs ? bs : LINE_OXOX;
    const chtype border_tl = tl ? tl : LINE_OXXO;
    const chtype border_tr = tr ? tr : LINE_OOXX;
    const chtype border_bl = bl ? bl : LINE_XXOO;
    const chtype border_br = br ? br : LINE_XOOX;

    for( j = 1; j < win->height - 1; j++ ) {
        mvwaddch( win_, point( 0, j ), border_ls );
    }
    for( j = 1; j < win->height - 1; j++ ) {
        mvwaddch( win_, point( win->width - 1, j ), border_rs );
    }
    for( i = 1; i < win->width - 1; i++ ) {
        mvwaddch( win_, point( i, 0 ), border_ts );
    }
    for( i = 1; i < win->width - 1; i++ ) {
        mvwaddch( win_, point( i, win->height - 1 ), border_bs );
    }
    mvwaddch( win_, point_zero, border_tl );
    mvwaddch( win_, point( win->width - 1, 0 ), border_tr );
    mvwaddch( win_, point( 0, win->height - 1 ), border_bl );
    mvwaddch( win_, point( win->width - 1, win->height - 1 ), border_br );

    // methods above move the cursor, put it back
    wmove( win_, old );
    wattroff( win_, c_white );
}

void catacurses::mvwhline( const window &win, const point &p, chtype ch, int n )
{
    wattron( win, BORDER_COLOR );
    const chtype hline_char = ch ? ch : LINE_OXOX;
    for( int i = 0; i < n; i++ ) {
        mvwaddch( win, p + point( i, 0 ), hline_char );
    }
    wattroff( win, BORDER_COLOR );
}

void catacurses::mvwvline( const window &win, const point &p, chtype ch, int n )
{
    wattron( win, BORDER_COLOR );
    const chtype vline_char = ch ? ch : LINE_XOXO;
    for( int j = 0; j < n; j++ ) {
        mvwaddch( win, p + point( 0, j ), vline_char );
    }
    wattroff( win, BORDER_COLOR );
}

//Refreshes a window, causing it to redraw on top.
void catacurses::wrefresh( const window &win_ )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    // TODO: log win == nullptr
    if( win != nullptr && win->draw ) {
        cata_cursesport::curses_drawwindow( win_ );
    }
}

//Refreshes the main window, causing it to redraw on top.
void catacurses::refresh()
{
    return wrefresh( stdscr );
}

void catacurses::wredrawln( const window &/*win*/, int /*beg_line*/, int /*num_lines*/ )
{
    /**
     * This is a no-op for non-curses implementations. wincurse.cpp doesn't
     * use windows console for rendering, and sdltiles.cpp doesn't either.
     * If we had a console-based windows implementation, we'd need to do
     * something here to force the line to redraw.
     */
}

// Get a sequence of Unicode code points, store them in target
// return the display width of the extracted string.
inline int fill( const char *&fmt, int &len, std::string &target )
{
    const char *const start = fmt;
    int dlen = 0; // display width
    const char *tmpptr = fmt; // pointer for UTF8_getch, which increments it
    int tmplen = len;
    while( tmplen > 0 ) {
        const uint32_t ch = UTF8_getch( &tmpptr, &tmplen );
        // UNKNOWN_UNICODE is most likely a (vertical/horizontal) line or similar
        const int cw = ch == UNKNOWN_UNICODE ? 1 : mk_wcwidth( ch );
        if( cw > 0 && dlen > 0 ) {
            // Stop at the *second* non-zero-width character
            break;
        } else if( cw == -1 && start == fmt ) {
            // First char is a control character: they only disturb the screen,
            // so replace it with a single space (e.g. instead of a '\t').
            // Newlines at the begin of a sequence are handled in printstring
            target.assign( " ", 1 );
            len = tmplen;
            fmt = tmpptr;
            return 1; // the space
        } else if( cw == -1 ) {
            // Control character but behind some other characters, finish the sequence.
            // The character will either by handled by printstring (if it's a newline),
            // or by the next call to this function (replaced with a space).
            break;
        }
        fmt = tmpptr;
        dlen += cw;
    }
    target.assign( start, fmt - start );
    len -= target.length();
    return dlen;
}

// The current cell of the window, pointed to by the cursor. The next character
// written to that window should go in this cell.
// Returns nullptr if the cursor is invalid (outside the window).
inline cata_cursesport::cursecell *cur_cell( cata_cursesport::WINDOW *win )
{
    if( win->cursor.y >= win->height || win->cursor.x >= win->width ) {
        return nullptr;
    }
    return &win->line[win->cursor.y].chars[win->cursor.x];
}

//The core printing function, prints characters to the array, and sets colors
inline void printstring( cata_cursesport::WINDOW *win, const std::string &text )
{
    using cata_cursesport::cursecell;
    win->draw = true;
    int len = text.length();
    if( len == 0 ) {
        return;
    }
    const char *fmt = text.c_str();
    // avoid having an invalid cursorx, so that cur_cell will only return nullptr
    // when the bottom of the window has been reached.
    if( win->cursor.x >= win->width ) {
        if( newline( win ) == 0 ) {
            return;
        }
    }
    if( win->cursor.y >= win->height || win->cursor.x >= win->width ) {
        return;
    }
    if( win->cursor.x > 0 && win->line[win->cursor.y].chars[win->cursor.x].ch.empty() ) {
        // start inside a wide character, erase it for good
        win->line[win->cursor.y].chars[win->cursor.x - 1].ch.assign( " " );
    }
    while( len > 0 ) {
        if( *fmt == '\n' ) {
            if( newline( win ) == 0 ) {
                return;
            }
            fmt++;
            len--;
            continue;
        }
        cursecell *curcell = cur_cell( win );
        if( curcell == nullptr ) {
            return;
        }
        const int dlen = fill( fmt, len, curcell->ch );
        if( dlen >= 1 ) {
            curcell->FG = win->FG;
            curcell->BG = win->BG;
            addedchar( win );
        }
        if( dlen == 1 ) {
            // a wide character was converted to a narrow character leaving a null in the
            // following cell ~> clear it
            cursecell *seccell = cur_cell( win );
            if( seccell && seccell->ch.empty() ) {
                seccell->ch.assign( ' ', 1 );
            }
        } else if( dlen == 2 ) {
            // the second cell, per definition must be empty
            cursecell *seccell = cur_cell( win );
            if( seccell == nullptr ) {
                // the previous cell was valid, this one is outside of the window
                // --> the previous was the last cell of the last line
                // --> there should not be a two-cell width character in the last cell
                curcell->ch.assign( ' ', 1 );
                return;
            }
            seccell->FG = win->FG;
            seccell->BG = win->BG;
            seccell->ch.erase();
            addedchar( win );
            // Have just written a wide-character into the last cell, it would not
            // display correctly if it was the last *cell* of a line
            if( win->cursor.x == 1 ) {
                // So make that last cell a space, move the width
                // character in the first cell of the line
                seccell->ch = curcell->ch;
                curcell->ch.assign( 1, ' ' );
                // and make the second cell on the new line empty.
                addedchar( win );
                cursecell *thicell = cur_cell( win );
                if( thicell != nullptr ) {
                    thicell->ch.erase();
                }
            }
        }
        if( win->cursor.y >= win->height ) {
            return;
        }
    }
}

//Prints a formatted string to a window at the current cursor, base function
void catacurses::wprintw( const window &win, const std::string &text )
{
    if( !win ) {
        // TODO: log this
        return;
    }

    return printstring( win.get<cata_cursesport::WINDOW>(), text );
}

//Prints a formatted string to a window, moves the cursor
void catacurses::mvwprintw( const window &win, const point &p, const std::string &text )
{
    if( !wmove_internal( win, p ) ) {
        return;
    }
    return printstring( win.get<cata_cursesport::WINDOW>(), text );
}

//Resizes the underlying terminal after a Window's console resize(maybe?) Not used in TILES
void catacurses::resizeterm()
{
    game_ui::init_ui();
}

//erases a window of all text and attributes
void catacurses::werase( const window &win_ )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    if( win == nullptr ) {
        // TODO: log this
        return;
    }

    for( int j = 0; j < win->height; j++ ) {
        win->line[j].chars.assign( win->width, cata_cursesport::cursecell() );
        win->line[j].touched = true;
    }
    win->draw = true;
    wmove( win_, point_zero );
    //    wrefresh(win);
    handle_additional_window_clear( win );
}

//erases the main window of all text and attributes
void catacurses::erase()
{
    return werase( stdscr );
}

//pairs up a foreground and background color and puts it into the array of pairs
void catacurses::init_pair( const short pair, const base_color f, const base_color b )
{
    cata_cursesport::colorpairs[pair].FG = f;
    cata_cursesport::colorpairs[pair].BG = b;
}

//moves the cursor in a window
void catacurses::wmove( const window &win_, const point &p )
{
    if( !wmove_internal( win_, p ) ) {
        return;
    }
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    win->cursor = p;
}

//Clears the main window     I'm not sure if its suppose to do this?
void catacurses::clear()
{
    return wclear( stdscr );
}

//adds a character to the window
void catacurses::mvwaddch( const window &win, const point &p, const chtype ch )
{
    if( !wmove_internal( win, p ) ) {
        return;
    }
    return waddch( win, ch );
}

//clears a window
void catacurses::wclear( const window &win_ )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    werase( win_ );
    if( win == nullptr ) {
        // TODO: log this
        return;
    }

    for( int i = 0; i < win->pos.y && i < stdscr.get<cata_cursesport::WINDOW>()->height; i++ ) {
        stdscr.get<cata_cursesport::WINDOW>()->line[i].touched = true;
    }
}

//gets the max x of a window (the width)
int catacurses::getmaxx( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->width : 0;
}

//gets the max y of a window (the height)
int catacurses::getmaxy( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->height : 0;
}

//gets the beginning x of a window (the x pos)
int catacurses::getbegx( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->pos.x : 0;
}

//gets the beginning y of a window (the y pos)
int catacurses::getbegy( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->pos.y : 0;
}

//gets the current cursor x position in a window
int catacurses::getcurx( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->cursor.x : 0;
}

//gets the current cursor y position in a window
int catacurses::getcury( const window &win )
{
    return win ? win.get<cata_cursesport::WINDOW>()->cursor.y : 0;
}

void catacurses::curs_set( int )
{
}

void catacurses::wattron( const window &win_, const nc_color &attrs )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    if( win == nullptr ) {
        // TODO: log this
        return;
    }

    int pairNumber = attrs.to_color_pair_index();
    win->FG = cata_cursesport::colorpairs[pairNumber].FG;
    win->BG = cata_cursesport::colorpairs[pairNumber].BG;
    if( attrs.is_bold() ) {
        win->FG = static_cast<base_color>( win->FG + 8 );
    }
    if( attrs.is_blink() ) {
        win->BG = static_cast<base_color>( win->BG + 8 );
    }
}

void catacurses::wattroff( const window &win_, int )
{
    cata_cursesport::WINDOW *const win = win_.get<cata_cursesport::WINDOW>();
    if( win == nullptr ) {
        // TODO: log this
        return;
    }

    win->FG = static_cast<base_color>( 8 );                                //reset to white
    win->BG = black;                                //reset to black
}

void catacurses::waddch( const window &win, const chtype ch )
{
    return printstring( win.get<cata_cursesport::WINDOW>(), string_from_int( ch ) );
}

static constexpr int A_BLINK = 0x00000800; /* Added characters are blinking. */
static constexpr int A_BOLD = 0x00002000; /* Added characters are bold. */
static constexpr int A_COLOR = 0x03fe0000; /* Color bits */

nc_color nc_color::from_color_pair_index( const int index )
{
    return nc_color( index << 17 & A_COLOR );
}

int nc_color::to_color_pair_index() const
{
    return ( attribute_value & A_COLOR ) >> 17;
}

nc_color nc_color::bold() const
{
    return nc_color( attribute_value | A_BOLD );
}

bool nc_color::is_bold() const
{
    return attribute_value & A_BOLD;
}

nc_color nc_color::blink() const
{
    return nc_color( attribute_value | A_BLINK );
}

bool nc_color::is_blink() const
{
    return attribute_value & A_BLINK;
}

#endif
