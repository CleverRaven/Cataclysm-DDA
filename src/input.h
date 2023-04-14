#pragma once
#ifndef CATA_SRC_INPUT_H
#define CATA_SRC_INPUT_H

#include <functional>
#include <iosfwd>
#include <map>
#include <set>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include <algorithm>
#include <list>
#endif

#include "point.h"
#include "translations.h"

enum action_id : int;
class cata_path;
class hotkey_queue;

namespace cata
{
template<typename T>
class optional;
} // namespace cata
namespace catacurses
{
class window;
} // namespace catacurses

// Curses key constants
constexpr int KEY_ESCAPE     = 27;
constexpr int KEY_MIN        =
    0x101;    /* minimum extended key value */ //<---------not used
constexpr int KEY_BREAK      =
    0x101;    /* break key */                  //<---------not used
constexpr int KEY_DOWN       = 0x102;    /* down arrow */
constexpr int KEY_UP         = 0x103;    /* up arrow */
constexpr int KEY_LEFT       = 0x104;    /* left arrow */
constexpr int KEY_RIGHT      = 0x105;    /* right arrow*/
constexpr int KEY_HOME       =
    0x106;    /* home key */
constexpr int KEY_BACKSPACE  =
    0x107;    /* Backspace */                  //<---------not used
constexpr int KEY_DC         = 0x14A;    /* Delete Character */
constexpr int KEY_F0         = 0x108;
inline constexpr int KEY_F( const int n )
{
    return KEY_F0 + n;    /* F1, F2, etc*/
}
constexpr int F_KEY_NUM_BEG  = 0;
constexpr int F_KEY_NUM_END  = 63;
inline constexpr int F_KEY_NUM( const int key )
{
    return key - KEY_F0;
}
inline constexpr bool IS_F_KEY( const int key )
{
    return key >= KEY_F( F_KEY_NUM_BEG ) && key <= KEY_F( F_KEY_NUM_END );
}
/** @return true if the given character is in the range of basic ASCII control characters */
inline constexpr bool IS_CTRL_CHAR( const int key )
{
    // https://en.wikipedia.org/wiki/C0_and_C1_control_codes#Basic_ASCII_control_codes
    return key >= 0 && key < ' ';
}
/** @return true if the given character is an ASCII control char but should not be rendered with "CTRL+" */
inline constexpr bool IS_NAMED_CTRL_CHAR( const int key )
{
    return key == '\t' || key == '\n' || key == KEY_ESCAPE || key == KEY_BACKSPACE;
}
inline constexpr int KEY_NUM( const int n )
{
    return 0x30 + n;     /* Numbers 0, 1, ..., 9 */
}
constexpr int KEY_NPAGE      = 0x152;    /* page down */
constexpr int KEY_PPAGE      = 0x153;    /* page up */
constexpr int KEY_ENTER      = 0x157;    /* enter */
constexpr int KEY_BTAB       = 0x161;    /* back-tab = shift + tab */
constexpr int KEY_END        = 0x168;    /* End */

// Platform independent key code (though largely based on SDL key code)
//
// These and other code related to keyboard_code event should NOT be guarded with
// platform macros, since they are required to load and save keyboard_code events,
// which should always be done regardless whether the platform supports it, otherwise
// custom keybindings might be lost when someone switches e.g. from tiles to curses
// and save the keybindings, and switches back to tiles later.
namespace keycode
{
enum : int {
    backspace = 0x08,
    tab       = 0x09,
    return_   = 0x0D,
    escape    = 0x1B,
    space     = 0x20,
    f1        = 0x4000003A,
    f2        = 0x4000003B,
    f3        = 0x4000003C,
    f4        = 0x4000003D,
    f5        = 0x4000003E,
    f6        = 0x4000003F,
    f7        = 0x40000040,
    f8        = 0x40000041,
    f9        = 0x40000042,
    f10       = 0x40000043,
    f11       = 0x40000044,
    f12       = 0x40000045,
    home      = 0x4000004A,
    ppage     = 0x4000004B,
    end       = 0x4000004D,
    npage     = 0x4000004E,
    right     = 0x4000004F,
    left      = 0x40000050,
    down      = 0x40000051,
    up        = 0x40000052,
    kp_divide = 0x40000054,
    kp_multiply = 0x40000055,
    kp_minus  = 0x40000056,
    kp_plus   = 0x40000057,
    kp_enter  = 0x40000058,
    kp_1      = 0x40000059,
    kp_2      = 0x4000005A,
    kp_3      = 0x4000005B,
    kp_4      = 0x4000005C,
    kp_5      = 0x4000005D,
    kp_6      = 0x4000005E,
    kp_7      = 0x4000005F,
    kp_8      = 0x40000060,
    kp_9      = 0x40000061,
    kp_0      = 0x40000062,
    kp_period = 0x40000063,
    f13       = 0x40000068,
    f14       = 0x40000069,
    f15       = 0x4000006A,
    f16       = 0x4000006B,
    f17       = 0x4000006C,
    f18       = 0x4000006D,
    f19       = 0x4000006E,
    f20       = 0x4000006F,
    f21       = 0x40000070,
    f22       = 0x40000071,
    f23       = 0x40000072,
    f24       = 0x40000073,
};
} // namespace keycode

