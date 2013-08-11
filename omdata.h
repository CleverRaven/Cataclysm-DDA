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
 map_extras& embellishments;
 bool known_down;
 bool known_up;
 int mondensity;
 oter_t& operator=(const oter_t right){
    name = right.name;
    sym = right.sym;
    color = right.color;
    see_cost = right.see_cost;
    embellishments = right.embellishments;
    known_down = right.known_down;
    known_up = right.known_up;
    mondensity = right.mondensity;
    return *this;
 }
};

enum oter_id {
 ot_null = 0,
 ot_crater,
// Wild terrain
 ot_field, ot_forest, ot_forest_thick, ot_forest_water,
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
 ot_s_lot, ot_park, ot_pool,
 ot_s_gas_north, ot_s_gas_east, ot_s_gas_south, ot_s_gas_west,
 ot_s_pharm_north, ot_s_pharm_east, ot_s_pharm_south, ot_s_pharm_west,
 ot_office_doctor_north, ot_office_doctor_east, ot_office_doctor_south, ot_office_doctor_west,
 ot_office_cubical_north, ot_office_cubical_east, ot_office_cubical_south, ot_office_cubical_west,
 ot_apartments_con_tower_1_entrance,ot_apartments_con_tower_1,
 ot_apartments_mod_tower_1_entrance,ot_apartments_mod_tower_1,
 ot_office_tower_1_entrance, ot_office_tower_1, ot_office_tower_b_entrance, ot_office_tower_b,
 ot_church_north, ot_church_east, ot_church_south, ot_church_west,
 ot_cathedral_1_entrance, ot_cathedral_1, ot_cathedral_b_entrance, ot_cathedral_b,
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
 ot_s_garage_north, ot_s_garage_east, ot_s_garage_south, ot_s_garage_west, 
 ot_cabin_strange, ot_cabin_strange_b, ot_cabin, 
 ot_dirtlot, ot_farm, ot_farm_field,
 ot_police_north, ot_police_east, ot_police_south, ot_police_west,
 ot_bank_north, ot_bank_east, ot_bank_south, ot_bank_west,
 ot_bar_north, ot_bar_east, ot_bar_south, ot_bar_west,
 ot_pawn_north, ot_pawn_east, ot_pawn_south, ot_pawn_west,
 ot_mil_surplus_north, ot_mil_surplus_east, ot_mil_surplus_south,
  ot_mil_surplus_west,
 ot_furniture_north, ot_furniture_east, ot_furniture_south,
 ot_furniture_west,
 ot_abstorefront_north, ot_abstorefront_east, ot_abstorefront_south,
 ot_abstorefront_west,
 ot_megastore_entrance, ot_megastore,
 ot_hospital_entrance, ot_hospital,
 ot_public_works_entrance, ot_public_works,
 ot_school_1, ot_school_2, ot_school_3, ot_school_4, ot_school_5, ot_school_6, ot_school_7, ot_school_8, ot_school_9,
 ot_prison_1, ot_prison_2, ot_prison_3, ot_prison_4, ot_prison_5, ot_prison_6, ot_prison_7, ot_prison_8, ot_prison_9,
 ot_prison_b, ot_prison_b_entrance,
 ot_hotel_tower_1_1, ot_hotel_tower_1_2, ot_hotel_tower_1_3, ot_hotel_tower_1_4, ot_hotel_tower_1_5, ot_hotel_tower_1_6,
 ot_hotel_tower_1_7, ot_hotel_tower_1_8, ot_hotel_tower_1_9,ot_hotel_tower_b_1, ot_hotel_tower_b_2, ot_hotel_tower_b_3,
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
 ot_haz_sar_entrance, ot_haz_sar,
 ot_haz_sar_entrance_b1, ot_haz_sar_b1,
 ot_cave, ot_cave_rat, ot_hive, ot_fungal_bloom, ot_spider_pit,
// Underground terrain
 ot_spider_pit_under,
 ot_anthill,
 ot_slimepit, ot_slimepit_down,
 ot_triffid_grove, ot_triffid_roots, ot_triffid_finale,
 ot_basement,
 ot_cavern, ot_rock, ot_rift, ot_hellmouth,
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

 ot_tutorial,
 num_ter_types
};

// LINE_**** corresponds to the ACS_**** macros in ncurses, and are patterned
// the same way; LINE_NESW, where X indicates a line and O indicates no line
// (thus, LINE_OXXX looks like 'T'). LINE_ is defined in output.h.  The ACS_
// macros can't be used here, since ncurses hasn't been initialized yet.

// Order MUST match enum oter_id above!

extern std::vector<oter_t> oterlist;

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
OMS_FLAG_3X3_FIXED, //3x3 square, made of tiles one ahead and seven after
OMS_FLAG_2X2,
OMS_FLAG_2X2_SECOND,
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
 bool never      (overmap *om, unsigned long f, tripoint p) { return false; }
 bool always     (overmap *om, unsigned long f, tripoint p) { return true;  }
 bool water      (overmap *om, unsigned long f, tripoint p); // Only on rivers
 bool land       (overmap *om, unsigned long f, tripoint p); // Only on land (no rivers)
 bool forest     (overmap *om, unsigned long f, tripoint p); // Forest
 bool wilderness (overmap *om, unsigned long f, tripoint p); // Forest or fields
 bool by_highway (overmap *om, unsigned long f, tripoint p); // Next to existing highways
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

 bool (omspec_place::*able) (overmap *om, unsigned long f, tripoint p); // See above
 unsigned long flags : NUM_OMS_FLAGS; // See above
};

enum omspec_id
{
 OMSPEC_CRATER,
 OMSPEC_HIVE,
 OMSPEC_HOUSE,
 OMSPEC_GAS,
 OMSPEC_CABIN,
 OMSPEC_CABIN_STRANGE,
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
 OMSPEC_PUBLIC_WORKS,
 OMSPEC_APARTMENT_CON_TOWER,
 OMSPEC_APARTMENT_MOD_TOWER,
 OMSPEC_OFFICE_TOWER,
 OMSPEC_CATHEDRAL,
 OMSPEC_SCHOOL,
 OMSPEC_PRISON,
 OMSPEC_HOTEL_TOWER,
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
 OMSPEC_LONE_GASSTATION,
 OMSPEC_HAZARDOUS_SAR,
 NUM_OMSPECS
};

// Set min or max to -1 to ignore them

extern overmap_special overmap_specials[NUM_OMSPECS];

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
