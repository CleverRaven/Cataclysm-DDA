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

typedef void (*building_gen_pointer)(map *,oter_id,mapgendata,int,float);
extern std::map<std::string, building_gen_pointer> mapgen_cfunction_map;

ter_id grass_or_dirt();
ter_id dirt_or_pile();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_crater(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_field(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_field_w_puddles(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_dirtlot(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_forest_general(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hive(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_spider_pit(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_fungal_bloom(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_river_center(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_road_straight(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_road_curved(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_road_tee(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_road_four_way(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_field(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_bridge(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_highway(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_river_curved_not(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_river_straight(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_river_curved(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_parking_lot(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_pool(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_park_playground(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_park_basketball(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_gas_station(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void mapgen_generic_house(map *m, oter_id terrain_type, mapgendata dat, int turn, float density, int variant); // not mapped
void mapgen_generic_house_boxy(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_generic_house_big_livingroom(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_generic_house_center_hallway(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void mapgen_church_new_england(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_church_gothic(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void mapgen_pharm(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_office_cubical(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void mapgen_s_grocery(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_hardware(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_electronics(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_sports(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_liquor(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_gun(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_clothes(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_library(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_restaurant(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_restaurant_fast(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_restaurant_coffee(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void mapgen_shelter(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_shelter_under(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_lmoe(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_lmoe_under(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void mapgen_basement_generic_layout(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_basement_junk(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_basement_guns(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_basement_survivalist(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_basement_chemlab(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_basement_weed(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void init_mapgen_builtin_functions();
void calculate_mapgen_weights();
#endif
