#pragma once
#ifndef CATA_SRC_OMDATA_H
#define CATA_SRC_OMDATA_H

#include <climits>
#include <cstddef>
#include <cstdint>
#include <bitset>
#include <list>
#include <set>
#include <vector>
#include <array>
#include <string>

#include "catacharset.h"
#include "color.h"
#include "common_types.h"
#include "int_id.h"
#include "point.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"
#include "optional.h"

struct city;
class overmap_land_use_code;
struct MonsterGroup;

using overmap_land_use_code_id = string_id<overmap_land_use_code>;
struct oter_t;
struct overmap_location;
class JsonObject;
class overmap_connection;
class overmap_special_batch;
class overmap_special;

using overmap_special_id = string_id<overmap_special>;

static const overmap_land_use_code_id land_use_code_forest( "forest" );
static const overmap_land_use_code_id land_use_code_wetland( "wetland" );
static const overmap_land_use_code_id land_use_code_wetland_forest( "wetland_forest" );
static const overmap_land_use_code_id land_use_code_wetland_saltwater( "wetland_saltwater" );

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

/** Number of bits needed to store directions. */
const size_t bits = static_cast<size_t>( -1 ) >> ( CHAR_BIT *sizeof( size_t ) - size );

/** Identifier for serialization purposes. */
const std::string &id( type dir );

/** Get Human readable name of a direction */
std::string name( type dir );

/** Various rotations. */
point rotate( const point &p, type dir );
tripoint rotate( const tripoint &p, type dir );
uint32_t rotate_symbol( uint32_t sym, type dir );

/** Returns point(0, 0) displaced in specified direction by a specified distance
 * @param dir Direction of displacement
 * @param dist Distance of displacement
 */
point displace( type dir, int dist = 1 );

/** Returns a sum of two numbers
 *  @param dir1 first number
 *  @param dir2 second number */
type add( type dir1, type dir2 );

/** Turn by 90 degrees to the left, to the right, or randomly (either left or right). */
type turn_left( type dir );
type turn_right( type dir );
type turn_random( type dir );

/** Returns an opposite direction. */
type opposite( type dir );

/** Returns a random direction. */
type random();

/** Whether these directions are parallel. */
bool are_parallel( type dir1, type dir2 );

} // namespace om_direction

class overmap_land_use_code
{
    public:
        overmap_land_use_code_id id = overmap_land_use_code_id::NULL_ID();

        int land_use_code = 0;
        std::string name;
        std::string detailed_definition;
        uint32_t symbol = 0;
        nc_color color = c_black;

        std::string get_symbol() const;

        // Used by generic_factory
        bool was_loaded = false;
        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;
};

struct overmap_spawns {
        overmap_spawns() : group( mongroup_id::NULL_ID() ) {}

        string_id<MonsterGroup> group;
        numeric_interval<int> population;

        bool operator==( const overmap_spawns &rhs ) const {
            return group == rhs.group && population == rhs.population;
        }

    protected:
        template<typename JsonObjectType>
        void load( JsonObjectType &jo ) {
            jo.read( "group", group );
            jo.read( "population", population );
        }
};

struct overmap_static_spawns : public overmap_spawns {
    int chance = 0;

    bool operator==( const overmap_static_spawns &rhs ) const {
        return overmap_spawns::operator==( rhs ) && chance == rhs.chance;
    }

    template<typename JsonStream>
    void deserialize( JsonStream &jsin ) {
        auto jo = jsin.get_object();
        overmap_spawns::load( jo );
        jo.read( "chance", chance );
    }
};

//terrain flags enum! this is for tracking the indices of each flag.
enum oter_flags {
    known_down = 0,
    known_up,
    no_rotate,    // this tile doesn't have four rotated versions (north, east, south, west)
    river_tile,
    has_sidewalk,
    line_drawing, // does this tile have 8 versions, including straights, bends, tees, and a fourway?
    subway_connection,
    lake,
    lake_shore,
    generic_loot,
    risk_high,
    risk_low,
    source_ammo,
    source_animals,
    source_books,
    source_chemistry,
    source_clothing,
    source_construction,
    source_cooking,
    source_drink,
    source_electronics,
    source_fabrication,
    source_farming,
    source_food,
    source_forage,
    source_fuel,
    source_gun,
    source_luxury,
    source_medicine,
    source_people,
    source_safety,
    source_tailoring,
    source_vehicles,
    source_weapon,
    num_oter_flags
};

