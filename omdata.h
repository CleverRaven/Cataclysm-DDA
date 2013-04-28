#ifndef _OMDATA_H_
#define _OMDATA_H_

#include <string>
#include <vector>
#include <bitset>
#include "mtype.h"
#include "itype.h"
#include "output.h"
#include "mongroup.h"
#include "mapdata.h"

#define OMAPX 180
#define OMAPY 180

class overmap;

struct oter_t {
 std::string name;
 long sym;	// This is a long, so we can support curses linedrawing
 nc_color color;
 unsigned char see_cost; // Affects how far the player can see in the overmap
 const map_extras& embellishments;
 bool known_down;
 bool known_up;
};

const map_extras no_extras(0);
const map_extras road_extras(
// %%% HEL MIL SCI STA DRG SUP PRT MIN WLF CGR PUD CRT FUM 1WY ART
    50, 40, 50,120,200, 30, 10,  5, 80, 20, 20, 200, 10,  8,  2,  3);
const map_extras field_extras(
    60, 40, 15, 40, 80, 10, 10,  3, 50, 30, 40, 300, 10,  8,  1,  3);
const map_extras subway_extras(
// %%% HEL MIL SCI STA DRG SUP PRT MIN WLF CGR PUD CRT FUM 1WY ART
    75,  0,  5, 12,  5,  5,  0,  7,  0,  0, 0, 120,  0, 20,  1,  3);
const map_extras build_extras(
    90,  0,  5, 12,  0, 10,  0,  5,  5,  0, 0, 0, 60,  8,  1,  3);

