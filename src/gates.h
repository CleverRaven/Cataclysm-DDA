#pragma once
#ifndef CATA_SRC_GATES_H
#define CATA_SRC_GATES_H

#include <string>
#include <vector>

#include "coords_fwd.h"
#include "type_id.h"

class Character;
class Creature;
class JsonObject;
class map;

namespace gates
{

void load( const JsonObject &jo, const std::string &src );
void check();
void reset();

/** opens the gate via player's activity */
void open_gate( const tripoint_bub_ms &pos, Character &p );
/** opens the gate immediately */
void open_gate( const tripoint_bub_ms &pos );

} // namespace gates

namespace doors
{

/**
 * Handles deducting moves, printing messages (only non-NPCs cause messages), actually closing it,
 * checking if it can be closed, etc.
*/
void close_door( map &m, Creature &who, const tripoint_bub_ms &closep );
/**
 * Forcefully closes a door
 * Checks for creatures/items/vehicles at the door tile and attempts to displace them, dealing bash damage.
 * If something remains that prevents the door from closing
 * (e.g. a very big creatures, a vehicle) the door will not be closed
 * @param p position gate is closed from, usually the player's position
 * @param affected_tiles positions of any tiles that are being closed alongside this one
 * @param bash_dmg controls how much damage the door does to the creatures/items/vehicle.
 * If bash_dmg is smaller than 0, _every_ item on the door tile will prevent the door from closing.
 * If bash_dmg is 0, only very small items will do so
 * If bash_dmg is greater than 0, items won't stop the door from closing at all.
 * @returns true if the door can successfully be closed
*/
bool forced_door_closing( const tripoint_bub_ms &p, std::vector<tripoint_bub_ms> affected_tiles,
                          const ter_id &door_type, int bash_dmg );
/**
 * Locks a door at "lockp" as "who."
 *
 * Involves deducting moves, printing messages (only non-NPCs can cause messages),
 * checking if it is locked, performing the action, making sounds, etc.
 * @param check_only prevents actions and returns whether a door can be locked here
 *
 * @returns whether a door was actually locked
 */
bool lock_door( map &m, Creature &who, const tripoint_bub_ms &lockp );

/**
 * Unlocks a door at "lockp" as "who."
 *
 * Involves printing messages (only non-NPCs can cause messages), actually unlocking it,
 * checking if it is locked, performing the action, making sounds, etc.
 *
 * @returns whether a door was actually unlocked
 */
bool unlock_door( map &m, Creature &who, const tripoint_bub_ms &lockp );

/**
* Whether a door at "lockp" can be locked by "who."
*/
bool can_lock_door( const map &m, const Creature &who, const tripoint_bub_ms &lockp );
/**
* Whether a door at "lockp" can be unlocked by "who."
*/
bool can_unlock_door( const map &m, const Creature &who, const tripoint_bub_ms &lockp );

} // namespace doors

#endif // CATA_SRC_GATES_H
