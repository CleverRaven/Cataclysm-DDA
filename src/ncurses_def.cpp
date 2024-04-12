#if !(defined(TILES) || defined(_WIN32))

// input.h must be include *before* the ncurses header. The latter has some macro
// defines that clash with the constants defined in input.h (e.g. KEY_UP).
#include "input.h"
#include "point.h"
#include "translations.h"
#include "cata_imgui.h"

// ncurses can define some functions as macros, but we need those identifiers
// to be unchanged by the preprocessor, as we use them as function names.
#define NCURSES_NOMACROS
#if !defined(__APPLE__)
#define NCURSES_WIDECHAR 1
#endif
#if defined(__CYGWIN__)
#include <ncurses/curses.h>
#else
#include <curses.h>
#endif

#include <cstdint>
#include <cstring>
#include <iosfwd>
#include <langinfo.h>
#include <memory>
#include <stdexcept>

#include "cached_options.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "color.h"
#include "cursesdef.h"
#include "game_constants.h"
#include "game_ui.h"
#include "output.h"
#include "ui_manager.h"

std::unique_ptr<cataimgui::client> imclient;

static void curses_check_result( const int result, const int expected, const char *const /*name*/ )
{
    if( result != expected ) {
        // TODO: debug message
    }
}

int get_window_width()
{
    return TERMX;
}

int get_window_height()
{
    return TERMY;
}

catacurses::window catacurses::newwin( const int nlines, const int ncols, const point &begin )
{
    // TODO: check for errors
    const auto w = ::newwin( nlines, ncols, begin.y, begin.x );
    return catacurses::window( std::shared_ptr<void>( w, []( void *const w ) {
        ::curses_check_result( ::delwin( static_cast<::WINDOW *>( w ) ), OK, "delwin" );
    } ) );
}

void catacurses::wnoutrefresh( const window &win )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::wnoutrefresh( win.get<::WINDOW>() ), OK, "wnoutrefresh" );
}

void catacurses::wrefresh( const window &win )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::wrefresh( win.get<::WINDOW>() ), OK, "wrefresh" );
}

void catacurses::werase( const window &win )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::werase( win.get<::WINDOW>() ), OK, "werase" );
}

int catacurses::getmaxx( const window &win )
{
    if( !win ) {
        return 0;
    }
    return ::getmaxx( win.get<::WINDOW>() );
}

int catacurses::getmaxy( const window &win )
{
    if( !win ) {
        return 0;
    }
    return ::getmaxy( win.get<::WINDOW>() );
}

int catacurses::getbegx( const window &win )
{
    if( !win ) {
        return 0;
    }
    return ::getbegx( win.get<::WINDOW>() );
}

int catacurses::getbegy( const window &win )
{
    if( !win ) {
        return 0;
    }
    return ::getbegy( win.get<::WINDOW>() );
}

int catacurses::getcurx( const window &win )
{
    if( !win ) {
        return 0;
    }
    return ::getcurx( win.get<::WINDOW>() );
}

int catacurses::getcury( const window &win )
{
    if( !win ) {
        return 0;
    }
    return ::getcury( win.get<::WINDOW>() );
}

void catacurses::wattroff( const window &win, const nc_color attrs )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::wattroff( win.get<::WINDOW>(), attrs.to_int() ), OK, "wattroff" );
}

void catacurses::wattron( const window &win, const nc_color &attrs )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::wattron( win.get<::WINDOW>(), attrs.to_int() ), OK, "wattron" );
}

void catacurses::wmove( const window &win, const point &p )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::wmove( win.get<::WINDOW>(), p.y, p.x ), OK, "wmove" );
}

void catacurses::mvwprintw( const window &win, const point &p, const std::string &text )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::mvwprintw( win.get<::WINDOW>(), p.y, p.x, "%s", text.c_str() ),
                                OK, "mvwprintw" );
}

void catacurses::wprintw( const window &win, const std::string &text )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::wprintw( win.get<::WINDOW>(), "%s", text.c_str() ),
                                OK, "wprintw" );
}

