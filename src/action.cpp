#include "action.h"
#include "output.h"
#include "path_info.h"
#include "file_wrapper.h"
#include "debug.h"
#include "game.h"
#include "options.h"
#include <istream>
#include <sstream>
#include <fstream>

std::map<char, action_id> keymap;
std::map<char, action_id> default_keymap;
std::set<action_id> unbound_keymap;

void parse_keymap(std::istream &keymap_txt, std::map<char, action_id> &kmap,
                  bool enable_unbound = false);

void load_keyboard_settings()
{
    // Load the default keymap
    std::istringstream sin;
    sin.str(default_keymap_txt());
    parse_keymap(sin, default_keymap);

    // Load the player's actual keymap
    std::ifstream fin;
    bool loaded_legacy_keymap = false;
    fin.open(PATH_INFO::FILENAMES["keymap"].c_str());
    if (!fin.is_open()) { // It doesn't exist
        // Try it at the legacy location.
        fin.open(PATH_INFO::FILENAMES["legacy_keymap"].c_str());
        if( !fin.is_open() ) {
            // Doesn't exist in either place, output a default keymap.
            assure_dir_exist(PATH_INFO::FILENAMES["config_dir"]);
            std::ofstream fout;
            fout.open(PATH_INFO::FILENAMES["keymap"].c_str());
            fout << default_keymap_txt();
            fout.close();
            fin.open(PATH_INFO::FILENAMES["keymap"].c_str());
        } else {
            loaded_legacy_keymap = true;
        }
    }
    if (!fin.is_open()) { // Still can't open it--probably bad permissions
        debugmsg(std::string("Can't open " + PATH_INFO::FILENAMES["keymap"] +
                             " This may be a permissions issue.").c_str());
        keymap = default_keymap;
        return;
    } else {
        parse_keymap(fin, keymap, true);
    }

    // Check for new defaults, and automatically bind them if possible
    std::map<char, action_id>::iterator d_it;
    for (d_it = default_keymap.begin(); d_it != default_keymap.end(); ++d_it) {
        bool found = false;
        if (unbound_keymap.find(d_it->second) != unbound_keymap.end()) {
            break;
        }
        std::map<char, action_id>::iterator k_it;
        for (k_it = keymap.begin(); k_it != keymap.end(); ++k_it) {
            if (d_it->second == k_it->second) {
                found = true;
                break;
            }
        }
        if (!found && !keymap.count(d_it->first)) {
            keymap[d_it->first] = d_it->second;
        }
    }
    if( loaded_legacy_keymap ) {
        assure_dir_exist(PATH_INFO::FILENAMES["config_dir"]);
        save_keymap();
    }
}

