#include "monster_helpers.h"

#include "monster.h"

void move_monster_turn( monster &mon )
{
    const int moves = mon.get_speed();
    mon.mod_moves( moves );
    while( mon.get_moves() > 0 ) {
        mon.move();
    }
}
