#include "game.h"
#include "keypress.h"
#include <fstream>

void game::load_keyboard_settings()
{
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
  return;
 }
 while (!fin.eof()) {
  std::string id;
  fin >> id;
  if (id == "")
   getline(fin, id); // Empty line, chomp it
  else if (id[0] != '#') {
   action_id act = look_up_action(id);
   if (act == ACTION_NULL)
    debugmsg("\
Warning!  data/keymap.txt contains an unknown action, \"%s\"\n\
Fix data/keymap.txt at your next chance!", id.c_str());
   else {
    while (fin.peek() != '\n' && !fin.eof()) {
     char ch;
     fin >> ch;
     if (keymap.find(ch) != keymap.end())
      debugmsg("\
Warning!  '%c' assigned twice in the keymap!\n\
%s is being ignored.\n\
Fix data/keymap.txt at your next chance!", ch, id.c_str());
     else
      keymap[ ch ] = act;
    }
   }
  } else {
   getline(fin, id); // Clear the whole line
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
  case ACTION_OPEN:
   return "open";
  case ACTION_CLOSE:
   return "close";
  case ACTION_SMASH:
   return "smash";
  case ACTION_EXAMINE:
   return "examine";
  case ACTION_PICKUP:
   return "pickup";
  case ACTION_BUTCHER:
   return "butcher";
  case ACTION_CHAT:
   return "chat";
  case ACTION_LOOK:
   return "look";
  case ACTION_INVENTORY:
   return "inventory";
  case ACTION_ORGANIZE:
   return "organize";
  case ACTION_USE:
   return "apply";
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
  case ACTION_DROP:
   return "drop";
  case ACTION_DIR_DROP:
   return "drop_adj";
  case ACTION_BIONICS:
   return "bionics";
  case ACTION_WAIT:
   return "wait";
  case ACTION_CRAFT:
   return "craft";
  case ACTION_CONSTRUCT:
   return "construct";
  case ACTION_SLEEP:
   return "sleep";
  case ACTION_TOGGLE_SAFEMODE:
   return "safemode";
  case ACTION_TOGGLE_AUTOSAFE:
   return "autosafe";
  case ACTION_IGNORE_ENEMY:
   return "ignore_enemy";
  case ACTION_SAVE:
   return "save";
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
 for (int i = 0; i < NUM_ACTIONS; i++) {
  if (action_ident( action_id(i) ) == ident)
   return action_id(i);
 }
 return ACTION_NULL;
}

std::string action_name(action_id act)
{
 switch (act) {
  case ACTION_PAUSE:
   return "Pause";
  case ACTION_MOVE_N:
   return "Move North";
  case ACTION_MOVE_NE:
   return "Move Northeast";
  case ACTION_MOVE_E:
   return "Move East";
  case ACTION_MOVE_SE:
   return "Move Southeast";
  case ACTION_MOVE_S:
   return "Move South";
  case ACTION_MOVE_SW:
   return "Move Southwest";
  case ACTION_MOVE_W:
   return "Move West";
  case ACTION_MOVE_NW:
   return "Move Northwest";
  case ACTION_MOVE_DOWN:
   return "Descend Stairs";
  case ACTION_MOVE_UP:
   return "Ascend Stairs";
  case ACTION_OPEN:
   return "Open Door";
  case ACTION_CLOSE:
   return "Close Door";
  case ACTION_SMASH:
   return "Smash Nearby Terrain";
  case ACTION_EXAMINE:
   return "Examine Nearby Terrain";
  case ACTION_PICKUP:
   return "Pick Item(s) Up";
  case ACTION_BUTCHER:
   return "Butcher";
  case ACTION_CHAT:
   return "Chat with NPC";
  case ACTION_LOOK:
   return "Look Around";
  case ACTION_INVENTORY:
   return "Open Inventory";
  case ACTION_ORGANIZE:
   return "Swap Inventory Letters";
  case ACTION_USE:
   return "Apply or Use Item";
  case ACTION_WEAR:
   return "Wear Item";
  case ACTION_TAKE_OFF:
   return "Take Off Worn Item";
  case ACTION_EAT:
   return "Eat";
  case ACTION_READ:
   return "Read";
  case ACTION_WIELD:
   return "Wield";
  case ACTION_PICK_STYLE:
   return "Select Unarmed Style";
  case ACTION_RELOAD:
   return "Reload Wielded Item";
  case ACTION_UNLOAD:
   return "Unload or Empty Wielded Item";
  case ACTION_THROW:
   return "Throw Item";
  case ACTION_FIRE:
   return "Fire Wielded Item";
  case ACTION_FIRE_BURST:
   return "Burst-Fire Wielded Item";
  case ACTION_DROP:
   return "Drop Item";
  case ACTION_DIR_DROP:
   return "Drop Item to Adjacent Tile";
  case ACTION_BIONICS:
   return "View/Activate Bionics";
  case ACTION_WAIT:
   return "Wait for Several Minutes";
  case ACTION_CRAFT:
   return "Craft Items";
  case ACTION_CONSTRUCT:
   return "Construct Terrain";
  case ACTION_SLEEP:
   return "Sleep";
  case ACTION_TOGGLE_SAFEMODE:
   return "Toggle Safemode";
  case ACTION_TOGGLE_AUTOSAFE:
   return "Toggle Auto-Safemode";
  case ACTION_IGNORE_ENEMY:
   return "Ignore Nearby Enemy";
  case ACTION_SAVE:
   return "Save and Quit";
  case ACTION_QUIT:
   return "Commit Suicide";
  case ACTION_PL_INFO:
   return "View Player Info";
  case ACTION_MAP:
   return "View Map";
  case ACTION_MISSIONS:
   return "View Missions";
  case ACTION_FACTIONS:
   return "View Factions";
  case ACTION_MORALE:
   return "View Morale";
  case ACTION_MESSAGES:
   return "View Message Log";
  case ACTION_HELP:
   return "View Help";
  case ACTION_DEBUG:
   return "Debug Menu";
  case ACTION_DISPLAY_SCENT:
   return "View Scentmap";
  case ACTION_TOGGLE_DEBUGMON:
   return "Toggle Debug Messages";
  case ACTION_NULL:
   return "No Action";
 }
 return "Someone forgot to name an action.";
}
