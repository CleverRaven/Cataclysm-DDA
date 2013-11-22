#include "cursesdef.h"
#include "input.h"
#include "json.h"
#include "output.h"
#include "keypress.h"
#include "game.h"
#include <fstream>

/* TODO Replace the hardcoded values with an abstraction layer.
 * Lower redundancy across the methods. */

InputEvent get_input(int ch)
{
    if (ch == '\0')
        ch = getch();

    switch(ch)
    {
        case 'k':
        case '8':
        case KEY_UP:
            return DirectionN;
        case 'j':
        case '2':
        case KEY_DOWN:
            return DirectionS;
        case 'l':
        case '6':
        case KEY_RIGHT:
            return DirectionE;
        case 'h':
        case '4':
        case KEY_LEFT:
            return DirectionW;
        case 'y':
        case '7':
            return DirectionNW;
        case 'u':
        case '9':
            return DirectionNE;
        case 'b':
        case '1':
            return DirectionSW;
        case 'n':
        case '3':
            return DirectionSE;
        case '.':
        case '5':
            return DirectionNone;
        case '>':
            return DirectionDown;
        case '<':
            return DirectionUp;

        case '\n':
            return Confirm;
        case ' ':
            return Close;
        case 27: /* TODO Fix delay */
        case 'q':
            return Cancel;
        case '\t':
            return Tab;
        case '?':
            return Help;

        case ',':
        case 'g':
            return Pickup;
        case 'f':
        case 'F':
            return Filter;
        case 'r':
        case 'R':
            return Reset;
        default:
            return Undefined;
    }

}

bool is_mouse_enabled()
{
#if ((defined _WIN32 || defined WINDOWS) && !(defined SDLTILES || defined TILES))
    return false;
#else
    return true;
#endif
}

void get_direction(int &x, int &y, InputEvent &input)
{
    x = 0;
    y = 0;

    switch(input) {
        case DirectionN:
            --y;
            break;
        case DirectionS:
            ++y;
            break;
        case DirectionE:
            ++x;
            break;
        case DirectionW:
            --x;
            break;
        case DirectionNW:
            --x;
            --y;
            break;
        case DirectionNE:
            ++x;
            --y;
            break;
        case DirectionSW:
            --x;
            ++y;
            break;
        case DirectionSE:
            ++x;
            ++y;
            break;
        case DirectionNone:
        case Pickup:
            break;
        default:
            x = -2;
            y = -2;
    }
}

//helper function for those have problem inputing certain characters.
std::string get_input_string_from_file(std::string fname)
{
    std::string ret = "";
    std::ifstream fin(fname.c_str());
    if (fin){
        getline(fin, ret);
        //remove utf8 bmm
        if(ret.size()>0 && (unsigned char)ret[0]==0xef) {
            ret.erase(0,3);
        }
        while(ret.size()>0 && (ret[ret.size()-1]=='\r' ||  ret[ret.size()-1]=='\n')){
            ret.erase(ret.size()-1,1);
        }
    }
    return ret;
}

input_manager inp_mngr;

