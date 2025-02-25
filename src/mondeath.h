#pragma once
#ifndef CATA_SRC_MONDEATH_H
#define CATA_SRC_MONDEATH_H

#include "item_location.h"

class map;
class monster;

namespace mdeath
{
// Drop a body
item_location normal( map *here, monster &z );
// Overkill splatter (also part of normal under conditions)
item_location splatter( map *here, monster &z );
// Hallucination disappears
void disappear( monster &z );
// Broken robot drop
void broken( map *here, monster &z );
} //namespace mdeath

item_location make_mon_corpse( map *here, monster &z, int damageLvl );

#endif // CATA_SRC_MONDEATH_H
