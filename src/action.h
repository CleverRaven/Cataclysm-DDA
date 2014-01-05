#ifndef _ACTION_H_
#define _ACTION_H_

#include <vector>
#include <map>
#include <string>

enum action_id {
ACTION_NULL = 0,
// Movement
ACTION_PAUSE,
ACTION_MOVE_N,
ACTION_MOVE_NE,
ACTION_MOVE_E,
ACTION_MOVE_SE,
ACTION_MOVE_S,
ACTION_MOVE_SW,
ACTION_MOVE_W,
ACTION_MOVE_NW,
ACTION_MOVE_DOWN,
ACTION_MOVE_UP,
// Shift view
ACTION_CENTER,
ACTION_SHIFT_N,
ACTION_SHIFT_NE,
ACTION_SHIFT_E,
ACTION_SHIFT_SE,
ACTION_SHIFT_S,
ACTION_SHIFT_SW,
ACTION_SHIFT_W,
ACTION_SHIFT_NW,
// Environment Interaction
ACTION_OPEN,
ACTION_CLOSE,
ACTION_SMASH,
ACTION_EXAMINE,
ACTION_PICKUP,
ACTION_GRAB,
ACTION_BUTCHER,
ACTION_CHAT,
ACTION_LOOK,
ACTION_PEEK,
ACTION_LIST_ITEMS,
// Inventory Interaction (including quasi-inventories like bionics)
ACTION_INVENTORY,
ACTION_ADVANCEDINV,
ACTION_COMPARE,
ACTION_ORGANIZE,
ACTION_USE,
ACTION_USE_WIELDED,
ACTION_WEAR,
ACTION_TAKE_OFF,
ACTION_EAT,
ACTION_READ,
ACTION_WIELD,
ACTION_PICK_STYLE,
ACTION_RELOAD,
ACTION_UNLOAD,
ACTION_THROW,
ACTION_FIRE,
ACTION_FIRE_BURST,
ACTION_SELECT_FIRE_MODE,
ACTION_DROP,
ACTION_DIR_DROP,
ACTION_BIONICS,
ACTION_SORT_ARMOR,
// Long-term / special actions
ACTION_WAIT,
ACTION_CRAFT,
ACTION_RECRAFT,
ACTION_LONGCRAFT,
ACTION_CONSTRUCT,
ACTION_DISASSEMBLE,
ACTION_SLEEP,
ACTION_CONTROL_VEHICLE,
ACTION_TOGGLE_SAFEMODE,
ACTION_TOGGLE_AUTOSAFE,
ACTION_IGNORE_ENEMY,
ACTION_SAVE,
ACTION_QUICKSAVE,
ACTION_QUIT,
// Info Screens
ACTION_PL_INFO,
ACTION_MAP,
ACTION_MISSIONS,
ACTION_KILLS,
ACTION_FACTIONS,
ACTION_MORALE,
ACTION_MESSAGES,
ACTION_HELP,
// Debug Functions
ACTION_TOGGLE_SIDEBAR_STYLE,
ACTION_DEBUG,
ACTION_DISPLAY_SCENT,
ACTION_TOGGLE_DEBUGMON,
NUM_ACTIONS
};

extern std::map<char, action_id> keymap;

// Load keybindings from disk
void load_keyboard_settings();
std::string default_keymap_txt();
// Save keybindings to disk
void save_keymap();
// All keys bound to act
std::vector<char> keys_bound_to(action_id act);
// Delete all keys bound to act
void clear_bindings(action_id act);
action_id look_up_action(std::string ident);
std::string action_ident(action_id);
std::string action_name(action_id);
// Lookup key in keymap, return the mapped action or ACTION_NULL
action_id action_from_key(char ch);
// Use the keymap to figure out direction properly
void get_direction(int &x, int &y, char ch);
// (Press X (or Y)|Try) to Z
std::string press_x(action_id act);
std::string press_x(action_id act, std::string key_bound,
                                   std::string key_unbound);
std::string press_x(action_id act, std::string key_bound_pre,
        std::string key_bound_suf, std::string key_unbound);
 // ('Z'ing|zing) (X( or Y)))
std::string press_x(action_id act, std::string act_desc);

// Helper function to convert co-ordinate delta to a movement direction
action_id get_movement_direction_from_delta(const int dx, const int dy);

#endif