constexpr int LEGEND_HEIGHT = 8;
constexpr int BORDER_SPACE = 2;

bool is_mouse_enabled();
bool is_keycode_mode_supported();
std::string get_input_string_from_file( const std::string &fname = "input.txt" );

// Mouse buttons and movement input
enum class MouseInput : int {

    LeftButtonPressed = 1,
    LeftButtonReleased,

    RightButtonPressed,
    RightButtonReleased,

    ScrollWheelUp,
    ScrollWheelDown,

    Move

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

/**
 * A set of attributes for an action
 */
struct action_attributes {
    action_attributes() : is_user_created( false ) {}
    bool is_user_created;
    translation name;
    std::vector<input_event> input_events;
};

// Definitions for joystick/gamepad.

// On the joystick there's a maximum of 256 key states.
// So for joy axis events, we simply use a number larger
// than that.
constexpr int JOY_0  = 0;
constexpr int JOY_1  = 1;
constexpr int JOY_2  = 2;
constexpr int JOY_3  = 3;
constexpr int JOY_4  = 4;
constexpr int JOY_5  = 5;
constexpr int JOY_6  = 6;
constexpr int JOY_7  = 7;
constexpr int JOY_8  = 8;
constexpr int JOY_9  = 9;
constexpr int JOY_10 = 10;
constexpr int JOY_11 = 11;
constexpr int JOY_12 = 12;
constexpr int JOY_13 = 13;
constexpr int JOY_14 = 14;
constexpr int JOY_15 = 15;
constexpr int JOY_16 = 16;
constexpr int JOY_17 = 17;
constexpr int JOY_18 = 18;
constexpr int JOY_19 = 19;
constexpr int JOY_20 = 20;
constexpr int JOY_21 = 21;
constexpr int JOY_22 = 22;
constexpr int JOY_23 = 23;
constexpr int JOY_24 = 24;
constexpr int JOY_25 = 25;
constexpr int JOY_26 = 26;
constexpr int JOY_27 = 27;
constexpr int JOY_28 = 28;
constexpr int JOY_29 = 29;
constexpr int JOY_30 = 30;

constexpr int JOY_LEFT      = 256 + 1;
constexpr int JOY_RIGHT     = 256 + 2;
constexpr int JOY_UP        = 256 + 3;
constexpr int JOY_DOWN      = 256 + 4;
constexpr int JOY_RIGHTUP   = 256 + 5;
constexpr int JOY_RIGHTDOWN = 256 + 6;
constexpr int JOY_LEFTUP    = 256 + 7;
constexpr int JOY_LEFTDOWN  = 256 + 8;

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

/**
 * Manages the translation from action IDs to associated input.
 *
 * Planned methods of input:
 * 1. Single key press: a
 * 2. Multi-key combination: `a
 * 3. Gamepad button: A
 */
class input_manager
{
    public:
        // TODO: rewrite this to have several alternative input events for the same action