void input_manager::init() {
    last_mouse_x = last_mouse_y = -1;
    
    init_keycode_mapping();

    std::ifstream data_file;

    std::string file_name = "data/raw/keybindings.json";
    data_file.open(file_name.c_str(), std::ifstream::in | std::ifstream::binary);

    if(!data_file.good()) {
        throw "Could not read " + file_name;
    }

    JsonIn jsin(&data_file);

    //Crawl through once and create an entry for every definition
    jsin.start_array();
    while (!jsin.end_array()) {
        // JSON object representing the action
        JsonObject action = jsin.get_object();

        const std::string action_id = action.get_string("id");
        actionID_to_name[action_id] = action.get_string("name", action_id);
        const std::string context = action.get_string("category", "default");

        // Iterate over the bindings JSON array
        JsonArray bindings = action.get_array("bindings");
        const bool defaultcontext = (context == "default");
        while (bindings.has_more()) {
            JsonObject keybinding = bindings.next_object();
            std::string input_method = keybinding.get_string("input_method");
            input_event new_event;
            if(input_method == "keyboard") {
                new_event.type = CATA_INPUT_KEYBOARD;
            } else if(input_method == "gamepad") {
                new_event.type = CATA_INPUT_GAMEPAD;
            } else if(input_method == "mouse") {
                new_event.type = CATA_INPUT_MOUSE;
            }

            if (keybinding.has_array("key")) {
                JsonArray keys = keybinding.get_array("key");
                while (keys.has_more()) {
                    new_event.sequence.push_back(
                        get_keycode(keys.next_string())
                    );
                }
            } else { // assume string if not array, and throw if not string
                new_event.sequence.push_back(
                    get_keycode(keybinding.get_string("key"))
                );
            }

            if (defaultcontext) {
                action_to_input[action_id].push_back(new_event);
            } else {
                action_contexts[context][action_id].push_back(new_event);
            }
        }
    }

    data_file.close();
        }

void input_manager::add_keycode_pair(long ch, const std::string& name) {
    keycode_to_keyname[ch] = name;
    keyname_to_keycode[name] = ch;
}

void input_manager::add_gamepad_keycode_pair(long ch, const std::string& name) {
    gamepad_keycode_to_keyname[ch] = name;
    keyname_to_keycode[name] = ch;
}

void input_manager::init_keycode_mapping() {
    // Between space and tilde, all keys more or less map
    // to themselves(see ASCII table)
    for(char c=' '; c<='~'; c++) {
        std::string name(1, c);
        add_keycode_pair(c, name);
    }

    add_keycode_pair(KEY_UP,        "UP");
    add_keycode_pair(KEY_DOWN,      "DOWN");
    add_keycode_pair(KEY_LEFT,      "LEFT");
    add_keycode_pair(KEY_RIGHT,     "RIGHT");
    add_keycode_pair(KEY_NPAGE,     "NPAGE");
    add_keycode_pair(KEY_PPAGE,     "PPAGE");
    add_keycode_pair(KEY_ESCAPE,    "ESC");
    add_keycode_pair('\n',          "RETURN");

    add_gamepad_keycode_pair(JOY_LEFT,      "JOY_LEFT");
    add_gamepad_keycode_pair(JOY_RIGHT,     "JOY_RIGHT");
    add_gamepad_keycode_pair(JOY_UP,        "JOY_UP");
    add_gamepad_keycode_pair(JOY_DOWN,      "JOY_DOWN");
    add_gamepad_keycode_pair(JOY_LEFTUP,    "JOY_LEFTUP");
    add_gamepad_keycode_pair(JOY_LEFTDOWN,  "JOY_LEFTDOWN");
    add_gamepad_keycode_pair(JOY_RIGHTUP,   "JOY_RIGHTUP");
    add_gamepad_keycode_pair(JOY_RIGHTDOWN, "JOY_RIGHTDOWN");

    add_gamepad_keycode_pair(JOY_0,         "JOY_0");
    add_gamepad_keycode_pair(JOY_1,         "JOY_1");
    add_gamepad_keycode_pair(JOY_2,         "JOY_2");
    add_gamepad_keycode_pair(JOY_3,         "JOY_3");
    add_gamepad_keycode_pair(JOY_4,         "JOY_4");
    add_gamepad_keycode_pair(JOY_5,         "JOY_5");
    add_gamepad_keycode_pair(JOY_6,         "JOY_6");
    add_gamepad_keycode_pair(JOY_7,         "JOY_7");

    keyname_to_keycode["MOUSE_LEFT"] = MOUSE_BUTTON_LEFT;
    keyname_to_keycode["MOUSE_RIGHT"] = MOUSE_BUTTON_RIGHT;
    keyname_to_keycode["SCROLL_UP"] = SCROLLWHEEL_UP;
    keyname_to_keycode["SCROLL_DOWN"] = SCROLLWHEEL_DOWN;
    keyname_to_keycode["MOUSE_MOVE"] = MOUSE_MOVE;
}

