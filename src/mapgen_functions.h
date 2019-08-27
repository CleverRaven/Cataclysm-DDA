#pragma once
#ifndef MAPGEN_FUNCTIONS_H
#define MAPGEN_FUNCTIONS_H

#include <map>
#include <string>
#include <functional>
#include <utility>

#include "type_id.h"
#include "weighted_list.h"
#include "point.h"

class time_point;
struct regional_settings;
class map;

class mission;

using mapgen_update_func = std::function<void( const tripoint &map_pos3, mission *miss )>;
class JsonObject;

namespace om_direction
{
enum class type : int;
} // namespace om_direction

struct mapgendata {
    public:
        oter_id t_nesw[8];
        int n_fac = 0;  // dir == 0
        int e_fac = 0;  // dir == 1
        int s_fac = 0;  // dir == 2
        int w_fac = 0;  // dir == 3
        int ne_fac = 0; // dir == 4
        int se_fac = 0; // dir == 5
        int sw_fac = 0; // dir == 6
        int nw_fac = 0; // dir == 7
        oter_id t_above;
        oter_id t_below;
        int zlevel;
        const regional_settings &region;
        map &m;
        weighted_int_list<ter_id> default_groundcover;
        mapgendata( oter_id t_north, oter_id t_east, oter_id t_south, oter_id t_west,
                    oter_id northeast, oter_id southeast, oter_id southwest, oter_id northwest,
                    oter_id up, oter_id down, int z, const regional_settings &rsettings, map &mp );
        void set_dir( int dir_in, int val );
        void fill( int val );
        int &dir( int dir_in );
        const oter_id &north() const {
            return t_nesw[0];
        }
        const oter_id &east()  const {
            return t_nesw[1];
        }
        const oter_id &south() const {
            return t_nesw[2];
        }
        const oter_id &west()  const {
            return t_nesw[3];
        }
        const oter_id &neast() const {
            return t_nesw[4];
        }
        const oter_id &seast() const {
            return t_nesw[5];
        }
        const oter_id &swest() const {
            return t_nesw[6];
        }
        const oter_id &nwest() const {
            return t_nesw[7];
        }
        const oter_id &above() const {
            return t_above;
        }
        const oter_id &below() const {
            return t_below;
        }
        const oter_id &neighbor_at( om_direction::type dir ) const;
        void fill_groundcover();
        void square_groundcover( int x1, int y1, int x2, int y2 );
        ter_id groundcover();
        bool is_groundcover( const ter_id &iid ) const;
        bool has_basement() const;
};

/**
 * Calculates the coordinates of a rotated point.
 * Should match the `mapgen_*` rotation.
 */
tripoint rotate_point( const tripoint &p, int rotations );

int terrain_type_to_nesw_array( oter_id terrain_type, bool array[4] );

// TODO: pass mapgendata by reference.
using building_gen_pointer = void ( * )( map *, oter_id, mapgendata, const time_point &, float );
building_gen_pointer get_mapgen_cfunction( const std::string &ident );
ter_id grass_or_dirt();
ter_id clay_or_sand();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                  float density );
void mapgen_crater( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                    float density );
void mapgen_field( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                   float density );
void mapgen_forest( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                    float density );
void mapgen_forest_trail_straight( map *m, oter_id terrain_type, mapgendata dat,
                                   const time_point &turn, float density );
void mapgen_forest_trail_curved( map *m, oter_id terrain_type, mapgendata dat,
                                 const time_point &turn, float density );
void mapgen_forest_trail_tee( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                              float density );
void mapgen_forest_trail_four_way( map *m, oter_id terrain_type, mapgendata dat,
                                   const time_point &turn, float density );
void mapgen_hive( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                  float density );
void mapgen_spider_pit( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                        float density );
void mapgen_fungal_bloom( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                          float density );
void mapgen_fungal_tower( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                          float density );
void mapgen_fungal_flowers( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                            float density );
void mapgen_river_center( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                          float density );
void mapgen_road( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                  float density );
void mapgen_bridge( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                    float density );
void mapgen_railroad( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                      float density );
void mapgen_railroad_bridge( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                             float density );
void mapgen_highway( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                     float density );
void mapgen_river_curved_not( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                              float density );
void mapgen_river_straight( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                            float density );
void mapgen_river_curved( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                          float density );
void mapgen_parking_lot( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                         float density );
void mapgen_generic_house( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                           float density, int variant ); // not mapped
void mapgen_generic_house_boxy( map *m, oter_id terrain_type, mapgendata dat,
                                const time_point &turn, float density );
void mapgen_generic_house_big_livingroom( map *m, oter_id terrain_type, mapgendata dat,
        const time_point &turn, float density );
void mapgen_generic_house_center_hallway( map *m, oter_id terrain_type, mapgendata dat,
        const time_point &turn, float density );
void mapgen_basement_generic_layout( map *m, oter_id terrain_type, mapgendata dat,
                                     const time_point &time, float density );
void mapgen_basement_junk( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                           float density );
void mapgen_basement_spiders( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                              float density );
void mapgen_cave( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                  float density );
void mapgen_cave_rat( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                      float density );
void mapgen_cavern( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                    float density );
void mapgen_rock( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                  float density );
void mapgen_rock_partial( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                          float density );
void mapgen_open_air( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                      float density );
void mapgen_rift( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                  float density );
void mapgen_hellmouth( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                       float density );
void mapgen_subway( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                    float density );
void mapgen_sewer_curved( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                          float density );
void mapgen_sewer_four_way( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                            float density );
void mapgen_sewer_straight( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                            float density );
void mapgen_sewer_tee( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                       float density );
void mapgen_ants_curved( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                         float density );
void mapgen_ants_four_way( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                           float density );
void mapgen_ants_straight( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                           float density );
void mapgen_ants_tee( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                      float density );
void mapgen_ants_food( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                       float density );
void mapgen_ants_larvae( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                         float density );
void mapgen_ants_queen( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                        float density );
void mapgen_tutorial( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                      float density );
void mapgen_lake_shore( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                        float density );

// Temporary wrappers
void mremove_trap( map *m, int x, int y );
void mtrap_set( map *m, int x, int y, trap_id type );
void madd_field( map *m, int x, int y, field_type_id type, int intensity );

void place_stairs( map *m, oter_id terrain_type, mapgendata dat );

mapgen_update_func add_mapgen_update_func( JsonObject &jo, bool &defer );
bool run_mapgen_update_func( const std::string &update_mapgen_id, const tripoint &omt_pos,
                             mission *miss = nullptr, bool cancel_on_collision = true );
bool run_mapgen_func( const std::string &mapgen_id, map *m, oter_id terrain_type, mapgendata dat,
                      const time_point &turn, float density );
std::pair<std::map<ter_id, int>, std::map<furn_id, int>> get_changed_ids_from_update(
            const std::string &update_mapgen_id );
#endif