        /**
         * Get the input events associated with an action ID in a given context.
         *
         * @param action_descriptor The action ID to get the input events for.
         * @param context The context in which to get the input events. Defaults to "default".
         * @param overwrites_default If this is non-NULL, this will be used as return parameter and will be set to true if the default
         *                           keybinding is overridden by something else in the given context.
         */
        const std::vector<input_event> &get_input_for_action( const std::string &action_descriptor,
                const std::string &context = "default", bool *overwrites_default = nullptr );

        /**
         * Return first char associated with an action ID in a given context.
         */
        int get_first_char_for_action( const std::string &action_descriptor,
                                       const std::string &context = "default" );

        /**
         * Initializes the input manager, aka loads the input mapping configuration JSON.
         */
        void init();
        /**
         * Opposite of @ref init, save the data that has been loaded by @ref init,
         * and possibly been modified.
         */
        void save();

        /**
         * Return the previously pressed key, or 0 if there is no previous input
         * or the previous input wasn't a key.
         */
        int get_previously_pressed_key() const;

        /**
         * Get the keycode associated with the given key name.
         */
        int get_keycode( input_event_t inp_type, const std::string &name ) const;

        /**
         * Get the key name associated with the given keyboard keycode.
         *
         * @param ch Character code.
         * @param inp_type Whether the keycode is a gamepad or a keyboard code.
         * @param portable If true, return a language independent and portable name
         * of the key. This acts as the inverse to get_keyname:
         * <code>get_keyname(get_keycode(a), , true) == a</code>
         */
        std::string get_keyname( int ch, input_event_t inp_type, bool portable = false ) const;

        /**
         * curses getch() replacement.
         *
         * Defined in the respective platform wrapper, e.g. sdlcurse.cpp
         */
        input_event get_input_event( keyboard_mode preferred_keyboard_mode = keyboard_mode::keycode );
        /**
         * Resize & refresh if necessary, process all pending window events, and ignore keypresses
         */
        void pump_events();

        /**
         * Wait until the user presses a key. Mouse and similar input is ignored,
         * only input events from the keyboard are considered.
         */
        void wait_for_any_key();

        /**
         * Sets global input polling timeout as appropriate for the current interface system.
         * Use `input_context::(re)set_timeout()` when possible so timeout will be properly
         * reset when entering a new input context.
         */
        void set_timeout( int delay );
        void reset_timeout() {
            set_timeout( -1 );
        }
        int get_timeout() const {
            return input_timeout;
        }

        static keyboard_mode actual_keyboard_mode( keyboard_mode preferred_keyboard_mode );

    private:
        friend class input_context;

        using t_input_event_list = std::vector<input_event>;
        using t_actions = std::map<std::string, action_attributes>;
        using t_action_contexts = std::map<std::string, t_actions>;
        t_action_contexts action_contexts;

        using t_key_to_name_map = std::map<int, std::string>;
        t_key_to_name_map keyboard_char_keycode_to_keyname;
        t_key_to_name_map keyboard_code_keycode_to_keyname;
        t_key_to_name_map gamepad_keycode_to_keyname;
        t_key_to_name_map mouse_keycode_to_keyname;
        using t_name_to_key_map = std::map<std::string, int>;
        t_name_to_key_map keyboard_char_keyname_to_keycode;
        t_name_to_key_map keyboard_code_keyname_to_keycode;
        t_name_to_key_map gamepad_keyname_to_keycode;
        t_name_to_key_map mouse_keyname_to_keycode;

        // See @ref get_previously_pressed_key
        int previously_pressed_key;

        // Maps the key names we see in keybindings.json and in-game to
        // the keycode integers.
        void init_keycode_mapping();
        void add_keyboard_char_keycode_pair( int ch, const std::string &name );
        void add_keyboard_code_keycode_pair( int ch, const std::string &name );
        void add_gamepad_keycode_pair( int ch, const std::string &name );
        void add_mouse_keycode_pair( MouseInput mouse_input, const std::string &name );
        /**
         * Load keybindings from a json file, override existing bindings.
         * Throws std::string on errors
         */
        void load( const cata_path &file_name, bool is_user_preferences );

        int input_timeout;