void catacurses::refresh()
{
    return curses_check_result( ::refresh(), OK, "refresh" );
}

void refresh_display()
{
    catacurses::doupdate();
}

void catacurses::doupdate()
{
    return curses_check_result( ::doupdate(), OK, "doupdate" );
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
    if( !win ) {
        return;
    }
    return curses_check_result( ::wborder( win.get<::WINDOW>(), ls, rs, ts, bs, tl, tr, bl, br ), OK,
                                "wborder" );
}

void catacurses::mvwhline( const window &win, const point &p, const chtype ch, const int n )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::mvwhline( win.get<::WINDOW>(), p.y, p.x, ch, n ), OK,
                                "mvwhline" );
}

void catacurses::mvwvline( const window &win, const point &p, const chtype ch, const int n )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::mvwvline( win.get<::WINDOW>(), p.y, p.x, ch, n ), OK,
                                "mvwvline" );
}

void catacurses::mvwaddch( const window &win, const point &p, const chtype ch )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::mvwaddch( win.get<::WINDOW>(), p.y, p.x, ch ), OK, "mvwaddch" );
}

void catacurses::waddch( const window &win, const chtype ch )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::waddch( win.get<::WINDOW>(), ch ), OK, "waddch" );
}

void catacurses::wredrawln( const window &win, const int beg_line, const int num_lines )
{
    if( !win ) {
        return;
    }
    return curses_check_result( ::wredrawln( win.get<::WINDOW>(), beg_line, num_lines ), OK,
                                "wredrawln" );
}

void catacurses::wclear( const window &win )
{
    if( !win ) {
        return;
    }
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
    if( imclient ) {
        imclient->upload_color_pair( pair, static_cast<int>( f ), static_cast<int>( b ) );
    }
    return curses_check_result( ::init_pair( pair, static_cast<short>( f ), static_cast<short>( b ) ),
                                OK, "init_pair" );
}

catacurses::window catacurses::newscr;
catacurses::window catacurses::stdscr;

void catacurses::resizeterm()
{
    const int new_x = ::getmaxx( stdscr.get<::WINDOW>() );
    const int new_y = ::getmaxy( stdscr.get<::WINDOW>() );
    if( ::is_term_resized( new_x, new_y ) ) {
        game_ui::init_ui();
        ui_manager::screen_resized();
        catacurses::doupdate();
    }
}
// init_interface is defined in another cpp file, depending on build type:
// wincurse.cpp for Windows builds without SDL and sdltiles.cpp for SDL builds.
void catacurses::init_interface()
{
    // ::endwin will free the pointer returned by ::initscr
    stdscr = window( std::shared_ptr<void>( ::initscr(), []( void *const ) { } ) );
    if( !stdscr ) {
        throw std::runtime_error( "initscr failed" );
    }
    newscr = window( std::shared_ptr<void>( ::newscr, []( void *const ) { } ) );
    if( !newscr ) {
        throw std::runtime_error( "null newscr" );
    }
    // our curses wrapper does not support changing this behavior, ncurses must
    // behave exactly like the wrapper, therefore:
    noecho();  // Don't echo keypresses
    cbreak();  // C-style breaks (e.g. ^C to SIGINT)
    keypad( stdscr.get<::WINDOW>(), true ); // Numpad is numbers
    set_escdelay( 10 ); // Make Escape actually responsive
    // TODO: error checking
    start_color();
    imclient = std::make_unique<cataimgui::client>();
    init_colors();
#if !defined(__CYGWIN__)
    // ncurses mouse registration
    mouseinterval( 0 );
    mousemask( ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr );
    // Sometimes the TERM variable is not set properly and mousemask won't turn on mouse pointer events.
    // The below line tries to force the mouse pointer events to be turned on anyway. ImTui misbehaves without them.
    printf( "\033[?1003h\n" );
#endif
}

bool catacurses::supports_256_colors()
{
    return COLORS >= 256;
}

