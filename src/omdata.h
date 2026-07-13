#pragma once
#ifndef CATA_SRC_OMDATA_H
#define CATA_SRC_OMDATA_H

#include <algorithm>
#include <array>
#include <climits>
#include <cstddef>
#include <cstdint>
#include <list>
#include <map>
#include <memory> // IWYU pragma: keep
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "cata_assert.h"
#include "cata_views.h"
#include "color.h"
#include "common_types.h"
#include "coordinates.h"
#include "cube_direction.h"
#include "debug.h"
#include "distribution.h"
#include "enum_bitset.h"
#include "enum_conversions.h"
#include "flat_set.h"
#include "flexbuffer_json.h"
#include "json.h"
#include "mapgen_parameter.h"
#include "memory_fast.h"
#include "overmap_location.h"
#include "point.h"
#include "translation.h"
#include "type_id.h"

class mapgendata;
class overmap_land_use_code;
struct MonsterGroup;
struct city;
struct mutable_overmap_terrain_join;
struct mutable_overmap_placement_rule_remainder;
template <typename E> struct enum_traits;
template <typename T> class generic_factory;

using join_map = std::unordered_map<cube_direction, mutable_overmap_terrain_join>;
using overmap_land_use_code_id = string_id<overmap_land_use_code>;
class overmap;
class overmap_connection;
class overmap_special_batch;
enum class om_vision_level : int8_t;
struct map_data_summary;
struct mapgen_arguments;
struct oter_t;

inline const overmap_land_use_code_id land_use_code_forest( "forest" );
inline const overmap_land_use_code_id land_use_code_wetland( "wetland" );
inline const overmap_land_use_code_id land_use_code_wetland_forest( "wetland_forest" );
inline const overmap_land_use_code_id land_use_code_wetland_saltwater( "wetland_saltwater" );

template<typename Tripoint>
struct pos_dir {
    Tripoint p;
    cube_direction dir;

    pos_dir opposite() const;

    void serialize( JsonOut &jsout ) const;
    void deserialize( const JsonArray &ja );

    bool operator==( const pos_dir &r ) const;
    bool operator<( const pos_dir &r ) const;
};

template<typename Tripoint>
pos_dir<Tripoint> pos_dir<Tripoint>::opposite() const
{
    switch( dir ) {
        case cube_direction::north:
            return { p + tripoint::north, cube_direction::south };
        case cube_direction::east:
            return { p + tripoint::east, cube_direction::west };
        case cube_direction::south:
            return { p + tripoint::south, cube_direction::north };
        case cube_direction::west:
            return { p + tripoint::west, cube_direction::east };
        case cube_direction::above:
            return { p + tripoint::above, cube_direction::below };
        case cube_direction::below:
            return { p + tripoint::below, cube_direction::above };
        case cube_direction::last:
            break;
    }
    cata_fatal( "Invalid cube_direction" );
}

template<typename Tripoint>
void pos_dir<Tripoint>::serialize( JsonOut &jsout ) const
{
    jsout.start_array();
    jsout.write( p );
    jsout.write( dir );
    jsout.end_array();
}

template<typename Tripoint>
void pos_dir<Tripoint>::deserialize( const JsonArray &ja )
{
    if( ja.size() != 2 ) {
        ja.throw_error( "Expected array of size 2" );
    }
    ja.read( 0, p );
    ja.read( 1, dir );
}

template<typename Tripoint>
bool pos_dir<Tripoint>::operator==( const pos_dir<Tripoint> &r ) const
{
    return p == r.p && dir == r.dir;
}

template<typename Tripoint>
bool pos_dir<Tripoint>::operator<( const pos_dir<Tripoint> &r ) const
{
    return std::tie( p, dir ) < std::tie( r.p, r.dir );
}

extern template struct pos_dir<tripoint_om_omt>;
extern template struct pos_dir<tripoint_rel_omt>;

