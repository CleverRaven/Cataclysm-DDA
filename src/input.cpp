#include "action.h"
#include "cursesdef.h"
#include "input.h"
#include "json.h"
#include "output.h"
#include "game.h"
#include "path_info.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <errno.h>

static const std::string default_context_id("default");

static long str_to_long(const std::string &number) {
    // ensure user's locale doesn't interfere with number format
    std::istringstream buffer(number);
    buffer.imbue(std::locale::classic());
    long result;
    buffer >> result;
    return result;
}

static std::string long_to_str(long number) {
    // ensure user's locale doesn't interfere with number format
    std::ostringstream buffer;
    buffer.imbue(std::locale::classic());
    buffer << number;
    return buffer.str();
}

/* TODO Replace the hardcoded values with an abstraction layer.
 * Lower redundancy across the methods. */

long input(long ch)
{
    if (ch == -1) {
        ch = get_keypress();
    }

    switch (ch) {
    case KEY_UP:
        return 'k';
    case KEY_LEFT:
        return 'h';
    case KEY_RIGHT:
        return 'l';
    case KEY_DOWN:
        return 'j';
    case KEY_NPAGE:
        return '>';
    case KEY_PPAGE:
        return '<';
    case 459:
        return '\n';
    default:
        return ch;
    }
}

long get_keypress()
{
    long ch = getch();

    // Our current tiles and Windows code doesn't have ungetch()
#if !(defined TILES || defined SDLTILES || defined _WIN32 || defined WINDOWS)
    if (ch != ERR) {
        int newch;

        // Clear the buffer of characters that match the one we're going to act on.
        timeout(0);
        do {
            newch = getch();
        } while( newch != ERR && newch == ch );
        timeout(-1);

        // If we read a different character than the one we're going to act on, re-queue it.
        if (newch != ERR && newch != ch) {
            ungetch(newch);
        }
    }
#endif

    return ch;
}

InputEvent get_input(int ch)
{
    if (ch == '\0') {
        ch = getch();
    }

    const action_id act = action_from_key(ch);
    switch(act) {
    case ACTION_MOVE_N:
        return DirectionN;
    case ACTION_MOVE_NE:
        return DirectionNE;
    case ACTION_MOVE_E:
        return DirectionE;
    case ACTION_MOVE_SE:
        return DirectionSE;
    case ACTION_MOVE_S:
        return DirectionS;
    case ACTION_MOVE_SW:
        return DirectionSW;
    case ACTION_MOVE_W:
        return DirectionW;
    case ACTION_MOVE_NW:
        return DirectionNW;
    default:
        break;
    }

    switch(ch) {
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
    if (fin) {
        getline(fin, ret);
        //remove utf8 bmm
        if(!ret.empty() && (unsigned char)ret[0] == 0xef) {
            ret.erase(0, 3);
        }
        while(!ret.empty() && (ret[ret.size() - 1] == '\r' ||  ret[ret.size() - 1] == '\n')) {
            ret.erase(ret.size() - 1, 1);
        }
    }
    return ret;
}

input_manager inp_mngr;

void input_manager::init()
{
    init_keycode_mapping();

    std::ifstream data_file;

    std::string file_name = FILENAMES["keybindings"];
    data_file.open(file_name.c_str(), std::ifstream::in | std::ifstream::binary);

    if(!data_file.good()) {
        throw "Could not read " + file_name;
    }

    JsonIn jsin(data_file);

    //Crawl through once and create an entry for every definition
    jsin.start_array();
    while (!jsin.end_array()) {
        // JSON object representing the action
        JsonObject action = jsin.get_object();

        const std::string action_id = action.get_string("id");
        actionID_to_name[action_id] = action.get_string("name", action_id);
        const std::string context = action.get_string("category", default_context_id);

        // Iterate over the bindings JSON array
        JsonArray bindings = action.get_array("bindings");
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

            action_contexts[context][action_id].push_back(new_event);
        }
    }

    data_file.close();
}