struct oter_type_t {
    public:
        static const oter_type_t null_type;

    public:
        string_id<oter_type_t> id;
        std::string name;               // Untranslated name
        uint32_t symbol = 0;
        nc_color color = c_black;
        overmap_land_use_code_id land_use_code = overmap_land_use_code_id::NULL_ID();
        unsigned char see_cost = 0;     // Affects how far the player can see in the overmap
        unsigned char travel_cost = 5;  // Affects the pathfinding and travel times
        std::string extras = "none";
        int mondensity = 0;
        // Spawns are added to the submaps *once* upon mapgen of the submaps
        overmap_static_spawns static_spawns;
        bool was_loaded = false;

        std::string get_symbol() const;

        oter_type_t() = default;

        oter_id get_first() const;
        oter_id get_rotated( om_direction::type dir ) const;
        oter_id get_linear( size_t n ) const;

        bool has_flag( oter_flags flag ) const {
            return flags[flag];
        }

        void set_flag( oter_flags flag, bool value = true ) {
            flags[flag] = value;
        }

        void load( const JsonObject &jo, const std::string &src );
        void check() const;
        void finalize();

        bool is_rotatable() const {
            return !has_flag( no_rotate ) && !has_flag( line_drawing );
        }

        bool is_linear() const {
            return has_flag( line_drawing );
        }

    private:
        std::bitset<num_oter_flags> flags;
        std::vector<oter_id> directional_peers;

        void register_terrain( const oter_t &peer, size_t n, size_t max_n );
};

struct oter_t {
    private:
        const oter_type_t *type;

    public:
        oter_str_id id;         // definitive identifier.

        oter_t();
        oter_t( const oter_type_t &type );
        oter_t( const oter_type_t &type, om_direction::type dir );
        oter_t( const oter_type_t &type, size_t line );

        const string_id<oter_type_t> &get_type_id() const {
            return type->id;
        }

        std::string get_mapgen_id() const;
        oter_id get_rotated( om_direction::type dir ) const;

        std::string get_name() const {
            return _( type->name );
        }

        std::string get_symbol( const bool from_land_use_code = false ) const {
            return utf32_to_utf8( from_land_use_code ? symbol_alt : symbol );
        }

        nc_color get_color( const bool from_land_use_code = false ) const {
            return from_land_use_code ? type->land_use_code->color : type->color;
        }

        om_direction::type get_dir() const {
            return dir;
        }

        size_t get_line() const {
            return line;
        }

        unsigned char get_see_cost() const {
            return type->see_cost;
        }
        unsigned char get_travel_cost() const {
            return type->travel_cost;
        }

        const std::string &get_extras() const {
            return type->extras;
        }

        int get_mondensity() const {
            return type->mondensity;
        }

        const overmap_static_spawns &get_static_spawns() const {
            return type->static_spawns;
        }

        overmap_land_use_code_id get_land_use_code() const {
            return type->land_use_code;
        }

        bool type_is( const int_id<oter_type_t> &type_id ) const;
        bool type_is( const oter_type_t &type ) const;

        bool has_connection( om_direction::type dir ) const;

        bool has_flag( oter_flags flag ) const {
            return type->has_flag( flag );
        }

        bool is_hardcoded() const;

        bool is_rotatable() const {
            return type->is_rotatable();
        }

        bool is_linear() const {
            return type->is_linear();
        }

        bool is_river() const {
            return type->has_flag( river_tile );
        }

        bool is_wooded() const {
            return type->land_use_code == land_use_code_forest ||
                   type->land_use_code == land_use_code_wetland ||
                   type->land_use_code == land_use_code_wetland_forest ||
                   type->land_use_code == land_use_code_wetland_saltwater;
        }

        bool is_lake() const {
            return type->has_flag( lake );
        }

        bool is_lake_shore() const {
            return type->has_flag( lake_shore );
        }

