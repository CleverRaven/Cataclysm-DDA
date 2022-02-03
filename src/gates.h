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
 * Handles opening regular, fence and vehicle doors.
 * This function will also handle diving from moving vehicles.
 *
 * @param m map the door is in.
 * @param who character opening the door.
 * @param where location of door on the map.
 * @return true if door was opened, false otherwise.
 */
bool open_door( map &m, Character &who, tripoint where );

/**
 * Handles deducting moves, printing messages (only non-NPCs cause messages), actually closing it,
 * checking if it can be closed, etc.
 */
void close_door( map &m, Creature &who, const tripoint &closep );

} // namespace doors

#endif // CATA_SRC_GATES_H
