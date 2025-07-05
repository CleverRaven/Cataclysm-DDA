#include "cube_direction.h" // IWYU pragma: associated
#include "omdata.h" // IWYU pragma: associated
#include "overmap.h" // IWYU pragma: associated

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <filesystem>
#include <list>
#include <memory>
#include <numeric>
#include <optional>
#include <set>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <vector>

#include "all_enum_values.h"
#include "assign.h"
#include "auto_note.h"
#include "avatar.h"
#include "cached_options.h"
#include "cata_assert.h"
#include "cata_path.h"
#include "cata_utility.h"
#include "cata_views.h"
#include "catacharset.h"
#include "character_id.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "dialogue.h"
#include "distribution.h"
#include "effect_on_condition.h"
#include "enum_conversions.h"
#include "filesystem.h"
#include "flood_fill.h"
#include "game.h"
#include "generic_factory.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_extras.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapgen.h"
#include "mapgen_functions.h"
#include "math_defines.h"
#include "messages.h"
#include "mod_tracker.h"
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
#include "talker.h"
#include "text_snippets.h"
#include "translations.h"
#include "weighted_list.h"
#include "worldfactory.h"
#include "zzip.h"

static const mongroup_id GROUP_NEMESIS( "GROUP_NEMESIS" );
static const mongroup_id GROUP_OCEAN_DEEP( "GROUP_OCEAN_DEEP" );
static const mongroup_id GROUP_OCEAN_SHORE( "GROUP_OCEAN_SHORE" );
static const mongroup_id GROUP_RIVER( "GROUP_RIVER" );
static const mongroup_id GROUP_SUBWAY_CITY( "GROUP_SUBWAY_CITY" );
static const mongroup_id GROUP_SWAMP( "GROUP_SWAMP" );
static const mongroup_id GROUP_ZOMBIE( "GROUP_ZOMBIE" );

static const oter_str_id oter_central_lab( "central_lab" );
static const oter_str_id oter_central_lab_core( "central_lab_core" );
static const oter_str_id oter_central_lab_train_depot( "central_lab_train_depot" );
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
static const oter_type_str_id oter_type_ice_lab_core( "ice_lab_core" );
static const oter_type_str_id oter_type_ice_lab_stairs( "ice_lab_stairs" );
static const oter_type_str_id oter_type_lab_core( "lab_core" );
static const oter_type_str_id oter_type_lab_stairs( "lab_stairs" );
static const oter_type_str_id oter_type_microlab_sub_connector( "microlab_sub_connector" );
static const oter_type_str_id oter_type_railroad_bridge( "railroad_bridge" );
static const oter_type_str_id oter_type_road( "road" );
static const oter_type_str_id oter_type_road_nesw_manhole( "road_nesw_manhole" );
static const oter_type_str_id oter_type_sewer_connector( "sewer_connector" );
static const oter_type_str_id oter_type_sub_station( "sub_station" );

static const oter_vision_id oter_vision_default( "default" );

static const overmap_location_id overmap_location_land( "land" );
static const overmap_location_id overmap_location_swamp( "swamp" );

static const species_id species_ZOMBIE( "ZOMBIE" );

#define dbg(x) DebugLog((x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

static constexpr int BUILDINGCHANCE = 4;

using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

////////////////
static oter_id ot_null;

const oter_type_t oter_type_t::null_type{};

namespace
{
generic_factory<oter_vision> oter_vision_factory( "oter_vision" );
} // namespace

template<>
const oter_vision &string_id<oter_vision>::obj() const
{
    return oter_vision_factory.obj( *this );
}

template<>
bool string_id<oter_vision>::is_valid() const
{
    return oter_vision_factory.is_valid( *this );
}

void oter_vision::load_oter_vision( const JsonObject &jo, const std::string &src )
{
    oter_vision_factory.load( jo, src );
}

void oter_vision::reset()
{
    oter_vision_factory.reset();
}

void oter_vision::check_oter_vision()
{
    oter_vision_factory.check();
}

