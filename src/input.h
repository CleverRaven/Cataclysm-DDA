#pragma once
#ifndef CATA_SRC_INPUT_H
#define CATA_SRC_INPUT_H

#include <map>
#include <string>
#include <vector>

#include "input_enums.h"
#include "translation.h"

class cata_path;
class keybindings_ui;

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

bool is_mouse_enabled();
bool is_keycode_mode_supported();
std::string get_input_string_from_file( const std::string &fname = "input.txt" );

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
        friend class keybindings_ui;
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
         * Sets global input polling timeout in milliseconds as appropriate for the current interface system.
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
        /**
         * called basic rather than default to not confuse default context
         * basic means default keybindings (without user changes)
         */
        t_action_contexts basic_action_contexts;

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
        /**
         * @return true if `remove_input_for_action` uncovers potentially new global keys. These could cause conflicts.
         */
        bool remove_input_for_action( const std::string &action_descriptor, const std::string &context );
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
         * @param use_basic_action_contexts If true, use non-customized keybindings.
         */
        const action_attributes &get_action_attributes(
            const std::string &action_id,
            const std::string &context = "default",
            bool *overwrites_default = nullptr,
            bool use_basic_action_contexts = false );

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
 * Check whether a gamepad is plugged in/available.
 *
 * Always false in non-SDL versions.
 */
bool gamepad_available();

#endif // CATA_SRC_INPUT_H
