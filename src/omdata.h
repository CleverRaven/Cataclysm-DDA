#pragma once
#ifndef CATA_SRC_OMDATA_H
#define CATA_SRC_OMDATA_H

#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <list>
#include <new>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "assign.h"
#include "catacharset.h"
#include "color.h"
#include "common_types.h"
#include "coordinates.h"
#include "cube_direction.h"
#include "enum_bitset.h"
#include "mapgen_parameter.h"
#include "point.h"
#include "translations.h"
#include "type_id.h"

class overmap_land_use_code;
struct MonsterGroup;
struct city;
template <typename E> struct enum_traits;

using overmap_land_use_code_id = string_id<overmap_land_use_code>;
class JsonObject;
class JsonValue;
class overmap;
class overmap_connection;
class overmap_special;
class overmap_special_batch;
struct mapgen_arguments;
struct oter_t;
struct overmap_location;

inline const overmap_land_use_code_id land_use_code_forest( "forest" );
inline const overmap_land_use_code_id land_use_code_wetland( "wetland" );
inline const overmap_land_use_code_id land_use_code_wetland_forest( "wetland_forest" );
inline const overmap_land_use_code_id land_use_code_wetland_saltwater( "wetland_saltwater" );

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
    last
};

/** For the purposes of iteration. */
const std::array<type, 4> all = {{ type::north, type::east, type::south, type::west }};
const size_t size = all.size();

/** Number of bits needed to store directions. */
constexpr size_t bits = static_cast<size_t>( -1 ) >> ( CHAR_BIT *sizeof( size_t ) - size );

/** Get Human readable name of a direction */
std::string name( type dir );

/** Various rotations. */
point rotate( const point &p, type dir );
tripoint rotate( const tripoint &p, type dir );
template<typename Point, coords::scale Scale>
auto rotate( const coords::coord_point<Point, coords::origin::relative, Scale> &p, type dir )
-> coords::coord_point<Point, coords::origin::relative, Scale>
{
    return coords::coord_point<Point, coords::origin::relative, Scale> { rotate( p.raw(), dir ) };
}
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

type from_cube( cube_direction, const std::string &error_msg );

} // namespace om_direction

template<>
struct enum_traits<om_direction::type> {
    static constexpr om_direction::type last = om_direction::type::last;
};

class overmap_land_use_code
{
    public:
        overmap_land_use_code_id id = overmap_land_use_code_id::NULL_ID();
        std::vector<std::pair<overmap_land_use_code_id, mod_id>> src;

        int land_use_code = 0;
        translation name;
        translation detailed_definition;
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
        void load( const JsonObject &jo ) {
            jo.read( "group", group );
            jo.read( "population", population );
        }
};

struct overmap_static_spawns : public overmap_spawns {
    int chance = 0;

    bool operator==( const overmap_static_spawns &rhs ) const {
        return overmap_spawns::operator==( rhs ) && chance == rhs.chance;
    }

    void deserialize( const JsonValue &jsin ) {
        JsonObject jo = jsin.get_object();
        overmap_spawns::load( jo );
        jo.read( "chance", chance );
    }
};

//terrain flags enum! this is for tracking the indices of each flag.
enum class oter_flags : int {
    known_down = 0,
    known_up,
    no_rotate,    // this tile doesn't have four rotated versions (north, east, south, west)
    should_not_spawn,
    river_tile,
    has_sidewalk,
    ignore_rotation_for_adjacency,
    line_drawing, // does this tile have 8 versions, including straights, bends, tees, and a fourway?
    subway_connection,
    requires_predecessor,
    lake,
    lake_shore,
    ravine,
    ravine_edge,
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

template<>
struct enum_traits<oter_flags> {
    static constexpr oter_flags last = oter_flags::num_oter_flags;
};

struct oter_type_t {
    public:
        static const oter_type_t null_type;

        string_id<oter_type_t> id;
        std::vector<std::pair<string_id<oter_type_t>, mod_id>> src;
        translation name;
        uint32_t symbol = 0;
        nc_color color = c_black;
        overmap_land_use_code_id land_use_code = overmap_land_use_code_id::NULL_ID();
        std::vector<std::string> looks_like;
        unsigned char see_cost = 0;     // Affects how far the player can see in the overmap
        unsigned char travel_cost = 5;  // Affects the pathfinding and travel times
        std::string extras = "none";
        int mondensity = 0;
        effect_on_condition_id entry_EOC;
        effect_on_condition_id exit_EOC;
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
            flags.set( flag, value );
        }

        void load( const JsonObject &jo, const std::string &src );
        void check() const;
        void finalize();

