#pragma once
#ifndef MAPGEN_FUNCTIONS_H
#define MAPGEN_FUNCTIONS_H

#include <map>
#include <string>
#include <functional>
#include <utility>

#include "type_id.h"

struct tripoint;
struct point;
class time_point;
class map;
class mapgendata;
class mission;

using mapgen_update_func = std::function<void( const tripoint &map_pos3, mission *miss )>;
class JsonObject;

/**
 * Calculates the coordinates of a rotated point.
 * Should match the `mapgen_*` rotation.
 */
tripoint rotate_point( const tripoint &p, int rotations );

int terrain_type_to_nesw_array( oter_id terrain_type, bool array[4] );

using building_gen_pointer = void ( * )( mapgendata & );
building_gen_pointer get_mapgen_cfunction( const std::string &ident );
ter_id grass_or_dirt();
ter_id clay_or_sand();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null( mapgendata &dat );
void mapgen_crater( mapgendata &dat );
void mapgen_field( mapgendata &dat );
void mapgen_forest( mapgendata &dat );
void mapgen_forest_trail_straight( mapgendata &dat );
void mapgen_forest_trail_curved( mapgendata &dat );
void mapgen_forest_trail_tee( mapgendata &dat );
void mapgen_forest_trail_four_way( mapgendata &dat );
void mapgen_hive( mapgendata &dat );
void mapgen_spider_pit( mapgendata &dat );
void mapgen_fungal_bloom( mapgendata &dat );
void mapgen_fungal_tower( mapgendata &dat );
void mapgen_fungal_flowers( mapgendata &dat );
void mapgen_river_center( mapgendata &dat );
void mapgen_road( mapgendata &dat );
void mapgen_bridge( mapgendata &dat );
void mapgen_railroad( mapgendata &dat );
void mapgen_railroad_bridge( mapgendata &dat );
void mapgen_highway( mapgendata &dat );
void mapgen_river_curved_not( mapgendata &dat );
void mapgen_river_straight( mapgendata &dat );
void mapgen_river_curved( mapgendata &dat );
void mapgen_parking_lot( mapgendata &dat );
void mapgen_generic_house( mapgendata &dat, int variant ); // not mapped
void mapgen_generic_house_boxy( mapgendata &dat );
void mapgen_generic_house_big_livingroom( mapgendata &dat );
void mapgen_generic_house_center_hallway( mapgendata &dat );
void mapgen_basement_generic_layout( mapgendata &dat );
void mapgen_basement_junk( mapgendata &dat );
void mapgen_basement_spiders( mapgendata &dat );
void mapgen_cave( mapgendata &dat );
void mapgen_cave_rat( mapgendata &dat );
void mapgen_cavern( mapgendata &dat );
void mapgen_rock( mapgendata &dat );
void mapgen_rock_partial( mapgendata &dat );
void mapgen_open_air( mapgendata &dat );
void mapgen_rift( mapgendata &dat );
void mapgen_hellmouth( mapgendata &dat );
void mapgen_subway( mapgendata &dat );
void mapgen_sewer_curved( mapgendata &dat );
void mapgen_sewer_four_way( mapgendata &dat );
void mapgen_sewer_straight( mapgendata &dat );
void mapgen_sewer_tee( mapgendata &dat );
void mapgen_ants_curved( mapgendata &dat );
void mapgen_ants_four_way( mapgendata &dat );
void mapgen_ants_straight( mapgendata &dat );
void mapgen_ants_tee( mapgendata &dat );
void mapgen_ants_food( mapgendata &dat );
void mapgen_ants_larvae( mapgendata &dat );
void mapgen_ants_queen( mapgendata &dat );
void mapgen_tutorial( mapgendata &dat );
void mapgen_lake_shore( mapgendata &dat );

// Temporary wrappers
void mremove_trap( map *m, int x, int y );
void mtrap_set( map *m, int x, int y, trap_id type );
void madd_field( map *m, int x, int y, field_type_id type, int intensity );

void place_stairs( mapgendata &dat );

mapgen_update_func add_mapgen_update_func( JsonObject &jo, bool &defer );
bool run_mapgen_update_func( const std::string &update_mapgen_id, const tripoint &omt_pos,
                             mission *miss = nullptr, bool cancel_on_collision = true );
bool run_mapgen_func( const std::string &mapgen_id, mapgendata &dat );
std::pair<std::map<ter_id, int>, std::map<furn_id, int>> get_changed_ids_from_update(
            const std::string &update_mapgen_id );
#endif