        t_input_event_list &get_or_create_event_list( const std::string &action_descriptor,
                const std::string &context );
        void remove_input_for_action( const std::string &action_descriptor, const std::string &context );
        void add_input_for_action( const std::string &action_descriptor, const std::string &context,
                                   const input_event &event );

        /**
         * Get the attributes of the action associated with an action ID by
         * searching the given context and the default context.
         *
         * @param action_id The action ID of the action to find.
         * @param context The context in which to get the action. If not found,
         *                the "default" context will additionally be checked.
         *                Defaults to "default".
         * @param overwrites_default If this is non-NULL, this will be used as a
         *                           return parameter. It will be set to true if
         *                           the found action was not in the default
         *                           context. It will be set to false if the found
         *                           action was in the default context.
         */
        const action_attributes &get_action_attributes(
            const std::string &action_id,
            const std::string &context = "default",
            bool *overwrites_default = nullptr );

        /**
         * Get a value to be used as the default name for a newly created action.
         * This name should be used as a fallback in cases where it is necessary
         * to create a new action.
         *
         * @param action_id The action ID of the action.
         *
         * @return If the action ID exists in the default context, the name of
         *         that action's name is returned. Otherwise, no_translation( action_id ) is
         *         returned.
         */
        translation get_default_action_name( const std::string &action_id ) const;
};

// Singleton for our input manager.
extern input_manager inp_mngr;

/**
 * Represents a context in which a set of actions can be performed.
 *
 * This class is responsible for registering possible actions
 * (traditionally keypresses), handling input, and yielding the correct
 * action string descriptors for given input.
 *
 * This turns this class into an abstraction method between actual
 * input(keyboard, gamepad etc.) and game.
 */
class input_context
{
    public:
#if defined(__ANDROID__)
        // Whatever's on top is our current input context.
        static std::list<input_context *> input_context_stack;
#endif

        input_context() : registered_any_input( false ), category( "default" ),
            coordinate_input_received( false ), handling_coordinate_input( false ) {
#if defined(__ANDROID__)
            input_context_stack.push_back( this );
            allow_text_entry = false;
#endif
        }
        // TODO: consider making the curses WINDOW an argument to the constructor, so that mouse input
        // outside that window can be ignored
        explicit input_context( const std::string &category,
                                const keyboard_mode preferred_keyboard_mode = keyboard_mode::keycode )
            : registered_any_input( false ), category( category ),
              coordinate_input_received( false ), handling_coordinate_input( false ),
              preferred_keyboard_mode( preferred_keyboard_mode ) {
#if defined(__ANDROID__)
            input_context_stack.push_back( this );
            allow_text_entry = false;
#endif
        }

#if defined(__ANDROID__)
        virtual ~input_context() {
            input_context_stack.remove( this );
        }

        // HACK: hack to allow creating manual keybindings for getch() instances, uilists etc. that don't use an input_context outside of the Android version
        struct manual_key {
            manual_key( int _key, const std::string &_text ) : key( _key ), text( _text ) {}
            int key;
            std::string text;
            bool operator==( const manual_key &other ) const {
                return key == other.key && text == other.text;
            }
        };

        std::vector<manual_key> registered_manual_keys;

        // If true, prevent virtual keyboard from dismissing after a key press while this input context is active.
        // NOTE: This won't auto-bring up the virtual keyboard, for that use sdltiles.cpp is_string_input()
        bool allow_text_entry;

        void register_manual_key( manual_key mk );
        void register_manual_key( int key, const std::string text = "" );

        std::string get_action_name_for_manual_key( int key ) {
            for( const auto &manual_key : registered_manual_keys ) {
                if( manual_key.key == key ) {
                    return manual_key.text;
                }
            }
            return std::string();
        }
        std::vector<manual_key> &get_registered_manual_keys() {
            return registered_manual_keys;
        }

        std::string &get_category() {
            return category;
        }
        std::vector<std::string> &get_registered_actions() {
            return registered_actions;
        }
        bool is_action_registered( const std::string &action_descriptor ) const {
            return std::find( registered_actions.begin(), registered_actions.end(),
                              action_descriptor ) != registered_actions.end();
        }

