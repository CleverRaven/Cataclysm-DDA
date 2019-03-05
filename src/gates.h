#pragma once
#ifndef GATES_H
#define GATES_H

#include <string>

class JsonObject;
class player;
class map;
class Character;
struct tripoint;

namespace gates
{

void load( JsonObject &jo, const std::string &src );
void check();
void reset();

/** opens the gate via player's activity */
void open_gate( const tripoint &pos, player &p );
/** opens the gate immediately */
void open_gate( const tripoint &pos );

}

namespace doors
{

/**
 * Handles deducting moves, printing messages (only non-NPCs cause messages), actually closing it,
 * checking if it can be closed, etc.
*/
void close_door( map &m, Character &who, const tripoint &closep );

}

#endif