using om_pos_dir = pos_dir<tripoint_om_omt>;
using rel_pos_dir = pos_dir<tripoint_rel_omt>;

enum class join_type {
    mandatory,
    available,
    last
};

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
auto rotate( const coords::coord_point_ob<Point, coords::origin::relative, Scale> &p, type dir )
-> coords::coord_point<Point, coords::origin::relative, Scale>
{
    return coords::coord_point<Point, coords::origin::relative, Scale> { rotate( p.raw(), dir ) };
}
uint32_t rotate_symbol( uint32_t sym, type dir );

/** Returns point(0, 0) displaced in specified direction by a specified distance
 * @param dir Direction of displacement
 * @param dist Distance of displacement
 */
point_rel_omt displace( type dir, int dist = 1 );

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

namespace io
{
template<>
std::string enum_to_string<om_vision_level>( om_vision_level );
} // namespace io

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
        void load( const JsonObject &jo, std::string_view src );
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
    water,
    river_tile,
    has_sidewalk,
    road,
    highway,
    highway_reserved,
    highway_special,
    bridge,
    ignore_rotation_for_adjacency,
    line_drawing, // does this tile have 8 versions, including straights, bends, tees, and a fourway?
    subway_connection,
    requires_predecessor,
    lake,
    lake_shore,
    ocean,
    ocean_shore,
    ravine,
    ravine_edge,
    pp_generate_riot_damage,
    pp_generate_ruined,
    generic_loot,
    risk_extreme,
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

enum class oter_travel_cost_type : int {
    other,
    impassable,
    highway,
    road,
    field,
    dirt_road,
    trail,
    forest,
    shore,
    swamp,
    water,
    air,
    structure,
    roof,
    basement,
    tunnel,
    last
};

template<>
struct enum_traits<oter_travel_cost_type> {
    static constexpr oter_travel_cost_type last = oter_travel_cost_type::last;
};

class oter_vision
{
    public:
        struct blended_omt {
            oter_id id;
            std::string sym;
            nc_color color;
            std::string name;
        };

        struct level {
            translation name;
            uint32_t symbol = 0;
            nc_color color = c_black;
            std::string looks_like;
            bool blends_adjacent;

            void deserialize( const JsonObject &jo );
        };
        const level *viewed( om_vision_level ) const;

        static void load_oter_vision( const JsonObject &jo, const std::string &src );
        static void finalize_all();
        static void reset();
        static void check_oter_vision();
        static const std::vector<oter_vision> &get_all();

        oter_vision_id get_id() const;

        void load( const JsonObject &jo, std::string_view src );
        void check() const;

        static blended_omt get_blended_omt_info( const tripoint_abs_omt &omp, om_vision_level vision );

    private:
        friend class generic_factory<oter_vision>;
        friend struct mod_tracker;
        oter_vision_id id;
        std::vector<std::pair<oter_vision_id, mod_id>> src;
        bool was_loaded = false;

        std::vector<level> levels;
};

struct oter_type_t {
    public:
        static const oter_type_t null_type;

        string_id<oter_type_t> id;
        std::vector<std::pair<string_id<oter_type_t>, mod_id>> src;
    private:
        friend struct oter_t;
        translation name;
        uint32_t symbol = 0;
        nc_color color = c_black;
    public:
        overmap_land_use_code_id land_use_code = overmap_land_use_code_id::NULL_ID();
        string_id<map_data_summary> default_map_data;
        std::vector<std::string> looks_like;
        enum class see_costs : uint8_t {
            all_clear, // no vertical or horizontal obstacles
            none, // no horizontal obstacles
            low, // low horizontal obstacles or few higher ones
            medium, // medium horizontal obstacles
            spaced_high, //
            high, // 0.9
            full_high, // 0.99
            opaque, // cannot see through
            last
        };
        static double see_cost_value( see_costs cost );