        input_context &operator=( const input_context &other ) {
            registered_actions = other.registered_actions;
            registered_manual_keys = other.registered_manual_keys;
            allow_text_entry = other.allow_text_entry;
            registered_any_input = other.registered_any_input;
            category = other.category;
            coordinate = other.coordinate;
            coordinate_input_received = other.coordinate_input_received;
            handling_coordinate_input = other.handling_coordinate_input;
            next_action = other.next_action;
            iso_mode = other.iso_mode;
            action_name_overrides = other.action_name_overrides;
            timeout = other.timeout;
            return *this;
        }

        bool operator==( const input_context &other ) const {
            return category == other.category &&
                   registered_actions == other.registered_actions &&
                   registered_manual_keys == other.registered_manual_keys &&
                   allow_text_entry == other.allow_text_entry &&
                   registered_any_input == other.registered_any_input &&
                   coordinate == other.coordinate &&
                   coordinate_input_received == other.coordinate_input_received &&
                   handling_coordinate_input == other.handling_coordinate_input &&
                   next_action == other.next_action &&
                   iso_mode == other.iso_mode &&
                   action_name_overrides == other.action_name_overrides &&
                   timeout == other.timeout;
        }
        bool operator!=( const input_context &other ) const {
            return !( *this == other );
        }
#endif

        /**
         * Register an action with this input context.
         *
         * Only registered actions will be returned by `handle_input()`, it's
         * thus possible to have multiple actions associated with the same keypress,
         * as long as they don't ever occur in the same input context.
         *
         * If `action_descriptor` is the special "ANY_INPUT", instead of ignoring
         * unregistered keys, those keys will all be linked to this "ANY_INPUT"
         * action.
         *
         * If `action_descriptor` is the special "COORDINATE", coordinate input will be processed
         * and the specified coordinates can be retrieved using `get_coordinates()`. Currently the
         * only form of coordinate input is mouse input(you can directly click coordinates on
         * the screen).
         *
         * @param action_descriptor String of action id.
         */
        void register_action( const std::string &action_descriptor );

        /**
         * Same as other @ref register_action function but allows a context specific
         * action name. The given name is displayed instead of the name taken from
         * the @ref input_manager.
         *
         * @param action_descriptor String of action id.
         * @param name Name of the action, displayed to the user. If empty use the
         * name reported by the input_manager.
         */
        void register_action( const std::string &action_descriptor, const translation &name );

        /**
         * Get the set of available single character keyboard keys that do not
         * conflict with any registered hotkeys.  The result will only include
         * characters from the requested_keys parameter that have no conflicts
         * i.e. the set difference requested_keys - conflicts.
         *
         * @param requested_keys The set of single character hotkeys to
         *                       potentially use. Defaults to all printable ascii.
         *
         * @return Returns the set of non-conflicting, single character keyboard
         *         keys suitable for use as hotkeys.
         */
        std::string get_available_single_char_hotkeys(
            std::string requested_keys =
                "abcdefghijkpqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-=:;'\",./<>?!@#$%^&*()_+[]\\{}|`~" );

        using input_event_filter = std::function<bool( const input_event & )>;

        // Helper functions to be used as @ref input_event_filter
        static bool disallow_lower_case_or_non_modified_letters( const input_event &evt );
        static bool allow_all_keys( const input_event &evt );

        /**
         * Get a description text for the key/other input method associated
         * with the given action. If there are multiple bound keys, no more
         * than max_limit will be described in the result. In addition, only
         * keys satisfying evt_filter will be described.
         *
         * @param action_descriptor The action descriptor for which to return
         *                          a description of the bound keys.
         *
         * @param max_limit No more than max_limit bound keys will be
         *                  described in the returned description. A value of
         *                  0 indicates no limit.
         *
         * @param evt_filter Only keys satisfying this function will be
         *                   described.
         */
        std::string get_desc( const std::string &action_descriptor,
                              unsigned int max_limit = 0,
                              const input_event_filter &evt_filter = allow_all_keys ) const;

