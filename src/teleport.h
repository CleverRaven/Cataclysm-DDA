#pragma once
#ifndef TELEPORT_H
#define TELEPORT_H

class Creature;

namespace teleport
{
/** Teleports a creature to a tile within min_distance and max_distance tiles. Limited to 2D.
*bool safe determines wether the teleported creature can telefrag others/itself.
*/
bool teleport( Creature &critter, int min_distance = 2, int max_distance = 12,
               bool safe = false,
               bool add_teleglow = true );
}

#endif
