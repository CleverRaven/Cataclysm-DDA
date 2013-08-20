#ifndef _INPUT_H_
#define _INPUT_H_

#include <string>
#include <map>
#include <vector>

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

enum input_event_t {
    INPUT_KEYBOARD,
    INPUT_GAMEPAD
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
    const std::vector<input_event>& get_input_for_action(const std::string& action_descriptor);

    /**
     * Initializes the input manager, aka loads the input mapping configuration JSON.
     */
    void init();

    /**
     * Get the keycode associated with the given key name.
     */
    long get_keycode(std::string name);

    /**
     * Get the key name associated with the given keycode.
     */
    std::string get_keyname(long ch);

private:
    std::map<std::string, std::vector<input_event> > action_to_input;

    std::map<long, std::string> keycode_to_keyname;
    std::map<std::string, long> keyname_to_keycode;

    // Maps the key names we see in keybindings.json and in-game to
    // the keycode integers.
    void init_keycode_mapping();
    void add_keycode_pair(long ch, const std::string& name);
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
 * This turns InputContext into an abstraction method between actual
 * input(keyboard, gamepad etc.) and game.
 */
class input_context {
public:
    input_context() : registered_any_input(false) {};

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

    /* For the future, something like this might be nice:
     * const std::string register_action(const std::string& action_descriptor, x, y, width, height);
     * (x, y, width, height) would describe an area on the visible window that, if clicked, triggers the action.
     */

private:
    std::vector<std::string> registered_actions;
    const std::string& input_to_action(input_event& inp);
    bool registered_any_input;
};

#endif
