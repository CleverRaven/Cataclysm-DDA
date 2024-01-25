#include "cube_direction.h" // IWYU pragma: associated
#include "omdata.h" // IWYU pragma: associated
#include "overmap.h" // IWYU pragma: associated

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <memory>
#include <numeric>
#include <optional>
#include <ostream>
#include <set>
#include <unordered_set>
#include <vector>

#include "all_enum_values.h"
#include "auto_note.h"
#include "avatar.h"
#include "assign.h"
#include "cached_options.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "cata_views.h"
#include "catacharset.h"
#include "character_id.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "distribution.h"
#include "effect_on_condition.h"
#include "flood_fill.h"
#include "game.h"
#include "generic_factory.h"
#include "json.h"
#include "line.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapgen.h"
#include "mapgen_functions.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmap_connection.h"
#include "overmap_location.h"
#include "overmap_noise.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "path_info.h"
#include "regional_settings.h"
#include "rng.h"
#include "rotatable_symbols.h"
#include "sets_intersect.h"
#include "simple_pathfinding.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"

static const mongroup_id GROUP_NEMESIS( "GROUP_NEMESIS" );
static const mongroup_id GROUP_OCEAN_DEEP( "GROUP_OCEAN_DEEP" );
static const mongroup_id GROUP_OCEAN_SHORE( "GROUP_OCEAN_SHORE" );
static const mongroup_id GROUP_RIVER( "GROUP_RIVER" );
static const mongroup_id GROUP_SUBWAY_CITY( "GROUP_SUBWAY_CITY" );
static const mongroup_id GROUP_SWAMP( "GROUP_SWAMP" );
static const mongroup_id GROUP_WORM( "GROUP_WORM" );
static const mongroup_id GROUP_ZOMBIE( "GROUP_ZOMBIE" );

static const oter_str_id oter_central_lab( "central_lab" );
static const oter_str_id oter_central_lab_core( "central_lab_core" );
static const oter_str_id oter_central_lab_train_depot( "central_lab_train_depot" );
static const oter_str_id oter_city_center( "city_center" );
static const oter_str_id oter_empty_rock( "empty_rock" );
static const oter_str_id oter_field( "field" );
static const oter_str_id oter_forest( "forest" );
static const oter_str_id oter_forest_thick( "forest_thick" );
static const oter_str_id oter_forest_water( "forest_water" );
static const oter_str_id oter_ice_lab( "ice_lab" );
static const oter_str_id oter_ice_lab_core( "ice_lab_core" );
static const oter_str_id oter_lab( "lab" );
static const oter_str_id oter_lab_core( "lab_core" );
static const oter_str_id oter_lab_escape_cells( "lab_escape_cells" );
static const oter_str_id oter_lab_escape_entrance( "lab_escape_entrance" );
static const oter_str_id oter_lab_train_depot( "lab_train_depot" );
static const oter_str_id oter_open_air( "open_air" );
static const oter_str_id oter_river_c_not_ne( "river_c_not_ne" );
static const oter_str_id oter_river_c_not_nw( "river_c_not_nw" );
static const oter_str_id oter_river_c_not_se( "river_c_not_se" );
static const oter_str_id oter_river_c_not_sw( "river_c_not_sw" );
static const oter_str_id oter_river_center( "river_center" );
static const oter_str_id oter_river_east( "river_east" );
static const oter_str_id oter_river_ne( "river_ne" );
static const oter_str_id oter_river_north( "river_north" );
static const oter_str_id oter_river_nw( "river_nw" );
static const oter_str_id oter_river_se( "river_se" );
static const oter_str_id oter_river_south( "river_south" );
static const oter_str_id oter_river_sw( "river_sw" );
static const oter_str_id oter_river_west( "river_west" );
static const oter_str_id oter_road_nesw( "road_nesw" );
static const oter_str_id oter_road_nesw_manhole( "road_nesw_manhole" );
static const oter_str_id oter_sewer_end_north( "sewer_end_north" );
static const oter_str_id oter_sewer_isolated( "sewer_isolated" );
static const oter_str_id oter_sewer_sub_station( "sewer_sub_station" );
static const oter_str_id oter_solid_earth( "solid_earth" );
static const oter_str_id oter_subway_end_north( "subway_end_north" );
static const oter_str_id oter_subway_isolated( "subway_isolated" );
static const oter_str_id oter_underground_sub_station( "underground_sub_station" );

static const oter_type_str_id oter_type_ants_queen( "ants_queen" );
static const oter_type_str_id oter_type_bridge( "bridge" );
static const oter_type_str_id oter_type_central_lab_core( "central_lab_core" );
static const oter_type_str_id oter_type_central_lab_stairs( "central_lab_stairs" );
static const oter_type_str_id oter_type_empty_rock( "empty_rock" );
static const oter_type_str_id oter_type_field( "field" );
static const oter_type_str_id oter_type_forest( "forest" );
static const oter_type_str_id oter_type_forest_thick( "forest_thick" );
static const oter_type_str_id oter_type_forest_water( "forest_water" );
static const oter_type_str_id oter_type_ice_lab_core( "ice_lab_core" );
static const oter_type_str_id oter_type_ice_lab_stairs( "ice_lab_stairs" );
static const oter_type_str_id oter_type_lab_core( "lab_core" );
static const oter_type_str_id oter_type_lab_stairs( "lab_stairs" );
static const oter_type_str_id oter_type_microlab_sub_connector( "microlab_sub_connector" );
static const oter_type_str_id oter_type_railroad_bridge( "railroad_bridge" );
static const oter_type_str_id oter_type_road( "road" );
static const oter_type_str_id oter_type_road_nesw_manhole( "road_nesw_manhole" );
static const oter_type_str_id oter_type_sewer_connector( "sewer_connector" );
static const oter_type_str_id oter_type_slimepit_bottom( "slimepit_bottom" );
static const oter_type_str_id oter_type_slimepit_down( "slimepit_down" );
static const oter_type_str_id oter_type_solid_earth( "solid_earth" );
static const oter_type_str_id oter_type_sub_station( "sub_station" );

static const overmap_connection_id overmap_connection_forest_trail( "forest_trail" );
static const overmap_connection_id overmap_connection_local_railroad( "local_railroad" );
static const overmap_connection_id overmap_connection_local_road( "local_road" );
static const overmap_connection_id overmap_connection_sewer_tunnel( "sewer_tunnel" );
static const overmap_connection_id overmap_connection_subway_tunnel( "subway_tunnel" );

static const overmap_location_id overmap_location_land( "land" );
static const overmap_location_id overmap_location_swamp( "swamp" );

static const species_id species_ZOMBIE( "ZOMBIE" );

class map_extra;

#define dbg(x) DebugLog((x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

static constexpr int BUILDINGCHANCE = 4;
static constexpr int MIN_GOO_SIZE = 1;
static constexpr int MAX_GOO_SIZE = 2;

using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

////////////////
static oter_id ot_null;

const oter_type_t oter_type_t::null_type{};

namespace io
{

template<>
std::string enum_to_string<overmap_special_subtype>( overmap_special_subtype data )
{
    switch( data ) {
        // *INDENT-OFF*
        case overmap_special_subtype::fixed: return "fixed";
        case overmap_special_subtype::mutable_: return "mutable";
        // *INDENT-ON*
        case overmap_special_subtype::last:
            break;
    }
    cata_fatal( "Invalid overmap_special_subtype" );
}

template<>
std::string enum_to_string<om_direction::type>( om_direction::type d )
{
    switch( d ) {
        // *INDENT-OFF*
        case om_direction::type::north: return "north";
        case om_direction::type::east: return "east";
        case om_direction::type::south: return "south";
        case om_direction::type::west: return "west";
        // *INDENT-ON*
        case om_direction::type::invalid:
        case om_direction::type::last:
            break;
    }
    cata_fatal( "Invalid om_direction" );
}

template<>
std::string enum_to_string<cube_direction>( cube_direction data )
{
    switch( data ) {
        // *INDENT-OFF*
        case cube_direction::north: return "north";
        case cube_direction::east: return "east";
        case cube_direction::south: return "south";
        case cube_direction::west: return "west";
        case cube_direction::above: return "above";
        case cube_direction::below: return "below";
        // *INDENT-ON*
        case cube_direction::last:
            break;
    }
    cata_fatal( "Invalid cube_direction" );
}

} // namespace io

namespace om_lines
{

struct type {
    uint32_t symbol;
    size_t mapgen;
    MULTITILE_TYPE subtile;
    int rotation;
    std::string suffix;
};

static const std::array<std::string, 5> mapgen_suffixes = {{
        "_straight", "_curved", "_end", "_tee", "_four_way"
    }
};

static const std::array < type, 1 + om_direction::bits > all = {{
        { UTF8_getch( LINE_XXXX_S ), 4, unconnected,  0, "_isolated"  }, // 0  ----
        { UTF8_getch( LINE_XOXO_S ), 2, end_piece,    2, "_end_south" }, // 1  ---n
        { UTF8_getch( LINE_OXOX_S ), 2, end_piece,    1, "_end_west"  }, // 2  --e-
        { UTF8_getch( LINE_XXOO_S ), 1, corner,       1, "_ne"        }, // 3  --en
        { UTF8_getch( LINE_XOXO_S ), 2, end_piece,    0, "_end_north" }, // 4  -s--
        { UTF8_getch( LINE_XOXO_S ), 0, edge,         0, "_ns"        }, // 5  -s-n
        { UTF8_getch( LINE_OXXO_S ), 1, corner,       0, "_es"        }, // 6  -se-
        { UTF8_getch( LINE_XXXO_S ), 3, t_connection, 1, "_nes"       }, // 7  -sen
        { UTF8_getch( LINE_OXOX_S ), 2, end_piece,    3, "_end_east"  }, // 8  w---
        { UTF8_getch( LINE_XOOX_S ), 1, corner,       2, "_wn"        }, // 9  w--n
        { UTF8_getch( LINE_OXOX_S ), 0, edge,         1, "_ew"        }, // 10 w-e-
        { UTF8_getch( LINE_XXOX_S ), 3, t_connection, 2, "_new"       }, // 11 w-en
        { UTF8_getch( LINE_OOXX_S ), 1, corner,       3, "_sw"        }, // 12 ws--
        { UTF8_getch( LINE_XOXX_S ), 3, t_connection, 3, "_nsw"       }, // 13 ws-n
        { UTF8_getch( LINE_OXXX_S ), 3, t_connection, 0, "_esw"       }, // 14 wse-
        { UTF8_getch( LINE_XXXX_S ), 4, center,       0, "_nesw"      }  // 15 wsen
    }
};

static const size_t size = all.size();
static const size_t invalid = 0;

static constexpr size_t rotate( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return line;
    }
    // Bitwise rotation to the left.
    return ( ( line << static_cast<size_t>( dir ) ) |
             ( line >> ( om_direction::size - static_cast<size_t>( dir ) ) ) ) & om_direction::bits;
}

static constexpr size_t set_segment( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return line;
    }
    return line | 1 << static_cast<int>( dir );
}

static constexpr bool has_segment( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return false;
    }
    return static_cast<bool>( line & 1 << static_cast<int>( dir ) );
}

static constexpr bool is_straight( size_t line )
{
    return line == 1
           || line == 2
           || line == 4
           || line == 5
           || line == 8
           || line == 10;
}

static size_t from_dir( om_direction::type dir )
{
    switch( dir ) {
        case om_direction::type::north:
        case om_direction::type::south:
            return 5;  // ns;
        case om_direction::type::east:
        case om_direction::type::west:
            return 10; // ew
        case om_direction::type::invalid:
        case om_direction::type::last:
            debugmsg( "Can't retrieve a line from the invalid direction." );
    }

    return 0;
}

} // namespace om_lines

//const regional_settings default_region_settings;
t_regional_settings_map region_settings_map;

namespace
{

generic_factory<overmap_land_use_code> land_use_codes( "overmap land use codes" );
generic_factory<oter_type_t> terrain_types( "overmap terrain type" );
generic_factory<oter_t> terrains( "overmap terrain" );
generic_factory<overmap_special> specials( "overmap special" );
generic_factory<overmap_special_migration> migrations( "overmap special migration" );

} // namespace

template<>
const overmap_land_use_code &overmap_land_use_code_id::obj() const
{
    return land_use_codes.obj( *this );
}

template<>
bool overmap_land_use_code_id::is_valid() const
{
    return land_use_codes.is_valid( *this );
}

template<>
const overmap_special &overmap_special_id::obj() const
{
    return specials.obj( *this );
}

template<>
bool overmap_special_id::is_valid() const
{
    return specials.is_valid( *this );
}

std::map<radio_type, std::string> radio_type_names =
{{ {radio_type::MESSAGE_BROADCAST, "broadcast"}, {radio_type::WEATHER_RADIO, "weather"} }};

/** @relates string_id */
template<>
bool string_id<oter_type_t>::is_valid() const
{
    return terrain_types.is_valid( *this );
}

/** @relates int_id */
template<>
const string_id<oter_type_t> &int_id<oter_type_t>::id() const
{
    return terrain_types.convert( *this );
}

/** @relates string_id */
template<>
int_id<oter_type_t> string_id<oter_type_t>::id() const
{
    return terrain_types.convert( *this, int_id<oter_type_t>( 0 ) );
}

/** @relates int_id */
template<>
int_id<oter_type_t>::int_id( const string_id<oter_type_t> &id ) : _id( id.id() ) {}

template<>
const oter_type_t &int_id<oter_type_t>::obj() const
{
    return terrain_types.obj( *this );
}

/** @relates string_id */
template<>
const oter_type_t &string_id<oter_type_t>::obj() const
{
    return terrain_types.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<oter_t>::is_valid() const
{
    return terrains.is_valid( *this );
}

/** @relates string_id */
template<>
const oter_t &string_id<oter_t>::obj() const
{
    return terrains.obj( *this );
}

/** @relates string_id */
template<>
int_id<oter_t> string_id<oter_t>::id() const
{
    return terrains.convert( *this, ot_null );
}

/** @relates int_id */
template<>
int_id<oter_t>::int_id( const string_id<oter_t> &id ) : _id( id.id() ) {}

/** @relates int_id */
template<>
bool int_id<oter_t>::is_valid() const
{
    return terrains.is_valid( *this );
}

/** @relates int_id */
template<>
const oter_t &int_id<oter_t>::obj() const
{
    return terrains.obj( *this );
}

/** @relates int_id */
template<>
const string_id<oter_t> &int_id<oter_t>::id() const
{
    return terrains.convert( *this );
}

static void set_oter_ids()   // FIXME: constify
{
    ot_null         = oter_str_id::NULL_ID();
}

std::string overmap_land_use_code::get_symbol() const
{
    return utf32_to_utf8( symbol );
}

void overmap_land_use_code::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";
    assign( jo, "land_use_code", land_use_code, strict );
    assign( jo, "name", name, strict );
    assign( jo, "detailed_definition", detailed_definition, strict );

    optional( jo, was_loaded, "sym", symbol, unicode_codepoint_from_symbol_reader, NULL_UNICODE );

    if( symbol == NULL_UNICODE ) {
        DebugLog( D_ERROR, D_GAME ) << "`sym` node is not defined properly for `land_use_code`: "
                                    << id.c_str() << " (" << name << ")";
    }

    assign( jo, "color", color );

}

void overmap_land_use_code::finalize()
{

}

void overmap_land_use_code::check() const
{

}

void overmap_land_use_codes::load( const JsonObject &jo, const std::string &src )
{
    land_use_codes.load( jo, src );
}

void overmap_land_use_codes::finalize()
{
    for( const overmap_land_use_code &elem : land_use_codes.get_all() ) {
        const_cast<overmap_land_use_code &>( elem ).finalize(); // This cast is ugly, but safe.
    }
}

void overmap_land_use_codes::check_consistency()
{
    land_use_codes.check();
}

void overmap_land_use_codes::reset()
{
    land_use_codes.reset();
}

const std::vector<overmap_land_use_code> &overmap_land_use_codes::get_all()
{
    return land_use_codes.get_all();
}

void overmap_specials::load( const JsonObject &jo, const std::string &src )
{
    specials.load( jo, src );
}

void city_buildings::load( const JsonObject &jo, const std::string &src )
{
    // Just an alias
    overmap_specials::load( jo, src );
}

void overmap_specials::finalize()
{
    for( const overmap_special &elem : specials.get_all() ) {
        const_cast<overmap_special &>( elem ).finalize(); // This cast is ugly, but safe.
    }
}

void overmap_specials::finalize_mapgen_parameters()
{
    for( const overmap_special &elem : specials.get_all() ) {
        // This cast is ugly, but safe.
        const_cast<overmap_special &>( elem ).finalize_mapgen_parameters();
    }
}

void overmap_specials::check_consistency()
{
    const size_t max_count = ( OMAPX / OMSPEC_FREQ ) * ( OMAPY / OMSPEC_FREQ );
    const size_t actual_count = std::accumulate( specials.get_all().begin(), specials.get_all().end(),
                                static_cast< size_t >( 0 ),
    []( size_t sum, const overmap_special & elem ) {
        size_t min_occur = static_cast<size_t>( std::max( elem.get_constraints().occurrences.min, 0 ) );
        const bool unique = elem.has_flag( "UNIQUE" ) || elem.has_flag( "GLOBALLY_UNIQUE" );
        return sum + ( unique ? 0 : min_occur );
    } );

    if( actual_count > max_count ) {
        debugmsg( "There are too many mandatory overmap specials (%d > %d). Some of them may not be placed.",
                  actual_count, max_count );
    }

    overmap_special_migration::check();
    specials.check();

    for( const overmap_special &os : specials.get_all() ) {
        overmap_special_id new_id = overmap_special_migration::migrate( os.id );
        if( new_id.is_null() ) {
            debugmsg( "Overmap special id %s has been removed or migrated to a different type.",
                      os.id.str() );
        } else if( new_id != os.id ) {
            debugmsg( "Overmap special id %s has been migrated.  Use %s instead.", os.id.str(),
                      new_id.str() );
        }
    }
}

void overmap_specials::reset()
{
    specials.reset();
}

const std::vector<overmap_special> &overmap_specials::get_all()
{
    return specials.get_all();
}

overmap_special_batch overmap_specials::get_default_batch( const point_abs_om &origin )
{
    std::vector<const overmap_special *> res;

    res.reserve( specials.size() );
    for( const overmap_special &elem : specials.get_all() ) {
        if( elem.can_spawn() ) {
            res.push_back( &elem );
        }
    }

    return overmap_special_batch( origin, res );
}

bool is_river( const oter_id &ter )
{
    return ter->is_river();
}

bool is_lake_or_river( const oter_id &ter )
{
    return ter->is_river() || ter->is_lake() || ter->is_lake_shore();
}

bool is_water_body( const oter_id &ter )
{
    return ter->is_river() || ter->is_lake() || ter->is_lake_shore() || ter->is_ocean() ||
           ter->is_ocean_shore();
}

bool is_ocean( const oter_id &ter )
{
    return ter->is_ocean() || ter->is_ocean_shore();
}

bool is_ot_match( const std::string &name, const oter_id &oter,
                  const ot_match_type match_type )
{
    static const auto is_ot = []( const std::string & otype, const oter_id & oter ) {
        return otype == oter.id().str();
    };

    static const auto is_ot_type = []( const std::string & otype, const oter_id & oter ) {
        // Is a match if the base type is the same which will allow for handling rotations/linear features
        // but won't incorrectly match other locations that happen to contain the substring.
        return otype == oter->get_type_id().str();
    };

    static const auto is_ot_subtype = []( const std::string & otype, const oter_id & oter ) {
        // Is a match if the base type and linear subtype (end/straight/curved/tee/four_way) are the same which will allow for handling rotations of linear features
        // but won't incorrectly match other locations that happen to contain the substring.
        return otype == oter->get_mapgen_id();
    };

    static const auto is_ot_prefix = []( const std::string & otype, const oter_id & oter ) {
        const size_t oter_size = oter.id().str().size();
        const size_t compare_size = otype.size();
        if( compare_size > oter_size ) {
            return false;
        }

        const auto &oter_str = oter.id();
        if( oter_str.str().compare( 0, compare_size, otype ) != 0 ) {
            return false;
        }

        // check if it's a full match
        if( compare_size == oter_size ) {
            return true;
        }

        // only okay for partial if next char is an underscore
        return oter_str.str()[compare_size] == '_';
    };

    static const auto is_ot_contains = []( const std::string & otype, const oter_id & oter ) {
        // Checks for any partial match.
        return strstr( oter.id().c_str(), otype.c_str() );
    };

    switch( match_type ) {
        case ot_match_type::exact:
            return is_ot( name, oter );
        case ot_match_type::type:
            return is_ot_type( name, oter );
        case ot_match_type::subtype:
            return is_ot_subtype( name, oter );
        case ot_match_type::prefix:
            return is_ot_prefix( name, oter );
        case ot_match_type::contains:
            return is_ot_contains( name, oter );
        default:
            return false;
    }
}

/*
 * load mapgen functions from an overmap_terrain json entry
 * suffix is for roads/subways/etc which have "_straight", "_curved", "_tee", "_four_way" function mappings
 */
static void load_overmap_terrain_mapgens( const JsonObject &jo, const std::string &id_base,
        const std::string &suffix = "" )
{
    const std::string fmapkey( id_base + suffix );
    const std::string jsonkey( "mapgen" + suffix );
    register_mapgen_function( fmapkey );
    if( jo.has_array( jsonkey ) ) {
        for( JsonObject jio : jo.get_array( jsonkey ) ) {
            // NOLINTNEXTLINE(cata-use-named-point-constants)
            load_and_add_mapgen_function( jio, fmapkey, point_zero, point( 1, 1 ) );
        }
    }
}

namespace io
{
template<>
std::string enum_to_string<oter_flags>( oter_flags data )
{
    switch( data ) {
        // *INDENT-OFF*
        case oter_flags::known_down: return "KNOWN_DOWN";
        case oter_flags::known_up: return "KNOWN_UP";
        case oter_flags::river_tile: return "RIVER";
        case oter_flags::bridge: return "BRIDGE";
        case oter_flags::has_sidewalk: return "SIDEWALK";
        case oter_flags::no_rotate: return "NO_ROTATE";
        case oter_flags::should_not_spawn: return "SHOULD_NOT_SPAWN";
        case oter_flags::ignore_rotation_for_adjacency: return "IGNORE_ROTATION_FOR_ADJACENCY";
        case oter_flags::line_drawing: return "LINEAR";
        case oter_flags::subway_connection: return "SUBWAY";
        case oter_flags::requires_predecessor: return "REQUIRES_PREDECESSOR";
        case oter_flags::water: return "WATER";
        case oter_flags::lake: return "LAKE";
        case oter_flags::lake_shore: return "LAKE_SHORE";
        case oter_flags::ocean: return "OCEAN";
        case oter_flags::ocean_shore: return "OCEAN_SHORE";
        case oter_flags::ravine: return "RAVINE";
        case oter_flags::ravine_edge: return "RAVINE_EDGE";
        case oter_flags::generic_loot: return "GENERIC_LOOT";
        case oter_flags::risk_high: return "RISK_HIGH";
        case oter_flags::risk_low: return "RISK_LOW";
        case oter_flags::source_ammo: return "SOURCE_AMMO";
        case oter_flags::source_animals: return "SOURCE_ANIMALS";
        case oter_flags::source_books: return "SOURCE_BOOKS";
        case oter_flags::source_chemistry: return "SOURCE_CHEMISTRY";
        case oter_flags::source_clothing: return "SOURCE_CLOTHING";
        case oter_flags::source_construction: return "SOURCE_CONSTRUCTION";
        case oter_flags::source_cooking: return "SOURCE_COOKING";
        case oter_flags::source_drink: return "SOURCE_DRINK";
        case oter_flags::source_electronics: return "SOURCE_ELECTRONICS";
        case oter_flags::source_fabrication: return "SOURCE_FABRICATION";
        case oter_flags::source_farming: return "SOURCE_FARMING";
        case oter_flags::source_food: return "SOURCE_FOOD";
        case oter_flags::source_forage: return "SOURCE_FORAGE";
        case oter_flags::source_fuel: return "SOURCE_FUEL";
        case oter_flags::source_gun: return "SOURCE_GUN";
        case oter_flags::source_luxury: return "SOURCE_LUXURY";
        case oter_flags::source_medicine: return "SOURCE_MEDICINE";
        case oter_flags::source_people: return "SOURCE_PEOPLE";
        case oter_flags::source_safety: return "SOURCE_SAFETY";
        case oter_flags::source_tailoring: return "SOURCE_TAILORING";
        case oter_flags::source_vehicles: return "SOURCE_VEHICLES";
        case oter_flags::source_weapon: return "SOURCE_WEAPON";
        // *INDENT-ON*
        case oter_flags::num_oter_flags:
            break;
    }
    cata_fatal( "Invalid oter_flags" );
}

} // namespace io

std::string oter_type_t::get_symbol() const
{
    return utf32_to_utf8( symbol );
}

namespace io
{
template<>
std::string enum_to_string<oter_travel_cost_type>( oter_travel_cost_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case oter_travel_cost_type::other: return "other";
        case oter_travel_cost_type::road: return "road";
        case oter_travel_cost_type::field: return "field";
        case oter_travel_cost_type::dirt_road: return "dirt_road";
        case oter_travel_cost_type::trail: return "trail";
        case oter_travel_cost_type::forest: return "forest";
        case oter_travel_cost_type::shore: return "shore";
        case oter_travel_cost_type::swamp: return "swamp";
        case oter_travel_cost_type::water: return "water";
        case oter_travel_cost_type::air: return "air";
        case oter_travel_cost_type::impassable: return "impassable";
        // *INDENT-ON*
        case oter_travel_cost_type::last:
            break;
    }
    cata_fatal( "Invalid oter_travel_cost_type" );
}
} // namespace io

void oter_type_t::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    optional( jo, was_loaded, "sym", symbol, unicode_codepoint_from_symbol_reader, NULL_UNICODE );

    assign( jo, "name", name, strict );
    assign( jo, "see_cost", see_cost, strict );
    assign( jo, "extras", extras, strict );
    assign( jo, "mondensity", mondensity, strict );
    assign( jo, "entry_eoc", entry_EOC, strict );
    assign( jo, "exit_eoc", exit_EOC, strict );
    assign( jo, "spawns", static_spawns, strict );
    assign( jo, "color", color );
    assign( jo, "land_use_code", land_use_code, strict );

    if( jo.has_member( "looks_like" ) ) {
        std::vector<std::string> ll;
        if( jo.has_array( "looks_like" ) ) {
            jo.read( "looks_like", ll );
        } else if( jo.has_string( "looks_like" ) ) {
            const std::string one_look = jo.get_string( "looks_like" );
            ll.push_back( one_look );
        }
        looks_like = ll;
    } else if( jo.has_member( "copy-from" ) ) {
        looks_like.insert( looks_like.begin(), jo.get_string( "copy-from" ) );
    }

    const auto flag_reader = typed_flag_reader<oter_flags>( "overmap terrain flag" );
    optional( jo, was_loaded, "flags", flags, flag_reader );

    optional( jo, was_loaded, "connect_group", connect_group, string_reader{} );
    optional( jo, was_loaded, "travel_cost_type", travel_cost_type, oter_travel_cost_type::other );

    if( has_flag( oter_flags::line_drawing ) ) {
        if( has_flag( oter_flags::no_rotate ) ) {
            jo.throw_error( R"(Mutually exclusive flags: "NO_ROTATE" and "LINEAR".)" );
        }

        for( const auto &elem : om_lines::mapgen_suffixes ) {
            load_overmap_terrain_mapgens( jo, id.str(), elem );
        }

        if( symbol == NULL_UNICODE ) {
            // Default the sym for linear terrains to a specific value which
            // has special behaviour when using fallback ASCII tiles so as to
            // cause it to draw using the box drawing characters (see
            // load_ascii_set).
            symbol = LINE_XOXO_C;
        }
    } else {
        if( symbol == NULL_UNICODE && !jo.has_string( "abstract" ) ) {
            DebugLog( D_ERROR, D_MAP_GEN ) << "sym is not defined for overmap_terrain: "
                                           << id.c_str() << " (" << name << ")";
        }
        if( !jo.has_string( "sym" ) && jo.has_number( "sym" ) ) {
            debugmsg( "sym is defined as number instead of string for overmap_terrain %s (%s)", id.c_str(),
                      name );
        }
        load_overmap_terrain_mapgens( jo, id.str() );
    }
}

void oter_type_t::check() const
{

}

void oter_type_t::finalize()
{
    directional_peers.clear();  // In case of a second finalization.

    if( is_rotatable() ) {
        for( om_direction::type dir : om_direction::all ) {
            register_terrain( oter_t( *this, dir ), static_cast<size_t>( dir ), om_direction::size );
        }
    } else if( has_flag( oter_flags::line_drawing ) ) {
        for( size_t i = 0; i < om_lines::size; ++i ) {
            register_terrain( oter_t( *this, i ), i, om_lines::size );
        }
    } else {
        register_terrain( oter_t( *this ), 0, 1 );
    }
}

void oter_type_t::register_terrain( const oter_t &peer, size_t n, size_t max_n )
{
    cata_assert( n < max_n );
    cata_assert( peer.type_is( *this ) );

    directional_peers.resize( max_n );

    if( peer.id.is_valid() ) {
        directional_peers[n] = peer.id.id();
        debugmsg( "Can't register the new overmap terrain \"%s\". It already exists.", peer.id.c_str() );
    } else {
        directional_peers[n] = terrains.insert( peer ).id.id();
    }
}

oter_id oter_type_t::get_first() const
{
    cata_assert( !directional_peers.empty() );
    return directional_peers.front();
}

oter_id oter_type_t::get_rotated( om_direction::type dir ) const
{
    if( dir == om_direction::type::invalid ) {
        debugmsg( "Invalid rotation was asked from overmap terrain \"%s\".", id.c_str() );
        return ot_null;
    } else if( dir == om_direction::type::none || !is_rotatable() ) {
        return directional_peers.front();
    }
    cata_assert( directional_peers.size() == om_direction::size );
    return directional_peers[static_cast<size_t>( dir )];
}

oter_id oter_type_t::get_linear( size_t n ) const
{
    if( !has_flag( oter_flags::line_drawing ) ) {
        debugmsg( "Overmap terrain \"%s \" isn't drawn with lines.", id.c_str() );
        return ot_null;
    }
    if( n >= om_lines::size ) {
        debugmsg( "Invalid overmap line (%d) was asked from overmap terrain \"%s\".", n, id.c_str() );
        return ot_null;
    }
    cata_assert( directional_peers.size() == om_lines::size );
    return directional_peers[n];
}

oter_t::oter_t() : oter_t( oter_type_t::null_type ) {}

oter_t::oter_t( const oter_type_t &type ) :
    type( &type ),
    id( type.id.str() ),
    symbol( type.symbol ),
    symbol_alt( type.land_use_code ? type.land_use_code->symbol : symbol ) {}

oter_t::oter_t( const oter_type_t &type, om_direction::type dir ) :
    type( &type ),
    id( type.id.str() + "_" + io::enum_to_string( dir ) ),
    dir( dir ),
    symbol( om_direction::rotate_symbol( type.symbol, dir ) ),
    symbol_alt( om_direction::rotate_symbol( type.land_use_code ? type.land_use_code->symbol :
                type.symbol, dir ) ),
    line( om_lines::from_dir( dir ) ) {}

oter_t::oter_t( const oter_type_t &type, size_t line ) :
    type( &type ),
    id( type.id.str() + om_lines::all[line].suffix ),
    symbol( om_lines::all[line].symbol ),
    symbol_alt( om_lines::all[line].symbol ),
    line( line ) {}

std::string oter_t::get_mapgen_id() const
{
    return type->has_flag( oter_flags::line_drawing )
           ? type->id.str() + om_lines::mapgen_suffixes[om_lines::all[line].mapgen]
           : type->id.str();
}

oter_id oter_t::get_rotated( om_direction::type dir ) const
{
    return type->has_flag( oter_flags::line_drawing )
           ? type->get_linear( om_lines::rotate( this->line, dir ) )
           : type->get_rotated( om_direction::add( this->dir, dir ) );
}

void oter_t::get_rotation_and_subtile( int &rotation, int &subtile ) const
{
    if( is_linear() ) {
        const om_lines::type &t = om_lines::all[line];
        rotation = t.rotation;
        subtile = t.subtile;
    } else if( is_rotatable() ) {
        rotation = ( 4 - static_cast<int>( get_dir() ) ) % 4;
        subtile = -1;
    } else {
        rotation = 0;
        subtile = -1;
    }
}

