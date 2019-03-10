#pragma once
#ifndef ACTION_H
#define ACTION_H

#include <map>
#include <set>
#include <string>
#include <vector>

namespace cata
{
template<typename T>
class optional;
} // namespace cata
struct tripoint;

/**
 * Enumerates all discrete actions that can be performed by player
 */
enum action_id : int {
    /** Invalid action used for various lookup errors */
    ACTION_NULL = 0,

    // Mouse actions
    /**@{*/
    /** Click on a point with primary mouse button (usually left button) */
    ACTION_SELECT,
    /** Click on a point with secondary mouse button (usually right button) */
    ACTION_SEC_SELECT,
    /**@}*/

    // Character movement actions
    /**@{*/
    /** Pause an on-going activity. */
    ACTION_PAUSE,
    /** Input timeout */
    ACTION_TIMEOUT,
    /** Move character north */
    ACTION_MOVE_N,
    /** Move character north-east */
    ACTION_MOVE_NE,
    /** Move character east */
    ACTION_MOVE_E,
    /** Move character south-east */
    ACTION_MOVE_SE,
    /** Move character south */
    ACTION_MOVE_S,
    /** Move character south-west */
    ACTION_MOVE_SW,
    /** Move character west */
    ACTION_MOVE_W,
    /** Move character north-west */
    ACTION_MOVE_NW,
    /** Descend a staircase */
    ACTION_MOVE_DOWN,
    /** Ascend a staircase */
    ACTION_MOVE_UP,
    /** Toggle run/walk mode */
    ACTION_TOGGLE_MOVE,
    /**@}*/

    // Viewport movement actions and related
    /**@{*/
    /** Toggle memorized tiles being shown */
    ACTION_TOGGLE_MAP_MEMORY,
    /** Center the viewport on character */
    ACTION_CENTER,
    /** Move viewport north */
    ACTION_SHIFT_N,
    /** Move viewport north-east */
    ACTION_SHIFT_NE,
    /** Move viewport east */
    ACTION_SHIFT_E,
    /** Move viewport south-east */
    ACTION_SHIFT_SE,
    /** Move viewport south */
    ACTION_SHIFT_S,
    /** Move viewport south-west */
    ACTION_SHIFT_SW,
    /** Move viewport west */
    ACTION_SHIFT_W,
    /** Move viewport north-west */
    ACTION_SHIFT_NW,
    /**@}*/

    // Environment Interaction Actions
    /**@{*/
    /** Open an item (e.g. a door) */
    ACTION_OPEN,
    /** Close an item (e.g. a door) */
    ACTION_CLOSE,
    /** Smash something */
    ACTION_SMASH,
    /** Examine or pick up items from adjacent square */
    ACTION_EXAMINE,
    /** Pick up items from current square */
    ACTION_PICKUP,
    /** Grab or let go of an object */
    ACTION_GRAB,
    /** Haul pile of items, or let go of them */
    ACTION_HAUL,
    /** Butcher or disassemble objects in current square */
    ACTION_BUTCHER,
    /** Chat with something */
    ACTION_CHAT,
    /** Toggle look mode */
    ACTION_LOOK,
    /** Peek through something (e.g. out of a curtained window) */
    ACTION_PEEK,
    /** List items and monsters in a given square */
    ACTION_LIST_ITEMS,
    /** Open the zone manager */
    ACTION_ZONES,
    /** Sort out the loot */
    ACTION_LOOT,
    /**@}*/

