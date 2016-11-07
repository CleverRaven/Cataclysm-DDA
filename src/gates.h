#ifndef GATES_H
#define GATES_H

#include "player.h"
#include "enums.h"
#include "json.h"

namespace gates
{

void load( JsonObject &jo, const std::string &src );
void check();
void reset();

/** opens the gate via player's activity */
void open_gate( const tripoint &pos, player &p );
/** opens the gate immediately */
void open_gate( const tripoint &pos );

};

#endif
