#include "action.h"
#include "output.h"
#include "options.h"
#include "path_info.h"
#include "file_wrapper.h"
#include "debug.h"
#include "game.h"
#include "options.h"
#include "messages.h"
#include <istream>
#include <sstream>
#include <fstream>
#include <iterator>

extern input_context get_default_mode_input_context();

void parse_keymap(std::istream &keymap_txt, std::map<char, action_id> &kmap,
                  std::set<action_id> &unbound_keymap);

void load_keyboard_settings(std::map<char, action_id> &keymap, std::string &keymap_file_loaded_from,
                            std::set<action_id> &unbound_keymap)
{
    // Load the player's actual keymap
    std::ifstream fin;
    fin.open(FILENAMES["keymap"].c_str());
    if (!fin.is_open()) { // It doesn't exist
        // Try it at the legacy location.
        fin.open(FILENAMES["legacy_keymap"].c_str());
        if (fin.is_open()) {
            keymap_file_loaded_from = FILENAMES["legacy_keymap"];
        }
    } else {
        keymap_file_loaded_from = FILENAMES["keymap"];
    }
    if (!fin.is_open()) { // Still can't open it--probably bad permissions
        return;
    }
    parse_keymap(fin, keymap, unbound_keymap);
}

void parse_keymap(std::istream &keymap_txt, std::map<char, action_id> &kmap,
                  std::set<action_id> &unbound_keymap)
{
    while (!keymap_txt.eof()) {
        std::string id;
        keymap_txt >> id;
        if (id == "") {
            getline(keymap_txt, id);    // Empty line, chomp it
        } else if (id == "unbind") {
            keymap_txt >> id;
            action_id act = look_up_action(id);
            if (act != ACTION_NULL) {
                unbound_keymap.insert(act);
            }
            break;
        } else if (id[0] != '#') {
            action_id act = look_up_action(id);
            if (act == ACTION_NULL)
                debugmsg("\
Warning! keymap.txt contains an unknown action, \"%s\"\n\
Fix \"%s\" at your next chance!", id.c_str(), FILENAMES["keymap"].c_str());
            else {
                while (!keymap_txt.eof()) {
                    char ch;
                    keymap_txt >> std::noskipws >> ch >> std::skipws;
                    if (ch == '\n') {
                        break;
                    } else if (ch != ' ' || keymap_txt.peek() == '\n') {
                        if (kmap.find(ch) != kmap.end()) {
                            debugmsg("\
Warning!  '%c' assigned twice in the keymap!\n\
%s is being ignored.\n\
Fix \"%s\" at your next chance!", ch, id.c_str(), FILENAMES["keymap"].c_str());
                        } else {
                            kmap[ ch ] = act;
                        }
                    }
                }
            }
        } else {
            getline(keymap_txt, id); // Clear the whole line
        }
    }
}

std::vector<char> keys_bound_to(action_id act)
{
    input_context ctxt = get_default_mode_input_context();
    return ctxt.keys_bound_to(action_ident(act));
}

action_id action_from_key(char ch)
{
    input_context ctxt = get_default_mode_input_context();
    input_event event((long) ch, CATA_INPUT_KEYBOARD);
    const std::string action = ctxt.input_to_action(event);
    return look_up_action(action);
}

