#if !(defined TILES || defined _WIN32 || defined WINDOWS)

#include "cursesdef.h"

#include <stdexcept>

void init_interface()
{
    if( initscr() == nullptr ) {
        throw std::runtime_error( "initscr failed" );
    }
#if !(defined __CYGWIN__)
    // ncurses mouse registration
    mousemask( BUTTON1_CLICKED | BUTTON3_CLICKED | REPORT_MOUSE_POSITION, NULL );
#endif
    // our curses wrapper does not support changing this behavior, ncurses must
    // behave exactly like the wrapper, therefor:
    noecho();  // Don't echo keypresses
    cbreak();  // C-style breaks (e.g. ^C to SIGINT)
    keypad( stdscr, true ); // Numpad is numbers
    set_escdelay( 10 ); // Make escape actually responsive
}

#endif