int oter_t::get_rotation() const
{
    if( is_linear() ) {
        const om_lines::type &t = om_lines::all[line];
        // It turns out the rotation used for linear things is the opposite of
        // the rotation used for other things.  Sigh.
        return ( 4 - t.rotation ) % 4;
    }
    if( is_rotatable() ) {
        return static_cast<int>( get_dir() );
    }
    return 0;
}

bool oter_t::type_is( const int_id<oter_type_t> &type_id ) const
{
    return type->id.id() == type_id;
}

bool oter_t::type_is( const oter_type_t &type ) const
{
    return this->type == &type;
}

bool oter_t::has_connection( om_direction::type dir ) const
{
    // TODO: It's a DAMN UGLY hack. Remove it as soon as possible.
    if( id == oter_road_nesw_manhole || id == oter_city_center ) {
        return true;
    }
    return om_lines::has_segment( line, dir );
}

bool oter_t::is_hardcoded() const
{
    // TODO: This set only exists because so does the monstrous 'if-else' statement in @ref map::draw_map(). Get rid of both.
    static const std::set<std::string> hardcoded_mapgen = {
        "ants_lab",
        "ants_lab_stairs",
        "ice_lab",
        "ice_lab_stairs",
        "ice_lab_core",
        "ice_lab_finale",
        "central_lab",
        "central_lab_stairs",
        "central_lab_core",
        "central_lab_finale",
        "tower_lab",
        "tower_lab_stairs",
        "tower_lab_finale",
        "lab",
        "lab_core",
        "lab_stairs",
        "lab_finale",
        "slimepit",
        "slimepit_down"
    };

    return hardcoded_mapgen.find( get_mapgen_id() ) != hardcoded_mapgen.end();
}

void overmap_terrains::load( const JsonObject &jo, const std::string &src )
{
    terrain_types.load( jo, src );
}

void overmap_terrains::check_consistency()
{
    for( const oter_type_t &elem : terrain_types.get_all() ) {
        elem.check();
        if( elem.static_spawns.group && !elem.static_spawns.group.is_valid() ) {
            debugmsg( "Invalid monster group \"%s\" in spawns of \"%s\".", elem.static_spawns.group.c_str(),
                      elem.id.c_str() );
        }
    }

    for( const oter_t &elem : terrains.get_all() ) {
        const std::string mid = elem.get_mapgen_id();

        if( mid.empty() ) {
            continue;
        }

        const bool exists_hardcoded = elem.is_hardcoded();

        if( has_mapgen_for( mid ) ) {
            if( test_mode && exists_hardcoded ) {
                debugmsg( "Mapgen terrain \"%s\" exists in both JSON and a hardcoded function.  Consider removing the latter.",
                          mid.c_str() );
            }
            check_mapgen_consistent_with( mid, elem );
        } else if( !exists_hardcoded ) {
            debugmsg( "No mapgen terrain exists for \"%s\".", mid.c_str() );
        }
    }
}

void overmap_terrains::finalize()
{
    terrain_types.finalize();

    for( const oter_type_t &elem : terrain_types.get_all() ) {
        const_cast<oter_type_t &>( elem ).finalize(); // This cast is ugly, but safe.
    }

    if( region_settings_map.find( "default" ) == region_settings_map.end() ) {
        debugmsg( "ERROR: can't find default overmap settings (region_map_settings 'default'), "
                  "Cataclysm pending.  And not the fun kind." );
    }

    for( auto &elem : region_settings_map ) {
        elem.second.finalize();
    }

    set_oter_ids();
}

void overmap_terrains::reset()
{
    terrain_types.reset();
    terrains.reset();
}

const std::vector<oter_t> &overmap_terrains::get_all()
{
    return terrains.get_all();
}

static bool is_amongst_locations( const oter_id &oter,
                                  const cata::flat_set<string_id<overmap_location>> &locations )
{
    return std::any_of( locations.begin(), locations.end(),
    [&oter]( const string_id<overmap_location> &loc ) {
        return loc->test( oter );
    } );
}

bool overmap_special_locations::can_be_placed_on( const oter_id &oter ) const
{
    return is_amongst_locations( oter, locations );
}

void overmap_special_locations::deserialize( const JsonArray &ja )
{
    if( ja.size() != 2 ) {
        ja.throw_error( "expected array of size 2" );
    }

    ja.read( 0, p, true );
    ja.read( 1, locations, true );
}

void overmap_special_terrain::deserialize( const JsonObject &om )
{
    om.read( "point", p );
    om.read( "overmap", terrain );
    om.read( "flags", flags );
    om.read( "locations", locations );
}

overmap_special_terrain::overmap_special_terrain(
    const tripoint &p, const oter_str_id &t, const cata::flat_set<string_id<overmap_location>> &l,
    const std::set<std::string> &fs )
    : overmap_special_locations{ p, l }
    , terrain( t )
    , flags( fs )
{}

cube_direction operator+( const cube_direction l, const om_direction::type r )
{
    return l + static_cast<int>( r );
}

cube_direction operator+( const cube_direction d, int i )
{
    switch( d ) {
        case cube_direction::north:
        case cube_direction::east:
        case cube_direction::south:
        case cube_direction::west:
            return static_cast<cube_direction>( ( static_cast<int>( d ) + i ) % 4 );
        case cube_direction::above:
        case cube_direction::below:
            return d;
        case cube_direction::last:
            break;
    }
    constexpr_fatal( cube_direction::last, "Invalid cube_direction" );
}

cube_direction operator-( const cube_direction l, const om_direction::type r )
{
    return l - static_cast<int>( r );
}

cube_direction operator-( const cube_direction d, int i )
{
    switch( d ) {
        case cube_direction::north:
        case cube_direction::east:
        case cube_direction::south:
        case cube_direction::west:
            return static_cast<cube_direction>( ( static_cast<int>( d ) - i + 4 ) % 4 );
        case cube_direction::above:
        case cube_direction::below:
            return d;
        case cube_direction::last:
            break;
    }
    constexpr_fatal( cube_direction::last, "Invalid cube_direction" );
}

tripoint displace( cube_direction d )
{
    switch( d ) {
        case cube_direction::north:
            return tripoint_north;
        case cube_direction::east:
            return tripoint_east;
        case cube_direction::south:
            return tripoint_south;
        case cube_direction::west:
            return tripoint_west;
        case cube_direction::above:
            return tripoint_above;
        case cube_direction::below:
            return tripoint_below;
        case cube_direction::last:
            break;
    }
    cata_fatal( "Invalid cube_direction" );
}

struct special_placement_result {
    std::vector<tripoint_om_omt> omts_used;
    std::vector<std::pair<om_pos_dir, std::string>> joins_used;
};

struct overmap_special_data {
    virtual ~overmap_special_data() = default;
    virtual void finalize(
        const std::string &context,
        const cata::flat_set<string_id<overmap_location>> &default_locations ) = 0;
    virtual void finalize_mapgen_parameters(
        mapgen_parameters &, const std::string &context ) const = 0;
    virtual void check( const std::string &context ) const = 0;
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
        : terrains{ ter }
    {}

    void finalize(
        const std::string &/*context*/,
        const cata::flat_set<string_id<overmap_location>> &default_locations ) override {
        // If the special has default locations, then add those to the locations
        // of each of the terrains IF the terrain has no locations already.
        for( overmap_special_terrain &t : terrains ) {
            if( t.locations.empty() ) {
                t.locations = default_locations;
            }
        }

        for( overmap_special_connection &elem : connections ) {
            const overmap_special_terrain &oter = get_terrain_at( elem.p );
            if( !elem.terrain && oter.terrain ) {
                elem.terrain = oter.terrain->get_type_id();    // Defaulted.
            }

            // If the connection type hasn't been specified, we'll guess for them.
            // The guess isn't always right (hence guessing) in the case where
            // multiple connections types can be made on a single location type,
            // e.g. both roads and forest trails can be placed on "forest" locations.
            if( elem.connection.is_null() ) {
                elem.connection = overmap_connections::guess_for( elem.terrain );
            }

            // If the connection has a "from" hint specified, then figure out what the
            // resulting direction from the hinted location to the connection point is,
            // and use that as the initial direction to be passed off to the connection
            // building code.
            if( elem.from ) {
                const direction calculated_direction = direction_from( *elem.from, elem.p );
                switch( calculated_direction ) {
                    case direction::NORTH:
                        elem.initial_dir = cube_direction::north;
                        break;
                    case direction::EAST:
                        elem.initial_dir = cube_direction::east;
                        break;
                    case direction::SOUTH:
                        elem.initial_dir = cube_direction::south;
                        break;
                    case direction::WEST:
                        elem.initial_dir = cube_direction::west;
                        break;
                    default:
                        // The only supported directions are north/east/south/west
                        // as those are the four directions that overmap connections
                        // can be made in. If the direction we figured out wasn't
                        // one of those, just set this as invalid. We'll provide
                        // a warning to the user/developer in overmap_special::check().
                        elem.initial_dir = cube_direction::last;
                        break;
                }
            }
        }
    }

    void finalize_mapgen_parameters( mapgen_parameters &params,
                                     const std::string &context ) const override {
        for( const overmap_special_terrain &t : terrains ) {
            if( !t.terrain.is_valid() ) {
                if( oter_str_id( t.terrain.str() + "_north" ).is_valid() ) {
                    debugmsg( "In %s, terrain \"%s\" rotates, but is specified without a "
                              "rotation.", context, t.terrain.str() );
                } else {
                    debugmsg( "In %s, terrain \"%s\" is invalid.", context, t.terrain.str() );
                }
            }
            std::string mapgen_id = t.terrain->get_mapgen_id();
            params.check_and_merge( get_map_special_params( mapgen_id ), context );
        }
    }

    void check( const std::string &context ) const override {
        std::set<oter_str_id> invalid_terrains;
        std::set<tripoint> points;

        for( const overmap_special_terrain &elem : terrains ) {
            const oter_str_id &oter = elem.terrain;

            if( !oter.is_valid() ) {
                if( !invalid_terrains.count( oter ) ) {
                    // Not a huge fan of the the direct id manipulation here, but I don't know
                    // how else to do this
                    // Because we try to access all the terrains in the finalization,
                    // this is a little redundant, but whatever
                    oter_str_id invalid( oter.str() + "_north" );
                    if( invalid.is_valid() ) {
                        debugmsg( "In %s, terrain \"%s\" rotates, but is specified without a "
                                  "rotation.", context, oter.str() );
                    } else  {
                        debugmsg( "In %s, terrain \"%s\" is invalid.", context, oter.str() );
                    }
                    invalid_terrains.insert( oter );
                }
            }

            const tripoint &pos = elem.p;

            if( points.count( pos ) > 0 ) {
                debugmsg( "In %s, point %s is duplicated.", context, pos.to_string() );
            } else {
                points.insert( pos );
            }

            if( elem.locations.empty() ) {
                debugmsg( "In %s, no location is defined for point %s or the "
                          "overall special.", context, pos.to_string() );
            }

            for( const auto &l : elem.locations ) {
                if( !l.is_valid() ) {
                    debugmsg( "In %s, point %s, location \"%s\" is invalid.",
                              context, pos.to_string(), l.c_str() );
                }
            }
        }

        for( const overmap_special_connection &elem : connections ) {
            const overmap_special_terrain &oter = get_terrain_at( elem.p );
            if( !elem.terrain ) {
                debugmsg( "In %s, connection %s doesn't have a terrain.",
                          context, elem.p.to_string() );
            } else if( !elem.existing && !elem.terrain->has_flag( oter_flags::line_drawing ) ) {
                debugmsg( "In %s, connection %s \"%s\" isn't drawn with lines.",
                          context, elem.p.to_string(), elem.terrain.c_str() );
            } else if( !elem.existing && oter.terrain && !oter.terrain->type_is( elem.terrain ) ) {
                debugmsg( "In %s, connection %s overwrites \"%s\".",
                          context, elem.p.to_string(), oter.terrain.c_str() );
            }

            if( elem.from ) {
                // The only supported directions are north/east/south/west
                // as those are the four directions that overmap connections
                // can be made in. If the direction we figured out wasn't
                // one of those, warn the user/developer.
                const direction calculated_direction = direction_from( *elem.from, elem.p );
                switch( calculated_direction ) {
                    case direction::NORTH:
                    case direction::EAST:
                    case direction::SOUTH:
                    case direction::WEST:
                        continue;
                    default:
                        debugmsg( "In %s, connection %s is not directly north, "
                                  "east, south or west of the defined \"from\" %s.",
                                  context, elem.p.to_string(), elem.from->to_string() );
                        break;
                }
            }
        }
    }

    const overmap_special_terrain &get_terrain_at( const tripoint &p ) const {
        const auto iter = std::find_if( terrains.begin(), terrains.end(),
        [ &p ]( const overmap_special_terrain & elem ) {
            return elem.p == p;
        } );
        if( iter == terrains.end() ) {
            static const overmap_special_terrain null_terrain{};
            return null_terrain;
        }
        return *iter;
    }

    std::vector<overmap_special_terrain> preview_terrains() const override {
        std::vector<overmap_special_terrain> result;
        std::copy_if( terrains.begin(), terrains.end(), std::back_inserter( result ),
        []( const overmap_special_terrain & terrain ) {
            return terrain.p.z == 0;
        } );
        return result;
    }

    std::vector<overmap_special_locations> required_locations() const override {
        std::vector<overmap_special_locations> result;
        result.reserve( terrains.size() );
        std::copy( terrains.begin(), terrains.end(), std::back_inserter( result ) );
        return result;
    }

    int score_rotation_at( const overmap &om, const tripoint_om_omt &p,
                           om_direction::type r ) const override {
        int score = 0;
        for( const overmap_special_connection &con : connections ) {
            const tripoint_om_omt rp = p + om_direction::rotate( con.p, r );
            if( !overmap::inbounds( rp ) ) {
                return -1;
            }
            const oter_id &oter = om.ter( rp );

            if( ( oter->get_type_id() == oter_type_str_id( con.terrain.str() ) ) ) {
                ++score; // Found another one satisfied connection.
            } else if( !oter || con.existing || !con.connection->pick_subtype_for( oter ) ) {
                return -1;
            }
        }
        return score;
    }

    special_placement_result place(
        overmap &om, const tripoint_om_omt &origin, om_direction::type dir, bool blob,
        const city &cit, bool must_be_unexplored ) const override {
        special_placement_result result;

        for( const overmap_special_terrain &elem : terrains ) {
            const tripoint_om_omt location = origin + om_direction::rotate( elem.p, dir );
            if( !( elem.terrain == oter_str_id::NULL_ID() ) ) {
                result.omts_used.push_back( location );
                const oter_id tid = elem.terrain->get_rotated( dir );
                om.ter_set( location, tid );
                if( blob ) {
                    for( int x = -2; x <= 2; x++ ) {
                        for( int y = -2; y <= 2; y++ ) {
                            const tripoint_om_omt nearby_pos = location + point( x, y );
                            if( !overmap::inbounds( nearby_pos ) ) {
                                continue;
                            }
                            if( one_in( 1 + std::abs( x ) + std::abs( y ) ) &&
                                elem.can_be_placed_on( om.ter( nearby_pos ) ) ) {
                                om.ter_set( nearby_pos, tid );
                            }
                        }
                    }
                }
            }
        }
        // Make connections.
        for( const overmap_special_connection &elem : connections ) {
            if( elem.connection ) {
                const tripoint_om_omt rp = origin + om_direction::rotate( elem.p, dir );
                cube_direction initial_dir = elem.initial_dir;

                if( initial_dir != cube_direction::last ) {
                    initial_dir = initial_dir + dir;
                }
                if( cit ) {
                    om.build_connection( cit.pos, rp.xy(), elem.p.z, *elem.connection,
                                         must_be_unexplored, initial_dir );
                }
                // if no city present, search for nearby road within 50 tiles and make
                // connection to it instead
                else {
                    for( const tripoint_om_omt &nearby_point : closest_points_first( rp, 50 ) ) {
                        if( om.check_ot( "road", ot_match_type::contains, nearby_point ) ) {
                            om.build_connection(
                                nearby_point.xy(), rp.xy(), elem.p.z, *elem.connection,
                                must_be_unexplored, initial_dir );
                        }
                    }
                }
            }
        }

        return result;
    }

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

enum class join_type {
    mandatory,
    available,
    last
};

template<>
struct enum_traits<join_type> {
    static constexpr join_type last = join_type::last;
};

namespace io
{

template<>
std::string enum_to_string<join_type>( join_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case join_type::mandatory: return "mandatory";
        case join_type::available: return "available";
        // *INDENT-ON*
        case join_type::last:
            break;
    }
    cata_fatal( "Invalid join_type" );
}

} // namespace io

struct mutable_overmap_terrain_join {
    std::string join_id;
    const mutable_overmap_join *join = nullptr; // NOLINT(cata-serialize)
    cata::flat_set<std::string> alternative_join_ids;
    cata::flat_set<const mutable_overmap_join *> alternative_joins; // NOLINT(cata-serialize)
    join_type type = join_type::mandatory;

    void finalize( const std::string &context,
                   const std::unordered_map<std::string, mutable_overmap_join *> &joins ) {
        auto join_it = joins.find( join_id );
        if( join_it != joins.end() ) {
            join = join_it->second;
        } else {
            debugmsg( "invalid join id %s in %s", join_id, context );
        }
        for( const std::string &alt_join_id : alternative_join_ids ) {
            auto alt_join_it = joins.find( alt_join_id );
            if( alt_join_it != joins.end() ) {
                alternative_joins.insert( alt_join_it->second );
            } else {
                debugmsg( "invalid join id %s in %s", alt_join_id, context );
            }
        }
    }

    void deserialize( const JsonValue &jin ) {
        if( jin.test_string() ) {
            jin.read( join_id, true );
        } else if( jin.test_object() ) {
            JsonObject jo = jin.get_object();
            jo.read( "id", join_id, true );
            jo.read( "type", type, true );
            jo.read( "alternatives", alternative_join_ids, true );
        } else {
            jin.throw_error( "Expected string or object" );
        }
    }
};

using join_map = std::unordered_map<cube_direction, mutable_overmap_terrain_join>;

struct mutable_special_connection {
    string_id<overmap_connection> connection;

    void deserialize( const JsonObject &jo ) {
        jo.read( "connection", connection );
    }

    void check( const std::string &context ) const {
        if( !connection.is_valid() ) {
            debugmsg( "invalid connection id %s in %s", connection.str(), context );
        }
    }
};

struct mutable_overmap_terrain {
    oter_str_id terrain;
    cata::flat_set<string_id<overmap_location>> locations;
    join_map joins;
    std::map<cube_direction, mutable_special_connection> connections;

    void finalize( const std::string &context,
                   const std::unordered_map<std::string, mutable_overmap_join *> &special_joins,
                   const cata::flat_set<string_id<overmap_location>> &default_locations ) {
        if( locations.empty() ) {
            locations = default_locations;
        }
        for( join_map::value_type &p : joins ) {
            mutable_overmap_terrain_join &ter_join = p.second;
            ter_join.finalize( context, special_joins );
        }
    }

    void check( const std::string &context ) const {
        if( !terrain.is_valid() ) {
            debugmsg( "invalid overmap terrain id %s in %s", terrain.str(), context );
        }

        if( locations.empty() ) {
            debugmsg( "In %s, no locations are defined", context );
        }

        for( const string_id<overmap_location> &loc : locations ) {
            if( !loc.is_valid() ) {
                debugmsg( "invalid overmap location id %s in %s", loc.str(), context );
            }
        }

        for( const std::pair<const cube_direction, mutable_special_connection> &p :
             connections ) {
            p.second.check( string_format( "connection %s in %s", io::enum_to_string( p.first ),
                                           context ) );
        }
    }

    void deserialize( const JsonObject &jo ) {
        jo.read( "overmap", terrain, true );
        jo.read( "locations", locations );
        for( int i = 0; i != static_cast<int>( cube_direction::last ); ++i ) {
            cube_direction dir = static_cast<cube_direction>( i );
            std::string dir_s = io::enum_to_string( dir );
            if( jo.has_member( dir_s ) ) {
                jo.read( dir_s, joins[dir], true );
            }
        }
        jo.read( "connections", connections );
    }
};

struct mutable_overmap_piece_candidate {
    const mutable_overmap_terrain *overmap; // NOLINT(cata-serialize)
    tripoint_om_omt pos;
    om_direction::type rot = om_direction::type::north;
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

struct mutable_overmap_placement_rule_remainder;

struct mutable_overmap_placement_rule {
    std::string name;
    std::vector<mutable_overmap_placement_rule_piece> pieces;
    // NOLINTNEXTLINE(cata-serialize)
    std::vector<std::pair<rel_pos_dir, const mutable_overmap_terrain_join *>> outward_joins;
    int_distribution max = int_distribution( INT_MAX );
    int weight = INT_MAX;

    std::string description() const {
        if( !name.empty() ) {
            return name;
        }
        std::string first_om_id = pieces[0].overmap_id;
        if( pieces.size() == 1 ) {
            return first_om_id;
        } else {
            return "chunk using overmap " + first_om_id;
        }
    }

    void finalize( const std::string &context,
                   const std::unordered_map<std::string, mutable_overmap_terrain> &special_overmaps
                 ) {
        std::unordered_map<tripoint_rel_omt, const mutable_overmap_placement_rule_piece *>
        pieces_by_pos;
        for( mutable_overmap_placement_rule_piece &piece : pieces ) {
            bool inserted = pieces_by_pos.emplace( piece.pos, &piece ).second;
            if( !inserted ) {
                debugmsg( "phase of %s has chunk with duplicated position %s",
                          context, piece.pos.to_string() );
            }
            auto it = special_overmaps.find( piece.overmap_id );
            if( it == special_overmaps.end() ) {
                cata_fatal( "phase of %s specifies overmap %s which is not defined for that "
                            "special", context, piece.overmap_id );
            } else {
                piece.overmap = &it->second;
            }
        }
        for( const mutable_overmap_placement_rule_piece &piece : pieces ) {
            const mutable_overmap_terrain &ter = *piece.overmap;
            for( const join_map::value_type &p : ter.joins ) {
                const cube_direction dir = p.first;
                const mutable_overmap_terrain_join &ter_join = p.second;
                rel_pos_dir this_side{ piece.pos, dir + piece.rot };
                rel_pos_dir other_side = this_side.opposite();
                auto opposite_piece = pieces_by_pos.find( other_side.p );
                if( opposite_piece == pieces_by_pos.end() ) {
                    outward_joins.emplace_back( this_side, &ter_join );
                } else {
                    const std::string &opposite_join = ter_join.join->opposite_id;
                    const mutable_overmap_placement_rule_piece &other_piece =
                        *opposite_piece->second;
                    const mutable_overmap_terrain &other_om = *other_piece.overmap;

                    auto opposite_om_join =
                        other_om.joins.find( other_side.dir - other_piece.rot );
                    if( opposite_om_join == other_om.joins.end() ) {
                        debugmsg( "in phase of %s, %s has adjacent pieces %s at %s and %s at "
                                  "%s where the former has a join %s pointed towards the latter, "
                                  "but the latter has no join pointed towards the former",
                                  context, description(), piece.overmap_id, piece.pos.to_string(),
                                  other_piece.overmap_id, other_piece.pos.to_string(),
                                  ter_join.join_id );
                    } else if( opposite_om_join->second.join_id != opposite_join ) {
                        debugmsg( "in phase of %s, %s has adjacent pieces %s at %s and %s at "
                                  "%s where the former has a join %s pointed towards the latter, "
                                  "expecting a matching join %s whereas the latter has the join %s "
                                  "pointed towards the former",
                                  context, description(), piece.overmap_id, piece.pos.to_string(),
                                  other_piece.overmap_id, other_piece.pos.to_string(),
                                  ter_join.join_id, opposite_join,
                                  opposite_om_join->second.join_id );
                    }
                }
            }
        }
    }
    void check( const std::string &context ) const {
        if( pieces.empty() ) {
            cata_fatal( "phase of %s has chunk with zero pieces" );
        }
        int min_max = max.minimum();
        if( min_max < 0 ) {
            debugmsg( "phase of %s specifies max which might be as low as %d; this should "
                      "be a positive number", context, min_max );
        }
    }

    mutable_overmap_placement_rule_remainder realise() const;

    void deserialize( const JsonObject &jo ) {
        jo.read( "name", name );
        if( jo.has_member( "overmap" ) ) {
            pieces.emplace_back();
            jo.read( "overmap", pieces.back().overmap_id, true );
        } else if( jo.has_member( "chunk" ) ) {
            jo.read( "chunk", pieces );
        } else {
            jo.throw_error( R"(placement rule must specify at least one of "overmap" or "chunk")" );
        }
        jo.read( "max", max );
        jo.read( "weight", weight );
        if( !jo.has_member( "max" ) && weight == INT_MAX ) {
            jo.throw_error( R"(placement rule must specify at least one of "max" or "weight")" );
        }
    }
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
    auto pieces( const tripoint_om_omt &origin, om_direction::type rot ) const {
        using orig_t = mutable_overmap_placement_rule_piece;
        using dest_t = mutable_overmap_piece_candidate;
        return cata::views::transform < decltype( parent->pieces ), dest_t > ( parent->pieces,
        [origin, rot]( const orig_t &piece ) -> dest_t {
            tripoint_rel_omt rotated_offset = rotate( piece.pos, rot );
            return { piece.overmap, origin + rotated_offset, add( rot, piece.rot ) };
        } );
    }
    auto outward_joins( const tripoint_om_omt &origin, om_direction::type rot ) const {
        using orig_t = std::pair<rel_pos_dir, const mutable_overmap_terrain_join *>;
        using dest_t = std::pair<om_pos_dir, const mutable_overmap_terrain_join *>;
        return cata::views::transform < decltype( parent->outward_joins ), dest_t > ( parent->outward_joins,
        [origin, rot]( const orig_t &p ) -> dest_t {
            tripoint_rel_omt rotated_offset = rotate( p.first.p, rot );
            om_pos_dir p_d{ origin + rotated_offset, p.first.dir + rot };
            return { p_d, p.second };
        } );
    }
};

mutable_overmap_placement_rule_remainder mutable_overmap_placement_rule::realise() const
{
    return mutable_overmap_placement_rule_remainder{ this, max.sample(), weight };
}

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
        using iterator = std::list<join>::iterator;
        using const_iterator = std::list<join>::const_iterator;

        bool any_unresolved() const {
            return !unresolved.empty();
        }