std::string action_ident(action_id act)
{
    switch (act) {
    case ACTION_PAUSE:
        return "pause";
    case ACTION_MOVE_N:
        return "UP";
    case ACTION_MOVE_NE:
        return "RIGHTUP";
    case ACTION_MOVE_E:
        return "RIGHT";
    case ACTION_MOVE_SE:
        return "RIGHTDOWN";
    case ACTION_MOVE_S:
        return "DOWN";
    case ACTION_MOVE_SW:
        return "LEFTDOWN";
    case ACTION_MOVE_W:
        return "LEFT";
    case ACTION_MOVE_NW:
        return "LEFTUP";
    case ACTION_MOVE_DOWN:
        return "LEVEL_DOWN";
    case ACTION_MOVE_UP:
        return "LEVEL_UP";
    case ACTION_CENTER:
        return "center";
    case ACTION_SHIFT_N:
        return "shift_n";
    case ACTION_SHIFT_NE:
        return "shift_ne";
    case ACTION_SHIFT_E:
        return "shift_e";
    case ACTION_SHIFT_SE:
        return "shift_se";
    case ACTION_SHIFT_S:
        return "shift_s";
    case ACTION_SHIFT_SW:
        return "shift_sw";
    case ACTION_SHIFT_W:
        return "shift_w";
    case ACTION_SHIFT_NW:
        return "shift_nw";
    case ACTION_OPEN:
        return "open";
    case ACTION_CLOSE:
        return "close";
    case ACTION_SMASH:
        return "smash";
    case ACTION_EXAMINE:
        return "examine";
    case ACTION_ADVANCEDINV:
        return "advinv";
    case ACTION_PICKUP:
        return "pickup";
    case ACTION_GRAB:
        return "grab";
    case ACTION_BUTCHER:
        return "butcher";
    case ACTION_CHAT:
        return "chat";
    case ACTION_LOOK:
        return "look";
    case ACTION_PEEK:
        return "peek";
    case ACTION_LIST_ITEMS:
        return "listitems";
    case ACTION_ZONES:
        return "zones";
    case ACTION_INVENTORY:
        return "inventory";
    case ACTION_COMPARE:
        return "compare";
    case ACTION_ORGANIZE:
        return "organize";
    case ACTION_USE:
        return "apply";
    case ACTION_USE_WIELDED:
        return "apply_wielded";
    case ACTION_WEAR:
        return "wear";
    case ACTION_TAKE_OFF:
        return "take_off";
    case ACTION_EAT:
        return "eat";
    case ACTION_READ:
        return "read";
    case ACTION_WIELD:
        return "wield";
    case ACTION_PICK_STYLE:
        return "pick_style";
    case ACTION_RELOAD:
        return "reload";
    case ACTION_UNLOAD:
        return "unload";
    case ACTION_THROW:
        return "throw";
    case ACTION_FIRE:
        return "fire";
    case ACTION_FIRE_BURST:
        return "fire_burst";
    case ACTION_SELECT_FIRE_MODE:
        return "select_fire_mode";
    case ACTION_DROP:
        return "drop";
    case ACTION_DIR_DROP:
        return "drop_adj";
    case ACTION_BIONICS:
        return "bionics";
    case ACTION_MUTATIONS:
        return "mutations";
    case ACTION_SORT_ARMOR:
        return "sort_armor";
    case ACTION_WAIT:
        return "wait";
    case ACTION_CRAFT:
        return "craft";
    case ACTION_RECRAFT:
        return "recraft";
    case ACTION_LONGCRAFT:
        return "long_craft";
    case ACTION_CONSTRUCT:
        return "construct";
    case ACTION_DISASSEMBLE:
        return "disassemble";
    case ACTION_SLEEP:
        return "sleep";
    case ACTION_CONTROL_VEHICLE:
        return "control_vehicle";
    case ACTION_TOGGLE_SAFEMODE:
        return "safemode";
    case ACTION_TOGGLE_AUTOSAFE:
        return "autosafe";
    case ACTION_IGNORE_ENEMY:
        return "ignore_enemy";
    case ACTION_SAVE:
        return "save";
    case ACTION_QUICKSAVE:
        return "quicksave";
    case ACTION_QUIT:
        return "quit";
    case ACTION_PL_INFO:
        return "player_data";
    case ACTION_MAP:
        return "map";
    case ACTION_MISSIONS:
        return "missions";
    case ACTION_FACTIONS:
        return "factions";
    case ACTION_KILLS:
        return "kills";
    case ACTION_MORALE:
        return "morale";
    case ACTION_MESSAGES:
        return "messages";
    case ACTION_HELP:
        return "help";
    case ACTION_DEBUG:
        return "debug";
    case ACTION_DISPLAY_SCENT:
        return "debug_scent";
    case ACTION_TOGGLE_DEBUG_MODE:
        return "debug_mode";
    case ACTION_ZOOM_OUT:
        return "zoom_out";
    case ACTION_ZOOM_IN:
        return "zoom_in";
    case ACTION_TOGGLE_SIDEBAR_STYLE:
        return "toggle_sidebar_style";
    case ACTION_TOGGLE_FULLSCREEN:
        return "toggle_fullscreen";
    case ACTION_ACTIONMENU:
        return "action_menu";
    case ACTION_ITEMACTION:
        return "item_action_menu";
    case ACTION_NULL:
        return "null";
    default:
        return "unknown";
    }
}