enum oter_id {
 ot_null = 0,
 ot_crater,
// Wild terrain
 ot_field, ot_dirtlot, ot_forest, ot_forest_thick, ot_forest_water, ot_hive, ot_spider_pit,
  ot_fungal_bloom,
// Roads
 ot_hiway_ns, ot_hiway_ew,
 ot_road_null,
 ot_road_ns, ot_road_ew,
 ot_road_ne, ot_road_es, ot_road_sw, ot_road_wn,
 ot_road_nes, ot_road_new, ot_road_nsw, ot_road_esw, ot_road_nesw,
 ot_road_nesw_manhole,
 ot_bridge_ns, ot_bridge_ew,
 ot_river_center,
 ot_river_c_not_ne, ot_river_c_not_nw, ot_river_c_not_se, ot_river_c_not_sw,
 ot_river_north, ot_river_east, ot_river_south, ot_river_west,
 ot_river_ne, ot_river_se, ot_river_sw, ot_river_nw,
// City buildings
 ot_house_north, ot_house_east, ot_house_south, ot_house_west,
 ot_house_base_north, ot_house_base_east, ot_house_base_south,
  ot_house_base_west,
 ot_s_lot, ot_park,
 ot_s_gas_north, ot_s_gas_east, ot_s_gas_south, ot_s_gas_west,
 ot_s_pharm_north, ot_s_pharm_east, ot_s_pharm_south, ot_s_pharm_west,
 ot_office_doctor_north, ot_office_doctor_east, ot_office_doctor_south, ot_office_doctor_west,
 ot_office_cubical_north, ot_office_cubical_east, ot_office_cubical_south, ot_office_cubical_west,
 ot_church_north, ot_church_east, ot_church_south, ot_church_west,
 ot_s_grocery_north, ot_s_grocery_east, ot_s_grocery_south, ot_s_grocery_west,
 ot_s_hardware_north, ot_s_hardware_east, ot_s_hardware_south,
  ot_s_hardware_west,
 ot_s_electronics_north, ot_s_electronics_east, ot_s_electronics_south,
  ot_s_electronics_west,
 ot_s_sports_north, ot_s_sports_east, ot_s_sports_south, ot_s_sports_west,
 ot_s_liquor_north, ot_s_liquor_east, ot_s_liquor_south, ot_s_liquor_west,
 ot_s_gun_north, ot_s_gun_east, ot_s_gun_south, ot_s_gun_west,
 ot_s_clothes_north, ot_s_clothes_east, ot_s_clothes_south, ot_s_clothes_west,
 ot_s_library_north, ot_s_library_east, ot_s_library_south, ot_s_library_west,
 ot_s_restaurant_north, ot_s_restaurant_east, ot_s_restaurant_south,
  ot_s_restaurant_west,
 ot_s_restaurant_fast_north, ot_s_restaurant_fast_east, ot_s_restaurant_fast_south,
  ot_s_restaurant_fast_west,
 ot_s_restaurant_coffee_north, ot_s_restaurant_coffee_east, ot_s_restaurant_coffee_south,
  ot_s_restaurant_coffee_west,  
 ot_sub_station_north, ot_sub_station_east, ot_sub_station_south,
  ot_sub_station_west,
 ot_s_garage_north, ot_s_garage_east, ot_s_garage_south, ot_s_garage_west, ot_cabin, ot_farm, ot_farm_field,
 ot_police_north, ot_police_east, ot_police_south, ot_police_west,
 ot_bank_north, ot_bank_east, ot_bank_south, ot_bank_west,
 ot_bar_north, ot_bar_east, ot_bar_south, ot_bar_west,
 ot_pawn_north, ot_pawn_east, ot_pawn_south, ot_pawn_west,
 ot_mil_surplus_north, ot_mil_surplus_east, ot_mil_surplus_south,
  ot_mil_surplus_west,
 ot_megastore_entrance, ot_megastore,
 ot_hospital_entrance, ot_hospital,
 ot_mansion_entrance, ot_mansion, ot_fema_entrance, ot_fema,
 ot_station_radio_north, ot_station_radio_east, ot_station_radio_south, ot_station_radio_west,
// Goodies/dungeons
 ot_shelter, ot_shelter_under, ot_lmoe, ot_lmoe_under,
 ot_lab, ot_lab_stairs, ot_lab_core, ot_lab_finale,
 ot_nuke_plant_entrance, ot_nuke_plant, // TODO
 ot_bunker, ot_outpost,
 ot_silo, ot_silo_finale,
 ot_temple, ot_temple_stairs, ot_temple_core, ot_temple_finale, // TODO
 ot_sewage_treatment, ot_sewage_treatment_hub, ot_sewage_treatment_under,
 ot_mine_entrance, ot_mine_shaft, ot_mine, ot_mine_down, ot_mine_finale,
 ot_spiral_hub, ot_spiral,
 ot_radio_tower,
 ot_toxic_dump,
 ot_cave, ot_cave_rat,
// Underground terrain
 ot_spider_pit_under,
 ot_anthill,
 ot_rock, ot_rift, ot_hellmouth,
 ot_slimepit, ot_slimepit_down,
 ot_triffid_grove, ot_triffid_roots, ot_triffid_finale,
 ot_basement,
 ot_subway_station,
 ot_subway_ns, ot_subway_ew,
 ot_subway_ne, ot_subway_es, ot_subway_sw, ot_subway_wn,
 ot_subway_nes, ot_subway_new, ot_subway_nsw, ot_subway_esw, ot_subway_nesw,
 ot_sewer_ns, ot_sewer_ew,
 ot_sewer_ne, ot_sewer_es, ot_sewer_sw, ot_sewer_wn,
 ot_sewer_nes, ot_sewer_new, ot_sewer_nsw, ot_sewer_esw, ot_sewer_nesw,
 ot_ants_ns, ot_ants_ew,
 ot_ants_ne, ot_ants_es, ot_ants_sw, ot_ants_wn,
 ot_ants_nes, ot_ants_new, ot_ants_nsw, ot_ants_esw, ot_ants_nesw,
 ot_ants_food, ot_ants_larvae, ot_ants_queen,
 ot_cavern,

 ot_tutorial,
 num_ter_types
};

// LINE_**** corresponds to the ACS_**** macros in ncurses, and are patterned
// the same way; LINE_NESW, where X indicates a line and O indicates no line
// (thus, LINE_OXXX looks like 'T'). LINE_ is defined in output.h.  The ACS_
// macros can't be used here, since ncurses hasn't been initialized yet.

