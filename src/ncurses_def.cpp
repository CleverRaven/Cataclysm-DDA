#if !(defined(TILES) || defined(_WIN32))

// input.h must be include *before* the ncurses header. The later has some macro
// defines that clash with the constants defined in input.h (e.g. KEY_UP).
#include "input.h"

// ncurses can define some functions as macros, but we need those identifiers
// to be unchanged by the preprocessor, as we use them as function names.
#define NCURSES_NOMACROS
#if defined(__CYGWIN__)
#include <ncurses/curses.h>
#else
#include <curses.h>
#endif

#include <stdexcept>

#include "cursesdef.h"
#include "catacharset.h"
#include "color.h"
#include "game_ui.h"

extern int VIEW_OFFSET_X; // X position of terrain window
extern int VIEW_OFFSET_Y; // Y position of terrain window

static void curses_check_result( const int result, const int expected, const char *const /*name*/ )
{
    if( result != expected ) {
        // TODO: debug message
    }
}

catacurses::window catacurses::newwin( const int nlines, const int ncols, const point &begin )
{
    const auto w = ::newwin( nlines, ncols, begin.y, begin.x ); // TODO: check for errors
    return std::shared_ptr<void>( w, []( void *const w ) {
        ::curses_check_result( ::delwin( static_cast<::WINDOW *>( w ) ), OK, "delwin" );
    } );
}

void catacurses::wrefresh( const window &win )
{
    return curses_check_result( ::wrefresh( win.get<::WINDOW>() ), OK, "wrefresh" );
}

void catacurses::werase( const window &win )
{
    return curses_check_result( ::werase( win.get<::WINDOW>() ), OK, "werase" );
}

int catacurses::getmaxx( const window &win )
{
    return ::getmaxx( win.get<::WINDOW>() );
}

int catacurses::getmaxy( const window &win )
{
    return ::getmaxy( win.get<::WINDOW>() );
}

int catacurses::getbegx( const window &win )
{
    return ::getbegx( win.get<::WINDOW>() );
}

int catacurses::getbegy( const window &win )
{
    return ::getbegy( win.get<::WINDOW>() );
}

int catacurses::getcurx( const window &win )
{
    return ::getcurx( win.get<::WINDOW>() );
}

int catacurses::getcury( const window &win )
{
    return ::getcury( win.get<::WINDOW>() );
}

void catacurses::wattroff( const window &win, const int attrs )
{
    return curses_check_result( ::wattroff( win.get<::WINDOW>(), attrs ), OK, "wattroff" );
}

void catacurses::wattron( const window &win, const nc_color &attrs )
{
    return curses_check_result( ::wattron( win.get<::WINDOW>(), attrs ), OK, "wattron" );
}

void catacurses::wmove( const window &win, const point &p )
{
    return curses_check_result( ::wmove( win.get<::WINDOW>(), p.y, p.x ), OK, "wmove" );
}

void catacurses::mvwprintw( const window &win, const point &p, const std::string &text )
{
    return curses_check_result( ::mvwprintw( win.get<::WINDOW>(), p.y, p.x, "%s", text.c_str() ),
                                OK, "mvwprintw" );
}

void catacurses::wprintw( const window &win, const std::string &text )
{
    return curses_check_result( ::wprintw( win.get<::WINDOW>(), "%s", text.c_str() ),
                                OK, "wprintw" );
}

void catacurses::refresh()
{
    return curses_check_result( ::refresh(), OK, "refresh" );
}

void catacurses::clear()
{
    return curses_check_result( ::clear(), OK, "clear" );
}

void catacurses::erase()
{
    return curses_check_result( ::erase(), OK, "erase" );
}

void catacurses::endwin()
{
    return curses_check_result( ::endwin(), OK, "endwin" );
}

void catacurses::wborder( const window &win, const chtype ls, const chtype rs, const chtype ts,
                          const chtype bs, const chtype tl, const chtype tr, const chtype bl, const chtype br )
{
    return curses_check_result( ::wborder( win.get<::WINDOW>(), ls, rs, ts, bs, tl, tr, bl, br ), OK,
                                "wborder" );
}

