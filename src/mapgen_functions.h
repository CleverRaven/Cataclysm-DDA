#pragma once
#ifndef MAPGEN_FUNCTIONS_H
#define MAPGEN_FUNCTIONS_H

#include <map>
#include <string>
#include <functional>
#include <utility>

#include "type_id.h"
#include "point.h"
#include "mapgendata.h"

class time_point;
class map;

class mission;

using mapgen_update_func = std::function<void( const tripoint &map_pos3, mission *miss )>;
class JsonObject;

/**
 * Calculates the coordinates of a rotated point.
 * Should match the `mapgen_*` rotation.
 */
tripoint rotate_point( const tripoint &p, int rotations );

int terrain_type_to_nesw_array( oter_id terrain_type, bool array[4] );

using building_gen_pointer = void ( * )( map *, oter_id, mapgendata &, const time_point &, float );
building_gen_pointer get_mapgen_cfunction( const std::string &ident );
ter_id grass_or_dirt();
ter_id clay_or_sand();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                  float density );
void mapgen_crater( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                    float density );
void mapgen_field( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                   float density );
void mapgen_forest( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                    float density );
void mapgen_forest_trail_straight( map *m, oter_id terrain_type, mapgendata &dat,
                                   const time_point &turn, float density );
void mapgen_forest_trail_curved( map *m, oter_id terrain_type, mapgendata &dat,
                                 const time_point &turn, float density );
void mapgen_forest_trail_tee( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                              float density );
void mapgen_forest_trail_four_way( map *m, oter_id terrain_type, mapgendata &dat,
                                   const time_point &turn, float density );
void mapgen_hive( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                  float density );
void mapgen_spider_pit( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                        float density );
void mapgen_fungal_bloom( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                          float density );
void mapgen_fungal_tower( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                          float density );
void mapgen_fungal_flowers( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                            float density );
void mapgen_river_center( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                          float density );
void mapgen_road( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                  float density );
void mapgen_bridge( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                    float density );
void mapgen_railroad( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                      float density );
void mapgen_railroad_bridge( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                             float density );
void mapgen_highway( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                     float density );
void mapgen_river_curved_not( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                              float density );
void mapgen_river_straight( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                            float density );
void mapgen_river_curved( map *m, oter_id terrain_type, mapgendata &dat, const time_point &time,
                          float density );
void mapgen_parking_lot( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                         float density );
void mapgen_generic_house( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                           float density, int variant ); // not mapped
void mapgen_generic_house_boxy( map *m, oter_id terrain_type, mapgendata &dat,
                                const time_point &turn, float density );
void mapgen_generic_house_big_livingroom( map *m, oter_id terrain_type, mapgendata &dat,
        const time_point &turn, float density );
void mapgen_generic_house_center_hallway( map *m, oter_id terrain_type, mapgendata &dat,
        const time_point &turn, float density );
void mapgen_basement_generic_layout( map *m, oter_id terrain_type, mapgendata &dat,
                                     const time_point &time, float density );
void mapgen_basement_junk( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                           float density );
void mapgen_basement_spiders( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                              float density );
void mapgen_cave( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                  float density );
void mapgen_cave_rat( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                      float density );
void mapgen_cavern( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                    float density );
void mapgen_rock( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                  float density );
void mapgen_rock_partial( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                          float density );
void mapgen_open_air( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                      float density );
void mapgen_rift( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                  float density );
void mapgen_hellmouth( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                       float density );
void mapgen_subway( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                    float density );
void mapgen_sewer_curved( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                          float density );
void mapgen_sewer_four_way( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                            float density );
void mapgen_sewer_straight( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                            float density );
void mapgen_sewer_tee( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                       float density );
void mapgen_ants_curved( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                         float density );
void mapgen_ants_four_way( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                           float density );
void mapgen_ants_straight( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                           float density );
void mapgen_ants_tee( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                      float density );
void mapgen_ants_food( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                       float density );
void mapgen_ants_larvae( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                         float density );
void mapgen_ants_queen( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                        float density );
void mapgen_tutorial( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                      float density );
void mapgen_lake_shore( map *m, oter_id terrain_type, mapgendata &dat, const time_point &turn,
                        float density );

// Temporary wrappers
void mremove_trap( map *m, int x, int y );
void mtrap_set( map *m, int x, int y, trap_id type );
void madd_field( map *m, int x, int y, field_type_id type, int intensity );

void place_stairs( map *m, oter_id terrain_type, mapgendata &dat );

mapgen_update_func add_mapgen_update_func( JsonObject &jo, bool &defer );
bool run_mapgen_update_func( const std::string &update_mapgen_id, const tripoint &omt_pos,
                             mission *miss = nullptr, bool cancel_on_collision = true );
bool run_mapgen_func( const std::string &mapgen_id, map *m, oter_id terrain_type, mapgendata &dat,
                      const time_point &turn, float density );
std::pair<std::map<ter_id, int>, std::map<furn_id, int>> get_changed_ids_from_update(
            const std::string &update_mapgen_id );
#endif