        bool is_rotatable() const {
            return !has_flag( oter_flags::no_rotate ) && !has_flag( oter_flags::line_drawing );
        }

        bool is_linear() const {
            return has_flag( oter_flags::line_drawing );
        }

        bool has_connections() const {
            return !connect_group.empty();
        }

        bool connects_to( const oter_type_id &other ) const {
            return has_connections() && connect_group == other->connect_group;
        }

    private:
        enum_bitset<oter_flags> flags;
        std::vector<oter_id> directional_peers;
        std::string connect_group; // Group for connection when rendering overmap tiles

        void register_terrain( const oter_t &peer, size_t n, size_t max_n );
};

struct oter_t {
    private:
        const oter_type_t *type;

    public:
        oter_str_id id;         // definitive identifier.
        std::vector < std::pair < oter_str_id, mod_id>> src;

        oter_t();
        explicit oter_t( const oter_type_t &type );
        oter_t( const oter_type_t &type, om_direction::type dir );
        oter_t( const oter_type_t &type, size_t line );

        const string_id<oter_type_t> &get_type_id() const {
            return type->id;
        }

        std::string get_mapgen_id() const;
        oter_id get_rotated( om_direction::type dir ) const;

        std::string get_name() const {
            return type->name.translated();
        }

        std::string get_symbol( const bool from_land_use_code = false ) const {
            return utf32_to_utf8( from_land_use_code ? symbol_alt : symbol );
        }

        uint32_t get_uint32_symbol() const {
            return symbol;
        }

        nc_color get_color( const bool from_land_use_code = false ) const {
            return from_land_use_code ? type->land_use_code->color : type->color;
        }

        // dir is only meaningful for rotatable, non-linear terrain.  If you
        // need an answer that also works for linear terrain, call
        // get_rotation() instead.
        om_direction::type get_dir() const {
            return dir;
        }

        size_t get_line() const {
            return line;
        }
        void get_rotation_and_subtile( int &rotation, int &subtile ) const;
        int get_rotation() const;

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

        effect_on_condition_id get_entry_EOC() const {
            return type->entry_EOC;
        }

        effect_on_condition_id get_exit_EOC() const {
            return type->exit_EOC;
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
            return type->has_flag( oter_flags::river_tile );
        }

        bool is_wooded() const {
            return type->land_use_code == land_use_code_forest ||
                   type->land_use_code == land_use_code_wetland ||
                   type->land_use_code == land_use_code_wetland_forest ||
                   type->land_use_code == land_use_code_wetland_saltwater;
        }

        bool is_lake() const {
            return type->has_flag( oter_flags::lake );
        }

        bool is_lake_shore() const {
            return type->has_flag( oter_flags::lake_shore );
        }

        bool is_ravine() const {
            return type->has_flag( oter_flags::ravine );
        }

        bool is_ravine_edge() const {
            return type->has_flag( oter_flags::ravine_edge );
        }

    private:
        om_direction::type dir = om_direction::type::none;
        uint32_t symbol;
        uint32_t symbol_alt;
        size_t line = 0;         // Index of line. Only valid in case of line drawing.
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
constexpr int OMSPEC_FREQ = 15;

struct overmap_special_spawns : public overmap_spawns {
    numeric_interval<int> radius;

    bool operator==( const overmap_special_spawns &rhs ) const {
        return overmap_spawns::operator==( rhs ) && radius == rhs.radius;
    }

    void deserialize( const JsonValue &jsin ) {
        JsonObject jo = jsin.get_object();
        overmap_spawns::load( jo );
        jo.read( "radius", radius );
    }
};

// This is the information needed to know whether you can place a particular
// piece of an overmap_special at a particular location
struct overmap_special_locations {
    tripoint p;
    cata::flat_set<string_id<overmap_location>> locations;

    /**
     * Returns whether this terrain of the special can be placed on the specified terrain.
     * It's true if oter meets any of locations.
     */
    bool can_be_placed_on( const oter_id &oter ) const;
    void deserialize( const JsonArray &ja );
};

struct overmap_special_terrain : overmap_special_locations {
    overmap_special_terrain() = default;
    overmap_special_terrain( const tripoint &, const oter_str_id &,
                             const cata::flat_set<string_id<overmap_location>> &,
                             const std::set<std::string> & );
    oter_str_id terrain;
    std::set<std::string> flags;

    void deserialize( const JsonObject &om );
};

struct overmap_special_connection {
    tripoint p;
    std::optional<tripoint> from;
    cube_direction initial_dir = cube_direction::last; // NOLINT(cata-serialize)
    // TODO: Remove it.
    string_id<oter_type_t> terrain;
    string_id<overmap_connection> connection;
    bool existing = false;

