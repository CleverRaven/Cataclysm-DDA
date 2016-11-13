#ifndef OMDATA_H
#define OMDATA_H

#include "color.h"
#include "common_types.h"
#include "json.h"
#include "enums.h"
#include "int_id.h"
#include "string_id.h"

#include <string>
#include <vector>
#include <limits>
#include <list>
#include <set>

struct MonsterGroup;
struct city;
struct overmap_special_location;

using mongroup_id = string_id<MonsterGroup>;

class overmap;

struct overmap_spawns {
    mongroup_id group;
    numeric_interval<int> population;
    int chance = 0;
};

//terrain flags enum! this is for tracking the indices of each flag.
enum oter_flags {
    known_down = 0,
    known_up,
    river_tile,
    road_tile,
    has_sidewalk,
    allow_road,
    rotates,      // does this tile have four versions, one for each direction?
    line_drawing, // does this tile have 8 versions, including straights, bends, tees, and a fourway?
    num_oter_flags
};

struct oter_t;
using oter_id = int_id<oter_t>;
using oter_str_id = string_id<oter_t>;

struct oter_t {
        oter_str_id id;      /// definitive identifier
        oter_id loadid;      /// position in 'terlist'
        std::string name;
        long sym; // This is a long, so we can support curses linedrawing
        nc_color color;
        unsigned char see_cost; // Affects how far the player can see in the overmap
        std::string extras;
        int mondensity;
        /**
         * base identifier; either the same as id, or id without directional variations. (ie, 'house' / 'house_west' )
         */
        std::string id_base;
        int loadid_base;                        /// self || directional_peers[0]? or seperate base_oter_map ?
        std::vector<oter_id> directional_peers; /// fast reliable method of determining whatever_west, etc.
        std::string
        id_mapgen;  // *only* for mapgen and almost always == id_base. Unless line_drawing / road.

        // Spawns are added to the submaps *once* upon mapgen of the submaps
        overmap_spawns static_spawns;
        //this bitset contains boolean values for:
        //is_asphalt, is_building, is_subway, is_sewer, is_ants,
        //is_base_terrain, known_down, known_up, is_river,
        //is_road, has_sidewalk, allow_road, rotates, line_drawing
    private:
        std::bitset<num_oter_flags> flags; //contains a bitset for all the bools this terrain might have.
    public:
        static size_t count();  /// Overall number of loaded objects

        bool has_flag( oter_flags flag ) const {
            return flags[flag];
        }

        void set_flag( oter_flags flag, bool value = true ) {
            flags[flag] = value;
        }
};

// @todo: Deprecate these operators
bool operator==( const oter_id &lhs, const char *rhs );
bool operator!=( const oter_id &lhs, const char *rhs );
bool operator>=( const oter_id &lhs, const char *rhs );
bool operator<=( const oter_id &lhs, const char *rhs );

typedef oter_id oter_iid;

/** Direction on the overmap. */
namespace om_direction
{
/** Basic enum for directions. */
enum class type : int {
    invalid = -1,
    none,
    north = none,
    east,
    south,
    west,
};

/** For the purposes of iteration. */
const std::array<type, 4> all = {{ type::north, type::east, type::south, type::west }};
const size_t size = all.size();

/** Human readable name of @param dir. */
const std::string &name( type dir );

/** Various rotations. */
point rotate( const point &p, type dir );
tripoint rotate( const tripoint &p, type dir );
oter_id rotate( const oter_id &oter, type dir );

/** Returns point(0, 0) displaced in direction @param dir by the @param dist. */
point displace( type dir, int dist = 1 );

/** Turn by 90 degrees to the left, to the right, or randomly (either left or right). */
type turn_left( type dir );
type turn_right( type dir );
type turn_random( type dir );

/** Returns an opposite direction. */
type opposite( type dir );

/** Returns a random direction. */
type random();

};

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

struct overmap_special_spawns : public JsonDeserializer {
    mongroup_id group;
    numeric_interval<int> population;
    numeric_interval<int> radius;

    bool operator==( const overmap_special_spawns &rhs ) const {
        return group == rhs.group &&
               population == rhs.population &&
               radius == rhs.radius;
    }

    void deserialize( JsonIn &jsin ) override {
        JsonObject jo = jsin.get_object();
        jo.read( "group", group );
        jo.read( "population", population );
        jo.read( "radius", radius );
    }
};

struct overmap_special_terrain : public JsonDeserializer {
    overmap_special_terrain() { };
    tripoint p;
    oter_str_id terrain;
    std::set<std::string> flags;

    void deserialize( JsonIn &jsin ) override {
        JsonObject om = jsin.get_object();
        om.read( "point", p );
        om.read( "overmap", terrain );
        om.read( "flags", flags );
    }
};

struct overmap_special_connection : public JsonDeserializer {
    tripoint p;
    oter_str_id terrain;
    bool existing = false;

    void deserialize( JsonIn &jsin ) override {
        JsonObject jo = jsin.get_object();
        jo.read( "point", p );
        jo.read( "terrain", terrain );
        jo.read( "existing", existing );
    }
};

class overmap_special
{
    public:
        /** Returns terrain at the given point. */
        const overmap_special_terrain &get_terrain_at( const tripoint &p ) const;
        /**
         * Returns whether the special can be placed on the specified terrain.
         * It's true if @ref oter meets any of @ref locations.
         */
        bool can_be_placed_on( const oter_id &oter ) const;
        /** Returns whether this special requires a city at all. */
        bool requires_city() const;
        /** Returns whether the special at @ref p can belong to the specified city. */
        bool can_belong_to_city( const tripoint &p, const city &cit ) const;

        string_id<overmap_special> id;
        std::list<overmap_special_terrain> terrains;
        std::vector<overmap_special_connection> connections;

        numeric_interval<int> city_size{ 0, INT_MAX };
        numeric_interval<int> city_distance{ 0, INT_MAX };
        numeric_interval<int> occurrences;

        bool rotatable = true;
        overmap_special_spawns spawns;
        std::set<const overmap_special_location *> locations;
        std::set<std::string> flags;

        // Used by generic_factory
        bool was_loaded = false;
        void load( JsonObject &jo, const std::string &src );
        void check() const;
};

namespace overmap_specials
{

void load( JsonObject &jo, const std::string &src );
void check_consistency();
void reset();

}

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
