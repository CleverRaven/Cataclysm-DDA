#include "cursesdef.h"
#include "input.h"
#include "picojson.h"
#include "output.h"
#include "keypress.h"
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
    init_keycode_mapping();

    std::ifstream data_file;
    picojson::value input_value;

    std::string file_name = "data/raw/keybindings.json";
    data_file.open(file_name.c_str());

    if(!data_file.good()) {
        throw "Could not read " + file_name;
    }

    data_file >> input_value;
    data_file.close();

    if(!input_value.is<picojson::array>()) {
        throw file_name + "is not an array";
    }

    //Crawl through once and create an entry for every definition
    const picojson::array& root = input_value.get<picojson::array>();

    for (picojson::array::const_iterator entry = root.begin();
         entry != root.end(); ++entry) {
        if( !(entry->is<picojson::object>()) ){
            debugmsg("Invalid keybinding setting, entry not a JSON object");
            continue;
        }
        const picojson::value& keybinding = *entry;

        const std::string& action_id = keybinding.get("id").get<std::string>();
        const std::string& input_method = keybinding.get("input_method").get<std::string>();

        if(input_method == "keyboard") {
            input_event new_event;
            new_event.type = INPUT_KEYBOARD;
            if(keybinding.get("key").is<std::string>()) {
                const std::string& key = keybinding.get("key").get<std::string>();

                new_event.sequence.push_back(inp_mngr.get_keycode(key));
            } else if(keybinding.get("key").is<picojson::array>()) {
                picojson::array keys = keybinding.get("key").get<picojson::array>();
                for(int i=0; i<keys.size(); i++) {
                    const std::string& next_key = keybinding.get("key").get(i).get<std::string>();

                    new_event.sequence.push_back(inp_mngr.get_keycode(next_key));
                }
            }
            if(!keybinding.contains("name")) {
                actionID_to_name[action_id] = action_id;
            } else {
                actionID_to_name[action_id] = keybinding.get("name").get<std::string>();
            }
            action_to_input[action_id].push_back(new_event);
        }
    }
}

void input_manager::add_keycode_pair(long ch, const std::string& name) {
    keycode_to_keyname[ch] = name;
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
    add_keycode_pair(KEY_NPAGE,     "PGUP");
    add_keycode_pair(KEY_PPAGE,     "PGDWN");
    add_keycode_pair(KEY_ESCAPE,    "ESC");
}

long input_manager::get_keycode(std::string name) {
    return keyname_to_keycode[name];
}

std::string input_manager::get_keyname(long ch) {
    return keycode_to_keyname[ch];
}

const std::vector<input_event>& input_manager::get_input_for_action(const std::string& action_descriptor) {
    return action_to_input[action_descriptor];
}

const std::string& input_manager::get_action_name(const std::string& action) {
    return actionID_to_name[action];
}

const std::string ERROR = "ERROR";
const std::string UNDEFINED = "UNDEFINED";
const std::string ANY_INPUT = "ANY_INPUT";

const std::string& input_context::input_to_action(input_event& inp) {
    for(int i=0; i<registered_actions.size(); i++) {
        const std::string& action = registered_actions[i];
        const std::vector<input_event>& check_inp = inp_mngr.get_input_for_action(action);

        // Does this action have our queried input event in its keybindings?
        for(int i=0; i<check_inp.size(); i++) {
            if(check_inp[i] == inp) {
                return action;
            }
        }
    }
    return ERROR;
}

void input_context::register_action(const std::string& action_descriptor) {
    if(action_descriptor == "ANY_INPUT") {
        registered_any_input = true;
    }

    registered_actions.push_back(action_descriptor);
}

const std::string input_context::get_desc(const std::string& action_descriptor) {
    if(action_descriptor == "ANY_INPUT") {
        return "(*)"; // * for wildcard
    }

    const std::vector<input_event>& events = inp_mngr.get_input_for_action(action_descriptor);

    if(events.size() == 0) {
        return UNDEFINED;
    }

    std::stringstream rval;
    for(int i=0; i < events.size(); i++) {
        if(events[i].type == INPUT_KEYBOARD) {
            for(int j=0; j<events[i].sequence.size(); j++) {
                rval << inp_mngr.get_keyname(events[i].sequence[j]);
            }
        }
        if(i + 1 < events.size()) {
            // We're generating a list separated by or
            rval << " or ";
        }
    }
    return rval.str();
}


const std::string& input_context::handle_input() {
    while(1) {
        long ch = getch();

        input_event next_action;
        next_action.type = INPUT_KEYBOARD;
        next_action.sequence.push_back(ch);

        const std::string& action = input_to_action(next_action);

        // Special help action
        if(action == "HELP_KEYBINDINGS") {
            display_help();
            continue;
        }

        if(action != ERROR) {
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
    register_action("UPLEFT");
    register_action("DOWNLEFT");
    register_action("UPRIGHT");
    register_action("DOWNRIGHT");
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
    } else if(action == "UPLEFT") {
        dx = -1;
        dy = -1;
    } else if(action == "UPRIGHT") {
        dx = 1;
        dy = -1;
    } else if(action == "DOWNLEFT") {
        dx = -1;
        dy = 1;
    } else if(action == "DOWNRIGHT") {
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

    // Clear the lines
    for (int i = 0; i < FULL_SCREEN_HEIGHT-2; i++)
    mvwprintz(w_help, i, 0, c_black, "                                                ");

    for (int i=0; i<registered_actions.size(); i++) {
        const std::string& action_id = registered_actions[i];
        if(action_id == "ANY_INPUT") continue;

        const std::vector<input_event>& input_events = inp_mngr.get_input_for_action(action_id);

        nc_color col = input_events.size() ? c_white : c_ltred;
        mvwprintz(w_help, i, 3, col, "%s: ", inp_mngr.get_action_name(action_id).c_str());

        if (!input_events.size()) {
            mvwprintz(w_help, i, 30, c_ltred, _("Unbound!"));
        } else {
            mvwprintz(w_help, i, 30, c_ltgreen, "%s", get_desc(action_id).c_str());
        }
    }
    wrefresh(w_help);
    refresh();

    long ch = getch();
    while (ch != 'q' && ch != 'Q' && ch != KEY_ESCAPE) {};

    werase(w_help);
}