        see_costs see_cost = see_costs::none;     // Affects how far the player can see in the overmap
        oter_travel_cost_type travel_cost_type =
            oter_travel_cost_type::other;  // Affects the pathfinding and travel times
        std::string extras = "none";
        int mondensity = 0;
        effect_on_condition_id entry_EOC;
        effect_on_condition_id exit_EOC;
        // Spawns are added to the submaps *once* upon mapgen of the submaps
        overmap_static_spawns static_spawns;
        bool was_loaded = false;
        std::optional<ter_str_id> uniform_terrain;

        oter_vision_id vision_levels;

        std::string get_symbol() const;

        uint32_t get_uint32_symbol() const {
            return symbol;
        }

        nc_color get_color() const {
            return color;
        }

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

        void load( const JsonObject &jo, std::string_view src );
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

template<>
struct enum_traits<oter_type_t::see_costs> {
    static constexpr oter_type_t::see_costs last = oter_type_t::see_costs::last;
};

namespace io
{
template<>
std::string enum_to_string<oter_type_t::see_costs>( oter_type_t::see_costs );
} // namespace io

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

        bool blends_adjacent( om_vision_level vision ) const;

        std::string get_name( om_vision_level ) const;
        std::string get_symbol( om_vision_level, bool from_land_use_code = false ) const;
        uint32_t get_uint32_symbol() const;
        nc_color get_color( om_vision_level, bool from_land_use_code = false ) const;

        std::string get_tileset_id( om_vision_level vision ) const;

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