action_id look_up_action(std::string ident)
{
    // Temporarily for the interface with the input manager!
    if (ident == "move_nw") {
        return ACTION_MOVE_NW;
    } else if (ident == "move_sw") {
        return ACTION_MOVE_SW;
    } else if (ident == "move_ne") {
        return ACTION_MOVE_NE;
    } else if (ident == "move_se") {
        return ACTION_MOVE_SE;
    } else if (ident == "move_n") {
        return ACTION_MOVE_N;
    } else if (ident == "move_s") {
        return ACTION_MOVE_S;
    } else if (ident == "move_w") {
        return ACTION_MOVE_W;
    } else if (ident == "move_e") {
        return ACTION_MOVE_E;
    } else if (ident == "move_down") {
        return ACTION_MOVE_DOWN;
    } else if (ident == "move_up") {
        return ACTION_MOVE_UP;
    }
    // ^^ Temporarily for the interface with the input manager!
    for (int i = 0; i < NUM_ACTIONS; i++) {
        if (action_ident( action_id(i) ) == ident) {
            return action_id(i);
        }
    }
    return ACTION_NULL;
}

void get_direction(int &x, int &y, char ch)
{
    x = 0;
    y = 0;
    action_id act = action_from_key(ch);

    switch (act) {
    case ACTION_MOVE_NW:
        x = -1;
        y = -1;
        return;
    case ACTION_MOVE_NE:
        x = 1;
        y = -1;
        return;
    case ACTION_MOVE_W:
        x = -1;
        return;
    case ACTION_MOVE_S:
        y = 1;
        return;
    case ACTION_MOVE_N:
        y = -1;
        return;
    case ACTION_MOVE_E:
        x = 1;
        return;
    case ACTION_MOVE_SW:
        x = -1;
        y = 1;
        return;
    case ACTION_MOVE_SE:
        x = 1;
        y = 1;
        return;
    case ACTION_PAUSE:
    case ACTION_PICKUP:
        x = 0;
        y = 0;
        return;
    default:
        x = -2;
        y = -2;
    }
}

// (Press X (or Y)|Try) to Z
std::string press_x(action_id act)
{
    input_context ctxt = get_default_mode_input_context();
    return ctxt.press_x(action_ident(act), _("Press "), "", _("Try"));
}
std::string press_x(action_id act, std::string key_bound, std::string key_unbound)
{
    input_context ctxt = get_default_mode_input_context();
    return ctxt.press_x(action_ident(act), key_bound, "", key_unbound);
}
std::string press_x(action_id act, std::string key_bound_pre, std::string key_bound_suf,
                    std::string key_unbound)
{
    input_context ctxt = get_default_mode_input_context();
    return ctxt.press_x(action_ident(act), key_bound_pre, key_bound_suf, key_unbound);
}

