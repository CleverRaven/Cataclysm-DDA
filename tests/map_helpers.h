#pragma once
#ifndef MAP_HELPERS_H
#define MAP_HELPERS_H

#include <string>

#include "enums.h"
#include "map.h"

class monster;

void i_clear_adjacent( const tripoint &p );
void generate_forest_OMT( const tripoint &p, const oter_id f );
void wipe_map_terrain();
void clear_creatures();
void clear_fields( int zlevel );
void clear_map();
void clear_map_and_put_player_underground();
monster &spawn_test_monster( const std::string &monster_type, const tripoint &start );

#endif
