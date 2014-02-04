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
    std::string id;      // definitive identifier
    int loadid;          // position in termap / terlist 
    std::string name;
    long sym; // This is a long, so we can support curses linedrawing
    nc_color color;
    unsigned char see_cost; // Affects how far the player can see in the overmap
    std::string extras;
    bool known_down;
    bool known_up;
    int mondensity;
    bool sidewalk;
    bool allow_road;
    bool is_river;
    bool is_road;
    // bool disable_default_mapgen;
    // automatically set. We can be wasteful of memory here for num_oters * sizeof(extrastuff), if it'll save us from thousands of string ops
    std::string id_base; // base identifier; either the same as id, or id without directional variations. (ie, 'house' / 'house_west' )
    int loadid_base; // self || directional_peers[0]? or seperate base_oter_map ?
    std::vector<int> directional_peers; // fast reliable (?) method of determining whatever_west, etc.
    bool rotates; // lazy for; directional_peers.size() == 4
    bool line_drawing; // lazy for; directional_peers.size() == 8
    std::string id_mapgen;  // *only* for mapgen and almost always == id_base. Unless line_drawing / road.

    oter_t& operator=(const oter_t right){
        id = right.id;
        loadid = right.loadid;
        name = right.name;
        id_base = right.id_base;
        sym = right.sym;
        color = right.color;
        see_cost = right.see_cost;
        extras = right.extras;
        known_down = right.known_down;
        known_up = right.known_up;
        mondensity = right.mondensity;
        sidewalk = right.sidewalk;
        allow_road = right.allow_road;
        is_river = right.is_river;
        is_road = right.is_road;
        //disable_default_mapgen = right.disable_default_mapgen;
        loadid_base = right.loadid_base;
        directional_peers = right.directional_peers;
        rotates = right.rotates;
        line_drawing = right.line_drawing;
        id_mapgen = right.id_mapgen;
        return *this;
    }
};

struct oter_id {
   int _val; // just numeric index of oter_t, but typically invoked as string

   // Hi, I'm an
   operator int() const;
   // pretending to be a
   operator std::string() const;
   // in order to map
   operator oter_t() const;

const oter_t & t() const;

   // set and compare by string
   const int& operator=(const int& i);
   bool operator!=(const char * v) const;
   bool operator==(const char * v) const;
   bool operator>=(const char * v) const;
   bool operator<=(const char * v) const;

   // or faster, with another oter_id
   bool operator!=(const oter_id& v) const;
   bool operator==(const oter_id& v) const;


   // initialize as raw value
   oter_id() : _val(0) { };
   oter_id(int i) : _val(i) { };
   // or as "something" by consulting otermap
   oter_id(const std::string& v);
   oter_id(const char * v);

   // these std::string functions are provided for convenience, for others,
   //  best invoke as actual string; std::string( ter(1, 2, 3) ).substr(...
   const char * c_str() const;
   int size() const;
   int find(const std::string &v, const int start, const int end) const;
   int compare(size_t pos, size_t len, const char* s, size_t n=0) const;
};



//typedef std::string oter_id;
typedef oter_id oter_iid;

// LINE_**** corresponds to the ACS_**** macros in ncurses, and are patterned
// the same way; LINE_NESW, where X indicates a line and O indicates no line
// (thus, LINE_OXXX looks like 'T'). LINE_ is defined in output.h.  The ACS_
// macros can't be used here, since ncurses hasn't been initialized yet.

// Overmap specials--these are "special encounters," dungeons, nests, etc.
// This specifies how often and where they may be placed.

// OMSPEC_FREQ determines the length of the side of the square in which each
// overmap special will be placed.  At OMSPEC_FREQ 6, the overmap is divided
// into 900 squares; lots of space for interesting stuff!
#define OMSPEC_FREQ 15

// Flags that determine special behavior for placement
enum omspec_flag {
OMS_FLAG_NULL = 0,
OMS_FLAG_ROTATE_ROAD,   // Rotate to face road--assumes 3 following rotations
OMS_FLAG_ROTATE_RANDOM, // Rotate randomly--assumes 3 following rotations
OMS_FLAG_3X3,           // 3x3 square, e.g. bee hive
OMS_FLAG_BLOB,          // Randomly shaped blob
OMS_FLAG_3X3_SECOND,    // 3x3 square, made of the tile AFTER the main one
OMS_FLAG_3X3_FIXED,     //3x3 square, made of tiles one ahead and seven after
OMS_FLAG_2X2,
OMS_FLAG_2X2_SECOND,
OMS_FLAG_BIG,           // As big as possible
OMS_FLAG_ROAD,          // Add a road_point here; connect to towns etc.
OMS_FLAG_PARKING_LOT,   // Add a road_point to the north of here
OMS_FLAG_DIRT_LOT,      // Dirt lot flag for specials
OMS_FLAG_CLASSIC,       // Allow this location to spawn in classic mode
NUM_OMS_FLAGS
};

struct omspec_place
{
// Able functions - true if p is valid
 bool never      (overmap *, unsigned long, tripoint) { return false; }
 bool always     (overmap *, unsigned long, tripoint) { return true;  }
 bool water      (overmap *om, unsigned long f, tripoint p); // Only on rivers
 bool land       (overmap *om, unsigned long f, tripoint p); // Only on land (no rivers)
 bool forest     (overmap *om, unsigned long f, tripoint p); // Forest
 bool wilderness (overmap *om, unsigned long f, tripoint p); // Forest or fields
 bool by_highway (overmap *om, unsigned long f, tripoint p); // Next to existing highways
};

struct overmap_special
{
// oter_id ter;           // Terrain placed
std::string ter;
 int min_appearances;   // Min number in an overmap
 int max_appearances;   // Max number in an overmap
 int min_dist_from_city;// Min distance from city limits
 int max_dist_from_city;// Max distance from city limits

 std::string monsters;  // Type of monsters that appear here
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
 OMSPEC_ICE_LAB,
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
    OMZONE_CITY,        // Basic city; place corpses
    OMZONE_BOMBED,      // Terrain is heavily destroyed
    OMZONE_IRRADIATED,  // Lots of radioactivity TODO
    OMZONE_CORRUPTED,   // Fabric of space is weak TODO
    OMZONE_OVERGROWN,   // Lots of plants, etc. TODO
    OMZONE_FUNGAL,      // Overgrown with fungus TODO
    OMZONE_MILITARIZED, // _Was_ occupied by the military TODO
    OMZONE_FLOODED,     // Flooded out TODO
    OMZONE_TRAPPED,     // Heavily booby-trapped TODO
    OMZONE_MUTATED,     // Home of mutation experiments - mutagen & monsters TODO
    OMZONE_FORTIFIED,   // Boarded up windows &c TODO
    OMZONE_BOTS,        // Home of the bots TODO
    OMZONE_MAX
};

//////////////////////////////////
///// convenience definitions for hard-coded functions.
extern oter_iid ot_null,
     ot_crater,
     ot_field,
     ot_forest,
     ot_forest_thick,
     ot_forest_water,
     ot_river_center;

void set_oter_ids();

#endif