void input_manager::save() {
    std::ofstream data_file;

    std::string file_name = FILENAMES["keybindings"];
    std::string file_name_tmp = file_name + ".tmp";
    data_file.open(file_name_tmp.c_str(), std::ifstream::binary);

    if(!data_file.good()) {
        throw std::runtime_error(file_name_tmp + ": could not write");
    }
    data_file.exceptions(std::ios::badbit | std::ios::failbit);
    JsonOut jsout(data_file, true);

    jsout.start_array();
    for (t_action_contexts::const_iterator a = action_contexts.begin(); a != action_contexts.end(); ++a) {
        const t_keybinding_map &mapping = a->second;
        for (t_keybinding_map::const_iterator b = mapping.begin(); b != mapping.end(); ++b) {
            const t_input_event_list &events = b->second;
            jsout.start_object();

            jsout.member("id", b->first);
            jsout.member("name", actionID_to_name[b->first]);
            jsout.member("category", a->first);
            jsout.member("bindings");
            jsout.start_array();
            for(t_input_event_list::const_iterator c = events.begin(); c != events.end(); ++c) {
                jsout.start_object();
                switch(c->type) {
                    case CATA_INPUT_KEYBOARD:
                        jsout.member("input_method", "keyboard");
                        break;
                    case CATA_INPUT_GAMEPAD:
                        jsout.member("input_method", "gamepad");
                        break;
                    case CATA_INPUT_MOUSE:
                        jsout.member("input_method", "mouse");
                        break;
                    default:
                        throw std::runtime_error("unknown input_event_t");
                }
                jsout.member("key");
                jsout.start_array();
                for(size_t i = 0; i < c->sequence.size(); i++) {
                    jsout.write(get_keyname(c->sequence[i], c->type, true));
                }
                jsout.end_array();
                jsout.end_object();
            }
            jsout.end_array();

            jsout.end_object();
        }
    }
    jsout.end_array();

    data_file.close();
    rename(file_name_tmp.c_str(), file_name.c_str());
}

void input_manager::add_keycode_pair(long ch, const std::string &name)
{
    keycode_to_keyname[ch] = name;
    keyname_to_keycode[name] = ch;
}

void input_manager::add_gamepad_keycode_pair(long ch, const std::string &name)
{
    gamepad_keycode_to_keyname[ch] = name;
    keyname_to_keycode[name] = ch;
}