        std::vector<const join *> all_unresolved_at( const tripoint_om_omt &pos ) const {
            std::vector<const join *> result;
            for( iterator it : unresolved.all_at( pos ) ) {
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

        void consistency_check() const {
#if 0 // Enable this to check the class invariants, at the cost of more runtime
            // verify that there are no positions in common between the
            // resolved and postponed lists
            for( const join &j : postponed ) {
                auto j_pos = j.where.p;
                if( unresolved.any_at( j_pos ) ) {
                    std::vector<iterator> unr = unresolved.all_at( j_pos );
                    if( unr.empty() ) {
                        cata_fatal( "inconsistency between all_at and any_at" );
                    } else {
                        const join &unr_j = *unr.front();
                        cata_fatal( "postponed and unresolved should be disjoint but are not at "
                                    "%s where unresolved has %s: %s",
                                    j_pos.to_string(), unr_j.where.p.to_string(), unr_j.join_id );
                    }
                }
            }
#endif
        }

        enum class join_status {
            disallowed, // Conflicts with existing join, and at least one was mandatory
            matched_available, // Matches an existing non-mandatory join
            matched_non_available, // Matches an existing mandatory join
            mismatched_available, // Points at an incompatible join, but both are non-mandatory
            free, // Doesn't point at another join at all
        };

        join_status allows( const om_pos_dir &this_side,
                            const mutable_overmap_terrain_join &this_ter_join ) const {
            om_pos_dir other_side = this_side.opposite();

            auto is_allowed_opposite = [&]( const std::string & candidate ) {
                const mutable_overmap_join &this_join = *this_ter_join.join;

                if( this_join.opposite_id == candidate ) {
                    return true;
                }

                for( const mutable_overmap_join *alt_join : this_ter_join.alternative_joins ) {
                    if( alt_join->opposite_id == candidate ) {
                        return true;
                    }
                }

                return false;
            };

            if( const join *existing = resolved.find( other_side ) ) {
                bool other_side_mandatory = unresolved.count( this_side );
                if( is_allowed_opposite( existing->join->id ) ) {
                    return other_side_mandatory
                           ? join_status::matched_non_available : join_status::matched_available;
                } else {
                    if( other_side_mandatory || this_ter_join.type != join_type::available ) {
                        return join_status::disallowed;
                    } else {
                        return join_status::mismatched_available;
                    }
                }
            } else {
                return join_status::free;
            }
        }

        void add_joins_for(
            const mutable_overmap_terrain &ter, const tripoint_om_omt &pos,
            om_direction::type rot, const std::vector<om_pos_dir> &suppressed_joins ) {
            consistency_check();

            std::unordered_set<om_pos_dir> avoid(
                suppressed_joins.begin(), suppressed_joins.end() );

            for( const std::pair<const cube_direction, mutable_overmap_terrain_join> &p :
                 ter.joins ) {
                cube_direction dir = p.first + rot;
                const mutable_overmap_terrain_join &this_side_join = p.second;

                om_pos_dir this_side{ pos, dir };
                om_pos_dir other_side = this_side.opposite();

                if( const join *other_side_join = resolved.find( other_side ) ) {
                    erase_unresolved( this_side );
                    if( !avoid.count( this_side ) ) {
                        used.emplace_back( other_side, other_side_join->join->id );
                        // Because of the existence of alternative joins, we don't
                        // simply add this_side_join here, we add the opposite of
                        // the opposite that was actually present (this saves us
                        // from heaving to search through the alternates to find
                        // which one actually matched).
                        used.emplace_back( this_side, other_side_join->join->opposite_id );
                    }
                } else {
                    // If there were postponed joins pointing into this point,
                    // so we need to un-postpone them because it might now be
                    // possible to satisfy them.
                    restore_postponed_at( other_side.p );
                    if( this_side_join.type == join_type::mandatory ) {
                        if( !overmap::inbounds( other_side.p ) ) {
                            debugmsg( "out of bounds join" );
                            continue;
                        }
                        const mutable_overmap_join *opposite_join = this_side_join.join->opposite;
                        add_unresolved( other_side, opposite_join );
                    }
                }
                resolved.add( this_side, this_side_join.join );
            }
            consistency_check();
        }

        tripoint_om_omt pick_top_priority() const {
            cata_assert( any_unresolved() );
            auto priority_it =
                std::find_if( unresolved_priority_index.begin(), unresolved_priority_index.end(),
            []( const cata::flat_set<iterator, compare_iterators> &its ) {
                return !its.empty();
            } );
            cata_assert( priority_it != unresolved_priority_index.end() );
            auto it = random_entry( *priority_it );
            const tripoint_om_omt &pos = it->where.p;
            cata_assert( !postponed.any_at( pos ) );
            return pos;
        }
        void postpone( const tripoint_om_omt &pos ) {
            consistency_check();
            for( iterator it : unresolved.all_at( pos ) ) {
                postponed.add( *it );
                [[maybe_unused]] const bool erased = erase_unresolved( it->where );
                cata_assert( erased );
            }
            consistency_check();
        }
        void restore_postponed_at( const tripoint_om_omt &pos ) {
            for( iterator it : postponed.all_at( pos ) ) {
                add_unresolved( it->where, it->join );
                postponed.erase( it );
            }
            consistency_check();
        }
        void restore_postponed() {
            consistency_check();
            for( const join &j : postponed ) {
                add_unresolved( j.where, j.join );
            }
            postponed.clear();
        }

        const std::vector<std::pair<om_pos_dir, std::string>> &all_used() const {
            return used;
        }
    private:
        struct indexed_joins {
            std::list<join> joins;
            std::unordered_map<om_pos_dir, iterator> position_index;

            iterator begin() {
                return joins.begin();
            }

            iterator end() {
                return joins.end();
            }

            const_iterator begin() const {
                return joins.begin();
            }

            const_iterator end() const {
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

            bool any_at( const tripoint_om_omt &pos ) const {
                for( cube_direction dir : all_enum_values<cube_direction>() ) {
                    if( count( om_pos_dir{ pos, dir } ) ) {
                        return true;
                    }
                }
                return false;
            }

            std::vector<iterator> all_at( const tripoint_om_omt &pos ) const {
                std::vector<iterator> result;
                for( cube_direction dir : all_enum_values<cube_direction>() ) {
                    om_pos_dir key{ pos, dir };
                    auto pos_it = position_index.find( key );
                    if( pos_it != position_index.end() ) {
                        result.push_back( pos_it->second );
                    }
                }
                return result;
            }

            std::size_t count_at( const tripoint_om_omt &pos ) const {
                std::size_t result = 0;
                for( cube_direction dir : all_enum_values<cube_direction>() ) {
                    if( position_index.find( { pos, dir } ) != position_index.end() ) {
                        ++result;
                    }
                }
                return result;
            }

            iterator add( const om_pos_dir &p, const mutable_overmap_join *j ) {
                return add( { p, j } );
            }

            iterator add( const join &j ) {
                joins.push_front( j );
                auto it = joins.begin();
                [[maybe_unused]] const bool inserted = position_index.emplace( j.where, it ).second;
                cata_assert( inserted );
                return it;
            }

            void erase( const iterator it ) {
                [[maybe_unused]] const size_t erased = position_index.erase( it->where );
                cata_assert( erased );
                joins.erase( it );
            }

            void clear() {
                joins.clear();
                position_index.clear();
            }
        };

        void add_unresolved( const om_pos_dir &p, const mutable_overmap_join *j ) {
            iterator it = unresolved.add( p, j );
            unsigned priority = it->join->priority;
            if( unresolved_priority_index.size() <= priority ) {
                unresolved_priority_index.resize( priority + 1 );
            }
            [[maybe_unused]] const bool inserted = unresolved_priority_index[priority].insert( it ).second;
            cata_assert( inserted );
        }

        bool erase_unresolved( const om_pos_dir &p ) {
            auto pos_it = unresolved.position_index.find( p );
            if( pos_it == unresolved.position_index.end() ) {
                return false;
            }
            iterator it = pos_it->second;
            unsigned priority = it->join->priority;
            cata_assert( priority < unresolved_priority_index.size() );
            [[maybe_unused]] const size_t erased = unresolved_priority_index[priority].erase( it );
            cata_assert( erased );
            unresolved.erase( it );
            return true;
        }

        struct compare_iterators {
            bool operator()( iterator l, iterator r ) {
                return l->where < r->where;
            }
        };

        indexed_joins unresolved;
        std::vector<cata::flat_set<iterator, compare_iterators>> unresolved_priority_index;

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
    ) const {
        int context_mandatory_joins_shortfall = 0;

        for( const mutable_overmap_piece_candidate piece : rule.pieces( origin, dir ) ) {
            if( !overmap::inbounds( piece.pos ) ) {
                return std::nullopt;
            }
            if( !is_amongst_locations( om.ter( piece.pos ), piece.overmap->locations ) ) {
                return std::nullopt;
            }
            if( unresolved.any_postponed_at( piece.pos ) ) {
                return std::nullopt;
            }
            context_mandatory_joins_shortfall -= unresolved.count_unresolved_at( piece.pos );
        }

        int num_my_non_available_matched = 0;

        std::vector<om_pos_dir> suppressed_joins;

        for( const std::pair<om_pos_dir, const mutable_overmap_terrain_join *> p :
             rule.outward_joins( origin, dir ) ) {
            const om_pos_dir &pos_d = p.first;
            const mutable_overmap_terrain_join &ter_join = *p.second;
            const mutable_overmap_join &join = *ter_join.join;
            switch( unresolved.allows( pos_d, ter_join ) ) {
                case joins_tracker::join_status::disallowed:
                    return std::nullopt;
                case joins_tracker::join_status::matched_non_available:
                    ++context_mandatory_joins_shortfall;
                // fallthrough
                case joins_tracker::join_status::matched_available:
                    if( ter_join.type != join_type::available ) {
                        ++num_my_non_available_matched;
                    }
                    continue;
                case joins_tracker::join_status::mismatched_available:
                    suppressed_joins.push_back( pos_d );
                case joins_tracker::join_status::free:
                    break;
            }
            if( ter_join.type == join_type::available ) {
                continue;
            }
            // Verify that the remaining joins lead to
            // suitable locations
            tripoint_om_omt neighbour = pos_d.p + displace( pos_d.dir );
            if( !overmap::inbounds( neighbour ) ) {
                return std::nullopt;
            }
            const oter_id &neighbour_terrain = om.ter( neighbour );
            if( !is_amongst_locations( neighbour_terrain, join.into_locations ) ) {
                return std::nullopt;
            }
        }
        return can_place_result{ context_mandatory_joins_shortfall,
                                 num_my_non_available_matched, suppressed_joins };
    }

    satisfy_result satisfy( const overmap &om, const tripoint_om_omt &pos,
                            const joins_tracker &unresolved ) {
        weighted_int_list<satisfy_result> options;

        for( mutable_overmap_placement_rule_remainder &rule : rules ) {
            std::vector<satisfy_result> pos_dir_options;
            can_place_result best_result{ 0, 0, {} };

            for( om_direction::type dir : om_direction::all ) {
                for( const tripoint_rel_omt &piece_pos : rule.positions( dir ) ) {
                    tripoint_om_omt origin = pos - piece_pos;

                    if( std::optional<can_place_result> result = can_place(
                                om, rule, origin, dir, unresolved ) ) {
                        if( best_result < *result ) {
                            pos_dir_options.clear();
                            best_result = *result;
                        }
                        if( *result == best_result ) {
                            pos_dir_options.push_back(
                                satisfy_result{ origin, dir, &rule, result.value().supressed_joins,
                                                {} } );
                        }
                    }
                }
            }

            if( auto chosen_result = random_entry_opt( pos_dir_options ) ) {
                options.add( *chosen_result, rule.get_weight() );
            }
        }
        std::string joins_s = enumerate_as_string( unresolved.all_unresolved_at( pos ),
        []( const joins_tracker::join * j ) {
            return string_format( "%s: %s", io::enum_to_string( j->where.dir ), j->join->id );
        } );

        if( satisfy_result *picked = options.pick() ) {
            om_direction::type dir = picked->dir;
            const mutable_overmap_placement_rule_remainder &rule = *picked->rule;
            picked->description =
                string_format(
                    // NOLINTNEXTLINE(cata-translate-string-literal)
                    "At %s chose '%s' rot %d with neighbours N:%s E:%s S:%s W:%s and constraints "
                    "%s",
                    pos.to_string(), rule.description(), static_cast<int>( dir ),
                    om.ter( pos + point_north ).id().str(), om.ter( pos + point_east ).id().str(),
                    om.ter( pos + point_south ).id().str(), om.ter( pos + point_west ).id().str(),
                    joins_s );
            picked->rule->decrement();
            return *picked;
        } else {
            std::string rules_s = enumerate_as_string( rules,
            []( const mutable_overmap_placement_rule_remainder & rule ) {
                if( rule.is_exhausted() ) {
                    return string_format( "(%s)", rule.description() );
                } else {
                    return rule.description();
                }
            } );
            std::string message =
                string_format(
                    // NOLINTNEXTLINE(cata-translate-string-literal)
                    "At %s FAILED to match on terrain %s with neighbours N:%s E:%s S:%s W:%s and "
                    "constraints %s from amongst rules %s",
                    pos.to_string(), om.ter( pos ).id().str(),
                    om.ter( pos + point_north ).id().str(), om.ter( pos + point_east ).id().str(),
                    om.ter( pos + point_south ).id().str(), om.ter( pos + point_west ).id().str(),
                    joins_s, rules_s );
            return { {}, om_direction::type::invalid, nullptr, {}, std::move( message ) };
        }
    }
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

template<typename Tripoint>
pos_dir<Tripoint> pos_dir<Tripoint>::opposite() const
{
    switch( dir ) {
        case cube_direction::north:
            return { p + tripoint_north, cube_direction::south };
        case cube_direction::east:
            return { p + tripoint_east, cube_direction::west };
        case cube_direction::south:
            return { p + tripoint_south, cube_direction::north };
        case cube_direction::west:
            return { p + tripoint_west, cube_direction::east };
        case cube_direction::above:
            return { p + tripoint_above, cube_direction::below };
        case cube_direction::below:
            return { p + tripoint_below, cube_direction::above };
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

template struct pos_dir<tripoint_om_omt>;
template struct pos_dir<tripoint_rel_omt>;

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
        : parent_id( p_id )
    {}

    void finalize( const std::string &context,
                   const cata::flat_set<string_id<overmap_location>> &default_locations ) override {
        if( check_for_locations.empty() ) {
            check_for_locations.push_back( root_as_overmap_special_terrain() );
        }
        for( size_t i = 0; i != joins_vec.size(); ++i ) {
            mutable_overmap_join &join = joins_vec[i];
            if( join.into_locations.empty() ) {
                join.into_locations = default_locations;
            }
            join.priority = i;
            joins.emplace( join.id, &join );
        }
        for( mutable_overmap_join &join : joins_vec ) {
            if( join.opposite_id.empty() ) {
                join.opposite_id = join.id;
                join.opposite = &join;
                continue;
            }
            auto opposite_it = joins.find( join.opposite_id );
            if( opposite_it == joins.end() ) {
                // Error reported later in check()
                continue;
            }
            join.opposite = opposite_it->second;
        }
        for( std::pair<const std::string, mutable_overmap_terrain> &p : overmaps ) {
            mutable_overmap_terrain &ter = p.second;
            ter.finalize( string_format( "overmap %s in %s", p.first, context ), joins,
                          default_locations );
        }
        for( mutable_overmap_phase &phase : phases ) {
            for( mutable_overmap_placement_rule &rule : phase.rules ) {
                rule.finalize( context, overmaps );
            }
        }
    }

    void finalize_mapgen_parameters(
        mapgen_parameters &params, const std::string &context ) const override {
        for( const std::pair<const std::string, mutable_overmap_terrain> &p : overmaps ) {
            const mutable_overmap_terrain &t = p.second;
            std::string mapgen_id = t.terrain->get_mapgen_id();
            params.check_and_merge( get_map_special_params( mapgen_id ), context );
        }
    }

    void check( const std::string &context ) const override {
        if( joins_vec.size() != joins.size() ) {
            debugmsg( "duplicate join id in %s", context );
        }
        for( const mutable_overmap_join &join : joins_vec ) {
            if( join.opposite ) {
                if( join.opposite->opposite_id != join.id ) {
                    debugmsg( "in %1$s: join id %2$s specifies its opposite to be %3$s, but "
                              "the opposite of %3$s is %4$s, when it should match the "
                              "original id %2$s",
                              context, join.id, join.opposite_id, join.opposite->opposite_id );
                }
            } else {
                debugmsg( "in %s: join id '%s' specified as opposite of '%s' not valid",
                          context, join.opposite_id, join.id );
            }
        }
        for( const std::pair<const std::string, mutable_overmap_terrain> &p : overmaps ) {
            const mutable_overmap_terrain &ter = p.second;
            ter.check( string_format( "overmap %s in %s", p.first, context ) );
        }
        if( !overmaps.count( root ) ) {
            debugmsg( "root %s is not amongst the defined overmaps for %s", root, context );
        }
        for( const mutable_overmap_phase &phase : phases ) {
            for( const mutable_overmap_placement_rule &rule : phase.rules ) {
                rule.check( context );
            }
        }
    }

    overmap_special_terrain root_as_overmap_special_terrain() const {
        auto it = overmaps.find( root );
        if( it == overmaps.end() ) {
            debugmsg( "root '%s' is not an overmap in this special", root );
            return {};
        }
        const mutable_overmap_terrain &root_om = it->second;
        return { tripoint_zero, root_om.terrain, root_om.locations, {} };
    }

    std::vector<overmap_special_terrain> preview_terrains() const override {
        return std::vector<overmap_special_terrain> { root_as_overmap_special_terrain() };
    }

    std::vector<overmap_special_locations> required_locations() const override {
        return check_for_locations;
    }

    int score_rotation_at( const overmap &, const tripoint_om_omt &,
                           om_direction::type ) const override {
        // TODO: worry about connections for mutable specials
        // For now we just allow all rotations, but will be restricted by
        // can_place_special
        return 0;
    }

    // Returns a list of the points placed and a list of the joins used
    special_placement_result place(
        overmap &om, const tripoint_om_omt &origin, om_direction::type dir, bool /*blob*/,
        const city &cit, bool must_be_unexplored ) const override {
        // TODO: respect must_be_unexplored
        std::vector<tripoint_om_omt> result;

        auto it = overmaps.find( root );
        if( it == overmaps.end() ) {
            debugmsg( "Invalid root %s", root );
            return { result, {} };
        }

        joins_tracker unresolved;

        struct placed_connection {
            overmap_connection_id connection;
            pos_dir<tripoint_om_omt> where;
        };

        std::vector<placed_connection> connections_placed;

        // This is for debugging only, it tracks a human-readable description
        // of what happened to be put in the debugmsg in the event of failure.
        std::vector<std::string> descriptions;

        // Helper function to add a particular mutable_overmap_terrain at a
        // particular place.
        auto add_ter = [&](
                           const mutable_overmap_terrain & ter, const tripoint_om_omt & pos,
        om_direction::type rot, const std::vector<om_pos_dir> &suppressed_joins ) {
            const oter_id tid = ter.terrain->get_rotated( rot );
            om.ter_set( pos, tid );
            unresolved.add_joins_for( ter, pos, rot, suppressed_joins );
            result.push_back( pos );

            // Accumulate connections to be dealt with later
            for( const std::pair<const cube_direction, mutable_special_connection> &p :
                 ter.connections ) {
                cube_direction base_dir = p.first;
                const mutable_special_connection &conn = p.second;
                cube_direction dir = base_dir + rot;
                tripoint_om_omt conn_pos = pos + displace( dir );
                if( overmap::inbounds( conn_pos ) ) {
                    connections_placed.push_back( { conn.connection, { conn_pos, dir } } );
                }
            }
        };

        const mutable_overmap_terrain &root_omt = it->second;
        add_ter( root_omt, origin, dir, {} );

        auto current_phase = phases.begin();
        mutable_overmap_phase_remainder phase_remaining = current_phase->realise();

        while( unresolved.any_unresolved() ) {
            tripoint_om_omt next_pos = unresolved.pick_top_priority();
            mutable_overmap_phase_remainder::satisfy_result satisfy_result =
                phase_remaining.satisfy( om, next_pos, unresolved );
            descriptions.push_back( std::move( satisfy_result.description ) );
            const mutable_overmap_placement_rule_remainder *rule = satisfy_result.rule;
            if( rule ) {
                const tripoint_om_omt &satisfy_origin = satisfy_result.origin;
                om_direction::type rot = satisfy_result.dir;
                for( const mutable_overmap_piece_candidate piece : rule->pieces( satisfy_origin, rot ) ) {
                    const mutable_overmap_terrain &ter = *piece.overmap;
                    add_ter( ter, piece.pos, piece.rot, satisfy_result.suppressed_joins );
                }
            } else {
                unresolved.postpone( next_pos );
            }
            if( !unresolved.any_unresolved() || phase_remaining.all_rules_exhausted() ) {
                ++current_phase;
                if( current_phase == phases.end() ) {
                    break;
                }
                descriptions.push_back(
                    // NOLINTNEXTLINE(cata-translate-string-literal)
                    string_format( "## Entering phase %td", current_phase - phases.begin() ) );
                phase_remaining = current_phase->realise();
                unresolved.restore_postponed();
            }
        }

        if( unresolved.any_postponed() ) {
            // This is an error in the JSON; extract some useful info to help
            // the user debug it
            unresolved.restore_postponed();
            tripoint_om_omt p = unresolved.pick_top_priority();

            const oter_id &current_terrain = om.ter( p );
            std::string joins = enumerate_as_string( unresolved.all_unresolved_at( p ),
            []( const joins_tracker::join * dir_join ) {
                // NOLINTNEXTLINE(cata-translate-string-literal)
                return string_format( "%s: %s", io::enum_to_string( dir_join->where.dir ),
                                      dir_join->join->id );
            } );

            debugmsg( "Spawn of mutable special %s had unresolved joins.  Existing terrain "
                      "at %s was %s; joins were %s\nComplete record of placement follows:\n%s",
                      parent_id.str(), p.to_string(), current_terrain.id().str(), joins,
                      string_join( descriptions, "\n" ) );

            om.add_note(
                p, string_format(
                    // NOLINTNEXTLINE(cata-translate-string-literal)
                    "U:R;DEBUG: unresolved joins %s at %s placing %s",
                    joins, p.to_string(), parent_id.str() ) );
        }

        // Deal with connections
        for( const placed_connection &elem : connections_placed ) {
            const tripoint_om_omt &pos = elem.where.p;
            cube_direction connection_dir = elem.where.dir;

            if( cit ) {
                om.build_connection( cit.pos, pos.xy(), pos.z(), *elem.connection,
                                     must_be_unexplored, connection_dir );
            }
            // if no city present, search for nearby road within 50 tiles and make connection to it instead
            else {
                for( const tripoint_om_omt &nearby_point : closest_points_first( pos, 50 ) ) {
                    if( om.check_ot( "road", ot_match_type::contains, nearby_point ) ) {
                        om.build_connection(
                            nearby_point.xy(), pos.xy(), pos.z(), *elem.connection,
                            must_be_unexplored, connection_dir );
                    }
                }
            }
        }

        return { result, unresolved.all_used() };
    }
};

overmap_special::overmap_special( const overmap_special_id &i, const overmap_special_terrain &ter )
    : id( i )
    , subtype_( overmap_special_subtype::fixed )
    , data_{ make_shared_fast<fixed_overmap_special_data>( ter ) }
{}

bool overmap_special::can_spawn() const
{
    if( get_constraints().occurrences.empty() ) {
        return false;
    }

    const int city_size = get_option<int>( "CITY_SIZE" );
    return city_size != 0 || get_constraints().city_size.min <= city_size;
}

bool overmap_special::requires_city() const
{
    return constraints_.city_size.min > 0 ||
           constraints_.city_distance.max < std::max( OMAPX, OMAPY );
}

bool overmap_special::can_belong_to_city( const tripoint_om_omt &p, const city &cit ) const
{
    if( !requires_city() ) {
        return true;
    }
    if( !cit || !constraints_.city_size.contains( cit.size ) ) {
        return false;
    }
    return constraints_.city_distance.contains( cit.get_distance_from( p ) - ( cit.size ) );
}

bool overmap_special::has_flag( const std::string &flag ) const
{
    return flags_.count( flag );
}

int overmap_special::longest_side() const
{
    // Figure out the longest side of the special for purposes of determining our sector size
    // when attempting placements.
    std::vector<overmap_special_locations> req_locations = required_locations();
    auto min_max_x = std::minmax_element( req_locations.begin(), req_locations.end(),
    []( const overmap_special_locations & lhs, const overmap_special_locations & rhs ) {
        return lhs.p.x < rhs.p.x;
    } );

    auto min_max_y = std::minmax_element( req_locations.begin(), req_locations.end(),
    []( const overmap_special_locations & lhs, const overmap_special_locations & rhs ) {
        return lhs.p.y < rhs.p.y;
    } );

    const int width = min_max_x.second->p.x - min_max_x.first->p.x;
    const int height = min_max_y.second->p.y - min_max_y.first->p.y;
    return std::max( width, height ) + 1;
}

std::vector<overmap_special_terrain> overmap_special::preview_terrains() const
{
    return data_->preview_terrains();
}

std::vector<overmap_special_locations> overmap_special::required_locations() const
{
    return data_->required_locations();
}

int overmap_special::score_rotation_at( const overmap &om, const tripoint_om_omt &p,
                                        om_direction::type r ) const
{
    return data_->score_rotation_at( om, p, r );
}

special_placement_result overmap_special::place(
    overmap &om, const tripoint_om_omt &origin, om_direction::type dir,
    const city &cit, bool must_be_unexplored ) const
{
    if( has_eoc() ) {
        dialogue d( get_talker_for( get_avatar() ), nullptr );
        get_eoc()->apply_true_effects( d );
    }
    const bool blob = has_flag( "BLOB" );
    return data_->place( om, origin, dir, blob, cit, must_be_unexplored );
}

void overmap_special::force_one_occurrence()
{
    constraints_.occurrences.min = 1;
    constraints_.occurrences.max = 1;
}

mapgen_arguments overmap_special::get_args( const mapgendata &md ) const
{
    return mapgen_params_.get_args( md, mapgen_parameter_scope::overmap_special );
}

void overmap_special::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";
    // city_building is just an alias of overmap_special
    // TODO: This comparison is a hack. Separate them properly.
    const bool is_special = jo.get_string( "type", "" ) == "overmap_special";

    optional( jo, was_loaded, "subtype", subtype_, overmap_special_subtype::fixed );
    optional( jo, was_loaded, "locations", default_locations_ );
    if( jo.has_member( "eoc" ) ) {
        eoc = effect_on_conditions::load_inline_eoc( jo.get_member( "eoc" ), src );
        has_eoc_ = true;
    }
    switch( subtype_ ) {
        case overmap_special_subtype::fixed: {
            shared_ptr_fast<fixed_overmap_special_data> fixed_data =
                make_shared_fast<fixed_overmap_special_data>();
            optional( jo, was_loaded, "overmaps", fixed_data->terrains );
            if( is_special ) {
                optional( jo, was_loaded, "connections", fixed_data->connections );
            }
            data_ = std::move( fixed_data );
            break;
        }
        case overmap_special_subtype::mutable_: {
            shared_ptr_fast<mutable_overmap_special_data> mutable_data =
                make_shared_fast<mutable_overmap_special_data>( id );
            std::vector<overmap_special_locations> check_for_locations_merged_data;
            optional( jo, was_loaded, "check_for_locations", check_for_locations_merged_data );
            if( jo.has_array( "check_for_locations_area" ) ) {
                JsonArray jar = jo.get_array( "check_for_locations_area" );
                while( jar.has_more() ) {
                    JsonObject joc = jar.next_object();

                    cata::flat_set<string_id<overmap_location>> type;
                    tripoint from;
                    tripoint to;
                    mandatory( joc, was_loaded, "type", type );
                    mandatory( joc, was_loaded, "from", from );
                    mandatory( joc, was_loaded, "to", to );
                    if( from.x > to.x ) {
                        std::swap( from.x, to.x );
                    }
                    if( from.y > to.y ) {
                        std::swap( from.y, to.y );
                    }
                    if( from.z > to.z ) {
                        std::swap( from.z, to.z );
                    }
                    for( int x = from.x; x <= to.x; x++ ) {
                        for( int y = from.y; y <= to.y; y++ ) {
                            for( int z = from.z; z <= to.z; z++ ) {
                                overmap_special_locations loc;
                                loc.p = tripoint( x, y, z );
                                loc.locations = type;
                                check_for_locations_merged_data.push_back( loc );
                            }
                        }
                    }
                }
            }
            mutable_data->check_for_locations = check_for_locations_merged_data;
            mandatory( jo, was_loaded, "joins", mutable_data->joins_vec );
            mandatory( jo, was_loaded, "overmaps", mutable_data->overmaps );
            mandatory( jo, was_loaded, "root", mutable_data->root );
            mandatory( jo, was_loaded, "phases", mutable_data->phases );
            data_ = std::move( mutable_data );
            break;
        }
        default:
            jo.throw_error( string_format( "subtype %s not implemented",
                                           io::enum_to_string( subtype_ ) ) );
    }

    if( is_special ) {
        mandatory( jo, was_loaded, "occurrences", constraints_.occurrences );

        assign( jo, "city_sizes", constraints_.city_size, strict );
        assign( jo, "city_distance", constraints_.city_distance, strict );
        assign( jo, "priority", priority_, strict );
    }

    assign( jo, "spawns", monster_spawns_, strict );

    assign( jo, "rotate", rotatable_, strict );
    assign( jo, "flags", flags_, strict );
}

void overmap_special::finalize()
{
    const_cast<overmap_special_data &>( *data_ ).finalize(
        "overmap special " + id.str(), default_locations_ );
}

void overmap_special::finalize_mapgen_parameters()
{
    // Extract all the map_special-scoped params from the constituent terrains
    // and put them here
    std::string context = string_format( "overmap_special %s", id.str() );
    data_->finalize_mapgen_parameters( mapgen_params_, context );
}

void overmap_special::check() const
{
    data_->check( string_format( "overmap special %s", id.str() ) );
}

// *** BEGIN overmap FUNCTIONS ***
overmap::overmap( const point_abs_om &p ) : loc( p )
{
    const std::string rsettings_id = get_option<std::string>( "DEFAULT_REGION" );
    t_regional_settings_map_citr rsit = region_settings_map.find( rsettings_id );

    if( rsit == region_settings_map.end() ) {
        debugmsg( "overmap%s: can't find region '%s'", p.to_string(),
                  rsettings_id.c_str() ); // gonna die now =[
    }
    settings = &rsit->second;

    init_layers();
}

overmap::~overmap() = default;

void overmap::populate( overmap_special_batch &enabled_specials )
{
    try {
        open( enabled_specials );
    } catch( const std::exception &err ) {
        debugmsg( "overmap (%d,%d) failed to load: %s", loc.x(), loc.y(), err.what() );
    }
}

void overmap::populate()
{
    overmap_special_batch enabled_specials = overmap_specials::get_default_batch( loc );
    const overmap_feature_flag_settings &overmap_feature_flag = settings->overmap_feature_flag;

    const bool should_blacklist = !overmap_feature_flag.blacklist.empty();
    const bool should_whitelist = !overmap_feature_flag.whitelist.empty();

    // If this region's settings has blacklisted or whitelisted overmap feature flags, let's
    // filter our default batch.

    // Remove any items that have a flag that is present in the blacklist.
    if( should_blacklist ) {
        for( auto it = enabled_specials.begin(); it != enabled_specials.end(); ) {
            if( cata::sets_intersect( overmap_feature_flag.blacklist,
                                      it->special_details->get_flags() ) ) {
                it = enabled_specials.erase( it );
            } else {
                ++it;
            }
        }
    }

    // Remove any items which do not have any of the flags from the whitelist.
    if( should_whitelist ) {
        for( auto it = enabled_specials.begin(); it != enabled_specials.end(); ) {
            if( cata::sets_intersect( overmap_feature_flag.whitelist,
                                      it->special_details->get_flags() ) ) {
                ++it;
            } else {
                it = enabled_specials.erase( it );
            }
        }
    }

    populate( enabled_specials );
}

oter_id overmap::get_default_terrain( int z ) const
{
    return settings->default_oter[OVERMAP_DEPTH + z].id();
}

void overmap::init_layers()
{
    for( int k = 0; k < OVERMAP_LAYERS; ++k ) {
        const oter_id tid = get_default_terrain( k - OVERMAP_DEPTH );
        map_layer &l = layer[k];
        l.terrain.fill( tid );
        l.visible.fill( false );
        l.explored.fill( false );
    }
}

void overmap::ter_set( const tripoint_om_omt &p, const oter_id &id )
{
    if( !inbounds( p ) ) {
        /// TODO: Add a debug message reporting this, but currently there are way too many place that would trigger it.
        return;
    }

    oter_id &current_oter = layer[p.z() + OVERMAP_DEPTH].terrain[p.xy()];
    const oter_type_str_id &current_type_id = current_oter->get_type_id();
    const oter_type_str_id &incoming_type_id = id->get_type_id();
    const bool current_type_same = current_type_id == incoming_type_id;

    // Mapgen refinement can push multiple different roads over each other.
    // Roads require a predecessor. A road pushed over a road might cause a
    // road to be a predecessor to another road. That causes too many spawns
    // to happen. So when pushing a predecessor, if the predecessor to-be-pushed
    // is linear and the previous predecessor is linear, overwrite it.
    // This way only the 'last' rotation/variation generated is kept.
    if( id->has_flag( oter_flags::requires_predecessor ) ) {
        std::vector<oter_id> &om_predecessors = predecessors_[p];
        if( om_predecessors.empty() || ( !current_oter->is_linear() && !current_type_same ) ) {
            // If we need a predecessor, we must have a predecessor no matter what.
            // Or, if the oter to-be-pushed is not linear, push it only if the incoming oter is different.
            om_predecessors.push_back( current_oter );
        } else if( !current_type_same ) {
            // Current oter is linear, incoming oter is different from current.
            // If the last predecessor is the same type as the current type, overwrite.
            // Else push the current type.
            oter_id &last_predecessor = om_predecessors.back();
            if( last_predecessor->get_type_id() == current_type_id ) {
                last_predecessor = current_oter;
            } else {
                om_predecessors.push_back( current_oter );
            }
        }
        // We had a predecessor, and it was the same type as the incoming one
        // Don't push another copy.
    }
    current_oter = id;
}

const oter_id &overmap::ter( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {
        /// TODO: Add a debug message reporting this, but currently there are way too many place that would trigger it.
        return ot_null;
    }

    return ter_unsafe( p );
}

const oter_id &overmap::ter_unsafe( const tripoint_om_omt &p ) const
{
    return layer[p.z() + OVERMAP_DEPTH].terrain[p.xy()];
}

std::optional<mapgen_arguments> *overmap::mapgen_args( const tripoint_om_omt &p )
{
    auto it = mapgen_args_index.find( p );
    if( it == mapgen_args_index.end() ) {
        return nullptr;
    }
    return it->second;
}

std::string *overmap::join_used_at( const om_pos_dir &p )
{
    auto it = joins_used.find( p );
    if( it == joins_used.end() ) {
        return nullptr;
    }
    return &it->second;
}

std::vector<oter_id> overmap::predecessors( const tripoint_om_omt &p )
{
    auto it = predecessors_.find( p );
    if( it == predecessors_.end() ) {
        return {};
    }
    return it->second;
}

void overmap::set_seen( const tripoint_om_omt &p, bool val )
{
    if( !inbounds( p ) ) {
        return;
    }

    if( seen( p ) == val ) {
        return;
    }

    layer[p.z() + OVERMAP_DEPTH].visible[p.xy()] = val;

    if( val ) {
        add_extra_note( p );
    }
}

bool overmap::seen( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    return layer[p.z() + OVERMAP_DEPTH].visible[p.xy()];
}

bool &overmap::explored( const tripoint_om_omt &p )
{
    if( !inbounds( p ) ) {
        nullbool = false;
        return nullbool;
    }
    return layer[p.z() + OVERMAP_DEPTH].explored[p.xy()];
}

bool overmap::is_explored( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    return layer[p.z() + OVERMAP_DEPTH].explored[p.xy()];
}

bool overmap::mongroup_check( const mongroup &candidate ) const
{
    tripoint_om_sm relp = candidate.rel_pos();
    const auto matching_range = zg.equal_range( relp );
    return std::find_if( matching_range.first, matching_range.second,
    [candidate]( const std::pair<tripoint_om_sm, mongroup> &match ) {
        // This is extra strict since we're using it to test serialization.
        return candidate.type == match.second.type && candidate.abs_pos == match.second.abs_pos &&
               candidate.population == match.second.population &&
               candidate.target == match.second.target &&
               candidate.interest == match.second.interest &&
               candidate.dying == match.second.dying &&
               candidate.horde == match.second.horde;
    } ) != matching_range.second;
}

bool overmap::monster_check( const std::pair<tripoint_om_sm, monster> &candidate ) const
{
    const auto matching_range = monster_map.equal_range( candidate.first );
    return std::find_if( matching_range.first, matching_range.second,
    [candidate]( const std::pair<tripoint_om_sm, monster> &match ) {
        return candidate.second.pos() == match.second.pos() &&
               candidate.second.type == match.second.type;
    } ) != matching_range.second;
}

void overmap::insert_npc( const shared_ptr_fast<npc> &who )
{
    npcs.push_back( who );
    g->set_npcs_dirty();
}

shared_ptr_fast<npc> overmap::erase_npc( const character_id &id )
{
    const auto iter = std::find_if( npcs.begin(),
    npcs.end(), [id]( const shared_ptr_fast<npc> &n ) {
        return n->getID() == id;
    } );
    if( iter == npcs.end() ) {
        return nullptr;
    }
    auto ptr = *iter;
    npcs.erase( iter );
    g->set_npcs_dirty();
    return ptr;
}

std::vector<shared_ptr_fast<npc>> overmap::get_npcs( const
                               std::function<bool( const npc & )>
                               &predicate ) const
{
    std::vector<shared_ptr_fast<npc>> result;
    for( const auto &g : npcs ) {
        if( predicate( *g ) ) {
            result.push_back( g );
        }
    }
    return result;
}

bool overmap::has_note( const tripoint_om_omt &p ) const
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return false;
    }

    for( const om_note &i : layer[p.z() + OVERMAP_DEPTH].notes ) {
        if( i.p == p.xy() ) {
            return true;
        }
    }
    return false;
}

bool overmap::is_marked_dangerous( const tripoint_om_omt &p ) const
{
    for( const om_note &i : layer[p.z() + OVERMAP_DEPTH].notes ) {
        if( !i.dangerous ) {
            continue;
        } else if( p.xy() == i.p ) {
            return true;
        }
        const int radius = i.danger_radius;
        if( i.danger_radius == 0 && i.p != p.xy() ) {
            continue;
        }
        for( int x = -radius; x <= radius; x++ ) {
            for( int y = -radius; y <= radius; y++ ) {
                const tripoint_om_omt rad_point = tripoint_om_omt( i.p, p.z() ) + point( x, y );
                if( p.xy() == rad_point.xy() ) {
                    return true;
                }
            }
        }
    }
    return false;
}

const std::string &overmap::note( const tripoint_om_omt &p ) const
{
    static const std::string fallback {};

    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return fallback;
    }