void catacurses::mvwhline( const window &win, const point &p, const chtype ch, const int n )
{
    return curses_check_result( ::mvwhline( win.get<::WINDOW>(), p.y, p.x, ch, n ), OK,
                                "mvwhline" );
}

void catacurses::mvwvline( const window &win, const point &p, const chtype ch, const int n )
{
    return curses_check_result( ::mvwvline( win.get<::WINDOW>(), p.y, p.x, ch, n ), OK,
                                "mvwvline" );
}

void catacurses::mvwaddch( const window &win, const point &p, const chtype ch )
{
    return curses_check_result( ::mvwaddch( win.get<::WINDOW>(), p.y, p.x, ch ), OK, "mvwaddch" );
}

void catacurses::waddch( const window &win, const chtype ch )
{
    return curses_check_result( ::waddch( win.get<::WINDOW>(), ch ), OK, "waddch" );
}

void catacurses::wredrawln( const window &win, const int beg_line, const int num_lines )
{
    return curses_check_result( ::wredrawln( win.get<::WINDOW>(), beg_line, num_lines ), OK,
                                "wredrawln" );
}

void catacurses::wclear( const window &win )
{
    return curses_check_result( ::wclear( win.get<::WINDOW>() ), OK, "wclear" );
}

void catacurses::curs_set( const int visibility )
{
    return curses_check_result( ::curs_set( visibility ), OK, "curs_set" );
}

// As long as these assertions hold, we can just case base_color into short and
// forward the result to ncurses init_pair. Otherwise we would have to translate them.
static_assert( catacurses::black == COLOR_BLACK,
               "black must have the same value as COLOR_BLACK (from ncurses)" );
static_assert( catacurses::red == COLOR_RED,
               "red must have the same value as COLOR_RED (from ncurses)" );
static_assert( catacurses::green == COLOR_GREEN,
               "green must have the same value as COLOR_GREEN (from ncurses)" );
static_assert( catacurses::yellow == COLOR_YELLOW,
               "yellow must have the same value as COLOR_YELLOW (from ncurses)" );
static_assert( catacurses::blue == COLOR_BLUE,
               "blue must have the same value as COLOR_BLUE (from ncurses)" );
static_assert( catacurses::magenta == COLOR_MAGENTA,
               "magenta must have the same value as COLOR_MAGENTA (from ncurses)" );
static_assert( catacurses::cyan == COLOR_CYAN,
               "cyan must have the same value as COLOR_CYAN (from ncurses)" );
static_assert( catacurses::white == COLOR_WHITE,
               "base_color::white must have the same value as COLOR_WHITE (from ncurses)" );

void catacurses::init_pair( const short pair, const base_color f, const base_color b )
{
    return curses_check_result( ::init_pair( pair, static_cast<short>( f ), static_cast<short>( b ) ),
                                OK, "init_pair" );
}

catacurses::window catacurses::stdscr;

void catacurses::resizeterm()
{
    const int new_x = ::getmaxx( stdscr.get<::WINDOW>() );
    const int new_y = ::getmaxy( stdscr.get<::WINDOW>() );
    if( ::is_term_resized( new_x, new_y ) ) {
        game_ui::init_ui();
    }
}

// init_interface is defined in another cpp file, depending on build type:
// wincurse.cpp for Windows builds without SDL and sdltiles.cpp for SDL builds.
void catacurses::init_interface()
{
    // ::endwin will free the pointer returned by ::initscr
    stdscr = std::shared_ptr<void>( ::initscr(), []( void *const ) { } );
    if( !stdscr ) {
        throw std::runtime_error( "initscr failed" );
    }
#if !defined(__CYGWIN__)
    // ncurses mouse registration
    mousemask( BUTTON1_CLICKED | BUTTON3_CLICKED | REPORT_MOUSE_POSITION, NULL );
#endif
    // our curses wrapper does not support changing this behavior, ncurses must
    // behave exactly like the wrapper, therefor:
    noecho();  // Don't echo keypresses
    cbreak();  // C-style breaks (e.g. ^C to SIGINT)
    keypad( stdscr.get<::WINDOW>(), true ); // Numpad is numbers
    set_escdelay( 10 ); // Make Escape actually responsive
    start_color(); // TODO: error checking
    init_colors();
}

