#pragma once
#ifndef INPUT_H
#define INPUT_H

#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <vector>

#if defined(__ANDROID__)
#include <list>
#include <algorithm>
#endif

#include "point.h"

namespace cata
{
template<typename T>
class optional;
} // namespace cata
namespace catacurses
{
class window;
} // namespace catacurses

static constexpr int KEY_ESCAPE     = 27;
static constexpr int KEY_MIN        =
    0x101;    /* minimum extended key value */ //<---------not used
static constexpr int KEY_BREAK      =
    0x101;    /* break key */                  //<---------not used
static constexpr int KEY_DOWN       = 0x102;    /* down arrow */
static constexpr int KEY_UP         = 0x103;    /* up arrow */
static constexpr int KEY_LEFT       = 0x104;    /* left arrow */
static constexpr int KEY_RIGHT      = 0x105;    /* right arrow*/
static constexpr int KEY_HOME       =
    0x106;    /* home key */                   //<---------not used
static constexpr int KEY_BACKSPACE  =
    0x107;    /* Backspace */                  //<---------not used
static constexpr int KEY_DC         = 0x151;    /* Delete Character */
inline constexpr int KEY_F( const int n )
{
    return 0x108 + n;    /* F1, F2, etc*/
}
inline constexpr int KEY_NUM( const int n )
{
    return 0x30 + n;     /* Numbers 0, 1, ..., 9 */
}
static constexpr int KEY_NPAGE      = 0x152;    /* page down */
static constexpr int KEY_PPAGE      = 0x153;    /* page up */
static constexpr int KEY_ENTER      = 0x157;    /* enter */
static constexpr int KEY_BTAB       = 0x161;    /* back-tab = shift + tab */
static constexpr int KEY_END        = 0x168;    /* End */

static constexpr int LEGEND_HEIGHT = 11;
static constexpr int BORDER_SPACE = 2;

bool is_mouse_enabled();
std::string get_input_string_from_file( const std::string &fname = "input.txt" );

enum mouse_buttons { MOUSE_BUTTON_LEFT = 1, MOUSE_BUTTON_RIGHT, SCROLLWHEEL_UP, SCROLLWHEEL_DOWN, MOUSE_MOVE };

enum input_event_t {
    CATA_INPUT_ERROR,
    CATA_INPUT_TIMEOUT,
    CATA_INPUT_KEYBOARD,
    CATA_INPUT_GAMEPAD,
    CATA_INPUT_MOUSE
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

    std::vector<int> modifiers; // Keys that need to be held down for
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
    int shortcut_last_used_action_counter;
#endif

    input_event() {
        type = CATA_INPUT_ERROR;
#if defined(__ANDROID__)
        shortcut_last_used_action_counter = 0;
#endif
    }
    input_event( int s, input_event_t t )
        : type( t ) {
        sequence.push_back( s );
#if defined(__ANDROID__)
        shortcut_last_used_action_counter = 0;
#endif
    }

    int get_first_input() const;

    void add_input( const int input ) {
        sequence.push_back( input );
    }

#if defined(__ANDROID__)
    input_event &operator=( const input_event &other ) {
        type = other.type;
        modifiers = other.modifiers;
        sequence = other.sequence;
        mouse_pos = other.mouse_pos;
        text = other.text;
        shortcut_last_used_action_counter = other.shortcut_last_used_action_counter;
        return *this;
    }
#endif

    bool operator==( const input_event &other ) const {
        if( type != other.type ) {
            return false;
        }

        if( sequence.size() != other.sequence.size() ) {
            return false;
        }
        for( size_t i = 0; i < sequence.size(); ++i ) {
            if( sequence[i] != other.sequence[i] ) {
                return false;
            }
        }

        if( modifiers.size() != other.modifiers.size() ) {
            return false;
        }
        for( size_t i = 0; i < modifiers.size(); ++i ) {
            if( modifiers[i] != other.modifiers[i] ) {
                return false;
            }
        }

        return true;
    }
};

/**
 * A set of attributes for an action
 */
struct action_attributes {
    action_attributes() : is_user_created( false ) {}
    bool is_user_created;
    std::string name;
    std::vector<input_event> input_events;
};

// Definitions for joystick/gamepad.

// On the joystick there's a maximum of 256 key states.
// So for joy axis events, we simply use a number larger
// than that.
#define JOY_0        0
#define JOY_1        1
#define JOY_2        2
#define JOY_3        3
#define JOY_4        4
#define JOY_5        5
#define JOY_6        6
#define JOY_7        7