    const auto &notes = layer[p.z() + OVERMAP_DEPTH].notes;
    const auto it = std::find_if( begin( notes ), end( notes ), [&]( const om_note & n ) {
        return n.p == p.xy();
    } );

    return ( it != std::end( notes ) ) ? it->text : fallback;
}

void overmap::add_note( const tripoint_om_omt &p, std::string message )
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        debugmsg( "Attempting to add not to overmap for blank layer %d", p.z() );
        return;
    }

    auto &notes = layer[p.z() + OVERMAP_DEPTH].notes;
    const auto it = std::find_if( begin( notes ), end( notes ), [&]( const om_note & n ) {
        return n.p == p.xy();
    } );

    if( it == std::end( notes ) ) {
        notes.emplace_back( om_note{ std::move( message ), p.xy() } );
    } else if( !message.empty() ) {
        it->text = std::move( message );
    } else {
        notes.erase( it );
    }
}

void overmap::mark_note_dangerous( const tripoint_om_omt &p, int radius, bool is_dangerous )
{
    for( om_note &i : layer[p.z() + OVERMAP_DEPTH].notes ) {
        if( p.xy() == i.p ) {
            i.dangerous = is_dangerous;
            i.danger_radius = radius;
            return;
        }
    }
}

void overmap::delete_note( const tripoint_om_omt &p )
{
    add_note( p, std::string{} );
}

std::vector<point_abs_omt> overmap::find_notes( const int z, const std::string &text )
{
    std::vector<point_abs_omt> note_locations;
    map_layer &this_layer = layer[z + OVERMAP_DEPTH];
    for( const om_note &note : this_layer.notes ) {
        if( match_include_exclude( note.text, text ) ) {
            note_locations.push_back( project_combine( pos(), note.p ) );
        }
    }
    return note_locations;
}

bool overmap::has_extra( const tripoint_om_omt &p ) const
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return false;
    }

    for( const om_map_extra &i : layer[p.z() + OVERMAP_DEPTH].extras ) {
        if( i.p == p.xy() ) {
            return true;
        }
    }
    return false;
}

const map_extra_id &overmap::extra( const tripoint_om_omt &p ) const
{
    static const map_extra_id fallback{};

    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return fallback;
    }

    const auto &extras = layer[p.z() + OVERMAP_DEPTH].extras;
    const auto it = std::find_if( begin( extras ),
    end( extras ), [&]( const om_map_extra & n ) {
        return n.p == p.xy();
    } );

    return ( it != std::end( extras ) ) ? it->id : fallback;
}

void overmap::add_extra( const tripoint_om_omt &p, const map_extra_id &id )
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        debugmsg( "Attempting to add not to overmap for blank layer %d", p.z() );
        return;
    }

    auto &extras = layer[p.z() + OVERMAP_DEPTH].extras;
    const auto it = std::find_if( begin( extras ),
    end( extras ), [&]( const om_map_extra & n ) {
        return n.p == p.xy();
    } );

    if( it == std::end( extras ) ) {
        extras.emplace_back( om_map_extra{ id, p.xy() } );
        add_extra_note( p );
    } else if( !id.is_null() ) {
        it->id = id;
        add_extra_note( p );
    } else {
        extras.erase( it );
    }
}

void overmap::add_extra_note( const tripoint_om_omt &p )
{
    if( !seen( p ) ) {
        return;
    }

    const std::vector<om_map_extra> &layer_extras = layer[p.z() + OVERMAP_DEPTH].extras;
    auto extrait = std::find_if( layer_extras.begin(),
    layer_extras.end(), [&p]( const om_map_extra & extra ) {
        return extra.p == p.xy();
    } );
    if( extrait == layer_extras.end() ) {
        return;
    }
    const map_extra_id &extra = extrait->id;

    auto_notes::auto_note_settings &auto_note_settings = get_auto_notes_settings();

    // The player has discovered a map extra of this type.
    auto_note_settings.set_discovered( extra );

    if( get_option<bool>( "AUTO_NOTES" ) && get_option<bool>( "AUTO_NOTES_MAP_EXTRAS" ) ) {
        // Only place note if the user has not disabled it via the auto note manager
        if( !auto_note_settings.has_auto_note_enabled( extra, true ) ) {
            return;
        }

        const std::optional<auto_notes::custom_symbol> &symbol =
            auto_note_settings.get_custom_symbol( extra );
        const std::string note_symbol = symbol ? ( *symbol ).get_symbol_string() : extra->get_symbol();
        const nc_color note_color = symbol ? ( *symbol ).get_color() : extra->color;
        const std::string mx_note =
            string_format( "%s:%s;<color_yellow>%s</color>: <color_white>%s</color>",
                           note_symbol,
                           get_note_string_from_color( note_color ),
                           extra->name(),
                           extra->description() );
        add_note( p, mx_note );
    }
}

void overmap::delete_extra( const tripoint_om_omt &p )
{
    add_extra( p, map_extra_id::NULL_ID() );
}

std::vector<point_abs_omt> overmap::find_extras( const int z, const std::string &text )
{
    std::vector<point_abs_omt> extra_locations;
    map_layer &this_layer = layer[z + OVERMAP_DEPTH];
    for( const om_map_extra &extra : this_layer.extras ) {
        const std::string extra_text = extra.id.c_str();
        if( match_include_exclude( extra_text, text ) ) {
            extra_locations.push_back( project_combine( pos(), extra.p ) );
        }
    }
    return extra_locations;
}

bool overmap::inbounds( const tripoint_om_omt &p, int clearance )
{
    static constexpr tripoint_om_omt overmap_boundary_min( 0, 0, -OVERMAP_DEPTH );
    static constexpr tripoint_om_omt overmap_boundary_max( OMAPX, OMAPY, OVERMAP_HEIGHT + 1 );

    static constexpr half_open_cuboid<tripoint_om_omt> overmap_boundaries(
        overmap_boundary_min, overmap_boundary_max );
    half_open_cuboid<tripoint_om_omt> stricter_boundaries = overmap_boundaries;
    stricter_boundaries.shrink( tripoint( clearance, clearance, 0 ) );

    return stricter_boundaries.contains( p );
}

const scent_trace &overmap::scent_at( const tripoint_abs_omt &loc ) const
{
    static const scent_trace null_scent;
    const auto &scent_found = scents.find( loc );
    if( scent_found != scents.end() ) {
        return scent_found->second;
    }
    return null_scent;
}

void overmap::set_scent( const tripoint_abs_omt &loc, const scent_trace &new_scent )
{
    // TODO: increase strength of scent trace when applied repeatedly in a short timespan.
    scents[loc] = new_scent;
}

void overmap::generate( const overmap *north, const overmap *east,
                        const overmap *south, const overmap *west,
                        overmap_special_batch &enabled_specials )
{
    dbg( D_INFO ) << "overmap::generate start…";
    const oter_id omt_outside_defined_omap = static_cast<oter_id>
            ( get_option<std::string>( "OUTSIDE_DEFINED_OMAP_OMT" ) );
    const std::string overmap_pregenerated_path =
        get_option<std::string>( "OVERMAP_PREGENERATED_PATH" );
    if( !overmap_pregenerated_path.empty() ) {
        // HACK: For some reason gz files are automatically unpacked and renamed during Android build process
#if defined(__ANDROID__)
        static const std::string fname = "%s/overmap_%d_%d.omap";
#else
        static const std::string fname = "%s/overmap_%d_%d.omap.gz";
#endif
        const cata_path fpath = PATH_INFO::moddir() / string_format( fname,
                                overmap_pregenerated_path, pos().x(), pos().y() );
        dbg( D_INFO ) << "trying" << fpath;
        if( !read_from_file_optional_json( fpath, [this, &fpath]( const JsonValue & jv ) {
        unserialize_omap( jv, fpath );
        } ) ) {
            dbg( D_INFO ) << "failed" << fpath;
            int z = 0;
            for( int j = 0; j < OMAPY; j++ ) {
                // NOLINTNEXTLINE(modernize-loop-convert)
                for( int i = 0; i < OMAPX; i++ ) {
                    layer[z + OVERMAP_DEPTH].terrain[i][j] = omt_outside_defined_omap;
                }
            }
        }
    }
    calculate_urbanity();
    calculate_forestosity();
    if( get_option<bool>( "OVERMAP_POPULATE_OUTSIDE_CONNECTIONS_FROM_NEIGHBORS" ) ) {
        populate_connections_out_from_neighbors( north, east, south, west );
    }
    if( get_option<bool>( "OVERMAP_PLACE_RIVERS" ) ) {
        place_rivers( north, east, south, west );
    }
    if( get_option<bool>( "OVERMAP_PLACE_LAKES" ) ) {
        place_lakes();
    }
    if( get_option<bool>( "OVERMAP_PLACE_OCEANS" ) ) {
        place_oceans();
    }
    if( get_option<bool>( "OVERMAP_PLACE_FORESTS" ) ) {
        place_forests();
    }
    if( get_option<bool>( "OVERMAP_PLACE_SWAMPS" ) ) {
        place_swamps();
    }
    if( get_option<bool>( "OVERMAP_PLACE_RAVINES" ) ) {
        place_ravines();
    }
    if( get_option<bool>( "OVERMAP_PLACE_CITIES" ) ) {
        place_cities();
    }
    if( get_option<bool>( "OVERMAP_PLACE_FOREST_TRAILS" ) ) {
        place_forest_trails();
    }
    if( get_option<bool>( "OVERMAP_PLACE_RAILROADS_BEFORE_ROADS" ) ) {
        if( get_option<bool>( "OVERMAP_PLACE_RAILROADS" ) ) {
            place_railroads( north, east, south, west );
        }
        if( get_option<bool>( "OVERMAP_PLACE_ROADS" ) ) {
            place_roads( north, east, south, west );
        }
    } else {
        if( get_option<bool>( "OVERMAP_PLACE_ROADS" ) ) {
            place_roads( north, east, south, west );
        }
        if( get_option<bool>( "OVERMAP_PLACE_RAILROADS" ) ) {
            place_railroads( north, east, south, west );
        }
    }
    if( get_option<bool>( "OVERMAP_PLACE_SPECIALS" ) ) {
        place_specials( enabled_specials );
    }
    if( get_option<bool>( "OVERMAP_PLACE_FOREST_TRAILHEADS" ) ) {
        place_forest_trailheads();
    }

    polish_river();

    // TODO: there is no reason we can't generate the sublevels in one pass
    //       for that matter there is no reason we can't as we add the entrance ways either

    // Always need at least one sublevel, but how many more
    int z = -1;
    bool requires_sub = false;
    do {
        requires_sub = generate_sub( z );
    } while( requires_sub && ( --z >= -OVERMAP_DEPTH ) );

    // Always need at least one overlevel, but how many more
    z = 1;
    bool requires_over = false;
    do {
        requires_over = generate_over( z );
    } while( requires_over && ( ++z <= OVERMAP_HEIGHT ) );

    // Place the monsters, now that the terrain is laid out
    place_mongroups();
    place_radios();
    dbg( D_INFO ) << "overmap::generate done";
}

bool overmap::generate_sub( const int z )
{
    cata_assert( z < 0 );

    bool requires_sub = false;
    std::vector<point_om_omt> subway_points;
    std::vector<point_om_omt> sewer_points;

    std::vector<city> ant_points;
    std::vector<city> goo_points;
    std::vector<city> lab_points;
    std::vector<city> ice_lab_points;
    std::vector<city> central_lab_points;
    std::vector<point_om_omt> lab_train_points;
    std::vector<point_om_omt> central_lab_train_points;

    const auto add_goo_point = [&]( const tripoint_om_omt & p ) {
        const int size = rng( MIN_GOO_SIZE, MAX_GOO_SIZE );
        goo_points.emplace_back( p.xy(), size );
    };

    std::unordered_map<oter_type_id, std::function<void( const tripoint_om_omt &p )>>
    oter_above_actions = {
        { oter_type_empty_rock.id(), []( const tripoint_om_omt & ) {} },
        { oter_type_forest.id(), []( const tripoint_om_omt & ) {} },
        { oter_type_field.id(), []( const tripoint_om_omt & ) {} },
        { oter_type_forest_water.id(), []( const tripoint_om_omt & ) {} },
        { oter_type_forest_thick.id(), []( const tripoint_om_omt & ) {} },
        { oter_type_solid_earth.id(), []( const tripoint_om_omt & ) {} },
        {
            oter_type_road_nesw_manhole.id(),
            [&]( const tripoint_om_omt & p )
            {
                ter_set( p, oter_sewer_isolated.id() );
                sewer_points.emplace_back( p.xy() );
            }
        },
        { oter_type_slimepit_down.id(), add_goo_point },
        { oter_type_slimepit_bottom.id(), add_goo_point },
        {
            oter_type_lab_core.id(),
            [&]( const tripoint_om_omt & p )
            {
                lab_points.emplace_back( p.xy(), rng( 1, 5 + z ) );
            }
        },
        {
            oter_type_lab_stairs.id(),
            [&]( const tripoint_om_omt & p )
            {
                if( z == -1 ) {
                    lab_points.emplace_back( p.xy(), rng( 1, 5 + z ) );
                } else {
                    ter_set( p, oter_lab.id() );
                }
            }
        },
        {
            oter_type_ice_lab_core.id(),
            [&]( const tripoint_om_omt & p )
            {
                ice_lab_points.emplace_back( p.xy(), rng( 1, 5 + z ) );
            }
        },
        {
            oter_type_ice_lab_stairs.id(),
            [&]( const tripoint_om_omt & p )
            {
                if( z == -1 ) {
                    ice_lab_points.emplace_back( p.xy(), rng( 1, 5 + z ) );
                } else {
                    ter_set( p, oter_ice_lab.id() );
                }
            }
        },
        {
            oter_type_central_lab_core.id(),
            [&]( const tripoint_om_omt & p )
            {
                central_lab_points.emplace_back( p.xy(), rng( std::max( 1, 7 + z ), 9 + z ) );
            }
        },
        {
            oter_type_central_lab_stairs.id(),
            [&]( const tripoint_om_omt & p )
            {
                ter_set( p, oter_central_lab.id() );
            }
        },
    };

    // Avoid constructing strings inside the loop
    static const std::string s_hidden_lab_stairs = "hidden_lab_stairs";

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            tripoint_om_omt p( i, j, z );
            const oter_id oter_id_here = ter_unsafe( p );
            const oter_t &oter_here = *oter_id_here;
            const oter_id oter_above = ter_unsafe( p + tripoint_above );
            const oter_id oter_ground = ter_unsafe( tripoint_om_omt( p.xy(), 0 ) );

            if( oter_here.get_type_id() == oter_type_sewer_connector ) {
                om_direction::type rotation = oter_here.get_dir();
                ter_set( p, oter_sewer_end_north.id()->get_rotated( rotation ) );
                sewer_points.emplace_back( p.xy() );
            }

            if( oter_here.get_type_id() == oter_type_microlab_sub_connector ) {
                om_direction::type rotation = oter_here.get_dir();
                ter_set( p, oter_subway_end_north.id()->get_rotated( rotation ) );
                subway_points.emplace_back( p.xy() );
            }

            if( oter_ground->get_type_id() == oter_type_sub_station ) {
                if( z == -1 ) {
                    ter_set( p, oter_sewer_sub_station.id() );
                    requires_sub = true;
                    continue;
                } else if( z == -2 ) {
                    ter_set( p, oter_subway_isolated.id() );
                    subway_points.emplace_back( i, j - 1 );
                    subway_points.emplace_back( i, j );
                    subway_points.emplace_back( i, j + 1 );
                    continue;
                }
            }

            auto above_action_it = oter_above_actions.find( oter_above->get_type_id().id() );

            if( above_action_it != oter_above_actions.end() ) {
                above_action_it->second( p );
            } else if( is_ot_match( s_hidden_lab_stairs, oter_above, ot_match_type::contains ) ) {
                lab_points.emplace_back( p.xy(), rng( 1, 5 + z ) );
            }
        }
    }

    for( city &i : goo_points ) {
        requires_sub |= build_slimepit( tripoint_om_omt( i.pos, z ), i.size );
    }
    connect_closest_points( sewer_points, z, *overmap_connection_sewer_tunnel );

    // A third of overmaps have labs with a 1-in-2 chance of being subway connected.
    // If the central lab exists, all labs which go down to z=4 will have a subway to central.
    int lab_train_odds = 0;
    if( z == -2 && one_in( 3 ) ) {
        lab_train_odds = 2;
    }
    if( z == -4 && !central_lab_points.empty() ) {
        lab_train_odds = 1;
    }

    for( city &i : lab_points ) {
        bool lab = build_lab( tripoint_om_omt( i.pos, z ), i.size, &lab_train_points, "", lab_train_odds );
        requires_sub |= lab;
        if( !lab && ter( tripoint_om_omt( i.pos, z ) ) == oter_lab_core ) {
            ter_set( tripoint_om_omt( i.pos, z ), oter_lab.id() );
        }
    }
    for( city &i : ice_lab_points ) {
        bool ice_lab = build_lab( tripoint_om_omt( i.pos, z ), i.size, &lab_train_points, "ice_",
                                  lab_train_odds );
        requires_sub |= ice_lab;
        if( !ice_lab && ter( tripoint_om_omt( i.pos, z ) ) == oter_ice_lab_core ) {
            ter_set( tripoint_om_omt( i.pos, z ), oter_ice_lab.id() );
        }
    }
    for( city &i : central_lab_points ) {
        bool central_lab = build_lab( tripoint_om_omt( i.pos, z ), i.size, &central_lab_train_points,
                                      "central_", lab_train_odds );
        requires_sub |= central_lab;
        if( !central_lab && ter( tripoint_om_omt( i.pos, z ) ) == oter_central_lab_core ) {
            ter_set( tripoint_om_omt( i.pos, z ), oter_central_lab.id() );
        }
    }

    const auto create_real_train_lab_points = [this, z](
                const std::vector<point_om_omt> &train_points,
    std::vector<point_om_omt> &real_train_points ) {
        bool is_first_in_pair = true;
        for( const point_om_omt &p : train_points ) {
            tripoint_om_omt i( p, z );
            const std::vector<tripoint_om_omt> nearby_locations {
                i + point_north,
                i + point_south,
                i + point_east,
                i + point_west };
            if( is_first_in_pair ) {
                ter_set( i, oter_open_air.id() ); // mark tile to prevent subway gen

                for( const tripoint_om_omt &nearby_loc : nearby_locations ) {
                    if( is_ot_match( "empty_rock", ter( nearby_loc ), ot_match_type::contains ) ) {
                        // mark tile to prevent subway gen
                        ter_set( nearby_loc, oter_open_air.id() );
                    }
                    if( is_ot_match( "solid_earth", ter( nearby_loc ), ot_match_type::contains ) ) {
                        // mark tile to prevent subway gen
                        ter_set( nearby_loc, oter_field.id() );
                    }
                }
            } else {
                // change train connection point back to rock to allow gen
                if( is_ot_match( "open_air", ter( i ), ot_match_type::contains ) ) {
                    ter_set( i, oter_empty_rock.id() );
                }
                if( is_ot_match( "field", ter( i ), ot_match_type::contains ) ) {
                    ter_set( i, oter_solid_earth.id() );
                }
                real_train_points.push_back( i.xy() );
            }
            is_first_in_pair = !is_first_in_pair;
        }
    };
    std::vector<point_om_omt>
    subway_lab_train_points; // real points for subway, excluding train depot points
    create_real_train_lab_points( lab_train_points, subway_lab_train_points );
    create_real_train_lab_points( central_lab_train_points, subway_lab_train_points );

    subway_points.insert( subway_points.end(), subway_lab_train_points.begin(),
                          subway_lab_train_points.end() );
    connect_closest_points( subway_points, z, *overmap_connection_subway_tunnel );

    for( auto &i : subway_points ) {
        if( ( ter( tripoint_om_omt( i, z + 2 ) )->get_type_id() == oter_type_sub_station ) ) {
            ter_set( tripoint_om_omt( i, z ), oter_underground_sub_station.id() );
        }
    }

    // The first lab point is adjacent to a lab, set it a depot (as long as track was actually laid).
    const auto create_train_depots = [this, z]( const oter_id & train_type,
    const std::vector<point_om_omt> &train_points ) {
        bool is_first_in_pair = true;
        std::vector<point_om_omt> extra_route;
        for( const point_om_omt &p : train_points ) {
            tripoint_om_omt i( p, z );
            if( is_first_in_pair ) {
                const std::vector<tripoint_om_omt> subway_possible_loc {
                    i + point_north,
                    i + point_south,
                    i + point_east,
                    i + point_west };
                extra_route.clear();
                ter_set( i, oter_empty_rock.id() ); // this clears marked tiles
                bool is_depot_generated = false;
                for( const tripoint_om_omt &subway_loc : subway_possible_loc ) {
                    if( !is_depot_generated &&
                        is_ot_match( "subway", ter( subway_loc ), ot_match_type::contains ) ) {
                        extra_route.push_back( i.xy() );
                        extra_route.push_back( subway_loc.xy() );
                        connect_closest_points( extra_route, z, *overmap_connection_subway_tunnel );

                        ter_set( i, train_type );
                        is_depot_generated = true; // only one connection to depot
                    } else if( is_ot_match( "open_air", ter( subway_loc ),
                                            ot_match_type::contains ) ) {
                        // clear marked
                        ter_set( subway_loc, oter_empty_rock.id() );
                    } else if( is_ot_match( "field", ter( subway_loc ),
                                            ot_match_type::contains ) ) {
                        // clear marked
                        ter_set( subway_loc, oter_solid_earth.id() );
                    }
                }
            }
            is_first_in_pair = !is_first_in_pair;
        }
    };
    create_train_depots( oter_lab_train_depot.id(), lab_train_points );
    create_train_depots( oter_central_lab_train_depot.id(), central_lab_train_points );

    for( city &i : cities ) {
        tripoint_om_omt omt_pos( i.pos, z );
        tripoint_om_sm sm_pos = project_to<coords::sm>( omt_pos );
        tripoint_abs_sm abs_pos = project_combine( pos(), sm_pos );
        // Normal subways are present at z == -2, but filtering for the terrain would be much nicer
        if( z == -2 ) {
            spawn_mon_group( mongroup( GROUP_SUBWAY_CITY, abs_pos, i.size * i.size * 2 ),
                             i.size * 2 );
        }
    }

    return requires_sub;
}

bool overmap::generate_over( const int z )
{
    bool requires_over = false;
    std::vector<point_om_omt> bridge_points;
    std::vector<point_om_omt> railroad_bridge_points;

    // These are so common that it's worth checking first as int.
    const std::set<oter_id> skip_below = {
        oter_empty_rock.id(), oter_forest.id(), oter_field.id(),
        oter_forest_thick.id(), oter_forest_water.id(), oter_solid_earth.id()
    };

    if( z == 1 ) {
        for( int i = 0; i < OMAPX; i++ ) {
            for( int j = 0; j < OMAPY; j++ ) {
                tripoint_om_omt p( i, j, z );
                const oter_id oter_below = ter( p + tripoint_below );
                const oter_id oter_ground = ter( tripoint_om_omt( p.xy(), 0 ) );

                // implicitly skip skip_below oter_ids
                if( skip_below.find( oter_below ) != skip_below.end() ) {
                    continue;
                }

                if( oter_ground->get_type_id() == oter_type_bridge ) {
                    ter_set( p, oter_id( "bridge_road" + oter_get_rotation_string( oter_ground ) ) );
                    bridge_points.push_back( p.xy() );
                    tripoint_om_omt support_point = p + tripoint_below;
                    int support_z = 0;
                    // place the rest of the support columns
                    while( ter( support_point ) -> is_water() && --support_z >= -OVERMAP_DEPTH ) {
                        ter_set( support_point, oter_id( "bridge" + oter_get_rotation_string( oter_ground ) ) );
                        support_point += tripoint_below;
                    }
                }
                if( oter_ground->get_type_id() == oter_type_railroad_bridge ) {
                    ter_set( p, oter_id( "railroad_bridge_overpass" + oter_get_rotation_string( oter_ground ) ) );
                    railroad_bridge_points.push_back( p.xy() );
                }
            }
        }
    }

    generate_bridgeheads( bridge_points, oter_type_bridge,
                          "bridgehead_ground", "bridgehead_ramp" );
    generate_bridgeheads( railroad_bridge_points, oter_type_railroad_bridge,
                          "railroad_bridgehead_ground", "railroad_bridgehead_ramp" );

    return requires_over;
}

void overmap::generate_bridgeheads( const std::vector<point_om_omt> &bridge_points,
                                    const oter_type_str_id bridge_type,
                                    const std::string &bridgehead_ground,
                                    const std::string &bridgehead_ramp )
{
    std::vector<std::pair<point_om_omt, std::string>> bridgehead_points;
    for( const point_om_omt &bp : bridge_points ) {
        //const oter_id oter_ground = ter( tripoint_om_omt( bp, 0 ) );
        const oter_id oter_ground_north = ter( tripoint_om_omt( bp, 0 ) + tripoint_north );
        const oter_id oter_ground_south = ter( tripoint_om_omt( bp, 0 ) + tripoint_south );
        const oter_id oter_ground_east = ter( tripoint_om_omt( bp, 0 ) + tripoint_east );
        const oter_id oter_ground_west = ter( tripoint_om_omt( bp, 0 ) + tripoint_west );
        const bool is_bridge_north = ( oter_ground_north->get_type_id() == bridge_type )
                                     && ( oter_get_rotation( oter_ground_north ) % 2 == 0 );
        const bool is_bridge_south = ( oter_ground_south->get_type_id() == bridge_type )
                                     && ( oter_get_rotation( oter_ground_south ) % 2 == 0 );
        const bool is_bridge_east = ( oter_ground_east->get_type_id() == bridge_type )
                                    && ( oter_get_rotation( oter_ground_east ) % 2 == 1 );
        const bool is_bridge_west = ( oter_ground_west->get_type_id() == bridge_type )
                                    && ( oter_get_rotation( oter_ground_west ) % 2 == 1 );

        if( is_bridge_north ^ is_bridge_south || is_bridge_east ^ is_bridge_west ) {
            std::string ramp_facing;
            if( is_bridge_north ) {
                ramp_facing = "_south";
            } else if( is_bridge_south ) {
                ramp_facing = "_north";
            } else if( is_bridge_east ) {
                ramp_facing = "_west";
            } else {
                ramp_facing = "_east";
            }
            bridgehead_points.emplace_back( bp, ramp_facing );
        }
    }
    for( const std::pair<point_om_omt, std::string> &bhp : bridgehead_points ) {
        tripoint_om_omt p( bhp.first, 0 );
        ter_set( p, oter_id( bridgehead_ground + bhp.second ) );
        ter_set( p + tripoint_above, oter_id( bridgehead_ramp + bhp.second ) );
    }
}

std::vector<point_abs_omt> overmap::find_terrain( const std::string_view term, int zlevel ) const
{
    std::vector<point_abs_omt> found;
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            tripoint_om_omt p( x, y, zlevel );
            if( seen( p ) &&
                lcmatch( ter( p )->get_name(), term ) ) {
                found.push_back( project_combine( pos(), p.xy() ) );
            }
        }
    }
    return found;
}

const city &overmap::get_nearest_city( const tripoint_om_omt &p ) const
{
    int distance = 999;
    const city *res = nullptr;
    for( const city &elem : cities ) {
        const int dist = elem.get_distance_from( p );
        if( dist < distance ) {
            distance = dist;
            res = &elem;
        }
    }
    if( res != nullptr ) {
        return *res;
    }
    static city invalid_city;
    return invalid_city;
}

tripoint_om_omt overmap::find_random_omt( const std::pair<std::string, ot_match_type> &target,
        std::optional<city> target_city ) const
{
    const bool check_nearest_city = target_city.has_value();
    std::vector<tripoint_om_omt> valid;
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            for( int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++ ) {
                tripoint_om_omt p( i, j, k );
                if( is_ot_match( target.first, ter( p ), target.second ) ) {
                    if( !check_nearest_city || get_nearest_city( p ) == target_city.value() ) {
                        valid.push_back( p );
                    }
                }
            }
        }
    }
    return random_entry( valid, tripoint_om_omt( tripoint_min ) );
}

void overmap::process_mongroups()
{
    for( auto it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( mg.dying ) {
            mg.population = ( mg.population * 4 ) / 5;
        }
        if( mg.empty() ) {
            zg.erase( it++ );
        } else {
            ++it;
        }
    }
}

void overmap::clear_mon_groups()
{
    zg.clear();
}

void overmap::clear_overmap_special_placements()
{
    overmap_special_placements.clear();
}
void overmap::clear_cities()
{
    cities.clear();
}
void overmap::clear_connections_out()
{
    connections_out.clear();
}

static std::map<std::string, std::string> oter_id_migrations;

void overmap::load_oter_id_migration( const JsonObject &jo )
{
    for( const JsonMember &kv : jo.get_object( "oter_ids" ) ) {
        oter_id_migrations.emplace( kv.name(), kv.get_string() );
    }
}

void overmap::reset_oter_id_migrations()
{
    oter_id_migrations.clear();
}

bool overmap::is_oter_id_obsolete( const std::string &oterid )
{
    return oter_id_migrations.count( oterid ) > 0;
}

void overmap::migrate_oter_ids( const std::unordered_map<tripoint_om_omt, std::string> &points )
{
    for( const auto&[pos, old_id] : points ) {
        const oter_str_id new_id = oter_str_id( oter_id_migrations.at( old_id ) );
        const tripoint_abs_sm pos_abs = project_to<coords::sm>( project_combine( this->pos(), pos ) );

        if( new_id.is_valid() ) {
            DebugLog( D_WARNING, DC_ALL ) << "migrated oter_id '" << old_id << "' at " << pos_abs
                                          << " to '" << new_id.str() << "'";

            ter_set( pos, new_id );
        } else {
            debugmsg( "oter_id migration defined from '%s' to invalid ter_id '%s'", old_id, new_id.str() );
        }
    }
}

void overmap::place_special_forced( const overmap_special_id &special_id,
                                    const tripoint_om_omt &p,
                                    om_direction::type dir )
{
    static city invalid_city;
    place_special( *special_id, p, dir, invalid_city, false, true );
}

void mongroup::wander( const overmap &om )
{
    const city *target_city = nullptr;
    int target_distance = 0;

    point_om_sm rel_p = rel_pos().xy();

    if( behaviour == horde_behaviour::city ) {
        // Find a nearby city to return to..
        for( const city &check_city : om.cities ) {
            // Check if this is the nearest city so far.
            int distance = rl_dist( project_to<coords::sm>( check_city.pos ), rel_p );
            if( !target_city || distance < target_distance ) {
                target_distance = distance;
                target_city = &check_city;
            }
        }
    }

    if( target_city ) {
        // TODO: somehow use the same algorithm that distributes zombie
        // density at world gen to spread the hordes over the actual
        // city, rather than the center city tile
        point_abs_sm target_abs =
            project_to<coords::sm>( project_combine( om.pos(), target_city->pos ) );
        int range = target_city->size * 2;
        point delta( rng( -range, range ), rng( -range, range ) );
        target = target_abs + delta;
        interest = 100;
    } else {
        target = abs_pos.xy() + point( rng( -10, 10 ), rng( -10, 10 ) );
        interest = 30;
    }
}

