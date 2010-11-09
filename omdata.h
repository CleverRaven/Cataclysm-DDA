#ifndef _OMDATA_H_
#define _OMDATA_H_

#include <string>
#include <vector>
#include <bitset>
#include "mtype.h"
#include "itype.h"
#include "output.h"

#define OMAPX 180
#define OMAPY 180

#define NORTH 1
#define EAST  2
#define SOUTH 4
#define WEST  8

#define TUTORIAL_Z	10
#define NETHER_Z 	20

// oter_t defines almost all data needed to describe a location... however,
// the list of eligible items is too long to store in this struct!  That data
// is now a part of the game class, and a pointer to said data is passed to new
// maps; please see mapitemsdef.cpp.
struct oter_t {
 std::string name;
 long sym;	// This is type long so we can support curses linedrawing
 nc_color color;
 unsigned char see_cost; // Affects how far the player can see in the overmap
};

enum oter_id {
 ot_null = 0,
// Wild terrain
 ot_field, ot_forest, ot_forest_thick, ot_forest_water, ot_hive, ot_hive_center,
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
 ot_s_lot,
 ot_s_gas_north, ot_s_gas_east, ot_s_gas_south, ot_s_gas_west,
 ot_s_pharm_north, ot_s_pharm_east, ot_s_pharm_south, ot_s_pharm_west,
 ot_s_grocery_north, ot_s_grocery_east, ot_s_grocery_south, ot_s_grocery_west,
 ot_s_hardware_north, ot_s_hardware_east, ot_s_hardware_south,
  ot_s_hardware_west,
 ot_s_sports_north, ot_s_sports_east, ot_s_sports_south, ot_s_sports_west,
 ot_s_liquor_north, ot_s_liquor_east, ot_s_liquor_south, ot_s_liquor_west,
 ot_s_gun_north, ot_s_gun_east, ot_s_gun_south, ot_s_gun_west,
 ot_s_clothes_north, ot_s_clothes_east, ot_s_clothes_south, ot_s_clothes_west,
 ot_s_library_north, ot_s_library_east, ot_s_library_south, ot_s_library_west,
 ot_sub_station_north, ot_sub_station_east, ot_sub_station_south,
  ot_sub_station_west,
// Goodies/dungeons
 ot_lab, ot_lab_stairs, ot_lab_core, ot_lab_finale,
 ot_nuke_plant_entrance, ot_nuke_plant,
 ot_silo, ot_silo_finale,
 ot_temple, ot_temple_stairs, ot_temple_core, ot_temple_finale,
// Settlement
 ot_set_center,
 ot_set_house, ot_set_food, ot_set_weapons, ot_set_guns, ot_set_clinic,
  ot_set_clothing, ot_set_general, ot_set_casino, ot_set_library, ot_set_lab,
  ot_set_bionics, 
 ot_radio_tower,
 ot_gate, ot_wall,
// Underground terrain
 ot_anthill,
 ot_rock, ot_rift, ot_hellmouth,
 ot_slimepit, ot_slimepit_down,
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
{"nothing",		'%',	c_white,	0},
{"field",		'.',	c_brown,	2},
{"forest",		'F',	c_green,	3},
{"forest",		'F',	c_green,	4},
{"swamp",		'F',	c_cyan,		4},
{"bee hive",		'8',	c_yellow,	3},
{"bee hive",		'8',	c_yellow,	3},
{"highway",		'H',	c_dkgray,	2},
{"highway",		'=',	c_dkgray,	2},
{"BUG",			'%',	c_magenta,	0},
{"road",          LINE_XOXO,	c_dkgray,	2},
{"road",          LINE_OXOX,	c_dkgray,	2},
{"road",          LINE_XXOO,	c_dkgray,	2},
{"road",          LINE_OXXO,	c_dkgray,	2},
{"road",          LINE_OOXX,	c_dkgray,	2},
{"road",          LINE_XOOX,	c_dkgray,	2},
{"road",          LINE_XXXO,	c_dkgray,	2},
{"road",          LINE_XXOX,	c_dkgray,	2},
{"road",          LINE_XOXX,	c_dkgray,	2},
{"road",          LINE_OXXX,	c_dkgray,	2},
{"road",          LINE_XXXX,	c_dkgray,	2},
{"road w/ manhole",LINE_XXXX,	c_yellow,	2},
{"bridge",		'|',	c_dkgray,	2},
{"bridge",		'-',	c_dkgray,	2},
{"river",		'R',	c_blue,		1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"river bank",		'R',	c_ltblue,	1},
{"house",		'^',	c_ltgreen,	5},
{"house",		'>',	c_ltgreen,	5},
{"house",		'v',	c_ltgreen,	5},
{"house",		'<',	c_ltgreen,	5},
{"house",		'^',	c_ltgreen,	5},
{"house",		'>',	c_ltgreen,	5},
{"house",		'v',	c_ltgreen,	5},
{"house",		'<',	c_ltgreen,	5},
{"parking lot",		'O',	c_dkgray,	1},
{"gas station",		'^',	c_ltblue,	5},
{"gas station",		'>',	c_ltblue,	5},
{"gas station",		'v',	c_ltblue,	5},
{"gas station",		'<',	c_ltblue,	5},
{"pharmacy",		'^',	c_ltred,	5},
{"pharmacy",		'>',	c_ltred,	5},
{"pharmacy",		'v',	c_ltred,	5},
{"pharmacy",		'<',	c_ltred,	5},
{"grocery store",	'^',	c_green,	5},
{"grocery store",	'>',	c_green,	5},
{"grocery store",	'v',	c_green,	5},
{"grocery store",	'<',	c_green,	5},
{"hardware store",	'^',	c_cyan,		5},
{"hardware store",	'>',	c_cyan,		5},
{"hardware store",	'v',	c_cyan,		5},
{"hardware store",	'<',	c_cyan,		5},
{"sporting goods store",'^',	c_ltcyan,	5},
{"sporting goods store",'>',	c_ltcyan,	5},
{"sporting goods store",'v',	c_ltcyan,	5},
{"sporting goods store",'<',	c_ltcyan,	5},
{"liquor store",	'^',	c_magenta,	5},
{"liquor store",	'>',	c_magenta,	5},
{"liquor store",	'v',	c_magenta,	5},
{"liquor store",	'<',	c_magenta,	5},
{"gun store",		'^',	c_red,		5},
{"gun store",		'>',	c_red,		5},
{"gun store",		'v',	c_red,		5},
{"gun store",		'<',	c_red,		5},
{"clothing store",	'^',	c_blue,		5},
{"clothing store",	'>',	c_blue,		5},
{"clothing store",	'v',	c_blue,		5},
{"clothing store",	'<',	c_blue,		5},
{"library",		'^',	c_brown,	5},
{"library",		'>',	c_brown,	5},
{"library",		'v',	c_brown,	5},
{"library",		'<',	c_brown,	5},
{"subway station",	'^',	c_yellow,	5},
{"subway station",	'>',	c_yellow,	5},
{"subway station",	'v',	c_yellow,	5},
{"subway station",	'<',	c_yellow,	5},
{"science lab",		'L',	c_ltblue,	5},
{"science lab",		'L',	c_blue,		5},
{"science lab",		'L',	c_ltblue,	5},
{"science lab",		'L',	c_cyan,		5},
{"nuclear plant",	'P',	c_ltgreen,	5},
{"nuclear plant",	'P',	c_ltgreen,	5},
{"missile silo",	'0',	c_ltgray,	2},
{"missile silo",	'0',	c_ltgray,	2},
{"strange temple",	'T',	c_magenta,	5},
{"strange temple",	'T',	c_pink,		5},
{"strange temple",	'T',	c_pink,		5},
{"strange temple",	'T',	c_yellow,	5},
{"town center",		'O',	c_white,	2},
{"living quarters",	'*',	c_ltgreen,	2},
{"food market",		'*',	c_green,	2},
{"weapon shop",		'*',	c_cyan,		2},
{"gun shop",		'*',	c_red,		2},
{"clinic",		'*',	c_ltred,	2},
{"clothing store",	'*',	c_blue,		2},
{"general store",	'*',	c_ltblue,	2},
{"casino",		'*',	c_yellow,	2},
{"library",		'*',	c_brown,	2},
{"small laboratory",	'*',	c_magenta,	2},
{"bionics shop",	'*',	c_ltgray,	2},
{"radio tower",		'X',	c_ltgray,	2},
{"city gate",		'o',	c_ltred,	5},
{"wall",		'x',	c_ltgray,	5},
{"anthill",		'%',	c_brown,	2},
{"solid rock",		'%',	c_dkgray,	5},
{"rift",		'^',	c_red,		2},
{"hellmouth",		'^',	c_ltred,	2},
{"slime pit",		'~',	c_ltgreen,	2},
{"slime pit",		'~',	c_ltgreen,	2},
{"basement",		'O',	c_dkgray,	5},
{"subway station",	'S',	c_yellow,	5},
{"subway",        LINE_XOXO,	c_dkgray,	5},
{"subway",        LINE_OXOX,	c_dkgray,	5},
{"subway",        LINE_XXOO,	c_dkgray,	5},
{"subway",        LINE_OXXO,	c_dkgray,	5},
{"subway",        LINE_OOXX,	c_dkgray,	5},
{"subway",        LINE_XOOX,	c_dkgray,	5},
{"subway",        LINE_XXXO,	c_dkgray,	5},
{"subway",        LINE_XXOX,	c_dkgray,	5},
{"subway",        LINE_XOXX,	c_dkgray,	5},
{"subway",        LINE_OXXX,	c_dkgray,	5},
{"subway",        LINE_XXXX,	c_dkgray,	5},
{"sewer",         LINE_XOXO,	c_green,	5},
{"sewer",         LINE_OXOX,	c_green,	5},
{"sewer",         LINE_XXOO,	c_green,	5},
{"sewer",         LINE_OXXO,	c_green,	5},
{"sewer",         LINE_OOXX,	c_green,	5},
{"sewer",         LINE_XOOX,	c_green,	5},
{"sewer",         LINE_XXXO,	c_green,	5},
{"sewer",         LINE_XXOX,	c_green,	5},
{"sewer",         LINE_XOXX,	c_green,	5},
{"sewer",         LINE_OXXX,	c_green,	5},
{"sewer",         LINE_XXXX,	c_green,	5},
{"ant tunnel",    LINE_XOXO,	c_brown,	5},
{"ant tunnel",    LINE_OXOX,	c_brown,	5},
{"ant tunnel",    LINE_XXOO,	c_brown,	5},
{"ant tunnel",    LINE_OXXO,	c_brown,	5},
{"ant tunnel",    LINE_OOXX,	c_brown,	5},
{"ant tunnel",    LINE_XOOX,	c_brown,	5},
{"ant tunnel",    LINE_XXXO,	c_brown,	5},
{"ant tunnel",    LINE_XXOX,	c_brown,	5},
{"ant tunnel",    LINE_XOXX,	c_brown,	5},
{"ant tunnel",    LINE_OXXX,	c_brown,	5},
{"ant tunnel",    LINE_XXXX,	c_brown,	5},
{"ant food storage",	'O',	c_green,	5},
{"ant larva chamber",	'O',	c_white,	5},
{"ant queen chamber",	'O',	c_red,		5},
{"cavern",		'0',	c_ltgray,	5},
{"tutorial room",	'O',	c_cyan,		5}
};
#endif