void parse_keymap(std::istream &keymap_txt, std::map<char, action_id> &kmap, bool enable_unbound)
{
    while (!keymap_txt.eof()) {
        std::string id;
        keymap_txt >> id;
        if (id == "") {
            getline(keymap_txt, id);    // Empty line, chomp it
        } else if (id == "unbind") {
            keymap_txt >> id;
            action_id act = look_up_action(id);
            if (act != ACTION_NULL && enable_unbound) {
                unbound_keymap.insert(act);
            }
            break;
        } else if (id[0] != '#') {
            action_id act = look_up_action(id);
            if (act == ACTION_NULL)
                debugmsg("\
Warning! keymap.txt contains an unknown action, \"%s\"\n\
Fix \"%s\" at your next chance!", id.c_str(), PATH_INFO::FILENAMES["keymap"].c_str());
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
Fix \"%s\" at your next chance!", ch, id.c_str(), PATH_INFO::FILENAMES["keymap"].c_str());
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

void save_keymap()
{
    std::ofstream fout;
    fout.open(PATH_INFO::FILENAMES["keymap"].c_str());
    if (!fout) { // It doesn't exist
        debugmsg("Can't open \"%s\".", PATH_INFO::FILENAMES["keymap"].c_str());
        fout.close();
        return;
    }
    for (std::map<char, action_id>::iterator it = keymap.begin(); it != keymap.end(); ++it) {
        fout << action_ident( (*it).second ) << " " << (*it).first << std::endl;
    }
    for (std::set<action_id>::iterator it = unbound_keymap.begin(); it != unbound_keymap.end(); ++it) {
        fout << "unbind" << " " << action_ident(*it) << std::endl;
    }

    fout.close();
}

std::vector<char> keys_bound_to(action_id act)
{
    std::vector<char> ret;
    std::map<char, action_id>::iterator it;
    for (it = keymap.begin(); it != keymap.end(); ++it) {
        if ( (*it).second == act ) {
            ret.push_back( (*it).first );
        }
    }

    return ret;
}

action_id action_from_key(char ch)
{
    const std::map<char, action_id>::const_iterator it = keymap.find(ch);
    if(it == keymap.end()) {
        return ACTION_NULL;
    }
    return it->second;
}

void clear_bindings(action_id act)
{
    std::map<char, action_id>::iterator it;
    for (it = keymap.begin(); it != keymap.end(); ++it) {
        if ( (*it).second == act ) {
            keymap.erase(it);
            it = keymap.begin();
        }
    }
}

std::string action_ident(action_id act)
{
    switch (act) {
    case ACTION_PAUSE:
        return "pause";
    case ACTION_MOVE_N:
        return "move_n";
    case ACTION_MOVE_NE:
        return "move_ne";
    case ACTION_MOVE_E:
        return "move_e";
    case ACTION_MOVE_SE:
        return "move_se";
    case ACTION_MOVE_S:
        return "move_s";
    case ACTION_MOVE_SW:
        return "move_sw";
    case ACTION_MOVE_W:
        return "move_w";
    case ACTION_MOVE_NW:
        return "move_nw";
    case ACTION_MOVE_DOWN:
        return "move_down";
    case ACTION_MOVE_UP:
        return "move_up";
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
    case ACTION_TOGGLE_DEBUGMON:
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
    case ACTION_NULL:
        return "null";
    default:
        return "unknown";
    }
}

action_id look_up_action(std::string ident)
{
    for (int i = 0; i < NUM_ACTIONS; i++) {
        if (action_ident( action_id(i) ) == ident) {
            return action_id(i);
        }
    }
    return ACTION_NULL;
}

std::string action_name(action_id act)
{
    switch (act) {
    case ACTION_PAUSE:
        return _("Pause");
    case ACTION_MOVE_N:
        return _("Move North");
    case ACTION_MOVE_NE:
        return _("Move Northeast");
    case ACTION_MOVE_E:
        return _("Move East");
    case ACTION_MOVE_SE:
        return _("Move Southeast");
    case ACTION_MOVE_S:
        return _("Move South");
    case ACTION_MOVE_SW:
        return _("Move Southwest");
    case ACTION_MOVE_W:
        return _("Move West");
    case ACTION_MOVE_NW:
        return _("Move Northwest");
    case ACTION_MOVE_DOWN:
        return _("Descend Stairs");
    case ACTION_MOVE_UP:
        return _("Ascend Stairs");
    case ACTION_CENTER:
        return _("Center View");
    case ACTION_SHIFT_N:
        return _("Move View North");
    case ACTION_SHIFT_NE:
        return _("Move View Northeast");
    case ACTION_SHIFT_E:
        return _("Move View East");
    case ACTION_SHIFT_SE:
        return _("Move View Southeast");
    case ACTION_SHIFT_S:
        return _("Move View South");
    case ACTION_SHIFT_SW:
        return _("Move View Southwest");
    case ACTION_SHIFT_W:
        return _("Move View West");
    case ACTION_SHIFT_NW:
        return _("Move View Northwest");
    case ACTION_OPEN:
        return _("Open Door");
    case ACTION_CLOSE:
        return _("Close Door");
    case ACTION_SMASH:
        return _("Smash Nearby Terrain");
    case ACTION_EXAMINE:
        return _("Examine Nearby Terrain");
    case ACTION_PICKUP:
        return _("Pick Item(s) Up");
    case ACTION_GRAB:
        return _("Grab a nearby vehicle");
    case ACTION_BUTCHER:
        return _("Butcher");
    case ACTION_CHAT:
        return _("Chat with NPC");
    case ACTION_LOOK:
        return _("Look Around");
    case ACTION_PEEK:
        return _("Peek Around Corners");
    case ACTION_LIST_ITEMS:
        return _("List all items around the player");
    case ACTION_INVENTORY:
        return _("Open Inventory");
    case ACTION_ADVANCEDINV:
        return _("Advanced Inventory management");
    case ACTION_COMPARE:
        return _("Compare two Items");
    case ACTION_ORGANIZE:
        return _("Swap Inventory Letters");
    case ACTION_USE:
        return _("Apply or Use Item");
    case ACTION_USE_WIELDED:
        return _("Apply or Use Wielded Item");
    case ACTION_WEAR:
        return _("Wear Item");
    case ACTION_TAKE_OFF:
        return _("Take Off Worn Item");
    case ACTION_EAT:
        return _("Eat");
    case ACTION_READ:
        return _("Read");
    case ACTION_WIELD:
        return _("Wield");
    case ACTION_PICK_STYLE:
        return _("Select Unarmed Style");
    case ACTION_RELOAD:
        return _("Reload Wielded Item");
    case ACTION_UNLOAD:
        return _("Unload or Empty Wielded Item");
    case ACTION_THROW:
        return _("Throw Item");
    case ACTION_FIRE:
        return _("Fire Wielded Item");
    case ACTION_FIRE_BURST:
        return _("Burst-Fire Wielded Item");
    case ACTION_SELECT_FIRE_MODE:
        return _("Toggle attack mode of Wielded Item");
    case ACTION_DROP:
        return _("Drop Item");
    case ACTION_DIR_DROP:
        return _("Drop Item to Adjacent Tile");
    case ACTION_BIONICS:
        return _("View/Activate Bionics");
    case ACTION_SORT_ARMOR:
        return _("Re-layer armour/clothing");
    case ACTION_WAIT:
        return _("Wait for Several Minutes");
    case ACTION_CRAFT:
        return _("Craft Items");
    case ACTION_RECRAFT:
        return _("Recraft last recipe");
    case ACTION_LONGCRAFT:
        return _("Craft as long as possible");
    case ACTION_CONSTRUCT:
        return _("Construct Terrain");
    case ACTION_DISASSEMBLE:
        return _("Disassemble items");
    case ACTION_SLEEP:
        return _("Sleep");
    case ACTION_CONTROL_VEHICLE:
        return _("Control Vehicle");
    case ACTION_TOGGLE_SAFEMODE:
        return _("Toggle Safemode");
    case ACTION_TOGGLE_AUTOSAFE:
        return _("Toggle Auto-Safemode");
    case ACTION_IGNORE_ENEMY:
        return _("Ignore Nearby Enemy");
    case ACTION_SAVE:
        return _("Save and Quit");
    case ACTION_QUICKSAVE:
        return _("Quicksave");
    case ACTION_QUIT:
        return _("Commit Suicide");
    case ACTION_PL_INFO:
        return _("View Player Info");
    case ACTION_MAP:
        return _("View Map");
    case ACTION_MISSIONS:
        return _("View Missions");
    case ACTION_FACTIONS:
        return _("View Factions");
    case ACTION_KILLS:
        return _("View Kills");
    case ACTION_MORALE:
        return _("View Morale");
    case ACTION_MESSAGES:
        return _("View Message Log");
    case ACTION_HELP:
        return _("View Help");
    case ACTION_DEBUG:
        return _("Debug Menu");
    case ACTION_DISPLAY_SCENT:
        return _("View Scentmap");
    case ACTION_TOGGLE_DEBUGMON:
        return _("Toggle Debug Messages");
    case ACTION_ZOOM_OUT:
        return _("Zoom Out");
    case ACTION_ZOOM_IN:
        return _("Zoom In");
    case ACTION_TOGGLE_SIDEBAR_STYLE:
        return _("Switch Sidebar Style");
    case ACTION_TOGGLE_FULLSCREEN:
        return _("Toggle Fullscreen mode");
    case ACTION_ACTIONMENU:
        return _("Action Menu");
    case ACTION_NULL:
        return _("No Action");
    default:
        return "Someone forgot to name an action.";
    }
}

void get_direction(int &x, int &y, char ch)
{
    x = 0;
    y = 0;
    action_id act;
    if (keymap.find(ch) == keymap.end()) {
        act = ACTION_NULL;
    } else {
        act = keymap[ch];
    }

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

std::string default_keymap_txt()
{
    return "\
# This is the keymapping for Cataclysm.\n\
# You can start a line with # to make it a comment--it will be ignored.\n\
# Blank lines are ignored too.\n\
# Extra whitespace, including tab, is ignored, so format things how you like.\n\
# If you wish to restore defaults, simply remove this file.\n\
\n\
# The format for each line is an action identifier, followed by several\n\
# keys.  Any action may have an unlimited number of keys bound to it.\n\
# If you bind the same key to multiple actions, the second and subsequent\n\
# bindings will be ignored--and you'll get a warning when the game starts.\n\
# Keys are case-sensitive, of course; c and C are different.\n\
 \n\
# WARNING: If you skip an action identifier, there will be no key bound to\n\
# that action!  You will be NOT be warned of this when the game starts.\n\
# If you're going to mess with stuff, maybe you should back this file up?\n\
\n\
# It is okay to split commands across lines.\n\
# pause . 5      is equivalent to:\n\
# pause .\n\
# pause 5\n\
\n\
# Note that movement keybindings ONLY apply to movement (for now).\n\
# That is, binding w to move_n will let you use w to move north, but you\n\
# cannot use w to smash north, examine to the north, etc.\n\
# For now, you must use vikeys, the numpad, or arrow keys for those actions.\n\
# This is planned to change in the future.\n\
\n\
# Finally, there is no support for special keys, like spacebar, Home, and\n\
# so on.  This is not a planned feature, but if it's important to you, please\n\
# let me know.\n\
\n\
# MOVEMENT:\n\
pause     . 5\n\
move_n    k 8\n\
move_ne   u 9\n\
move_e    l 6\n\
move_se   n 3\n\
move_s    j 2\n\
move_sw   b 1\n\
move_w    h 4\n\
move_nw   y 7\n\
move_down >\n\
move_up   <\n\
\n\
# MOVEMENT:\n\
center     :\n\
shift_n    K\n\
shift_e    L\n\
shift_s    J\n\
shift_w    H\n\
\n\
# ENVIRONMENT INTERACTION\n\
open  o\n\
close c\n\
smash s\n\
examine e\n\
advinv /\n\
pickup , g\n\
grab G\n\
butcher B\n\
chat C\n\
look ; x\n\
peek X\n\
listitems V\n\
\n\
# INVENTORY & QUASI-INVENTORY INTERACTION\n\
inventory i\n\
compare I\n\
organize =\n\
apply a\n\
apply_wielded A\n\
wear W\n\
take_off T\n\
eat E\n\
read R\n\
wield w\n\
pick_style _\n\
reload r\n\
unload U\n\
throw t\n\
fire f\n\
#fire_burst F\n\
select_fire_mode F\n\
drop d\n\
drop_adj D\n\
bionics p\n\
sort_armor +\n\
\n\
# LONG TERM & SPECIAL ACTIONS\n\
wait |\n\
craft &\n\
recraft -\n\
construct *\n\
disassemble (\n\
sleep $\n\
control_vehicle ^\n\
safemode !\n\
autosafe \"\n\
ignore_enemy '\n\
save S\n\
quit Q\n\
\n\
# INFO SCREENS\n\
player_data @\n\
map m\n\
missions M\n\
factions #\n\
kills )\n\
morale v\n\
messages P\n\
help ?\n\
zoom_in z\n\
zoom_out Z\n\
\n\
# DEBUG FUNCTIONS\n\
debug_mode ~\n\
# debug Z\n\
# debug_scent -\n\
";
}

// (Press X (or Y)|Try) to Z
std::string press_x(action_id act)
{
    return press_x(act, _("Press "), "", _("Try"));
}
std::string press_x(action_id act, std::string key_bound, std::string key_unbound)
{
    return press_x(act, key_bound, "", key_unbound);
}
std::string press_x(action_id act, std::string key_bound_pre, std::string key_bound_suf,
                    std::string key_unbound)
{
    std::vector<char> keys = keys_bound_to( action_id(act) );
    if (keys.empty()) {
        return key_unbound;
    } else {
        std::string keyed = key_bound_pre.append("");
        for (unsigned j = 0; j < keys.size(); j++) {
            if (keys[j] == '\'' || keys[j] == '"') {
                if (j < keys.size() - 1) {
                    keyed += keys[j];
                    keyed += _(" or ");
                } else {
                    keyed += keys[j];
                }
            } else {
                if (j < keys.size() - 1) {
                    keyed += "'";
                    keyed += keys[j];
                    keyed += _("' or ");
                } else {
                    if (keys[j] == '_') {
                        keyed += _("'_' (underscore)");
                    } else {
                        keyed += "'";
                        keyed += keys[j];
                        keyed += "'";
                    }
                }
            }
        }
        return keyed.append(key_bound_suf.c_str());
    }
}
// ('Z'ing|zing) (\(X( or Y))\))
std::string press_x(action_id act, std::string act_desc)
{
    bool key_after = false;
    bool z_ing = false;
    char zing = tolower(act_desc.at(0));
    std::vector<char> keys = keys_bound_to( action_id(act) );
    if (keys.empty()) {
        return act_desc;
    } else {
        std::string keyed = ("");
        for (unsigned j = 0; j < keys.size(); j++) {
            if (tolower(keys[j]) == zing) {
                if (z_ing) {
                    keyed.replace(1, 1, 1, act_desc.at(0));
                    if (key_after) {
                        keyed += _(" or '");
                        keyed += (islower(act_desc.at(0)) ? toupper(act_desc.at(0))
                                  : tolower(act_desc.at(0)));
                        keyed += "'";
                    } else {
                        keyed += " ('";
                        keyed += (islower(act_desc.at(0)) ? toupper(act_desc.at(0))
                                  : tolower(act_desc.at(0)));
                        keyed += "'";
                        key_after = true;
                    }
                } else {
                    std::string uhh = "";
                    if (keys[j] == '\'' || keys[j] == '"') {
                        uhh += "(";
                        uhh += keys[j];
                        uhh += ")";
                    } else {
                        uhh += "'";
                        uhh += keys[j];
                        uhh += "'";
                    }
                    if(act_desc.length() > 1) {
                        uhh += act_desc.substr(1);
                    }
                    if (keys[j] == '_') {
                        uhh += _(" (underscore)");
                    }
                    keyed.insert(0, uhh);
                    z_ing = true;
                }
            } else {
                if (key_after) {
                    if (keys[j] == '\'' || keys[j] == '"') {
                        keyed += _(" or ");
                        keyed += keys[j];
                    } else if (keys[j] == '_') {
                        keyed += _("or '_' (underscore)");
                    } else {
                        keyed += _(" or '");
                        keyed += keys[j];
                        keyed += "'";
                    }
                } else {
                    if (keys[j] == '\'' || keys[j] == '"') {
                        keyed += " (";
                        keyed += keys[j];
                    } else if (keys[j] == '_') {
                        keyed += _(" ('_' (underscore)");
                    } else {
                        keyed += " ('";
                        keyed += keys[j];
                        keyed += "'";
                    }
                    key_after = true;
                }
            }
        }
        if (!z_ing) {
            keyed.insert(0, act_desc);
        }
        if (key_after) {
            keyed += ")";
        }
        return keyed;
    }
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

// get the key for an action, used in the action menu to give each
// action the hotkey it's bound to
long hotkey_for_action(action_id action)
{
    std::vector<char> keys = keys_bound_to(action);
    if(keys.size() >= 1) {
        return keys[0];
    } else {
        return -1;
    }
}

bool can_butcher_at(int x, int y)
{
    // TODO: unify this with game::butcher
    const int factor = g->u.butcher_factor();
    std::vector<item> &items = g->m.i_at(x, y);
    bool has_corpse, has_item = false;
    inventory crafting_inv = g->crafting_inventory(&g->u);
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].type->id == "corpse" && items[i].corpse != NULL) {
            if (factor != 999) {
                has_corpse = true;
            }
        }
    }
    for (size_t i = 0; i < items.size(); i++) {
        if (items[i].type->id != "corpse" || items[i].corpse == NULL) {
            recipe *cur_recipe = g->get_disassemble_recipe(items[i].type->id);
            if (cur_recipe != NULL && g->can_disassemble(&items[i], cur_recipe, crafting_inv, false)) {
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

    if(g->m.tr_at(x, y) != tr_null) {
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

#define REGISTER_ACTION(name) entries.push_back(uimenu_entry(name, true, hotkey_for_action(name), \
        action_name(name)));
#define REGISTER_CATEGORY(name)  categories_by_int[last_category] = name; \
    entries.push_back(uimenu_entry(last_category, true, -1, \
                                   std::string(":")+name)); \
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


    // Default category is called "back" so we can simply add a link to it
    // in sub-categories.
    std::string category = "back";

    while(1) {
        std::vector<uimenu_entry> entries;
        std::map<int, std::string> categories_by_int;
        int last_category = NUM_ACTIONS + 1;

        if(category == "back") {
            std::vector<std::pair<action_id, int> >::iterator it;
            for (it = sorted_pairs.begin(); it != sorted_pairs.end(); it++) {
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
            REGISTER_ACTION(ACTION_SAVE);
            if (hotkey_for_action(ACTION_QUIT) >-1) {
                REGISTER_ACTION(ACTION_QUIT);
            }
        } else if(category == "look") {
            REGISTER_ACTION(ACTION_LOOK);
            REGISTER_ACTION(ACTION_PEEK);
            REGISTER_ACTION(ACTION_LIST_ITEMS);
            REGISTER_CATEGORY("back");
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
            REGISTER_CATEGORY("back");
        } else if(category == "debug") {
            REGISTER_ACTION(ACTION_TOGGLE_SIDEBAR_STYLE);
            REGISTER_ACTION(ACTION_TOGGLE_FULLSCREEN);
            REGISTER_ACTION(ACTION_DEBUG);
            REGISTER_ACTION(ACTION_DISPLAY_SCENT);
            REGISTER_ACTION(ACTION_TOGGLE_DEBUGMON);
            REGISTER_ACTION(ACTION_PICKUP);
            REGISTER_ACTION(ACTION_GRAB);
            REGISTER_ACTION(ACTION_BUTCHER);
            REGISTER_CATEGORY("back");
        } else if(category == "interact") {
            REGISTER_ACTION(ACTION_EXAMINE);
            REGISTER_ACTION(ACTION_SMASH);
            REGISTER_ACTION(ACTION_MOVE_DOWN);
            REGISTER_ACTION(ACTION_MOVE_UP);
            REGISTER_ACTION(ACTION_OPEN);
            REGISTER_ACTION(ACTION_CLOSE);
            REGISTER_ACTION(ACTION_CHAT);
            REGISTER_CATEGORY("back");
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
            REGISTER_CATEGORY("back");
        } else if(category == "craft") {
            REGISTER_ACTION(ACTION_CRAFT);
            REGISTER_ACTION(ACTION_RECRAFT);
            REGISTER_ACTION(ACTION_LONGCRAFT);
            REGISTER_ACTION(ACTION_CONSTRUCT);
            REGISTER_ACTION(ACTION_DISASSEMBLE);
            REGISTER_CATEGORY("back");
        } else if(category == "info") {
            REGISTER_ACTION(ACTION_PL_INFO);
            REGISTER_ACTION(ACTION_MAP);
            REGISTER_ACTION(ACTION_MISSIONS);
            REGISTER_ACTION(ACTION_KILLS);
            REGISTER_ACTION(ACTION_FACTIONS);
            REGISTER_ACTION(ACTION_MORALE);
            REGISTER_ACTION(ACTION_MESSAGES);
            REGISTER_ACTION(ACTION_HELP);
            REGISTER_CATEGORY("back");
        } else if(category == "misc") {
            REGISTER_ACTION(ACTION_WAIT);
            REGISTER_ACTION(ACTION_SLEEP);
            REGISTER_ACTION(ACTION_BIONICS);
            REGISTER_ACTION(ACTION_CONTROL_VEHICLE);
            if (use_tiles) { // from options.h
                REGISTER_ACTION(ACTION_ZOOM_OUT);
                REGISTER_ACTION(ACTION_ZOOM_IN);
            };
            REGISTER_CATEGORY("back");
        }

        entries.push_back(uimenu_entry(2 * NUM_ACTIONS, true, KEY_ESCAPE, "Cancel"));

        std::string title = "Actions";
        if(category != "back") {
            title += ": " + category;
        }
        int selection = (int) uimenu(0, 50, 0, title, entries);

        erase();
        g->refresh_all();
        g->draw();

        if(selection == 2 * NUM_ACTIONS) {
            if(category != "back") {
                category = "back";
            } else {
                return ACTION_NULL;
            };
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
    //~ appended to "Close where?" "Pry where?" etc.
    std::string query_text = message + _(" (Direction button)");
    mvwprintw(stdscr, 0, 0, "%s", query_text.c_str());
    wrefresh(stdscr);
    DebugLog() << "calling get_input() for " << message << "\n";
    InputEvent input = get_input();
    if (input == Cancel || input == Close) {
        return false;
    } else {
        get_direction(x, y, input);
    }
    if (x == -2 || y == -2) {
        g->add_msg(_("Invalid direction."));
        return false;
    }
    return true;
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
                g->m.drawsq(g->w_terrain, g->u, x, y, true, true, g->u.xpos(), g->u.ypos());
            }
        }
    }
    if( highlighted ) {
        wrefresh(g->w_terrain);
    }

    std::string query_text = message + _(" (Direction button)");
    mvwprintw(stdscr, 0, 0, "%s", query_text.c_str());
    wrefresh(stdscr);
    DebugLog() << "calling get_input() for " << message << "\n";
    InputEvent input = get_input();
    if (input == Cancel || input == Close) {
        return false;
    } else {
        get_direction(x, y, input);
    }
    if (x == -2 || y == -2) {
        g->add_msg(_("Invalid direction."));
        return false;
    }
    x += g->u.posx;
    y += g->u.posy;
    return true;
}
