#pragma once
#ifndef CATA_SRC_AVATAR_ACTION_H
#define CATA_SRC_AVATAR_ACTION_H

#include "optional.h"
#include "point.h"
#include "units.h"

class avatar;
class item;
class item_location;
class map;
class turret_data;
class aim_activity_actor;

namespace avatar_action
{

/** Eat food or fuel  'E' (or 'a') */
void eat( avatar &you );
void eat( avatar &you, const item_location &loc, bool open_consume_menu = false );
// special rules for eating: grazing etc
// returns false if no rules are needed
bool eat_here( avatar &you );

// Standard movement; handles attacks, traps, &c. Returns false if auto move
// should be canceled
bool move( avatar &you, map &m, const tripoint &d );
inline bool move( avatar &you, map &m, const point &d )
{
    return move( you, m, tripoint( d, 0 ) );
}

// Handle moving from a ramp
bool ramp_move( avatar &you, map &m, const tripoint &dest );

/** Handles swimming by the player. Called by avatar_action::move(). */
void swim( map &m, avatar &you, const tripoint &p );

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
void fire_ranged_mutation( avatar &you, const item &fake_gun );

/** Stores fake gun specified by the bionic and starts interactive aiming */
void fire_ranged_bionic( avatar &you, const item &fake_gun, const units::energy &cost_per_shot );

/**
 * Checks if the player can manually (with their 2 hands, not via vehicle controls)
 * fire a turret and then starts interactive aiming.
 * Assumes that the turret is on player position.
 */
void fire_turret_manual( avatar &you, map &m, turret_data &turret );

// Throw an item  't'
void plthrow( avatar &you, item_location loc,
              const cata::optional<tripoint> &blind_throw_from_pos = cata::nullopt );

void unload( avatar &you );

// Use item; also tries E,R,W  'a'
void use_item( avatar &you, item_location &loc );
void use_item( avatar &you );
} // namespace avatar_action

#endif // CATA_SRC_AVATAR_ACTION_H