    // Inventory Interaction (including quasi-inventories like bionics)
    /**@{*/
    /** Open the primary inventory screen */
    ACTION_INVENTORY,
    /** Open the advanced inventory screen */
    ACTION_ADVANCEDINV,
    /** Open the item compare screen */
    ACTION_COMPARE,
    /** Swap inventory letters */
    ACTION_ORGANIZE,
    /** Open the use menu */
    ACTION_USE,
    /** Use currently wielded item */
    ACTION_USE_WIELDED,
    /** Open the wear clothing selection menu */
    ACTION_WEAR,
    /** Open the take-off clothing selection menu */
    ACTION_TAKE_OFF,
    /** Open the consume item menu */
    ACTION_EAT,
    /** Open the read menu */
    ACTION_READ,
    /** Open the wield menu */
    ACTION_WIELD,
    /** Open the martial-arts style menu */
    ACTION_PICK_STYLE,
    /** Open the load item (e.g. firearms) select menu */
    ACTION_RELOAD,
    /** Open the unload item (e.g. firearms) select menu */
    ACTION_UNLOAD,
    /** Open the mending menu (e.g. when using a sewing kit) */
    ACTION_MEND,
    /** Open the throw menu */
    ACTION_THROW,
    /** Fire the wielded weapon, or open fire menu if none */
    ACTION_FIRE,
    /** Burst-fire the current weapon */
    ACTION_FIRE_BURST,
    /** Change fire mode of the current weapon */
    ACTION_SELECT_FIRE_MODE,
    /** Open the drop-item menu */
    ACTION_DROP,
    /** Drop items in a given direction */
    ACTION_DIR_DROP,
    /** Open the bionics menu */
    ACTION_BIONICS,
    /** Open the mutations menu */
    ACTION_MUTATIONS,
    /** Open the armor sorting menu */
    ACTION_SORT_ARMOR,
    /** Auto select and attack hostile creature within range */
    ACTION_AUTOATTACK,
    /**@}*/

    // Long-term / special actions
    /**@{*/
    /** Open wait menu */
    ACTION_WAIT,
    /** Open crafting menu */
    ACTION_CRAFT,
    /** Repeat last craft command */
    ACTION_RECRAFT,
    /** Open batch crafting menu */
    ACTION_LONGCRAFT,
    /** Open construct menu */
    ACTION_CONSTRUCT,
    /** Open disassemble menu */
    ACTION_DISASSEMBLE,
    /** Open sleep menu */
    ACTION_SLEEP,
    /** Open vehicle control menu */
    ACTION_CONTROL_VEHICLE,
    /** Turn safemode on/off, while leaving autosafemode intact */
    ACTION_TOGGLE_SAFEMODE,
    /** Turn automatic triggering of safemode on/off */
    ACTION_TOGGLE_AUTOSAFE,
    /** Ignore the enemy that triggered safemode */
    ACTION_IGNORE_ENEMY,
    /** Whitelist the enemy that triggered safemode */
    ACTION_WHITELIST_ENEMY,
    /** Save the game and quit */
    ACTION_SAVE,
    /** Quicksave the game */
    ACTION_QUICKSAVE,
    /** Quickload the game */
    ACTION_QUICKLOAD,
    /** Quit the game */
    ACTION_QUIT,
    /**@}*/

    // Info Screens
    /**@{*/
    /** Display player status screen */
    ACTION_PL_INFO,
    /** Display over-map */
    ACTION_MAP,
    /** Show sky state for trying to predict weather */
    ACTION_SKY,
    /** Display missions screen */
    ACTION_MISSIONS,
    /** Display kills list screen */
    ACTION_KILLS,
    /** Display factions screen */
    ACTION_FACTIONS,
    /** Display morale effects screen */
    ACTION_MORALE,
    /** Display messages screen */
    ACTION_MESSAGES,
    /** Display help screen */
    ACTION_HELP,
    /** Display main menu */
    ACTION_MAIN_MENU,
    /** Display keybindings list */
    ACTION_KEYBINDINGS,
    /** Display options window */
    ACTION_OPTIONS,
    /** Open autopickup manager */
    ACTION_AUTOPICKUP,
    /** Open safemode manager */
    ACTION_SAFEMODE,
    /** Open color manager */
    ACTION_COLOR,
    /** Open active world mods */
    ACTION_WORLD_MODS,
    /**@}*/