// Order MUST match enum oter_id above!

const oter_t oterlist[num_ter_types] = {
{"nothing",		'%',	c_white,	0, no_extras, false, false},
{"crater",		'O',	c_red,		2, field_extras, false, false},
{"field",		'.',	c_brown,	2, field_extras, false, false},
{"dirt lot",		'O',	c_brown,	1, field_extras, false, false},
{"forest",		'F',	c_green,	3, field_extras, false, false},
{"forest",		'F',	c_green,	4, field_extras, false, false},
{"swamp",		'F',	c_cyan,		4, field_extras, false, false},
{"bee hive",		'8',	c_yellow,	3, field_extras, false, false},
{"forest",		'F',	c_green,	3, field_extras, false, false},
/* The tile above is a spider pit. */
{"fungal bloom",	'T',	c_ltgray,	2, field_extras, false, false},
{"highway",		'H',	c_dkgray,	2, road_extras, false, false},
{"highway",		'=',	c_dkgray,	2, road_extras, false, false},
{"BUG",			'%',	c_magenta,	0, no_extras, false, false},
{"road",          LINE_XOXO,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_OXOX,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_XXOO,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_OXXO,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_OOXX,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_XOOX,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_XXXO,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_XXOX,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_XOXX,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_OXXX,	c_dkgray,	2, road_extras, false, false},
{"road",          LINE_XXXX,	c_dkgray,	2, road_extras, false, false},
{"road, manhole", LINE_XXXX,	c_yellow,	2, road_extras, true, false},
{"bridge",		'|',	c_dkgray,	2, road_extras, false, false},
{"bridge",		'-',	c_dkgray,	2, road_extras, false, false},
{"river",		'R',	c_blue,		1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"river bank",		'R',	c_ltblue,	1, no_extras, false, false},
{"house",		'^',	c_ltgreen,	5, build_extras, false, false},
{"house",		'>',	c_ltgreen,	5, build_extras, false, false},
{"house",		'v',	c_ltgreen,	5, build_extras, false, false},
{"house",		'<',	c_ltgreen,	5, build_extras, false, false},
{"house",		'^',	c_ltgreen,	5, build_extras, false, false},
{"house",		'>',	c_ltgreen,	5, build_extras, false, false},
{"house",		'v',	c_ltgreen,	5, build_extras, false, false},
{"house",		'<',	c_ltgreen,	5, build_extras, false, false},
{"parking lot",		'O',	c_dkgray,	1, build_extras, false, false},
{"park",		'O',	c_green,	2, build_extras, false, false},
{"gas station",		'^',	c_ltblue,	5, build_extras, false, false},
{"gas station",		'>',	c_ltblue,	5, build_extras, false, false},
{"gas station",		'v',	c_ltblue,	5, build_extras, false, false},
{"gas station",		'<',	c_ltblue,	5, build_extras, false, false},
{"pharmacy",		'^',	c_ltred,	5, build_extras, false, false},
{"pharmacy",		'>',	c_ltred,	5, build_extras, false, false},
{"pharmacy",		'v',	c_ltred,	5, build_extras, false, false},
{"pharmacy",		'<',	c_ltred,	5, build_extras, false, false},
{"doctor's office", 	'^',	   i_ltred,	5, build_extras, false, false},
{"doctor's office",  '>',    i_ltred, 5, build_extras, false, false},
{"doctor's office",  'v',    i_ltred,	5, build_extras, false, false},
{"doctor's office",  '<',    i_ltred,	5, build_extras, false, false},
{"office", 	'^',	c_ltgray,	5, build_extras, false, false},
{"office",		'>',	c_ltgray,	5, build_extras, false, false},
{"office",		'v',	c_ltgray,	5, build_extras, false, false},
{"office",		'<',	c_ltgray,	5, build_extras, false, false},
{"church",		'C',	c_ltred,	5, build_extras, false, false},
{"church",		'C',	c_ltred,	5, build_extras, false, false},
{"church",		'C',	c_ltred,	5, build_extras, false, false},
{"church",		'C',	c_ltred,	5, build_extras, false, false},
{"grocery store",	'^',	c_green,	5, build_extras, false, false},
{"grocery store",	'>',	c_green,	5, build_extras, false, false},
{"grocery store",	'v',	c_green,	5, build_extras, false, false},
{"grocery store",	'<',	c_green,	5, build_extras, false, false},
{"hardware store",	'^',	c_cyan,		5, build_extras, false, false},
{"hardware store",	'>',	c_cyan,		5, build_extras, false, false},
{"hardware store",	'v',	c_cyan,		5, build_extras, false, false},
{"hardware store",	'<',	c_cyan,		5, build_extras, false, false},
{"electronics store",   '^',	c_yellow,	5, build_extras, false, false},
{"electronics store",   '>',	c_yellow,	5, build_extras, false, false},
{"electronics store",   'v',	c_yellow,	5, build_extras, false, false},
{"electronics store",   '<',	c_yellow,	5, build_extras, false, false},
{"sporting goods store",'^',	c_ltcyan,	5, build_extras, false, false},
{"sporting goods store",'>',	c_ltcyan,	5, build_extras, false, false},
{"sporting goods store",'v',	c_ltcyan,	5, build_extras, false, false},
{"sporting goods store",'<',	c_ltcyan,	5, build_extras, false, false},
{"liquor store",	'^',	c_magenta,	5, build_extras, false, false},
{"liquor store",	'>',	c_magenta,	5, build_extras, false, false},
{"liquor store",	'v',	c_magenta,	5, build_extras, false, false},
{"liquor store",	'<',	c_magenta,	5, build_extras, false, false},
{"gun store",		'^',	c_red,		5, build_extras, false, false},
{"gun store",		'>',	c_red,		5, build_extras, false, false},
{"gun store",		'v',	c_red,		5, build_extras, false, false},
{"gun store",		'<',	c_red,		5, build_extras, false, false},
{"clothing store",	'^',	c_blue,		5, build_extras, false, false},
{"clothing store",	'>',	c_blue,		5, build_extras, false, false},
{"clothing store",	'v',	c_blue,		5, build_extras, false, false},
{"clothing store",	'<',	c_blue,		5, build_extras, false, false},
{"library",		'^',	c_brown,	5, build_extras, false, false},
{"library",		'>',	c_brown,	5, build_extras, false, false},
{"library",		'v',	c_brown,	5, build_extras, false, false},
{"library",		'<',	c_brown,	5, build_extras, false, false},
{"restaurant",		'^',	c_pink,		5, build_extras, false, false},
{"restaurant",		'>',	c_pink,		5, build_extras, false, false},
{"restaurant",		'v',	c_pink,		5, build_extras, false, false},
{"restaurant",		'<',	c_pink,		5, build_extras, false, false},
{"fast food restaurant",    '^', c_pink,		5, build_extras, false, false},
{"fast food restaurant",    '>',	c_pink,		5, build_extras, false, false},
{"fast food restaurant",    'v',	c_pink,		5, build_extras, false, false},
{"fast food restaurant",    '<',	c_pink,		5, build_extras, false, false},
{"coffee shop",    '^',	c_pink,		5, build_extras, false, false},
{"coffee shop",    '>',	c_pink,		5, build_extras, false, false},
{"coffee shop",    'v',	c_pink,		5, build_extras, false, false},
{"coffee shop",    '<',	c_pink,		5, build_extras, false, false},
{"subway station",	'S',	c_yellow,	5, build_extras, true, false},
{"subway station",	'S',	c_yellow,	5, build_extras, true, false},
{"subway station",	'S',	c_yellow,	5, build_extras, true, false},
{"subway station",	'S',	c_yellow,	5, build_extras, true, false},
{"garage",              'O',    c_white,       5, build_extras, false, false},
{"garage",              'O',    c_white,       5, build_extras, false, false},
{"garage",              'O',    c_white,       5, build_extras, false, false},
{"garage",              'O',    c_white,       5, build_extras, false, false},
{"cabin",              'C',    i_green,       5, build_extras, false, false},
{"farm",              '^',    i_brown,       5, build_extras, false, false},
{"farm field",              '#',    i_brown,       5, field_extras, false, false},
{"police station",	'^',	h_yellow,	5, build_extras, false, false},
{"police station",	'>',	h_yellow,	5, build_extras, false, false},
{"police station",	'v',	h_yellow,	5, build_extras, false, false},
{"police station",	'<',	h_yellow,	5, build_extras, false, false},
{"bank",		'$',	c_ltgray,	5, no_extras, false, false},
{"bank",		'$',	c_ltgray,	5, no_extras, false, false},
{"bank",		'$',	c_ltgray,	5, no_extras, false, false},
{"bank",		'$',	c_ltgray,	5, no_extras, false, false},
{"bar",			'^',	i_magenta,		5, build_extras, false, false},
{"bar",			'>',	i_magenta,		5, build_extras, false, false},
{"bar",			'v',	i_magenta,		5, build_extras, false, false},
{"bar",			'<',	i_magenta,		5, build_extras, false, false},
{"pawn shop",		'^',	c_white,	5, build_extras, false, false},
{"pawn shop",		'>',	c_white,	5, build_extras, false, false},
{"pawn shop",		'v',	c_white,	5, build_extras, false, false},
{"pawn shop",		'<',	c_white,	5, build_extras, false, false},
{"mil. surplus",	'^',	i_ltgray,	5, build_extras, false, false},
{"mil. surplus",	'>',	i_ltgray,	5, build_extras, false, false},
{"mil. surplus",	'v',	i_ltgray,	5, build_extras, false, false},
{"mil. surplus",	'<',	i_ltgray,	5, build_extras, false, false},
{"megastore",		'+',	c_ltblue,	5, build_extras, false, false},
{"megastore",		'M',	c_blue,		5, build_extras, false, false},
{"hospital",		'H',	c_ltred,	5, build_extras, false, false},
{"hospital",		'H',	c_red,		5, build_extras, false, false},
{"mansion",		'M',	c_ltgreen,	5, build_extras, false, false},
{"mansion",		'M',	c_green,	5, build_extras, false, false},
{"fema camp",		'+',	c_blue,	5, build_extras, false, false},
{"fema camp",		'F',	i_blue,	5, build_extras, false, false},
{"radio station",  'X',    i_ltgray, 	5, build_extras, false, false},
{"radio station",  'X',    i_ltgray,		5, build_extras, false, false},
{"radio station",  'X',    i_ltgray,		5, build_extras, false, false},
{"radio station",  'X',    i_ltgray,		5, build_extras, false, false},
{"evac shelter",	'+',	c_white,	2, no_extras, true, false},
{"evac shelter",	'+',	c_white,	2, no_extras, false, true},
{"LMOE shelter",	'+',	c_red,	2, no_extras, true, false},
{"LMOE shelter",	'+',	c_red,	2, no_extras, false, true},
{"science lab",		'L',	c_ltblue,	5, no_extras, false, false},
{"science lab",		'L',	c_blue,		5, no_extras, true, false},
{"science lab",		'L',	c_ltblue,	5, no_extras, false, false},
{"science lab",		'L',	c_cyan,		5, no_extras, false, false},
{"nuclear plant",	'P',	c_ltgreen,	5, no_extras, false, false},
{"nuclear plant",	'P',	c_ltgreen,	5, no_extras, false, false},
{"military bunker",	'B',	c_dkgray,	2, no_extras, true, true},
{"military outpost",	'M',	c_dkgray,	2, build_extras, false, false},
{"missile silo",	'0',	c_ltgray,	2, no_extras, false, false},
{"missile silo",	'0',	c_ltgray,	2, no_extras, false, false},
{"strange temple",	'T',	c_magenta,	5, no_extras, true, false},
{"strange temple",	'T',	c_pink,		5, no_extras, true, false},
{"strange temple",	'T',	c_pink,		5, no_extras, false, false},
{"strange temple",	'T',	c_yellow,	5, no_extras, false, false},
{"sewage treatment",	'P',	c_red,		5, no_extras, true, false},
{"sewage treatment",	'P',	c_green,	5, no_extras, false, true},
{"sewage treatment",	'P',	c_green,	5, no_extras, false, false},
{"mine entrance",	'M',	c_ltgray,	5, no_extras, true, false},
{"mine shaft",		'O',	c_dkgray,	5, no_extras, true, true},
{"mine",		'M',	c_brown,	2, no_extras, false, false},
{"mine",		'M',	c_brown,	2, no_extras, false, false},
{"mine",		'M',	c_brown,	2, no_extras, false, false},
{"spiral cavern",	'@',	c_pink,		2, no_extras, false, false},
{"spiral cavern",	'@',	c_pink,		2, no_extras, false, false},
{"radio tower",         'X',    c_ltgray,       2, no_extras, false, false},
{"toxic waste dump",	'D',	c_pink,		2, no_extras, false, false},
{"cave",		'C',	c_brown,	2, field_extras, false, false},
{"rat cave",		'C',	c_dkgray,	2, no_extras, true, false},
{"cavern",		'0',	c_ltgray,	2, no_extras, false, false},
{"anthill",		'%',	c_brown,	2, no_extras, true, false},
{"solid rock",		'%',	c_dkgray,	5, no_extras, false, false},
{"rift",		'^',	c_red,		2, no_extras, false, false},
{"hellmouth",		'^',	c_ltred,	2, no_extras, true, false},
{"slime pit",		'~',	c_ltgreen,	2, no_extras, true, false},
{"slime pit",		'~',	c_ltgreen,	2, no_extras, false, false},
{"triffid grove",	'T',	c_ltred,	5, no_extras, true, false},
{"triffid roots",	'T',	c_ltred,	5, no_extras, true, true},
{"triffid heart",	'T',	c_red,		5, no_extras, false, true},
{"basement",		'O',	c_dkgray,	5, no_extras, false, true},
{"subway station",	'S',	c_yellow,	5, subway_extras, false, true},
{"subway",        LINE_XOXO,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_OXOX,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_XXOO,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_OXXO,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_OOXX,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_XOOX,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_XXXO,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_XXOX,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_XOXX,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_OXXX,	c_dkgray,	5, subway_extras, false, false},
{"subway",        LINE_XXXX,	c_dkgray,	5, subway_extras, false, false},
{"sewer",         LINE_XOXO,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_OXOX,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_XXOO,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_OXXO,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_OOXX,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_XOOX,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_XXXO,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_XXOX,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_XOXX,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_OXXX,	c_green,	5, no_extras, false, false},
{"sewer",         LINE_XXXX,	c_green,	5, no_extras, false, false},
{"ant tunnel",    LINE_XOXO,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_OXOX,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_XXOO,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_OXXO,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_OOXX,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_XOOX,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_XXXO,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_XXOX,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_XOXX,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_OXXX,	c_brown,	5, no_extras, false, false},
{"ant tunnel",    LINE_XXXX,	c_brown,	5, no_extras, false, false},
{"ant food storage",	'O',	c_green,	5, no_extras, false, false},
{"ant larva chamber",	'O',	c_white,	5, no_extras, false, false},
{"ant queen chamber",	'O',	c_red,		5, no_extras, false, false},
{"cavern",		'0',	c_ltgray,	5, no_extras, false, false},
{"tutorial room",	'O',	c_cyan,		5, no_extras, false, false}
};

