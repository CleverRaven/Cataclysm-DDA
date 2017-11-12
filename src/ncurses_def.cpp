#if !(defined TILES || defined _WIN32 || defined WINDOWS)

#include "input.h"

#include "cursesdef.h"

#include "catacharset.h"

#include <stdexcept>

extern int VIEW_OFFSET_X; // X position of terrain window
extern int VIEW_OFFSET_Y; // Y position of terrain window

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

input_event input_manager::get_input_event()
{
    previously_pressed_key = 0;
    const long key = getch();
    if( key != ERR ) {
        long newch;
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
    } else if( key == KEY_MOUSE ) {
        MEVENT event;
        if( getmouse( &event ) == OK ) {
            rval.type = CATA_INPUT_MOUSE;
            rval.mouse_x = event.x - VIEW_OFFSET_X;
            rval.mouse_y = event.y - VIEW_OFFSET_Y;
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
        rval.text.append( 1, ( char ) key );
        // Read the UTF-8 sequence (if any)
        if( key < 127 ) {
            // Single byte sequence
        } else if( 194 <= key && key <= 223 ) {
            rval.text.append( 1, ( char ) getch() );
        } else if( 224 <= key && key <= 239 ) {
            rval.text.append( 1, ( char ) getch() );
            rval.text.append( 1, ( char ) getch() );
        } else if( 240 <= key && key <= 244 ) {
            rval.text.append( 1, ( char ) getch() );
            rval.text.append( 1, ( char ) getch() );
            rval.text.append( 1, ( char ) getch() );
        } else {
            // Other control character, etc. - no text at all, return an event
            // without the text property
            previously_pressed_key = key;
            return input_event( key, CATA_INPUT_KEYBOARD );
        }
        // Now we have loaded an UTF-8 sequence (possibly several bytes)
        // but we should only return *one* key, so return the code point of it.
        const char *utf8str = rval.text.c_str();
        int len = rval.text.length();
        const uint32_t cp = UTF8_getch( &utf8str, &len );
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

#endif