    void deserialize( const JsonValue &jsin ) {
        JsonObject jo = jsin.get_object();
        jo.read( "point", p );
        jo.read( "terrain", terrain );
        jo.read( "existing", existing );
        jo.read( "connection", connection );
        assign( jo, "from", from );
    }
};

struct overmap_special_placement_constraints {
    numeric_interval<int> city_size{ 0, INT_MAX };
    numeric_interval<int> city_distance{ 0, INT_MAX };
    numeric_interval<int> occurrences;
};

enum class overmap_special_subtype {
    fixed,
    mutable_,
    last
};

template<>
struct enum_traits<overmap_special_subtype> {
    static constexpr overmap_special_subtype last = overmap_special_subtype::last;
};

struct overmap_special_data;
struct special_placement_result;

class overmap_special
{
    public:
        overmap_special() = default;
        overmap_special( const overmap_special_id &, const overmap_special_terrain & );
        overmap_special_subtype get_subtype() const {
            return subtype_;
        }
        const overmap_special_placement_constraints &get_constraints() const {
            return constraints_;
        }
        bool is_rotatable() const {
            return rotatable_;
        }
        bool has_eoc() const {
            return has_eoc_;
        }
        effect_on_condition_id get_eoc() const {
            return eoc;
        }
        bool can_spawn() const;
        /** Returns terrain at the given point. */
        const overmap_special_terrain &get_terrain_at( const tripoint &p ) const;
        /** @returns true if this special requires a city */
        bool requires_city() const;
        /** @returns whether the special at specified tripoint can belong to the specified city. */
        bool can_belong_to_city( const tripoint_om_omt &p, const city &cit ) const;
        const cata::flat_set<std::string> &get_flags() const {
            return flags_;
        }
        bool has_flag( const std::string & ) const;
        int get_priority() const {
            return priority_;
        }
        int longest_side() const;
        std::vector<overmap_special_terrain> preview_terrains() const;
        std::vector<overmap_special_locations> required_locations() const;
        int score_rotation_at( const overmap &om, const tripoint_om_omt &p,
                               om_direction::type r ) const;
        special_placement_result place(
            overmap &om, const tripoint_om_omt &origin, om_direction::type dir,
            const city &cit, bool must_be_unexplored ) const;
        const overmap_special_spawns &get_monster_spawns() const {
            return monster_spawns_;
        }

        void force_one_occurrence();

        const mapgen_parameters &get_params() const {
            return mapgen_params_;
        }
        mapgen_arguments get_args( const mapgendata & ) const;

        overmap_special_id id;
        // TODO: fix how this works with fake specials
        // Due to fake specials being created after data loading, if any mod has a region settings
        // which has a fake special defined in DDA, it will count as from the same src, and thus
        // a duplicate.
        //std::vector<std::pair<overmap_special_id, mod_id>> src;

        // Used by generic_factory
        bool was_loaded = false;
        void load( const JsonObject &jo, const std::string &src );
        void finalize();
        void finalize_mapgen_parameters();
        void check() const;
    private:
        overmap_special_subtype subtype_;
        overmap_special_placement_constraints constraints_;
        shared_ptr_fast<const overmap_special_data> data_;
        effect_on_condition_id eoc;
        bool has_eoc_ = false;
        bool rotatable_ = true;
        overmap_special_spawns monster_spawns_;
        cata::flat_set<std::string> flags_;
        int priority_ = 0;

        // These locations are the default values if ones are not specified for the individual OMTs.
        cata::flat_set<string_id<overmap_location>> default_locations_;
        mapgen_parameters mapgen_params_;
};

struct overmap_special_migration {
    public:
        static void load_migrations( const JsonObject &jo, const std::string &src );
        static void reset();
        void load( const JsonObject &jo, std::string_view src );
        static void check();
        // Check if the given overmap special should be migrated
        static bool migrated( const overmap_special_id &os_id );
        // Get the migrated id. Returns null id if the special was removed,
        // or the same id if no migration is necessary
        static overmap_special_id migrate( const overmap_special_id &old_id );

    private:
        bool was_loaded = false;
        overmap_special_migration_id id;
        std::vector<std::pair<overmap_special_migration_id, mod_id>> src;
        overmap_special_id new_id;
        friend generic_factory<overmap_special_migration>;
        friend struct mod_tracker;
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
void finalize_mapgen_parameters();
void check_consistency();
void reset();

const std::vector<overmap_special> &get_all();

overmap_special_batch get_default_batch( const point_abs_om &origin );
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