void overmap::move_hordes()
{
    // Prevent hordes to be moved twice by putting them in here after moving.
    decltype( zg ) tmpzg;
    //MOVE ZOMBIE GROUPS
    for( auto it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( !mg.horde || mg.behaviour == mongroup::horde_behaviour::nemesis ) {
            //nemesis hordes have their own move function
            ++it;
            continue;
        }

        if( mg.behaviour == mongroup::horde_behaviour::none ) {
            mg.behaviour =
                one_in( 2 ) ? mongroup::horde_behaviour::city : mongroup::horde_behaviour::roam;
        }

        // Gradually decrease interest.
        mg.dec_interest( 1 );

        if( ( mg.abs_pos.xy() == mg.target ) || mg.interest <= 15 ) {
            mg.wander( *this );
        }

        // Decrease movement chance according to the terrain we're currently on.
        const oter_id &walked_into = ter( project_to<coords::omt>( mg.rel_pos() ) );
        int movement_chance = 1;
        if( walked_into == oter_forest || walked_into == oter_forest_water ) {
            movement_chance = 3;
        } else if( walked_into == oter_forest_thick ) {
            movement_chance = 6;
        } else if( walked_into == oter_river_center ) {
            movement_chance = 10;
        }

        // If the average horde speed is 50% that of normal, then the chance to
        // move should be 1/2 what it would be if the speed was 100%.
        // Since the max speed for a horde is one map space per 2.5 minutes,
        // choose that to be the speed of the fastest horde monster, which is
        // roughly 200 at the time of writing. So a horde with average speed
        // 200 or over will move at max speed, and slower hordes will move less
        // frequently. The average horde speed for regular Z's is around 100,
        // or one space per 5 minutes.
        if( one_in( movement_chance ) && rng( 0, 100 ) < mg.interest && rng( 0, 200 ) < mg.avg_speed() ) {
            // TODO: Handle moving to adjacent overmaps.
            if( mg.abs_pos.x() > mg.target.x() ) {
                mg.abs_pos.x()--;
            }
            if( mg.abs_pos.x() < mg.target.x() ) {
                mg.abs_pos.x()++;
            }
            if( mg.abs_pos.y() > mg.target.y() ) {
                mg.abs_pos.y()--;
            }
            if( mg.abs_pos.y() < mg.target.y() ) {
                mg.abs_pos.y()++;
            }

            // Erase the group at it's old location, add the group with the new location
            tmpzg.emplace( mg.rel_pos(), mg );
            zg.erase( it++ );
        } else {
            ++it;
        }
    }
    // and now back into the monster group map.
    zg.insert( tmpzg.begin(), tmpzg.end() );

    if( get_option<bool>( "WANDER_SPAWNS" ) ) {

        // Re-absorb zombies into hordes.
        // Scan over monsters outside the player's view and place them back into hordes.
        auto monster_map_it = monster_map.begin();
        while( monster_map_it != monster_map.end() ) {
            const tripoint_om_sm &p = monster_map_it->first;
            monster &this_monster = monster_map_it->second;

            // Only zombies on z-level 0 may join hordes.
            if( p.z() != 0 ) {
                monster_map_it++;
                continue;
            }

            // Check if the monster is a zombie.
            const mtype &type = *this_monster.type;
            if(
                !type.species.count( species_ZOMBIE ) || // Only add zombies to hordes.
                this_monster.get_speed() <= 30 || // So are very slow zombies, like crawling zombies.
                !this_monster.will_join_horde( INT_MAX ) || // So are zombies who won't join a horde of any size.
                !this_monster.mission_ids.empty() // We mustn't delete monsters that are related to missions.
            ) {
                // Don't delete the monster, just increment the iterator.
                monster_map_it++;
                continue;
            }

            // Only monsters in the open (fields, forests, roads) are eligible to wander
            const oter_id &om_here = ter( project_to<coords::omt>( p ) );
            if( !is_ot_match( "field", om_here, ot_match_type::contains ) ) {
                if( !is_ot_match( "road", om_here, ot_match_type::contains ) ) {
                    if( !is_ot_match( "forest", om_here, ot_match_type::prefix ) ) {
                        if( !is_ot_match( "swamp", om_here, ot_match_type::prefix ) ) {
                            monster_map_it++;
                            continue;
                        }
                    }
                }
            }

            // Scan for compatible hordes in this area, selecting the largest.
            mongroup *add_to_group = nullptr;
            auto group_bucket = zg.equal_range( p );
            std::vector<monster>::size_type add_to_horde_size = 0;
            std::for_each( group_bucket.first, group_bucket.second,
            [&]( std::pair<const tripoint_om_sm, mongroup> &horde_entry ) {
                mongroup &horde = horde_entry.second;

                // We only absorb zombies into GROUP_ZOMBIE hordes
                if( horde.horde && !horde.monsters.empty() && horde.type == GROUP_ZOMBIE &&
                    horde.monsters.size() > add_to_horde_size ) {
                    add_to_group = &horde;
                    add_to_horde_size = horde.monsters.size();
                }
            } );

            // Check again if the zombie will join the largest horde, now that we know the accurate size.
            if( this_monster.will_join_horde( add_to_horde_size ) ) {
                // If there is no horde to add the monster to, create one.
                if( add_to_group == nullptr ) {
                    tripoint_abs_sm abs_pos = project_combine( pos(), p );
                    mongroup m( GROUP_ZOMBIE, abs_pos, 0 );
                    m.horde = true;
                    m.monsters.push_back( this_monster );
                    m.interest = 0; // Ensures that we will select a new target.
                    add_mon_group( m );
                } else {
                    add_to_group->monsters.push_back( this_monster );
                }
            } else { // Bad luck--the zombie would have joined a larger horde, but not this one.  Skip.
                // Don't delete the monster, just increment the iterator.
                monster_map_it++;
                continue;
            }

            // Delete the monster, continue iterating.
            monster_map_it = monster_map.erase( monster_map_it );
        }
    }
}

void overmap::move_nemesis()
{
    // Prevent hordes to be moved twice by putting them in here after moving.
    decltype( zg ) tmpzg;
    //cycle through zombie groups, skip non-nemesis hordes
    for( std::multimap<tripoint_om_sm, mongroup>::iterator it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( !mg.horde || mg.behaviour != mongroup::horde_behaviour::nemesis ) {
            ++it;
            continue;
        }

        point_abs_om omp;
        tripoint_om_sm local_sm;
        std::tie( omp, local_sm ) = project_remain<coords::om>( mg.abs_pos );

        // Decrease movement chance according to the terrain we're currently on.
        // Hordes will tend toward roads / open fields and path around specials.
        const oter_id &walked_into = ter( project_to<coords::omt>( local_sm ) );
        int movement_chance = 25;
        if( is_ot_match( "road", walked_into, ot_match_type::contains ) ) {
            movement_chance = 1;
        } else if( is_ot_match( "field", walked_into, ot_match_type::contains ) ) {
            movement_chance = 3;
        } else if( is_ot_match( "forest", walked_into, ot_match_type::prefix ) ||
                   is_ot_match( "swamp", walked_into, ot_match_type::prefix ) ||
                   is_ot_match( "bridge", walked_into, ot_match_type::prefix ) ) {
            movement_chance = 6;
        } else if( is_ot_match( "river", walked_into, ot_match_type::prefix ) ) {
            movement_chance = 10;
        }

        //update the nemesis coordinates in abs_sm for movement across overmaps
        if( one_in( movement_chance ) && rng( 0, 200 ) < mg.avg_speed() ) {
            if( mg.abs_pos.x() > mg.nemesis_target.x() ) {
                mg.abs_pos.x()--;
            }
            if( mg.abs_pos.x() < mg.nemesis_target.x() ) {
                mg.abs_pos.x()++;
            }
            if( mg.abs_pos.y() > mg.nemesis_target.y() ) {
                mg.abs_pos.y()--;
            }
            if( mg.abs_pos.y() < mg.nemesis_target.y() ) {
                mg.abs_pos.y()++;
            }

            //if the nemesis horde is on the same overmap as its target
            //update the horde's om_sm coords from the abs_sm so it can spawn in correctly
            if( project_to<coords::om>( mg.nemesis_target ) == omp ) {

                // Erase the group at its old location, add the group with the new location
                tmpzg.emplace( mg.rel_pos(), mg );
                zg.erase( it++ );

                //there is only one nemesis horde, so we can stop looping after we move it
                break;

            }

            //only one nemesis, so we break after moving it
            break;

        } else {
            //and we also break if it doesnt move
            break;
        }
    }
    // and now back into the monster group map.
    zg.insert( tmpzg.begin(), tmpzg.end() );

}

bool overmap::remove_nemesis()
{
    //cycle through zombie groups, find nemesis horde
    for( std::multimap<tripoint_om_sm, mongroup>::iterator it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( mg.behaviour == mongroup::horde_behaviour::nemesis ) {
            zg.erase( it++ );
            return true;
        }
        it++;
    }
    return false;
}

/**
* @param p location of signal relative to this overmap origin
* @param sig_power - power of signal or max distance for reaction of zombies
*/
void overmap::signal_hordes( const tripoint_rel_sm &p_rel, const int sig_power )
{
    tripoint_om_sm p( p_rel.raw() );
    tripoint_abs_sm absp = project_combine( pos(), p );
    for( auto &elem : zg ) {
        mongroup &mg = elem.second;
        if( !mg.horde ) {
            continue;
        }
        const int dist = rl_dist( absp, mg.abs_pos );
        if( sig_power < dist ) {
            continue;
        }
        if( mg.behaviour == mongroup::horde_behaviour::nemesis ) {
            // nemesis hordes are signaled to the player by their own function and dont react to noise
            continue;
        }
        // TODO: base this in monster attributes, foremost GOODHEARING.
        const int inter_per_sig_power = 15; //Interest per signal value
        const int min_initial_inter = 30; //Min initial interest for horde
        const int calculated_inter = ( sig_power + 1 - dist ) * inter_per_sig_power; // Calculated interest
        const int roll = rng( 0, mg.interest );
        // Minimum capped calculated interest. Used to give horde enough interest to really investigate the target at start.
        const int min_capped_inter = std::max( min_initial_inter, calculated_inter );
        if( roll < min_capped_inter ) { //Rolling if horde interested in new signal
            // TODO: Z-coordinate for mongroup targets
            const int targ_dist = rl_dist( absp.xy(), mg.target );
            // TODO: Base this on targ_dist:dist ratio.
            if( targ_dist < 5 ) {  // If signal source already pursued by horde
                mg.set_target( midpoint( mg.target, absp.xy() ) );
                const int min_inc_inter = 3; // Min interest increase to already targeted source
                const int inc_roll = rng( min_inc_inter, calculated_inter );
                mg.inc_interest( inc_roll );
                add_msg_debug( debugmode::DF_OVERMAP, "horde inc interest %d dist %d", inc_roll, dist );
            } else { // New signal source
                mg.set_target( absp.xy() );
                mg.set_interest( min_capped_inter );
                add_msg_debug( debugmode::DF_OVERMAP, "horde set interest %d dist %d", min_capped_inter, dist );
            }
        }
    }
}

void overmap::signal_nemesis( const tripoint_abs_sm &p_abs_sm )
{
    for( std::pair<const tripoint_om_sm, mongroup> &elem : zg ) {
        mongroup &mg = elem.second;

        if( mg.behaviour == mongroup::horde_behaviour::nemesis ) {
            // if the horde is a nemesis, we set its target directly on the player
            mg.set_target( p_abs_sm.xy() );
            mg.set_nemesis_target( p_abs_sm.xy() );

        }
    }
}

void overmap::populate_connections_out_from_neighbors( const overmap *north, const overmap *east,
        const overmap *south, const overmap *west )
{
    const auto populate_for_side =
        [&]( const overmap * adjacent,
             const std::function<bool( const tripoint_om_omt & )> &should_include,
    const std::function<tripoint_om_omt( const tripoint_om_omt & )> &build_point ) {
        if( adjacent == nullptr ) {
            return;
        }

        for( const std::pair<const string_id<overmap_connection>, std::vector<tripoint_om_omt>> &kv :
             adjacent->connections_out ) {
            std::vector<tripoint_om_omt> &out = connections_out[kv.first];
            const auto adjacent_out = adjacent->connections_out.find( kv.first );
            if( adjacent_out != adjacent->connections_out.end() ) {
                for( const tripoint_om_omt &p : adjacent_out->second ) {
                    if( should_include( p ) ) {
                        out.push_back( build_point( p ) );
                    }
                }
            }
        }
    };

    populate_for_side( north, []( const tripoint_om_omt & p ) {
        return p.y() == OMAPY - 1;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( p.x(), 0, p.z() );
    } );
    populate_for_side( west, []( const tripoint_om_omt & p ) {
        return p.x() == OMAPX - 1;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( 0, p.y(), p.z() );
    } );
    populate_for_side( south, []( const tripoint_om_omt & p ) {
        return p.y() == 0;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( p.x(), OMAPY - 1, p.z() );
    } );
    populate_for_side( east, []( const tripoint_om_omt & p ) {
        return p.x() == 0;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( OMAPX - 1, p.y(), p.z() );
    } );
}

void overmap::place_forest_trails()
{
    std::unordered_set<point_om_omt> visited;

    const auto is_forest = [&]( const point_om_omt & p ) {
        if( !inbounds( p, 1 ) ) {
            return false;
        }
        const oter_id current_terrain = ter( tripoint_om_omt( p, 0 ) );
        return current_terrain == oter_forest || current_terrain == oter_forest_thick ||
               current_terrain == oter_forest_water;
    };
    const forest_trail_settings &forest_trail = settings->forest_trail;

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            tripoint_om_omt seed_point( i, j, 0 );

            oter_id oter = ter( seed_point );
            if( !is_ot_match( "forest", oter, ot_match_type::prefix ) ) {
                continue;
            }

            // If we've already visited this point, we don't need to
            // process it since it's already part of another forest.
            if( visited.find( seed_point.xy() ) != visited.end() ) {
                continue;
            }

            // Get the contiguous forest from this point.
            std::vector<point_om_omt> forest_points =
                ff::point_flood_fill_4_connected( seed_point.xy(), visited, is_forest );

            // If we don't have enough points to build a trail, move on.
            if( forest_points.empty() ||
                forest_points.size() < static_cast<std::vector<point>::size_type>
                ( forest_trail.minimum_forest_size ) ) {
                continue;
            }

            // If we don't rng a forest based on our settings, move on.
            if( !one_in( forest_trail.chance ) ) {
                continue;
            }

            // Get the north and south most points in the forest.
            auto north_south_most = std::minmax_element( forest_points.begin(),
            forest_points.end(), []( const point_om_omt & lhs, const point_om_omt & rhs ) {
                return lhs.y() < rhs.y();
            } );

            // Get the west and east most points in the forest.
            auto west_east_most = std::minmax_element( forest_points.begin(),
            forest_points.end(), []( const point_om_omt & lhs, const point_om_omt & rhs ) {
                return lhs.x() < rhs.x();
            } );

            // We'll use these points later as points that are guaranteed to be
            // at a boundary and will form a good foundation for the trail system.
            point_om_omt northmost = *north_south_most.first;
            point_om_omt southmost = *north_south_most.second;
            point_om_omt westmost = *west_east_most.first;
            point_om_omt eastmost = *west_east_most.second;

            // Do a simplistic calculation of the center of the forest (rather than
            // calculating the actual centroid--it's not that important) to have another
            // good point to form the foundation of the trail system.
            point_om_omt center( westmost.x() + ( eastmost.x() - westmost.x() ) / 2,
                                 northmost.y() + ( southmost.y() - northmost.y() ) / 2 );

            point_om_omt center_point = center;

            // Because we didn't do the centroid of a concave polygon, there's no
            // guarantee that our center point is actually within the bounds of the
            // forest. Just find the point within our set that is closest to our
            // center point and use that.
            point_om_omt actual_center_point =
                *std::min_element( forest_points.begin(), forest_points.end(),
            [&center_point]( const point_om_omt & lhs, const point_om_omt & rhs ) {
                return square_dist( lhs, center_point ) < square_dist( rhs,
                        center_point );
            } );

            // Figure out how many random points we'll add to our trail system, based on the forest
            // size and our configuration.
            int max_random_points = forest_trail.random_point_min + forest_points.size() /
                                    forest_trail.random_point_size_scalar;
            max_random_points = std::min( max_random_points, forest_trail.random_point_max );

            // Start with the center...
            std::vector<point_om_omt> chosen_points = { actual_center_point };

            // ...and then add our random points.
            int random_point_count = 0;
            std::shuffle( forest_points.begin(), forest_points.end(), rng_get_engine() );
            for( auto &random_point : forest_points ) {
                if( random_point_count >= max_random_points ) {
                    break;
                }
                random_point_count++;
                chosen_points.emplace_back( random_point );
            }

            // Add our north/south/west/east-most points based on our configuration.
            if( one_in( forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( northmost );
            }
            if( one_in( forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( southmost );
            }
            if( one_in( forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( westmost );
            }
            if( one_in( forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( eastmost );
            }

            // Finally, connect all the points and make a forest trail out of them.
            connect_closest_points( chosen_points, 0, *overmap_connection_forest_trail );
        }
    }
}

void overmap::place_forest_trailheads()
{
    // No trailheads if there are no cities.
    const int city_size = get_option<int>( "CITY_SIZE" );
    if( city_size <= 0 ) {
        return;
    }

    // Trailheads may be placed if all of the following are true:
    // 1. we're at a forest_trail_end_north/south/west/east,
    // 2. we're within trailhead_road_distance from an existing road
    // 3. rng rolls a success for our trailhead_chance from the configuration
    // 4. the trailhead special we've picked can be placed in the selected location

    const auto trailhead_close_to_road = [&]( const tripoint_om_omt & trailhead ) {
        bool close = false;
        for( const tripoint_om_omt &nearby_point : closest_points_first(
                 trailhead,
                 settings->forest_trail.trailhead_road_distance
             ) ) {
            if( check_ot( "road", ot_match_type::contains, nearby_point ) ) {
                close = true;
            }
        }
        return close;
    };

    const auto try_place_trailhead_special = [&]( const tripoint_om_omt & trail_end,
    const om_direction::type & dir ) {
        overmap_special_id trailhead = settings->forest_trail.trailheads.pick();
        if( one_in( settings->forest_trail.trailhead_chance ) &&
            trailhead_close_to_road( trail_end ) &&
            can_place_special( *trailhead, trail_end, dir, false ) ) {
            const city &nearest_city = get_nearest_city( trail_end );
            place_special( *trailhead, trail_end, dir, nearest_city, false, false );
        }
    };

    for( int i = 2; i < OMAPX - 2; i++ ) {
        for( int j = 2; j < OMAPY - 2; j++ ) {
            const tripoint_om_omt p( i, j, 0 );
            oter_id oter = ter( p );
            if( is_ot_match( "forest_trail_end", oter, ot_match_type::prefix ) ) {
                try_place_trailhead_special( p, static_cast<om_direction::type>( oter->get_rotation() ) );
            }
        }
    }
}

void overmap::place_forests()
{
    const oter_id default_oter_id( settings->default_oter[OVERMAP_DEPTH] );
    const om_noise::om_noise_layer_forest f( global_base_point(), g->get_seed() );

    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            const tripoint_om_omt p( x, y, 0 );
            const oter_id &oter = ter( p );

            // At this point in the process, we only want to consider converting the terrain into
            // a forest if it's currently the default terrain type (e.g. a field).
            if( oter != default_oter_id ) {
                continue;
            }

            const float n = f.noise_at( p.xy() );

            // If the noise here meets our threshold, turn it into a forest.
            if( n + forest_size_adjust > settings->overmap_forest.noise_threshold_forest_thick ) {
                ter_set( p, oter_forest_thick );
            } else if( n + forest_size_adjust > settings->overmap_forest.noise_threshold_forest ) {
                ter_set( p, oter_forest );
            }
        }
    }
}


void overmap::place_lakes()
{
    const om_noise::om_noise_layer_lake f( global_base_point(), g->get_seed() );

    const auto is_lake = [&]( const point_om_omt & p ) {
        // credit to ehughsbaird for thinking up this inbounds solution to infinite flood fill lag.
        bool inbounds = p.x() > -5 && p.y() > -5 && p.x() < OMAPX + 5 && p.y() < OMAPY + 5;
        if( !inbounds ) {
            return false;
        }
        return f.noise_at( p ) > settings->overmap_lake.noise_threshold_lake;
    };

    const oter_id lake_surface( "lake_surface" );
    const oter_id lake_shore( "lake_shore" );
    const oter_id lake_water_cube( "lake_water_cube" );
    const oter_id lake_bed( "lake_bed" );

    // We'll keep track of our visited lake points so we don't repeat the work.
    std::unordered_set<point_om_omt> visited;

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            point_om_omt seed_point( i, j );
            if( visited.find( seed_point ) != visited.end() ) {
                continue;
            }

            // It's a lake if it exceeds the noise threshold defined in the region settings.
            if( !is_lake( seed_point ) ) {
                continue;
            }

            // We're going to flood-fill our lake so that we can consider the entire lake when evaluating it
            // for placement, even when the lake runs off the edge of the current overmap.
            std::vector<point_om_omt> lake_points =
                ff::point_flood_fill_4_connected( seed_point, visited, is_lake );

            // If this lake doesn't exceed our minimum size threshold, then skip it. We can use this to
            // exclude the tiny lakes that don't provide interesting map features and exist mostly as a
            // noise artifact.
            if( lake_points.size() < static_cast<std::vector<point>::size_type>
                ( settings->overmap_lake.lake_size_min ) ) {
                continue;
            }

            // Build a set of "lake" points. We're actually going to combine both the lake points
            // we just found AND all of the rivers on the map, because we want our lakes to write
            // over any rivers that are placed already. Note that the assumption here is that river
            // overmap generation (e.g. place_rivers) runs BEFORE lake overmap generation.
            std::unordered_set<point_om_omt> lake_set;
            for( auto &p : lake_points ) {
                lake_set.emplace( p );
            }

            for( int x = 0; x < OMAPX; x++ ) {
                for( int y = 0; y < OMAPY; y++ ) {
                    const tripoint_om_omt p( x, y, 0 );
                    if( ter( p )->is_river() ) {
                        lake_set.emplace( p.xy() );
                    }
                }
            }

            // Iterate through all of our lake points, rejecting the ones that are out of bounds. For
            // those that are inbounds, look at the 8 adjacent locations and see if they are also part
            // of our lake points set. If they are, that means that this location is entirely surrounded
            // by lake and should be considered a lake surface. If at least one adjacent location is not
            // part of this lake points set, that means this location should be considered a lake shore.
            // Either way, make the determination and set the overmap terrain.
            for( auto &p : lake_points ) {
                if( !inbounds( p ) ) {
                    continue;
                }

                bool shore = false;
                for( int ni = -1; ni <= 1 && !shore; ni++ ) {
                    for( int nj = -1; nj <= 1 && !shore; nj++ ) {
                        const point_om_omt n = p + point( ni, nj );
                        if( lake_set.find( n ) == lake_set.end() ) {
                            shore = true;
                        }
                    }
                }

                ter_set( tripoint_om_omt( p, 0 ), shore ? lake_shore : lake_surface );

                // If this is not a shore, we'll make our subsurface lake cubes and beds.
                if( !shore ) {
                    for( int z = -1; z > settings->overmap_lake.lake_depth; z-- ) {
                        ter_set( tripoint_om_omt( p, z ), lake_water_cube );
                    }
                    ter_set( tripoint_om_omt( p, settings->overmap_lake.lake_depth ), lake_bed );
                }
            }

            // We're going to attempt to connect some points on this lake to the nearest river.
            const auto connect_lake_to_closest_river =
            [&]( const point_om_omt & lake_connection_point ) {
                int closest_distance = -1;
                point_om_omt closest_point;
                for( int x = 0; x < OMAPX; x++ ) {
                    for( int y = 0; y < OMAPY; y++ ) {
                        const tripoint_om_omt p( x, y, 0 );
                        if( !ter( p )->is_river() ) {
                            continue;
                        }
                        const int distance = square_dist( lake_connection_point, p.xy() );
                        if( distance < closest_distance || closest_distance < 0 ) {
                            closest_point = p.xy();
                            closest_distance = distance;
                        }
                    }
                }

                if( closest_distance > 0 ) {
                    place_river( closest_point, lake_connection_point );
                }
            };

            // Get the north and south most points in our lake.
            auto north_south_most = std::minmax_element( lake_points.begin(), lake_points.end(),
            []( const point_om_omt & lhs, const point_om_omt & rhs ) {
                return lhs.y() < rhs.y();
            } );

            point_om_omt northmost = *north_south_most.first;
            point_om_omt southmost = *north_south_most.second;

            // It's possible that our northmost/southmost points in the lake are not on this overmap, because our
            // lake may extend across multiple overmaps.
            if( inbounds( northmost ) ) {
                connect_lake_to_closest_river( northmost );
            }

            if( inbounds( southmost ) ) {
                connect_lake_to_closest_river( southmost );
            }
        }
    }
}

// helper function for code deduplication, as it is needed multiple times
float overmap::calculate_ocean_gradient( const point_om_omt &p, const point_abs_om this_om )
{
    const int northern_ocean = settings->overmap_ocean.ocean_start_north;
    const int eastern_ocean = settings->overmap_ocean.ocean_start_east;
    const int western_ocean = settings->overmap_ocean.ocean_start_west;
    const int southern_ocean = settings->overmap_ocean.ocean_start_south;

    float ocean_adjust_N = 0.0f;
    float ocean_adjust_E = 0.0f;
    float ocean_adjust_W = 0.0f;
    float ocean_adjust_S = 0.0f;
    if( northern_ocean > 0 && this_om.y() <= northern_ocean * -1 ) {
        ocean_adjust_N = 0.0005f * static_cast<float>( OMAPY - p.y()
                         + std::abs( ( this_om.y() + northern_ocean ) * OMAPY ) );
    }
    if( eastern_ocean > 0 && this_om.x() >= eastern_ocean ) {
        ocean_adjust_E = 0.0005f * static_cast<float>( p.x() + ( this_om.x() - eastern_ocean )
                         * OMAPX );
    }
    if( western_ocean > 0 && this_om.x() <= western_ocean * -1 ) {
        ocean_adjust_W = 0.0005f * static_cast<float>( OMAPX - p.x()
                         + std::abs( ( this_om.x() + western_ocean ) * OMAPX ) );
    }
    if( southern_ocean > 0 && this_om.y() >= southern_ocean ) {
        ocean_adjust_S = 0.0005f * static_cast<float>( p.y() + ( this_om.y() - southern_ocean ) * OMAPY );
    }
    return std::max( { ocean_adjust_N, ocean_adjust_E, ocean_adjust_W, ocean_adjust_S } );
}

void overmap::place_oceans()
{
    int northern_ocean = settings->overmap_ocean.ocean_start_north;
    int eastern_ocean = settings->overmap_ocean.ocean_start_east;
    int western_ocean = settings->overmap_ocean.ocean_start_west;
    int southern_ocean = settings->overmap_ocean.ocean_start_south;

    const om_noise::om_noise_layer_ocean f( global_base_point(), g->get_seed() );
    const point_abs_om this_om = pos();

    const auto is_ocean = [&]( const point_om_omt & p ) {
        // credit to ehughsbaird for thinking up this inbounds solution to infinite flood fill lag.
        if( northern_ocean == 0 && eastern_ocean  == 0 && western_ocean  == 0 && southern_ocean == 0 ) {
            // you know you could just turn oceans off in global_settings.json right?
            return false;
        }
        bool inbounds = p.x() > -5 && p.y() > -5 && p.x() < OMAPX + 5 && p.y() < OMAPY + 5;
        if( !inbounds ) {
            return false;
        }
        float ocean_adjust = calculate_ocean_gradient( p, this_om );
        if( ocean_adjust == 0.0f ) {
            // It's too soon!  Too soon for an ocean!!  ABORT!!!
            return false;
        }
        return f.noise_at( p ) + ocean_adjust > settings->overmap_ocean.noise_threshold_ocean;
    };

    const oter_id ocean_surface( "ocean_surface" );
    const oter_id ocean_shore( "ocean_shore" );
    const oter_id ocean_water_cube( "ocean_water_cube" );
    const oter_id ocean_bed( "ocean_bed" );

    // This code is repeated from is_lake(), see comments there for explanation.
    std::unordered_set<point_om_omt> visited;

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            point_om_omt seed_point( i, j );
            if( visited.find( seed_point ) != visited.end() ) {
                continue;
            }

            if( !is_ocean( seed_point ) ) {
                continue;
            }

            std::vector<point_om_omt> ocean_points =
                ff::point_flood_fill_4_connected( seed_point, visited, is_ocean );

            // Ocean size is checked like lake size, but minimum size is much bigger.
            // you could change this, if you want little tiny oceans all over the place.
            // I'm not sure why you'd want that.  Use place_lakes, my friend.
            if( ocean_points.size() < static_cast<std::vector<point>::size_type>
                ( settings->overmap_ocean.ocean_size_min ) ) {
                continue;
            }

            std::unordered_set<point_om_omt> ocean_set;
            for( auto &p : ocean_points ) {
                ocean_set.emplace( p );
            }

            for( int x = 0; x < OMAPX; x++ ) {
                for( int y = 0; y < OMAPY; y++ ) {
                    const tripoint_om_omt p( x, y, 0 );
                    if( ter( p )->is_river() ) {
                        ocean_set.emplace( p.xy() );
                    }
                }
            }

            for( auto &p : ocean_points ) {
                if( !inbounds( p ) ) {
                    continue;
                }

                bool shore = false;
                for( int ni = -1; ni <= 1 && !shore; ni++ ) {
                    for( int nj = -1; nj <= 1 && !shore; nj++ ) {
                        const point_om_omt n = p + point( ni, nj );
                        if( ocean_set.find( n ) == ocean_set.end() ) {
                            shore = true;
                        }
                    }
                }

                ter_set( tripoint_om_omt( p, 0 ), shore ? ocean_shore : ocean_surface );

                if( !shore ) {
                    for( int z = -1; z > settings->overmap_ocean.ocean_depth; z-- ) {
                        ter_set( tripoint_om_omt( p, z ), ocean_water_cube );
                    }
                    ter_set( tripoint_om_omt( p, settings->overmap_ocean.ocean_depth ), ocean_bed );
                }
            }

            // We're going to attempt to connect some points to the nearest river.
            // This isn't a lake but the code is the same, we can reuse it.  Water is water.
            const auto connect_lake_to_closest_river =
            [&]( const point_om_omt & lake_connection_point ) {
                int closest_distance = -1;
                point_om_omt closest_point;
                for( int x = 0; x < OMAPX; x++ ) {
                    for( int y = 0; y < OMAPY; y++ ) {
                        const tripoint_om_omt p( x, y, 0 );
                        if( !ter( p )->is_river() ) {
                            continue;
                        }
                        const int distance = square_dist( lake_connection_point, p.xy() );
                        if( distance < closest_distance || closest_distance < 0 ) {
                            closest_point = p.xy();
                            closest_distance = distance;
                        }
                    }
                }

                if( closest_distance > 0 ) {
                    place_river( closest_point, lake_connection_point );
                }
            };

            // Get the north and south most points in our ocean.
            auto north_south_most = std::minmax_element( ocean_points.begin(), ocean_points.end(),
            []( const point_om_omt & lhs, const point_om_omt & rhs ) {
                return lhs.y() < rhs.y();
            } );

            point_om_omt northmost = *north_south_most.first;
            point_om_omt southmost = *north_south_most.second;

            // It's possible that our northmost/southmost points in the lake are not on this overmap, because our
            // lake may extend across multiple overmaps.
            if( inbounds( northmost ) ) {
                connect_lake_to_closest_river( northmost );
            }

            if( inbounds( southmost ) ) {
                connect_lake_to_closest_river( southmost );
            }
        }
    }
}

