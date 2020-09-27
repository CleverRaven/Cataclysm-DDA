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

} // namespace doors

#endif // CATA_SRC_GATES_H