// Overmap specials--these are "special encounters," dungeons, nests, etc.
// This specifies how often and where they may be placed.

// OMSPEC_FREQ determines the length of the side of the square in which each
// overmap special will be placed.  At OMSPEC_FREQ 6, the overmap is divided
// into 900 squares; lots of space for interesting stuff!
#define OMSPEC_FREQ 15

// Flags that determine special behavior for placement
enum omspec_flag {
OMS_FLAG_NULL = 0,
OMS_FLAG_ROTATE_ROAD,	// Rotate to face road--assumes 3 following rotations
OMS_FLAG_ROTATE_RANDOM, // Rotate randomly--assumes 3 following rotations
OMS_FLAG_3X3,		// 3x3 square, e.g. bee hive
OMS_FLAG_BLOB,		// Randomly shaped blob
OMS_FLAG_3X3_SECOND,	// 3x3 square, made of the tile AFTER the main one
OMS_FLAG_BIG,		// As big as possible
OMS_FLAG_ROAD,		// Add a road_point here; connect to towns etc.
OMS_FLAG_PARKING_LOT,	// Add a road_point to the north of here
OMS_FLAG_DIRT_LOT,      // Dirt lot flag for specials
OMS_FLAG_CLASSIC, // Allow this location to spawn in classic mode
NUM_OMS_FLAGS
};

