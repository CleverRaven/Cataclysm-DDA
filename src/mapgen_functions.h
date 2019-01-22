#pragma once
#ifndef MAPGEN_FUNCTIONS_H
#define MAPGEN_FUNCTIONS_H

#include <string>

#include "enums.h"
#include "int_id.h"
#include "weighted_list.h"

class time_point;
struct ter_t;
using ter_id = int_id<ter_t>;
struct furn_t;
using furn_id = int_id<furn_t>;
struct trap;
using trap_id = int_id<trap>;
struct regional_settings;
class map;
struct oter_t;
using oter_id = int_id<oter_t>;
enum field_id : int;
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
                    oter_id t_neast, oter_id t_seast, oter_id t_swest, oter_id t_nwest,
                    oter_id up, oter_id below, int z, const regional_settings &rsettings, map &mp );
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
        void square_groundcover( const int x1, const int y1, const int x2, const int y2 );
        ter_id groundcover();
        bool is_groundcover( const ter_id iid ) const;
        bool has_basement() const;
};

/**
 * Calculates the coordinates of a rotated point.
 * Should match the `mapgen_*` rotation.
 */
tripoint rotate_point( const tripoint &p, int turn );

int terrain_type_to_nesw_array( oter_id terrain_type, bool array[4] );

// @todo: pass mapgendata by reference.
typedef void ( *building_gen_pointer )( map *, oter_id, mapgendata, const time_point &, float );
building_gen_pointer get_mapgen_cfunction( const std::string &ident );
ter_id grass_or_dirt();
ter_id clay_or_sand();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                  float density );
void mapgen_crater( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                    float density );
void mapgen_field( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                   float density );
void mapgen_dirtlot( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                     float density );
void mapgen_forest( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                    float density );
void mapgen_forest_trail_straight( map *m, oter_id terrain_type, mapgendata dat,
                                   const time_point &time, float density );
void mapgen_forest_trail_curved( map *m, oter_id terrain_type, mapgendata dat,
                                 const time_point &time, float density );
void mapgen_forest_trail_tee( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                              float density );
void mapgen_forest_trail_four_way( map *m, oter_id terrain_type, mapgendata dat,
                                   const time_point &time, float density );
void mapgen_hive( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                  float density );
void mapgen_spider_pit( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                        float density );
void mapgen_fungal_bloom( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                          float density );
void mapgen_fungal_tower( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                          float density );
void mapgen_fungal_flowers( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                            float density );
void mapgen_river_center( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                          float density );
void mapgen_road( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                  float density );
void mapgen_field( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                   float density );
void mapgen_bridge( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                    float density );
void mapgen_railroad( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                      float density );
void mapgen_railroad_bridge( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                             float density );
void mapgen_highway( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                     float density );
void mapgen_river_curved_not( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                              float density );
void mapgen_river_straight( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                            float density );
void mapgen_river_curved( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                          float density );
void mapgen_parking_lot( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                         float density );
void mapgen_gas_station( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                         float density );

void mapgen_generic_house( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                           float density, int variant ); // not mapped
void mapgen_generic_house_boxy( map *m, oter_id terrain_type, mapgendata dat,
                                const time_point &time, float density );
void mapgen_generic_house_big_livingroom( map *m, oter_id terrain_type, mapgendata dat,
        const time_point &time, float density );
void mapgen_generic_house_center_hallway( map *m, oter_id terrain_type, mapgendata dat,
        const time_point &time, float density );

void mapgen_pharm( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                   float density );

void mapgen_s_sports( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                      float density );

void mapgen_basement_generic_layout( map *m, oter_id terrain_type, mapgendata dat,
                                     const time_point &time, float density );
void mapgen_basement_junk( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                           float density );
void mapgen_basement_chemlab( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                              float density );
void mapgen_basement_weed( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                           float density );
void mapgen_basement_game( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                           float density );
void mapgen_basement_spiders( map *m, oter_id terrain_type, mapgendata dat, const time_point &time,
                              float density );
// autogen.sh
void mapgen_police( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                    float density );
void mapgen_pawn( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
                  float density );
void mapgen_mil_surplus( map *m, oter_id terrain_type, mapgendata dat, const time_point &turn,
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

// Temporary wrappers
void mremove_trap( map *m, int x, int y );
void mtrap_set( map *m, int x, int y, trap_id type );
void madd_field( map *m, int x, int y, field_id type, int density );

void place_stairs( map *m, oter_id terrain_type, mapgendata dat );

#endif