    // Debug Functions
    /**@{*/
    /** Toggle sidebar layout type */
    ACTION_TOGGLE_SIDEBAR_STYLE,
    /** Toggle full-screen mode */
    ACTION_TOGGLE_FULLSCREEN,
    /** Open debug menu */
    ACTION_DEBUG,
    /** Toggle scent map */
    ACTION_DISPLAY_SCENT,
    /** Toggle debug mode */
    ACTION_TOGGLE_DEBUG_MODE,
    /** Zoom view in */
    ACTION_ZOOM_OUT,
    /** Zoom view out */
    ACTION_ZOOM_IN,
    /** Open the action menu */
    ACTION_ACTIONMENU,
    /** Open the item uses menu */
    ACTION_ITEMACTION,
    /** Turn pixel minimap on/off */
    ACTION_TOGGLE_PIXEL_MINIMAP,
    /** Reload current tileset */
    ACTION_RELOAD_TILESET,
    /** Turn auto features on/off */
    ACTION_TOGGLE_AUTO_FEATURES,
    /** Change auto pulp/butcher mode */
    ACTION_TOGGLE_AUTO_PULP_BUTCHER,
    /** Turn auto mining on/off */
    ACTION_TOGGLE_AUTO_MINING,
    /** Turn auto foraging on/off */
    ACTION_TOGGLE_AUTO_FORAGING,
    /** Not an action, serves as count of enumerated actions */
    NUM_ACTIONS
    /**@}*/
};

/**
 *  Load keymap from disk
 *
 *  Sets the state of a keymap in memory to the state of a keymap state saved to disk.
 *  The actual filename we read the keymap from is returned by reference, not specified in this
 *  function call.  The filename used is set elsewhere (in a variety of places).  Take a look at
 *  @ref FILENAMES to see where this happens.  The returned file name is used to detect errors, such
 *  as a non-existent file or a file that didn't actually contain a keymap.
 *
 *  Output is returned as two separate maps:
 *  1. The keymap parameter is used to store the set of keys that were mapped by the file.  This
 *  is not a complete map of all available action IDs, rather it contains only those IDs explicitly
 *  set by the file.
 *
 *  2. The unbound_keymap parameter contains keys that the file specifically unmaps.
 *
 *  The caller of this function is intended to set those keys explicitly set in parameter keymap, and
 *  unset those keys explicitly unbound in parameter unbound_keymap.  Actions and/or keys that are not
 *  mentioned in either output are left in place.  See @ref input_manager::init() for the current way
 *  that this is done.
 *
 *  @param[out] keymap Place in which to store the keys explicitly bound by the file
 *  @param[out] keymap_file_loaded_from Name of file that keymap was loaded from
 *  @param[out] unbound_keymap Place to store the keys explicitly unbound by the file
 */
void load_keyboard_settings( std::map<char, action_id> &keymap,
                             std::string &keymap_file_loaded_from,
                             std::set<action_id> &unbound_keymap );

/**
 * Get list of keys bound to an action ID.
 *
 * Returns a vector of all keys currently bound to the given action.  If not keys are bound to the
 * given action then the returned vector is simply left empty.
 *
 * @param act Action ID to lookup in keymap
 * @returns all keys (as characters) currently bound to a give action ID
 */
std::vector<char> keys_bound_to( action_id act );

/**
 * Lookup an action ID by its unique string identifier
 *
 * Translates a unique string identifier into an @ref action_id.  This identifier is generally the
 * value used in a keymap configuration file.  If no corresponding action_id is found for this
 * identifier then ACTION_NULL is returned instead.
 *
 * @param ident Unique string identifier corresponding to an @ref action_id
 * @returns Corresponding action_id for the supplied string identifier
 */
action_id look_up_action( const std::string &ident );

/**
 * Lookup a unique string identifier for a given action ID.
 *
 * Translates an @ref action_id into a unique string identifier.  This is the value recorded in the
 * keymap configuration file.
 *
 * @note The values we use here are more or less human-readable, but are not always suitable for
 * display directly to the user.
 *
 * @param act The action ID to lookup an identifier for
 * @returns The string identifier for the specified action ID.
 */
std::string action_ident( action_id act );

