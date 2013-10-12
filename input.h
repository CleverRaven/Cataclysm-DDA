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

enum InputEvent {
    Confirm,
    Cancel,
    Close,
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
void get_direction(int &x, int &y, InputEvent &input);
std::string get_input_string_from_file(std::string fname="input.txt");

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

enum input_event_t {
    CATA_INPUT_ERROR,
    CATA_INPUT_KEYBOARD,
    CATA_INPUT_GAMEPAD
};

/**
 * An instance of an input, like a keypress etc.
 *
 * Both gamepad and keyboard keypresses will be represented as `long`.
 * Whether a gamepad or keyboard was used can be checked using the
 * `type` member.
 *
 */
struct input_event {
    input_event_t type;

    std::vector<long> modifiers; // Keys that need to be held down for
                                 // this event to be activated.

    std::vector<long> sequence; // The sequence of key events that
                                // triggers this event. For single-key
                                // events, simply make this of size 1.

    bool operator==(const input_event& other) const {
        if(type != other.type) return false;

        if(sequence.size() != other.sequence.size()) {
            return false;
        }
        for(int i=0; i<sequence.size(); i++) {
            if(sequence[i] != other.sequence[i]) {
                return false;
            }
        }

        if(modifiers.size() != other.modifiers.size()) {
            return false;
        }
        for(int i=0; i<modifiers.size(); i++) {
            if(modifiers[i] != other.modifiers[i]) {
                return false;
            }
        }

        return true;
    }
};

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
     * Get the keycode associated with the given key name.
     */
    long get_keycode(std::string name);

    /**
     * Get the key name associated with the given keyboard keycode.
     * 
     * @param input_type Whether the keycode is a gamepad or a keyboard code.
     */
    std::string get_keyname(long ch, input_event_t input_type);

    /**
     * Get the human-readable name for an action.
     */
    const std::string& get_action_name(const std::string& action);

    /**
     * curses getch() replacement.
     *
     * Defined in the respective platform wrapper, e.g. sdlcurse.cpp
     */
    input_event get_input_event(WINDOW* win);

private:
    std::map<std::string, std::vector<input_event> > action_to_input;
    std::map<std::string, std::map<std::string,std::vector<input_event> > > action_contexts;
    std::map<std::string, std::string> actionID_to_name;

    std::map<long, std::string> keycode_to_keyname;
    std::map<long, std::string> gamepad_keycode_to_keyname;
    std::map<std::string, long> keyname_to_keycode;

    // Maps the key names we see in keybindings.json and in-game to
    // the keycode integers.
    void init_keycode_mapping();
    void add_keycode_pair(long ch, const std::string& name);
    void add_gamepad_keycode_pair(long ch, const std::string& name);
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
    input_context() : registered_any_input(false), category("default") {};
    input_context(std::string category) : registered_any_input(false), category(category) {};

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
    void get_direction(int& dx, int& dy, const std::string& action);

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

    /* For the future, something like this might be nice:
     * const std::string register_action(const std::string& action_descriptor, x, y, width, height);
     * (x, y, width, height) would describe an area on the visible window that, if clicked, triggers the action.
     */

private:
    std::vector<std::string> registered_actions;
    const std::string& input_to_action(input_event& inp);
    bool registered_any_input;
    std::string category; // The input category this context uses.
};

/**
 * Check whether a gamepad is plugged in/available.
 * 
 * Always false in non-SDL versions.
 */
bool gamepad_available();

#endif