struct omspec_place
{
// Able functions - true if p is valid
 bool never      (overmap *om, tripoint p) { return false; }
 bool always     (overmap *om, tripoint p) { return true;  }
 bool water      (overmap *om, tripoint p); // Only on rivers
 bool land       (overmap *om, tripoint p); // Only on land (no rivers)
 bool forest     (overmap *om, tripoint p); // Forest
 bool wilderness (overmap *om, tripoint p); // Forest or fields
 bool by_highway (overmap *om, tripoint p); // Next to existing highways
};

struct overmap_special
{
 oter_id ter;           // Terrain placed
 int min_appearances;	// Min number in an overmap
 int max_appearances;   // Max number in an overmap
 int min_dist_from_city;// Min distance from city limits
 int max_dist_from_city;// Max distance from city limits

 std::string monsters;    // Type of monsters that appear here
 int monster_pop_min;   // Minimum monster population
 int monster_pop_max;   // Maximum monster population
 int monster_rad_min;   // Minimum monster radius
 int monster_rad_max;   // Maximum monster radius

 bool (omspec_place::*able) (overmap *om, tripoint p); // See above
 unsigned flags : NUM_OMS_FLAGS; // See above
};

enum omspec_id
{
 OMSPEC_CRATER,
 OMSPEC_HIVE,
 OMSPEC_HOUSE,
 OMSPEC_GAS,
 OMSPEC_CABIN,
 OMSPEC_LMOE,
 OMSPEC_FARM,
 OMSPEC_TEMPLE,
 OMSPEC_LAB,
 OMSPEC_FEMA,
 OMSPEC_BUNKER,
 OMSPEC_OUTPOST,
 OMSPEC_SILO,
 OMSPEC_RADIO,
 OMSPEC_MANSION,
 OMSPEC_MANSION_WILD,
 OMSPEC_MEGASTORE,
 OMSPEC_HOSPITAL,
 OMSPEC_SEWAGE,
 OMSPEC_MINE,
 OMSPEC_ANTHILL,
 OMSPEC_SPIDER,
 OMSPEC_SLIME,
 OMSPEC_FUNGUS,
 OMSPEC_TRIFFID,
 OMSPEC_LAKE,
 OMSPEC_SHELTER,
 OMSPEC_CAVE,
 OMSPEC_TOXIC_DUMP,
 NUM_OMSPECS
};