/**
 * Lookup whether an action can affect the state of the game world.
 *
 * Looks an action ID up and determines if that action can change world state in any case.  This
 * is a static determination from a hard-coded list.
 *
 * This function can be used to count the number of user actions that actually affected the game
 * state separate from other actions that only result in view and menu navigation.  The only current
 * example of this is @ref game::user_action_counter.
 *
 * @param act action ID to lookup in table
 * @returns true if action has potential to alter world state, otherwise returns false.
 */
bool can_action_change_worldstate( const action_id act );

/**
 * Lookup the action ID assigned to a given key.
 *
 * Looks up a key by character and returns the @ref action_id currently mapped to that key.  If no
 * key is currently mapped then ACTION_NULL is returned instead
 *
 * @param ch The character corresponding to the key to look up
 * @returns The action id of the specified key
 */
action_id action_from_key( char ch );

/**
 * Request player input of adjacent tile on current z-level
 *
 * Asks the player to input desired direction of an adjacent tile, for example when executing
 * an examine or directional item drop.  This version of the function assumes that the requested
 * tile will be on the player's current z-level.
 *
 * @param[in] message Message used in assembling the prompt to the player
 */
cata::optional<tripoint> choose_adjacent( const std::string &message );

/**
 * Request player input of adjacent tile, possibly including vertical tiles
 *
 * Asks the player to input desired direction of an adjacent tile, for example when executing
 * an examine or directional item drop.  This version of the function supports selection of tiles
 * above and below the player if an appropriate flag is set.
 *
 * @param[in] message Message used in assembling the prompt to the player
 * @param[in] allow_vertical Allows player to select tiles above/below them if true
 */
cata::optional<tripoint> choose_adjacent( const std::string &message, bool allow_vertical );

/**
 * Request player input of a direction on current z-level
 *
 * Asks the player to input a desired direction.  This differs from @ref choose_adjacent in that
 * the selected direction is returned as an offset to the player's current position rather than
 * coordinate of a tile. This version of the function assumes the requested z-level is the same
 * as the player's current z-level.
 *
 * @param[in] message Message used in assembling the prompt to the player
 */
cata::optional<tripoint> choose_direction( const std::string &message );

/**
 * Request player input of adjacent tile on current z-level with highlighting
 *
 * Asks the player to input desired direction of an adjacent tile, for example when executing
 * an examine or directional item drop.  This version of the function assumes that the requested
 * tile will be on the player's current z-level.
 *
 * This function is identical to @ref choose_adjacent except that squares are highlighted for
 * the player to indicate valid squares for a given @ref action_id
 *
 * @param[in] message Message used in assembling the prompt to the player
 * @param[in] action_to_highlight An action ID to drive the highlighting output
 */
cata::optional<tripoint> choose_adjacent_highlight( const std::string &message,
        action_id action_to_highlight );

/**
 * Request player input of a direction, possibly including vertical component
 *
 * Asks the player to input a desired direction.  This differs from @ref choose_adjacent in that
 * the selected direction is returned as an offset to the player's current position rather than
 * coordinate of a tile.  This version of the function allows selection of the tile above and below
 * the player if the appropriate flag is set.
 *
 * @param[in] message Message used in assembling the prompt to the player
 * @param[in] allow_vertical Allows direction vector to have vertical component if true
 */
cata::optional<tripoint> choose_direction( const std::string &message, bool allow_vertical );

/**
 * Request player input of adjacent tile with highlighting, possibly on different z-level
 *
 * Asks the player to input desired direction of an adjacent tile, for example when executing
 * an examine or directional item drop.  This version of the function allows the player to select
 * a tile above or below.
 *
 * This function is identical to @ref choose_adjacent except that squares are highlighted for
 * the player to indicate valid squares for a given @ref action_id
 *
 * @param[in] message Message used in assembling the prompt to the player
 * @param[in] action_to_highlight An action ID to drive the highlighting output
 */
cata::optional<tripoint> choose_adjacent_highlight( const std::string &message,
        action_id action_to_highlight, bool allow_vertical );