void input_manager::pump_events()
{
    if( test_mode ) {
        return;
    }

    // Handle all events, but ignore any keypress
    int key = ERR;
    bool resize = false;
    const int prev_timeout = input_timeout;
    set_timeout( 0 );
    do {
        key = getch();
        if( key == KEY_RESIZE ) {
            resize = true;
        }
    } while( key != ERR );
    set_timeout( prev_timeout );
    if( resize ) {
        catacurses::resizeterm();
    }

    previously_pressed_key = 0;
}

// there isn't a portable way to get raw key code on curses,
// ignoring preferred keyboard mode
input_event input_manager::get_input_event( const keyboard_mode /*preferred_keyboard_mode*/ )
{
    if( test_mode ) {
        // input should be skipped in caller's code
        throw std::runtime_error( "input_manager::get_input_event called in test mode" );
    }

    int key = ERR;
    input_event rval;
    do {
        previously_pressed_key = 0;
        // flush any output
        catacurses::doupdate();
        key = getch();
        if( key != ERR ) {
            int newch;
            // Clear the buffer of characters that match the one we're going to act on.
            const int prev_timeout = input_timeout;
            set_timeout( 0 );
            do {
                newch = wgetch( stdscr );
            } while( newch != ERR && newch == key );
            set_timeout( prev_timeout );
            // If we read a different character than the one we're going to act on, re-queue it.
            if( newch != ERR && newch != key ) {
                ungetch( newch );
            }
        }
        rval = input_event();
        if( key == ERR ) {
            if( input_timeout > 0 ) {
                rval.type = input_event_t::timeout;
            } else {
                rval.type = input_event_t::error;
            }
            // ncurses mouse handling
        } else if( key == KEY_RESIZE ) {
            catacurses::resizeterm();
        } else if( key == KEY_MOUSE ) {
            MEVENT event;
            if( getmouse( &event ) == OK ) {
                rval.type = input_event_t::mouse;
                rval.mouse_pos = point( event.x, event.y );
                if( event.bstate & BUTTON1_CLICKED || event.bstate & BUTTON1_RELEASED ) {
                    rval.add_input( MouseInput::LeftButtonReleased );
                } else if( event.bstate & BUTTON1_PRESSED ) {
                    rval.add_input( MouseInput::LeftButtonPressed );
                } else if( event.bstate & BUTTON3_CLICKED || event.bstate & BUTTON3_RELEASED ) {
                    rval.add_input( MouseInput::RightButtonReleased );
                    // If curses version is prepared for a 5-button mouse, enable mousewheel
#if defined(BUTTON5_PRESSED)
                } else if( event.bstate & BUTTON4_PRESSED ) {
                    rval.add_input( MouseInput::ScrollWheelUp );
                } else if( event.bstate & BUTTON5_PRESSED ) {
                    rval.add_input( MouseInput::ScrollWheelDown );
#endif
                } else {
                    rval.add_input( MouseInput::Move );
                    if( input_timeout > 0 ) {
                        // Mouse movement seems to clear ncurses timeout
                        set_timeout( input_timeout );
                    }
                }
            } else {
                rval.type = input_event_t::error;
            }
        } else {
            if( key == 127 ) { // == Unicode DELETE
                previously_pressed_key = KEY_BACKSPACE;
                return input_event( KEY_BACKSPACE, input_event_t::keyboard_char );
            }
            rval.type = input_event_t::keyboard_char;
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
                input_event tmp_event( key, input_event_t::keyboard_char );
                imclient->process_input( &tmp_event );
                return tmp_event;
            }
            // Now we have loaded an UTF-8 sequence (possibly several bytes)
            // but we should only return *one* key, so return the code point of it.
            const uint32_t cp = UTF8_getch( rval.text );
            if( cp == UNKNOWN_UNICODE ) {
                // Invalid UTF-8 sequence, this should never happen, what now?
                // Maybe return any error instead?
                previously_pressed_key = key;
                return input_event( key, input_event_t::keyboard_char );
            }
            previously_pressed_key = cp;
            // for compatibility only add the first byte, not the code point
            // as it would  conflict with the special keys defined by ncurses
            rval.add_input( key );
        }
        imclient->process_input( &rval );
    } while( key == KEY_RESIZE );

    return rval;
}

