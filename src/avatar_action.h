#pragma once
#ifndef AVATAR_ACTION_H
#define AVATAR_ACTION_H

#include "enums.h"

class avatar;
class map;
struct point;

namespace avatar_action
{
// Standard movement; handles attacks, traps, &c. Returns false if auto move
// should be canceled
bool plmove( avatar &you, map &m, int dx, int dy, int dz = 0 );
inline bool plmove( avatar &you, map &m, const tripoint &d )
{
    return plmove( you, m, d.x, d.y, d.z );
}
inline bool plmove( avatar &you, map &m, const point &d )
{
    return plmove( you, m, d.x, d.y );
}

// Handle moving from a ramp
bool ramp_move( avatar &you, map &m, const tripoint &dest );

void autoattack( avatar &you, map &m );
}


#endif // !AVATAR_MOVE_H