action_id get_movement_direction_from_delta(const int dx, const int dy)
{
    if (dx == 0 && dy == -1) {
        return ACTION_MOVE_N;
    } else if (dx == 1 && dy == -1) {
        return ACTION_MOVE_NE;
    } else if (dx == 1 && dy == 0) {
        return ACTION_MOVE_E;
    } else if (dx == 1 && dy == 1) {
        return ACTION_MOVE_SE;
    } else if (dx == 0 && dy == 1) {
        return ACTION_MOVE_S;
    } else if (dx == -1 && dy == 1) {
        return ACTION_MOVE_SW;
    } else if (dx == -1 && dy == 0) {
        return ACTION_MOVE_W;
    } else {
        return ACTION_MOVE_NW;
    }
}

// Get the key for an action, used in the action menu to give each action the
// hotkey it is bound to.
long hotkey_for_action(action_id action)
{
    std::vector<char> keys = keys_bound_to(action);
    return keys.empty() ? -1 : keys[0];
}

bool can_butcher_at(int x, int y)
{
    // TODO: unify this with game::butcher
    const int factor = g->u.butcher_factor();
    auto items = g->m.i_at(x, y);
    bool has_corpse, has_item = false;
    const inventory &crafting_inv = g->u.crafting_inventory();
    for( auto &items_it : items ) {
        if( items_it.type->id == "corpse" && items_it.corpse != NULL ) {
            if (factor != INT_MIN) {
                has_corpse = true;
            }
        } else {
            const recipe *cur_recipe = get_disassemble_recipe( items_it.type->id );
            if( cur_recipe != NULL &&
                g->u.can_disassemble( &items_it, cur_recipe, crafting_inv, false ) ) {
                has_item = true;
            }
        }
    }
    return has_corpse || has_item;
}

bool can_move_vertical_at(int x, int y, int movez)
{
    // TODO: unify this with game::move_vertical
    if (g->m.has_flag("SWIMMABLE", x, y) && g->m.has_flag(TFLAG_DEEP_WATER, x, y)) {
        if (movez == -1) {
            return !g->u.is_underwater() && !g->u.worn_with_flag("FLOATATION");
        } else {
            return g->u.swim_speed() < 500 || g->u.is_wearing("swim_fins");
        }
    }

    if (movez == -1) {
        return g->m.has_flag("GOES_DOWN", x, y);
    } else {
        return g->m.has_flag("GOES_UP", x, y);
    }
}

bool can_examine_at(int x, int y)
{
    int veh_part = 0;
    vehicle *veh = NULL;

    veh = g->m.veh_at (x, y, veh_part);
    if (veh) {
        return true;
    }
    if (g->m.has_flag("CONSOLE", x, y)) {
        return true;
    }
    const furn_t *xfurn_t = &furnlist[g->m.furn(x, y)];
    const ter_t *xter_t = &terlist[g->m.ter(x, y)];

    if (g->m.has_furn(x, y) && xfurn_t->examine != &iexamine::none) {
        return true;
    } else if(xter_t->examine != &iexamine::none) {
        return true;
    }

    const trap_id t = g->m.tr_at( x, y );
    if( t != tr_null && traplist[t]->can_see( g->u, x, y ) ) {
        return true;
    }

    return false;
}

bool can_interact_at(action_id action, int x, int y)
{
    switch(action) {
    case ACTION_OPEN:
        return g->m.open_door(x, y, !g->m.is_outside(g->u.posx, g->u.posy), true);
        break;
    case ACTION_CLOSE:
        return g->m.close_door(x, y, !g->m.is_outside(g->u.posx, g->u.posy), true);
        break;
    case ACTION_BUTCHER:
        return can_butcher_at(x, y);
    case ACTION_MOVE_UP:
        return can_move_vertical_at(x, y, 1);
    case ACTION_MOVE_DOWN:
        return can_move_vertical_at(x, y, -1);
        break;
    case ACTION_EXAMINE:
        return can_examine_at(x, y);
        break;
    default:
        return false;
        break;
    }
}

action_id handle_action_menu()
{
    const input_context ctxt = get_default_mode_input_context();
    std::string catgname;

#define REGISTER_ACTION(name) entries.push_back(uimenu_entry(name, true, hotkey_for_action(name), \
        ctxt.get_action_name(action_ident(name))));
