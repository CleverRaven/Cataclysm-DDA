#ifndef _INPUT_H_
#define _INPUT_H_

#include <string>
#include <map>
#include <vector>
#include "cursesdef.h"

// Compiling with SDL enables gamepad support.
#ifdef TILES
    #define GAMEPAD_ENABLED
#endif

#define KEY_ESCAPE 27

enum InputEvent {
    Confirm,
    Cancel,
    Close,
    Tab,
    Help,

    DirectionN,
    DirectionS,
    DirectionE,
    DirectionW,
    DirectionNW,
    DirectionNE,
    DirectionSE,
    DirectionSW,
    DirectionNone,

    DirectionDown, /* Think stairs */
    DirectionUp,
    Filter,
    Reset,
    Pickup,
    Nothing,
    Undefined
};

InputEvent get_input(int ch = '\0');
bool is_mouse_enabled();
void get_direction(int &x, int &y, InputEvent &input);
std::string get_input_string_from_file(std::string fname = "input.txt");

// Simple text input--translates numpad to vikeys
long input(long ch = -1);
long get_keypress();

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
 * Gamepad, mouse and keyboard keypresses will be represented as `long`.
 * Whether a gamepad, mouse or keyboard was used can be checked using the
 * `type` member.
 *
 */
struct input_event {
    input_event_t type;

    std::vector<long> modifiers; // Keys that need to be held down for
                                 // this event to be activated.

    std::vector<long> sequence; // The sequence of key or mouse events that
                                // triggers this event. For single-key
                                // events, simply make this of size 1.

    int mouse_x, mouse_y;       // Mouse click co-ordinates, if applicable

    input_event()
    {
        mouse_x = mouse_y = 0;
        type = CATA_INPUT_ERROR;
    }
    input_event(long s, input_event_t t)
    : type(t)
    {
        mouse_x = mouse_y = 0;
        sequence.push_back(s);
    }

    long get_first_input() const
    {
        if (sequence.empty()) {
            return 0;
        }

        return sequence[0];
    }

    void add_input(const long input)
    {
        sequence.push_back(input);
    }

    bool operator==(const input_event& other) const
    {
        if(type != other.type) {
            return false;
        }

        if(sequence.size() != other.sequence.size()) {
            return false;
        }
        for( size_t i = 0; i < sequence.size(); ++i ) {
            if(sequence[i] != other.sequence[i]) {
                return false;
            }
        }

        if(modifiers.size() != other.modifiers.size()) {
            return false;
        }
        for( size_t i = 0; i < modifiers.size(); ++i ) {
            if(modifiers[i] != other.modifiers[i]) {
                return false;
            }
        }

        return true;
    }
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
class input_manager {
public:
    // TODO: rewrite this to have several alternative input events for the same action

    /**
     * Get the input events associated with an action ID in a given context.
     *
     * Note that if context is something other than "default", the default bindings will not be returned.
     *
     * @param action_descriptor The action ID to get the input events for.
     * @param context The context in which to get the input events. Defaults to "default".
     * @param overwrites_default If this is non-NULL, this will be used as return parameter and will be set to true if the default
     *                           keybinding is overriden by something else in the given context.
     */
    const std::vector<input_event>& get_input_for_action(const std::string& action_descriptor, const std::string context="default", bool *overwrites_default=NULL);

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
     * Get the keycode associated with the given key name.
     */
    long get_keycode(const std::string &name) const;

    /**
     * Get the key name associated with the given keyboard keycode.
     *
     * @param input_type Whether the keycode is a gamepad or a keyboard code.
     * @param portable If true, return a language independent and portable name
     * of the key. This acts as the inverse to get_keyname:
     * <code>get_keyname(get_keycode(a), , true) == a</code>
     */
    std::string get_keyname(long ch, input_event_t input_type, bool portable = false) const;

    /**
     * Get the human-readable name for an action.
     */
    const std::string& get_action_name(const std::string& action) const;

    /**
     * curses getch() replacement.
     *
     * Defined in the respective platform wrapper, e.g. sdlcurse.cpp
     */
    input_event get_input_event(WINDOW* win);

