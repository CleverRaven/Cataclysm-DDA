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
  oter_id t_above;
  int zlevel;
  mapgendata(oter_id t_north, oter_id t_east, oter_id t_south, oter_id t_west, oter_id t_neast,
              oter_id t_seast, oter_id t_nwest, oter_id t_swest, oter_id up, int z);
  void set_dir(int dir_in, int val);
  void fill(int val);
  int& dir(int dir_in);
  oter_id  north() const { return t_nesw[0]; }
  oter_id  east()  const { return t_nesw[1]; }
  oter_id  south() const { return t_nesw[2]; }
  oter_id  west()  const { return t_nesw[3]; }
  oter_id  neast() const { return t_nesw[4]; }
  oter_id  seast() const { return t_nesw[5]; }
  oter_id  nwest() const { return t_nesw[6]; }
  oter_id  swest() const { return t_nesw[7]; }
  oter_id  above() const { return t_above; }


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
// autogen.sh
void mapgen_office_doctor(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_apartments_con_tower_1_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_apartments_con_tower_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_apartments_mod_tower_1_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_apartments_mod_tower_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_office_tower_1_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_office_tower_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_office_tower_b_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_office_tower_b(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cathedral_1_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cathedral_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cathedral_b_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cathedral_b(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_sub_station(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_s_garage(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cabin_strange(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cabin_strange_b(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cabin(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_farm(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_farm_field(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_police(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_bank(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_bar(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_pawn(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_mil_surplus(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_furniture(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_abstorefront(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_megastore_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_megastore(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hospital_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hospital(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_public_works_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_public_works(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_school_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_school_2(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_school_3(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_school_4(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_school_5(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_school_6(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_school_7(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_school_8(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_school_9(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_2(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_3(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_4(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_5(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_6(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_7(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_8(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_9(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_b(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_prison_b_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_1_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_1_2(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_1_3(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_1_4(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_1_5(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_1_6(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_1_7(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_1_8(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_1_9(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_b_1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_b_2(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hotel_tower_b_3(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_mansion_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_mansion(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_fema_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_fema(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_station_radio(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_lab(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_lab_stairs(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_lab_core(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_lab_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_ice_lab(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_ice_lab_stairs(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_ice_lab_core(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_ice_lab_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_nuke_plant_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_nuke_plant(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_bunker(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_outpost(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_silo(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_silo_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_temple(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_temple_stairs(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_temple_core(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_temple_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_sewage_treatment(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_sewage_treatment_hub(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_sewage_treatment_under(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_mine_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_mine_shaft(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_mine(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_mine_down(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_mine_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_spiral_hub(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_spiral(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_radio_tower(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_toxic_dump(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_haz_sar_entrance(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_haz_sar(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_haz_sar_entrance_b1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_haz_sar_b1(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cave(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cave_rat(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_spider_pit_under(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_anthill(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_slimepit(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_slimepit_down(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_triffid_grove(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_triffid_roots(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_triffid_finale(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_cavern(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_rock(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_rift(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_hellmouth(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_subway_station(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_subway_curved(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_subway_four_way(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_subway_straight(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_subway_tee(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void mapgen_sewer_curved(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_sewer_four_way(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_sewer_straight(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_sewer_tee(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void mapgen_ants_curved(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_ants_four_way(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_ants_straight(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_ants_tee(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

void mapgen_ants_food(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_ants_larvae(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_ants_queen(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);
void mapgen_tutorial(map *m, oter_id terrain_type, mapgendata dat, int turn, float density);

//
void init_mapgen_builtin_functions();
#endif