// Set min or max to -1 to ignore them

const overmap_special overmap_specials[NUM_OMSPECS] = {

// Terrain	 MIN MAX DISTANCE
{ot_crater,	   0, 10,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::land, mfb(OMS_FLAG_BLOB) | mfb(OMS_FLAG_CLASSIC)},

{ot_hive, 	   0, 50, 10, -1, "GROUP_BEE", 20, 60, 2, 4,
 &omspec_place::forest, mfb(OMS_FLAG_3X3)},

{ot_house_north,   0,100,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_ROTATE_ROAD) | mfb(OMS_FLAG_CLASSIC)},

{ot_s_gas_north,   0,100,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_ROTATE_ROAD) | mfb(OMS_FLAG_CLASSIC)},

{ot_cabin,   0, 30, 20, -1, "GROUP_NULL", 0, 0, 0, 0,  // Woods cabin
 &omspec_place::forest, mfb(OMS_FLAG_CLASSIC)},

 {ot_lmoe,   0, 3, 20, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::land, mfb(OMS_FLAG_CLASSIC)},

 {ot_farm,   0, 20, 20, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_3X3_SECOND) |mfb(OMS_FLAG_DIRT_LOT) | mfb(OMS_FLAG_CLASSIC)},

{ot_temple_stairs, 0,  3, 20, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::forest, 0},