long input_manager::get_keycode(std::string name) {
    return keyname_to_keycode[name];
}

std::string input_manager::get_keyname(long ch, input_event_t inp_type) {
    if(inp_type == CATA_INPUT_KEYBOARD) {
        return keycode_to_keyname[ch];
    } else if(inp_type == CATA_INPUT_MOUSE) {
        if(ch == MOUSE_BUTTON_LEFT) {
            return "MOUSE_LEFT";
        } else if(ch == MOUSE_BUTTON_RIGHT) {
            return "MOUSE_RIGHT";
        } else if(ch == SCROLLWHEEL_UP) {
            return "SCROLL_UP";
        } else if(ch == SCROLLWHEEL_DOWN) {
            return "SCROLL_DOWN";
        } else if(ch == MOUSE_MOVE) {
            return "MOUSE_MOVE";
        } else {
            return "MOUSE_UNKNOWN";
        }
    } else if (inp_type == CATA_INPUT_GAMEPAD) {
        return gamepad_keycode_to_keyname[ch];
    } else {
        return "UNKNOWN";
    }
}

const std::vector<input_event>& input_manager::get_input_for_action(const std::string& action_descriptor, const std::string context, bool *overwrites_default) {
    // First we check if we have a special override in this particular context.
    if(context != "default" && action_contexts[context].count(action_descriptor)) {
        if(overwrites_default) *overwrites_default = true;
        return action_contexts[context][action_descriptor];
    }

    // If not, we use the default binding.
    if(overwrites_default) *overwrites_default = false;
    return action_to_input[action_descriptor];
}

const std::string& input_manager::get_action_name(const std::string& action) {
    return actionID_to_name[action];
}

const std::string CATA_ERROR = "ERROR";
const std::string UNDEFINED = "UNDEFINED";
const std::string ANY_INPUT = "ANY_INPUT";
const std::string COORDINATE = "COORDINATE";
const std::string TIMEOUT = "TIMEOUT";

const std::string& input_context::input_to_action(input_event& inp) {
    for(int i=0; i<registered_actions.size(); i++) {
        const std::string& action = registered_actions[i];
        const std::vector<input_event>& check_inp = inp_mngr.get_input_for_action(action, category);

        // Does this action have our queried input event in its keybindings?
        for(int i=0; i<check_inp.size(); i++) {
            if(check_inp[i] == inp) {
                return action;
            }
        }
    }
    return CATA_ERROR;
}

void input_manager::set_timeout(int delay)
{
    timeout(delay);
    // Use this to determine when curses should return a CATA_INPUT_TIMEOUT event.
    should_timeout = delay > 0;
}


void input_context::register_action(const std::string& action_descriptor) {
    if(action_descriptor == "ANY_INPUT") {
        registered_any_input = true;
    } else if(action_descriptor == "COORDINATE") {
        handling_coordinate_input = true;
    }

    registered_actions.push_back(action_descriptor);
}

const std::string input_context::get_desc(const std::string& action_descriptor) {
    if(action_descriptor == "ANY_INPUT") {
        return "(*)"; // * for wildcard
    }

    const std::vector<input_event>& events = inp_mngr.get_input_for_action(action_descriptor, category);

    if(events.size() == 0) {
        return UNDEFINED;
    }

    std::vector<input_event> inputs_to_show;
    for(int i=0; i<events.size(); i++) {
        const input_event& event = events[i];

        // Only display gamepad buttons if a gamepad is available.
        if(gamepad_available() || event.type != CATA_INPUT_GAMEPAD) {
            inputs_to_show.push_back(event);
        }
    }

    std::stringstream rval;
    for(int i=0; i < inputs_to_show.size(); i++) {
        for(int j=0; j<inputs_to_show[i].sequence.size(); j++) {
            rval << inp_mngr.get_keyname(inputs_to_show[i].sequence[j], inputs_to_show[i].type);
        }

        // We're generating a list separated by "," and "or"
        if(i + 2 == inputs_to_show.size()) {
            rval << " or ";
        } else if(i + 1 < inputs_to_show.size()) {
            rval << ", ";
        }
    }
    return rval.str();
}

