#pragma once
#ifndef CATA_SRC_GATES_H
#define CATA_SRC_GATES_H

#include <string>

class Character;
class Creature;
class JsonObject;
class map;
struct tripoint;

namespace gates
{

void load( const JsonObject &jo, const std::string &src );
void check();
void reset();

/** opens the gate via player's activity */
void open_gate( const tripoint &pos, Character &p );
/** opens the gate immediately */
void open_gate( const tripoint &pos );

} // namespace gates

namespace doors
{

/**
 * Handles deducting moves, printing messages (only non-NPCs cause messages), actually closing it,
 * checking if it can be closed, etc.
*/
void close_door( map &m, Creature &who, const tripoint &closep );
/**
 * Locks a door at "lockp" as "who."
 *
 * Involves deducting moves, printing messages (only non-NPCs can cause messages),
 * checking if it is locked, performing the action, making sounds, etc.
 * @param check_only prevents actions and returns whether a door can be locked here
 *
 * @returns whether a door was actually locked
 */
bool lock_door( map &m, Creature &who, const tripoint &lockp );

/**
 * Unlocks a door at "lockp" as "who."
 *
 * Involves printing messages (only non-NPCs can cause messages), actually unlocking it,
 * checking if it is locked, performing the action, making sounds, etc.
 *
 * @returns whether a door was actually unlocked
 */
bool unlock_door( map &m, Creature &who, const tripoint &lockp );

/**
* Whether a door at "lockp" can be locked by "who."
*/
bool can_lock_door( const map &m, const Creature &who, const tripoint &lockp );
/**
* Whether a door at "lockp" can be unlocked by "who."
*/
bool can_unlock_door( const map &m, const Creature &who, const tripoint &lockp );

} // namespace doors

#endif // CATA_SRC_GATES_H
