#ifndef MAP_HELPERS_H
#define MAP_HELPERS_H

#include "enums.h"

#include <string>

class monster;

void wipe_map_terrain();
void clear_creatures();
void clear_map();
monster &spawn_test_monster( const std::string &monster_type, const tripoint &start );

#endif