{ot_lab_stairs,	   0, 30,  8, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::land, mfb(OMS_FLAG_ROAD)},

{ot_fema_entrance,	   0, 5,  8, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::land, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

// Terrain	 MIN MAX DISTANCE
{ot_bunker,	   2, 10,  4, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::land, mfb(OMS_FLAG_ROAD)},

{ot_outpost,	   0, 10,  4, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, 0},

{ot_silo,	   0,  1, 30, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD)},

{ot_radio_tower,   1,  5,  0, 20, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_CLASSIC)},

{ot_mansion_entrance, 0, 8, 0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

{ot_mansion_entrance, 0, 4, 10, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

{ot_megastore_entrance, 0, 5, 0, 10, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

{ot_hospital_entrance, 1, 5, 3, 15, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::by_highway, mfb(OMS_FLAG_3X3_SECOND) | mfb(OMS_FLAG_CLASSIC)},

{ot_sewage_treatment, 1,  5, 10, 20, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::land, mfb(OMS_FLAG_PARKING_LOT) | mfb(OMS_FLAG_CLASSIC)},

{ot_mine_entrance,  0,  5,  15, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_PARKING_LOT)},

// Terrain	 MIN MAX DISTANCE
{ot_anthill,	   0, 30,  10, -1, "GROUP_ANT", 10, 30, 1000, 2000,
 &omspec_place::wilderness, 0},

{ot_spider_pit,	   0,500,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::forest, 0},

{ot_slimepit,	   0,  4,  0, -1, "GROUP_GOO", 2, 10, 100, 200,
 &omspec_place::land, 0},

{ot_fungal_bloom,  0,  3,  5, -1, "GROUP_FUNGI", 600, 1200, 30, 50,
 &omspec_place::wilderness, 0},

{ot_triffid_grove, 0,  4,  0, -1, "GROUP_TRIFFID", 800, 1300, 12, 20,
 &omspec_place::forest, 0},

{ot_river_center,  0, 10, 10, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::always, mfb(OMS_FLAG_BLOB) | mfb(OMS_FLAG_CLASSIC)},

// Terrain	 MIN MAX DISTANCE
{ot_shelter,       5, 10,  5, 10, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_ROAD) | mfb(OMS_FLAG_CLASSIC)},

{ot_cave,	   0, 30,  0, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, 0},

{ot_toxic_dump,	   0,  5, 15, -1, "GROUP_NULL", 0, 0, 0, 0,
 &omspec_place::wilderness, mfb(OMS_FLAG_CLASSIC)}

};

// Overmap "Zones"
// Areas which have special post-generation processing attached to them

enum overmap_zone
{
 OMZONE_NULL = 0,
 OMZONE_CITY,		// Basic city; place corpses
 OMZONE_BOMBED,		// Terrain is heavily destroyed
 OMZONE_IRRADIATED,	// Lots of radioactivity TODO
 OMZONE_CORRUPTED,	// Fabric of space is weak TODO
 OMZONE_OVERGROWN,	// Lots of plants, etc. TODO
 OMZONE_FUNGAL,		// Overgrown with fungus TODO
 OMZONE_MILITARIZED,	// _Was_ occupied by the military TODO
 OMZONE_FLOODED,	// Flooded out TODO
 OMZONE_TRAPPED,	// Heavily booby-trapped TODO
 OMZONE_MUTATED,	// Home of mutation experiments - mutagen & monsters TODO
 OMZONE_FORTIFIED,	// Boarded up windows &c TODO
 OMZONE_BOTS,		// Home of the bots TODO
 OMZONE_MAX
};

#endif