#define JOY_LEFT        256 + 1
#define JOY_RIGHT       256 + 2
#define JOY_UP          256 + 3
#define JOY_DOWN        256 + 4
#define JOY_RIGHTUP     256 + 5
#define JOY_RIGHTDOWN   256 + 6
#define JOY_LEFTUP      256 + 7
#define JOY_LEFTDOWN    256 + 8

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
        int get_keycode( const std::string &name ) const;

        /**
         * Get the key name associated with the given keyboard keycode.
         *
         * @param ch Character code.
         * @param input_type Whether the keycode is a gamepad or a keyboard code.
         * @param portable If true, return a language independent and portable name
         * of the key. This acts as the inverse to get_keyname:
         * <code>get_keyname(get_keycode(a), , true) == a</code>
         */
        std::string get_keyname( int ch, input_event_t input_type, bool portable = false ) const;

        /**
         * curses getch() replacement.
         *
         * Defined in the respective platform wrapper, e.g. sdlcurse.cpp
         */
        input_event get_input_event();

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

    private:
        friend class input_context;

        using t_input_event_list = std::vector<input_event>;
        using t_actions = std::map<std::string, action_attributes>;
        using t_action_contexts = std::map<std::string, t_actions>;
        t_action_contexts action_contexts;
        using t_string_string_map = std::map<std::string, std::string>;

        using t_key_to_name_map = std::map<int, std::string>;
        t_key_to_name_map keycode_to_keyname;
        t_key_to_name_map gamepad_keycode_to_keyname;
        using t_name_to_key_map = std::map<std::string, int>;
        t_name_to_key_map keyname_to_keycode;

        // See @ref get_previously_pressed_key
        int previously_pressed_key;

        // Maps the key names we see in keybindings.json and in-game to
        // the keycode integers.
        void init_keycode_mapping();
        void add_keycode_pair( int ch, const std::string &name );
        void add_gamepad_keycode_pair( int ch, const std::string &name );

        /**
         * Load keybindings from a json file, override existing bindings.
         * Throws std::string on errors
         */
        void load( const std::string &file_name, bool is_user_preferences );

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
         *         that action's name is returned. Otherwise, the action_id is
         *         returned.
         */
        std::string get_default_action_name( const std::string &action_id ) const;
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
            handling_coordinate_input( false ) {
#if defined(__ANDROID__)
            input_context_stack.push_back( this );
            allow_text_entry = false;
#endif
        }
        // TODO: consider making the curses WINDOW an argument to the constructor, so that mouse input
        // outside that window can be ignored
        input_context( const std::string &category ) : registered_any_input( false ),
            category( category ), handling_coordinate_input( false ) {
#if defined(__ANDROID__)
            input_context_stack.push_back( this );
            allow_text_entry = false;
#endif
        }

#if defined(__ANDROID__)
        virtual ~input_context() {
            input_context_stack.remove( this );
        }

        // hack to allow creating manual keybindings for getch() instances, uilists etc. that don't use an input_context outside of the Android version
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
        void register_action( const std::string &action_descriptor, const std::string &name );

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
                              const unsigned int max_limit = 0,
                              const std::function<bool( const input_event & )> evt_filter =
        []( const input_event & ) {
            return true;
        } ) const;

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
         */
        std::string get_desc( const std::string &action_descriptor,
                              const std::string &text,
                              const std::function<bool( const input_event & )> evt_filter =
        []( const input_event & ) {
            return true;
        } ) const;

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
        cata::optional<tripoint> get_direction( const std::string &action ) const;

        /**
         * Get the coordinates associated with the last mouse click (if any).
         *
         * TODO: This right now is more or less specific to the map window,
         *       and returns the absolute map coordinate.
         *       Eventually this should be made more flexible.
         */
        cata::optional<tripoint> get_coordinates( const catacurses::window &window );

        // Below here are shortcuts for registering common key combinations.
        void register_directions();
        void register_updown();
        void register_leftright();
        void register_cardinal();

        /**
         * Displays the possible actions in the current context and their
         * keybindings.
         */
        void display_menu();

        /**
         * Temporary method to retrieve the raw input received, so that input_contexts
         * can be used in screens where not all possible actions have been defined in
         * keybindings.json yet.
         */
        input_event get_raw_input();

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
         * @param restrict_to_printable If `true` the function returns the bound keys only if they are printable. If `false`, all keys (whether they are printable or not) are returned.
         * @returns All keys bound to the given action descriptor.
         */
        std::vector<char> keys_bound_to( const std::string &action_descriptor,
                                         bool restrict_to_printable = true ) const;
        std::string key_bound_to( const std::string &action_descriptor, size_t index = 0,
                                  bool restrict_to_printable = true ) const;

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
         * this method will cause CATA_INPUT_TIMEOUT events to be generated correctly,
         * and will reset timeout correctly when a new input context is entered.
         */
        void set_timeout( int timeout );
        void reset_timeout();
    private:

        std::vector<std::string> registered_actions;
        std::string edittext;
    public:
        const std::string &input_to_action( const input_event &inp ) const;
    private:
        bool registered_any_input;
        std::string category; // The input category this context uses.
        point coordinate;
        bool coordinate_input_received;
        bool handling_coordinate_input;
        input_event next_action;
        bool iso_mode = false; // should this context follow the game's isometric settings?
        int timeout = -1;

        /**
         * When registering for actions within an input_context, callers can
         * specify a custom action name that will override the action's normal
         * name. This map stores those overrides. The key is the action ID and the
         * value is the user-visible name.
         */
        input_manager::t_string_string_map action_name_overrides;

        /**
         * Returns whether action uses the specified input
         */
        bool action_uses_input( const std::string &action_id, const input_event &event ) const;
        /**
         * Return a user presentable list of actions that conflict with the
         * proposed keybinding. Returns an empty string if nothing conflicts.
         */
        std::string get_conflicts( const input_event &event ) const;
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

#endif