void overmap::place_rivers( const overmap *north, const overmap *east, const overmap *south,
                            const overmap *west )
{
    if( settings->river_scale == 0.0 ) {
        return;
    }
    int river_chance = static_cast<int>( std::max( 1.0, 1.0 / settings->river_scale ) );
    int river_scale = static_cast<int>( std::max( 1.0, settings->river_scale ) );
    // West/North endpoints of rivers
    std::vector<point_om_omt> river_start;
    // East/South endpoints of rivers
    std::vector<point_om_omt> river_end;

    // Determine points where rivers & roads should connect w/ adjacent maps
    // optimized comparison.

    if( north != nullptr ) {
        for( int i = 2; i < OMAPX - 2; i++ ) {
            const tripoint_om_omt p_neighbour( i, OMAPY - 1, 0 );
            const tripoint_om_omt p_mine( i, 0, 0 );

            if( is_river( north->ter( p_neighbour ) ) ) {
                ter_set( p_mine, oter_river_center );
            }
            if( is_river( north->ter( p_neighbour ) ) &&
                is_river( north->ter( p_neighbour + point_east ) ) &&
                is_river( north->ter( p_neighbour + point_west ) ) ) {
                if( one_in( river_chance ) && ( river_start.empty() ||
                                                river_start[river_start.size() - 1].x() < ( i - 6 ) * river_scale ) ) {
                    river_start.push_back( p_mine.xy() );
                }
            }
        }
    }
    size_t rivers_from_north = river_start.size();
    if( west != nullptr ) {
        for( int i = 2; i < OMAPY - 2; i++ ) {
            const tripoint_om_omt p_neighbour( OMAPX - 1, i, 0 );
            const tripoint_om_omt p_mine( 0, i, 0 );

            if( is_river( west->ter( p_neighbour ) ) ) {
                ter_set( p_mine, oter_river_center );
            }
            if( is_river( west->ter( p_neighbour ) ) &&
                is_river( west->ter( p_neighbour + point_north ) ) &&
                is_river( west->ter( p_neighbour + point_south ) ) ) {
                if( one_in( river_chance ) && ( river_start.size() == rivers_from_north ||
                                                river_start[river_start.size() - 1].y() < ( i - 6 ) * river_scale ) ) {
                    river_start.push_back( p_mine.xy() );
                }
            }
        }
    }
    if( south != nullptr ) {
        for( int i = 2; i < OMAPX - 2; i++ ) {
            const tripoint_om_omt p_neighbour( i, 0, 0 );
            const tripoint_om_omt p_mine( i, OMAPY - 1, 0 );

            if( is_river( south->ter( p_neighbour ) ) ) {
                ter_set( p_mine, oter_river_center );
            }
            if( is_river( south->ter( p_neighbour ) ) &&
                is_river( south->ter( p_neighbour + point_east ) ) &&
                is_river( south->ter( p_neighbour + point_west ) ) ) {
                if( river_end.empty() ||
                    river_end[river_end.size() - 1].x() < i - 6 ) {
                    river_end.push_back( p_mine.xy() );
                }
            }
        }
    }
    size_t rivers_to_south = river_end.size();
    if( east != nullptr ) {
        for( int i = 2; i < OMAPY - 2; i++ ) {
            const tripoint_om_omt p_neighbour( 0, i, 0 );
            const tripoint_om_omt p_mine( OMAPX - 1, i, 0 );

            if( is_river( east->ter( p_neighbour ) ) ) {
                ter_set( p_mine, oter_river_center );
            }
            if( is_river( east->ter( p_neighbour ) ) &&
                is_river( east->ter( p_neighbour + point_north ) ) &&
                is_river( east->ter( p_neighbour + point_south ) ) ) {
                if( river_end.size() == rivers_to_south ||
                    river_end[river_end.size() - 1].y() < i - 6 ) {
                    river_end.push_back( p_mine.xy() );
                }
            }
        }
    }

    // Even up the start and end points of rivers. (difference of 1 is acceptable)
    // Also ensure there's at least one of each.
    std::vector<point_om_omt> new_rivers;
    if( north == nullptr || west == nullptr ) {
        while( river_start.empty() || river_start.size() + 1 < river_end.size() ) {
            new_rivers.clear();
            if( north == nullptr && one_in( river_chance ) ) {
                new_rivers.emplace_back( rng( 10, OMAPX - 11 ), 0 );
            }
            if( west == nullptr && one_in( river_chance ) ) {
                new_rivers.emplace_back( 0, rng( 10, OMAPY - 11 ) );
            }
            river_start.push_back( random_entry( new_rivers ) );
        }
    }
    if( south == nullptr || east == nullptr ) {
        while( river_end.empty() || river_end.size() + 1 < river_start.size() ) {
            new_rivers.clear();
            if( south == nullptr && one_in( river_chance ) ) {
                new_rivers.emplace_back( rng( 10, OMAPX - 11 ), OMAPY - 1 );
            }
            if( east == nullptr && one_in( river_chance ) ) {
                new_rivers.emplace_back( OMAPX - 1, rng( 10, OMAPY - 11 ) );
            }
            river_end.push_back( random_entry( new_rivers ) );
        }
    }

    // Now actually place those rivers.
    if( river_start.size() > river_end.size() && !river_end.empty() ) {
        std::vector<point_om_omt> river_end_copy = river_end;
        while( !river_start.empty() ) {
            const point_om_omt start = random_entry_removed( river_start );
            if( !river_end.empty() ) {
                place_river( start, river_end[0] );
                river_end.erase( river_end.begin() );
            } else {
                place_river( start, random_entry( river_end_copy ) );
            }
        }
    } else if( river_end.size() > river_start.size() && !river_start.empty() ) {
        std::vector<point_om_omt> river_start_copy = river_start;
        while( !river_end.empty() ) {
            const point_om_omt end = random_entry_removed( river_end );
            if( !river_start.empty() ) {
                place_river( river_start[0], end );
                river_start.erase( river_start.begin() );
            } else {
                place_river( random_entry( river_start_copy ), end );
            }
        }
    } else if( !river_end.empty() ) {
        if( river_start.size() != river_end.size() ) {
            river_start.emplace_back( rng( OMAPX / 4, ( OMAPX * 3 ) / 4 ),
                                      rng( OMAPY / 4, ( OMAPY * 3 ) / 4 ) );
        }
        for( size_t i = 0; i < river_start.size(); i++ ) {
            place_river( river_start[i], river_end[i] );
        }
    }
}

void overmap::place_swamps()
{
    // Buffer our river terrains by a variable radius and increment a counter for the location each
    // time it's included in a buffer. It's a floodplain that we'll then intersect later with some
    // noise to adjust how frequently it occurs.
    std::unique_ptr<cata::mdarray<int, point_om_omt>> floodptr =
                std::make_unique<cata::mdarray<int, point_om_omt>>( 0 );
    cata::mdarray<int, point_om_omt> &floodplain = *floodptr;
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            const tripoint_om_omt pos( x, y, 0 );
            if( is_ot_match( "river", ter_unsafe( pos ), ot_match_type::contains ) ) {
                std::vector<point_om_omt> buffered_points =
                    closest_points_first(
                        pos.xy(),
                        rng( settings->overmap_forest.river_floodplain_buffer_distance_min,
                             settings->overmap_forest.river_floodplain_buffer_distance_max ) );
                for( const point_om_omt &p : buffered_points )  {
                    if( !inbounds( p ) ) {
                        continue;
                    }
                    floodplain[p] += 1;
                }
            }
        }
    }

    // Get a layer of noise to use in conjunction with our river buffered floodplain.
    const om_noise::om_noise_layer_floodplain f( global_base_point(), g->get_seed() );

    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            const tripoint_om_omt pos( x, y, 0 );
            // If this location isn't a forest, there's nothing to do here. We'll only grow swamps in existing
            // forest terrain.
            if( !is_ot_match( "forest", ter( pos ), ot_match_type::contains ) ) {
                continue;
            }

            // If this was a part of our buffered floodplain, and the noise here meets the threshold, and the one_in rng
            // triggers, then we should flood this location and make it a swamp.
            const bool should_flood = ( floodplain[x][y] > 0 && !one_in( floodplain[x][y] ) && f.noise_at( { x, y } )
                                        > settings->overmap_forest.noise_threshold_swamp_adjacent_water );

            // If this location meets our isolated swamp threshold, regardless of floodplain values, we'll make it
            // into a swamp.
            const bool should_isolated_swamp = f.noise_at( pos.xy() ) >
                                               settings->overmap_forest.noise_threshold_swamp_isolated;
            if( should_flood || should_isolated_swamp )  {
                ter_set( pos, oter_forest_water );
            }
        }
    }
}

void overmap::place_roads( const overmap *north, const overmap *east, const overmap *south,
                           const overmap *west )
{
    int op_city_size = get_option<int>( "CITY_SIZE" );
    if( op_city_size <= 0 ) {
        return;
    }
    std::vector<tripoint_om_omt> &roads_out = connections_out[overmap_connection_local_road];

    // At least 3 exit points, to guarantee road continuity across overmaps
    if( roads_out.size() < 3 ) {

        std::array<const overmap *, 4> neighbors = { east, south, west, north };
        static constexpr std::array<point, 4> neighbor_deltas = {
            point_east, point_south, point_west, point_north
        };

        // x and y coordinates for a point on the edge in each direction
        // -1 represents a variable one dimensional coordinate along that edge
        // east == point( OMAPX - 1, n ); north == point( n, 0 );
        static constexpr std::array<int, 4> edge_coords_x = {OMAPX - 1, -1, 0, -1};
        static constexpr std::array<int, 4> edge_coords_y = {-1, OMAPY - 1, -1, 0};

        // all the points on an edge except the 10 on each end
        std::array < int, OMAPX - 20 > omap_num;
        for( int i = 0; i < OMAPX - 20; i++ ) {
            omap_num[i] = i + 10;
        }

        std::array < size_t, 4 > dirs = {0, 1, 2, 3};
        std::shuffle( dirs.begin(), dirs.end(), rng_get_engine() );

        for( size_t dir : dirs ) {
            // only potentially add a new random connection toward ungenerated overmaps
            if( neighbors[dir] == nullptr ) {
                std::shuffle( omap_num.begin(), omap_num.end(), rng_get_engine() );
                for( const int &i : omap_num ) {
                    tripoint_om_omt tmp = tripoint_om_omt(
                                              edge_coords_x[dir] >= 0 ? edge_coords_x[dir] : i,
                                              edge_coords_y[dir] >= 0 ? edge_coords_y[dir] : i,
                                              0 );
                    // Make sure these points don't conflict with rivers.
                    if( !( is_river( ter( tmp ) ) ||
                           // avoid adjacent rivers
                           // east/west of a point on the north/south edge, and vice versa
                           is_river( ter( tmp + neighbor_deltas[( dir + 1 ) % 4] ) ) ||
                           is_river( ter( tmp + neighbor_deltas[( dir + 3 ) % 4] ) ) ) ) {
                        roads_out.push_back( tmp );
                        break;
                    }
                }
                if( roads_out.size() == 3 ) {
                    break;
                }
            }
        }
    }

    std::vector<point_om_omt> road_points; // cities and roads_out together
    // Compile our master list of roads; it's less messy if roads_out is first
    road_points.reserve( roads_out.size() + cities.size() );
    for( const auto &elem : roads_out ) {
        road_points.emplace_back( elem.xy() );
    }
    for( const city &elem : cities ) {
        road_points.emplace_back( elem.pos );
    }

    // And finally connect them via roads.
    connect_closest_points( road_points, 0, *overmap_connection_local_road );
}

void overmap::place_railroads( const overmap *north, const overmap *east, const overmap *south,
                               const overmap *west )
{
    // no railroads if there are no cities
    int op_city_size = get_option<int>( "CITY_SIZE" );
    if( op_city_size <= 0 ) {
        return;
    }
    std::vector<tripoint_om_omt> &railroads_out = connections_out[overmap_connection_local_railroad];

    // At least 3 exit points, to guarantee railroad continuity across overmaps
    if( railroads_out.size() < 3 ) {

        std::array<const overmap *, 4> neighbors = { east, south, west, north };
        static constexpr std::array<point, 4> neighbor_deltas = {
            point_east, point_south, point_west, point_north
        };

        // x and y coordinates for a point on the edge in each direction
        // -1 represents a variable one dimensional coordinate along that edge
        // east == point( OMAPX - 1, n ); north == point( n, 0 );
        static constexpr std::array<int, 4> edge_coords_x = {OMAPX - 1, -1, 0, -1};
        static constexpr std::array<int, 4> edge_coords_y = {-1, OMAPY - 1, -1, 0};

        // all the points on an edge except the 10 on each end
        std::array < int, OMAPX - 20 > omap_num;
        for( int i = 0; i < OMAPX - 20; i++ ) {
            omap_num[i] = i + 10;
        }

        std::array < size_t, 4 > dirs = {0, 1, 2, 3};
        std::shuffle( dirs.begin(), dirs.end(), rng_get_engine() );

        for( size_t dir : dirs ) {
            // only potentially add a new random connection toward ungenerated overmaps
            if( neighbors[dir] == nullptr ) {
                std::shuffle( omap_num.begin(), omap_num.end(), rng_get_engine() );
                for( const int &i : omap_num ) {
                    tripoint_om_omt tmp = tripoint_om_omt(
                                              edge_coords_x[dir] >= 0 ? edge_coords_x[dir] : i,
                                              edge_coords_y[dir] >= 0 ? edge_coords_y[dir] : i,
                                              0 );
                    // Make sure these points don't conflict with rivers.
                    if( !( is_river( ter( tmp ) ) ||
                           // avoid adjacent rivers
                           // east/west of a point on the north/south edge, and vice versa
                           is_river( ter( tmp + neighbor_deltas[( dir + 1 ) % 4] ) ) ||
                           is_river( ter( tmp + neighbor_deltas[( dir + 3 ) % 4] ) ) ) ) {
                        railroads_out.push_back( tmp );
                        break;
                    }
                }
                if( railroads_out.size() == 3 ) {
                    break;
                }
            }
        }
    }

    std::vector<point_om_omt> railroad_points; // cities and railroads_out together
    // Compile our master list of railroads; it's less messy if railroads_out is first
    railroad_points.reserve( railroads_out.size() + cities.size() );
    for( const auto &elem : railroads_out ) {
        railroad_points.emplace_back( elem.xy() );
    }
    for( const city &elem : cities ) {
        // place railroads in random point around the center of the city
        railroad_points.emplace_back( random_entry(
                                          points_in_radius( tripoint_om_omt( elem.pos, 0 ), elem.size * 4 ) ).xy()
                                    );
    }

    // And finally connect them via railroads.
    connect_closest_points( railroad_points, 0, *overmap_connection_local_railroad );
}

void overmap::place_river( const point_om_omt &pa, const point_om_omt &pb )
{
    int river_chance = static_cast<int>( std::max( 1.0, 1.0 / settings->river_scale ) );
    int river_scale = static_cast<int>( std::max( 1.0, settings->river_scale ) );
    point_om_omt p2( pa );
    do {
        p2.x() += rng( -1, 1 );
        p2.y() += rng( -1, 1 );
        if( p2.x() < 0 ) {
            p2.x() = 0;
        }
        if( p2.x() > OMAPX - 1 ) {
            p2.x() = OMAPX - 1;
        }
        if( p2.y() < 0 ) {
            p2.y() = 0;
        }
        if( p2.y() > OMAPY - 1 ) {
            p2.y() = OMAPY - 1;
        }
        for( int i = -1 * river_scale; i <= 1 * river_scale; i++ ) {
            for( int j = -1 * river_scale; j <= 1 * river_scale; j++ ) {
                tripoint_om_omt p( p2 + point( j, i ), 0 );
                if( p.y() >= 0 && p.y() < OMAPY && p.x() >= 0 && p.x() < OMAPX ) {
                    if( !ter( p )->is_lake() && one_in( river_chance ) ) {
                        ter_set( p, oter_river_center );
                    }
                }
            }
        }
        if( pb.x() > p2.x() && ( rng( 0, static_cast<int>( OMAPX * 1.2 ) - 1 ) < pb.x() - p2.x() ||
                                 ( rng( 0, static_cast<int>( OMAPX * 0.2 ) - 1 ) > pb.x() - p2.x() &&
                                   rng( 0, static_cast<int>( OMAPY * 0.2 ) - 1 ) > std::abs( pb.y() - p2.y() ) ) ) ) {
            p2.x()++;
        }
        if( pb.x() < p2.x() && ( rng( 0, static_cast<int>( OMAPX * 1.2 ) - 1 ) < p2.x() - pb.x() ||
                                 ( rng( 0, static_cast<int>( OMAPX * 0.2 ) - 1 ) > p2.x() - pb.x() &&
                                   rng( 0, static_cast<int>( OMAPY * 0.2 ) - 1 ) > std::abs( pb.y() - p2.y() ) ) ) ) {
            p2.x()--;
        }
        if( pb.y() > p2.y() && ( rng( 0, static_cast<int>( OMAPY * 1.2 ) - 1 ) < pb.y() - p2.y() ||
                                 ( rng( 0, static_cast<int>( OMAPY * 0.2 ) - 1 ) > pb.y() - p2.y() &&
                                   rng( 0, static_cast<int>( OMAPX * 0.2 ) - 1 ) > std::abs( p2.x() - pb.x() ) ) ) ) {
            p2.y()++;
        }
        if( pb.y() < p2.y() && ( rng( 0, static_cast<int>( OMAPY * 1.2 ) - 1 ) < p2.y() - pb.y() ||
                                 ( rng( 0, static_cast<int>( OMAPY * 0.2 ) - 1 ) > p2.y() - pb.y() &&
                                   rng( 0, static_cast<int>( OMAPX * 0.2 ) - 1 ) > std::abs( p2.x() - pb.x() ) ) ) ) {
            p2.y()--;
        }
        p2.x() += rng( -1, 1 );
        p2.y() += rng( -1, 1 );
        if( p2.x() < 0 ) {
            p2.x() = 0;
        }
        if( p2.x() > OMAPX - 1 ) {
            p2.x() = OMAPX - 2;
        }
        if( p2.y() < 0 ) {
            p2.y() = 0;
        }
        if( p2.y() > OMAPY - 1 ) {
            p2.y() = OMAPY - 1;
        }
        for( int i = -1 * river_scale; i <= 1 * river_scale; i++ ) {
            for( int j = -1 * river_scale; j <= 1 * river_scale; j++ ) {
                // We don't want our riverbanks touching the edge of the map for many reasons
                tripoint_om_omt p( p2 + point( j, i ), 0 );
                if( inbounds( p, 1 ) ||
                    // UNLESS, of course, that's where the river is headed!
                    ( std::abs( pb.y() - p.y() ) < 4 && std::abs( pb.x() - p.x() ) < 4 ) ) {
                    if( !inbounds( p ) ) {
                        continue;
                    }

                    if( !ter( p )->is_lake() && one_in( river_chance ) ) {
                        ter_set( p, oter_river_center );
                    }
                }
            }
        }
    } while( pb != p2 );
}

void overmap::calculate_forestosity()
{
    float northern_forest_increase = get_option<float>( "OVERMAP_FOREST_INCREASE_NORTH" );
    float eastern_forest_increase = get_option<float>( "OVERMAP_FOREST_INCREASE_EAST" );
    float western_forest_increase = get_option<float>( "OVERMAP_FOREST_INCREASE_WEST" );
    float southern_forest_increase = get_option<float>( "OVERMAP_FOREST_INCREASE_SOUTH" );
    const point_abs_om this_om = pos();
    if( western_forest_increase != 0 && this_om.x() < 0 ) {
        forest_size_adjust -= this_om.x() * western_forest_increase;
    }
    if( northern_forest_increase != 0 && this_om.y() < 0 ) {
        forest_size_adjust -= this_om.y() * northern_forest_increase;
    }
    if( eastern_forest_increase != 0 && this_om.x() > 0 ) {
        forest_size_adjust += this_om.x() * eastern_forest_increase;
    }
    if( southern_forest_increase != 0 && this_om.y() > 0 ) {
        forest_size_adjust += this_om.y() * southern_forest_increase;
    }
    forestosity = forest_size_adjust * 25.0f;
    //debugmsg( "forestosity = %1.2f at OM %i, %i", forestosity, this_om.x(), this_om.y() );
    // make sure forest size never totally overwhelms the map
    forest_size_adjust = std::min( forest_size_adjust,
                                   get_option<float>( "OVERMAP_FOREST_LIMIT" ) - static_cast<float>
                                   ( settings->overmap_forest.noise_threshold_forest ) );
}

void overmap::calculate_urbanity()
{
    int op_city_size = get_option<int>( "CITY_SIZE" );
    if( op_city_size <= 0 ) {
        return;
    }
    int northern_urban_increase = get_option<int>( "OVERMAP_URBAN_INCREASE_NORTH" );
    int eastern_urban_increase = get_option<int>( "OVERMAP_URBAN_INCREASE_EAST" );
    int western_urban_increase = get_option<int>( "OVERMAP_URBAN_INCREASE_WEST" );
    int southern_urban_increase = get_option<int>( "OVERMAP_URBAN_INCREASE_SOUTH" );
    if( northern_urban_increase == 0 && eastern_urban_increase == 0 && western_urban_increase == 0 &&
        southern_urban_increase == 0 ) {
        return;
    }
    float urbanity_adj = 0.0f;

    const point_abs_om this_om = pos();
    if( northern_urban_increase != 0 && this_om.y() < 0 ) {
        urbanity_adj -= this_om.y() * northern_urban_increase / 10.0f;
        // add some falloff to the sides, keeping cities larger but breaking up the megacity a bit.
        // Doesn't apply if we expect megacity in those directions as well.
        if( this_om.x() < 0 && western_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.x() / -2.0f, 1.0f );
        }
        if( this_om.x() > 0 && eastern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.x() / 2.0f, 1.0f );
        }
    }
    if( eastern_urban_increase != 0 && this_om.x() > 0 ) {
        urbanity_adj += this_om.x() * eastern_urban_increase / 10.0f;
        if( this_om.y() < 0 && northern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.y() / -2.0f, 1.0f );
        }
        if( this_om.y() > 0 && southern_urban_increase == 0 ) {
            urbanity_adj /= std::max( this_om.y() / 2.0f, 1.0f );
        }
    }
    if( western_urban_increase != 0 && this_om.x() < 0 ) {
        urbanity_adj -= this_om.x() * western_urban_increase / 10.0f;
        if( this_om.y() < 0 && northern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.y() / -2.0f, 1.0f );
        }
        if( this_om.y() > 0 && southern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.y() / 2.0f, 1.0f );
        }
    }
    if( southern_urban_increase != 0 && this_om.y() > 0 ) {
        urbanity_adj += this_om.y() * southern_urban_increase / 10.0f;
        if( this_om.x() < 0 && western_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.x() / -2.0f, 1.0f );
        }
        if( this_om.x() > 0 && eastern_urban_increase == 0 ) {
            urbanity_adj /=  std::max( this_om.x() / 2.0f, 1.0f );
        }
    }
    urbanity = static_cast<int>( urbanity_adj );
    //debugmsg( "urbanity = %i at OM %i, %i", urbanity, this_om.x(), this_om.y() );
}

/*: the root is overmap::place_cities()
20:50 <kevingranade>: which is at overmap.cpp:1355 or so
20:51 <kevingranade>: the key is cs = rng(4, 17), setting the "size" of the city
20:51 <kevingranade>: which is roughly it's radius in overmap tiles
20:52 <kevingranade>: then later overmap::place_mongroups() is called
20:52 <kevingranade>: which creates a mongroup with radius city_size * 2.5 and population city_size * 80
20:53 <kevingranade>: tadaa

spawns happen at... <cue Clue music>
20:56 <kevingranade>: game:pawn_mon() in game.cpp:7380*/
void overmap::place_cities()
{
    int op_city_spacing = get_option<int>( "CITY_SPACING" );
    int op_city_size = get_option<int>( "CITY_SIZE" );
    int max_urbanity = get_option<int>( "OVERMAP_MAXIMUM_URBANITY" );
    if( op_city_size <= 0 ) {
        return;
    }
    // make sure city size adjust is never high enough to drop op_city_size below 2
    int city_size_adjust = std::min( urbanity -  static_cast<int>( forestosity / 2.0f ),
                                     -1 * op_city_size + 2 );
    int city_space_adjust = urbanity / 2;
    int max_city_size = std::min( op_city_size + city_size_adjust, op_city_size * max_urbanity );
    if( max_city_size < op_city_size ) {
        // funny things happen if max_city_size is less than op_city_size.
        max_city_size = op_city_size;
    }
    if( op_city_spacing > 0 ) {
        city_space_adjust = std::min( city_space_adjust, op_city_spacing - 2 );
        op_city_spacing = op_city_spacing - city_space_adjust + static_cast<int>( forestosity );
    }
    // make sure not to get too extreme on the spacing if you go way far.
    op_city_spacing = std::min( op_city_spacing, 10 );

    // spacing dictates how much of the map is covered in cities
    //   city  |  cities  |   size N cities per overmap
    // spacing | % of map |  2  |  4  |  8  |  12 |  16
    //     0   |   ~99    |2025 | 506 | 126 |  56 |  31
    //     1   |    50    |1012 | 253 |  63 |  28 |  15
    //     2   |    25    | 506 | 126 |  31 |  14 |   7
    //     3   |    12    | 253 |  63 |  15 |   7 |   3
    //     4   |     6    | 126 |  31 |   7 |   3 |   1
    //     5   |     3    |  63 |  15 |   3 |   1 |   0
    //     6   |     1    |  31 |   7 |   1 |   0 |   0
    //     7   |     0    |  15 |   3 |   0 |   0 |   0
    //     8   |     0    |   7 |   1 |   0 |   0 |   0

    const double omts_per_overmap = OMAPX * OMAPY;
    const double city_map_coverage_ratio = 1.0 / std::pow( 2.0, op_city_spacing );
    const double omts_per_city = ( op_city_size * 2 + 1 ) * ( max_city_size * 2 + 1 ) * 3 / 4.0;

    // how many cities on this overmap?
    int num_cities_on_this_overmap = 0;
    std::vector<city> cities_to_place;
    for( const city &c : city::get_all() ) {
        if( c.pos_om == pos() ) {
            num_cities_on_this_overmap++;
            cities_to_place.emplace_back( c );
        }
    }

    const bool use_random_cities = city::get_all().empty();

    // Random cities if no cities were defined in regional settings
    if( use_random_cities ) {
        num_cities_on_this_overmap = roll_remainder( omts_per_overmap * city_map_coverage_ratio /
                                     omts_per_city );
    }

    const overmap_connection &local_road( *overmap_connection_local_road );

    // if there is only a single free tile, the probability of NOT finding it after MAX_PLACEMENT_ATTEMPTS attempts
    // is (1 - 1/(OMAPX * OMAPY))^MAX_PLACEMENT_ATTEMPTS ≈ 36% for the OMAPX=OMAPY=180 and MAX_PLACEMENT_ATTEMPTS=OMAPX * OMAPY
    const int MAX_PLACEMENT_ATTEMPTS = 50;//OMAPX * OMAPY;
    int placement_attempts = 0;

    // place a seed for num_cities_on_this_overmap cities, and maybe one more
    while( cities.size() < static_cast<size_t>( num_cities_on_this_overmap ) &&
           placement_attempts < MAX_PLACEMENT_ATTEMPTS ) {
        placement_attempts++;

        tripoint_om_omt p;
        city tmp;
        tmp.pos_om = pos();
        if( use_random_cities ) {
            // randomly make some cities smaller or larger
            int size = rng( op_city_size - 1, max_city_size );
            if( one_in( 3 ) ) { // 33% tiny
                size = size * 1 / 3;
            } else if( one_in( 2 ) ) { // 33% small
                size = size * 2 / 3;
            } else if( one_in( 2 ) ) { // 17% large
                size = size * 3 / 2;
            } else {             // 17% huge
                size = size * 2;
            }
            // Ensure that cities are at least size 2, as city of size 1 is just a crossroad with no buildings at all
            size = std::max( size, 2 );
            size = std::min( size, 55 );
            // TODO: put cities closer to the edge when they can span overmaps
            // don't draw cities across the edge of the map, they will get clipped
            point_om_omt c( rng( size - 1, OMAPX - size ), rng( size - 1, OMAPY - size ) );
            p = tripoint_om_omt( c, 0 );
            if( ter( p ) == settings->default_oter[OVERMAP_DEPTH] ) {
                placement_attempts = 0;
                ter_set( p, oter_road_nesw ); // every city starts with an intersection
                tmp.pos = p.xy();
                tmp.size = size;
            }
        } else {
            placement_attempts = 0;
            tmp = random_entry( cities_to_place );
            p = tripoint_om_omt( tmp.pos, 0 );
            ter_set( tripoint_om_omt( tmp.pos, 0 ), oter_road_nesw );
        }
        if( placement_attempts == 0 ) {
            cities.push_back( tmp );
            const om_direction::type start_dir = om_direction::random();
            om_direction::type cur_dir = start_dir;

            do {
                build_city_street( local_road, tmp.pos, tmp.size, cur_dir, tmp );
            } while( ( cur_dir = om_direction::turn_right( cur_dir ) ) != start_dir );

            // Replace city's original intersection OMT with a dedicated 'city_center' OMT
            // This allows setting map extras specifically to cities (or their centers)
            ter_set( tripoint_om_omt( tmp.pos, 0 ), oter_city_center );
        }
    }
}

overmap_special_id overmap::pick_random_building_to_place( int town_dist ) const
{
    const city_settings &city_spec = settings->city_spec;
    int shop_radius = city_spec.shop_radius;
    int park_radius = city_spec.park_radius;

    int shop_sigma = city_spec.shop_sigma;
    int park_sigma = city_spec.park_sigma;

    //Normally distribute shops and parks
    //Clamp at 1/2 radius to prevent houses from spawning in the city center.
    //Parks are nearly guaranteed to have a non-zero chance of spawning anywhere in the city.
    int shop_normal = shop_radius;
    if( shop_sigma > 0 ) {
        shop_normal = std::max( shop_normal, static_cast<int>( normal_roll( shop_radius, shop_sigma ) ) );
    }
    int park_normal = park_radius;
    if( park_sigma > 0 ) {
        park_normal = std::max( park_normal, static_cast<int>( normal_roll( park_radius, park_sigma ) ) );
    }

    if( shop_normal > town_dist ) {
        return city_spec.pick_shop();
    } else if( park_normal > town_dist ) {
        return city_spec.pick_park();
    } else {
        return city_spec.pick_house();
    }
}

void overmap::place_building( const tripoint_om_omt &p, om_direction::type dir,
                              const city &town )
{
    const tripoint_om_omt building_pos = p + om_direction::displace( dir );
    const om_direction::type building_dir = om_direction::opposite( dir );

    const int town_dist = ( trig_dist( building_pos.xy(), town.pos ) * 100 ) / std::max( town.size, 1 );

    for( size_t retries = 10; retries > 0; --retries ) {
        const overmap_special_id building_tid = pick_random_building_to_place( town_dist );

        if( can_place_special( *building_tid, building_pos, building_dir, false ) ) {
            place_special( *building_tid, building_pos, building_dir, town, false, false );
            break;
        }
    }
}

void overmap::build_city_street(
    const overmap_connection &connection, const point_om_omt &p, int cs,
    om_direction::type dir, const city &town, int block_width )
{
    int c = cs;
    int croad = cs;

    if( dir == om_direction::type::invalid ) {
        debugmsg( "Invalid road direction." );
        return;
    }

    const pf::directed_path<point_om_omt> street_path = lay_out_street( connection, p, dir, cs + 1 );

    if( street_path.nodes.size() <= 1 ) {
        return; // Don't bother.
    }
    // Build the actual street.
    build_connection( connection, street_path, 0 );
    // Grow in the stated direction, sprouting off sub-roads and placing buildings as we go.
    const auto from = std::next( street_path.nodes.begin() );
    const auto to = street_path.nodes.end();

    //Alternate wide and thin blocks
    int new_width = block_width == 2 ? rng( 3, 5 ) : 2;

    for( auto iter = from; iter != to; ++iter ) {
        --c;

        const tripoint_om_omt rp( iter->pos, 0 );
        if( c >= 2 && c < croad - block_width ) {
            croad = c;
            int left = cs - rng( 1, 3 );
            int right = cs - rng( 1, 3 );

            //Remove 1 length road nubs
            if( left == 1 ) {
                left++;
            }
            if( right == 1 ) {
                right++;
            }

            build_city_street( connection, iter->pos, left, om_direction::turn_left( dir ),
                               town, new_width );

            build_city_street( connection, iter->pos, right, om_direction::turn_right( dir ),
                               town, new_width );

            const oter_id &oter = ter( rp );
            // TODO: Get rid of the hardcoded terrain ids.
            if( one_in( 2 ) && oter->get_line() == 15 && oter->type_is( oter_type_id( "road" ) ) ) {
                ter_set( rp, oter_road_nesw_manhole.id() );
            }
        }

        if( !one_in( BUILDINGCHANCE ) ) {
            place_building( rp, om_direction::turn_left( dir ), town );
        }
        if( !one_in( BUILDINGCHANCE ) ) {
            place_building( rp, om_direction::turn_right( dir ), town );
        }
    }

    // If we're big, make a right turn at the edge of town.
    // Seems to make little neighborhoods.
    cs -= rng( 1, 3 );

    if( cs >= 2 && c == 0 ) {
        const auto &last_node = street_path.nodes.back();
        const om_direction::type rnd_dir = om_direction::turn_random( dir );
        build_city_street( connection, last_node.pos, cs, rnd_dir, town );
        if( one_in( 5 ) ) {
            build_city_street( connection, last_node.pos, cs, om_direction::opposite( rnd_dir ),
                               town, new_width );
        }
    }
}