const std::string& input_context::handle_input() {
    next_action.type = CATA_INPUT_ERROR;
    while(1) {
        
#if !(defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS || defined __CYGWIN__)
        // Register for ncurses mouse input
        mousemask(BUTTON1_CLICKED | BUTTON3_CLICKED | REPORT_MOUSE_POSITION, NULL);
#endif

        next_action = inp_mngr.get_input_event(NULL);

#if !(defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS || defined __CYGWIN__)
        // De-register from ncurses mouse input
        mousemask(0, NULL);
#endif
        

        if (next_action.type == CATA_INPUT_TIMEOUT) {
            return TIMEOUT;
        }

        const std::string& action = input_to_action(next_action);

        // Special help action
        if(action == "HELP_KEYBINDINGS") {
            display_help();
            continue;
        }

        if(next_action.type == CATA_INPUT_MOUSE) {
            if(!handling_coordinate_input) {
                continue; // Ignore this mouse input.
            }

            coordinate_input_received = true;
            coordinate_x = next_action.mouse_x;
            coordinate_y = next_action.mouse_y;
        } else {
            coordinate_input_received = false;
        }

        if(action != CATA_ERROR) {
            return action;
        }

        // If we registered to receive any input, return ANY_INPUT
        // to signify that an unregistered key was pressed.
        if(registered_any_input) {
            return ANY_INPUT;
        }

        // If it's an invalid key, just keep looping until the user
        // enters something proper.
    }
}

void input_context::register_directions() {
    register_cardinal();
    register_action("LEFTUP");
    register_action("LEFTDOWN");
    register_action("RIGHTUP");
    register_action("RIGHTDOWN");
}

void input_context::register_updown() {
    register_action("UP");
    register_action("DOWN");
}

void input_context::register_leftright() {
    register_action("LEFT");
    register_action("RIGHT");
}

void input_context::register_cardinal() {
    register_updown();
    register_leftright();
}

void input_context::get_direction(int& dx, int& dy, const std::string& action) {
    if(action == "UP") {
        dx = 0;
        dy = -1;
    } else if(action == "DOWN") {
        dx = 0;
        dy = 1;
    } else if(action == "LEFT") {
        dx = -1;
        dy = 0;
    } else if(action ==  "RIGHT") {
        dx = 1;
        dy = 0;
    } else if(action == "LEFTUP") {
        dx = -1;
        dy = -1;
    } else if(action == "RIGHTUP") {
        dx = 1;
        dy = -1;
    } else if(action == "LEFTDOWN") {
        dx = -1;
        dy = 1;
    } else if(action == "RIGHTDOWN") {
        dx = 1;
        dy = 1;
    } else {
        dx = -2;
        dy = -2;
    }
}