    bool translate_to_window_position();

    /**
     * Sets input polling timeout as appropriate for the current interface system.
     * Use this method to set timeouts when using input_manager, rather than calling
     * the old timeout() method, as using this method will cause CATA_INPUT_TIMEOUT
     * events to be generated correctly.
     */
    void set_timeout(int delay);

private:
    friend class input_context;

    typedef std::vector<input_event> t_input_event_list;
    typedef std::map<std::string, t_input_event_list> t_keybinding_map;
    typedef std::map<std::string, t_keybinding_map> t_action_contexts;
    t_action_contexts action_contexts;
    typedef std::map<std::string, std::string> t_string_string_map;
    t_string_string_map actionID_to_name;

    typedef std::map<long, std::string> t_key_to_name_map;
    t_key_to_name_map keycode_to_keyname;
    t_key_to_name_map gamepad_keycode_to_keyname;
    typedef std::map<std::string, long> t_name_to_key_map;
    t_name_to_key_map keyname_to_keycode;

    // Maps the key names we see in keybindings.json and in-game to
    // the keycode integers.
    void init_keycode_mapping();
    void add_keycode_pair(long ch, const std::string& name);
    void add_gamepad_keycode_pair(long ch, const std::string& name);

    int input_timeout;

    t_input_event_list &get_event_list(const std::string &action_descriptor, const std::string &context);
    void remove_input_for_action(const std::string &action_descriptor, const std::string &context);
    void add_input_for_action(const std::string &action_descriptor, const std::string &context, const input_event &event);
    /**
     * Return a user presentable list of actions that conflict with the
     * proposed keybinding. Returns an empty string if nothing conflicts.
     */
    std::string get_conflicts(const std::string &context, const input_event &event) const;
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
class input_context {
public:
    input_context() : registered_any_input(false), category("default"),
        handling_coordinate_input(false) {};
    // TODO: consider making the curses WINDOW an argument to the constructor, so that mouse input
    // outside that window can be ignored
    input_context(std::string category) : registered_any_input(false),
        category(category), handling_coordinate_input(false) {};

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
     */
    void register_action(const std::string& action_descriptor);

    /**
     * Get a description text for the key/other input method associated
     * with the given action.
     */
    const std::string get_desc(const std::string& action_descriptor);

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
    const std::string& handle_input();

    /**
     * Convert a direction action(UP, DOWN etc) to a delta x and y.
     *
     * @param dx Output parameter for x delta.
     * @param dy Output parameter for y delta.
     */
    static bool get_direction(int& dx, int& dy, const std::string& action);

    /**
     * Get the coordinates associated with the last mouse click.
     *
     * TODO: This right now is more or less specific to the map window,
     *       and returns the absolute map coordinate.
     *       Eventually this should be made more flexible.
     *
     * @return true if we could process a click inside the window, false otherwise.
     */
    bool get_coordinates(WINDOW* window, int& x, int& y);

    // Below here are shortcuts for registering common key combinations.
    void register_directions();
    void register_updown();
    void register_leftright();
    void register_cardinal();

    /**
     * Displays the possible actions in the current context and their
     * keybindings.
     */
    void display_help();

    /**
     * Temporary method to retrieve the raw input received, so that input_contexts
     * can be used in screens where not all possible actions have been defined in
     * keybindings.json yet.
     */
    input_event get_raw_input();


    /* For the future, something like this might be nice:
     * const std::string register_action(const std::string& action_descriptor, x, y, width, height);
     * (x, y, width, height) would describe an area on the visible window that, if clicked, triggers the action.
     */

private:

    std::vector<std::string> registered_actions;
    const std::string& input_to_action(input_event& inp);
    bool registered_any_input;
    std::string category; // The input category this context uses.
    int coordinate_x, coordinate_y;
    bool coordinate_input_received;
    bool handling_coordinate_input;
    input_event next_action;
};

/**
 * Check whether a gamepad is plugged in/available.
 *
 * Always false in non-SDL versions.
 */
bool gamepad_available();

#endif