bool overmap::build_lab(
    const tripoint_om_omt &p, int s, std::vector<point_om_omt> *lab_train_points,
    const std::string &prefix, int train_odds )
{
    std::vector<tripoint_om_omt> generated_lab;
    const oter_id labt( prefix + "lab" );
    const oter_id labt_stairs( labt.id().str() + "_stairs" );
    const oter_id labt_core( labt.id().str() + "_core" );
    const oter_id labt_finale( labt.id().str() + "_finale" );
    const oter_id labt_ants( "ants_lab" );
    const oter_id labt_ants_stairs( "ants_lab_stairs" );

    ter_set( p, labt );
    generated_lab.push_back( p );

    // maintain a list of potential new lab maps
    // grows outwards from previously placed lab maps
    std::set<tripoint_om_omt> candidates;
    candidates.insert( { p + point_north, p + point_east, p + point_south, p + point_west } );
    while( !candidates.empty() ) {
        const tripoint_om_omt cand = *candidates.begin();
        candidates.erase( candidates.begin() );
        if( !inbounds( cand ) ) {
            continue;
        }
        const int dist = manhattan_dist( p.xy(), cand.xy() );
        if( dist <= s * 2 ) { // increase radius to compensate for sparser new algorithm
            int dist_increment = s > 3 ? 3 : 2; // Determines at what distance the odds of placement decreases
            if( one_in( dist / dist_increment + 1 ) ) { // odds diminish farther away from the stairs
                // make an ants lab if it's a basic lab and ants were there before.
                if( prefix.empty() && check_ot( "ants", ot_match_type::type, cand ) ) {
                    // skip over a queen's chamber.
                    if( ter( cand )->get_type_id() != oter_type_ants_queen ) {
                        ter_set( cand, labt_ants );
                    }
                } else {
                    ter_set( cand, labt );
                }
                generated_lab.push_back( cand );
                // add new candidates, don't backtrack
                for( const point &offset : four_adjacent_offsets ) {
                    const tripoint_om_omt new_cand = cand + offset;
                    const int new_dist = manhattan_dist( p.xy(), new_cand.xy() );
                    if( ter( new_cand ) != labt && new_dist > dist ) {
                        candidates.insert( new_cand );
                    }
                }
            }
        }
    }

    bool generate_stairs = true;
    for( tripoint_om_omt &elem : generated_lab ) {
        // Use a check for "_stairs" to catch the hidden_lab_stairs tiles.
        if( is_ot_match( "_stairs", ter( elem + tripoint_above ), ot_match_type::contains ) ) {
            generate_stairs = false;
        }
    }
    if( generate_stairs && !generated_lab.empty() ) {
        std::shuffle( generated_lab.begin(), generated_lab.end(), rng_get_engine() );

        // we want a spot where labs are above, but we'll settle for the last element if necessary.
        tripoint_om_omt lab_pos;
        for( tripoint_om_omt elem : generated_lab ) {
            lab_pos = elem;
            if( ter( lab_pos + tripoint_above ) == labt ) {
                break;
            }
        }
        ter_set( lab_pos + tripoint_above, labt_stairs );
    }

    ter_set( p, labt_core );
    int numstairs = 0;
    if( s > 0 ) { // Build stairs going down
        while( !one_in( 6 ) ) {
            tripoint_om_omt stair;
            int tries = 0;
            do {
                stair = p + point( rng( -s, s ), rng( -s, s ) );
                tries++;
            } while( ( ter( stair ) != labt && ter( stair ) != labt_ants ) && tries < 15 );
            if( tries < 15 ) {
                if( ter( stair ) == labt_ants ) {
                    ter_set( stair, labt_ants_stairs );
                } else {
                    ter_set( stair, labt_stairs );
                }
                numstairs++;
            }
        }
    }

    // We need a finale on the bottom of labs.  Central labs have a chance of additional finales.
    if( numstairs == 0 || ( prefix == "central_" && one_in( -p.z() - 1 ) ) ) {
        tripoint_om_omt finale;
        int tries = 0;
        do {
            finale = p + point( rng( -s, s ), rng( -s, s ) );
            tries++;
        } while( tries < 15 && ter( finale ) != labt && ter( finale ) != labt_core );
        ter_set( finale, labt_finale );
    }

    if( train_odds > 0 && one_in( train_odds ) ) {
        tripoint_om_omt train;
        int tries = 0;
        int adjacent_labs;

        do {
            train = p + point( rng( -s * 1.5 - 1, s * 1.5 + 1 ), rng( -s * 1.5 - 1, s * 1.5 + 1 ) );
            tries++;

            adjacent_labs = 0;
            for( const point &offset : four_adjacent_offsets ) {
                if( is_ot_match( "lab", ter( train + offset ), ot_match_type::contains ) &&
                    !is_ot_match( "lab_subway", ter( train + offset ), ot_match_type::contains ) ) {
                    ++adjacent_labs;
                }
            }
        } while( tries < 50 && (
                     ter( train ) == labt ||
                     ter( train ) == labt_stairs ||
                     ter( train ) == labt_finale ||
                     adjacent_labs != 1 ) );
        if( tries < 50 ) {
            lab_train_points->push_back( train.xy() ); // possible train depot
            // next is rail connection
            for( const point &offset : four_adjacent_offsets ) {
                if( is_ot_match( "lab", ter( train + offset ), ot_match_type::contains ) &&
                    !is_ot_match( "lab_subway", ter( train + offset ), ot_match_type::contains ) ) {
                    lab_train_points->push_back( train.xy() - offset );
                    break;
                }
            }
        }
    }

    // 4th story of labs is a candidate for lab escape, as long as there's no train or finale.
    if( prefix.empty() && p.z() == -4 && train_odds == 0 && numstairs > 0 ) {
        tripoint_om_omt cell;
        int tries = 0;
        int adjacent_labs = 0;

        // Find a space bordering just one lab to the south.
        do {
            cell = p + point( rng( -s * 1.5 - 1, s * 1.5 + 1 ), rng( -s * 1.5 - 1, s * 1.5 + 1 ) );
            tries++;

            adjacent_labs = 0;
            for( const point &offset : four_adjacent_offsets ) {
                if( is_ot_match( "lab", ter( cell + offset ), ot_match_type::contains ) &&
                    !is_ot_match( "lab_subway", ter( cell + offset ), ot_match_type::contains ) ) {
                    ++adjacent_labs;
                }
            }
        } while( tries < 50 && (
                     ter( cell ) == labt_stairs ||
                     ter( cell ) == labt_finale ||
                     ter( cell + point_south ) != labt ||
                     adjacent_labs != 1 ) );
        if( tries < 50 ) {
            ter_set( cell, oter_lab_escape_cells.id() );
            ter_set( cell + point_south, oter_lab_escape_entrance.id() );
        }
    }

    return numstairs > 0;
}

bool overmap::build_slimepit( const tripoint_om_omt &origin, int s )
{
    const oter_id slimepit_down( "slimepit_down" );
    const oter_id slimepit( "slimepit" );

    bool requires_sub = false;
    for( auto p : points_in_radius( origin, s + origin.z() + 1, 0 ) ) {
        int dist = square_dist( origin.xy(), p.xy() );
        if( one_in( 2 * dist ) ) {
            chip_rock( p );
            if( one_in( 8 ) && origin.z() > -OVERMAP_DEPTH ) {
                ter_set( p, slimepit_down );
                requires_sub = true;
            } else {
                ter_set( p, slimepit );
            }
        }
    }

    return requires_sub;
}

void overmap::place_ravines()
{
    if( settings->overmap_ravine.num_ravines == 0 ) {
        return;
    }

    const oter_id rift( "ravine" );
    const oter_id rift_edge( "ravine_edge" );
    const oter_id rift_floor( "ravine_floor" );
    const oter_id rift_floor_edge( "ravine_floor_edge" );

    std::set<point_om_omt> rift_points;

    // We dont really care about the paths each ravine takes, so this can be whatever
    // The random return value was chosen because it easily produces decent looking windy ravines
    const pf::two_node_scoring_fn<point_om_omt> estimate =
    [&]( pf::directed_node<point_om_omt>, const std::optional<pf::directed_node<point_om_omt>> & ) {
        return pf::node_score( 0, rng( 1, 2 ) );
    };
    // A path is generated for each of ravine, and all its constituent points are stored within the
    // rift_points set. In the code block below, the set is then used to determine edges and place the
    // actual terrain pieces of the ravine.
    for( int n = 0; n < settings->overmap_ravine.num_ravines; n++ ) {
        const point_rel_omt offset( rng( -settings->overmap_ravine.ravine_range,
                                         settings->overmap_ravine.ravine_range ),
                                    rng( -settings->overmap_ravine.ravine_range, settings->overmap_ravine.ravine_range ) );
        const point_om_omt origin( rng( 0, OMAPX ), rng( 0, OMAPY ) );
        const point_om_omt destination = origin + offset;
        if( !inbounds( destination, settings->overmap_ravine.ravine_width * 3 ) ) {
            continue;
        }
        const auto path = pf::greedy_path( origin, destination, point_om_omt( OMAPX, OMAPY ), estimate );
        for( const auto &node : path.nodes ) {
            for( int i = 1 - settings->overmap_ravine.ravine_width; i < settings->overmap_ravine.ravine_width;
                 i++ ) {
                for( int j = 1 - settings->overmap_ravine.ravine_width; j < settings->overmap_ravine.ravine_width;
                     j++ ) {
                    const point_om_omt n = node.pos + point( j, i );
                    if( inbounds( n, 1 ) ) {
                        rift_points.emplace( n );
                    }
                }
            }
        }
    }
    // We look at the 8 adjacent locations of each ravine point and see if they are also part of a
    // ravine, if at least one of them isn't, the location is part of the ravine's edge. Whatever the
    // case, the chosen ravine terrain is then propagated downwards until the ravine_depth specified
    // by the region settings.
    for( const point_om_omt &p : rift_points ) {
        bool edge = false;
        for( int ni = -1; ni <= 1 && !edge; ni++ ) {
            for( int nj = -1; nj <= 1 && !edge; nj++ ) {
                const point_om_omt n = p + point_rel_omt( ni, nj );
                if( rift_points.find( n ) == rift_points.end() || !inbounds( n ) ) {
                    edge = true;
                }
            }
        }
        for( int z = 0; z >= settings->overmap_ravine.ravine_depth; z-- ) {
            if( z == settings->overmap_ravine.ravine_depth ) {
                ter_set( tripoint_om_omt( p, z ), edge ? rift_floor_edge : rift_floor );
            } else {
                ter_set( tripoint_om_omt( p, z ), edge ? rift_edge : rift );
            }
        }
    }

}

pf::directed_path<point_om_omt> overmap::lay_out_connection(
    const overmap_connection &connection, const point_om_omt &source, const point_om_omt &dest,
    int z, const bool must_be_unexplored ) const
{
    const pf::two_node_scoring_fn<point_om_omt> estimate =
    [&]( pf::directed_node<point_om_omt> cur, std::optional<pf::directed_node<point_om_omt>> prev ) {
        const oter_id id = ter( tripoint_om_omt( cur.pos, z ) );

        const overmap_connection::subtype *subtype = connection.pick_subtype_for( id );

        if( !subtype ) {
            return pf::node_score::rejected;  // No option for this terrain.
        }

        const bool existing_connection = connection.has( id );

        // Only do this check if it needs to be unexplored and there isn't already a connection.
        if( must_be_unexplored && !existing_connection ) {
            // If this must be unexplored, check if we've already got a submap generated.
            const bool existing_submap = is_omt_generated( tripoint_om_omt( cur.pos, z ) );

            // If there is an existing submap, this area has already been explored and this
            // isn't a valid placement.
            if( existing_submap ) {
                return pf::node_score::rejected;
            }
        }

        if( existing_connection && id->is_rotatable() && cur.dir != om_direction::type::invalid &&
            !om_direction::are_parallel( id->get_dir(), cur.dir ) ) {
            return pf::node_score::rejected; // Can't intersect.
        }

        if( prev && prev->dir != om_direction::type::invalid && prev->dir != cur.dir ) {
            // Direction has changed.
            const oter_id &prev_id = ter( tripoint_om_omt( prev->pos, z ) );
            const overmap_connection::subtype *prev_subtype = connection.pick_subtype_for( prev_id );

            if( !prev_subtype || !prev_subtype->allows_turns() ) {
                return pf::node_score::rejected;
            }
        }

        const int dist = subtype->is_orthogonal() ?
                         manhattan_dist( dest, cur.pos ) :
                         trig_dist( dest, cur.pos );
        const int existency_mult = existing_connection ? 1 : 5; // Prefer existing connections.

        return pf::node_score( subtype->basic_cost, existency_mult * dist );
    };

    return pf::greedy_path( source, dest, point_om_omt( OMAPX, OMAPY ), estimate );
}

static pf::directed_path<point_om_omt> straight_path( const point_om_omt &source,
        om_direction::type dir, size_t len )
{
    pf::directed_path<point_om_omt> res;
    if( len == 0 ) {
        return res;
    }
    point_om_omt p = source;
    res.nodes.reserve( len );
    for( size_t i = 0; i + 1 < len; ++i ) {
        res.nodes.emplace_back( p, dir );
        p += om_direction::displace( dir );
    }
    res.nodes.emplace_back( p, om_direction::type::invalid );
    return res;
}

pf::directed_path<point_om_omt> overmap::lay_out_street( const overmap_connection &connection,
        const point_om_omt &source, om_direction::type dir, size_t len ) const
{
    const tripoint_om_omt from( source, 0 );
    // See if we need to make another one "step" further.
    const tripoint_om_omt en_pos = from + om_direction::displace( dir, len + 1 );
    if( inbounds( en_pos, 1 ) && connection.has( ter( en_pos ) ) ) {
        ++len;
    }

    size_t actual_len = 0;

    while( actual_len < len ) {
        const tripoint_om_omt pos = from + om_direction::displace( dir, actual_len );

        if( !inbounds( pos, 1 ) ) {
            break;  // Don't approach overmap bounds.
        }

        const oter_id &ter_id = ter( pos );

        if( ter_id->is_river() || ter_id->is_ravine() || ter_id->is_ravine_edge() ||
            !connection.pick_subtype_for( ter_id ) ) {
            break;
        }

        bool collided = false;
        int collisions = 0;
        for( int i = -1; i <= 1; i++ ) {
            if( collided ) {
                break;
            }
            for( int j = -1; j <= 1; j++ ) {
                const tripoint_om_omt checkp = pos + tripoint( i, j, 0 );

                if( checkp != pos + om_direction::displace( dir, 1 ) &&
                    checkp != pos + om_direction::displace( om_direction::opposite( dir ), 1 ) &&
                    checkp != pos ) {
                    if( ter( checkp )->get_type_id() == oter_type_road ) {
                        collisions++;
                    }
                }
            }

            //Stop roads from running right next to each other
            if( collisions >= 3 ) {
                collided = true;
                break;
            }
        }
        if( collided ) {
            break;
        }

        ++actual_len;

        if( actual_len > 1 && connection.has( ter_id ) ) {
            break;  // Stop here.
        }
    }

    return straight_path( source, dir, actual_len );
}

void overmap::build_connection(
    const overmap_connection &connection, const pf::directed_path<point_om_omt> &path, int z,
    const cube_direction initial_dir )
{
    if( path.nodes.empty() ) {
        return;
    }

    om_direction::type prev_dir =
        om_direction::from_cube( initial_dir, "Up and down connections not yet supported" );

    const pf::directed_node<point_om_omt> start = path.nodes.front();
    const pf::directed_node<point_om_omt> end = path.nodes.back();

    for( const auto &node : path.nodes ) {
        const tripoint_om_omt pos( node.pos, z );
        const oter_id &ter_id = ter( pos );
        const om_direction::type new_dir = node.dir;
        const overmap_connection::subtype *subtype = connection.pick_subtype_for( ter_id );

        if( !subtype ) {
            debugmsg( "No suitable subtype of connection \"%s\" found for \"%s\".", connection.id.c_str(),
                      ter_id.id().c_str() );
            return;
        }

        if( subtype->terrain->is_linear() ) {
            size_t new_line = connection.has( ter_id ) ? ter_id->get_line() : 0;

            if( new_dir != om_direction::type::invalid ) {
                new_line = om_lines::set_segment( new_line, new_dir );
            }

            if( prev_dir != om_direction::type::invalid ) {
                new_line = om_lines::set_segment( new_line, om_direction::opposite( prev_dir ) );
            }

            for( const om_direction::type dir : om_direction::all ) {
                const tripoint_om_omt np( pos + om_direction::displace( dir ) );

                if( inbounds( np ) ) {
                    const oter_id &near_id = ter( np );

                    if( connection.has( near_id ) ) {
                        if( near_id->is_linear() ) {
                            const size_t near_line = near_id->get_line();

                            if( om_lines::is_straight( near_line ) || om_lines::has_segment( near_line, new_dir ) ) {
                                // Mutual connection.
                                const size_t new_near_line = om_lines::set_segment( near_line, om_direction::opposite( dir ) );
                                ter_set( np, near_id->get_type_id()->get_linear( new_near_line ) );
                                new_line = om_lines::set_segment( new_line, dir );
                            }
                        } else if( near_id->is_rotatable() && om_direction::are_parallel( dir, near_id->get_dir() ) ) {
                            new_line = om_lines::set_segment( new_line, dir );
                        }
                    }
                } else if( pos.xy() == start.pos || pos.xy() == end.pos ) {
                    // Only automatically connect to out of bounds locations if we're the start or end of this path.
                    new_line = om_lines::set_segment( new_line, dir );

                    // Add this connection point to our connections out.
                    std::vector<tripoint_om_omt> &outs = connections_out[connection.id];
                    const auto existing_out = std::find_if( outs.begin(),
                    outs.end(), [pos]( const tripoint_om_omt & c ) {
                        return c == pos;
                    } );
                    if( existing_out == outs.end() ) {
                        outs.emplace_back( pos );
                    }
                }
            }

            if( new_line == om_lines::invalid ) {
                debugmsg( "Invalid path for connection \"%s\".", connection.id.c_str() );
                return;
            }

            ter_set( pos, subtype->terrain->get_linear( new_line ) );
        } else if( new_dir != om_direction::type::invalid ) {
            ter_set( pos, subtype->terrain->get_rotated( new_dir ) );
        }

        prev_dir = new_dir;
    }
}

void overmap::build_connection(
    const point_om_omt &source, const point_om_omt &dest, int z,
    const overmap_connection &connection, const bool must_be_unexplored,
    const cube_direction initial_dir )
{
    build_connection(
        connection, lay_out_connection( connection, source, dest, z, must_be_unexplored ),
        z, initial_dir );
}

// connect the points to each other using a minimum spanning tree
// plus rare extra connections to make loops
void overmap::connect_closest_points( const std::vector<point_om_omt> &points, int z,
                                      const overmap_connection &connection )
{
    // { weight, {i, j} }
    using edge = std::pair<float, std::pair<size_t, size_t>>;

    if( points.size() < 2 ) {
        return;
    }

    std::vector<edge> edges;

    // enumerate every possible connection between the points
    edges.reserve( points.size() * ( points.size() - 1 ) / 2 );
    for( size_t i = 0; i < points.size() - 1; ++i ) {
        for( size_t j = i + 1; j < points.size(); j++ ) {
            const float distance = trig_dist( points[i], points[j] );
            edges.push_back( {distance, {i, j}} );
        }
    }

    // sort from shortest to longest
    sort( edges.begin(), edges.end() );

    // track which subgraph each point belongs to
    std::vector<int> subgraphs( points.size(), -1 );

    for( edge candidate : edges ) {
        const size_t i = candidate.second.first;
        const size_t j = candidate.second.second;
        bool connect = false;
        if( subgraphs[i] < 0 && subgraphs[j] < 0 ) {
            // neither point of this connection has been connected yet
            // identify them each as the root of a new subgraph
            subgraphs[i] = i;
            subgraphs[j] = j;
            // and connect them
            connect = true;
        } else if( subgraphs[i] < 0 ) {
            // j is already connected to something
            // add i to j's subgraph
            subgraphs[i] = subgraphs[j];
            // and connect them
            connect = true;
        } else if( subgraphs[j] < 0 ) {
            // i is already connected to something
            // add j to i's subgraph
            subgraphs[j] = subgraphs[i];
            // and connect them
            connect = true;
        } else if( subgraphs[i] != subgraphs[j] ) {
            // i and j are both connected to different subgraphs
            // join the subgraphs
            int dead_subtree = subgraphs[j];
            for( size_t k = 0; k < points.size(); k++ ) {
                if( subgraphs[k] == dead_subtree ) {
                    subgraphs[k] = subgraphs[i];
                }
            }
            // and connect them
            connect = true;
        } else if( one_in( 10 ) ) {
            // the remaining case is that i and j are already in the same subgraph
            // making this connection creates a loop
            connect = true;
        }

        if( connect ) {
            build_connection( points[i], points[j], z, connection, false );
        }
    }
}

void overmap::polish_river()
{
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            good_river( { x, y, 0 } );
        }
    }
}

// Changes neighboring empty rock to partial rock
void overmap::chip_rock( const tripoint_om_omt &p )
{
    const oter_id rock( "rock" );
    const oter_id empty_rock( "empty_rock" );

    for( const point &offset : four_adjacent_offsets ) {
        if( ter( p + offset ) == empty_rock ) {
            ter_set( p + offset, rock );
        }
    }
}

bool overmap::check_ot( const std::string &otype, ot_match_type match_type,
                        const tripoint_om_omt &p ) const
{
    /// TODO: this check should be done by the caller. Probably.
    if( !inbounds( p ) ) {
        return false;
    }
    const oter_id &oter = ter( p );
    return is_ot_match( otype, oter, match_type );
}

bool overmap::check_overmap_special_type( const overmap_special_id &id,
        const tripoint_om_omt &location ) const
{
    // Try and find the special associated with this location.
    auto found_id = overmap_special_placements.find( location );

    // There was no special here, so bail.
    if( found_id == overmap_special_placements.end() ) {
        return false;
    }

    // Return whether the found special was a match with our requested id.
    return found_id->second == id;
}

std::optional<overmap_special_id> overmap::overmap_special_at( const tripoint_om_omt &p ) const
{
    auto it = overmap_special_placements.find( p );

    if( it == overmap_special_placements.end() ) {
        return std::nullopt;
    }

    return it->second;
}

void overmap::good_river( const tripoint_om_omt &p )
{
    if( !is_ot_match( "river", ter( p ), ot_match_type::prefix ) ) {
        return;
    }
    if( ( p.x() == 0 ) || ( p.x() == OMAPX - 1 ) ) {
        if( !is_water_body( ter( p + point_north ) ) ) {
            ter_set( p, oter_river_north.id() );
        } else if( !is_water_body( ter( p + point_south ) ) ) {
            ter_set( p, oter_river_south.id() );
        } else {
            ter_set( p, oter_river_center.id() );
        }
        return;
    }
    if( ( p.y() == 0 ) || ( p.y() == OMAPY - 1 ) ) {
        if( !is_water_body( ter( p + point_west ) ) ) {
            ter_set( p, oter_river_west.id() );
        } else if( !is_water_body( ter( p + point_east ) ) ) {
            ter_set( p, oter_river_east.id() );
        } else {
            ter_set( p, oter_river_center.id() );
        }
        return;
    }
    if( is_water_body( ter( p + point_west ) ) ) {
        if( is_water_body( ter( p + point_north ) ) ) {
            if( is_water_body( ter( p + point_south ) ) ) {
                if( is_water_body( ter( p + point_east ) ) ) {
                    // River on N, S, E, W;
                    // but we might need to take a "bite" out of the corner
                    if( !is_water_body( ter( p + point_north_west ) ) ) {
                        ter_set( p, oter_river_c_not_nw.id() );
                    } else if( !is_water_body( ter( p + point_north_east ) ) ) {
                        ter_set( p, oter_river_c_not_ne.id() );
                    } else if( !is_water_body( ter( p + point_south_west ) ) ) {
                        ter_set( p, oter_river_c_not_sw.id() );
                    } else if( !is_water_body( ter( p + point_south_east ) ) ) {
                        ter_set( p, oter_river_c_not_se.id() );
                    } else {
                        ter_set( p, oter_river_center.id() );
                    }
                } else {
                    ter_set( p, oter_river_east.id() );
                }
            } else {
                if( is_water_body( ter( p + point_east ) ) ) {
                    ter_set( p, oter_river_south.id() );
                } else {
                    ter_set( p, oter_river_se.id() );
                }
            }
        } else {
            if( is_water_body( ter( p + point_south ) ) ) {
                if( is_water_body( ter( p + point_east ) ) ) {
                    ter_set( p, oter_river_north.id() );
                } else {
                    ter_set( p, oter_river_ne.id() );
                }
            } else {
                if( is_water_body( ter( p + point_east ) ) ) { // Means it's swampy
                    ter_set( p, oter_forest_water.id() );
                }
            }
        }
    } else {
        if( is_water_body( ter( p + point_north ) ) ) {
            if( is_water_body( ter( p + point_south ) ) ) {
                if( is_water_body( ter( p + point_east ) ) ) {
                    ter_set( p, oter_river_west.id() );
                } else { // Should never happen
                    ter_set( p, oter_forest_water.id() );
                }
            } else {
                if( is_water_body( ter( p + point_east ) ) ) {
                    ter_set( p, oter_river_sw.id() );
                } else { // Should never happen
                    ter_set( p, oter_forest_water.id() );
                }
            }
        } else {
            if( is_water_body( ter( p + point_south ) ) ) {
                if( is_water_body( ter( p + point_east ) ) ) {
                    ter_set( p, oter_river_nw.id() );
                } else { // Should never happen
                    ter_set( p, oter_forest_water.id() );
                }
            } else { // Should never happen
                ter_set( p, oter_forest_water.id() );
            }
        }
    }
}

std::string om_direction::name( type dir )
{
    static const std::array < std::string, size + 1 > names = {{
            translate_marker( "invalid" ), translate_marker( "north" ),
            translate_marker( "east" ), translate_marker( "south" ),
            translate_marker( "west" )
        }
    };
    return _( names[static_cast<size_t>( dir ) + 1] );
}

// new x = (x-c.x)*cos() - (y-c.y)*sin() + c.x
// new y = (x-c.x)*sin() + (y-c.y)*cos() + c.y
// r1x = 0*x - 1*y = -1*y, r1y = 1*x + y*0 = x
// r2x = -1*x - 0*y = -1*x , r2y = x*0 + y*-1 = -1*y
// r3x = x*0 - (-1*y) = y, r3y = x*-1 + y*0 = -1*x
// c=0,0, rot90 = (-y, x); rot180 = (-x, y); rot270 = (y, -x)
/*
    (0,0)(1,0)(2,0) 90 (0,0)(0,1)(0,2)       (-2,0)(-1,0)(0,0)
    (0,1)(1,1)(2,1) -> (-1,0)(-1,1)(-1,2) -> (-2,1)(-1,1)(0,1)
    (0,2)(1,2)(2,2)    (-2,0)(-2,1)(-2,2)    (-2,2)(-1,2)(0,2)
*/

point om_direction::rotate( const point &p, type dir )
{
    switch( dir ) {
        case om_direction::type::invalid:
        case om_direction::type::last:
            debugmsg( "Invalid overmap rotation (%d).", static_cast<int>( dir ) );
        // Intentional fallthrough.
        case om_direction::type::north:
            break;  // No need to do anything.
        case om_direction::type::east:
            return point( -p.y, p.x );
        case om_direction::type::south:
            return point( -p.x, -p.y );
        case om_direction::type::west:
            return point( p.y, -p.x );
    }
    return p;
}

tripoint om_direction::rotate( const tripoint &p, type dir )
{
    return tripoint( rotate( { p.xy() }, dir ), p.z );
}

uint32_t om_direction::rotate_symbol( uint32_t sym, type dir )
{
    return rotatable_symbols::get( sym, static_cast<int>( dir ) );
}

point om_direction::displace( type dir, int dist )
{
    return rotate( { 0, -dist }, dir );
}

static om_direction::type rotate_internal( om_direction::type dir, int step )
{
    if( dir == om_direction::type::invalid ) {
        debugmsg( "Can't rotate an invalid overmap rotation." );
        return dir;
    }
    step = step % om_direction::size;
    return static_cast<om_direction::type>( ( static_cast<int>( dir ) + step ) % om_direction::size );
}

om_direction::type om_direction::add( type dir1, type dir2 )
{
    return rotate_internal( dir1, static_cast<int>( dir2 ) );
}

om_direction::type om_direction::turn_left( type dir )
{
    return rotate_internal( dir, -static_cast<int>( size ) / 4 );
}

om_direction::type om_direction::turn_right( type dir )
{
    return rotate_internal( dir, static_cast<int>( size ) / 4 );
}

om_direction::type om_direction::turn_random( type dir )
{
    return rng( 0, 1 ) ? turn_left( dir ) : turn_right( dir );
}

om_direction::type om_direction::opposite( type dir )
{
    return rotate_internal( dir, static_cast<int>( size ) / 2 );
}

om_direction::type om_direction::random()
{
    return static_cast<type>( rng( 0, size - 1 ) );
}

bool om_direction::are_parallel( type dir1, type dir2 )
{
    return dir1 == dir2 || dir1 == opposite( dir2 );
}

om_direction::type om_direction::from_cube( cube_direction c, const std::string &error_msg )
{
    switch( c ) {
        case cube_direction::north:
            return om_direction::type::north;
        case cube_direction::east:
            return om_direction::type::east;
        case cube_direction::south:
            return om_direction::type::south;
        case cube_direction::west:
            return om_direction::type::west;
        case cube_direction::above:
        case cube_direction::below:
            debugmsg( error_msg );
        // fallthrough
        case cube_direction::last:
            return om_direction::type::invalid;
    }
    debugmsg( "Invalid cube_direction" );
    return om_direction::type::invalid;
}

om_direction::type overmap::random_special_rotation( const overmap_special &special,
        const tripoint_om_omt &p, const bool must_be_unexplored ) const
{
    std::vector<om_direction::type> rotations( om_direction::size );
    const auto first = rotations.begin();
    auto last = first;

    int top_score = 0; // Maximal number of existing connections (roads).
    // Try to find the most suitable rotation: satisfy as many connections
    // as possible with the existing terrain.
    for( om_direction::type r : om_direction::all ) {
        int score = special.score_rotation_at( *this, p, r );

        if( score >= top_score ) {
            if( score > top_score ) {
                top_score = score;
                last = first; // New top score. Forget previous rotations.
            }
            *last = r;
            ++last;
        }

        if( !special.is_rotatable() ) {
            break;
        }
    }
    // Pick first valid rotation at random.
    std::shuffle( first, last, rng_get_engine() );
    const auto rotation = std::find_if( first, last, [&]( om_direction::type elem ) {
        return can_place_special( special, p, elem, must_be_unexplored );
    } );

    return rotation != last ? *rotation : om_direction::type::invalid;
}

bool overmap::can_place_special( const overmap_special &special, const tripoint_om_omt &p,
                                 om_direction::type dir, const bool must_be_unexplored ) const
{
    cata_assert( dir != om_direction::type::invalid );

    if( !special.id ) {
        return false;
    }
    if( special.has_flag( "GLOBALLY_UNIQUE" ) &&
        overmap_buffer.contains_unique_special( special.id ) ) {
        return false;
    }

    if( special.has_eoc() ) {
        dialogue d( get_talker_for( get_avatar() ), nullptr );
        if( !special.get_eoc()->test_condition( d ) ) {
            return false;
        }
    }

    const std::vector<overmap_special_locations> fixed_terrains = special.required_locations();

    return std::all_of( fixed_terrains.begin(), fixed_terrains.end(),
    [&]( const overmap_special_locations & elem ) {
        const tripoint_om_omt rp = p + om_direction::rotate( elem.p, dir );

        if( !inbounds( rp, 1 ) ) {
            return false;
        }

        if( must_be_unexplored ) {
            // If this must be unexplored, check if we've already got a submap generated.
            const bool existing_submap = is_omt_generated( rp );

            // If there is an existing submap, this area has already been explored and this
            // isn't a valid placement.
            if( existing_submap ) {
                return false;
            }
        }

        const oter_id &tid = ter( rp );

        return elem.can_be_placed_on( tid ) || ( rp.z() != 0 && tid == get_default_terrain( rp.z() ) );
    } );
}

