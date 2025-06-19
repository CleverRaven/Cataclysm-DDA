#pragma once
#ifndef CATA_SRC_AVATAR_ACTION_H
#define CATA_SRC_AVATAR_ACTION_H

#include <optional>
#include <string>
#include <vector>

#include "coordinates.h"
#include "type_id.h"

class Character;
class avatar;
class item;
class item_location;
class map;
class turret_data;

namespace avatar_action
{

/** Eat food or fuel  'E' (or 'a') */
void eat( avatar &you, item_location &loc );
void eat( avatar &you, item_location &loc,
          const std::vector<int> &consume_menu_selections,
          const std::vector<item_location> &consume_menu_selected_items,
          const std::string &consume_menu_filter, activity_id type );
// special rules for eating: grazing etc
// returns false if no rules are needed
bool eat_here( avatar &you );
void eat_or_use( avatar &you, item_location loc );

// Standard movement; handles attacks, traps, &c. Returns false if auto move
// should be canceled
bool move( avatar &you, map &m, const tripoint_rel_ms &d );
/** Handles swimming by the player. Called by avatar_action::move(). */
void swim( map &m, avatar &you, const tripoint_bub_ms &p );

void autoattack( avatar &you, map &m );

void mend( avatar &you, item_location loc );

/**
 * Checks if the weapon is valid and if the player meets certain conditions for firing it.
 * Used for validating ACT_AIM and turret weapon
 * @return True if all conditions are true, otherwise false.
 */
bool can_fire_weapon( avatar &you, const map &m, const item &weapon );

/** Checks if the wielded weapon is a gun and can be fired then starts interactive aiming */
void fire_wielded_weapon( avatar &you );

/** Stores fake gun specified by the mutation and starts interactive aiming */
void fire_ranged_mutation( Character &you, const item &fake_gun );

/** Stores fake gun specified by the bionic and starts interactive aiming */
void fire_ranged_bionic( avatar &you, const item &fake_gun );

/**
 * Checks if the player can manually (with their 2 hands, not via vehicle controls)
 * fire a turret and then starts interactive aiming.
 * Assumes that the turret is on player position.
 * @return true if attempt to fire was successful (aim then cancel is also considered success)
 */
bool fire_turret_manual( avatar &you, map &m, turret_data &turret );

// Throw an item  't'
void plthrow( avatar &you, item_location loc,
              const std::optional<tripoint_bub_ms> &blind_throw_from_pos = std::nullopt );

// Throw the wielded item
void plthrow_wielded( avatar &you,
                      const std::optional<tripoint_bub_ms> &blind_throw_from_pos = std::nullopt );

/**
 * Opens up a menu to Unload a container, gun, or tool
 * If it's a gun, some gunmods can also be loaded
 */
void unload( avatar &you );

// Use item; also tries E,R,W  'a'
void use_item( avatar &you, item_location &loc, std::string const &method = {} );
void use_item( avatar &you );

/** Check if avatar is stealing a weapon. */
bool check_stealing( Character &who, item &weapon );
} // namespace avatar_action

#endif // CATA_SRC_AVATAR_ACTION_H