const std::vector<oter_vision> &oter_vision::get_all()
{
    return oter_vision_factory.get_all();
}


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
        const bool unique = elem.has_flag( "OVERMAP_UNIQUE" ) || elem.has_flag( "GLOBALLY_UNIQUE" );
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
        if( static_cast<int>( os.has_flag( "GLOBALLY_UNIQUE" ) ) +
            static_cast<int>( os.has_flag( "OVERMAP_UNIQUE" ) ) +
            static_cast<int>( os.has_flag( "CITY_UNIQUE" ) ) > 1 ) {
            debugmsg( "In special %s, the mutually exclusive flags GLOBALLY_UNIQUE, "
                      "OVERMAP_UNIQUE and CITY_UNIQUE cannot be used together.", os.id.str() );
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

bool is_water_body_not_shore( const oter_id &ter )
{
    return ter->is_river() || ter->is_lake() || ter->is_ocean();
}

bool is_ocean( const oter_id &ter )
{
    return ter->is_ocean() || ter->is_ocean_shore();
}

bool is_road( const oter_id &ter )
{
    return ter->is_road();
}

bool is_highway( const oter_id &ter )
{
    return ter->is_highway();
}

bool is_highway_reserved( const oter_id &ter )
{
    return ter->is_highway_reserved();
}

bool is_highway_special( const oter_id &ter )
{
    return ter->is_highway_special();
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
            load_and_add_mapgen_function( jio, fmapkey, point_rel_omt::zero, point_rel_omt( 1, 1 ) );
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
        case oter_flags::road: return "ROAD";
        case oter_flags::highway: return "HIGHWAY";
        case oter_flags::highway_reserved: return "HIGHWAY_RESERVED";
        case oter_flags::highway_special: return "HIGHWAY_SPECIAL";
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
        case oter_flags::pp_generate_riot_damage: return "PP_GENERATE_RIOT_DAMAGE";
        case oter_flags::generic_loot: return "GENERIC_LOOT";
        case oter_flags::risk_extreme: return "RISK_EXTREME";
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

double oter_type_t::see_cost_value( oter_type_t::see_costs cost )
{
    switch( cost ) {
        // *INDENT-OFF*
        case oter_type_t::see_costs::all_clear:
        case oter_type_t::see_costs::none: return 0;
        case oter_type_t::see_costs::low: return 1;
        case oter_type_t::see_costs::medium: return 2;
        case oter_type_t::see_costs::spaced_high: return 4;
        case oter_type_t::see_costs::high: return 5;
        case oter_type_t::see_costs::full_high: return 10;
        case oter_type_t::see_costs::opaque: return 999;
        default: break;
        // *INDENT-ON*
    }
    return 0;
}

namespace io
{
template<>
std::string enum_to_string<oter_travel_cost_type>( oter_travel_cost_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case oter_travel_cost_type::other: return "other";
        case oter_travel_cost_type::highway: return "highway";
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

void oter_vision::level::deserialize( const JsonObject &jo )
{
    optional( jo, false, "blends_adjacent", blends_adjacent, false );
    if( blends_adjacent ) {
        return;
    }
    mandatory( jo, false, "name", name );
    mandatory( jo, false, "sym", symbol, unicode_codepoint_from_symbol_reader );
    assign( jo, "color", color );
    optional( jo, false, "looks_like", looks_like );
}

oter_vision_id oter_vision::get_id() const
{
    return id;
}

void oter_vision::load( const JsonObject &jo, std::string_view )
{
    if( id.str().find( '$' ) != std::string::npos ) {
        jo.throw_error( string_format( "id for vision level %s contains a '$'", id.str() ) );
    }
    mandatory( jo, was_loaded, "levels", levels );
}

const oter_vision::level *oter_vision::viewed( om_vision_level vision ) const
{
    size_t idx = -1;
    switch( vision ) {
        case om_vision_level::vague:
            idx = 0;
            break;
        case om_vision_level::outlines:
            idx = 1;
            break;
        case om_vision_level::details:
            idx = 2;
            break;
        default:
            return nullptr;
    }
    if( idx >= levels.size() ) {
        return nullptr;
    }
    return &levels[idx];
}

void oter_vision::check() const
{
    if( levels.size() > 3 ) {
        debugmsg( "Too many vision levels assigned!" );
    }
}


void oter_type_t::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    optional( jo, was_loaded, "sym", symbol, unicode_codepoint_from_symbol_reader, NULL_UNICODE );

    assign( jo, "name", name, strict );
    // For some reason an enum can be read as a number??
    if( jo.has_number( "see_cost" ) ) {
        jo.throw_error( string_format( "In %s: See cost uses invalid number format", id.str() ) );
    }
    mandatory( jo, was_loaded, "see_cost", see_cost );
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

    optional( jo, was_loaded, "vision_levels", vision_levels, oter_vision_default );
    optional( jo, false, "uniform_terrain", uniform_terrain );
    if( uniform_terrain ) {
        return;
    } else if( has_flag( oter_flags::line_drawing ) ) {
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
    if( !vision_levels.is_valid() ) {
        debugmsg( "Invalid vision_levels '%s' for '%s'", vision_levels.str(), id.str() );
    }
    if( uniform_terrain && !uniform_terrain->is_valid() ) {
        debugmsg( "Invalid uniform_terrain id '%s' for '%s'", uniform_terrain->c_str(), id.str() );
    }
    /* find omts without vision_levels assigned
    if( vision_levels == oter_vision_default && !has_flag( oter_flags::should_not_spawn ) ) {
        fprintf( stderr, "%s (%s)\n", id.c_str(), name.translated().c_str() );
    }
    */
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

bool oter_t::blends_adjacent( om_vision_level vision ) const
{
    if( const oter_vision::level *seen = type->vision_levels->viewed( vision ) ) {
        return seen->blends_adjacent;
    }
    return false;
}

std::string oter_t::get_name( om_vision_level vision ) const
{
    if( const oter_vision::level *seen = type->vision_levels->viewed( vision ) ) {
        return seen->name.translated();
    }
    return type->name.translated();
}

std::string oter_t::get_symbol( om_vision_level vision, const bool from_land_use_code ) const
{
    if( from_land_use_code ) {
        return utf32_to_utf8( symbol_alt );
    }
    if( const oter_vision::level *seen = type->vision_levels->viewed( vision ) ) {
        return utf32_to_utf8( seen->symbol );
    }
    return utf32_to_utf8( symbol );
}

uint32_t oter_t::get_uint32_symbol() const
{
    return symbol;
}

nc_color oter_t::get_color( om_vision_level vision, const bool from_land_use_code ) const
{
    if( from_land_use_code ) {
        return type->land_use_code->color;
    }
    if( const oter_vision::level *seen = type->vision_levels->viewed( vision ) ) {
        return seen->color;
    }
    return type->color;
}

std::string oter_t::get_tileset_id( om_vision_level vision ) const
{
    // If this changes, be sure to change the debug display on the overmap ui to not strip the prefix!
    if( type->vision_levels->viewed( vision ) != nullptr ) {
        return string_format( "vl#%s$%s", type->vision_levels.str(), io::enum_to_string( vision ) );
    }
    return "om#" + type->id.str();
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
    if( id == oter_road_nesw_manhole ) {
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
        "lab_finale"
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

        if( has_mapgen_for( mid ) ) {
            if( test_mode ) {
                if( elem.is_hardcoded() ) {
                    debugmsg( "Mapgen terrain \"%s\" exists in both JSON and a hardcoded function.  Consider removing the latter.",
                              mid.c_str() );
                } else if( elem.has_uniform_terrain() ) {
                    debugmsg( "Mapgen terrain \"%s\" specifies a uniform_terrain which is incompatible with additional JSON mapgen.",
                              mid.c_str() );
                }
            }
            check_mapgen_consistent_with( mid, elem );
        } else if( !elem.is_hardcoded() && !elem.has_uniform_terrain() ) {
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
    om.read( "camp", camp_owner );
    om.read( "camp_name", camp_name );
    om.read( "flags", flags );
    om.read( "locations", locations );
}

overmap_special_terrain::overmap_special_terrain(
    const tripoint_rel_omt &p, const oter_str_id &t,
    const cata::flat_set<string_id<overmap_location>> &l,
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
            return tripoint::north;
        case cube_direction::east:
            return tripoint::east;
        case cube_direction::south:
            return tripoint::south;
        case cube_direction::west:
            return tripoint::west;
        case cube_direction::above:
            return tripoint::above;
        case cube_direction::below:
            return tripoint::below;
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
        std::set<tripoint_rel_omt> points;

        for( const overmap_special_terrain &elem : terrains ) {
            const oter_str_id &oter = elem.terrain;

            if( !oter.is_valid() ) {
                if( !invalid_terrains.count( oter ) ) {
                    // Not a huge fan of the direct id manipulation here, but I don't know
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

            const tripoint_rel_omt &pos = elem.p;

            if( points.count( pos ) > 0 ) {
                debugmsg( "In %s, point %s is duplicated.", context, pos.to_string() );
            } else {
                points.insert( pos );
            }

            if( elem.camp_owner.has_value() ) {
                if( !elem.camp_owner.value().is_valid() ) {
                    debugmsg( "In %s, camp at %s has invalid owner %s", context, pos.to_string(),
                              elem.camp_owner.value().c_str() );
                }
                if( elem.camp_name.empty() ) {
                    debugmsg( "In %s, camp was defined but missing a camp_name.", context );
                }
            } else if( !elem.camp_name.empty() ) {
                debugmsg( "In %s, camp_name defined but no owner.  Invalid name is discarded.", context );
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

    const overmap_special_terrain &get_terrain_at( const tripoint_rel_omt &p ) const {
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

    std::vector<overmap_special_terrain> get_terrains() const override {
        std::vector<overmap_special_terrain> result;
        std::copy( terrains.begin(), terrains.end(), std::back_inserter( result ) );
        return result;
    }

    std::vector<overmap_special_terrain> preview_terrains() const override {
        std::vector<overmap_special_terrain> result;
        std::copy_if( terrains.begin(), terrains.end(), std::back_inserter( result ),
        []( const overmap_special_terrain & terrain ) {
            return terrain.p.z() == 0;
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
                if( elem.camp_owner.has_value() ) {
                    // This always results in z=0, but pos() doesn't return z-level information...
                    tripoint_abs_omt camp_loc =  {project_combine( om.pos(), location.xy() ), 0};
                    get_map().add_camp( camp_loc, "faction_camp", false );
                    std::optional<basecamp *> bcp = overmap_buffer.find_camp( camp_loc.xy() );
                    if( !bcp ) {
                        debugmsg( "Camp placement during special generation failed at %s", camp_loc.to_string() );
                    } else {
                        basecamp *temp_camp = *bcp;
                        temp_camp->set_owner( elem.camp_owner.value() );
                        temp_camp->set_name( elem.camp_name.translated() );
                        // FIXME? Camp types are raw strings! Not ideal.
                        temp_camp->define_camp( camp_loc, "faction_base_bare_bones_NPC_camp_0", false );
                    }
                }
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
                // TODO: JSONification of logic + don't treat non roads like roads
                point_om_omt target;
                if( cit ) {
                    target = cit.pos;
                } else {
                    target = om.get_fallback_road_connection_point();
                }
                om.build_connection( target, rp.xy(), elem.p.z(), *elem.connection, must_be_unexplored,
                                     initial_dir );
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
    std::optional<faction_id> camp_owner;
    translation camp_name;

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
        if( camp_owner.has_value() ) {
            if( !camp_owner.value().is_valid() ) {
                debugmsg( "In %s, camp at %s has invalid owner %s", context, terrain.str(),
                          camp_owner.value().c_str() );
            }
            if( camp_name.empty() ) {
                debugmsg( "In %s, camp was defined but missing a camp_name.", context );
            }
        } else if( !camp_name.empty() ) {
            debugmsg( "In %s, camp_name defined but no owner.  Invalid name is discarded.", context );
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
        jo.read( "camp", camp_owner );
        jo.read( "camp_name", camp_name );
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

        explicit satisfy_result( const tripoint_om_omt origin, const om_direction::type dir,
                                 mutable_overmap_placement_rule_remainder *rule,
                                 std::vector<om_pos_dir> suppressed_joins, std::string description ) :
            origin( origin ), dir( dir ), rule( rule ),
            suppressed_joins( std::move( suppressed_joins ) ), description( std::move( description ) ) {}
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

        for( const mutable_overmap_piece_candidate &piece : rule.pieces( origin, dir ) ) {
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

        for( const std::pair<om_pos_dir, const mutable_overmap_terrain_join *> &p :
             rule.outward_joins( origin, dir ) ) {
            const om_pos_dir &pos_d = p.first;
            const mutable_overmap_terrain_join &ter_join = *p.second;
            const mutable_overmap_join &join = *ter_join.join;
            switch( unresolved.allows( pos_d, ter_join ) ) {
                case joins_tracker::join_status::disallowed:
                    return std::nullopt;
                case joins_tracker::join_status::matched_non_available:
                    ++context_mandatory_joins_shortfall;
                    [[fallthrough]];
                case joins_tracker::join_status::matched_available:
                    if( ter_join.type != join_type::available ) {
                        ++num_my_non_available_matched;
                    }
                    continue;
                case joins_tracker::join_status::mismatched_available:
                    suppressed_joins.push_back( pos_d );
                    break;
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
                            pos_dir_options.emplace_back( origin, dir, &rule, result.value().supressed_joins, std::string{} );
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
                    om.ter( pos + point::north ).id().str(), om.ter( pos + point::east ).id().str(),
                    om.ter( pos + point::south ).id().str(), om.ter( pos + point::west ).id().str(),
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
                    om.ter( pos + point::north ).id().str(), om.ter( pos + point::east ).id().str(),
                    om.ter( pos + point::south ).id().str(), om.ter( pos + point::west ).id().str(),
                    joins_s, rules_s );
            return satisfy_result{ {}, om_direction::type::invalid, nullptr, std::vector<om_pos_dir>{}, std::move( message ) };
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
        return { tripoint_rel_omt::zero, root_om.terrain, root_om.locations, {} };
    }


    std::vector<overmap_special_terrain> get_terrains() const override {
        return std::vector<overmap_special_terrain> { root_as_overmap_special_terrain() };
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
            if( ter.camp_owner.has_value() ) {
                tripoint_abs_omt camp_loc =  {project_combine( om.pos(), pos.xy() ), pos.z()};
                get_map().add_camp( camp_loc, "faction_camp", false );
                std::optional<basecamp *> bcp = overmap_buffer.find_camp( camp_loc.xy() );
                if( !bcp ) {
                    debugmsg( "Camp placement during special generation failed at %s", camp_loc.to_string() );
                } else {
                    basecamp *temp_camp = *bcp;
                    temp_camp->set_owner( ter.camp_owner.value() );
                    temp_camp->set_name( ter.camp_name.translated() );
                    // FIXME? Camp types are raw strings! Not ideal.
                    temp_camp->define_camp( camp_loc, "faction_base_bare_bones_NPC_camp_0", false );
                }
            }
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
                for( const mutable_overmap_piece_candidate &piece : rule->pieces( satisfy_origin, rot ) ) {
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
        // TODO: JSONification of logic + don't treat non roads like roads + deduplicate with fixed data
        for( const placed_connection &elem : connections_placed ) {
            const tripoint_om_omt &pos = elem.where.p;
            cube_direction connection_dir = elem.where.dir;

            point_om_omt target;
            if( cit ) {
                target = cit.pos;
            } else {
                target = om.get_fallback_road_connection_point();
            }
            om.build_connection( target, pos.xy(), pos.z(), *elem.connection, must_be_unexplored,
                                 connection_dir );
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

bool overmap_special::can_belong_to_city( const tripoint_om_omt &p, const city &cit,
        const overmap &omap ) const
{
    if( !requires_city() ) {
        return true;
    }
    if( !cit || !constraints_.city_size.contains( cit.size ) ) {
        return false;
    }
    if( constraints_.city_distance.max > std::max( OMAPX, OMAPY ) ) {
        // Only care that we're more than min away from a city
        return !omap.approx_distance_to_city( p, constraints_.city_distance.min ).has_value();
    }
    const std::optional<int> dist = omap.approx_distance_to_city( p, constraints_.city_distance.max );
    // Found a city within max and it's greater than min away
    return dist.has_value() && constraints_.city_distance.min < *dist;
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
        return lhs.p.x() < rhs.p.x();
    } );

    auto min_max_y = std::minmax_element( req_locations.begin(), req_locations.end(),
    []( const overmap_special_locations & lhs, const overmap_special_locations & rhs ) {
        return lhs.p.y() < rhs.p.y();
    } );

    const int width = min_max_x.second->p.x() - min_max_x.first->p.x();
    const int height = min_max_y.second->p.y() - min_max_y.first->p.y();
    return std::max( width, height ) + 1;
}

std::vector<overmap_special_terrain> overmap_special::get_terrains() const
{
    return data_->get_terrains();
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
                    tripoint_rel_omt from;
                    tripoint_rel_omt to;
                    mandatory( joc, was_loaded, "type", type );
                    mandatory( joc, was_loaded, "from", from );
                    mandatory( joc, was_loaded, "to", to );
                    if( from.x() > to.x() ) {
                        std::swap( from.x(), to.x() );
                    }
                    if( from.y() > to.y() ) {
                        std::swap( from.y(), to.y() );
                    }
                    if( from.z() > to.z() ) {
                        std::swap( from.z(), to.z() );
                    }
                    for( int x = from.x(); x <= to.x(); x++ ) {
                        for( int y = from.y(); y <= to.y(); y++ ) {
                            for( int z = from.z(); z <= to.z(); z++ ) {
                                overmap_special_locations loc;
                                loc.p = tripoint_rel_omt( x, y, z );
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

    assign( jo, "city_sizes", constraints_.city_size, strict );

    if( is_special ) {
        mandatory( jo, was_loaded, "occurrences", constraints_.occurrences );
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
        l.visible.fill( om_vision_level::unseen );
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

void overmap::set_seen( const tripoint_om_omt &p, om_vision_level val, bool force )
{
    if( !inbounds( p ) ) {
        return;
    }

    if( !force && seen( p ) >= val ) {
        return;
    }

    layer[p.z() + OVERMAP_DEPTH].visible[p.xy()] = val;

    if( val > om_vision_level::details ) {
        add_extra_note( p );
    }
}

om_vision_level overmap::seen( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {
        return om_vision_level::unseen;
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
        return candidate.second.pos_bub() == match.second.pos_bub() &&
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

        int dist = rl_dist( i.p, p.xy() );
        if( dist <= radius ) {
            return true;
        }
    }
    return false;
}

point_om_omt overmap::get_fallback_road_connection_point() const
{
    if( fallback_road_connection_point ) {
        return *fallback_road_connection_point;
    } else {
        return point_om_omt( rng( OMAPX / 4, ( 3 * OMAPX ) / 4 ),
                             rng( OMAPY / 4, ( 3 * OMAPY ) / 4 ) );
    }
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

int overmap::note_danger_radius( const tripoint_om_omt &p ) const
{
    if( p.z() < -OVERMAP_DEPTH || p.z() > OVERMAP_HEIGHT ) {
        return -1;
    }

    const auto &notes = layer[p.z() + OVERMAP_DEPTH].notes;
    const auto it = std::find_if( begin( notes ), end( notes ), [&]( const om_note & n ) {
        return n.p == p.xy();
    } );

    return ( it != std::end( notes ) ) && it->dangerous ? it->danger_radius : -1;
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
    if( seen( p ) < om_vision_level::details ) {
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

std::optional<mapgen_arguments> overmap::get_existing_omt_stack_arguments(
    const point_abs_omt &p ) const
{
    auto it_args = omt_stack_arguments_map.find( p );
    if( it_args != omt_stack_arguments_map.end() ) {
        return it_args->second;
    }
    return std::nullopt;
}

std::optional<cata_variant> overmap::get_existing_omt_stack_argument( const point_abs_omt &p,
        const std::string &param_name ) const
{
    auto it_args = omt_stack_arguments_map.find( p );
    if( it_args != omt_stack_arguments_map.end() ) {
        const std::unordered_map<std::string, cata_variant> &args_map = it_args->second.map;
        auto it_arg = args_map.find( param_name );
        if( it_arg != args_map.end() ) {
            return it_arg->second;
        }
    }
    return std::nullopt;
}

void overmap::add_omt_stack_argument( const point_abs_omt &p, const std::string &param_name,
                                      const cata_variant &value )
{
    omt_stack_arguments_map[p].add( param_name, value );
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

void overmap::generate( const std::vector<const overmap *> &neighbor_overmaps,
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

    std::vector<std::vector<highway_node>> highway_paths;
    calculate_urbanity();
    calculate_forestosity();
    if( get_option<bool>( "OVERMAP_POPULATE_OUTSIDE_CONNECTIONS_FROM_NEIGHBORS" ) ) {
        populate_connections_out_from_neighbors( neighbor_overmaps );
    }
    if( get_option<bool>( "OVERMAP_PLACE_RIVERS" ) ) {
        place_rivers( neighbor_overmaps );
    }
    if( get_option<bool>( "OVERMAP_PLACE_LAKES" ) ) {
        place_lakes( neighbor_overmaps );
    }
    if( get_option<bool>( "OVERMAP_PLACE_OCEANS" ) ) {
        place_oceans( neighbor_overmaps );
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
    if( get_option<bool>( "OVERMAP_PLACE_RIVERS" ) ) {
        polish_river(
            neighbor_overmaps ); // Polish rivers now so highways get the correct predecessors rather than river_center
    }
    if( get_option<bool>( "OVERMAP_PLACE_HIGHWAYS" ) ) {
        highway_paths = place_highways( neighbor_overmaps );
    }
    if( get_option<bool>( "OVERMAP_PLACE_CITIES" ) ) {
        place_cities();
    }
    if( get_option<bool>( "OVERMAP_PLACE_FOREST_TRAILS" ) ) {
        place_forest_trails();
    }
    if( get_option<bool>( "OVERMAP_PLACE_RAILROADS_BEFORE_ROADS" ) ) {
        if( get_option<bool>( "OVERMAP_PLACE_RAILROADS" ) ) {
            place_railroads( neighbor_overmaps );
        }
        if( get_option<bool>( "OVERMAP_PLACE_ROADS" ) ) {
            place_roads( neighbor_overmaps );
        }
    } else {
        if( get_option<bool>( "OVERMAP_PLACE_ROADS" ) ) {
            place_roads( neighbor_overmaps );
        }
        if( get_option<bool>( "OVERMAP_PLACE_RAILROADS" ) ) {
            place_railroads( neighbor_overmaps );
        }
    }
    if( get_option<bool>( "OVERMAP_PLACE_SPECIALS" ) ) {
        place_specials( enabled_specials );
    }
    if( get_option<bool>( "OVERMAP_PLACE_HIGHWAYS" ) ) {
        finalize_highways( highway_paths );
    }
    if( get_option<bool>( "OVERMAP_PLACE_FOREST_TRAILHEADS" ) ) {
        place_forest_trailheads();
    }
    if( get_option<bool>( "OVERMAP_PLACE_RIVERS" ) ) {
        polish_river( neighbor_overmaps ); // Polish again for placed specials
    }

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

    std::unordered_map<oter_type_id, std::function<void( const tripoint_om_omt &p )>>
    oter_above_actions = {
        {
            oter_type_road_nesw_manhole.id(),
            [&]( const tripoint_om_omt & p )
            {
                ter_set( p, oter_sewer_isolated.id() );
                sewer_points.emplace_back( p.xy() );
            }
        },
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
            const oter_id oter_above = ter_unsafe( p + tripoint::above );
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
    const overmap_connection_id &overmap_connection_sewer_tunnel =
        settings->overmap_connection.sewer_connection;
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
                i + point::north,
                i + point::south,
                i + point::east,
                i + point::west };
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
    const overmap_connection_id &overmap_connection_subway_tunnel =
        settings->overmap_connection.subway_connection;
    connect_closest_points( subway_points, z, *overmap_connection_subway_tunnel );

    for( auto &i : subway_points ) {
        if( ( ter( tripoint_om_omt( i, z + 2 ) )->get_type_id() == oter_type_sub_station ) ) {
            ter_set( tripoint_om_omt( i, z ), oter_underground_sub_station.id() );
        }
    }

    // The first lab point is adjacent to a lab, set it a depot (as long as track was actually laid).
    const auto create_train_depots = [this, z,
                                            overmap_connection_subway_tunnel]( const oter_id & train_type,
    const std::vector<point_om_omt> &train_points ) {
        bool is_first_in_pair = true;
        std::vector<point_om_omt> extra_route;
        for( const point_om_omt &p : train_points ) {
            tripoint_om_omt i( p, z );
            if( is_first_in_pair ) {
                const std::vector<tripoint_om_omt> subway_possible_loc {
                    i + point::north,
                    i + point::south,
                    i + point::east,
                    i + point::west };
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
                const oter_id oter_below = ter( p + tripoint::below );
                const oter_id oter_ground = ter( tripoint_om_omt( p.xy(), 0 ) );

                // implicitly skip skip_below oter_ids
                if( skip_below.find( oter_below ) != skip_below.end() ) {
                    continue;
                }

                if( oter_ground->get_type_id() == oter_type_bridge ) {
                    ter_set( p, oter_id( "bridge_road" + oter_get_rotation_string( oter_ground ) ) );
                    bridge_points.push_back( p.xy() );
                    tripoint_om_omt support_point = p + tripoint::below;
                    int support_z = 0;
                    // place the rest of the support columns
                    while( ter( support_point ) -> is_water() && --support_z >= -OVERMAP_DEPTH ) {
                        ter_set( support_point, oter_id( "bridge" + oter_get_rotation_string( oter_ground ) ) );
                        support_point += tripoint::below;
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
        const oter_id oter_ground_north = ter( tripoint_om_omt( bp, 0 ) + tripoint::north );
        const oter_id oter_ground_south = ter( tripoint_om_omt( bp, 0 ) + tripoint::south );
        const oter_id oter_ground_east = ter( tripoint_om_omt( bp, 0 ) + tripoint::east );
        const oter_id oter_ground_west = ter( tripoint_om_omt( bp, 0 ) + tripoint::west );
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
        ter_set( p + tripoint::above, oter_id( bridgehead_ramp + bhp.second ) );
    }
}

std::vector<point_abs_omt> overmap::find_terrain( std::string_view term, int zlevel ) const
{
    std::vector<point_abs_omt> found;
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            tripoint_om_omt p( x, y, zlevel );
            om_vision_level vision = seen( p );
            if( vision != om_vision_level::unseen &&
                lcmatch( ter( p )->get_name( vision ), term ) ) {
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

const city &overmap::get_invalid_city() const
{
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
    return random_entry( valid, tripoint_om_omt::invalid );
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

bool overmap::is_in_city( const tripoint_om_omt &p ) const
{
    if( !city_tiles.empty() ) {
        return city_tiles.find( p.xy() ) != city_tiles.end();
    } else {
        // Legacy handling
        return distance_to_city( p ) == 0;
    }
}

std::optional<int> overmap::distance_to_city( const tripoint_om_omt &p,
        int max_dist_to_check ) const
{
    if( !city_tiles.empty() ) {
        for( int i = 0; i <= max_dist_to_check; i++ ) {
            for( const tripoint_om_omt &tile : closest_points_first( p, i, i ) ) {
                if( is_in_city( tile ) ) {
                    return i;
                }
            }
        }
    } else {
        // Legacy handling
        const city &nearest_city = get_nearest_city( p );
        if( !!nearest_city ) {
            // 0 if within city
            return std::max( 0, nearest_city.get_distance_from( p ) - nearest_city.size );
        }
    }
    return {};
}

std::optional<int> overmap::approx_distance_to_city( const tripoint_om_omt &p,
        int max_dist_to_check ) const
{
    std::optional<int> ret;
    for( const city &elem : cities ) {
        const int dist = elem.get_distance_from( p );
        if( dist == 0 ) {
            return 0;
        }
        if( dist <= max_dist_to_check ) {
            ret = ret.has_value() ? std::min( ret.value(), dist ) : dist;
        }
    }
    return ret;
}

void overmap::flood_fill_city_tiles()
{
    std::unordered_set<point_om_omt> visited;
    // simplifies bounds checking
    const half_open_rectangle<point_om_omt> omap_bounds( point_om_omt( 0, 0 ), point_om_omt( OMAPX,
            OMAPY ) );

    // Look through every point on the overmap
    for( int y = 0; y < OMAPY; y++ ) {
        for( int x = 0; x < OMAPX; x++ ) {
            point_om_omt checked( x, y );
            // If we already looked at it in a previous flood-fill, ignore it
            if( visited.find( checked ) != visited.end() ) {
                continue;
            }
            // Is the area connected to this point enclosed by city_tiles?
            bool enclosed = true;
            // Predicate for flood-fill. Also detects if any point flood-filled to borders the edge
            // of the overmap and is thus not enclosed
            const auto is_unchecked = [&enclosed, &omap_bounds, this]( const point_om_omt & pt ) {
                if( city_tiles.find( pt ) != city_tiles.end() ) {
                    return false;
                }
                // We hit the edge of the overmap! We're free!
                if( !omap_bounds.contains( pt ) ) {
                    enclosed = false;
                    return false;
                }
                return true;
            };
            // All the points connected to this point that aren't part of a city
            std::vector<point_om_omt> area = ff::point_flood_fill_4_connected( checked, visited, is_unchecked );
            if( !enclosed ) {
                continue;
            }
            // They are enclosed, and so should be considered part of the city.
            city_tiles.reserve( city_tiles.size() + area.size() );
            for( const point_om_omt &pt : area ) {
                city_tiles.insert( pt );
            }
        }
    }
}

static std::map<std::string, std::string> oter_id_migrations;
static std::map<oter_type_str_id, std::pair<translation, faction_id>> camp_migration_map;

void overmap::load_oter_id_migration( const JsonObject &jo )
{
    for( const JsonMember &kv : jo.get_object( "oter_ids" ) ) {
        const std::string old_id = kv.name();
        const std::string new_id = kv.get_string();
        // Allow overriding migrations for omts moved to mods
        if( old_id == new_id ) {
            if( auto it = oter_id_migrations.find( old_id ); it != oter_id_migrations.end() ) {
                oter_id_migrations.erase( it );
            }
        } else {
            // Allow overriding migrations for mods that have better omts to use
            oter_id_migrations.insert_or_assign( old_id, new_id );
        }
    }
}

void overmap::load_oter_id_camp_migration( const JsonObject &jo )
{
    std::string name;
    oter_type_str_id oter;
    faction_id owner;
    JsonObject jsobj = jo.get_object( "camp_migrations" );
    jsobj.read( "name", name );
    jsobj.read( "overmap_terrain", oter );
    jsobj.read( "faction", owner );
    camp_migration_map.emplace( oter,
                                std::pair<translation, faction_id>( translation::to_translation( name ), owner ) );
}

void overmap::reset_oter_id_camp_migrations()
{
    camp_migration_map.clear();
}

void overmap::reset_oter_id_migrations()
{
    oter_id_migrations.clear();
}

bool overmap::oter_id_should_have_camp( const oter_type_str_id &oter )
{
    return camp_migration_map.count( oter ) > 0;
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

void overmap::migrate_camps( const std::vector<tripoint_abs_omt> &points ) const
{
    for( const tripoint_abs_omt &point : points ) {
        std::optional<basecamp *> bcp = overmap_buffer.find_camp( point.xy() );
        if( bcp ) {
            continue; // Already a camp nearby, can't put down another one
        }
        get_map().add_camp( point, "faction_camp", false );
        bcp = overmap_buffer.find_camp( point.xy() );
        if( !bcp ) {
            debugmsg( "Camp placement failed during migration?!" );
            continue;
        }
        basecamp *temp_camp = *bcp;
        const oter_type_str_id &keyvalue = ter( project_remain<coords::om>
                                                ( point ).remainder_tripoint )->get_type_id();
        temp_camp->set_owner( camp_migration_map[keyvalue].second );
        temp_camp->set_name( camp_migration_map[keyvalue].first.translated() );
        temp_camp->define_camp( point, "faction_base_bare_bones_NPC_camp_0", false );
    }
}

oter_id overmap::get_or_migrate_oter( const std::string &oterid )
{
    auto migration = oter_id_migrations.find( oterid );
    if( migration != oter_id_migrations.end() ) {
        return oter_id( migration->second );
    } else {
        return oter_id( oterid );
    }
}

void overmap::place_special_forced( const overmap_special_id &special_id,
                                    const tripoint_om_omt &p,
                                    om_direction::type dir )
{
    static city invalid_city;
    place_special( *special_id, p, dir, invalid_city, false, true );
}
/**
 * Let the group wander aimlessly, unless behaviour is set to city, in which case it will move to the city center.
 */
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
        point_rel_sm delta( rng( -range, range ), rng( -range, range ) );
        target = target_abs + delta;
        interest = 100;
    } else {

        // No city to target, wander aimlessly.
        target = abs_pos.xy() + point_rel_sm( rng( -10, 10 ), rng( -10, 10 ) );
        interest = 30;
    }
}

/**
 * Moves hordes around the map according to their behaviour and target.
 * Also, emerge hordes from monsters that are outside the player's view. Currently only works for zombies.
 */
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
                ++monster_map_it;
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
                ++monster_map_it;
                continue;
            }

            // Only monsters in the open (fields, forests, roads) are eligible to wander
            const oter_id &om_here = ter( project_to<coords::omt>( p ) );
            if( !is_ot_match( "field", om_here, ot_match_type::contains ) &&
                !is_ot_match( "road", om_here, ot_match_type::contains ) &&
                !is_ot_match( "forest", om_here, ot_match_type::prefix ) &&
                !is_ot_match( "swamp", om_here, ot_match_type::prefix ) ) {
                ++monster_map_it;
                continue;
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
                ++monster_map_it;
                continue;
            }

            // Delete the monster, continue iterating.
            monster_map_it = monster_map.erase( monster_map_it );
        }
    }
}

/**
 * Move the nemesis horde towards the player.
 * Currently only works for the first nemesis horde. If there are multiple, only the first one will be moved.
 *
 */
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
        ++it;
    }
    return false;
}

/**
 * Alert hordes to the signal source such as a loud explosion.
 *
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

void overmap::populate_connections_out_from_neighbors( const std::vector<const overmap *>
        &neighbor_overmaps )
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

    populate_for_side( neighbor_overmaps[static_cast<int>( om_direction::type::north )], [](
    const tripoint_om_omt & p ) {
        return p.y() == OMAPY - 1;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( p.x(), 0, p.z() );
    } );
    populate_for_side( neighbor_overmaps[static_cast<int>( om_direction::type::west )], [](
    const tripoint_om_omt & p ) {
        return p.x() == OMAPX - 1;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( 0, p.y(), p.z() );
    } );
    populate_for_side( neighbor_overmaps[static_cast<int>( om_direction::type::south )], [](
    const tripoint_om_omt & p ) {
        return p.y() == 0;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( p.x(), OMAPY - 1, p.z() );
    } );
    populate_for_side( neighbor_overmaps[static_cast<int>( om_direction::type::east )], [](
    const tripoint_om_omt & p ) {
        return p.x() == 0;
    }, []( const tripoint_om_omt & p ) {
        return tripoint_om_omt( OMAPX - 1, p.y(), p.z() );
    } );
}

//in a box made by p1, p2, return { corner point in direction, direction from corner to p2 }
static std::pair<tripoint_om_omt, om_direction::type> closest_corner_in_direction(
    const tripoint_om_omt &p1,
    const tripoint_om_omt &p2, const om_direction::type &direction )
{
    const tripoint_rel_omt diff = p2 - p1;
    const tripoint_rel_omt offset( point_rel_omt( four_adjacent_offsets[static_cast<int>
                                   ( direction )] ), p1.z() );
    const tripoint_om_omt result( p1.x() + offset.x() * abs( diff.x() ),
                                  p1.y() + offset.y() * abs( diff.y() ), p1.z() );
    const tripoint_rel_omt diff2 = p2 - result;
    const point_rel_omt sign( diff2.x() == 0 ? 0 : std::copysign( 1, diff2.x() ),
                              diff2.y() == 0 ? 0 : std::copysign( 1, diff2.y() ) );
    const int cardinal = static_cast<int>( om_direction::size );
    int new_direction;
    for( new_direction = 0; new_direction < cardinal; new_direction++ ) {
        if( point_rel_omt( four_adjacent_offsets[new_direction] ) == sign ) {
            break;
        }
    }
    return std::make_pair( result, om_direction::all[new_direction] );
}

//TODO: generalize
//if point is on edge of overmap, wrap to other end
static tripoint_om_omt wrap_point( const tripoint_om_omt &p )
{
    point wrap( p.x(), p.y() );
    if( wrap.x == OMAPX - 1 || wrap.x == 0 ) {
        wrap.x = abs( wrap.x - ( OMAPX - 1 ) );
    }
    if( wrap.y == OMAPY - 1 || wrap.y == 0 ) {
        wrap.y = abs( wrap.y - ( OMAPY - 1 ) );
    }
    return tripoint_om_omt( wrap.x, wrap.y, p.z() );
}

//checks whether point p is outside square corners of the overmap
//uses two rectangles
static bool overmap_outside_corner( const tripoint_om_omt &p, int corner_length )
{
    const half_open_rectangle<point_om_omt> valid_bounds(
        point_om_omt( corner_length, 0 ),
        point_om_omt( OMAPX - corner_length, OMAPY ) );
    const half_open_rectangle<point_om_omt> valid_bounds_2(
        point_om_omt( 0, corner_length ),
        point_om_omt( OMAPX, OMAPY - corner_length ) );
    return valid_bounds.contains( p.xy() ) || valid_bounds_2.contains( p.xy() );
}

void overmap::place_highway_supported_special( const overmap_special_id &special,
        const tripoint_om_omt &placement, const om_direction::type &dir )
{
    if( placement.is_invalid() ) {
        return;
    }
    place_special_forced( special, placement, dir );
    const std::vector<overmap_special_terrain> locs = special->get_terrains();

    const oter_id fallback_supports = settings->overmap_highway.fallback_supports.obj().get_first();

    for( const overmap_special_terrain &loc : locs ) {
        const tripoint_om_omt rotated = placement + om_direction::rotate( loc.p, dir );
        if( loc.terrain.id() == fallback_supports ) {
            place_special_forced( settings->overmap_highway.segment_bridge_supports,
                                  rotated, dir );
        }
    }
};

std::vector<std::vector<highway_node>> overmap::place_highways(
                                        const std::vector<const overmap *> &neighbor_overmaps )
{
    std::vector<std::vector<highway_node>> paths;
    const point_abs_om this_om = pos();
    const int base_z = RIVER_Z;

    const overmap_highway_settings &highway_settings = settings->overmap_highway;
    // Base distance in overmaps between vertical highways ( whole overmap gap of column_seperation - 1 )
    const int &c_seperation = highway_settings.grid_column_seperation;
    // Base distance in overmaps between horizontal highways ( whole overmap gap of row_seperation - 1 )
    const int &r_seperation = highway_settings.grid_row_seperation;
    // base-level segment OMT to use until finalize_highways()
    const oter_type_str_id &reserved_terrain_id = highway_settings.reserved_terrain_id;
    // upper-level segment OMT to use until finalize_highways()
    const oter_type_str_id &reserved_terrain_water_id = highway_settings.reserved_terrain_water_id;
    const building_bin &bends = highway_settings.bends;
    const overmap_special_id &fallback_onramp = highway_settings.fallback_onramp;
    // Segment of highway bridge for over rivers and lakes
    const overmap_special_id &segment_bridge = settings->overmap_highway.segment_bridge;
    // segment of ramp to connect bridges and non-bridges
    const overmap_special_id &segment_ramp = settings->overmap_highway.segment_ramp;

    //finds longest special in a building collection
    auto find_longest_special = [&]( const building_bin & b ) {
        int longest_length = 0;
        for( const auto &weighted_pair : b.get_all_buildings() ) {
            const overmap_special_id &special = weighted_pair.obj;
            int spec_length = special.obj().longest_side();
            if( spec_length > longest_length ) {
                longest_length = spec_length;
            }
        }
        return longest_length;
    };

    //TODO: cache
    const int longest_slant_length = highway_settings.clockwise_slant.obj().longest_side();
    const int longest_bend_length = find_longest_special( bends );
    const int HIGHWAY_MAX_DEVIANCE = ( longest_bend_length + 1 ) * 2;

    //checks whether the special, if placed and rotated in dir, would replace a single water tile
    auto special_on_water = [&]( const overmap_special_id & special,
    const tripoint_om_omt & placement, om_direction::type dir ) {
        std::vector<overmap_special_locations> locs = special->required_locations();
        for( overmap_special_locations &loc : locs ) {
            if( is_water_body( ter( placement + om_direction::rotate( loc.p, dir ) ) ) ) {
                return true;
            }
        }
        return false;
    };

    //tries to place an intersection special not on water
    //@return point in radius of center without water; invalid if none found
    auto find_intersection_point = [&]( const overmap_special_id & special,
    const tripoint_om_omt & center, const om_direction::type & dir ) {

        if( !special_on_water( special, center, dir ) ) {
            return center;
        }
        tripoint_om_omt not_on_water = tripoint_om_omt::invalid;
        const tripoint_range intersection_radius = points_in_radius( center, HIGHWAY_MAX_DEVIANCE );
        const half_open_rectangle<point_om_omt> bounds( point_om_omt( HIGHWAY_MAX_DEVIANCE,
                HIGHWAY_MAX_DEVIANCE ),
                point_om_omt( OMAPX - HIGHWAY_MAX_DEVIANCE, OMAPY - HIGHWAY_MAX_DEVIANCE ) );
        for( const tripoint_om_omt &p : intersection_radius ) {
            if( bounds.contains( p.xy() ) ) {
                if( !special_on_water( special, p, dir ) ) {
                    not_on_water = p;
                    break;
                }
            }
        }
        //if failure, return invalid point
        return not_on_water;
    };

    std::bitset<HIGHWAY_MAX_CONNECTIONS> ocean_adjacent;
    bool any_ocean_adjacent = false;
    if( get_option<bool>( "OVERMAP_PLACE_OCEANS" ) ) {
        // Not ideal as oceans can start later than these settings but it's at least reliably stopping before them
        const int ocean_start_north = settings->overmap_ocean.ocean_start_north == 0 ? INT_MAX :
                                      settings->overmap_ocean.ocean_start_north;
        const int ocean_start_east = settings->overmap_ocean.ocean_start_east == 0 ? INT_MAX :
                                     settings->overmap_ocean.ocean_start_east;
        const int ocean_start_west = settings->overmap_ocean.ocean_start_west == 0 ? INT_MAX :
                                     settings->overmap_ocean.ocean_start_west;
        const int ocean_start_south = settings->overmap_ocean.ocean_start_south == 0 ? INT_MAX :
                                      settings->overmap_ocean.ocean_start_south;
        // Don't place highways over the ocean
        if( this_om.y() <= -ocean_start_north || this_om.x() >= ocean_start_east ||
            this_om.y() >= ocean_start_south || this_om.x() <= -ocean_start_west ) {
            return paths;
        }
        // Check if we need to place partial highway with different intersections
        ocean_adjacent[0] = this_om.y() - 1 == -ocean_start_north;
        ocean_adjacent[1] = this_om.x() + 1 == ocean_start_east;
        ocean_adjacent[2] = this_om.y() + 1 == ocean_start_south;
        ocean_adjacent[3] = this_om.x() - 1 == -ocean_start_west;
        if( ocean_adjacent.count() == HIGHWAY_MAX_CONNECTIONS ) {
            // This should never happen but would break everything
            debugmsg( "Not placing highways because expecting ocean on all sides?!" );
            return paths;
        }
        any_ocean_adjacent = ocean_adjacent.any();
    }

    if( c_seperation == 0 && r_seperation == 0 ) {
        debugmsg( "Use the external option OVERMAP_PLACE_HIGHWAYS to disable highways instead" );
        return paths;
    }

    // guaranteed intersection close to (but not at) avatar start location
    if( overmap_buffer.highway_global_offset.is_invalid() ) {
        set_highway_global_offset();
        generate_highway_intersection_point( get_highway_global_offset() );
    }

    //resolve connection points, and generate new connection points
    std::array<tripoint_om_omt, HIGHWAY_MAX_CONNECTIONS> end_points;
    std::fill( end_points.begin(), end_points.end(), tripoint_om_omt::invalid );
    std::optional<std::bitset<HIGHWAY_MAX_CONNECTIONS>> is_highway_neighbors = is_highway_overmap();
    std::bitset<HIGHWAY_MAX_CONNECTIONS> neighbor_connections;
    if( !is_highway_neighbors ) {
        add_msg_debug( debugmode::DF_HIGHWAY, "This overmap (%s) is NOT a highway overmap.",
                       loc.to_string_writable() );
        return paths;
    } else {
        neighbor_connections = *is_highway_neighbors;
        add_msg_debug( debugmode::DF_HIGHWAY,
                       "This overmap (%s) IS a highway overmap, with required connections: %s, %s, %s, %s",
                       loc.to_string_writable(),
                       neighbor_connections[0] ? "NORTH" : "",
                       neighbor_connections[1] ? "EAST" : "",
                       neighbor_connections[2] ? "SOUTH" : "",
                       neighbor_connections[3] ? "WEST" : "" );

        //if there are adjacent oceans, cut highway connections
        if( any_ocean_adjacent ) {
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                if( ocean_adjacent[i] ) {
                    neighbor_connections[i] = false;
                }
            }
        }

        std::bitset<HIGHWAY_MAX_CONNECTIONS> new_end_point;
        for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
            if( neighbor_connections[i] ) {
                if( neighbor_overmaps[i] != nullptr ) {
                    //neighboring overmap highway edge connects to this overmap
                    const tripoint_om_omt opposite_edge = neighbor_overmaps[i]->highway_connections[( i + 2 ) %
                                                          HIGHWAY_MAX_CONNECTIONS];
                    if( opposite_edge == tripoint_om_omt::zero ) {
                        debugmsg( "highway connections not initialized; inter-overmap highway pathing failed" );
                        return paths;
                    }
                    end_points[i] = wrap_point( opposite_edge );
                } else {
                    new_end_point[i] = true;
                }
            }
        }

        //if going N/S or E/W, new highways tend to go straight through an overmap
        for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
            if( new_end_point[i] ) {
                tripoint_om_omt to_wrap = end_points[( i + 2 ) % HIGHWAY_MAX_CONNECTIONS];
                tripoint_om_omt fallback_random_point = random_entry(
                        get_border( point_rel_om( four_adjacent_offsets[i] ), base_z, HIGHWAY_MAX_DEVIANCE ) );
                if( to_wrap.is_invalid() ) {
                    end_points[i] = fallback_random_point;
                } else {
                    //additional check for an almost-straight point instead of a completely random point
                    if( !x_in_y( highway_settings.straightness_chance, 1.0 ) ) {
                        tripoint_om_omt wrapped_point = wrap_point( to_wrap );
                        std::vector<tripoint_om_omt> border_points = get_border( point_rel_om( four_adjacent_offsets[i] ),
                                base_z, HIGHWAY_MAX_DEVIANCE );
                        std::vector<tripoint_om_omt> border_points_in_radius;
                        for( const tripoint_om_omt &p : border_points ) {
                            if( rl_dist( p, wrapped_point ) < HIGHWAY_MAX_DEVIANCE &&
                                overmap_outside_corner( p, HIGHWAY_MAX_DEVIANCE ) ) {
                                border_points_in_radius.emplace_back( p );
                            }
                        }
                        end_points[i] = random_entry( border_points_in_radius );
                    } else {
                        end_points[i] = fallback_random_point;
                    }
                }
                if( is_water_body( ter( end_points[i] ) ) ) {
                    end_points[i] += tripoint_rel_omt( 0, 0, 1 );
                }
            }
        }
    }

    //highways align to N/E, so their specials must offset due to rotation
    auto highway_special_offset = [&]( tripoint_om_omt & special_point,
    om_direction::type special_dir ) {
        if( special_point.is_invalid() ) {
            return;
        }
        std::array<tripoint, HIGHWAY_MAX_CONNECTIONS> bend_offset =
        { tripoint::zero, tripoint::east, tripoint::south_east, tripoint::south };
        special_point += bend_offset[static_cast<int>( special_dir )];
    };

    //highways align to N/E, so their segments must offset due to rotation
    auto highway_segment_offset = [&]( tripoint_om_omt & segment_point,
    om_direction::type segment_dir ) {
        if( segment_point.is_invalid() ) {
            return;
        }
        std::array<tripoint, HIGHWAY_MAX_CONNECTIONS> segment_offsets =
        { tripoint::zero, tripoint::zero, tripoint::east, tripoint::south };

        segment_point += segment_offsets[static_cast<int>( segment_dir )];
    };

    auto segment_special = [&highway_settings]( bool raised ) {
        return raised ? highway_settings.segment_bridge :
               highway_settings.segment_flat;
    };

    //draw a line segment of highway, placing slants
    //draw_direction is from sp1 to sp2
    auto draw_highway_line = [&]( const tripoint_om_omt & sp1, const tripoint_om_omt & sp2,
    const om_direction::type & draw_direction, int path_init_z, int base_z ) {

        const point_rel_omt draw_direction_vector =
            point_rel_omt( four_adjacent_offsets[static_cast<int>( draw_direction )] );
        add_msg_debug( debugmode::DF_HIGHWAY,
                       "overmap (%s) attempting to draw line segment from %s to %s.",
                       loc.to_string_writable(), sp1.to_string_writable(), sp2.to_string_writable() );
        const tripoint_rel_omt diff = sp2 - sp1;
        const tripoint_rel_omt abs_diff = diff.abs();
        int minimum_slants = std::min( abs_diff.x(), abs_diff.y() ); //assumes length > slants

        bool slants_optional = minimum_slants == 0;
        std::vector<highway_node> slant_path;

        tripoint_rel_omt slant_direction_vector;
        const tripoint_rel_omt slant_vector( draw_direction_vector * longest_slant_length, 0 );
        bool clockwise = one_in( 2 );
        //TODO: generalize
        if( !slants_optional ) {
            //get mandatory slant direction
            point_rel_omt rotate_slant( draw_direction_vector.x() == 0 ? 1 : 0,
                                        draw_direction_vector.y() == 0 ? 1 : 0 );
            slant_direction_vector = tripoint_rel_omt(
                                         ( abs_diff.x() != 0 ? diff.x() / abs_diff.x() : 0 ) * rotate_slant.x(),
                                         ( abs_diff.y() != 0 ? diff.y() / abs_diff.y() : 0 ) * rotate_slant.y(), 0 );
            clockwise = point_rel_omt(
                            four_adjacent_offsets[static_cast<int>(
                                    om_direction::turn_right( draw_direction ) )] ) == slant_direction_vector.xy();
        }

        // base z for testing terrain
        tripoint_om_omt slant_current_point = tripoint_om_omt( sp1.xy(), base_z );
        int previous_z = path_init_z;
        //the first placed node may not be a slant
        bool no_first_slant = true;
        const overmap_special_id &selected_slant =
            clockwise ? highway_settings.clockwise_slant :
            highway_settings.counterclockwise_slant;
        while( slant_current_point.xy() != sp2.xy() + draw_direction_vector ) {
            if( !inbounds( slant_current_point ) ) {
                debugmsg( "highway slant pathing out of bounds; falling back to onramp" );
                slant_path.clear();
                return slant_path;
            }
            //leave space for ramps
            bool z_change =
                special_on_water( selected_slant, slant_current_point, draw_direction ) ^
                special_on_water( selected_slant, slant_current_point + slant_vector + slant_direction_vector,
                                  draw_direction );
            if( !no_first_slant && ( minimum_slants > 0 && !z_change ) ) {
                if( slants_optional ) {
                    clockwise = !clockwise;
                    slant_direction_vector = tripoint_rel_omt(
                                                 point_rel_omt( four_adjacent_offsets[static_cast<int>( clockwise ?
                                                                           om_direction::turn_right( draw_direction ) :
                                                                           om_direction::turn_left( draw_direction ) )] ), 0 );
                }
                tripoint_om_omt slant_pos = slant_current_point;
                //because highways are locked to N/E, account for S/W offset
                if( draw_direction == om_direction::type::south ||
                    draw_direction == om_direction::type::west ) {
                    slant_pos += tripoint_rel_omt( point_rel_omt( four_adjacent_offsets[
                                    static_cast<int>( om_direction::turn_left( draw_direction ) )] ), previous_z );
                }
                slant_path.emplace_back( slant_pos, draw_direction, selected_slant, false );
                slant_current_point += slant_vector + slant_direction_vector;
                minimum_slants--;
            } else {
                bool is_segment_water = is_water_body( ter( slant_current_point ) );
                previous_z = is_segment_water ? base_z + 1 : base_z;
                slant_path.emplace_back(
                    tripoint_om_omt( slant_current_point.xy(), previous_z ), draw_direction,
                    segment_special( is_segment_water ) );
                slant_current_point += tripoint_rel_omt( draw_direction_vector, 0 );
            }
            no_first_slant = false;
        }

        return slant_path;
    };

    //truncates highway segment by bend length, accounting for offset
    auto truncate_segment = [&]( om_direction::type dir, int last_bend_length ) {
        tripoint_om_omt bend_offset_base( 0, 0, 0 );
        highway_special_offset( bend_offset_base, dir );
        point_rel_omt current_direction( four_adjacent_offsets[static_cast<int>( dir )] );
        point_rel_omt bend_length_vector = current_direction * last_bend_length;
        return point_rel_omt( bend_offset_base.x() * current_direction.x() - bend_length_vector.x(),
                              bend_offset_base.y() * current_direction.y() - bend_length_vector.y() );
    };

    //handle ramp placement given z-levels of path nodes
    auto handle_ramps = [&]( std::vector<highway_node> &path, int base_z ) {
        const int range = path.size();

        int current_z = path[0].path_node.pos.z();
        int last_z = current_z;

        auto fill_bridge = [&segment_bridge]( highway_node & node ) {
            if( node.is_segment ) {
                node.placed_special = segment_bridge;
                node.path_node.pos += tripoint_rel_omt( 0, 0, 1 );
                node.is_ramp = false;
            }
        };

        for( int i = 0; i < range; i++ ) {
            current_z = path[i].path_node.pos.z();
            if( current_z != last_z ) {
                if( current_z == base_z + 1 ) {
                    if( i > 0 && path[i - 1].is_segment ) {
                        path[i - 1].is_ramp = true;
                    }
                } else if( current_z == base_z && path[i].is_segment ) {
                    bool place_ramp = true;
                    if( i + 1 < range ) {
                        if( path[i + 1].path_node.pos.z() == base_z ) {
                            if( i + 2 < range ) {
                                if( path[i + 2].path_node.pos.z() == base_z + 1 ) {
                                    fill_bridge( path[i] );
                                    fill_bridge( path[i + 1] );
                                    place_ramp = false;
                                    current_z = base_z + 1;
                                }
                            }
                        } else {
                            fill_bridge( path[i] );
                            place_ramp = false;
                            current_z = base_z + 1;
                        }
                    }

                    if( place_ramp ) {
                        path[i].is_ramp = true;
                        path[i].ramp_down = true;
                    }
                }
            }
            last_z = current_z;
        }

        //place ramp specials
        for( int i = 0; i < range; i++ ) {
            if( path[i].is_ramp ) {
                om_direction::type ramp_dir = path[i].path_node.dir;
                tripoint_om_omt ramp_offset = path[i].path_node.pos;
                if( path[i].ramp_down ) {
                    ramp_dir = om_direction::opposite( ramp_dir );
                }
                highway_segment_offset( ramp_offset, ramp_dir );
                place_special_forced( segment_ramp, ramp_offset, ramp_dir );
            }
        }
    };

    // draws highway from p1 to p2
    // dir1/dir2 are the directions from which p1/p2 are being drawn
    // e.g. from west edge of OM to south edge would be 3/2
    // connecting p1 to p2 may have no more than 2 bends
    auto draw_reserved_highway = [&]( const tripoint_om_omt & p1, const tripoint_om_omt & p2, int dir1,
                                      int dir2,
    int base_z ) {

        std::vector<highway_node> highway_path;
        om_direction::type direction1 = om_direction::all[dir1];
        om_direction::type direction2 = om_direction::all[dir2];
        cata_assert( direction1 != direction2 );
        //reverse directions for drawing
        direction1 = om_direction::opposite( direction1 );
        direction2 = om_direction::opposite( direction2 );
        bool parallel = om_direction::are_parallel( direction1, direction2 );
        bool north_south = direction1 == om_direction::type::north ||
                           direction1 == om_direction::type::south;

        //if either point is invalid, fallback to onramp for valid points only
        bool p1_invalid = p1.is_invalid();
        bool p2_invalid = p2.is_invalid();
        if( !p1_invalid ) {
            if( !overmap_outside_corner( p1, HIGHWAY_MAX_DEVIANCE ) ) {
                debugmsg( "highway path start point outside of expected deviance" );
            }
        }
        if( !p2_invalid ) {
            if( !overmap_outside_corner( p2, HIGHWAY_MAX_DEVIANCE ) ) {
                debugmsg( "highway path end point outside of expected deviance" );
            }
        }

        if( p1_invalid || p2_invalid ) {
            if( !p1_invalid ) {
                tripoint_om_omt p1_rotated = p1;
                highway_segment_offset( p1_rotated, direction1 );
                place_special_forced( fallback_onramp, p1_rotated, direction1 );
            }
            if( !p2_invalid ) {
                tripoint_om_omt p2_rotated = p2;
                highway_segment_offset( p2_rotated, direction2 );
                place_special_forced( fallback_onramp, p2_rotated, direction2 );
            }
            add_msg_debug( debugmode::DF_HIGHWAY,
                           "overmap (%s) took invalid points and is falling back to onramp.",
                           loc.to_string_writable() );
            return highway_path;
        }

        add_msg_debug( debugmode::DF_HIGHWAY, "overmap (%s) attempting to draw bend from %s to %s.",
                       loc.to_string_writable(), p1.to_string_writable(), p2.to_string_writable() );

        std::vector<std::pair<tripoint_om_omt, om_direction::type>> bend_points;
        bool bend_draw_mode = !( p1.x() == p2.x() || p1.y() == p2.y() );

        //determine bends; z-values are not accounted for here
        if( bend_draw_mode ) {
            //we need to draw two bends
            if( parallel ) {
                bool two_bends = false;
                if( north_south ) { // N/S
                    two_bends |= ( abs( p1.x() - p2.x() ) > HIGHWAY_MAX_DEVIANCE );
                } else { // E/W
                    two_bends |= ( abs( p1.y() - p2.y() ) > HIGHWAY_MAX_DEVIANCE );
                }
                if( two_bends ) {
                    tripoint_om_omt bend_midpoint = midpoint( p1, p2 );
                    bend_points.emplace_back( closest_corner_in_direction( p1, bend_midpoint, direction1 ) );
                    bend_points.emplace_back( closest_corner_in_direction( bend_midpoint, p2,
                                              bend_points.back().second ) );
                } else {
                    bend_draw_mode = false;
                }
            } else { //we need to draw exactly one bend (p1->p2)
                bend_points.emplace_back( closest_corner_in_direction( p1, p2, direction1 ) );
            }
        }

        //the z-value of any tripoints is an indicator for whether the returned highway path is above water
        //but all reserved highway OMTs are placed at z=0
        //the current direction the highway path is traveling (also as vector)
        om_direction::type current_direction_index = direction1;
        point_rel_omt current_direction = point_rel_omt( four_adjacent_offsets[static_cast<int>
                                          ( current_direction_index )] );

        if( bend_draw_mode ) {
            if( bend_points.empty() ) {
                debugmsg( "no highway bends found when expected!" );
            }

            //determine all bends before placement
            std::vector<overmap_special_id> precalc_bends;
            //pairs of start/end points to build segments with
            std::vector<std::pair<tripoint_om_omt, tripoint_om_omt>> path_points;

            tripoint_om_omt previous_point = p1;
            for( const std::pair<tripoint_om_omt, om_direction::type> &p : bend_points ) {
                precalc_bends.emplace_back( bends.pick() );
                path_points.emplace_back( previous_point, p.first );
                previous_point = p.first;
            }
            //last segment on path
            path_points.emplace_back( previous_point, p2 );

            //draw raw paths

            const int path_point_count = path_points.size();
            //bend size is guaranteed to be path count - 1
            const int bend_points_size = bend_points.size();
            overmap_special_id selected_bend;
            int last_bend_length = 0;;
            for( int i = 0; i < path_point_count; i++ ) {
                //truncate path length by bend length
                bool draw_bend = i < bend_points_size;
                if( draw_bend ) {
                    selected_bend = precalc_bends[i];
                    last_bend_length = selected_bend->longest_side();
                }
                om_direction::type new_bend_direction;
                int bend_z = base_z;

                if( i > 0 ) {
                    //start of segment
                    path_points[i].first = tripoint_om_omt( path_points[i].first.xy() +
                                                            truncate_segment( om_direction::opposite( bend_points[i - 1].second ), last_bend_length ),
                                                            path_points[i].first.z() );
                }
                if( draw_bend ) {
                    //end of segment
                    path_points[i].second = tripoint_om_omt( path_points[i].second.xy() +
                                            truncate_segment( current_direction_index, last_bend_length ),
                                            path_points[i].second.z() );
                }
                std::vector<highway_node> path = draw_highway_line( path_points[i].first,
                                                 path_points[i].second, current_direction_index, path_points[i].first.z(), base_z );
                if( path.empty() ) {
                    place_special_forced( fallback_onramp, path_points[i].first,
                                          om_direction::opposite( current_direction_index ) );
                    place_special_forced( fallback_onramp, path_points[i].second, current_direction_index );
                }
                for( const highway_node &p : path ) {
                    highway_path.emplace_back( p );
                }

                if( draw_bend ) {
                    std::pair<tripoint_om_omt, om_direction::type> bend_point = bend_points[i];
                    //find bend special rotation
                    bool clockwise = om_direction::turn_right( current_direction_index ) == bend_point.second;
                    om_direction::type bend_direction = clockwise ? current_direction_index : om_direction::turn_right(
                                                            current_direction_index );
                    tripoint_om_omt bend_pos = bend_point.first;
                    highway_special_offset( bend_pos, bend_direction );
                    bend_z = special_on_water( selected_bend, bend_pos, bend_direction ) ? base_z + 1 : base_z;
                    bend_pos = tripoint_om_omt( bend_pos.xy(), bend_z );

                    highway_path.emplace_back( bend_pos, bend_direction, selected_bend, false );

                    new_bend_direction = bend_point.second;
                    current_direction_index = new_bend_direction;
                    current_direction = point_rel_omt( four_adjacent_offsets[static_cast<int>
                                                       ( current_direction_index )] );

                }
            }
        } else {
            //quick path for no bends
            std::vector<highway_node> straight_line = draw_highway_line( p1, p2, current_direction_index,
                    p1.z(),
                    base_z );
            for( const highway_node &p : straight_line ) {
                highway_path.emplace_back( p );
            }
        }

        //segments adjacent to specials must have the special's z-value for correct ramp handling
        const int path_size = highway_path.size();
        for( int i = 1; i < path_size - 1; i++ ) {
            const highway_node &current_node = highway_path[i];
            if( !current_node.is_segment ) {
                int special_z = current_node.path_node.pos.z();
                bool raised = special_z == base_z + 1;
                highway_node &next_node = highway_path[i + 1];
                highway_node &prev_node = highway_path[i - 1];
                if( next_node.is_segment ) {
                    next_node.path_node.pos = tripoint_om_omt( next_node.path_node.pos.xy(), special_z );
                    next_node.placed_special = segment_special( raised );
                }
                if( prev_node.is_segment ) {
                    prev_node.path_node.pos = tripoint_om_omt( prev_node.path_node.pos.xy(), special_z );
                    prev_node.placed_special = segment_special( raised );
                }
            }
        }

        //place reserved terrain and specials on path
        for( const highway_node &node : highway_path ) {
            pf::directed_node<tripoint_om_omt> node_z0( tripoint_om_omt(
                        node.path_node.pos.xy(), base_z ), node.path_node.dir );
            if( node.is_segment ) {
                int node_z = node.path_node.pos.z();
                bool reserve_water = node_z > base_z;

                std::vector<overmap_special_locations> locs = node.placed_special->required_locations();
                for( overmap_special_locations &loc : locs ) {
                    const oter_type_str_id z0_terrain_to_place = reserve_water ?
                            reserved_terrain_water_id : reserved_terrain_id;
                    om_direction::type reserved_direction = node.get_effective_dir();

                    tripoint_om_omt rotated = node.path_node.pos +
                                              om_direction::rotate( loc.p, reserved_direction );
                    ter_set( tripoint_om_omt( rotated.xy(), base_z ),
                             z0_terrain_to_place.obj().get_rotated( reserved_direction ) );
                    //prevent road bridges intersecting
                    if( rotated.z() == base_z + 1 ) {
                        ter_set( rotated, z0_terrain_to_place.obj().get_rotated( reserved_direction ) );
                    }
                }
            } else {
                place_highway_supported_special( node.placed_special,
                                                 node.path_node.pos, node.get_effective_dir() );
            }
        }
        handle_ramps( highway_path, base_z );
        return highway_path;
    };

    int neighbor_connection_count = neighbor_connections.count();
    overmap_special_id special;
    overmap_special_id fallback_special;

    //determine existing intersections/curves
    switch( neighbor_connection_count ) {
        case 2: {
            //draw end-to-end
            if( neighbor_connections[0] && neighbor_connections[2] ) {
                paths.emplace_back( draw_reserved_highway( end_points[0], end_points[2], 0, 2, base_z ) );
            } else if( neighbor_connections[1] && neighbor_connections[3] ) {
                paths.emplace_back( draw_reserved_highway( end_points[1], end_points[3], 1, 3, base_z ) );
            } else {
                for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                    int next_edge = ( i + 1 ) % HIGHWAY_MAX_CONNECTIONS;
                    if( neighbor_connections[i] && neighbor_connections[next_edge] ) {
                        paths.emplace_back( draw_reserved_highway( end_points[i], end_points[next_edge], i, next_edge,
                                            base_z ) );
                    }
                }
            }
            break;
        }
        case 3: {
            om_direction::type empty_direction = om_direction::type::north;
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                if( !neighbor_connections[i] ) {
                    empty_direction = om_direction::type( i );
                    break;
                }
            }

            //draw 3-way intersection (favors towards closer pair of points if possible)
            //this will be very rare, usually only for ocean-adjacent OMs
            tripoint_om_omt three_point( OMAPX / 2, OMAPY / 2, 0 );
            //draw end-to-end from end points to 3-way intersection
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                if( i != static_cast<int>( empty_direction ) ) {
                    paths.emplace_back( draw_reserved_highway( end_points[i], three_point, i, ( i + 2 ) % 4,
                                        RIVER_Z ) );
                }
            }
            fallback_special = settings->overmap_highway.fallback_three_way_intersection;
            special = settings->overmap_highway.three_way_intersections.pick();
            highway_special_offset( three_point, empty_direction );
            place_highway_supported_special( special, three_point, empty_direction );
            break;
        }
        case 4: {
            // first, check pairs of corners -- we can't draw a pair of bends to a center intersection if there's no room!
            tripoint_om_omt four_point( OMAPX / 2, OMAPY / 2, 0 );
            const double corner_threshold = OMAPX / 3.0;
            std::bitset<HIGHWAY_MAX_CONNECTIONS> corners_close; //starting at NE
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                corners_close[i] = rl_dist( end_points[i],
                                            end_points[( i + 1 ) % HIGHWAY_MAX_CONNECTIONS] ) < corner_threshold * std::sqrt( 2.0 );
            }
            if( corners_close.count() == 1 ) {
                //for one pair of close corners, set the intersection to their shared point
                for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                    if( corners_close[i] ) {
                        four_point = closest_corner_in_direction( end_points[i],
                                     end_points[( i + 1 ) % HIGHWAY_MAX_CONNECTIONS],
                                     om_direction::opposite( om_direction::all[i] ) ).first;
                    }
                }
            } else if( corners_close.count() == 2 ) {
                //if there are two pairs of close corners, draw two 3-way intersections at corners and connect them
                const int THREE_WAY_COUNT = 2;
                std::array<tripoint_om_omt, THREE_WAY_COUNT> three_way_intersections;
                std::array<om_direction::type, THREE_WAY_COUNT> three_way_direction;
                special = settings->overmap_highway.three_way_intersections.pick();
                for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                    if( corners_close[i] ) {
                        const int dir1 = i;
                        const int dir2 = ( i + 1 ) % HIGHWAY_MAX_CONNECTIONS;
                        const tripoint_om_omt &end1 = end_points[dir1];
                        const tripoint_om_omt &end2 = end_points[dir2];
                        const int three_way_index = i / 2;

                        om_direction::type opposite_dir = om_direction::opposite( om_direction::all[dir1] );
                        three_way_direction[three_way_index] = opposite_dir;
                        three_way_intersections[three_way_index] = find_intersection_point( special,
                                closest_corner_in_direction( end1, end2, opposite_dir ).first, opposite_dir );
                        paths.emplace_back( draw_reserved_highway( end1,
                                            three_way_intersections[three_way_index], dir1, ( dir1 + 2 ) % HIGHWAY_MAX_CONNECTIONS, RIVER_Z ) );
                        paths.emplace_back( draw_reserved_highway( end2,
                                            three_way_intersections[three_way_index], dir2, ( dir2 + 2 ) % HIGHWAY_MAX_CONNECTIONS, RIVER_Z ) );
                    }
                }
                paths.emplace_back( draw_reserved_highway( three_way_intersections[0], three_way_intersections[1],
                                    static_cast<int>( om_direction::turn_left( three_way_direction[0] ) ),
                                    static_cast<int>( om_direction::turn_left( three_way_direction[1] ) ), base_z ) );

                for( int i = 0; i < THREE_WAY_COUNT; i++ ) {
                    highway_special_offset( three_way_intersections[i], three_way_direction[i] );
                    place_highway_supported_special( special, three_way_intersections[i], three_way_direction[i] );
                }
                break;
            }

            special = settings->overmap_highway.four_way_intersections.pick();
            om_direction::type intersection_dir = om_direction::type::north;
            four_point = find_intersection_point( special, four_point, intersection_dir );

            //draw end-to-end from ends points to 4-way intersection
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                paths.emplace_back( draw_reserved_highway( end_points[i], four_point, i, ( i + 2 ) % 4,
                                    base_z ) );
            }
            fallback_special = settings->overmap_highway.fallback_four_way_intersection;
            place_highway_supported_special( special, four_point, om_direction::type::north );
            break;
        }
        default: // 1
            //this shouldn't happen often, but if it does, end the highway at overmap edges
            //by drawing start point -> invalid point
            tripoint_om_omt dummy_point = tripoint_om_omt::invalid;
            for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                if( !end_points[i].is_invalid() ) {
                    paths.emplace_back( draw_reserved_highway( end_points[i], dummy_point, i, ( i + 2 ) % 4,
                                        base_z ) );
                }
            }
    }

    highway_connections = end_points;
    return paths;
}

void overmap::place_highway_supports( const tripoint_om_omt &p, om_direction::type dir, int base_z,
                                      bool is_segment )
{
    //ignores non-water terrain, but only once
    bool at_least_one = p.z() > base_z;
    tripoint_om_omt current_point = p + tripoint_rel_omt::below;
    while( at_least_one || ( is_water_body( ter( current_point ) )
                             && current_point.z() >= -OVERMAP_DEPTH ) ) {
        at_least_one = false;
        if( is_segment ) {
            place_special_forced( settings->overmap_highway.segment_bridge_supports,
                                  current_point, dir );
        } else {
            //ter_set( current_point, settings->overmap_highway.fallback_supports.obj().get_first() );
        }
        current_point += tripoint_rel_omt::below;
    }
}

std::pair<std::vector<tripoint_om_omt>, om_direction::type>
overmap::get_highway_segment_points( const pf::directed_node<tripoint_om_omt> &node ) const
{
    //highway segments can only draw N/E
    om_direction::type draw_dir = om_direction::type( static_cast<int>( node.dir ) % 2 );
    const tripoint_rel_omt draw_point_dir( point_rel_omt( four_adjacent_offsets[static_cast<int>
                                           ( om_direction::turn_right( draw_dir ) )] ), 0 );
    const tripoint_om_omt z0( node.pos.xy(), RIVER_Z );
    std::vector<tripoint_om_omt> draw_points = { z0, z0 + draw_point_dir };
    return std::make_pair( draw_points, draw_dir );
}

void overmap::finalize_highways( std::vector<std::vector<highway_node>> &paths )
{
    // Segment of flat highway with a road bridge
    const overmap_special_id &segment_road_bridge = settings->overmap_highway.segment_road_bridge;

    // Replace roads that travelled over reserved highway segments
    auto handle_road_bridges = [&]( std::vector<highway_node> &path ) {
        const int range = path.size();
        for( int i = 0; i < range; i++ ) {
            //only one of two points should ever need to be checked for roads specifically
            if( is_road( ter( path[i].path_node.pos ) ) ) {
                path[i].placed_special = segment_road_bridge;
            }
        }
    };

    for( std::vector<highway_node> &path : paths ) {
        if( path.empty() ) {
            continue;
        }
        handle_road_bridges( path );

        for( const highway_node &node : path ) {
            if( !is_highway_special( ter( node.path_node.pos ) ) && node.is_segment && !node.is_ramp ) {
                place_highway_supported_special( node.placed_special,
                                                 node.path_node.pos, node.get_effective_dir() );
            }
        }
    }
}

void overmap::set_highway_global_offset()
{
    //this only happens exactly once, upon generation of the first overmap
    const tripoint_abs_om center = project_to<coords::om>( get_avatar().pos_abs() );
    overmap_buffer.highway_global_offset = point_abs_om( center.xy() );
}

std::optional<std::bitset<HIGHWAY_MAX_CONNECTIONS>> overmap::is_highway_overmap() const
{
    point_abs_om pos = this->pos();
    std::optional<std::bitset<HIGHWAY_MAX_CONNECTIONS>> result;

    // first, get the offset for every overmap_highway_intersection_point bounding this overmap
    // generate points if needed
    std::vector<point_abs_om> bounds = find_highway_intersection_bounds( pos );

    // if we're on one of those intersection OMs, return immediately with all cardinal connection points
    for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
        point_abs_om intersection_grid = bounds[i];
        if( overmap_buffer.get_overmap_highway_intersection_point( intersection_grid ).offset_pos == pos ) {
            return result.emplace( "1111" );
        }
    }

    //otherwise, check EVERY path connected to bounds, generating if necessary
    std::vector<std::string> path_cache;

    auto check_on_path = [&pos, &path_cache]( const point_abs_om & center,
    const point_abs_om & adjacent ) {
        std::optional<std::vector<point_abs_om>> resulting_path;

        //orthogonal line must always take a set of two points in the same order for highway overmap paths
        std::vector<point_abs_om> ordered_points = { center, adjacent };
        std::sort( ordered_points.begin(), ordered_points.end(), []( const point_abs_om & p1,
        const point_abs_om & p2 ) {
            return p1.y() == p2.y() ? p1.x() < p2.x() : p1.y() < p2.y();
        } );

        //repeat paths are possible; ignore them
        std::string point_string = ordered_points.front().to_string() + ordered_points.back().to_string();
        if( std::find( path_cache.begin(), path_cache.end(), point_string ) != path_cache.end() ) {
            return resulting_path;
        }
        path_cache.emplace_back( point_string );
        std::vector<point_abs_om> highway_oms = orthogonal_line_to( ordered_points.front(),
                                                ordered_points.back() );

        std::string test;
        for( const point_abs_om &p : highway_oms ) {
            test += p.to_string() + " ";
        }
        add_msg_debug( debugmode::DF_HIGHWAY, "for %s to %s, checking path: %s",
                       center.to_string_writable(),
                       adjacent.to_string_writable(), test );

        if( std::find( highway_oms.begin(), highway_oms.end(), pos ) != highway_oms.end() ) {
            resulting_path.emplace( highway_oms );
        }
        return resulting_path;
    };

    std::vector<std::optional<std::vector<point_abs_om>>> highway_oms;
    for( const point_abs_om &bound_point : bounds ) {
        overmap_highway_intersection_point bound_intersection =
            overmap_buffer.get_overmap_highway_intersection_point( bound_point );
        for( const overmap_highway_intersection_point &intersection : find_highway_adjacent_intersections(
                 bound_intersection.grid_pos ) ) {
            highway_oms.emplace_back( check_on_path( bound_intersection.offset_pos, intersection.offset_pos ) );
        }
    }

    std::bitset<HIGHWAY_MAX_CONNECTIONS> connections;
    for( std::optional<std::vector<point_abs_om>> path : highway_oms ) {
        if( !!path ) {
            for( const point_abs_om &p : *path ) {
                for( int i = 0; i < HIGHWAY_MAX_CONNECTIONS; i++ ) {
                    if( p == pos + point_rel_om( four_adjacent_offsets[i] ) ) {
                        connections[i] = true;
                    }
                }
            }
            return result.emplace( connections );
        }
    }
    return result;
}

bool overmap::highway_intersection_exists( const point_abs_om &intersection_om ) const
{
    return overmap_buffer.highway_intersections.find( intersection_om.to_string_writable() ) !=
           overmap_buffer.highway_intersections.end();
}

point_abs_om overmap::get_highway_global_offset() const
{
    return overmap_buffer.highway_global_offset;
}

void overmap_highway_intersection_point::generate_offset( int intersection_max_radius )
{
    auto no_lakes = []( const point_abs_om & pt ) {
        //TODO: this can't be correct usage of default region settings...
        return !overmap::has_lake( pt,
                                   region_settings_map["default"].overmap_lake.noise_threshold_lake );
    };
    tripoint_abs_om as_tripoint( grid_pos, 0 );
    tripoint_range radius = points_in_radius( as_tripoint, intersection_max_radius );
    std::vector<point_abs_om> intersection_candidates;

    for( const tripoint_abs_om &p : radius ) {
        if( p != as_tripoint && no_lakes( p.xy() ) ) { //intersection cannot generate at origin
            intersection_candidates.emplace_back( p.xy() );
        }
    }
    if( intersection_candidates.empty() ) {
        debugmsg( "no highway intersections could be generated in overmap radius of %d at overmap %s; reduce lake frequency",
                  intersection_max_radius, grid_pos.to_string() );
    }
    offset_pos = random_entry( intersection_candidates );
}

void overmap_highway_intersection_point::serialize( JsonOut &out ) const
{
    out.start_object();
    out.member( "grid_pos", grid_pos );
    out.member( "offset_pos", offset_pos );
    out.member( "adjacent_intersections", adjacent_intersections );
    out.end_object();
}

void overmap_highway_intersection_point::deserialize( const JsonObject &obj )
{
    obj.read( "grid_pos", grid_pos );
    obj.read( "offset_pos", offset_pos );
    obj.read( "adjacent_intersections", adjacent_intersections );
}

void overmap::generate_highway_intersection_point( const point_abs_om &generated_om_pos ) const
{
    const int &intersection_max_radius = settings->overmap_highway.intersection_max_radius;
    if( !highway_intersection_exists( generated_om_pos ) ) {
        overmap_highway_intersection_point new_intersection( generated_om_pos );
        new_intersection.generate_offset( intersection_max_radius );
        add_msg_debug( debugmode::DF_HIGHWAY, "Generated intersection at overmap %s.",
                       new_intersection.offset_pos.to_string_writable() );
        overmap_buffer.highway_intersections.insert( { generated_om_pos.to_string_writable(), new_intersection} );
    }
}

std::vector<point_abs_om> overmap::find_highway_intersection_bounds( const point_abs_om
        &generated_om_pos ) const
{
    const int &c_seperation = settings->overmap_highway.grid_column_seperation;
    const int &r_seperation = settings->overmap_highway.grid_row_seperation;

    point_abs_om center = overmap_buffer.highway_global_offset;
    point_rel_om diff = generated_om_pos - center;
    int colsign = std::copysign( 1.0, diff.x() / static_cast<double>( c_seperation ) );
    int rowsign = std::copysign( 1.0, diff.y() / static_cast<double>( r_seperation ) );
    // these are returned in clockwise order AND so the furthest from the highway origin is last
    std::vector<point_abs_om> bounds = { point_abs_om( center.x(), center.y() + rowsign * r_seperation ),
                                         point_abs_om( center.x(), center.y() ),
                                         point_abs_om( center.x() + colsign * c_seperation, center.y() ),
                                         point_abs_om( center.x() + colsign * c_seperation, center.y() + rowsign * r_seperation )
                                       };
    for( const point_abs_om &p : bounds ) {
        if( !highway_intersection_exists( p ) ) {
            generate_highway_intersection_point( p );
        }
    }
    return bounds;
}

std::vector<overmap_highway_intersection_point>
overmap::find_highway_adjacent_intersections(
    const point_abs_om &generated_om_pos ) const
{
    const int &c_seperation = settings->overmap_highway.grid_column_seperation;
    const int &r_seperation = settings->overmap_highway.grid_row_seperation;

    //generate adjacent intersection points
    std::vector<overmap_highway_intersection_point> adjacent_intersections;
    const int adjacencies = four_adjacent_offsets.size();
    for( int i = 0; i < adjacencies; i++ ) {
        const point_abs_om p( generated_om_pos.x() + four_adjacent_offsets[i].x * c_seperation,
                              generated_om_pos.y() + four_adjacent_offsets[i].y * r_seperation );
        if( !highway_intersection_exists( p ) ) {
            generate_highway_intersection_point( p );
        }
        adjacent_intersections.emplace_back( overmap_buffer.get_overmap_highway_intersection_point( p ) );
    }

    //store adjacencies in calling intersection point
    overmap_highway_intersection_point this_intersection =
        overmap_buffer.get_overmap_highway_intersection_point( generated_om_pos );
    for( int i = 0; i < adjacencies; i++ ) {
        point_abs_om &new_intersection = this_intersection.adjacent_intersections[i];
        if( new_intersection.is_invalid() ) {
            new_intersection = adjacent_intersections[i].grid_pos;
        }
    }
    overmap_buffer.set_overmap_highway_intersection_point( generated_om_pos, this_intersection );

    return adjacent_intersections;
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
                forest_points.size() < static_cast<size_t>
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
            const overmap_connection_id &overmap_connection_forest_trail =
                settings->overmap_connection.trail_connection;
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

bool overmap::omt_is_lake( const point_abs_omt &origin, const point_om_omt &offset,
                           const double noise_threshold )
{
    const om_noise::om_noise_layer_lake noise_func( origin, g->get_seed() );
    // credit to ehughsbaird for thinking up this inbounds solution to infinite flood fill lag.
    bool inbounds = offset.x() > -5 && offset.y() > -5 && offset.x() < OMAPX + 5 &&
                    offset.y() < OMAPY + 5;
    if( !inbounds ) {
        return false;
    }
    return noise_func.noise_at( offset ) > noise_threshold;
}

bool overmap::has_lake( const point_abs_om &p, const double noise_threshold )
{
    const point_abs_omt origin = project_to<coords::omt>( p );
    const om_noise::om_noise_layer_lake noise_func( origin, g->get_seed() );
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            const point_om_omt seed_point( i, j );
            if( omt_is_lake( origin, seed_point, noise_threshold ) ) {
                return true;
            }
        }
    }
    return false;
}

void overmap::place_lakes( const std::vector<const overmap *> &neighbor_overmaps )
{
    const point_abs_omt origin = global_base_point();
    const om_noise::om_noise_layer_lake noise_func( origin, g->get_seed() );
    double noise_threshold = settings->overmap_lake.noise_threshold_lake;

    const auto is_lake = [&]( const point_om_omt & p ) {
        return omt_is_lake( origin, p, noise_threshold );
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
            if( !omt_is_lake( origin, seed_point, noise_threshold ) ) {
                continue;
            }

            // We're going to flood-fill our lake so that we can consider the entire lake when evaluating it
            // for placement, even when the lake runs off the edge of the current overmap.
            std::vector<point_om_omt> lake_points =
                ff::point_flood_fill_4_connected( seed_point, visited, is_lake );

            // If this lake doesn't exceed our minimum size threshold, then skip it. We can use this to
            // exclude the tiny lakes that don't provide interesting map features and exist mostly as a
            // noise artifact.
            if( lake_points.size() < static_cast<size_t>
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

            // Before we place the lake terrain and get river points, generate a river to somewhere
            // in the lake from an existing river.
            if( !rivers.empty() ) {
                const tripoint_om_omt random_lake_point( random_entry( lake_set ), 0 );
                point_om_omt river_connection = point_om_omt::invalid;
                for( const tripoint_om_omt &find_river : closest_points_first( random_lake_point, OMAPX / 2 ) ) {
                    if( inbounds( find_river ) && ter( find_river )->is_river() ) {
                        river_connection = find_river.xy();
                        break;
                    }
                }
                if( !river_connection.is_invalid() ) {
                    place_river( neighbor_overmaps, overmap_river_node{ river_connection, random_lake_point.xy() } );
                }
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

void overmap::place_oceans( const std::vector<const overmap *> &neighbor_overmaps )
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
            if( ocean_points.size() < static_cast<size_t>
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
                    place_river( neighbor_overmaps, overmap_river_node{ closest_point, lake_connection_point } );
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

void overmap::place_rivers( const std::vector<const overmap *> &neighbor_overmaps )
{
    const int OMAPX_edge = OMAPX - 1;
    const int OMAPY_edge = OMAPY - 1;
    const int directions = neighbor_overmaps.size();
    const int max_rivers = 2;
    int river_scale = settings->overmap_river.river_scale;
    if( river_scale == 0 ) {
        return;
    }
    river_scale = 1 + std::max( 1, river_scale );
    // North/West endpoints of rivers (2 major rivers max per overmap)
    std::array<point_om_omt, max_rivers> river_start = { point_om_omt::invalid, point_om_omt::invalid };
    // East/South endpoints of rivers
    std::array<point_om_omt, max_rivers> river_end = { point_om_omt::invalid, point_om_omt::invalid };
    // forced river bezier curve control points (closer to start point)
    std::array<point_om_omt, max_rivers> control_start = { point_om_omt::invalid, point_om_omt::invalid };
    // ..closer to end point
    std::array<point_om_omt, max_rivers> control_end = { point_om_omt::invalid, point_om_omt::invalid };

    auto bound_control_point = []( const point_om_omt & p, const point_rel_omt & init_diff ) {
        point_rel_omt diff = init_diff;
        point_om_omt continue_line = p + diff;
        while( !inbounds( continue_line ) ) {
            diff = point_rel_omt( diff.x() * 0.5, diff.y() * 0.5 );
            continue_line = p + diff;
        }
        return continue_line;
    };

    std::array<bool, 4> is_start = { true, false, false, true };
    std::array<bool, 4> node_present;
    for( int i = 0; i < directions; i++ ) {
        node_present[i] = neighbor_overmaps[i] == nullptr;
    }

    // set river starts and ends from neighboring overmaps' existing rivers
    // force control points from adjacent overmap to this overmap to smooth river curve
    int preset_start_nodes = 0;
    int preset_end_nodes = 0;
    std::array<overmap_river_border, 4> river_borders;
    for( int i = 0; i < directions; i++ ) {
        river_borders[i] = setup_adjacent_river( point_rel_om( four_adjacent_offsets[i] ), RIVER_BORDER );
        const overmap_river_border &river_border = river_borders[i];
        if( !river_border.border_river_nodes.empty() ) {
            //take the first node (major river) only
            point_om_omt p = river_border.border_river_nodes_omt.front();
            const overmap_river_node *river_node = river_border.border_river_nodes.front();
            if( is_start[i] ) {
                river_start[preset_start_nodes] = p;
                point_rel_omt diff = river_node->river_end - ( river_node->control_p2.is_invalid() ?
                                     river_node->river_end : river_node->control_p2 );
                control_start[preset_start_nodes] = bound_control_point( p, diff );
            } else {
                river_end[preset_end_nodes] = p;
                point_rel_omt diff = river_node->river_start - ( river_node->control_p1.is_invalid() ?
                                     river_node->river_start : river_node->control_p1 );
                control_end[preset_end_nodes] = bound_control_point( p, diff );
            }

            node_present[i] = true;
            is_start[i] ? preset_start_nodes++ : preset_end_nodes++;
        }
    }

    //if there wasn't an neighboring river, 1 / (river frequency ^ rivers generated) chance to continue
    bool no_neighboring_rivers = preset_start_nodes == 0 && preset_end_nodes == 0;
    if( no_neighboring_rivers ) {
        if( !x_in_y( 1.0, std::pow( settings->overmap_river.river_frequency,
                                    overmap_buffer.get_major_river_count() ) ) ) {
            return;
        }
    }

    // if there are two river start/ends from neighbor overmaps,
    // they must flow N->E, W->S to avoid intersecting
    bool lock_rivers = preset_start_nodes == max_rivers || preset_end_nodes == max_rivers;

    auto generate_new_node_point = [&]( int dir ) {
        switch( dir ) {
            case 0:
                return point_om_omt( rng( RIVER_BORDER, OMAPX_edge - RIVER_BORDER ), 0 );
            case 1:
                return point_om_omt( OMAPX_edge, rng( RIVER_BORDER, OMAPY_edge - RIVER_BORDER ) );
            case 2:
                return point_om_omt( rng( RIVER_BORDER, OMAPX_edge - RIVER_BORDER ), OMAPY_edge );
            case 3:
                return point_om_omt( 0, rng( RIVER_BORDER, OMAPY_edge - RIVER_BORDER ) );
            default:
                return point_om_omt( rng( RIVER_BORDER, OMAPX_edge - RIVER_BORDER ), rng( RIVER_BORDER,
                                     OMAPY_edge - RIVER_BORDER ) );
        }
    };

    // generate brand-new river nodes
    // generate two rivers if locking
    if( lock_rivers ) {
        if( river_start[0].is_invalid() ) {
            river_start[0] = generate_new_node_point( 0 );
        }
        if( river_start[1].is_invalid() ) {
            river_start[1] = generate_new_node_point( 3 );
        }
        if( river_end[0].is_invalid() ) {
            river_end[0] = generate_new_node_point( 1 );
        }
        if( river_end[1].is_invalid() ) {
            river_end[1] = generate_new_node_point( 2 );
        }
    }
    // if no river nodes were present, generate one river based on existing adjacent overmaps
    else {
        if( river_start[0].is_invalid() ) {
            if( node_present[0] && ( !node_present[3] || one_in( 2 ) ) ) {
                river_start[0] = generate_new_node_point( 0 );
            } else if( node_present[3] ) {
                river_start[0] = generate_new_node_point( 3 );
            } else {
                river_start[0] = generate_new_node_point( -1 );
            }
        }
        if( river_end[0].is_invalid() ) {
            if( node_present[2] && ( !node_present[1] || one_in( 2 ) ) ) {
                river_end[0] = generate_new_node_point( 2 );
            } else if( node_present[1] ) {
                river_end[0] = generate_new_node_point( 1 );
            } else {
                river_end[0] = generate_new_node_point( -1 );
            }
        }
    }

    //Finally, place rivers from start point to end point
    for( size_t i = 0; i < max_rivers; i++ ) {
        point_om_omt pa = river_start.at( i );
        point_om_omt pb = river_end.at( i );
        if( !pa.is_invalid() && !pb.is_invalid() ) {
            overmap_river_node temp_node{ pa, pb, control_start.at( i ), control_end.at( i ) };
            place_river( neighbor_overmaps, temp_node, river_scale, true );
            if( no_neighboring_rivers ) {
                overmap_buffer.inc_major_river_count();
            }
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
            // TODO: Use is_river or similar not a ot match
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

void overmap::place_roads( const std::vector<const overmap *> &neighbor_overmaps )
{
    int op_city_size = get_option<int>( "CITY_SIZE" );
    if( op_city_size <= 0 ) {
        return;
    }
    const overmap_connection_id &overmap_connection_inter_city_road =
        settings->overmap_connection.inter_city_road_connection;
    std::vector<tripoint_om_omt> &roads_out = connections_out[overmap_connection_inter_city_road];

    // At least 3 exit points, to guarantee road continuity across overmaps
    if( roads_out.size() < 3 ) {

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
            if( neighbor_overmaps[dir] == nullptr ) {
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
                           is_river( ter( tmp + point_rel_omt( four_adjacent_offsets[( dir + 1 ) % 4] ) ) ) ||
                           is_river( ter( tmp + point_rel_omt( four_adjacent_offsets[( dir + 3 ) % 4] ) ) ) ) ) {
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
    road_points.reserve( roads_out.size() + std::max( 1, static_cast<int>( cities.size() ) ) );
    for( const auto &elem : roads_out ) {
        road_points.emplace_back( elem.xy() );
    }
    if( cities.empty() ) {
        // If there's no cities in the overmap chose a random central point that special's road connections should path to
        fallback_road_connection_point = point_om_omt( rng( OMAPX / 4, ( 3 * OMAPX ) / 4 ),
                                         rng( OMAPY / 4, ( 3 * OMAPY ) / 4 ) );
        road_points.emplace_back( *fallback_road_connection_point );
    } else {
        for( const city &elem : cities ) {
            road_points.emplace_back( elem.pos );
        }
    }

    // And finally connect them via roads.
    connect_closest_points( road_points, 0, *overmap_connection_inter_city_road );
}

void overmap::place_railroads( const std::vector<const overmap *> &neighbor_overmaps )
{
    // no railroads if there are no cities
    int op_city_size = get_option<int>( "CITY_SIZE" );
    if( op_city_size <= 0 ) {
        return;
    }
    const overmap_connection_id &overmap_connection_local_railroad =
        settings->overmap_connection.rail_connection;
    std::vector<tripoint_om_omt> &railroads_out = connections_out[overmap_connection_local_railroad];

    // At least 3 exit points, to guarantee railroad continuity across overmaps
    if( railroads_out.size() < 3 ) {

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
            if( neighbor_overmaps[dir] == nullptr ) {
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
                           is_river( ter( tmp + point_rel_omt( four_adjacent_offsets[( dir + 1 ) % 4] ) ) ) ||
                           is_river( ter( tmp + point_rel_omt( four_adjacent_offsets[( dir + 3 ) % 4] ) ) ) ) ) {
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

void overmap::place_river( const std::vector<const overmap *> &neighbor_overmaps,
                           const overmap_river_node &initial_points, int river_scale, bool major_river )
{
    const int OMAPX_edge = OMAPX - 1;
    const int OMAPY_edge = OMAPY - 1;
    const int directions = 4;
    // TODO: Retain river_scale for the river node and allow branches to extend overmaps?
    if( river_scale <= 0 ) {
        return;
    }

    std::vector <std::vector<tripoint_om_omt>> border_points_by_dir;
    for( int i = 0; i < directions; i++ ) {
        const point_rel_om &p = point_rel_om( four_adjacent_offsets[i] );
        std::vector<tripoint_om_omt> border_points;
        if( neighbor_overmaps[i] == nullptr ) {
            border_points = get_border( p, RIVER_Z, RIVER_BORDER );
        }
        border_points_by_dir.emplace_back( border_points );
    }

    // Generate control points for Bezier curve
    // the start/end of the curve are the start/end of the river
    // the remaining control points are:
    // - one-third between river start and end
    // - two-thirds between river start and end

    const point_om_omt &river_start = initial_points.river_start;
    point_om_omt river_end = initial_points.river_end;
    const int distance = rl_dist( river_start, river_end );
    // variance of control points from the initial curve
    const int amplitude = distance / 2;
    point_rel_omt one_third(
        abs( river_start.x() - river_end.x() ) * ( 1.0 / 3.0 ),
        abs( river_start.y() - river_end.y() ) * ( 1.0 / 3.0 ) );
    const int river_z = 0;

    point_om_omt control_p1 = initial_points.control_p1;
    if( control_p1.is_invalid() ) {
        point_om_omt one_third_point = point_om_omt( river_start.x() + one_third.x(),
                                       river_start.y() + one_third.y() );
        control_p1 = point_om_omt( std::clamp( one_third_point.x() + rng( amplitude, 0 ), 0, OMAPX_edge ),
                                   std::clamp( one_third_point.y() + rng( amplitude, 0 ), 0, OMAPY_edge ) );
    }
    point_om_omt control_p2 = initial_points.control_p2;
    if( control_p2.is_invalid() ) {
        point_om_omt two_third_point = point_om_omt( river_start.x() + one_third.x() * 2,
                                       river_start.y() + one_third.y() * 2 );
        control_p2 = point_om_omt(
                         std::clamp( two_third_point.x() + rng( 0, -amplitude ), 0, OMAPX_edge ),
                         std::clamp( two_third_point.y() + rng( 0, -amplitude ), 0, OMAPY_edge ) );
    }
    const int n_segs = distance / 2; // Number of Bezier curve segments to calculate
    //there should be at least four points on the curve
    if( n_segs < 4 ) {
        return;
    }
    //Note: this is a list of *segments*; the points in this list are not always adjacent!
    std::vector<point_om_omt> segmented_curve = cubic_bezier( river_start, control_p1,
            control_p2, river_end, n_segs );
    //remove index-adjacent duplicate points
    auto iter = std::unique( segmented_curve.begin(), segmented_curve.end() );
    segmented_curve.erase( iter, segmented_curve.end() );

    //line_to only draws interval (p1, p2], so add the start again
    segmented_curve.emplace( segmented_curve.begin(), river_start );

    std::vector<point_om_omt> bezier_segment; // a complete Bezier segment (all points are adjacent)
    size_t size = 0;
    int curve_size = segmented_curve.size() - 1;
    int river_check_index = 0;

    for( river_check_index = 0; river_check_index < curve_size / 3; river_check_index++ ) {
        const point_om_omt &segment_start = segmented_curve.at( river_check_index );
        if( !is_water_body_not_shore( ter( tripoint_om_omt( segment_start.x(), segment_start.y(),
                                           river_z ) ) ) ) {
            break;
        }
    }

    //if the first third of the river is already water, abort
    if( river_check_index == curve_size / 3 ) {
        return;
    }

    //if any remaining points are water, stop the river at that point
    for( int i = river_check_index; i < curve_size; i++ ) {
        const point_om_omt segment_start = segmented_curve.at( i );
        tripoint_om_omt segment_start_tri( segment_start.x(), segment_start.y(), river_z );
        const oter_id &ter_here = ter( segment_start_tri );
        if( is_water_body_not_shore( ter_here ) ) {
            river_end = segment_start;
            curve_size = i;
            break;
        }
    }

    //draw the river by placing river OMTs between curve points
    for( int i = 0; i < curve_size; i++ ) {
        bezier_segment.clear();
        bezier_segment = line_to( segmented_curve.at( i ), segmented_curve.at( i + 1 ), 0 );
        // Now, draw the actual river tiles along the segment
        for( const point_om_omt &bezier_point : bezier_segment ) {

            tripoint_om_omt meandered_point( bezier_point, RIVER_Z );
            // no meander for start/end
            if( !( i == 0 || i == curve_size - 1 ) ) {
                river_meander( river_end, meandered_point, river_scale );
            }

            auto draw_river = [&size, this]( const tripoint_om_omt & pt ) {
                size++;
                ter_set( pt, oter_river_center );
            };
            // Draw river in radius [-river_scale, +river_scale]
            for( const tripoint_om_omt &pt : points_in_radius_circ( meandered_point, river_scale ) ) {
                // river points on the edge of the overmap are automatically drawn from neighbor,
                // but only if that neighbor doesn't exist
                if( !is_water_body_not_shore( ter( pt ) ) ) {
                    if( inbounds( pt, 1 ) ) {
                        draw_river( pt );
                    } else if( inbounds( pt ) ) {
                        for( int i = 0; i < directions; i++ ) {
                            if( !border_points_by_dir[i].empty() &&
                                std::find( border_points_by_dir[i].begin(), border_points_by_dir[i].end(),
                                           pt ) != border_points_by_dir[i].end() ) {
                                draw_river( pt );
                            }
                        }
                    }
                }
            }
        }
    }

    // create river branches
    const int branch_ahead_points = std::max( 2, curve_size / 5 );
    int branch_last_end = 0;
    for( int i = 0; i < curve_size; i++ ) {
        const point_om_omt &bezier_point = segmented_curve.at( i );
        if( inbounds( bezier_point, river_scale + 1 ) &&
            one_in( settings->overmap_river.river_branch_chance ) ) {
            point_om_omt branch_end_point = point_om_omt::invalid;

            //pick an end point from later along the curve
            //TODO: make remerge branches have control points that aren't straight;
            // remerges are less common because they get stopped by the already-generated river
            if( i > branch_last_end ) {
                if( one_in( settings->overmap_river.river_branch_remerge_chance ) ) {
                    int end_branch_node = rng( i + branch_ahead_points, i + branch_ahead_points * 2 );
                    if( end_branch_node < curve_size ) {
                        branch_end_point = segmented_curve.at( end_branch_node );
                        branch_last_end = end_branch_node;
                    }
                } else {
                    //or just randomly from the current segment
                    const int rad = 64;
                    branch_end_point = point_om_omt(
                                           rng( bezier_point.x() + rad / 2, bezier_point.x() + rad ),
                                           rng( bezier_point.y() + rad / 2, bezier_point.y() + rad ) );
                }
            }
            // if the end point is valid, place a new, smaller, overmap-local branch river
            if( !branch_end_point.is_invalid() && inbounds( branch_end_point ) ) {
                place_river( neighbor_overmaps, overmap_river_node{ bezier_point, branch_end_point },
                             river_scale - settings->overmap_river.river_branch_scale_decrease );
            }
        }
    }

    add_msg_debug( debugmode::DF_OVERMAP,
                   "for overmap %s, drew river at: start = %s, control points: %s, %s, end = %s",
                   loc.to_string_writable(), river_start.to_string_writable(),
                   control_p1.to_string_writable(), control_p2.to_string_writable(),
                   river_end.to_string_writable() );

    overmap_river_node new_node = { river_start, river_end, control_p1, control_p2, size, major_river };
    rivers.push_back( new_node );
}

void overmap::river_meander( const point_om_omt &river_end, tripoint_om_omt &current_point,
                             int river_scale )
{
    const auto random_uniform = []( int i ) -> bool {
        return rng( 0, static_cast<int>( OMAPX * 1.2 ) - 1 ) < i;
    };
    const auto random_close = []( int i ) -> bool {
        return rng( 0, static_cast<int>( OMAPX * 0.2 ) - 1 ) > i;
    };

    point_rel_omt abs_distances( abs( river_end.x() - current_point.x() ),
                                 abs( river_end.y() - current_point.y() ) );

    //as distance to river end decreases, meander closer to the river end
    if( current_point.x() != river_end.x() && ( random_uniform( abs_distances.x() ) ||
            ( random_close( abs_distances.x() ) &&
              random_close( abs_distances.y() ) ) ) ) {
        current_point += river_end.x() > current_point.x() ? tripoint::east : tripoint::west;
    }
    if( current_point.y() != river_end.y() && ( random_uniform( abs_distances.y() ) ||
            ( random_close( abs_distances.y() ) &&
              random_close( abs_distances.x() ) ) ) ) {
        current_point += river_end.y() > current_point.y() ? tripoint::south : tripoint::north;
    }
    //meander randomly, but not for rivers of size 1 (would exceed above meander)
    if( river_scale > 1 ) {
        current_point += tripoint_rel_omt( rng( -1, 1 ), rng( -1, 1 ), 0 );
    }
}

std::vector<tripoint_om_omt> overmap::get_neighbor_border( const point_rel_om &direction, int z,
        int distance_corner )
{
    const int OMAPX_edge = OMAPX - 1;
    const int OMAPY_edge = OMAPY - 1;

    tripoint_om_omt edges( direction.x() >= 0 ? 0 : OMAPX_edge, direction.y() >= 0 ? 0 : OMAPY_edge,
                           0 );
    bool iterx = direction.x() == 0;
    int corner_upper = iterx ? OMAPX_edge - distance_corner : OMAPY_edge - distance_corner;

    return iterx ?
           line_to( tripoint_om_omt( distance_corner, edges.y(), 0 ), tripoint_om_omt( corner_upper, edges.y(),
                    z ) ) :
           line_to( tripoint_om_omt( edges.x(), distance_corner, 0 ), tripoint_om_omt( edges.x(), corner_upper,
                    z ) );
}

std::vector<tripoint_om_omt> overmap::get_border( const point_rel_om &direction, int z,
        int distance_corner )
{
    point_rel_om flip_direction( -direction.x(), -direction.y() );
    return get_neighbor_border( flip_direction, z, distance_corner );
}

overmap_river_border overmap::setup_adjacent_river( const point_rel_om &adjacent_om, int border )
{
    overmap_river_border adjacent_border;
    std::vector<point_om_omt> &border_river_omt = adjacent_border.border_river_omt;
    std::vector<point_om_omt> &border_river_nodes_omt = adjacent_border.border_river_nodes_omt;
    std::vector<const overmap_river_node *> &border_river_nodes = adjacent_border.border_river_nodes;
    const overmap *adjacent_overmap = overmap_buffer.get_existing( pos() + adjacent_om );

    //if overmap doesn't exist yet, no need to check it
    if( adjacent_overmap == nullptr ) {
        return adjacent_border;
    }

    const int river_z = 0;
    std::vector<tripoint_om_omt> neighbor_border = get_neighbor_border( adjacent_om, river_z, border );
    std::vector<tripoint_om_omt> overmap_border = get_border( adjacent_om, river_z, border );
    const int neighbor_border_size = neighbor_border.size();

    for( int i = 0; i < neighbor_border_size; i++ ) {
        const tripoint_om_omt &neighbor_pt = neighbor_border[i];
        const tripoint_om_omt &current_pt = overmap_border[i];
        const point_om_omt p_neighbor_point = neighbor_pt.xy();
        const point_om_omt p_current_point = current_pt.xy();

        if( is_river( adjacent_overmap->ter( neighbor_pt ) ) ) {
            //if the neighbor overmap has an adjacent river tile, set to river
            //this guarantees that an out-of-bounds point adjacent to a river is always a river
            ter_set( current_pt, oter_river_center );
            border_river_omt.emplace_back( p_current_point );
            if( adjacent_overmap->is_river_node( p_neighbor_point ) ) {
                border_river_nodes_omt.emplace_back( p_current_point );
                border_river_nodes.emplace_back( adjacent_overmap->get_river_node_at( p_neighbor_point ) );
            }
        }
    }
    return adjacent_border;
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

    const overmap_connection_id &overmap_connection_intra_city_road =
        settings->overmap_connection.intra_city_road_connection;
    const overmap_connection &local_road( *overmap_connection_intra_city_road );

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
                city_tiles.insert( c );
                tmp.pos = p.xy();
                tmp.size = size;
            }
        } else {
            placement_attempts = 0;
            tmp = random_entry( cities_to_place );
            p = tripoint_om_omt( tmp.pos, 0 );
            ter_set( tripoint_om_omt( tmp.pos, 0 ), oter_road_nesw );
            city_tiles.insert( tmp.pos );
        }
        if( placement_attempts == 0 ) {
            cities.push_back( tmp );
            const om_direction::type start_dir = om_direction::random();
            om_direction::type cur_dir = start_dir;

            // Track placed CITY_UNIQUE buildings
            std::unordered_set<overmap_special_id> placed_unique_buildings;
            do {
                build_city_street( local_road, tmp.pos, tmp.size, cur_dir, tmp, placed_unique_buildings );
            } while( ( cur_dir = om_direction::turn_right( cur_dir ) ) != start_dir );
        }
    }
    flood_fill_city_tiles();
}

overmap_special_id overmap::pick_random_building_to_place( int town_dist, int town_size,
        const std::unordered_set<overmap_special_id> &placed_unique_buildings ) const
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
    auto building_type_to_pick = [&]() {
        if( shop_normal > town_dist ) {
            return std::mem_fn( &city_settings::pick_shop );
        } else if( park_normal > town_dist ) {
            return std::mem_fn( &city_settings::pick_park );
        } else {
            return std::mem_fn( &city_settings::pick_house );
        }
    };
    auto pick_building = building_type_to_pick();
    overmap_special_id ret;
    bool existing_unique;
    do {
        ret = pick_building( city_spec );
        if( ret->has_flag( "CITY_UNIQUE" ) ) {
            existing_unique = placed_unique_buildings.find( ret ) != placed_unique_buildings.end();
        } else if( ret->has_flag( "GLOBALLY_UNIQUE" ) || ret->has_flag( "OVERMAP_UNIQUE" ) ) {
            existing_unique = overmap_buffer.contains_unique_special( ret );
        } else {
            existing_unique = false;
        }
    } while( existing_unique || !ret->get_constraints().city_size.contains( town_size ) );
    return ret;
}

void overmap::place_building( const tripoint_om_omt &p, om_direction::type dir, const city &town,
                              std::unordered_set<overmap_special_id> &placed_unique_buildings )
{
    const tripoint_om_omt building_pos = p + om_direction::displace( dir );
    const om_direction::type building_dir = om_direction::opposite( dir );

    const int town_dist = ( trig_dist( building_pos.xy(), town.pos ) * 100 ) / std::max( town.size, 1 );

    for( size_t retries = 10; retries > 0; --retries ) {
        const overmap_special_id building_tid = pick_random_building_to_place( town_dist, town.size,
                                                placed_unique_buildings );
        if( can_place_special( *building_tid, building_pos, building_dir, false ) ) {
            std::vector<tripoint_om_omt> used_tripoints = place_special( *building_tid, building_pos,
                    building_dir, town, false, false );
            for( const tripoint_om_omt &p : used_tripoints ) {
                city_tiles.insert( p.xy() );
            }
            if( building_tid->has_flag( "CITY_UNIQUE" ) ) {
                placed_unique_buildings.emplace( building_tid );
            }
            break;
        }
    }
}

void overmap::build_city_street(
    const overmap_connection &connection, const point_om_omt &p, int cs, om_direction::type dir,
    const city &town, std::unordered_set<overmap_special_id> &placed_unique_buildings, int block_width )
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
                               town, placed_unique_buildings, new_width );

            build_city_street( connection, iter->pos, right, om_direction::turn_right( dir ),
                               town, placed_unique_buildings, new_width );

            const oter_id &oter = ter( rp );
            // TODO: Get rid of the hardcoded terrain ids.
            if( one_in( 2 ) && oter->get_line() == 15 && oter->type_is( oter_type_id( "road" ) ) ) {
                ter_set( rp, oter_road_nesw_manhole.id() );
            }
        }

        if( !one_in( BUILDINGCHANCE ) ) {
            place_building( rp, om_direction::turn_left( dir ), town, placed_unique_buildings );
        }
        if( !one_in( BUILDINGCHANCE ) ) {
            place_building( rp, om_direction::turn_right( dir ), town, placed_unique_buildings );
        }
    }

    // If we're big, make a right turn at the edge of town.
    // Seems to make little neighborhoods.
    cs -= rng( 1, 3 );

    if( cs >= 2 && c == 0 ) {
        const auto &last_node = street_path.nodes.back();
        const om_direction::type rnd_dir = om_direction::turn_random( dir );
        build_city_street( connection, last_node.pos, cs, rnd_dir, town, placed_unique_buildings );
        if( one_in( 5 ) ) {
            build_city_street( connection, last_node.pos, cs, om_direction::opposite( rnd_dir ),
                               town, placed_unique_buildings, new_width );
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
    candidates.insert( { p + point::north, p + point::east, p + point::south, p + point::west } );
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
        if( is_ot_match( "_stairs", ter( elem + tripoint::above ), ot_match_type::contains ) ) {
            generate_stairs = false;
        }
    }
    if( generate_stairs && !generated_lab.empty() ) {
        std::shuffle( generated_lab.begin(), generated_lab.end(), rng_get_engine() );

        // we want a spot where labs are above, but we'll settle for the last element if necessary.
        tripoint_om_omt lab_pos;
        for( tripoint_om_omt elem : generated_lab ) {
            lab_pos = elem;
            if( ter( lab_pos + tripoint::above ) == labt ) {
                break;
            }
        }
        ter_set( lab_pos + tripoint::above, labt_stairs );
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
                     ter( cell + point::south ) != labt ||
                     adjacent_labs != 1 ) );
        if( tries < 50 ) {
            ter_set( cell, oter_lab_escape_cells.id() );
            ter_set( cell + point::south, oter_lab_escape_entrance.id() );
        }
    }

    return numstairs > 0;
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

        //forces perpendicular crossing
        if( subtype->is_perpendicular_crossing() && id->is_rotatable() &&
            om_direction::are_parallel( cur.dir, id->get_dir() ) ) {
            return pf::node_score::rejected;
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
        const point_om_omt &source, om_direction::type dir, size_t len )
{
    auto valid_placement = [&]( const tripoint_om_omt pos ) {
        if( !inbounds( pos, 1 ) ) {
            return false;  // Don't approach overmap bounds.
        }
        const oter_id &ter_id = ter( pos );
        // TODO: Make it so the city picks a bridge direction ( ns or ew ) and allows bridging over rivers in that direction with the same logic as highways
        if( ter_id->is_river() || ter_id->is_ravine() || ter_id->is_ravine_edge() ||
            ter_id->is_highway() || ter_id->is_highway_reserved() || !connection.pick_subtype_for( ter_id ) ) {
            return false;
        }
        int collisions = 0;
        for( int i = -1; i <= 1; i++ ) {
            for( int j = -1; j <= 1; j++ ) {
                const tripoint_om_omt checkp = pos + tripoint( i, j, 0 );

                if( checkp != pos + om_direction::displace( dir, 1 ) &&
                    checkp != pos + om_direction::displace( om_direction::opposite( dir ), 1 ) &&
                    checkp != pos ) {
                    if( ter( checkp )->get_type_id() == oter_type_road ) {
                        //Stop roads from running right next to each other
                        if( collisions >= 2 ) {
                            return false;
                        }
                        collisions++;
                    }
                }
            }
        }
        return true;
    };

    const tripoint_om_omt from( source, 0 );
    // See if we need to make another one "step" further.
    const tripoint_om_omt en_pos = from + om_direction::displace( dir, len + 1 );
    if( inbounds( en_pos, 1 ) && connection.has( ter( en_pos ) ) ) {
        ++len;
    }
    size_t actual_len = 0;
    bool checked_highway = false;

    while( actual_len < len ) {
        const tripoint_om_omt pos = from + om_direction::displace( dir, actual_len );
        if( !valid_placement( pos ) ) {
            break;
        }
        const oter_id &ter_id = ter( pos );
        if( ter_id->is_highway_reserved() ) {
            if( !checked_highway ) {
                // Break if parallel to the highway direction
                if( are_parallel( dir, ter_id.obj().get_dir() ) ) {
                    break;
                }
                const int &highway_width = settings->overmap_highway.width_of_segments;
                const tripoint_om_omt pos_after_highway = pos + om_direction::displace( dir, highway_width );
                // Ensure we can pass fully through
                if( !valid_placement( pos_after_highway ) ) {
                    break;
                }
                checked_highway = true;
            }
            // Prevent stopping under highway
            if( actual_len == len - 1 ) {
                ++len;
            }
        }

        city_tiles.insert( pos.xy() );
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

    for( const edge &candidate : edges ) {
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

void overmap::polish_river( const std::vector<const overmap *> &neighbor_overmaps )
{
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            build_river_shores( neighbor_overmaps, { x, y, 0 } );
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

bool overmap::is_river_node( const point_om_omt &p ) const
{
    return !!get_river_node_at( p );
}

const overmap_river_node *overmap::get_river_node_at( const point_om_omt &p ) const
{
    for( const overmap_river_node &n : rivers ) {
        if( n.river_start == p || n.river_end == p ) {
            return &n;
        }
    }
    return nullptr;
}

void overmap::build_river_shores( const std::vector<const overmap *> &neighbor_overmaps,
                                  const tripoint_om_omt &p )
{
    const int neighbor_overmap_size = neighbor_overmaps.size();
    // TODO: Change this to the flag (or an else that sets a different bool?) and add handling for bridge maps to get overwritten by the correct tile then replaced
    if( !is_ot_match( "river", ter( p ), ot_match_type::prefix ) ) {
        return;
    }
    // Find and assign shores where they need to be.
    int mask = 0;
    int multiplier = 1;
    auto mask_add = [&multiplier, &mask]() {
        mask += ( 1 * multiplier );
    };
    const point_om_omt as_point( p.x(), p.y() );
    // TODO: Expand to eight_horizontal_neighbors dealing with corners here instead of afterwards?
    for( int i = 0; i < neighbor_overmap_size; i++ ) {
        const tripoint_om_omt p_offset = p + point_rel_omt( four_adjacent_offsets[i] );

        if( !inbounds( p_offset ) || is_water_body( ter( p_offset ) ) ) {
            //out of bounds OMTs adjacent to border river OMTs are always river OMTs
            mask_add();
        }
        multiplier *= 2;
    }

    auto river_ter = [&]() {
        const std::array<oter_str_id, 16> river_ters = {
            oter_forest_water, // 0 = no adjacent rivers.
            oter_river_south, // 1 = N adjacent river
            oter_river_west, // 2 = E adjacent river
            oter_river_sw, // 3 = 1+2 = N+E adjacent rivers
            oter_river_north, // 4 = S adjacent river
            oter_forest_water, // 5 = 1+4 = N+S adjacent rivers, no map though
            oter_river_nw, // 6 = 2+4 = E+S adjacent rivers
            oter_river_west, // 7 = 1+2+4 = N+E+S adjacent rivers
            oter_river_east, // 8 = W adjacent river
            oter_river_se, // 9 = 1+8 = N+W adjacent rivers
            oter_forest_water, // 10 = 2+8 = E+W adjacent rivers, no map though
            oter_river_south, // 11 = 1+2+8 = N+E+W adjacent rivers
            oter_river_ne, // 12 = 4+8 = S+W adjacent rivers
            oter_river_east, // 13 = 1+4+8 = N+S+W adjacent rivers
            oter_river_north, // 14 = 2+4+8 = E+S+W adjacent rivers
            oter_river_center // 15 = 1+2+4+8 = N+E+S+W adjacent rivers.
        };

        if( mask == 15 ) {
            // Trim corners if neccessary
            // We assume if corner are not inbounds then we are placed because of neighbouring overmap river
            const std::array<oter_str_id, 4> trimmed_corner_ters = { oter_river_c_not_ne, oter_river_c_not_se, oter_river_c_not_sw, oter_river_c_not_nw };
            for( int i = 0; i < 4; i++ ) {
                const tripoint_om_omt &corner = p + point_rel_omt( four_ordinal_directions[i] );
                if( inbounds( corner ) && !is_water_body( ter( corner ) ) ) {
                    return trimmed_corner_ters[i];
                }
            }
        }
        return river_ters[mask];
    };
    ter_set( p, river_ter() );
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
            [[fallthrough]];
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

point_rel_omt om_direction::displace( type dir, int dist )
{
    return rotate( point_rel_omt{ 0, -dist }, dir );
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
            [[fallthrough]];
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

void overmap::log_unique_special( const overmap_special_id &id )
{
    if( contains_unique_special( id ) ) {
        debugmsg( "Overmap unique overmap special placed more than once: %s", id.str() );
    }
    overmap_buffer.log_unique_special( id );
}

bool overmap::contains_unique_special( const overmap_special_id &id ) const
{
    return std::find_if( overmap_special_placements.begin(),
    overmap_special_placements.end(), [&id]( const auto & it ) {
        return it.second == id;
    } ) != overmap_special_placements.end();
}

bool overmap::can_place_special( const overmap_special &special, const tripoint_om_omt &p,
                                 om_direction::type dir, const bool must_be_unexplored ) const
{
    cata_assert( dir != om_direction::type::invalid );

    if( !special.id ) {
        return false;
    }
    if( ( special.has_flag( "GLOBALLY_UNIQUE" ) &&
          overmap_buffer.contains_unique_special( special.id ) ) ||
        ( special.has_flag( "OVERMAP_UNIQUE" ) && contains_unique_special( special.id ) ) ) {
        return false;
    }

    if( special.has_eoc() ) {
        dialogue d( get_talker_for( get_avatar() ), nullptr );
        if( !special.get_eoc()->test_condition( d ) ) {
            return false;
        }
    }

    // Don't spawn monster areas over locations designated as safe.
    // We're using the maximum radius rather than the generated one, as the latter hasn't been
    // produced yet, and it also provides some extra breathing room margin in most cases.
    const overmap_special_spawns &spawns = special.get_monster_spawns();
    if( spawns.group ) {
        const point_abs_omt base = coords::project_to<coords::omt>( this->pos() );
        for( int x = p.x() - spawns.radius.max; x <= p.x() + spawns.radius.max; x++ ) {
            for( int y = p.y() - spawns.radius.max; y <= p.y() + spawns.radius.max; y++ ) {
                const tripoint_abs_omt target = tripoint_abs_omt{ base, p.z() } + point_rel_omt{ x, y };
                if( overmap_buffer.get_existing( coords::project_to<coords::om>( target.xy() ) ) ) {
                    const std::optional<overmap_special_id> spec = overmap_buffer.overmap_special_at( target );
                    if( spec.has_value() &&
                        spec.value().obj().has_flag( "SAFE_AT_WORLDGEN" ) ) {
                        return false;
                    }
                }
            }
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
        // Debug output if you want to know where all globally unique locations are
        //       point_abs_omt location = coords::project_to<coords::omt>(this->pos()) + p.xy().raw();
        //        DebugLog(DL_ALL, DC_ALL) << "Globally Unique " << special.id.c_str() << " added at " << location.to_string_writable();
    } else if( special.has_flag( "OVERMAP_UNIQUE" ) ) {
        log_unique_special( special.id );
    }
    // CITY_UNIQUE is handled in place_building()

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
            if( !special.can_belong_to_city( p, nearest_city, *this ) ) {
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
            ++it;
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

        const bool unique = iter->special_details->has_flag( "OVERMAP_UNIQUE" );
        const bool globally_unique = iter->special_details->has_flag( "GLOBALLY_UNIQUE" );
        if( unique || globally_unique ) {
            const overmap_special_id &id = iter->special_details->id;
            const overmap_special_placement_constraints &constraints = iter->special_details->get_constraints();
            const float special_count = overmap_buffer.get_unique_special_count( id );
            const float overmap_count = overmap_buffer.get_overmap_count();
            const float min = special_count > 0 ? constraints.occurrences.min / special_count :
                              constraints.occurrences.min;
            const float max = std::max( overmap_count > 0 ? constraints.occurrences.max / overmap_count :
                                        constraints.occurrences.max, min );
            if( x_in_y( min, max ) && ( !overmap_buffer.contains_unique_special( id ) ) ) {
                // Min and max are overloaded to be the chance of occurrence,
                // so reset instances placed to one short of max so we don't place several.
                iter->instances_placed = constraints.occurrences.max - 1;
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
            std::string msg =
                "The following specials could not be placed, some missions may fail to initialize: ";
            int n = 0;
            for( auto iter = custom_overmap_specials.begin(); iter != custom_overmap_specials.end(); ) {
                if( iter->instances_placed < iter->special_details->get_constraints().occurrences.min ) {
                    msg.append( iter->special_details->id.c_str() ).append( ", " );
                    n++;
                }
                ++iter;
            }
            if( n > 0 ) {
                msg = msg.substr( 0, msg.length() - 2 );
            } else {
                msg = msg.append( "<unknown>" );
            }
            add_msg( _( msg ) );
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
                ++it;
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
            if( elem.size > city_spawn_threshold || one_in( city_spawn_chance ) ) {

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
                            local_sm_list.push_back( this_sm + point::east );
                            local_sm_list.push_back( this_sm + point::south );
                            local_sm_list.push_back( this_sm + point::south_east );

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

    if( get_option<bool>( "OVERMAP_PLACE_RIVERS" ) || get_option<bool>( "OVERMAP_PLACE_LAKES" ) ) {
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
    }
    if( get_option<bool>( "OVERMAP_PLACE_OCEANS" ) ) {
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
    if( world_generator->active_world->has_compression_enabled() ) {
        assure_dir_exist( PATH_INFO::world_base_save_path() / "overmaps" );
        const std::string terfilename = overmapbuffer::terrain_filename( loc );
        const std::filesystem::path terfilename_path = std::filesystem::u8path( terfilename );
        const cata_path zzip_path = PATH_INFO::world_base_save_path() / "overmaps" / terfilename_path +
                                    ".zzip";
        if( file_exist( zzip_path ) ) {
            std::shared_ptr<zzip> z = zzip::load( zzip_path.get_unrelative_path(),
                                                  ( PATH_INFO::world_base_save_path() / "overmaps.dict" ).get_unrelative_path()
                                                );

            if( read_from_zzip_optional( z, terfilename_path, [this]( std::string_view sv ) {
            std::istringstream is{ std::string( sv ) };
            unserialize( is );
            } ) ) {
                const cata_path plrfilename = overmapbuffer::player_filename( loc );
                read_from_file_optional( plrfilename, [this, &plrfilename]( std::istream & is ) {
                    unserialize_view( plrfilename, is );
                } );
                return;
            }
        }
    } else {
        const cata_path terfilename = PATH_INFO::world_base_save_path() / overmapbuffer::terrain_filename(
                                          loc );

        if( read_from_file_optional( terfilename, [this, &terfilename]( std::istream & is ) {
        unserialize( terfilename, is );
        } ) ) {
            const cata_path plrfilename = overmapbuffer::player_filename( loc );
            read_from_file_optional( plrfilename, [this, &plrfilename]( std::istream & is ) {
                unserialize_view( plrfilename, is );
            } );
            return;
        }
    }

    // pointers looks like (north, south, west, east)
    std::vector<const overmap *> neighbors;
    neighbors.reserve( four_adjacent_offsets.size() );
    for( const point &adjacent : four_adjacent_offsets ) {
        neighbors.emplace_back( overmap_buffer.get_existing( loc + adjacent ) );
    }
    generate( neighbors, enabled_specials );
}

// Note: this may throw io errors from std::ofstream
void overmap::save() const
{
    write_to_file( overmapbuffer::player_filename( loc ), [&]( std::ostream & stream ) {
        serialize_view( stream );
    } );

    if( world_generator->active_world->has_compression_enabled() ) {
        const std::string terfilename = overmapbuffer::terrain_filename( loc );
        const std::filesystem::path terfilename_path = std::filesystem::u8path( terfilename );
        const cata_path overmaps_folder = PATH_INFO::world_base_save_path() / "overmaps";
        assure_dir_exist( overmaps_folder );
        const cata_path zzip_path = overmaps_folder / terfilename_path + ".zzip";
        std::shared_ptr<zzip> z = zzip::load( zzip_path.get_unrelative_path(),
                                              ( PATH_INFO::world_base_save_path() / "overmaps.dict" ).get_unrelative_path()
                                            );
        if( !z ) {
            throw std::runtime_error(
                string_format(
                    "Failed to open %s",
                    zzip_path.get_unrelative_path().generic_u8string().c_str()
                )
            );
        }

        std::stringstream s;
        serialize( s );

        if( !z->add_file( terfilename_path, s.str() ) ) {
            throw std::runtime_error( string_format( "Failed to save omap %d.%d to %s", loc.x(),
                                      loc.y(), zzip_path.get_unrelative_path().generic_u8string().c_str() ) );
        }
        z->compact( 2.0 );
    } else {
        write_to_file( PATH_INFO::world_base_save_path() / overmapbuffer::terrain_filename( loc ), [&](
        std::ostream & stream ) {
            serialize( stream );
        } );
    }
}

void overmap::spawn_mon_group( const mongroup &group, int radius )
{
    tripoint_om_omt pos = project_to<coords::omt>( group.rel_pos() );
    if( safe_at_worldgen.find( pos ) != safe_at_worldgen.end() ) {
        return;
    }
    add_mon_group( group, radius );
}

void overmap::debug_force_add_group( const mongroup &group )
{
    add_mon_group( group, 1 );
}

std::vector<std::reference_wrapper<mongroup>> overmap::debug_unsafe_get_groups_at(
            tripoint_abs_omt &loc )
{
    point_abs_om overmap;
    tripoint_om_omt omt_within_overmap;
    std::tie( overmap, omt_within_overmap ) = project_remain<coords::om>( loc );
    tripoint_om_sm om_sm_pos = project_to<coords::sm>( omt_within_overmap );

    std::vector<std::reference_wrapper <mongroup>> groups_at;
    for( std::pair<const tripoint_om_sm, mongroup> &pair : zg ) {
        if( pair.first == om_sm_pos ) {
            groups_at.emplace_back( pair.second );
        }
    }
    return groups_at;
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
            const int dist = trig_dist( point( x, y ), point::zero );
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

    return MAPBUFFER.submap_exists( global_sm_loc );
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

template<>
std::string enum_to_string<om_vision_level>( om_vision_level data )
{
    switch( data ) {
        // *INDENT-OFF*
        case om_vision_level::unseen: return "unseen";
        case om_vision_level::vague: return "vague";
        case om_vision_level::outlines: return "outlines";
        case om_vision_level::details: return "details";
        case om_vision_level::full: return "full";
        // *INDENT-ON*
        default:
            break;
    }
    debugmsg( "Unknown om_vision_level %d", static_cast<int>( data ) );
    return "unseen";
}

template<>
std::string enum_to_string<oter_type_t::see_costs>( oter_type_t::see_costs data )
{
    switch( data ) {
        // *INDENT-OFF*
        case oter_type_t::see_costs::all_clear: return "all_clear";
        case oter_type_t::see_costs::none: return "none";
        case oter_type_t::see_costs::low: return "low";
        case oter_type_t::see_costs::medium: return "medium";
        case oter_type_t::see_costs::spaced_high: return "spaced_high";
        case oter_type_t::see_costs::high: return "high";
        case oter_type_t::see_costs::full_high: return "full_high";
        case oter_type_t::see_costs::opaque: return "opaque";
        // *INDENT-ON*
        default:
            break;
    }
    debugmsg( "Unknown see_cost %d", static_cast<int>( data ) );
    return "none";
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

std::string_view oter_no_dir_or_connections( const oter_id &oter )
{
    std::string_view base_oter_id = oter_no_dir( oter );
    for( const std::string &suffix : om_lines::mapgen_suffixes ) {
        if( string_ends_with( base_oter_id, suffix ) ) {
            base_oter_id = base_oter_id.substr( 0, base_oter_id.size() - suffix.size() );
        }
    }
    for( const auto &connection_type : om_lines::all ) {
        if( string_ends_with( base_oter_id, connection_type.suffix ) ) {
            base_oter_id = base_oter_id.substr( 0, base_oter_id.size() - connection_type.suffix.size() );
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

void overmap_special_migration::load( const JsonObject &jo, std::string_view )
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
