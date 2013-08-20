#include "cursesdef.h"
#include "input.h"
#include "picojson.h"
#include "output.h"
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
            const std::string& key = keybinding.get("key").get<std::string>();
            action_to_input[action_id].type = INPUT_KEYPRESS;

            // TODO: process special "keycodes", such as UP, DOWN etc.
            action_to_input[action_id].key = key[0];
        }
    }
}

const input_instance& input_manager::get_input_for_action(const std::string& action_descriptor) {
    return action_to_input[action_descriptor];
}

const std::string ERROR = "ERROR";
const std::string ANY_INPUT = "ANY_INPUT";

const std::string& input_context::input_to_action(input_instance& inp) {
    for(int i=0; i<registered_actions.size(); i++) {
        const std::string& action = registered_actions[i];
        if(inp_mngr.get_input_for_action(action) == inp) {
            return action;
        }
    }
    return ERROR;
}

const std::string input_context::register_action(const std::string& action_descriptor) {
    if(action_descriptor == "ANY_INPUT") {
        registered_any_input = true;
        return "(*)"; // * for wildcard
    }

    registered_actions.push_back(action_descriptor);

    const input_instance& event = inp_mngr.get_input_for_action(action_descriptor);
    if(event.type == INPUT_KEYPRESS) {
        std::string rval(1, (char) event.key);
        return rval;
    }

    return "ERROR";
}

const std::string& input_context::handle_input() {
    while(1) {
        long ch = getch();

        input_instance next_action;
        next_action.type = INPUT_KEYPRESS;
        next_action.key = ch;

        const std::string& action = input_to_action(next_action);
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
        dx = 0;
        dy = 0;
    }
}