// checks around the selected point to see if the special can be placed there
std::vector<tripoint_om_omt> overmap::place_special(
    const overmap_special &special, const tripoint_om_omt &p, om_direction::type dir,
    const city &cit, const bool must_be_unexplored, const bool force )
{
    cata_assert( dir != om_direction::type::invalid );
    if( !force ) {
        cata_assert( can_place_special( special, p, dir, must_be_unexplored ) );
    }
    if( special.has_flag( "GLOBALLY_UNIQUE" ) ) {
        overmap_buffer.add_unique_special( special.id );
    }

    const bool is_safe_zone = special.has_flag( "SAFE_AT_WORLDGEN" );

    std::optional<mapgen_arguments> *mapgen_args_p = &*mapgen_arg_storage.emplace();
    special_placement_result result = special.place( *this, p, dir, cit, must_be_unexplored );
    for( const std::pair<om_pos_dir, std::string> &join : result.joins_used ) {
        joins_used[join.first] = join.second;
    }
    for( const tripoint_om_omt &location : result.omts_used ) {
        overmap_special_placements[location] = special.id;
        mapgen_args_index[location] = mapgen_args_p;
        if( is_safe_zone ) {
            safe_at_worldgen.emplace( location );
        }
    }
    // Place spawns.
    const overmap_special_spawns &spawns = special.get_monster_spawns();
    if( spawns.group ) {
        const int pop = rng( spawns.population.min, spawns.population.max );
        const int rad = rng( spawns.radius.min, spawns.radius.max );
        spawn_mon_group(
            mongroup( spawns.group, project_to<coords::sm>( project_combine( pos(), p ) ), pop ),
            rad );
    }
    // If it's a safe zone, remove existing spawns
    if( is_safe_zone ) {
        for( auto it = zg.begin(); it != zg.end(); ) {
            tripoint_om_omt pos = project_to<coords::omt>( it->second.rel_pos() );
            if( safe_at_worldgen.find( pos ) != safe_at_worldgen.end() ) {
                zg.erase( it++ );
            } else {
                ++it;
            }
        }
    }

    return result.omts_used;
}

om_special_sectors get_sectors( const int sector_width )
{
    std::vector<point_om_omt> res;

    res.reserve( static_cast<size_t>( OMAPX / sector_width ) * static_cast<size_t>
                 ( OMAPY / sector_width ) );
    for( int x = 0; x < OMAPX; x += sector_width ) {
        for( int y = 0; y < OMAPY; y += sector_width ) {
            res.emplace_back( x, y );
        }
    }
    std::shuffle( res.begin(), res.end(), rng_get_engine() );
    return om_special_sectors{ res, sector_width };
}

bool overmap::place_special_attempt(
    overmap_special_batch &enabled_specials, const point_om_omt &sector,
    const int sector_width, const bool place_optional, const bool must_be_unexplored )
{
    const point_om_omt p2( sector );

    const tripoint_om_omt p( rng( p2.x(), p2.x() + sector_width - 1 ),
                             rng( p2.y(), p2.y() + sector_width - 1 ), 0 );
    const city &nearest_city = get_nearest_city( p );

    std::shuffle( enabled_specials.begin(), enabled_specials.end(), rng_get_engine() );
    std::set<int> priorities;
    for( const overmap_special_placement &os : enabled_specials ) {
        priorities.emplace( os.special_details->get_priority() );
    }
    for( auto pri_iter = priorities.rbegin(); pri_iter != priorities.rend(); ++pri_iter ) {
        for( auto iter = enabled_specials.begin(); iter != enabled_specials.end(); ++iter ) {
            const overmap_special &special = *iter->special_details;
            if( *pri_iter != special.get_priority() ) {
                continue;
            }
            const overmap_special_placement_constraints &constraints = special.get_constraints();
            // If we haven't finished placing minimum instances of all specials,
            // skip specials that are at their minimum count already.
            if( !place_optional && iter->instances_placed >= constraints.occurrences.min ) {
                continue;
            }
            // City check is the fastest => it goes first.
            if( !special.can_belong_to_city( p, nearest_city ) ) {
                continue;
            }
            // See if we can actually place the special there.
            const om_direction::type rotation = random_special_rotation( special, p, must_be_unexplored );
            if( rotation == om_direction::type::invalid ) {
                continue;
            }

            place_special( special, p, rotation, nearest_city, false, must_be_unexplored );

            if( ++iter->instances_placed >= constraints.occurrences.max ) {
                enabled_specials.erase( iter );
            }

            return true;
        }
    }

    return false;
}

void overmap::place_specials_pass(
    overmap_special_batch &enabled_specials, om_special_sectors &sectors,
    const bool place_optional, const bool must_be_unexplored )
{
    // Walk over sectors in random order, to minimize "clumping".
    std::shuffle( sectors.sectors.begin(), sectors.sectors.end(), rng_get_engine() );
    for( auto it = sectors.sectors.begin(); it != sectors.sectors.end(); ) {
        const size_t attempts = 10;
        bool placed = false;
        for( size_t i = 0; i < attempts; ++i ) {
            if( place_special_attempt( enabled_specials, *it, sectors.sector_width, place_optional,
                                       must_be_unexplored ) ) {
                placed = true;
                it = sectors.sectors.erase( it );
                if( enabled_specials.empty() ) {
                    return; // Job done. Bail out.
                }
                break;
            }
        }

        if( !placed ) {
            it++;
        }
    }
}

// Split map into sections, iterate through sections iterate through specials,
// check if special is valid  pick & place special.
// When a sector is populated it's removed from the list,
// and when a special reaches max instances it is also removed.
void overmap::place_specials( overmap_special_batch &enabled_specials )
{
    // Calculate if this overmap has any lake terrain--if it doesn't, we should just
    // completely skip placing any lake specials here since they'll never place and if
    // they're mandatory they just end up causing us to spiral out into adjacent overmaps
    // which probably don't have lakes either.
    bool overmap_has_lake = false;
    bool overmap_has_ocean = false;
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT && !overmap_has_lake; z++ ) {
        for( int x = 0; x < OMAPX && !overmap_has_lake; x++ ) {
            for( int y = 0; y < OMAPY && !overmap_has_lake; y++ ) {
                overmap_has_lake = ter_unsafe( { x, y, z } )->is_lake();
            }
        }
    }
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT && !overmap_has_ocean; z++ ) {
        for( int x = 0; x < OMAPX && !overmap_has_ocean; x++ ) {
            for( int y = 0; y < OMAPY && !overmap_has_ocean; y++ ) {
                overmap_has_ocean = ter_unsafe( { x, y, z } )->is_ocean();
            }
        }
    }

    for( auto iter = enabled_specials.begin(); iter != enabled_specials.end(); ) {
        // If this special has the LAKE flag and the overmap doesn't have any
        // lake terrain, then remove this special from the candidates for this
        // overmap.
        if( iter->special_details->has_flag( "LAKE" ) && !overmap_has_lake ) {
            iter = enabled_specials.erase( iter );
            continue;
        }
        if( iter->special_details->has_flag( "OCEAN" ) && !overmap_has_ocean ) {
            iter = enabled_specials.erase( iter );
            continue;
        }

        const bool unique = iter->special_details->has_flag( "UNIQUE" );
        const bool globally_unique = iter->special_details->has_flag( "GLOBALLY_UNIQUE" );
        if( unique || globally_unique ) {
            const overmap_special_id &id = iter->special_details->id;
            const overmap_special_placement_constraints &constraints = iter->special_details->get_constraints();
            const int min = constraints.occurrences.min;
            const int max = constraints.occurrences.max;

            if( x_in_y( min, max ) && ( !globally_unique || !overmap_buffer.contains_unique_special( id ) ) ) {
                // Min and max are overloaded to be the chance of occurrence,
                // so reset instances placed to one short of max so we don't place several.
                iter->instances_placed = max - 1;
            } else {
                iter = enabled_specials.erase( iter );
                continue;
            }
        }
        ++iter;
    }
    // Bail out early if we have nothing to place.
    if( enabled_specials.empty() ) {
        return;
    }
    om_special_sectors sectors = get_sectors( OMSPEC_FREQ );

    // First, place the mandatory specials to ensure that all minimum instance
    // counts are met.
    place_specials_pass( enabled_specials, sectors, false, false );

    // Snapshot remaining specials, which will be the optional specials and
    // any unplaced mandatory specials. By passing a copy into the creation of
    // the adjacent overmaps, we ensure that when we unwind the overmap creation
    // back to filling in our non-mandatory specials for this overmap, we won't
    // count the placement of the specials in those maps when looking for optional
    // specials to place here.
    overmap_special_batch custom_overmap_specials = enabled_specials;

    // Check for any unplaced mandatory specials, and if there are any, attempt to
    // place them on adjacent uncreated overmaps.
    bool any_below_minimum =
        std::any_of( custom_overmap_specials.begin(), custom_overmap_specials.end(),
    []( const overmap_special_placement & placement ) {
        return placement.instances_placed <
               placement.special_details->get_constraints().occurrences.min;
    } );

    if( any_below_minimum ) {
        // Randomly select from among the nearest uninitialized overmap positions.
        int previous_distance = 0;
        std::vector<point_abs_om> nearest_candidates;
        // Since this starts at enabled_specials::origin, it will only place new overmaps
        // in the 5x5 area surrounding the initial overmap, bounding the amount of work we will do.
        for( const point_abs_om &candidate_addr : closest_points_first(
                 custom_overmap_specials.get_origin(), 2 ) ) {
            if( !overmap_buffer.has( candidate_addr ) ) {
                int current_distance = square_dist( pos(), candidate_addr );
                if( nearest_candidates.empty() || current_distance == previous_distance ) {
                    nearest_candidates.push_back( candidate_addr );
                    previous_distance = current_distance;
                } else {
                    break;
                }
            }
        }
        if( !nearest_candidates.empty() ) {
            point_abs_om new_om_addr = random_entry( nearest_candidates );
            overmap_buffer.create_custom_overmap( new_om_addr, custom_overmap_specials );
        } else {
            add_msg( _( "Unable to place all configured specials, some missions may fail to initialize." ) );
        }
    }
    // Then fill in non-mandatory specials.
    place_specials_pass( enabled_specials, sectors, true, false );

    // Clean up...
    // Because we passed a copy of the specials for placement in adjacent overmaps rather than
    // the original, but our caller is concerned with whether or not they were placed at all,
    // regardless of whether we placed them or our callee did, we need to reconcile the placement
    // that we did of the optional specials with the placement our callee did of optional
    // and mandatory.

    // Make a lookup of our callee's specials after processing.
    // Because specials are removed from the list once they meet their maximum
    // occurrences, this will only contain those which have not yet met their
    // maximum.
    std::map<overmap_special_id, int> processed_specials;
    for( overmap_special_placement &elem : custom_overmap_specials ) {
        processed_specials[elem.special_details->id] = elem.instances_placed;
    }

    // Loop through the specials we started with.
    for( auto it = enabled_specials.begin(); it != enabled_specials.end(); ) {
        // Determine if this special is still in our callee's list of specials...
        std::map<overmap_special_id, int>::iterator iter = processed_specials.find(
                    it->special_details->id );
        if( iter != processed_specials.end() ) {
            // ... and if so, increment the placement count to reflect the callee's.
            it->instances_placed += ( iter->second - it->instances_placed );

            // If, after incrementing the placement count, we're at our max, remove
            // this special from our list.
            if( it->instances_placed >= it->special_details->get_constraints().occurrences.max ) {
                it = enabled_specials.erase( it );
            } else {
                it++;
            }
        } else {
            // This special is no longer in our callee's list, which means it was completely
            // placed, and we can remove it from our list.
            it = enabled_specials.erase( it );
        }
    }
}

void overmap::place_mongroups()
{
    // Cities can be full of zombies
    int city_spawn_threshold = get_option<int>( "SPAWN_CITY_HORDE_THRESHOLD" );
    if( city_spawn_threshold > -1 ) {
        int city_spawn_chance = get_option<int>( "SPAWN_CITY_HORDE_SMALL_CITY_CHANCE" );
        float city_spawn_scalar = get_option<float>( "SPAWN_CITY_HORDE_SCALAR" );
        float city_spawn_spread = get_option<float>( "SPAWN_CITY_HORDE_SPREAD" );
        float spawn_density = get_option<float>( "SPAWN_DENSITY" );

        for( city &elem : cities ) {
            if( elem.size > city_spawn_threshold || !one_in( city_spawn_chance ) ) {

                // with the default numbers (80 scalar, 1 density), a size 16 city
                // will produce 1280 zombies.
                int desired_zombies = elem.size * city_spawn_scalar * spawn_density;

                float city_effective_radius = elem.size * city_spawn_spread;

                int city_distance_increment = std::ceil( city_effective_radius / 4 );

                tripoint_abs_omt city_center = project_combine( elem.pos_om, tripoint_om_omt( elem.pos, 0 ) );

                std::vector<tripoint_abs_sm> submap_list;

                // gather all of the points in range to test for viable placement of hordes.
                for( tripoint_om_omt const &temp_omt : points_in_radius( tripoint_om_omt( elem.pos, 0 ),
                        static_cast<int>( city_effective_radius ), 0 ) ) {

                    // running too close to the edge of the overmap can get us cascading mapgen
                    if( inbounds( temp_omt, 2 ) ) {

                        tripoint_abs_omt target_omt = project_combine( elem.pos_om, temp_omt );

                        // right now we're only placing city horde spawns on roads, for simplicity.
                        // this can be replaced with an OMT flag for later for better flexibility.
                        if( overmap_buffer.ter( target_omt )->get_type_id() == oter_type_road ) {
                            tripoint_abs_sm this_sm = project_to<coords::sm>( target_omt );

                            // for some reason old style spawns are submap-aligned.
                            // get all four quadrants for better distribution.
                            std::vector<tripoint_abs_sm> local_sm_list;
                            local_sm_list.push_back( this_sm );
                            local_sm_list.push_back( this_sm + point_east );
                            local_sm_list.push_back( this_sm + point_south );
                            local_sm_list.push_back( this_sm + point_south_east );

                            // shuffle, then prune submaps based on distance from city center
                            // this should let us concentrate hordes closer to the center.
                            // the shuffling is so they aren't all aligned consistently.
                            int new_size = 4 - ( trig_dist( target_omt, city_center ) / city_distance_increment );
                            if( new_size > 0 ) {
                                std::shuffle( local_sm_list.begin(), local_sm_list.end(), rng_get_engine() );
                                local_sm_list.resize( new_size );

                                submap_list.insert( submap_list.end(), local_sm_list.begin(), local_sm_list.end() );
                            }

                        }
                    }
                }

                if( submap_list.empty() ) {
                    // somehow the city has no roads. this shouldn't happen.
                    add_msg_debug( debugmode::DF_OVERMAP,
                                   "tried to add zombie hordes to city %s centered at omt %s, but there were no roads!",
                                   elem.name, city_center.to_string_writable() );
                    continue;
                }

                add_msg_debug( debugmode::DF_OVERMAP, "adding %i zombies in hordes to city %s centered at omt %s.",
                               desired_zombies, elem.name, city_center.to_string_writable() );

                // if there aren't enough roads, we'll just reuse them, re-shuffled.
                while( desired_zombies > 0 ) {
                    std::shuffle( submap_list.begin(), submap_list.end(), rng_get_engine() );
                    for( tripoint_abs_sm const &s : submap_list ) {
                        if( desired_zombies <= 0 ) {
                            break;
                        }
                        mongroup m( GROUP_ZOMBIE, s, desired_zombies > 10 ? 10 : desired_zombies );

                        // with wander_spawns (aka wandering hordes) off, these become 'normal'
                        // zombie spawns and behave like ants, triffids, fungals, etc.
                        // they won't try very hard to get placed in the world, so there will
                        // probably be fewer zombies than expected.
                        m.horde = true;
                        if( get_option<bool>( "WANDER_SPAWNS" ) ) {
                            m.wander( *this );
                        }
                        add_mon_group( m );
                        desired_zombies -= 10;
                    }
                }
            }
        }
    }

    if( get_option<bool>( "DISABLE_ANIMAL_CLASH" ) ) {
        // Figure out where swamps are, and place swamp monsters
        for( int x = 3; x < OMAPX - 3; x += 7 ) {
            for( int y = 3; y < OMAPY - 3; y += 7 ) {
                int swamp_count = 0;
                for( int sx = x - 3; sx <= x + 3; sx++ ) {
                    for( int sy = y - 3; sy <= y + 3; sy++ ) {
                        if( ter( { sx, sy, 0 } ) == oter_forest_water ) {
                            swamp_count += 2;
                        }
                    }
                }
                if( swamp_count >= 25 ) {
                    tripoint_om_omt p( x, y, 0 );
                    float norm_factor = std::abs( GROUP_SWAMP->freq_total / 1000.0f );
                    unsigned int pop =
                        std::round( norm_factor * rng( swamp_count * 8, swamp_count * 25 ) );
                    spawn_mon_group(
                        mongroup(
                            GROUP_SWAMP, project_combine( pos(), project_to<coords::sm>( p ) ),
                            pop ), 3 );
                }
            }
        }
    }

    // Figure out where rivers and lakes are, and place appropriate critters
    for( int x = 3; x < OMAPX - 3; x += 7 ) {
        for( int y = 3; y < OMAPY - 3; y += 7 ) {
            int river_count = 0;
            for( int sx = x - 3; sx <= x + 3; sx++ ) {
                for( int sy = y - 3; sy <= y + 3; sy++ ) {
                    if( is_lake_or_river( ter( { sx, sy, 0 } ) ) ) {
                        river_count++;
                    }
                }
            }
            if( river_count >= 25 && is_lake_or_river( ter( { x, y, 0 } ) ) ) {
                tripoint_om_omt p( x, y, 0 );
                float norm_factor = std::abs( GROUP_RIVER->freq_total / 1000.0f );
                unsigned int pop =
                    std::round( norm_factor * rng( river_count * 8, river_count * 25 ) );
                spawn_mon_group(
                    mongroup( GROUP_RIVER, project_combine( pos(), project_to<coords::sm>( p ) ),
                              pop ), 3 );
            }
        }
    }

    // Now place ocean mongroup. Weights may need to be altered.
    const om_noise::om_noise_layer_ocean f( global_base_point(), g->get_seed() );
    const point_abs_om this_om = pos();
    const int northern_ocean = settings->overmap_ocean.ocean_start_north;
    const int eastern_ocean = settings->overmap_ocean.ocean_start_east;
    const int western_ocean = settings->overmap_ocean.ocean_start_west;
    const int southern_ocean = settings->overmap_ocean.ocean_start_south;

    // noise threshold adjuster for deep ocean. Increase to make deep ocean move further from the shore.
    constexpr float DEEP_OCEAN_THRESHOLD_ADJUST = 1.25;

    // code taken from place_oceans, but noise threshold increased to determine "deep ocean".
    const auto is_deep_ocean = [&]( const point_om_omt & p ) {
        // credit to ehughsbaird for thinking up this inbounds solution to infinite flood fill lag.
        if( northern_ocean == 0 && eastern_ocean == 0 && western_ocean == 0 && southern_ocean == 0 ) {
            // you know you could just turn oceans off in global_settings.json right?
            return false;
        }
        bool inbounds = p.x() > -5 && p.y() > -5 && p.x() < OMAPX + 5 && p.y() < OMAPY + 5;
        if( !inbounds ) {
            return false;
        }
        float ocean_adjust = calculate_ocean_gradient( p, this_om );
        if( ocean_adjust == 0.0f ) {
            // It's too soon!  Too soon for an ocean!!  ABORT!!!
            return false;
        }
        return f.noise_at( p ) + ocean_adjust > settings->overmap_ocean.noise_threshold_ocean *
               DEEP_OCEAN_THRESHOLD_ADJUST;
    };

    for( int x = 3; x < OMAPX - 3; x += 7 ) {
        for( int y = 3; y < OMAPY - 3; y += 7 ) {
            int ocean_count = 0;
            for( int sx = x - 3; sx <= x + 3; sx++ ) {
                for( int sy = y - 3; sy <= y + 3; sy++ ) {
                    if( is_ocean( ter( { sx, sy, 0 } ) ) ) {
                        ocean_count++;
                    }
                }
            }
            bool am_deep = is_deep_ocean( { x, y } );
            if( ocean_count >= 25 ) {
                tripoint_om_omt p( x, y, 0 );
                if( am_deep ) {
                    float norm_factor = std::abs( GROUP_OCEAN_DEEP->freq_total / 1000.0f );
                    unsigned int pop =
                        std::round( norm_factor * rng( ocean_count * 8, ocean_count * 25 ) );
                    spawn_mon_group(
                        mongroup( GROUP_OCEAN_DEEP, project_combine( pos(), project_to<coords::sm>( p ) ),
                                  pop ), 3 );
                } else {
                    float norm_factor = std::abs( GROUP_OCEAN_SHORE->freq_total / 1000.0f );
                    unsigned int pop =
                        std::round( norm_factor * rng( ocean_count * 8, ocean_count * 25 ) );
                    spawn_mon_group(
                        mongroup( GROUP_OCEAN_SHORE, project_combine( pos(), project_to<coords::sm>( p ) ),
                                  pop ), 3 );
                }
            }
        }
    }

    // Place the "put me anywhere" groups
    int numgroups = rng( 0, 3 );
    for( int i = 0; i < numgroups; i++ ) {
        float norm_factor = std::abs( GROUP_WORM->freq_total / 1000.0f );
        tripoint_om_sm p( rng( 0, OMAPX * 2 - 1 ), rng( 0, OMAPY * 2 - 1 ), 0 );
        unsigned int pop = std::round( norm_factor * rng( 30, 50 ) );
        // ensure GROUP WORM doesn't get placed in ocean or lake.
        if( !is_water_body( ter( {p.x(), p.y(), 0} ) ) ) {
            spawn_mon_group(
                mongroup( GROUP_WORM, project_combine( pos(), p ), pop ), rng( 20, 40 ) );
        }
    }
}

void overmap::place_nemesis( const tripoint_abs_omt &p )
{
    tripoint_abs_sm pos_sm = project_to<coords::sm>( p );

    mongroup nemesis = mongroup( GROUP_NEMESIS, pos_sm, 1 );
    nemesis.horde = true;
    nemesis.behaviour = mongroup::horde_behaviour::nemesis;
    add_mon_group( nemesis );
}

point_abs_omt overmap::global_base_point() const
{
    return project_to<coords::omt>( loc );
}

void overmap::place_radios()
{
    auto strength = []() {
        return rng( RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH );
    };
    std::string message;
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            tripoint_om_omt pos_omt( i, j, 0 );
            point_om_sm pos_sm = project_to<coords::sm>( pos_omt.xy() );
            // Since location have id such as "radio_tower_1_north", we must check the beginning of the id
            if( is_ot_match( "radio_tower", ter( pos_omt ), ot_match_type::prefix ) ) {
                if( one_in( 3 ) ) {
                    radios.emplace_back( pos_sm, strength(), "", radio_type::WEATHER_RADIO );
                } else {
                    message = SNIPPET.expand( SNIPPET.random_from_category( "radio_archive" ).value_or(
                                                  translation() ).translated() );
                    radios.emplace_back( pos_sm, strength(), message );
                }
            } else if( is_ot_match( "lmoe", ter( pos_omt ), ot_match_type::prefix ) ) {
                message = string_format( _( "This is automated emergency shelter beacon %d%d."
                                            "  Supplies, amenities and shelter are stocked." ), i, j );
                radios.emplace_back( pos_sm, strength() / 2, message );
            } else if( is_ot_match( "fema_entrance", ter( pos_omt ), ot_match_type::prefix ) ) {
                message = string_format( _( "This is FEMA camp %d%d."
                                            "  Supplies are limited, please bring supplemental food, water, and bedding."
                                            "  This is FEMA camp %d%d.  A designated long-term emergency shelter." ), i, j, i, j );
                radios.emplace_back( pos_sm, strength(), message );
            }
        }
    }
}

void overmap::open( overmap_special_batch &enabled_specials )
{
    const cata_path terfilename = overmapbuffer::terrain_filename( loc );

    if( read_from_file_optional( terfilename, [this, &terfilename]( std::istream & is ) {
    unserialize( terfilename, is );
    } ) ) {
        const cata_path plrfilename = overmapbuffer::player_filename( loc );
        read_from_file_optional( plrfilename, [this, &plrfilename]( std::istream & is ) {
            unserialize_view( plrfilename, is );
        } );
    } else { // No map exists!  Prepare neighbors, and generate one.
        std::vector<const overmap *> pointers;
        // Fetch south and north
        for( int i = -1; i <= 1; i += 2 ) {
            pointers.push_back( overmap_buffer.get_existing( loc + point( 0, i ) ) );
        }
        // Fetch east and west
        for( int i = -1; i <= 1; i += 2 ) {
            pointers.push_back( overmap_buffer.get_existing( loc + point( i, 0 ) ) );
        }

        // pointers looks like (north, south, west, east)
        generate( pointers[0], pointers[3], pointers[1], pointers[2], enabled_specials );
    }
}

// Note: this may throw io errors from std::ofstream
void overmap::save() const
{
    write_to_file( overmapbuffer::player_filename( loc ), [&]( std::ostream & stream ) {
        serialize_view( stream );
    } );

    write_to_file( overmapbuffer::terrain_filename( loc ), [&]( std::ostream & stream ) {
        serialize( stream );
    } );
}

void overmap::spawn_mon_group( const mongroup &group, int radius )
{
    tripoint_om_omt pos = project_to<coords::omt>( group.rel_pos() );
    if( safe_at_worldgen.find( pos ) != safe_at_worldgen.end() ) {
        return;
    }
    add_mon_group( group, radius );
}

void overmap::add_mon_group( const mongroup &group )
{
    zg.emplace( group.rel_pos(), group );
}

void overmap::add_mon_group( const mongroup &group, int radius )
{
    // We only spread the groups out when radius is greater than 1
    if( radius <= 1 ) {
        add_mon_group( group );
        return;
    }
    const int rad = std::max<int>( 0, radius );
    const double total_area = rad * rad * M_PI + 1;
    const double pop = std::max<int>( 0, group.population );
    for( int x = -rad; x <= rad; x++ ) {
        for( int y = -rad; y <= rad; y++ ) {
            const int dist = trig_dist( point( x, y ), point_zero );
            if( dist > rad ) {
                continue;
            }
            // Population on a single submap, *not* a integer
            double pop_here;
            if( rad == 0 ) {
                pop_here = pop;
            } else {
                // This computation is delicate, be careful and see
                // https://github.com/CleverRaven/Cataclysm-DDA/issues/26941
                pop_here = ( static_cast<double>( rad - dist ) / rad ) * pop / total_area;
            }
            if( pop_here > pop || pop_here < 0 ) {
                DebugLog( D_ERROR, D_GAME ) << group.type.str() << ": invalid population here: " << pop_here;
            }
            int p = std::max( 0, static_cast<int>( std::floor( pop_here ) ) );
            if( pop_here - p != 0 ) {
                // in case the population is something like 0.2, randomly add a
                // single population unit, this *should* on average give the correct
                // total population.
                const int mod = static_cast<int>( 10000.0 * ( pop_here - p ) );
                if( x_in_y( mod, 10000 ) ) {
                    p++;
                }
            }
            if( p == 0 ) {
                continue;
            }
            // Exact copy to keep all important values, only change what's needed
            // for a single-submap group.
            mongroup tmp( group );
            tmp.abs_pos += point( x, y );
            tmp.population = p;
            // This *can* create groups outside of the area of this overmap.
            // As this function is called during generating the overmap, the
            // neighboring overmaps might not have been generated and one can't access
            // them through the overmapbuffer as this would trigger generating them.
            // This would in turn to lead to a call to this function again.
            // To avoid this, the overmapbuffer checks the monster groups when loading
            // an overmap and moves groups with out-of-bounds position to another overmap.
            add_mon_group( tmp );
        }
    }
}

shared_ptr_fast<npc> overmap::find_npc( const character_id &id ) const
{
    for( const auto &guy : npcs ) {
        if( guy->getID() == id ) {
            return guy;
        }
    }
    return nullptr;
}

shared_ptr_fast<npc> overmap::find_npc_by_unique_id( const std::string &id ) const
{
    for( const auto &guy : npcs ) {
        if( guy->get_unique_id() == id ) {
            return guy;
        }
    }
    return nullptr;
}

std::optional<basecamp *> overmap::find_camp( const point_abs_omt &p )
{
    for( basecamp &v : camps ) {
        if( v.camp_omt_pos().xy() == p ) {
            return &v;
        }
    }
    return std::nullopt;
}

bool overmap::is_omt_generated( const tripoint_om_omt &loc ) const
{
    if( !inbounds( loc ) ) {
        return false;
    }

    // Location is local to this overmap, but we need global submap coordinates
    // for the mapbuffer lookup.
    tripoint_abs_sm global_sm_loc =
        project_to<coords::sm>( project_combine( pos(), loc ) );

    const bool is_generated = MAPBUFFER.lookup_submap( global_sm_loc ) != nullptr;

    return is_generated;
}

overmap_special_id overmap_specials::create_building_from( const string_id<oter_type_t> &base )
{
    // TODO: Get rid of the hard-coded ids.
    overmap_special_terrain ter;
    ter.terrain = base.obj().get_first().id();
    ter.locations.insert( overmap_location_land );
    ter.locations.insert( overmap_location_swamp );

    overmap_special_id new_id( "FakeSpecial_" + base.str() );
    overmap_special new_special( new_id, ter );
    mod_tracker::assign_src( new_special, base->src.back().second.str() );

    return specials.insert( new_special ).id;
}

namespace io
{
template<>
std::string enum_to_string<ot_match_type>( ot_match_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case ot_match_type::exact: return "EXACT";
        case ot_match_type::type: return "TYPE";
        case ot_match_type::subtype: return "SUBTYPE";
        case ot_match_type::prefix: return "PREFIX";
        case ot_match_type::contains: return "CONTAINS";
        // *INDENT-ON*
        case ot_match_type::num_ot_match_type:
            break;
    }
    cata_fatal( "Invalid ot_match_type" );
}
} // namespace io

static const std::array<std::string, 4> suffixes = {{ "_north", "_west", "_south", "_east" }};

std::string_view oter_no_dir( const oter_id &oter )
{
    std::string_view base_oter_id = oter.id().str();
    for( const std::string &suffix : suffixes ) {
        if( string_ends_with( base_oter_id, suffix ) ) {
            base_oter_id = base_oter_id.substr( 0, base_oter_id.size() - suffix.size() );
        }
    }
    return base_oter_id;
}

int oter_get_rotation( const oter_id &oter )
{
    std::string base_oter_id = oter.id().c_str();
    for( size_t i = 0; i < suffixes.size(); ++i ) {
        if( string_ends_with( base_oter_id, suffixes[i] ) ) {
            return i;
        }
    }
    return 0;
}

std::string oter_get_rotation_string( const oter_id &oter )
{
    std::string base_oter_id = oter.id().c_str();
    for( const std::string &suffix : suffixes ) {
        if( string_ends_with( base_oter_id, suffix ) ) {
            return suffix;
        }
    }
    return "";
}

void overmap_special_migration::load_migrations( const JsonObject &jo, const std::string &src )
{
    migrations.load( jo, src );
}

void overmap_special_migration::reset()
{
    migrations.reset();
}

void overmap_special_migration::load( const JsonObject &jo, const std::string_view )
{
    mandatory( jo, was_loaded, "id", id );
    optional( jo, was_loaded, "new_id", new_id, overmap_special_id() );
}

void overmap_special_migration::check()
{
    for( const overmap_special_migration &mig : migrations.get_all() ) {
        if( !mig.new_id.is_null() && !mig.new_id.is_valid() ) {
            debugmsg( "Invalid new_id \"%s\" for overmap special migration \"%s\"", mig.new_id.c_str(),
                      mig.id.c_str() );
        }
    }
}

bool overmap_special_migration::migrated( const overmap_special_id &os_id )
{
    std::vector<overmap_special_migration> migs = migrations.get_all();
    return std::find_if( migs.begin(), migs.end(), [&os_id]( overmap_special_migration & m ) {
        return os_id == overmap_special_id( m.id.str() );
    } ) != migs.end();
}

overmap_special_id overmap_special_migration::migrate( const overmap_special_id &old_id )
{
    for( const overmap_special_migration &mig : migrations.get_all() ) {
        if( overmap_special_id( mig.id.str() ) == old_id ) {
            return mig.new_id;
        }
    }
    return old_id;
}
