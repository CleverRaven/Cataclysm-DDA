#include "game.h"
#include "keypress.h"
#include <fstream>

action_id look_up_action(std::string ident);

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

action_id look_up_action(std::string ident)
{
 if (ident == "pause")
  return ACTION_PAUSE;
 if (ident == "move_n")
  return ACTION_MOVE_N;
 if (ident == "move_ne")
  return ACTION_MOVE_NE;
 if (ident == "move_e")
  return ACTION_MOVE_E;
 if (ident == "move_se")
  return ACTION_MOVE_SE;
 if (ident == "move_s")
  return ACTION_MOVE_S;
 if (ident == "move_sw")
  return ACTION_MOVE_SW;
 if (ident == "move_w")
  return ACTION_MOVE_W;
 if (ident == "move_nw")
  return ACTION_MOVE_NW;
 if (ident == "move_down")
  return ACTION_MOVE_DOWN;
 if (ident == "move_up")
  return ACTION_MOVE_UP;
 if (ident == "open")
  return ACTION_OPEN;
 if (ident == "close")
  return ACTION_CLOSE;
 if (ident == "smash")
  return ACTION_SMASH;
 if (ident == "examine")
  return ACTION_EXAMINE;
 if (ident == "pickup")
  return ACTION_PICKUP;
 if (ident == "butcher")
  return ACTION_BUTCHER;
 if (ident == "chat")
  return ACTION_CHAT;
 if (ident == "look")
  return ACTION_LOOK;
 if (ident == "inventory")
  return ACTION_INVENTORY;
 if (ident == "organize")
  return ACTION_ORGANIZE;
 if (ident == "apply")
  return ACTION_USE;
 if (ident == "wear")
  return ACTION_WEAR;
 if (ident == "take_off")
  return ACTION_TAKE_OFF;
 if (ident == "eat")
  return ACTION_EAT;
 if (ident == "read")
  return ACTION_READ;
 if (ident == "wield")
  return ACTION_WIELD;
 if (ident == "pick_style")
  return ACTION_PICK_STYLE;
 if (ident == "reload")
  return ACTION_RELOAD;
 if (ident == "unload")
  return ACTION_UNLOAD;
 if (ident == "throw")
  return ACTION_THROW;
 if (ident == "fire")
  return ACTION_FIRE;
 if (ident == "fire_burst")
  return ACTION_FIRE_BURST;
 if (ident == "drop")
  return ACTION_DROP;
 if (ident == "drop_adj")
  return ACTION_DIR_DROP;
 if (ident == "bionics")
  return ACTION_BIONICS;
 if (ident == "wait")
  return ACTION_WAIT;
 if (ident == "craft")
  return ACTION_CRAFT;
 if (ident == "construct")
  return ACTION_CONSTRUCT;
 if (ident == "sleep")
  return ACTION_SLEEP;
 if (ident == "safemode")
  return ACTION_TOGGLE_SAFEMODE;
 if (ident == "autosafe")
  return ACTION_TOGGLE_AUTOSAFE;
 if (ident == "ignore_enemy")
  return ACTION_IGNORE_ENEMY;
 if (ident == "save")
  return ACTION_SAVE;
 if (ident == "quit")
  return ACTION_QUIT;
 if (ident == "player_data")
  return ACTION_PL_INFO;
 if (ident == "map")
  return ACTION_MAP;
 if (ident == "missions")
  return ACTION_MISSIONS;
 if (ident == "factions")
  return ACTION_FACTIONS;
 if (ident == "morale")
  return ACTION_MORALE;
 if (ident == "help")
  return ACTION_HELP;
 if (ident == "debug")
  return ACTION_DEBUG;
 if (ident == "debug_scent")
  return ACTION_DISPLAY_SCENT;
 if (ident == "debug_mode")
  return ACTION_TOGGLE_DEBUGMON;

 return ACTION_NULL;
}
