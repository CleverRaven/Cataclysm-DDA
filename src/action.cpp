#include "action.h"
#include "keypress.h"
#include "output.h"
#include <istream>
#include <sstream>
#include <fstream>

std::map<char, action_id> keymap;
std::map<char, action_id> default_keymap;

void parse_keymap(std::istream &keymap_txt, std::map<char, action_id> &kmap);

void load_keyboard_settings()
{
    // Load the default keymap
    std::istringstream sin;
    sin.str(default_keymap_txt());
    parse_keymap(sin, default_keymap);

    // Load the player's actual keymap
    std::ifstream fin;
    fin.open("data/keymap.txt");
    if (!fin) { // It doesn't exist
        std::ofstream fout;
        fout.open("data/keymap.txt");
        fout << default_keymap_txt();
        fout.close();
        fin.open("data/keymap.txt");
    }
    if (!fin) { // Still can't open it--probably bad permissions
        debugmsg("Can't open data/keymap.txt.  This may be a permissions issue.");
        keymap = default_keymap;
        return;
    } else {
        parse_keymap(fin, keymap);
    }

    // Check for new defaults, and automatically bind them if possible
    std::map<char, action_id>::iterator d_it;
    for (d_it = default_keymap.begin(); d_it != default_keymap.end(); ++d_it) {
        bool found = false;
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
}

void parse_keymap(std::istream &keymap_txt, std::map<char, action_id> &kmap)
{
    while (!keymap_txt.eof()) {
        std::string id;
        keymap_txt >> id;
        if (id == "") {
            getline(keymap_txt, id);    // Empty line, chomp it
        } else if (id[0] != '#') {
            action_id act = look_up_action(id);
            if (act == ACTION_NULL)
                debugmsg("\
Warning!  data/keymap.txt contains an unknown action, \"%s\"\n\
Fix data/keymap.txt at your next chance!", id.c_str());
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
Fix data/keymap.txt at your next chance!", ch, id.c_str());
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
    fout.open("data/keymap.txt");
    if (!fout) { // It doesn't exist
        debugmsg("Can't open data/keymap.txt.");
        fout.close();
        return;
    }
    std::map<char, action_id>::iterator it;
    for (it = keymap.begin(); it != keymap.end(); ++it) {
        fout << action_ident( (*it).second ) << " " << (*it).first << std::endl;
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
