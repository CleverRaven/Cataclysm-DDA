#ifndef OMDATA_H
#define OMDATA_H

#include "color.h"
#include "json.h"
#include "enums.h"
#include "string_id.h"
#include <string>
#include <vector>
#include <list>
#include <set>

struct MonsterGroup;
using mongroup_id = string_id<MonsterGroup>;

class overmap;

struct overmap_spawns {
    overmap_spawns(): group( NULL_ID ), min_population(0), max_population(0),
        chance(0) {};
    mongroup_id group;
    int min_population;
    int max_population;
    int chance;
};
 //terrain flags enum! this is for tracking the indices of each flag.
    //is_asphalt, is_building, is_subway, is_sewer, is_ants,
    //is_base_terrain, known_down, known_up, is_river,
    //is_road, has_sidewalk, allow_road, rotates, line_drawing
enum oter_flags {
    is_asphalt = 0,
    is_building,
    is_subway,
    is_sewer,
    is_ants,
    is_base_terrain,
    known_down, 
    known_up, 
    river_tile, 
    road_tile, 
    has_sidewalk, 
    allow_road,
    rotates, // does this tile have four versions, one for each direction?
    line_drawing, // does this tile have 8 versions, including straights, bends, tees, and a fourway?
    num_oter_flags
};

struct oter_t {
    std::string id;      // definitive identifier
    unsigned loadid;          // position in termap / terlist
    std::string name;
    long sym; // This is a long, so we can support curses linedrawing
    nc_color color;
    unsigned char see_cost; // Affects how far the player can see in the overmap
    std::string extras;
    int mondensity;
    // bool disable_default_mapgen;
    // automatically set. We can be wasteful of memory here for num_oters * sizeof(extrastuff), if it'll save us from thousands of string ops
    std::string
    id_base; // base identifier; either the same as id, or id without directional variations. (ie, 'house' / 'house_west' )
    unsigned loadid_base; // self || directional_peers[0]? or seperate base_oter_map ?
    std::vector<int> directional_peers; // fast reliable (?) method of determining whatever_west, etc.
    std::string id_mapgen;  // *only* for mapgen and almost always == id_base. Unless line_drawing / road.

    // Spawns are added to the submaps *once* upon mapgen of the submaps
    overmap_spawns static_spawns;
    //this bitset contains boolean values for:
    //is_asphalt, is_building, is_subway, is_sewer, is_ants,
    //is_base_terrain, known_down, known_up, is_river,
    //is_road, has_sidewalk, allow_road, rotates, line_drawing
  private:
    std::bitset<num_oter_flags> flags; //contains a bitset for all the bools this terrain might have.
  public:
      bool has_flag(oter_flags flag) const {
          return flags[flag];
      }

      void set_flag(oter_flags flag, bool value = true) {
          flags[flag] = value;
      }
};

struct oter_id {
    unsigned _val; // just numeric index of oter_t, but typically invoked as string

    // Hi, I'm an
    operator int() const;
    // pretending to be a
    operator std::string const&() const;
    // in order to map
    operator oter_t() const;

    const oter_t &t() const;

    // set and compare by string
    const unsigned &operator=(const int &i);
    bool operator!=(const char *v) const;
    bool operator==(const char *v) const;
    bool operator>=(const char *v) const;
    bool operator<=(const char *v) const;

    // or faster, with another oter_id
    bool operator!=(const oter_id &v) const;
    bool operator==(const oter_id &v) const;


    // initialize as raw value
    oter_id() : _val(0) { };
    oter_id(int i) : _val(i) { };
    // or as "something" by consulting otermap
    oter_id(const std::string &v);
    oter_id(const char *v);

    // these std::string functions are provided for convenience, for others,
    //  best invoke as actual string; std::string( ter(1, 2, 3) ).substr(...
    const char *c_str() const;
    size_t size() const;
    int find(const std::string &v, const int start, const int end) const;
    int compare(size_t pos, size_t len, const char *s, size_t n = 0) const;
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

struct overmap_special_spawns {
    overmap_special_spawns(): group( NULL_ID ), min_population(0), max_population(0),
        min_radius(0), max_radius(0) {};
    mongroup_id group;
    int min_population;
    int max_population;
    int min_radius;
    int max_radius;
};

struct overmap_special_terrain {
    overmap_special_terrain() : p( 0, 0, 0 ) { };
    tripoint p;
    std::string connect;
    std::string terrain;
    std::set<std::string> flags;
};

class overmap_special
{
    public:
        bool operator<(const overmap_special &right) const
        {
            return (this->id.compare(right.id) < 0);
        }
        std::string id;
        std::list<overmap_special_terrain> terrains;
        int min_city_size, max_city_size;
        int min_city_distance, max_city_distance;
        int min_occurrences, max_occurrences;
        int height, width;
        bool rotatable;
        overmap_special_spawns spawns;
        std::list<std::string> locations;
        std::set<std::string> flags;
};

void load_overmap_specials(JsonObject &jo);

void clear_overmap_specials();

// Overmap "Zones"
// Areas which have special post-generation processing attached to them

enum overmap_zone {
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