void input_manager::set_timeout( const int delay )
{
    timeout( delay );
    // Use this to determine when curses should return a input_event_t::timeout event.
    input_timeout = delay;
}

nc_color nc_color::from_color_pair_index( const int index )
{
    return nc_color( COLOR_PAIR( index ), index );
}

int nc_color::to_color_pair_index() const
{
    return ( attribute_value & 0x03fe0000 ) >> 17;
}

nc_color nc_color::bold() const
{
    return nc_color( attribute_value | A_BOLD, index );
}

bool nc_color::is_bold() const
{
    return attribute_value & A_BOLD;
}

nc_color nc_color::blink() const
{
    return nc_color( attribute_value | A_BLINK, index );
}

bool nc_color::is_blink() const
{
    return attribute_value & A_BLINK;
}

void ensure_term_size(); // NOLINT(cata-static-declarations)
void check_encoding(); // NOLINT(cata-static-declarations)

void ensure_term_size()
{
    // do not use ui_adaptor here to avoid re-entry
    const int minHeight = EVEN_MINIMUM_TERM_HEIGHT;
    const int minWidth = EVEN_MINIMUM_TERM_WIDTH;

    while( TERMY < minHeight || TERMX < minWidth ) {
        catacurses::erase();
        if( TERMY < minHeight && TERMX < minWidth ) {
            fold_and_print( catacurses::stdscr, point_zero, TERMX, c_white,
                            _( "Whoa!  Your terminal is tiny!  This game requires a minimum terminal size of "
                               "%dx%d to work properly.  %dx%d just won't do.  Maybe a smaller font would help?" ),
                            minWidth, minHeight, TERMX, TERMY );
        } else if( TERMX < minWidth ) {
            fold_and_print( catacurses::stdscr, point_zero, TERMX, c_white,
                            _( "Oh!  Hey, look at that.  Your terminal is just a little too narrow.  This game "
                               "requires a minimum terminal size of %dx%d to function.  It just won't work "
                               "with only %dx%d.  Can you stretch it out sideways a bit?" ),
                            minWidth, minHeight, TERMX, TERMY );
        } else {
            fold_and_print( catacurses::stdscr, point_zero, TERMX, c_white,
                            _( "Woah, woah, we're just a little short on space here.  The game requires a "
                               "minimum terminal size of %dx%d to run.  %dx%d isn't quite enough!  Can you "
                               "make the terminal just a smidgen taller?" ),
                            minWidth, minHeight, TERMX, TERMY );
        }
        catacurses::refresh();
        // do not use input_manager or input_context here to avoid re-entry
        getch();
        TERMY = getmaxy( catacurses::stdscr );
        TERMX = getmaxx( catacurses::stdscr );
    }
}

// NOLINTNEXTLINE(cata-static-declarations)
void check_encoding()
{
    // Check whether LC_CTYPE supports the UTF-8 encoding
    // and show a warning if it doesn't
    if( std::strcmp( nl_langinfo( CODESET ), "UTF-8" ) != 0 ) {
        // do not use ui_adaptor here to avoid re-entry
        int key = ERR;
        do {
            const char *unicode_error_msg =
                _( "You don't seem to have a valid Unicode locale. You may see some weird "
                   "characters (e.g. empty boxes or question marks). You have been warned." );
            catacurses::erase();
            const int maxx = getmaxx( catacurses::stdscr );
            fold_and_print( catacurses::stdscr, point_zero, maxx, c_white, unicode_error_msg );
            catacurses::refresh();
            // do not use input_manager or input_context here to avoid re-entry
            key = getch();
        } while( key == KEY_RESIZE || key == KEY_MOUSE );
    }
}

void set_title( const std::string & )
{
    // curses does not seem to have a portable way of setting the window title.
}

#endif