        /**
         * Get a description based on `text`. If a bound key for `action_descriptor`
         * satisfying `evt_filter` is contained in `text`, surround the key with
         * brackets and change the case if necessary (e.g. "(Y)es"). Otherwise
         * prefix `text` with description of the first bound key satisfying
         * `evt_filter`, surrounded in square brackets (e.g "[RETURN] Yes").
         *
         * @param action_descriptor The action descriptor for which to return
         *                          a description of the bound keys.
         *
         * @param text The base text for action description
         *
         * @param evt_filter Only keys satisfying this function will be considered
         * @param inline_fmt Action description format when a key is found in the
         *                   text (for example "(a)ctive")
         * @param separate_fmt Action description format when a key is not found
         *                     in the text (for example "[X] active" or "[N/A] active")
         */
        std::string get_desc( const std::string &action_descriptor,
                              const std::string &text,
                              const input_event_filter &evt_filter,
                              const translation &inline_fmt,
                              const translation &separate_fmt ) const;
        std::string get_desc( const std::string &action_descriptor,
                              const std::string &text,
                              const input_event_filter &evt_filter = allow_all_keys ) const;

        /**
         * Equivalent to get_desc( act, get_action_name( act ), filter )
         **/
        std::string describe_key_and_name( const std::string &action_descriptor,
                                           const input_event_filter &evt_filter = allow_all_keys ) const;

        /**
         * Handles input and returns the next action in the queue.
         *
         * This internally calls getch() or whatever other input method
         * is available(e.g. gamepad).
         *
         * If the action is mouse input, returns "MOUSE".
         *
         * @return One of the input actions formerly registered with
         *         `register_action()`, or "ERROR" if an error happened.
         *
         */
        const std::string &handle_input();
        const std::string &handle_input( int timeout );

        /**
         * Convert a direction action (UP, DOWN etc) to a delta vector.
         *
         * @return If the action is a movement action (UP, DOWN, ...),
         * the delta vector associated with it. Otherwise returns an empty value.
         * The returned vector will always have a z component of 0.
         */
        std::optional<tripoint> get_direction( const std::string &action ) const;

        /**
         * Get the coordinates associated with the last mouse click (if any).
         */
        std::optional<tripoint> get_coordinates( const catacurses::window &capture_win_,
                const point &offset = point_zero, bool center_cursor = false ) const;

        // Below here are shortcuts for registering common key combinations.
        void register_directions();
        void register_updown();
        void register_leftright();
        void register_cardinal();
        void register_navigate_ui_list();

        /**
         * Displays the possible actions in the current context and their
         * keybindings.
         * @param permit_execute_action If `true` the function allows the user to specify an action to execute
         * @returns action_id of any action the user specified to execute, or ACTION_NULL if none
         */
        action_id display_menu( bool permit_execute_action = false );

        /**
         * Temporary method to retrieve the raw input received, so that input_contexts
         * can be used in screens where not all possible actions have been defined in
         * keybindings.json yet.
         */
        input_event get_raw_input();

        /**
         * Get coordinate of text level from mouse input, difference between this and get_coordinates is that one is getting pixel level coordinate.
         */
        std::optional<point> get_coordinates_text( const catacurses::window &capture_win ) const;

        /**
         * Get the human-readable name for an action.
         */
        std::string get_action_name( const std::string &action_id ) const;

        /* For the future, something like this might be nice:
         * const std::string register_action(const std::string& action_descriptor, x, y, width, height);
         * (x, y, width, height) would describe an area on the visible window that, if clicked, triggers the action.
         */

        // (Press X (or Y)|Try) to Z
        std::string press_x( const std::string &action_id ) const;
        std::string press_x( const std::string &action_id, const std::string &key_bound,
                             const std::string &key_unbound ) const;
        std::string press_x( const std::string &action_id, const std::string &key_bound_pre,
                             const std::string &key_bound_suf, const std::string &key_unbound ) const;

        /**
         * Keys (and only keys, other input types are not included) that
         * trigger the given action.
         * @param action_descriptor The action descriptor for which to get the bound keys.
         * @param maximum_modifier_count Maximum number of modifiers allowed for
         *        the returned action. <0 means any number is allowed.
         * @param restrict_to_printable If `true`, the function returns the bound
         *        keys only if they are printable (space counts as non-printable
         *        here). If `false`, all keys (whether they are printable or not)
         *        are returned.
         * @param restrict_to_keyboard If `true`, the function returns the bound keys only
         *        if they are keyboard inputs. If `false`, all inputs, such as mouse
         *        inputs are included.
         * @returns All keys bound to the given action descriptor.
         */
        std::vector<input_event> keys_bound_to( const std::string &action_descriptor,
                                                int maximum_modifier_count = -1,
                                                bool restrict_to_printable = true,
                                                bool restrict_to_keyboard = true ) const;

