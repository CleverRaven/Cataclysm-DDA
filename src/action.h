#ifndef ACTION_H
#define ACTION_H

#include <vector>
#include <map>
#include <string>
#include <set>

struct tripoint;

enum action_id : int {
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
    ACTION_TOGGLE_MOVE,
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
    ACTION_ZONES,
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
    ACTION_MUTATIONS,
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
    ACTION_QUICKLOAD,
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
    ACTION_TOGGLE_FULLSCREEN,
    ACTION_DEBUG,
    ACTION_DISPLAY_SCENT,
    ACTION_TOGGLE_DEBUG_MODE,
    ACTION_ZOOM_OUT,
    ACTION_ZOOM_IN,
    ACTION_ACTIONMENU,
    ACTION_ITEMACTION,
    NUM_ACTIONS
};

// Load keybindings from disk
void load_keyboard_settings( std::map<char, action_id> &keymap,
                             std::string &keymap_file_loaded_from,
                             std::set<action_id> &unbound_keymap );
std::string default_keymap_txt();
// All keys bound to act
std::vector<char> keys_bound_to( action_id act );
action_id look_up_action( std::string ident );
std::string action_ident( action_id );
// Lookup key in keymap, return the mapped action or ACTION_NULL
action_id action_from_key( char ch );
// Get input from the player to choose an adjacent tile (for examine() etc)
bool choose_adjacent( std::string message, int &x, int &y );
bool choose_adjacent( std::string message, tripoint &p, bool allow_vertical = false );
// Input from player for a direction, not related to the player position
bool choose_direction( const std::string &message, int &x, int &y );
bool choose_adjacent_highlight( std::string message, int &x, int &y,
                                action_id action_to_highlight );
bool choose_direction( const std::string &message, tripoint &offset, bool allow_vertical = false );
bool choose_adjacent_highlight( std::string message, tripoint &offset,
                                action_id action_to_highlight );

// (Press X (or Y)|Try) to Z
std::string press_x( action_id act );
std::string press_x( action_id act, std::string key_bound,
                     std::string key_unbound );
std::string press_x( action_id act, std::string key_bound_pre,
                     std::string key_bound_suf, std::string key_unbound );
// ('Z'ing|zing) (X( or Y)))
std::string press_x( action_id act, std::string act_desc );

// Helper function to convert co-ordinate delta to a movement direction
action_id get_movement_direction_from_delta( const int dx, const int dy, const int dz = 0 );

action_id handle_action_menu(); // Show the action menu.

/**
 * Check whether we can interact with something using the
 * specified action and the given tile.
 *
 * This is part of a new API that will allow for a more robust
 * user interface. Possible features include: Extending the
 * "select a nearby tile" widget to highlight tiles that can be
 * interacted with. "suggest" context-sensitive actions to the
 * user that are currently relevant.
 */
bool can_interact_at( action_id action, const tripoint &p );

bool can_butcher_at( const tripoint &p );
bool can_move_vertical_at( const tripoint &p, int movez );
bool can_examine_at( const tripoint &p );


#endif