void input_manager::init_keycode_mapping()
{
    // Between space and tilde, all keys more or less map
    // to themselves(see ASCII table)
    for(char c = ' '; c <= '~'; c++) {
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
    add_keycode_pair(KEY_BACKSPACE, "BACKSPACE");
    add_keycode_pair(KEY_HOME,      "HOME");
    add_keycode_pair(KEY_BREAK,     "BREAK");
    add_keycode_pair(KEY_END,       "END");
    add_keycode_pair('\n',          "RETURN");

    // function keys, as defined by ncurses
    for(int i = 0; i <= 63; i++) {
        add_keycode_pair(KEY_F(i), string_format("F%d", i));
    }

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

long input_manager::get_keycode(const std::string &name) const
{
    const t_name_to_key_map::const_iterator a = keyname_to_keycode.find(name);
    if (a != keyname_to_keycode.end()) {
        return a->second;
    }
    // Not found in map, try to parse as long
    if (name.compare(0, 8, "UNKNOWN_") == 0) {
        return str_to_long(name.substr(8));
    }
    return 0;
}

std::string input_manager::get_keyname(long ch, input_event_t inp_type, bool portable) const
{
    if(inp_type == CATA_INPUT_KEYBOARD) {
        const t_key_to_name_map::const_iterator a = keycode_to_keyname.find(ch);
        if (a != keycode_to_keyname.end()) {
            return a->second;
        }
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
        }
    } else if (inp_type == CATA_INPUT_GAMEPAD) {
        const t_key_to_name_map::const_iterator a = gamepad_keycode_to_keyname.find(ch);
        if (a != gamepad_keycode_to_keyname.end()) {
            return a->second;
        }
    } else {
        return "UNKNOWN";
    }
    if (portable) {
        return std::string("UNKNOWN_") + long_to_str(ch);
    }
    return string_format(_("unknown key %ld"), ch);
}

const std::vector<input_event> &input_manager::get_input_for_action(const std::string
        &action_descriptor, const std::string context, bool *overwrites_default)
{
    // First we check if we have a special override in this particular context.
    if(context != default_context_id && action_contexts[context][action_descriptor].size()) {
        if(overwrites_default) {
            *overwrites_default = true;
        }
        return action_contexts[context][action_descriptor];
    }

    // If not, we use the default binding.
    if(overwrites_default) {
        *overwrites_default = false;
    }
    return action_contexts[default_context_id][action_descriptor];
}

input_manager::t_input_event_list &input_manager::get_event_list(
    const std::string &action_descriptor, const std::string &context)
{
    t_action_contexts::iterator a = action_contexts.find(context);
    if (a != action_contexts.end()) {
        // Create a new empty list for this action if nothing exists.
        return a->second[action_descriptor];
    }
    static t_input_event_list empty;
    return empty;
}

void input_manager::remove_input_for_action(
    const std::string &action_descriptor, const std::string &context)
{
    get_event_list(action_descriptor, context).clear();
}

void input_manager::add_input_for_action(
    const std::string &action_descriptor, const std::string &context, const input_event &event)
{
    t_input_event_list &events = get_event_list(action_descriptor, context);
    for (t_input_event_list::iterator a = events.begin(); a != events.end(); ++a) {
        if (*a == event) {
            return;
        }
    }
    events.push_back(event);
}

std::string input_manager::get_conflicts(const std::string &context, const input_event &event) const
{
    std::ostringstream buffer;
    if (context != default_context_id) {
        // also include the default context all the time
        buffer << get_conflicts(default_context_id, event);
    }
    t_action_contexts::const_iterator a = action_contexts.find(context);
    if (a == action_contexts.end()) {
        return "";
    }
    const t_keybinding_map &keys = a->second;
    for (t_keybinding_map::const_iterator b = keys.begin(); b != keys.end(); ++b) {
        const std::string &action_id = b->first;
        const t_input_event_list &events = b->second;
        if (std::find(events.begin(), events.end(), event) != events.end()) {
            if (!buffer.str().empty()) {
                buffer << ", ";
            }
            buffer << get_action_name(action_id);
        }
    }
    return buffer.str();
}

const std::string &input_manager::get_action_name(const std::string &action) const
{
    t_string_string_map::const_iterator a = actionID_to_name.find(action);
    if (a != actionID_to_name.end()) {
        return a->second;
    }
    // default: return the action id
    return action;
}

const std::string CATA_ERROR = "ERROR";
const std::string ANY_INPUT = "ANY_INPUT";
const std::string COORDINATE = "COORDINATE";
const std::string TIMEOUT = "TIMEOUT";

const std::string &input_context::input_to_action(input_event &inp)
{
    for( size_t i = 0; i < registered_actions.size(); ++i ) {
        const std::string &action = registered_actions[i];
        const std::vector<input_event> &check_inp = inp_mngr.get_input_for_action(action, category);

        // Does this action have our queried input event in its keybindings?
        for( size_t i = 0; i < check_inp.size(); ++i ) {
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
    input_timeout = delay;
}


void input_context::register_action(const std::string &action_descriptor)
{
    if(action_descriptor == "ANY_INPUT") {
        registered_any_input = true;
    } else if(action_descriptor == "COORDINATE") {
        handling_coordinate_input = true;
    }

    registered_actions.push_back(action_descriptor);
}

const std::string input_context::get_desc(const std::string &action_descriptor)
{
    if(action_descriptor == "ANY_INPUT") {
        return "(*)"; // * for wildcard
    }

    const std::vector<input_event> &events = inp_mngr.get_input_for_action(action_descriptor, category);

    if(events.empty()) {
        return _("Unbound!");
    }

    std::vector<input_event> inputs_to_show;
    for( size_t i = 0; i < events.size(); ++i ) {
        const input_event &event = events[i];

        // Only display gamepad buttons if a gamepad is available.
        if(gamepad_available() || event.type != CATA_INPUT_GAMEPAD) {
            inputs_to_show.push_back(event);
        }
    }

    std::stringstream rval;
    for( size_t i = 0; i < inputs_to_show.size(); ++i ) {
        for( size_t j = 0; j < inputs_to_show[i].sequence.size(); ++j ) {
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

const std::string &input_context::handle_input()
{
    next_action.type = CATA_INPUT_ERROR;
    while(1) {
        next_action = inp_mngr.get_input_event(NULL);
        if (next_action.type == CATA_INPUT_TIMEOUT) {
            return TIMEOUT;
        }

        const std::string &action = input_to_action(next_action);

        // Special help action
        if(action == "HELP_KEYBINDINGS") {
            display_help();
            return ANY_INPUT;
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

void input_context::register_directions()
{
    register_cardinal();
    register_action("LEFTUP");
    register_action("LEFTDOWN");
    register_action("RIGHTUP");
    register_action("RIGHTDOWN");
}

void input_context::register_updown()
{
    register_action("UP");
    register_action("DOWN");
}

void input_context::register_leftright()
{
    register_action("LEFT");
    register_action("RIGHT");
}

void input_context::register_cardinal()
{
    register_updown();
    register_leftright();
}

void input_context::get_direction(int &dx, int &dy, const std::string &action)
{
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

void input_context::display_help()
{
    // Shamelessly stolen from help.cpp
    WINDOW *w_help = newwin(FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 2,
                            1 + (int)((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0),
                            1 + (int)((TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0));

    // has the user changed something?
    bool changed = false;
    // keybindings before the user changed anything.
    input_manager::t_action_contexts old_action_contexts(inp_mngr.action_contexts);
    // current status: adding/removing/showing keybindings
    enum { s_remove, s_add, s_add_global, s_show } status = s_show;
    // copy of registered_actions, but without the ANY_INPUT, which should not be shown
    std::vector<std::string> org_registered_actions(registered_actions);
    std::vector<std::string>::iterator any_input = std::find(org_registered_actions.begin(), org_registered_actions.end(), "ANY_INPUT");
    if (any_input != org_registered_actions.end()) {
        org_registered_actions.erase(any_input);
    }

    // colors of the keybindings
    static const nc_color global_key = c_ltgray;
    static const nc_color local_key = c_ltgreen;
    static const nc_color unbound_key = c_ltred;
    // (vertical) scroll offset
    int offset = 0;
    // height of the area usable for display of keybindings, excludes headers & borders
    int display_height = FULL_SCREEN_HEIGHT - 2 - 2; // -2 for the border
    // width of the legend
    const int legwidth = FULL_SCREEN_WIDTH - 51 - 2;
    // keybindings help
    std::ostringstream legend;
    legend << "<color_" << string_from_color(unbound_key) << ">" << _("Unbound keys") << "</color>\n";
    legend << "<color_" << string_from_color(local_key) << ">" << _("Keybinding active only on this screen") << "</color>\n";
    legend << "<color_" << string_from_color(global_key) << ">" << _("Keybinding active globally") << "</color>\n";
    legend << _("Press - to remove keybinding\nPress + to add local keybinding\nPress = to add global keybinding\n");

    while(true) {
        werase(w_help);
        draw_border(w_help);
        draw_scrollbar(w_help, offset, display_height, org_registered_actions.size(), 1);
        mvwprintz(w_help, 0, (FULL_SCREEN_WIDTH - utf8_width(_("Keybindings"))) / 2 - 1,
                c_ltred, " %s ", _("Keybindings"));

        fold_and_print(w_help, 1, 51, legwidth, c_white, legend.str());

        for (size_t i = 0; i + offset < org_registered_actions.size() && i < display_height; i++) {
            const std::string &action_id = org_registered_actions[i + offset];

            bool overwrite_default;
            const std::vector<input_event> &input_events = inp_mngr.get_input_for_action(action_id, category,
                    &overwrite_default);

            const char invlet = i + 'a';
            if (status == s_add_global && overwrite_default) {
                // We're trying to add a global, but this action has a local
                // defined, so gray out the invlet.
                mvwprintz(w_help, i + 1, 2, c_dkgray, "%c ", invlet);
            } else if (status == s_add || status == s_add_global) {
                mvwprintz(w_help, i + 1, 2, c_blue, "%c ", invlet);
            } else if (status == s_remove) {
                mvwprintz(w_help, i + 1, 2, c_blue, "%c ", invlet);
            } else {
                mvwprintz(w_help, i + 1, 2, c_blue, "  ");
            }
            nc_color col;
            if (input_events.empty()) {
                col = unbound_key;
            } else if (overwrite_default) {
                col = local_key;
            } else {
                col = global_key;
            }
            mvwprintz(w_help, i + 1, 4, col, "%s: ", inp_mngr.get_action_name(action_id).c_str());
            mvwprintz(w_help, i + 1, 30, col, "%s", get_desc(action_id).c_str());
        }
        wrefresh(w_help);
        refresh();

        const long ch = getch();
        if (ch == '+') {
            status = s_add;
        } else if (ch == '=') {
            status = s_add_global;
        } else if (ch == '-') {
            status = s_remove;
        } else if (status != s_show && ch >= 'a' && ch <= 'a' + org_registered_actions.size()) {
            const int action = ch - 'a' + offset;
            const std::string &action_id = org_registered_actions[action];
            const std::string name = inp_mngr.get_action_name(action_id);

            // Check if this entry is local or global.
            bool is_local = false;
            inp_mngr.get_input_for_action(action_id, category, &is_local);

            if (status == s_remove && query_yn(_("Clear keys for %s?"), name.c_str())) {

                // If it's global, reset the global actions.
                std::string category_to_access = category;
                if (!is_local) {
                    category_to_access = default_context_id;
                }

                inp_mngr.remove_input_for_action(action_id, category_to_access);
                changed = true;
            } else if (status == s_add_global && is_local) {
                // Disallow adding global actions to an action that already has a local defined.
                popup(_("There are already local keybindings defined for this action, please remove them first."));
            } else if (status == s_add || status == s_add_global) {
                const long newbind = popup_getkey(_("New key for %s:"), name.c_str());
                const input_event new_event(newbind, CATA_INPUT_KEYBOARD);
                const std::string conflicts = inp_mngr.get_conflicts(category, new_event);
                if (!conflicts.empty()) {
                    popup(_("This key conflicts with %s"), conflicts.c_str());
                } else {
                    // We might be adding a local or global action.
                    std::string category_to_access = category;
                    if (status == s_add_global) {
                        category_to_access = default_context_id;
                    }

                    inp_mngr.add_input_for_action(action_id, category_to_access, new_event);
                    changed = true;
                }
            }
            status = s_show;
        } else if (status != s_show) {
            // Pressed some key that is not mapped to an action to edit
            status = s_show;
        } else if (ch == KEY_DOWN && offset + 1 < org_registered_actions.size()) {
            offset++;
        } else if (ch == KEY_UP && offset > 0) {
            offset--;
        } else if (ch == 'q' || ch == 'Q' || ch == KEY_ESCAPE) {
            break;
        }
    }

    if (changed && query_yn(_("Save changes?"))) {
        try {
            inp_mngr.save();
        } catch(std::exception &err) {
            popup(_("saving keybindings failed: %s"), err.what());
        } catch(std::string &err) {
            popup(_("saving keybindings failed: %s"), err.c_str());
        }
    } else if(changed) {
        inp_mngr.action_contexts.swap(old_action_contexts);
    }
    werase(w_help);
    wrefresh(w_help);
    delwin(w_help);
}

input_event input_context::get_raw_input()
{
    return next_action;
}

#ifndef TILES
// If we're using curses, we need to provide get_input_event() here.
input_event input_manager::get_input_event(WINDOW *win)
{
    (void)win; // unused
    int key = get_keypress();
    input_event rval;
    if (key == ERR) {
        if (input_timeout > 0) {
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
            if (event.bstate & BUTTON1_CLICKED) {
                rval.add_input(MOUSE_BUTTON_LEFT);
            } else if (event.bstate & BUTTON3_CLICKED) {
                rval.add_input(MOUSE_BUTTON_RIGHT);
            } else if (event.bstate & REPORT_MOUSE_POSITION) {
                rval.add_input(MOUSE_MOVE);
                if (input_timeout > 0) {
                    // Mouse movement seems to clear ncurses timeout
                    set_timeout(input_timeout);
                }
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

    return rval;
}

// Also specify that we don't have a gamepad plugged in.
bool gamepad_available()
{
    return false;
}

bool input_context::get_coordinates(WINDOW *capture_win, int &x, int &y)
{
    if (!coordinate_input_received) {
        return false;
    }
    int view_columns = getmaxx(capture_win);
    int view_rows = getmaxy(capture_win);
    int win_left = getbegx(capture_win) - VIEW_OFFSET_X;
    int win_right = win_left + view_columns - 1;
    int win_top = getbegy(capture_win) - VIEW_OFFSET_Y;
    int win_bottom = win_top + view_rows - 1;
    if (coordinate_x < win_left || coordinate_x > win_right || coordinate_y < win_top ||
        coordinate_y > win_bottom) {
        return false;
    }

    x = g->ter_view_x - ((view_columns / 2) - coordinate_x);
    y = g->ter_view_y - ((view_rows / 2) - coordinate_y);

    return true;
}
#endif

#ifndef SDLTILES
void init_interface()
{
#if !(defined TILES || defined _WIN32 || defined WINDOWS || defined __CYGWIN__)
    // ncurses mouse registration
    mousemask(BUTTON1_CLICKED | BUTTON3_CLICKED | REPORT_MOUSE_POSITION, NULL);
#endif
}
#endif