#define REGISTER_CATEGORY(name)  categories_by_int[last_category] = name; \
    catgname = _(name);\
    catgname += "...";\
    capitalize_letter(catgname,0);\
    entries.push_back(uimenu_entry(last_category, true, -1, catgname)); \
    last_category++;

    // Calculate weightings for the various actions to give the player suggestions
    // Weight >= 200: Special action only available right now
    std::map<action_id, int> action_weightings;

    // Check if we're on a vehicle, if so, vehicle controls should be top.
    {
        int veh_part = 0;
        vehicle *veh = NULL;

        veh = g->m.veh_at(g->u.posx, g->u.posy, veh_part);
        if (veh) {
            // Make it 300 to prioritize it before examining the vehicle.
            action_weightings[ACTION_CONTROL_VEHICLE] = 300;
        }
    }

    // Check if we can perform one of our actions on nearby terrain. If so,
    // display that action at the top of the list.
    for(int dx = -1; dx <= 1; dx++) {
        for(int dy = -1; dy <= 1; dy++) {
            int x = g->u.xpos() + dx;
            int y = g->u.ypos() + dy;
            if(dx != 0 || dy != 0) {
                // Check for actions that work on nearby tiles
                if(can_interact_at(ACTION_OPEN, x, y)) {
                    action_weightings[ACTION_OPEN] = 200;
                }
                if(can_interact_at(ACTION_CLOSE, x, y)) {
                    action_weightings[ACTION_CLOSE] = 200;
                }
                if(can_interact_at(ACTION_EXAMINE, x, y)) {
                    action_weightings[ACTION_EXAMINE] = 200;
                }
            } else {
                // Check for actions that work on own tile only
                if(can_interact_at(ACTION_BUTCHER, x, y)) {
                    action_weightings[ACTION_BUTCHER] = 200;
                }
                if(can_interact_at(ACTION_MOVE_UP, x, y)) {
                    action_weightings[ACTION_MOVE_UP] = 200;
                }
                if(can_interact_at(ACTION_MOVE_DOWN, x, y)) {
                    action_weightings[ACTION_MOVE_DOWN] = 200;
                }
            }
        }
    }

    // sort the map by its weightings
    std::vector<std::pair<action_id, int> > sorted_pairs;
    std::copy(action_weightings.begin(), action_weightings.end(),
              std::back_inserter<std::vector<std::pair<action_id, int> > >(sorted_pairs));
    std::reverse(sorted_pairs.begin(), sorted_pairs.end());


    // Default category is called "back"
    std::string category = "back";

    while(1) {
        std::vector<uimenu_entry> entries;
        uimenu_entry *entry;
        std::map<int, std::string> categories_by_int;
        int last_category = NUM_ACTIONS + 1;

        if(category == "back") {
            std::vector<std::pair<action_id, int> >::iterator it;
            for (it = sorted_pairs.begin(); it != sorted_pairs.end(); ++it) {
                if(it->second >= 200) {
                    REGISTER_ACTION(it->first);
                }
            }

            REGISTER_CATEGORY("look");
            REGISTER_CATEGORY("interact");
            REGISTER_CATEGORY("inventory");
            REGISTER_CATEGORY("combat");
            REGISTER_CATEGORY("craft");
            REGISTER_CATEGORY("info");
            REGISTER_CATEGORY("misc");
            if (hotkey_for_action(ACTION_QUICKSAVE) > -1) {
                REGISTER_ACTION(ACTION_QUICKSAVE);
            }
            REGISTER_ACTION(ACTION_SAVE);
            if (hotkey_for_action(ACTION_QUIT) > -1) {
                REGISTER_ACTION(ACTION_QUIT);
            }
            REGISTER_ACTION(ACTION_HELP);
            if ((entry = &entries.back())) {
                entry->txt += "...";        // help _is_a menu.
            }
            if (hotkey_for_action(ACTION_DEBUG) > -1) {
                REGISTER_CATEGORY("debug"); // register with globalkey
                if ((entry = &entries.back())) {
                    entry->hotkey = hotkey_for_action(ACTION_DEBUG);
                }
            }
        } else if(category == "look") {
            REGISTER_ACTION(ACTION_LOOK);
            REGISTER_ACTION(ACTION_PEEK);
            REGISTER_ACTION(ACTION_LIST_ITEMS);
            REGISTER_ACTION(ACTION_ZONES);
            REGISTER_ACTION(ACTION_MAP);
        } else if(category == "inventory") {
            REGISTER_ACTION(ACTION_INVENTORY);
            REGISTER_ACTION(ACTION_ADVANCEDINV);
            REGISTER_ACTION(ACTION_SORT_ARMOR);
            REGISTER_ACTION(ACTION_DIR_DROP);

            // Everything below here can be accessed through
            // the inventory screen, so it's sorted to the
            // end of the list.
            REGISTER_ACTION(ACTION_DROP);
            REGISTER_ACTION(ACTION_COMPARE);
            REGISTER_ACTION(ACTION_ORGANIZE);
            REGISTER_ACTION(ACTION_USE);
            REGISTER_ACTION(ACTION_WEAR);
            REGISTER_ACTION(ACTION_TAKE_OFF);
            REGISTER_ACTION(ACTION_EAT);
            REGISTER_ACTION(ACTION_READ);
            REGISTER_ACTION(ACTION_WIELD);
            REGISTER_ACTION(ACTION_UNLOAD);
        } else if(category == "debug") {
            REGISTER_ACTION(ACTION_DEBUG);
            if ((entry = &entries.back())) {
                entry->txt += "..."; // debug _is_a menu.
            }
            REGISTER_ACTION(ACTION_TOGGLE_SIDEBAR_STYLE);
#ifndef TILES
            REGISTER_ACTION(ACTION_TOGGLE_FULLSCREEN);
#endif
            REGISTER_ACTION(ACTION_DISPLAY_SCENT);
            REGISTER_ACTION(ACTION_TOGGLE_DEBUG_MODE);
        } else if(category == "interact") {
            REGISTER_ACTION(ACTION_EXAMINE);
            REGISTER_ACTION(ACTION_SMASH);
            REGISTER_ACTION(ACTION_MOVE_DOWN);
            REGISTER_ACTION(ACTION_MOVE_UP);
            REGISTER_ACTION(ACTION_OPEN);
            REGISTER_ACTION(ACTION_CLOSE);
            REGISTER_ACTION(ACTION_CHAT);
            REGISTER_ACTION(ACTION_PICKUP);
            REGISTER_ACTION(ACTION_GRAB);
            REGISTER_ACTION(ACTION_BUTCHER);
        } else if(category == "combat") {
            REGISTER_ACTION(ACTION_FIRE);
            REGISTER_ACTION(ACTION_RELOAD);
            REGISTER_ACTION(ACTION_SELECT_FIRE_MODE);
            REGISTER_ACTION(ACTION_THROW);
            REGISTER_ACTION(ACTION_FIRE_BURST);
            REGISTER_ACTION(ACTION_PICK_STYLE);
            REGISTER_ACTION(ACTION_TOGGLE_SAFEMODE);
            REGISTER_ACTION(ACTION_TOGGLE_AUTOSAFE);
            REGISTER_ACTION(ACTION_IGNORE_ENEMY);
        } else if(category == "craft") {
            REGISTER_ACTION(ACTION_CRAFT);
            REGISTER_ACTION(ACTION_RECRAFT);
            REGISTER_ACTION(ACTION_LONGCRAFT);
            REGISTER_ACTION(ACTION_CONSTRUCT);
            REGISTER_ACTION(ACTION_DISASSEMBLE);
        } else if(category == "info") {
            REGISTER_ACTION(ACTION_PL_INFO);
            REGISTER_ACTION(ACTION_MISSIONS);
            REGISTER_ACTION(ACTION_KILLS);
            REGISTER_ACTION(ACTION_FACTIONS);
            REGISTER_ACTION(ACTION_MORALE);
            REGISTER_ACTION(ACTION_MESSAGES);
        } else if(category == "misc") {
            REGISTER_ACTION(ACTION_WAIT);
            REGISTER_ACTION(ACTION_SLEEP);
            REGISTER_ACTION(ACTION_BIONICS);
            REGISTER_ACTION(ACTION_MUTATIONS);
            REGISTER_ACTION(ACTION_CONTROL_VEHICLE);
            REGISTER_ACTION(ACTION_ITEMACTION);
#ifdef TILES
            if (use_tiles) {
                REGISTER_ACTION(ACTION_ZOOM_OUT);
                REGISTER_ACTION(ACTION_ZOOM_IN);
            }
#endif
        }

        std::string title = _("Back");
        title += "...";
        if(category == "back") {
            title = _("Cancel");
        }
        entries.push_back(uimenu_entry(2 * NUM_ACTIONS, true,
                                       hotkey_for_action(ACTION_ACTIONMENU), title));

        title = _("Actions");
        if(category != "back") {
            catgname = _(category.c_str());
            capitalize_letter(catgname, 0);
            title += ": " + catgname;
        }

        int width = 0;
        for( auto &entrie : entries ) {
            if( width < (int)entrie.txt.length() ) {
                width = entrie.txt.length();
            }
        }
        //border=2, selectors=3, after=3 for balance.
        width += 2 + 3 + 3;
        int ix = (TERMX > width) ? (TERMX - width) / 2 - 1 : 0;
        int iy = (TERMY > (int)entries.size() + 2) ? (TERMY - (int)entries.size() - 2) / 2 - 1 : 0;
        int selection = (int) uimenu(true, std::max(ix, 0), std::min(width, TERMX - 2),
                                     std::max(iy, 0), title, entries);

        g->draw();

        if (selection < 0) {
            return ACTION_NULL;
        } else if (selection == 2 * NUM_ACTIONS) {
            if (category != "back") {
                category = "back";
            } else {
                return ACTION_NULL;
            }
        } else if(selection > NUM_ACTIONS) {
            category = categories_by_int[selection];
        } else {
            return (action_id) selection;
        }
    }