        /**
        * Get/Set edittext to display IME unspecified string.
        */
        void set_edittext( const std::string &s );
        std::string get_edittext();

        void set_iso( bool mode = true );

        /**
         * Sets input polling timeout as appropriate for the current interface system.
         * Use this method to set timeouts when using input_context, rather than calling
         * the old timeout() method or using input_manager::(re)set_timeout, as using
         * this method will cause input_event_t::timeout events to be generated correctly,
         * and will reset timeout correctly when a new input context is entered.
         */
        void set_timeout( int val );
        void reset_timeout();

        input_event first_unassigned_hotkey( const hotkey_queue &queue ) const;
        input_event next_unassigned_hotkey( const hotkey_queue &queue, const input_event &prev ) const;
    private:

        std::vector<std::string> registered_actions;
        std::string edittext;
    public:
        const std::string &input_to_action( const input_event &inp ) const;
        bool is_event_type_enabled( input_event_t type ) const;
        bool is_registered_action( const std::string &action_name ) const;
    private:
        bool registered_any_input;
        std::string category; // The input category this context uses.
        point coordinate;
        bool coordinate_input_received;
        bool handling_coordinate_input;
        input_event next_action;
        bool iso_mode = false; // should this context follow the game's isometric settings?
        int timeout = -1;
        keyboard_mode preferred_keyboard_mode = keyboard_mode::keycode;

        /**
         * When registering for actions within an input_context, callers can
         * specify a custom action name that will override the action's normal
         * name. This map stores those overrides. The key is the action ID and the
         * value is the user-visible name.
         */
        std::map<std::string, translation> action_name_overrides;

        /**
         * Returns whether action uses the specified input
         */
        bool action_uses_input( const std::string &action_id, const input_event &event ) const;
        /**
         * Return a user presentable list of actions that conflict with the
         * proposed keybinding. Returns an empty string if nothing conflicts.
         */
        std::string get_conflicts( const input_event &event, const std::string &ignore_action ) const;
        /**
         * Clear an input_event from all conflicting keybindings that are
         * registered by this input_context.
         *
         * @param event The input event to be cleared from conflicting
         * keybindings.
         */
        void clear_conflicting_keybindings( const input_event &event );
        /**
         * Filter a vector of strings by a phrase, returning only strings that contain the phrase.
         *
         * @param strings The vector of strings to filter
         * @param phrase  The phrase to search within each of the given strings
         * @return A vector of the filtered strings
         */
        std::vector<std::string> filter_strings_by_phrase( const std::vector<std::string> &strings,
                const std::string &phrase ) const;
};

/**
 * Check whether a gamepad is plugged in/available.
 *
 * Always false in non-SDL versions.
 */
bool gamepad_available();

// rotate a delta direction clockwise
void rotate_direction_cw( int &dx, int &dy );

class hotkey_queue
{
    public:
        // ctxt is only used for determining hotkey input type
        // use input_context::first_unassigned_hotkey() instead to skip assigned actions
        input_event first( const input_context &ctxt ) const;
        // use input_context::next_unassigned_hotkey() instead to skip assigned actions
        input_event next( const input_event &prev ) const;

        /**
         * In keychar mode:
         *   a-z, A-Z
         * In keycode mode:
         *   a-z, shift a-z
         */
        static const hotkey_queue &alphabets();

        /**
         * In keychar mode:
         *   1-0, a-z, A-Z
         * In keycode mode:
         *   1-0, a-z, shift 1-0, shift a-z
         */
        static const hotkey_queue &alpha_digits();

    private:
        std::vector<int> codes_keychar;
        std::vector<int> codes_keycode;
        std::vector<std::set<keymod_t>> modifiers_keycode;
};

#endif // CATA_SRC_INPUT_H