        double get_see_cost() const {
            return oter_type_t::see_cost_value( type->see_cost );
        }
        bool can_see_down_through() const {
            return type->see_cost == oter_type_t::see_costs::all_clear;
        }
        oter_travel_cost_type get_travel_cost_type() const {
            return type->travel_cost_type;
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

        bool is_water() const {
            return type->has_flag( oter_flags::water );
        }
        bool is_lake() const {
            return type->has_flag( oter_flags::lake );
        }

        bool is_lake_shore() const {
            return type->has_flag( oter_flags::lake_shore );
        }

        bool is_ocean() const {
            return type->has_flag( oter_flags::ocean );
        }

        bool is_ocean_shore() const {
            return type->has_flag( oter_flags::ocean_shore );
        }

        bool is_ravine() const {
            return type->has_flag( oter_flags::ravine );
        }

        bool is_ravine_edge() const {
            return type->has_flag( oter_flags::ravine_edge );
        }

        bool has_uniform_terrain() const {
            return !!type->uniform_terrain;
        }

        std::optional<ter_str_id> get_uniform_terrain() const {
            return type->uniform_terrain;
        }

        bool is_road() const {
            return type->has_flag( oter_flags::road );
        }

        bool is_highway() const {
            return type->has_flag( oter_flags::highway );
        }

        bool is_highway_reserved() const {
            return type->has_flag( oter_flags::highway_reserved );
        }

        bool is_highway_special() const {
            return type->has_flag( oter_flags::highway_special );
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
    tripoint_rel_omt p;
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
    overmap_special_terrain( const tripoint_rel_omt &, const oter_str_id &,
                             const cata::flat_set<string_id<overmap_location>> &,
                             const std::set<std::string> & );
    oter_str_id terrain;
    std::set<std::string> flags;
    std::optional<faction_id> camp_owner;
    translation camp_name;

    void deserialize( const JsonObject &om );
};

struct overmap_special_connection {
    tripoint_rel_omt p;
    std::optional<tripoint_rel_omt> from;
    cube_direction initial_dir = cube_direction::last; // NOLINT(cata-serialize)
    // TODO: Remove it.
    string_id<oter_type_t> terrain;
    string_id<overmap_connection> connection;
    bool existing = false;

    void deserialize( const JsonObject &jo );
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
        const overmap_special_terrain &get_terrain_at( const tripoint_rel_omt &p ) const;
        /** @returns true if this special requires a city */
        bool requires_city() const;
        /** @returns whether the special at specified tripoint can belong to the specified city. */
        bool can_belong_to_city( const tripoint_om_omt &p, const city &cit, const overmap &omap ) const;
        const cata::flat_set<std::string> &get_flags() const {
            return flags_;
        }
        bool has_flag( const std::string & ) const;
        int get_priority() const {
            return priority_;
        }
        int longest_side() const;
        //NOTE: only useful for fixed overmap special
        std::vector<overmap_special_terrain> get_terrains() const;
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
        void load( const JsonObject &jo, std::string_view src );
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
        static void finalize_all();
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

struct overmap_special_data {
    virtual ~overmap_special_data() = default;
    virtual void finalize(
        const std::string &context,
        const cata::flat_set<string_id<overmap_location>> &default_locations ) = 0;
    virtual void finalize_mapgen_parameters(
        mapgen_parameters &, const std::string &context ) const = 0;
    virtual void check( const std::string &context ) const = 0;
    virtual std::vector<overmap_special_terrain> get_terrains() const = 0;
    virtual std::vector<overmap_special_terrain> preview_terrains() const = 0;
    virtual std::vector<overmap_special_locations> required_locations() const = 0;
    virtual int score_rotation_at( const overmap &om, const tripoint_om_omt &p,
                                   om_direction::type r ) const = 0;
    virtual special_placement_result place(
        overmap &om, const tripoint_om_omt &origin, om_direction::type dir, bool blob,
        const city &cit, bool must_be_unexplored ) const = 0;
};

struct fixed_overmap_special_data : overmap_special_data {
    fixed_overmap_special_data() = default;
    explicit fixed_overmap_special_data( const overmap_special_terrain &ter )
        : terrains{ ter } {
    }

    void finalize(
        const std::string &/*context*/,
        const cata::flat_set<string_id<overmap_location>> &default_locations ) override;
    void finalize_mapgen_parameters( mapgen_parameters &params,
                                     const std::string &context ) const override;
    void check( const std::string &context ) const override;
    const overmap_special_terrain &get_terrain_at( const tripoint_rel_omt &p ) const;
    std::vector<overmap_special_terrain> get_terrains() const override;
    std::vector<overmap_special_terrain> preview_terrains() const override;
    std::vector<overmap_special_locations> required_locations() const override;
    int score_rotation_at( const overmap &om, const tripoint_om_omt &p,
                           om_direction::type r ) const override;
    special_placement_result place(
        overmap &om, const tripoint_om_omt &origin, om_direction::type dir, bool blob,
        const city &cit, bool must_be_unexplored ) const override;

    std::vector<overmap_special_terrain> terrains;
    std::vector<overmap_special_connection> connections;
};

struct mutable_overmap_join {
    std::string id;
    std::string opposite_id;
    cata::flat_set<string_id<overmap_location>> into_locations;
    unsigned priority; // NOLINT(cata-serialize)
    const mutable_overmap_join *opposite = nullptr; // NOLINT(cata-serialize)

    void deserialize( const JsonValue &jin ) {
        if( jin.test_string() ) {
            id = jin.get_string();
        } else {
            JsonObject jo = jin.get_object();
            jo.read( "id", id, true );
            jo.read( "into_locations", into_locations, true );
            jo.read( "opposite", opposite_id, true );
        }
    }
};

struct mutable_overmap_terrain_join {
    std::string join_id;
    const mutable_overmap_join *join = nullptr; // NOLINT(cata-serialize)
    cata::flat_set<std::string> alternative_join_ids;
    cata::flat_set<const mutable_overmap_join *> alternative_joins; // NOLINT(cata-serialize)
    join_type type = join_type::mandatory;

    void finalize( const std::string &context,
                   const std::unordered_map<std::string, mutable_overmap_join *> &joins );
    void deserialize( const JsonValue &jin );
};

struct mutable_special_connection {
    string_id<overmap_connection> connection;

    void deserialize( const JsonObject &jo ) {
        jo.read( "connection", connection );
    }

    void check( const std::string &context ) const;
};

struct mutable_overmap_terrain {
    oter_str_id terrain;
    cata::flat_set<string_id<overmap_location>> locations;
    join_map joins;
    std::map<cube_direction, mutable_special_connection> connections;
    std::optional<faction_id> camp_owner;
    translation camp_name;

    void finalize( const std::string &context,
                   const std::unordered_map<std::string, mutable_overmap_join *> &special_joins,
                   const cata::flat_set<string_id<overmap_location>> &default_locations );
    void check( const std::string &context ) const;
    void deserialize( const JsonObject &jo );
};

struct mutable_overmap_placement_rule_piece {
    std::string overmap_id;
    const mutable_overmap_terrain *overmap; // NOLINT(cata-serialize)
    tripoint_rel_omt pos;
    om_direction::type rot = om_direction::type::north;

    void deserialize( const JsonObject &jo ) {
        jo.read( "overmap", overmap_id, true );
        jo.read( "pos", pos, true );
        jo.read( "rot", rot, true );
    }
};

struct mutable_overmap_piece_candidate {
    const mutable_overmap_terrain *overmap; // NOLINT(cata-serialize)
    tripoint_om_omt pos;
    om_direction::type rot = om_direction::type::north;
};

struct mutable_overmap_placement_rule {
    std::string name;
    std::vector<mutable_overmap_placement_rule_piece> pieces;
    // NOLINTNEXTLINE(cata-serialize)
    std::vector<std::pair<rel_pos_dir, const mutable_overmap_terrain_join *>> outward_joins;
    int_distribution max = int_distribution( INT_MAX );
    int weight = INT_MAX;

    std::string description() const;
    void finalize( const std::string &context,
                   const std::unordered_map<std::string, mutable_overmap_terrain> &special_overmaps );
    void check( const std::string &context ) const;
    mutable_overmap_placement_rule_remainder realise() const;
    void deserialize( const JsonObject &jo );
};

struct mutable_overmap_placement_rule_remainder {
    const mutable_overmap_placement_rule *parent;
    int max = INT_MAX;
    int weight = INT_MAX;

    std::string description() const {
        return parent->description();
    }

    int get_weight() const {
        return std::min( max, weight );
    }

    bool is_exhausted() const {
        return get_weight() == 0;
    }

    void decrement() {
        --max;
    }

    std::vector<tripoint_rel_omt> positions( om_direction::type rot ) const {
        std::vector<tripoint_rel_omt> result;
        result.reserve( parent->pieces.size() );
        for( const mutable_overmap_placement_rule_piece &piece : parent->pieces ) {
            result.push_back( rotate( piece.pos, rot ) );
        }
        return result;
    }
    cata::views::transform<std::vector<mutable_overmap_placement_rule_piece>, mutable_overmap_piece_candidate>
    pieces( const tripoint_om_omt &origin, om_direction::type rot ) const;
    using dest_outward_t = std::pair<om_pos_dir, const mutable_overmap_terrain_join *>;
    cata::views::transform<std::vector<std::pair<rel_pos_dir, const mutable_overmap_terrain_join *>>, dest_outward_t>
            outward_joins( const tripoint_om_omt &origin, om_direction::type rot ) const;
};

struct special_placement_result {
    std::vector<tripoint_om_omt> omts_used;
    std::vector<std::pair<om_pos_dir, std::string>> joins_used;
};

// When building a mutable overmap special we maintain a collection of
// unresolved joins.  We need to be able to index that collection in
// various ways, so it gets its own struct to maintain the relevant invariants.
class joins_tracker
{
    public:
        struct join {
            om_pos_dir where;
            const mutable_overmap_join *join;
        };
        using jl_iterator = std::list<join>::iterator;
        using jl_const_iterator = std::list<join>::const_iterator;

        bool any_unresolved() const {
            return !unresolved.empty();
        }

        std::vector<const join *> all_unresolved_at( const tripoint_om_omt &pos ) const {
            std::vector<const join *> result;
            for( jl_iterator it : unresolved.all_at( pos ) ) {
                result.push_back( &*it );
            }
            return result;
        }

        std::size_t count_unresolved_at( const tripoint_om_omt &pos ) const {
            return unresolved.count_at( pos );
        }

        bool any_postponed() const {
            return !postponed.empty();
        }

        bool any_postponed_at( const tripoint_om_omt &p ) const {
            return postponed.any_at( p );
        }

        void consistency_check() const;

        enum class join_status {
            disallowed, // Conflicts with existing join, and at least one was mandatory
            matched_available, // Matches an existing non-mandatory join
            matched_non_available, // Matches an existing mandatory join
            mismatched_available, // Points at an incompatible join, but both are non-mandatory
            free, // Doesn't point at another join at all
        };

        join_status allows( const om_pos_dir &this_side,
                            const mutable_overmap_terrain_join &this_ter_join ) const;

        void add_joins_for(
            const mutable_overmap_terrain &ter, const tripoint_om_omt &pos,
            om_direction::type rot, const std::vector<om_pos_dir> &suppressed_joins );

        tripoint_om_omt pick_top_priority() const;
        void postpone( const tripoint_om_omt &pos );
        void restore_postponed_at( const tripoint_om_omt &pos );
        void restore_postponed();

        const std::vector<std::pair<om_pos_dir, std::string>> &all_used() const {
            return used;
        }
    private:
        struct indexed_joins {
            std::list<join> joins;
            std::map<om_pos_dir, jl_iterator> position_index;

            jl_iterator begin() {
                return joins.begin();
            }

            jl_iterator end() {
                return joins.end();
            }

            jl_const_iterator begin() const {
                return joins.begin();
            }

            jl_const_iterator end() const {
                return joins.end();
            }

            bool empty() const {
                return joins.empty();
            }

            bool count( const om_pos_dir &p ) const {
                return position_index.count( p );
            }

            const join *find( const om_pos_dir &p ) const {
                auto it = position_index.find( p );
                if( it == position_index.end() ) {
                    return nullptr;
                }
                return &*it->second;
            }

            bool any_at( const tripoint_om_omt &pos ) const;

            std::vector<jl_iterator> all_at( const tripoint_om_omt &pos ) const;

            std::size_t count_at( const tripoint_om_omt &pos ) const;

            jl_iterator add( const om_pos_dir &p, const mutable_overmap_join *j ) {
                return add( { p, j } );
            }

            jl_iterator add( const join &j ) {
                joins.push_front( j );
                auto it = joins.begin();
                [[maybe_unused]] const bool inserted = position_index.emplace( j.where, it ).second;
                cata_assert( inserted );
                return it;
            }

            void erase( const jl_iterator it ) {
                [[maybe_unused]] const size_t erased = position_index.erase( it->where );
                cata_assert( erased );
                joins.erase( it );
            }

            void clear() {
                joins.clear();
                position_index.clear();
            }
        };

        void add_unresolved( const om_pos_dir &p, const mutable_overmap_join *j );
        bool erase_unresolved( const om_pos_dir &p );

        struct compare_iterators {
            bool operator()( jl_iterator l, jl_iterator r ) {
                return l->where < r->where;
            }
        };

        indexed_joins unresolved;
        std::vector<cata::flat_set<jl_iterator, compare_iterators>> unresolved_priority_index;

        indexed_joins resolved;
        indexed_joins postponed;

        std::vector<std::pair<om_pos_dir, std::string>> used;
};

struct mutable_overmap_phase_remainder {
    std::vector<mutable_overmap_placement_rule_remainder> rules;

    struct satisfy_result {
        tripoint_om_omt origin;
        om_direction::type dir;
        mutable_overmap_placement_rule_remainder *rule;
        std::vector<om_pos_dir> suppressed_joins;
        // For debugging purposes it's really handy to have a record of exactly
        // what happened during placement of a mutable special when it fails,
        // so to aid that we provide a human-readable description here which is
        // only used in the event of a placement error.
        std::string description;

        explicit satisfy_result( const tripoint_om_omt origin, const om_direction::type dir,
                                 mutable_overmap_placement_rule_remainder *rule,
                                 std::vector<om_pos_dir> suppressed_joins, std::string description ) :
            origin( origin ), dir( dir ), rule( rule ),
            suppressed_joins( std::move( suppressed_joins ) ), description( std::move( description ) ) {
        }
    };

    bool all_rules_exhausted() const {
        return std::all_of( rules.begin(), rules.end(),
        []( const mutable_overmap_placement_rule_remainder & rule ) {
            return rule.is_exhausted();
        } );
    }

    struct can_place_result {
        int num_context_mandatory_joins_matched;
        int num_my_non_available_matched;
        std::vector<om_pos_dir> supressed_joins;

        std::pair<int, int> as_pair() const {
            return { num_context_mandatory_joins_matched, num_my_non_available_matched };
        }

        friend bool operator==( const can_place_result &l, const can_place_result &r ) {
            return l.as_pair() == r.as_pair();
        }

        friend bool operator<( const can_place_result &l, const can_place_result &r ) {
            return l.as_pair() < r.as_pair();
        }
    };

    std::optional<can_place_result> can_place(
        const overmap &om, const mutable_overmap_placement_rule_remainder &rule,
        const tripoint_om_omt &origin, om_direction::type dir,
        const joins_tracker &unresolved
    ) const;

    satisfy_result satisfy( const overmap &om, const tripoint_om_omt &pos,
                            const joins_tracker &unresolved );
};

struct mutable_overmap_phase {
    std::vector<mutable_overmap_placement_rule> rules;

    mutable_overmap_phase_remainder realise() const {
        std::vector<mutable_overmap_placement_rule_remainder> realised_rules;
        realised_rules.reserve( rules.size() );
        for( const mutable_overmap_placement_rule &rule : rules ) {
            realised_rules.push_back( rule.realise() );
        }
        return { realised_rules };
    }

    void deserialize( const JsonValue &jin ) {
        jin.read( rules, true );
    }
};

struct mutable_overmap_special_data : overmap_special_data {
    overmap_special_id parent_id;
    std::vector<overmap_special_locations> check_for_locations;
    std::vector<overmap_special_locations> check_for_locations_area;
    std::vector<mutable_overmap_join> joins_vec;
    std::unordered_map<std::string, mutable_overmap_join *> joins;
    std::unordered_map<std::string, mutable_overmap_terrain> overmaps;
    std::string root;
    std::vector<mutable_overmap_phase> phases;

    explicit mutable_overmap_special_data( const overmap_special_id &p_id )
        : parent_id( p_id ) {
    }

    void finalize( const std::string &context,
                   const cata::flat_set<string_id<overmap_location>> &default_locations ) override;
    void finalize_mapgen_parameters(
        mapgen_parameters &params, const std::string &context ) const override;
    void check( const std::string &context ) const override;
    overmap_special_terrain root_as_overmap_special_terrain() const;
    std::vector<overmap_special_terrain> get_terrains() const override;
    std::vector<overmap_special_terrain> preview_terrains() const override;
    std::vector<overmap_special_locations> required_locations() const override;
    int score_rotation_at( const overmap &, const tripoint_om_omt &,
                           om_direction::type ) const override;
    special_placement_result place(
        overmap &om, const tripoint_om_omt &origin, om_direction::type dir, bool /*blob*/,
        const city &cit, bool must_be_unexplored ) const override;
    void load( const JsonObject &jo, bool was_loaded );
};

#endif // CATA_SRC_OMDATA_H
