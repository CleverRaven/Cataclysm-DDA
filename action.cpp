#include "game.h"
#include "keypress.h"
#include "output.h"
#include <istream>
#include <sstream>
#include <fstream>

void parse_keymap(std::istream &keymap_txt, std::map<char, action_id> &kmap);

void game::load_keyboard_settings()
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
 bool found;
 for (d_it = default_keymap.begin(); d_it != default_keymap.end(); d_it++) {
  found = false;
  std::map<char, action_id>::iterator k_it;
  for (k_it = keymap.begin(); k_it != keymap.end(); k_it++) {
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
  if (id == "")
   getline(keymap_txt, id); // Empty line, chomp it
  else if (id[0] != '#') {
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

void game::save_keymap()
{
 std::ofstream fout;
 fout.open("data/keymap.txt");
 if (!fout) { // It doesn't exist
  debugmsg("Can't open data/keymap.txt.");
  fout.close();
  return;
 }
 std::map<char, action_id>::iterator it;
 for (it = keymap.begin(); it != keymap.end(); it++)
  fout << action_ident( (*it).second ) << " " << (*it).first << std::endl;

 fout.close();
}

std::vector<char> game::keys_bound_to(action_id act)
{
 std::vector<char> ret;
 std::map<char, action_id>::iterator it;
 for (it = keymap.begin(); it != keymap.end(); it++) {
  if ( (*it).second == act )
   ret.push_back( (*it).first );
 }

 return ret;
}

void game::clear_bindings(action_id act)
{
 std::map<char, action_id>::iterator it;
 for (it = keymap.begin(); it != keymap.end(); it++) {
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
        }
    return "unknown";
    }

action_id look_up_action(std::string ident)
{
    for (int i = 0; i < NUM_ACTIONS; i++)
    {
        if (action_ident( action_id(i) ) == ident)
        {
            return action_id(i);
        }
    }
    return ACTION_NULL;
}

std::string action_name(action_id act)
{
    switch (act)
    {
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
    }
    return "Someone forgot to name an action.";
}

