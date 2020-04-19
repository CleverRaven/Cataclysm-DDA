#pragma once
#ifndef CATA_TESTS_MAP_HELPERS_H
#define CATA_TESTS_MAP_HELPERS_H

#include <string>

#include "type_id.h"

class monster;
struct tripoint;

void wipe_map_terrain();
void clear_creatures();
void clear_npcs();
void clear_fields( int zlevel );
void clear_items( int zlevel );
void clear_map();
void clear_map_and_put_player_underground();
monster &spawn_test_monster( const std::string &monster_type, const tripoint &start );
void clear_vehicles();
void build_test_map( const ter_id &terrain );

#endif // CATA_TESTS_MAP_HELPERS_H