input_event input_manager::get_input_event()
{
    previously_pressed_key = 0;
    const int key = getch();
    if( key != ERR ) {
        int newch;
        // Clear the buffer of characters that match the one we're going to act on.
        set_timeout( 0 );
        do {
            newch = getch();
        } while( newch != ERR && newch == key );
        reset_timeout();
        // If we read a different character than the one we're going to act on, re-queue it.
        if( newch != ERR && newch != key ) {
            ungetch( newch );
        }
    }
    input_event rval;
    if( key == ERR ) {
        if( input_timeout > 0 ) {
            rval.type = CATA_INPUT_TIMEOUT;
        } else {
            rval.type = CATA_INPUT_ERROR;
        }
        // ncurses mouse handling
    } else if( key == KEY_RESIZE ) {
        catacurses::resizeterm();
    } else if( key == KEY_MOUSE ) {
        MEVENT event;
        if( getmouse( &event ) == OK ) {
            rval.type = CATA_INPUT_MOUSE;
            rval.mouse_pos = point( event.x, event.y ) - point( VIEW_OFFSET_X, VIEW_OFFSET_Y );
            if( event.bstate & BUTTON1_CLICKED ) {
                rval.add_input( MOUSE_BUTTON_LEFT );
            } else if( event.bstate & BUTTON3_CLICKED ) {
                rval.add_input( MOUSE_BUTTON_RIGHT );
            } else if( event.bstate & REPORT_MOUSE_POSITION ) {
                rval.add_input( MOUSE_MOVE );
                if( input_timeout > 0 ) {
                    // Mouse movement seems to clear ncurses timeout
                    set_timeout( input_timeout );
                }
            } else {
                rval.type = CATA_INPUT_ERROR;
            }
        } else {
            rval.type = CATA_INPUT_ERROR;
        }
    } else {
        if( key == 127 ) { // == Unicode DELETE
            previously_pressed_key = KEY_BACKSPACE;
            return input_event( KEY_BACKSPACE, CATA_INPUT_KEYBOARD );
        }
        rval.type = CATA_INPUT_KEYBOARD;
        rval.text.append( 1, static_cast<char>( key ) );
        // Read the UTF-8 sequence (if any)
        if( key < 127 ) {
            // Single byte sequence
        } else if( 194 <= key && key <= 223 ) {
            rval.text.append( 1, static_cast<char>( getch() ) );
        } else if( 224 <= key && key <= 239 ) {
            rval.text.append( 1, static_cast<char>( getch() ) );
            rval.text.append( 1, static_cast<char>( getch() ) );
        } else if( 240 <= key && key <= 244 ) {
            rval.text.append( 1, static_cast<char>( getch() ) );
            rval.text.append( 1, static_cast<char>( getch() ) );
            rval.text.append( 1, static_cast<char>( getch() ) );
        } else {
            // Other control character, etc. - no text at all, return an event
            // without the text property
            previously_pressed_key = key;
            return input_event( key, CATA_INPUT_KEYBOARD );
        }
        // Now we have loaded an UTF-8 sequence (possibly several bytes)
        // but we should only return *one* key, so return the code point of it.
        const uint32_t cp = UTF8_getch( rval.text );
        if( cp == UNKNOWN_UNICODE ) {
            // Invalid UTF-8 sequence, this should never happen, what now?
            // Maybe return any error instead?
            previously_pressed_key = key;
            return input_event( key, CATA_INPUT_KEYBOARD );
        }
        previously_pressed_key = cp;
        // for compatibility only add the first byte, not the code point
        // as it would  conflict with the special keys defined by ncurses
        rval.add_input( key );
    }

    return rval;
}

void input_manager::set_timeout( const int delay )
{
    timeout( delay );
    // Use this to determine when curses should return a CATA_INPUT_TIMEOUT event.
    input_timeout = delay;
}

nc_color nc_color::from_color_pair_index( const int index )
{
    return nc_color( COLOR_PAIR( index ) );
}

int nc_color::to_color_pair_index() const
{
    return PAIR_NUMBER( attribute_value );
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
