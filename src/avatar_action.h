#pragma once
#ifndef AVATAR_ACTION_H
#define AVATAR_ACTION_H

#include <climits>

#include "optional.h"
#include "point.h"

class avatar;
class item;
class item_location;
class map;
struct targeting_data;

namespace avatar_action
{

/** Eat food or fuel  'E' (or 'a') */
void eat( avatar &you );
void eat( avatar &you, item_location loc );
// special rules for eating: grazing etc
// returns false if no rules are needed
bool eat_here( avatar &you );

// Standard movement; handles attacks, traps, &c. Returns false if auto move
// should be canceled
bool move( avatar &you, map &m, int dx, int dy, int dz = 0 );
inline bool move( avatar &you, map &m, const tripoint &d )
{
    return move( you, m, d.x, d.y, d.z );
}
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
 * @param args Contains item data and targeting mode for the gun we want to fire.
 * @return True if all conditions are true, otherwise false.
 */
bool fire_check( avatar &you, const map &m, const targeting_data &args );

/**
 * Validates avatar's targeting_data, then handles interactive parts of gun firing
 * (target selection, aiming, etc.)
 */
void aim_do_turn( avatar &you, map &m );

/**
 * Validates weapon, stores it into targeting_data and starts interactive aiming.
 * @param weapon Reference to a weapon we want to start aiming.
 * @param bp_cost The amount by which the player's power reserve is decreased after firing.
 */
void fire_weapon( avatar &you, map &m, item &weapon, int bp_cost = 0 );

// Throw an item  't'
void plthrow( avatar &you, item_location loc,
              const cata::optional<tripoint> &blind_throw_from_pos = cata::nullopt );

void unload( avatar &you );

// Use item; also tries E,R,W  'a'
void use_item( avatar &you, item_location &loc );
void use_item( avatar &you );
} // namespace avatar_action

#endif // !AVATAR_MOVE_H
