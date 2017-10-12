#ifndef MAP_HELPERS_H
#define MAP_HELPERS_H

#include "enums.h"

#include <string>

void wipe_map_terrain();
void clear_map();
monster &spawn_test_monster( const std::string &monster_type, const tripoint &start );

#endif
