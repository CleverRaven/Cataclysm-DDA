#ifndef _BUILDING_GENERATION_H_
#define _BUILDING_GENERATION_H_

#include "omdata.h"
#include "map.h"

struct mapgendata
{
public:
  oter_id t_nesw[8];
  int n_fac; // dir == 0
  int e_fac; // dir == 1
  int s_fac; // dir == 2
  int w_fac; // dir == 3
  int ne_fac; // dir == 4
  int se_fac; // dir == 5
  int nw_fac; // dir == 6
  int sw_fac; // dir == 7
  
  mapgendata(oter_id t_north, oter_id t_east, oter_id t_south, oter_id t_west, oter_id t_neast,
              oter_id t_seast, oter_id t_nwest, oter_id t_swest);
  void set_dir(int dir_in, int val);
  void fill(int val);
  int& dir(int dir_in);
};

ter_id grass_or_dirt();
ter_id dirt_or_pile();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null(map *m);
void mapgen_crater(map *m, mapgendata dat);
void mapgen_field(map *m, int turn);
void mapgen_dirtlot(map *m);
void mapgen_forest_general(map *m, oter_id terrain_type, mapgendata dat, int turn);
void mapgen_hive(map *m, mapgendata dat, int turn);
void mapgen_spider_pit(map *m, mapgendata dat, int turn);
void mapgen_fungal_bloom(map *m);
void mapgen_road_straight(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_road_curved(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_road_tee(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_road_four_way(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_field(map *m, int turn);
void mapgen_bridge(map *m, oter_id terrain_type, int turn);
void mapgen_highway(map *m, oter_id terrain_type, int turn);
void mapgen_river_curved_not(map *m, oter_id terrain_type);
void mapgen_river_straight(map *m, oter_id terrain_type);
void mapgen_river_curved(map *m, oter_id terrain_type);
void mapgen_parking_lot(map *m, mapgendata dat, int turn);
void mapgen_pool(map *m);
void mapgen_park(map *m);
void mapgen_gas_station(map *m, oter_id terrain_type, float density);

#endif