void input_context::display_help() {
    // Shamelessly stolen from help.cpp
    WINDOW* w_help = newwin(FULL_SCREEN_HEIGHT-2, FULL_SCREEN_WIDTH-2,
        1 + (int)((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY-FULL_SCREEN_HEIGHT)/2 : 0),
        1 + (int)((TERMX > FULL_SCREEN_WIDTH) ? (TERMX-FULL_SCREEN_WIDTH)/2 : 0));

    werase(w_help);

    mvwprintz(w_help, 1, 51, c_ltred, _("Unbound keys"));
    mvwprintz(w_help, 2, 51, c_ltgreen, _("Keybinding active only"));
    mvwprintz(w_help, 3, 51, c_ltgreen, _("on this screen"));
    mvwprintz(w_help, 4, 51, c_ltgray, _("Keybinding active globally"));

    // Clear the lines
    for (int i = 0; i < FULL_SCREEN_HEIGHT-2; i++)
    mvwprintz(w_help, i, 0, c_black, "                                                ");

    for (int i=0; i<registered_actions.size(); i++) {
        const std::string& action_id = registered_actions[i];
        if(action_id == "ANY_INPUT") continue;

        bool overwrite_default;
        const std::vector<input_event>& input_events = inp_mngr.get_input_for_action(action_id, category, &overwrite_default);

        nc_color col = input_events.size() ? c_white : c_ltred;
        mvwprintz(w_help, i, 3, col, "%s: ", inp_mngr.get_action_name(action_id).c_str());

        if (!input_events.size()) {
            mvwprintz(w_help, i, 30, c_ltred, _("Unbound!"));
        } else {
            // The color depends on whether this input draws from context-local or from
            // default settings. Default will be ltgray, overwrite will be ltgreen.
            col = overwrite_default ? c_ltgreen : c_ltgray;

            mvwprintz(w_help, i, 30, col, "%s", get_desc(action_id).c_str());
        }
    }
    wrefresh(w_help);
    refresh();

    long ch = getch();
    while (ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE) { ch = getch(); };

    werase(w_help);
}

input_event input_context::get_raw_input()
{
    return next_action;
}

#ifndef TILES
// If we're using curses, we need to provide get_input_event() here.
input_event input_manager::get_input_event(WINDOW* win)
{
    int key = get_keypress();
    input_event rval;
    if (key == ERR) {
        if (should_timeout) {
            rval.type = CATA_INPUT_TIMEOUT;
        } else {
            rval.type = CATA_INPUT_ERROR;
        }
#if !(defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS || defined __CYGWIN__)
    // ncurses mouse handling
    } else if (key == KEY_MOUSE) {
        MEVENT event;
        if (getmouse(&event) == OK) {
            rval.type = CATA_INPUT_MOUSE;
            rval.mouse_x = event.x - VIEW_OFFSET_X;
            rval.mouse_y = event.y - VIEW_OFFSET_Y;
            inp_mngr.last_mouse_x = rval.mouse_x;
            inp_mngr.last_mouse_y = rval.mouse_y;
            if (event.bstate & BUTTON1_CLICKED) {
                rval.add_input(MOUSE_BUTTON_LEFT);
            } else if (event.bstate & BUTTON3_CLICKED) {
                rval.add_input(MOUSE_BUTTON_RIGHT);
            } else if (event.bstate & REPORT_MOUSE_POSITION) {
                rval.add_input(MOUSE_MOVE);
            } else {
                rval.type = CATA_INPUT_ERROR;
            }
        } else {
            rval.type = CATA_INPUT_ERROR;
        }
#endif
    } else {
        rval.type = CATA_INPUT_KEYBOARD;
        rval.add_input(key);
    }
    should_timeout = false;

#if !(defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS || defined __CYGWIN__)
    // De-register ncurses mouse input. Otherwise unmanaged getch() calls will detect mouse input.
    mousemask(0, NULL);
#endif

    return rval;
}

// Also specify that we don't have a gamepad plugged in.
bool gamepad_available()
{
    return false;
}

// Coordinates just never happen(no mouse input)
bool input_context::get_coordinates(WINDOW* capture_win, int& x, int& y)
{
    int view_columns = getmaxx(capture_win);
    int view_rows = getmaxy(capture_win);
    int win_left = getbegx(capture_win) - VIEW_OFFSET_X;
    int win_right = win_left + view_columns - 1;
    int win_top = getbegy(capture_win) - VIEW_OFFSET_Y;
    int win_bottom = win_top + view_rows - 1;
    if (coordinate_x < win_left || coordinate_x > win_right || coordinate_y < win_top || coordinate_y > win_bottom) {
        return false;
    }
    
    x = g->ter_view_x - ((view_columns/2) - coordinate_x);
    y = g->ter_view_y - ((view_rows/2) - coordinate_y);
    
    return true;
}
#endif