    private:
        om_direction::type dir = om_direction::type::none;
        uint32_t symbol;
        uint32_t symbol_alt;
        size_t line = 0;         // Index of line. Only valid in case of line drawing.
};

// TODO: Deprecate these operators
bool operator==( const oter_id &lhs, const char *rhs );
bool operator!=( const oter_id &lhs, const char *rhs );

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

struct overmap_special_spawns : public overmap_spawns {
    numeric_interval<int> radius;

    bool operator==( const overmap_special_spawns &rhs ) const {
        return overmap_spawns::operator==( rhs ) && radius == rhs.radius;
    }

    template<typename JsonStream>
    void deserialize( JsonStream &jsin ) {
        auto jo = jsin.get_object();
        overmap_spawns::load( jo );
        jo.read( "radius", radius );
    }
};

struct overmap_special_terrain {
    overmap_special_terrain() = default;
    tripoint p;
    oter_str_id terrain;
    std::set<std::string> flags;
    std::set<string_id<overmap_location>> locations;

    template<typename JsonStream>
    void deserialize( JsonStream &jsin ) {
        auto om = jsin.get_object();
        om.read( "point", p );
        om.read( "overmap", terrain );
        om.read( "flags", flags );
        om.read( "locations", locations );
    }

    /**
     * Returns whether this terrain of the special can be placed on the specified terrain.
     * It's true if oter meets any of locations.
     */
    bool can_be_placed_on( const oter_id &oter ) const;
};

struct overmap_special_connection {
    tripoint p;
    cata::optional<tripoint> from;
    om_direction::type initial_dir = om_direction::type::invalid;
    // TODO: Remove it.
    string_id<oter_type_t> terrain;
    string_id<overmap_connection> connection;
    bool existing = false;

    template<typename JsonStream>
    void deserialize( JsonStream &jsin ) {
        auto jo = jsin.get_object();
        jo.read( "point", p );
        jo.read( "terrain", terrain );
        jo.read( "existing", existing );
        jo.read( "connection", connection );
        assign( jo, "from", from );
    }
};

class overmap_special
{
    public:
        /** Returns terrain at the given point. */
        const overmap_special_terrain &get_terrain_at( const tripoint &p ) const;
        /** @returns true if this special requires a city */
        bool requires_city() const;
        /** @returns whether the special at specified tripoint can belong to the specified city. */
        bool can_belong_to_city( const tripoint &p, const city &cit ) const;

        overmap_special_id id;
        std::list<overmap_special_terrain> terrains;
        std::vector<overmap_special_connection> connections;

        numeric_interval<int> city_size{ 0, INT_MAX };
        numeric_interval<int> city_distance{ 0, INT_MAX };
        numeric_interval<int> occurrences;

        bool rotatable = true;
        overmap_special_spawns spawns;
        std::set<std::string> flags;

        // Used by generic_factory
        bool was_loaded = false;
        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void check() const;
        // Minimum size of the box that can contain whole overmap special
        box dimensions;
    private:
        // These locations are the default values if ones are not specified for the individual OMTs.
        std::set<string_id<overmap_location>> default_locations;
};

namespace overmap_terrains
{

void load( const JsonObject &jo, const std::string &src );
void check_consistency();
void finalize();
void reset();

const std::vector<oter_t> &get_all();

} // namespace overmap_terrains

namespace overmap_land_use_codes
{

void load( const JsonObject &jo, const std::string &src );
void finalize();
void check_consistency();
void reset();

const std::vector<overmap_land_use_code> &get_all();

} // namespace overmap_land_use_codes

namespace overmap_specials
{

void load( const JsonObject &jo, const std::string &src );
void finalize();
void check_consistency();
void reset();

const std::vector<overmap_special> &get_all();

overmap_special_batch get_default_batch( const point &origin );
/**
 * Generates a simple special from a building id.
 */
overmap_special_id create_building_from( const string_id<oter_type_t> &base );

} // namespace overmap_specials

namespace city_buildings
{

void load( const JsonObject &jo, const std::string &src );

} // namespace city_buildings

#endif // CATA_SRC_OMDATA_H