#undef REGISTER_ACTION
#undef REGISTER_CATEGORY
}

bool choose_direction(const std::string &message, int &x, int &y)
{
    input_context ctxt("DEFAULTMODE");
    ctxt.register_directions();
    ctxt.register_action("pause");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS"); // why not?
    //~ appended to "Close where?" "Pry where?" etc.
    std::string query_text = message + _(" (Direction button)");
    mvwprintw(g->w_terrain, 0, 0, "%s", query_text.c_str());
    wrefresh(g->w_terrain);
    const std::string action = ctxt.handle_input();
    if (input_context::get_direction(x, y, action)) {
        return true;
    } else if (action == "pause") {
        x = 0;
        y = 0;
        return true;
    }
    add_msg(_("Invalid direction."));
    return false;
}

bool choose_adjacent(std::string message, int &x, int &y)
{
    if (!choose_direction(message, x, y)) {
        return false;
    }
    x += g->u.posx;
    y += g->u.posy;
    return true;
}

bool choose_adjacent_highlight(std::string message, int &x, int &y,
                               action_id action_to_highlight)
{
    // Highlight nearby terrain according to the highlight function
    bool highlighted = false;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int x = g->u.xpos() + dx;
            int y = g->u.ypos() + dy;

            if(can_interact_at(action_to_highlight, x, y)) {
                highlighted = true;
                g->m.drawsq(g->w_terrain, g->u, x, y, true, true, g->u.xpos() + g->u.view_offset_x,
                            g->u.ypos() + g->u.view_offset_y);
            }
        }
    }
    if( highlighted ) {
        wrefresh(g->w_terrain);
    }
    return choose_adjacent(message, x, y);
}
