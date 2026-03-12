#pragma once

#include <set>
#include <string>
#include <vector>

#include "point.h"

/**
 * These are comonalities used in input*.h
 */

enum action_id : int;


// Mouse buttons and movement input
enum class MouseInput : int {

    LeftButtonPressed = 1,
    LeftButtonReleased,

    RightButtonPressed,
    RightButtonReleased,

    ScrollWheelUp,
    ScrollWheelDown,

    Move,

    X1ButtonPressed,
    X1ButtonReleased,

    X2ButtonPressed,
    X2ButtonReleased

};

enum class input_event_t : int  {
    error,
    timeout,
    // used on platforms with only character/text input, such as curses
    keyboard_char,
    // used on platforms with raw keycode input, such as non-android sdl
    keyboard_code,
    gamepad,
    mouse
};

enum class keymod_t {
    ctrl,
    alt,
    shift,
};

/**
 * An instance of an input, like a keypress etc.
 *
 * Gamepad, mouse and keyboard keypresses will be represented as `int`.
 * Whether a gamepad, mouse or keyboard was used can be checked using the
 * `type` member.
 *
 */
struct input_event {
    input_event_t type;

    std::set<keymod_t> modifiers; // Keys that need to be held down for
    // this event to be activated.

    std::vector<int> sequence; // The sequence of key or mouse events that
    // triggers this event. For single-key
    // events, simply make this of size 1.

    point mouse_pos;       // Mouse click co-ordinates, if applicable

    // Actually entered text (if any), UTF-8 encoded, might be empty if
    // the input is not UTF-8 or not even text.
    std::string text;
    std::string edit;
    bool edit_refresh;

#if defined(__ANDROID__)
    // Used exclusively by the quick shortcuts to determine how stale a shortcut is
    int shortcut_last_used_action_counter = 0;
#endif

    input_event() : edit_refresh( false ) {
        type = input_event_t::error;
    }
    input_event( int s, input_event_t t )
        : type( t ), edit_refresh( false ) {
        sequence.push_back( s );
    }

    // overloaded function for a mouse input
    // made for a cleaner code, to get rid of static_cast's from scoped enum to int
    //
    // Instead of:
    //
    //    input_event( static_cast<int>( MouseInput::Move ), ... )
    //    input_event( static_cast<int>( MouseInput::LeftButtonPressed ), ... )
    //    input_event( static_cast<int>( MouseInput::RightButtonPressed ), ... )
    //
    // we now can just use
    //
    //    input_event( MouseInput::Move, ... )
    //    input_event( MouseInput::LeftButtonPressed, ... )
    //    input_event( MouseInput::RightButtonPressed, ... )
    //
    input_event( const MouseInput s, input_event_t t )
        : type( t ), edit_refresh( false ) {
        sequence.push_back( static_cast<int>( s ) );
    }

    input_event( const std::set<keymod_t> &mod, int s, input_event_t t );

    int get_first_input() const;

    void add_input( const int input ) {
        sequence.push_back( input );
    }

    // overloaded function for a mouse input
    // made for a cleaner code, to get rid of static_cast's from scoped enum to int
    //
    // Instead of:
    //
    //    add_input( static_cast<int>( MouseInput::Move ) )
    //    add_input( static_cast<int>( MouseInput::LeftButtonPressed ) )
    //    add_input( static_cast<int>( MouseInput::RightButtonPressed ) )
    //
    // we now can just use
    //
    //    add_input( MouseInput::Move )
    //    add_input( MouseInput::LeftButtonPressed )
    //    add_input( MouseInput::RightButtonPressed )
    //
    void add_input( const MouseInput mouse_input ) {
        sequence.push_back( static_cast<int>( mouse_input ) );
    }

    bool operator==( const input_event &other ) const {
        return type == other.type && modifiers == other.modifiers && sequence == other.sequence;
    }

    bool operator!=( const input_event &other ) const;

    std::string long_description() const;
    std::string short_description() const;

    /**
     * Lexicographical order considering input event type,
     * modifiers, and key code sequence.
     */
    static bool compare_type_mod_code( const input_event &lhs, const input_event &rhs );
};

enum class keyboard_mode {
    // Accept character input and text input. Input in this mode
    // may be manipulated by the system via IMEs or dead keys.
    keychar,
    // Accept raw key code input. Text input is not available in this
    // mode. All keyboard events are directly fed to the program
    // in this mode, bypassing IMEs and dead keys. Only supported on
    // some platforms, such as non-android SDL. On other platforms
    // this falls back to `keychar` automatically.
    keycode,
};
