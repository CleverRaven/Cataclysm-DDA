#pragma once
#ifndef CATA_SRC_GATES_H
#define CATA_SRC_GATES_H

#include <iosfwd>
#include <string>

#include "type_id.h"

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
 * Get movement cost of opening or closing doors for given character.
 *
 * @param who the character interacting the door.
 * @param door_name the terrain id name of the door.
 * @param open whether to open the door (close if false).
 * @return movement cost of interacting with doors.
 */
unsigned get_action_move_cost( const Character &who, int_id<ter_t> door, const bool open );

} // namespace doors

#endif // CATA_SRC_GATES_H
