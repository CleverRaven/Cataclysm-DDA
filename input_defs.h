#ifndef _INPUT_DEFS_H_
#define _INPUT_DEFS_H_

#include <vector>

// Common input definitions across Windows (including Cygwin) and Linux

enum input_event_t {
    CATA_INPUT_ERROR,
    CATA_INPUT_KEYBOARD,
    CATA_INPUT_GAMEPAD,
    CATA_INPUT_MOUSE_BUTTON,
    CATA_INPUT_MOUSE_MOVE
};

enum mouse_buttons { MOUSE_BUTTON_LEFT=1, MOUSE_BUTTON_RIGHT, MOUSE_MOVE };

#define ERR (-1) // Error return.
#define OK (0)   // Success return.

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
        type = CATA_INPUT_ERROR;
        mouse_x = mouse_y = 0;
    }

    long get_first_input() const
    {
        if (sequence.size() == 0) {
            return ERR;
        }

        return sequence[0];
    }

    bool is_valid_input() const
    {
        return get_first_input() != ERR;
    }

    void add_input(const long input)
    {
        sequence.push_back(input);
    }

    // True if mouse move event indicates a loss of focus from a given window
    bool is_lost_focus_event() const
    {
        return mouse_x == -1;
    }

    bool operator==(const input_event& other) const
    {
        if(type != other.type) {
            return false;
        }

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

#endif