// (Press X (or Y)|Try) to Z
std::string press_x( action_id act );
std::string press_x( action_id act, const std::string &key_bound,
                     const std::string &key_unbound );
std::string press_x( action_id act, const std::string &key_bound_pre,
                     const std::string &key_bound_suf, const std::string &key_unbound );
// ('Z'ing|zing) (X( or Y)))
std::string press_x( action_id act, const std::string &act_desc );

// Helper function to convert co-ordinate delta to a movement direction
/**
 * Translate coordinate delta into movement direction
 *
 * For a given coordinate delta, this function returns the associated user movement action
 * that would generated that delta.  See @ref action_id for the list of available movement
 * commands that may be generated.
 *
 * The only valid values for the parameters of this function are -1, 0 and 1
 *
 * @note: This function does not sanitize its inputs, which can result in some strange behavior:
 * 1. If dz is valid and non-zero, then dx and dy are ignored.
 * 2. If dz is invalid, it is treated as if it were zero.
 * 3. If dz is 0 or invalid, then any invalid dx or dy results in @ref ACTION_MOVE_NW
 * 4. If dz is 0 or invalid, then a dx == dy == 0 results in @ref ACTION_MOVE_NW
 *
 * @param[in] dx X component of direction, should be -1, 0, or 1
 * @param[in] dy Y component of direction, should be -1, 0, or 1
 * @param[in] dz Z component of direction, should be -1, 0, or 1
 * @returns ID of corresponding move action (usually... see note above)
 */
action_id get_movement_direction_from_delta( const int dx, const int dy, const int dz = 0 );

/**
 * Show the action menu
 *
 * Prompts the user with the action menu, and returns any action requested by user input at
 * that menu.
 *
 * @returns action_id ID of action requested by user at menu.
 */
action_id handle_action_menu(); // Show the action menu.

/**
 * Show in-game main menu
 *
 * Prompts the user with the main game menu, and returns any action requested by user input at
 * that menu.
 *
 * @returns action_id ID of action requested by user at menu.
 */
action_id handle_main_menu();

/**
 * Test whether it is possible to perform a given action.
 *
 * Checks whether we can interact with something using the specified action and the given tile.
 *
 * @note: This is part of a new API that will allow for a more robust user interface. Possible
 * features include: Extending the "select a nearby tile" widget to highlight tiles that can be
 * interacted with, "suggest" context-sensitive actions to the user that are currently relevant.
 *
 * @param action The action ID to perform the test for
 * @param p Point to perform test at
 * @returns true if movement is possible in the indicated direction
 */
bool can_interact_at( action_id action, const tripoint &p );

/**
 * Test whether it is possible to perform butcher action
 *
 * Checks whether the butcher action makes sense at a given point.  Checks for both corpses
 * and items that can be disassembled.
 *
 * This is part of a new API that will allow for a more robust user interface.  See the note in
 * @ref can_interact_at()
 *
 * @param p Point to perform the test at
 * @returns true if there is a corpse or item that can be disassembled at a point, otherwise false
 */
bool can_butcher_at( const tripoint &p );

/**
 * Test whether vertical movement is possible
 *
 * Checks whether it is possible to perform up or down movement actions at this location, defined
 * as whether it is possible to swim up/down at this location, or if there is an up or down
 * staircase at this location.
 *
 * This is part of a new API that will allow for a more robust user interface.  See the note in
 * @ref can_interact_at()
 *
 * @param p Point to perform test at
 * @param movez Direction to move. -1 for down, all other values for up
 * @returns true if movement is possible in the indicated direction, otherwise false
 */
bool can_move_vertical_at( const tripoint &p, int movez );

/**
 * Test whether examine is possible
 *
 * Checks whether the examine action makes sense at a given point.
 *
 * This is part of a new API that will allow for a more robust user interface.  See the note in
 * @ref can_interact_at()
 *
 * @param p Point to perform the test at
 * @returns true if the examine action is possible at this point, otherwise false
 */
bool can_examine_at( const tripoint &p );

#endif
