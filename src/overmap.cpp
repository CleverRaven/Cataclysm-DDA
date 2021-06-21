#include "omdata.h" // IWYU pragma: associated
#include "overmap.h" // IWYU pragma: associated

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <istream>
#include <memory>
#include <numeric>
#include <ostream>
#include <set>
#include <type_traits>
#include <unordered_set>
#include <vector>

#include "assign.h"
#include "cached_options.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "character_id.h"
#include "coordinates.h"
#include "cuboid_rectangle.h"
#include "debug.h"
#include "flood_fill.h"
#include "game.h"
#include "generic_factory.h"
#include "json.h"
#include "line.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapgen.h"
#include "math_defines.h"
#include "messages.h"
#include "mongroup.h"
#include "monster.h"
#include "mtype.h"
#include "name.h"
#include "npc.h"
#include "optional.h"
#include "options.h"
#include "output.h"
#include "overmap_connection.h"
#include "overmap_location.h"
#include "overmap_noise.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "regional_settings.h"
#include "rng.h"
#include "rotatable_symbols.h"
#include "simple_pathfinding.h"
#include "string_formatter.h"
#include "text_snippets.h"
#include "translations.h"

static const species_id species_ZOMBIE( "ZOMBIE" );

static const mongroup_id GROUP_CHUD( "GROUP_CHUD" );
static const mongroup_id GROUP_RIVER( "GROUP_RIVER" );
static const mongroup_id GROUP_SEWER( "GROUP_SEWER" );
static const mongroup_id GROUP_SPIRAL( "GROUP_SPIRAL" );
static const mongroup_id GROUP_SWAMP( "GROUP_SWAMP" );
static const mongroup_id GROUP_WORM( "GROUP_WORM" );
static const mongroup_id GROUP_ZOMBIE( "GROUP_ZOMBIE" );

class map_extra;

#define dbg(x) DebugLog((x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

static constexpr int BUILDINGCHANCE = 4;
static constexpr int MIN_ANT_SIZE = 8;
static constexpr int MAX_ANT_SIZE = 20;
static constexpr int MIN_GOO_SIZE = 1;
static constexpr int MAX_GOO_SIZE = 2;

using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

////////////////
static oter_id  ot_null,
       ot_crater,
       ot_field,
       ot_forest,
       ot_forest_thick,
       ot_forest_water,
       ot_river_center;

const oter_type_t oter_type_t::null_type{};

namespace om_lines
{

struct type {
    uint32_t symbol;
    size_t mapgen;
    std::string suffix;
};

const std::array<std::string, 5> mapgen_suffixes = {{
        "_straight", "_curved", "_end", "_tee", "_four_way"
    }
};

const std::array < type, 1 + om_direction::bits > all = {{
        { UTF8_getch( LINE_XXXX_S ), 4, "_isolated"  }, // 0  ----
        { UTF8_getch( LINE_XOXO_S ), 2, "_end_south" }, // 1  ---n
        { UTF8_getch( LINE_OXOX_S ), 2, "_end_west"  }, // 2  --e-
        { UTF8_getch( LINE_XXOO_S ), 1, "_ne"        }, // 3  --en
        { UTF8_getch( LINE_XOXO_S ), 2, "_end_north" }, // 4  -s--
        { UTF8_getch( LINE_XOXO_S ), 0, "_ns"        }, // 5  -s-n
        { UTF8_getch( LINE_OXXO_S ), 1, "_es"        }, // 6  -se-
        { UTF8_getch( LINE_XXXO_S ), 3, "_nes"       }, // 7  -sen
        { UTF8_getch( LINE_OXOX_S ), 2, "_end_east"  }, // 8  w---
        { UTF8_getch( LINE_XOOX_S ), 1, "_wn"        }, // 9  w--n
        { UTF8_getch( LINE_OXOX_S ), 0, "_ew"        }, // 10 w-e-
        { UTF8_getch( LINE_XXOX_S ), 3, "_new"       }, // 11 w-en
        { UTF8_getch( LINE_OOXX_S ), 1, "_sw"        }, // 12 ws--
        { UTF8_getch( LINE_XOXX_S ), 3, "_nsw"       }, // 13 ws-n
        { UTF8_getch( LINE_OXXX_S ), 3, "_esw"       }, // 14 wse-
        { UTF8_getch( LINE_XXXX_S ), 4, "_nesw"      }  // 15 wsen
    }
};

const size_t size = all.size();
const size_t invalid = 0;

constexpr size_t rotate( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return line;
    }
    // Bitwise rotation to the left.
    return ( ( ( line << static_cast<size_t>( dir ) ) |
               ( line >> ( om_direction::size - static_cast<size_t>( dir ) ) ) ) & om_direction::bits );
}

constexpr size_t set_segment( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return line;
    }
    return line | 1 << static_cast<int>( dir );
}

constexpr bool has_segment( size_t line, om_direction::type dir )
{
    if( dir == om_direction::type::invalid ) {
        return false;
    }
    return static_cast<bool>( line & 1 << static_cast<int>( dir ) );
}

constexpr bool is_straight( size_t line )
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

city::city( const point_om_omt &P, int const S )
    : pos( P )
    , size( S )
    , name( Name::get( nameFlags::IsTownName ) )
{
}

int city::get_distance_from( const tripoint_om_omt &p ) const
{
    return std::max( static_cast<int>( trig_dist( p, tripoint_om_omt{ pos, 0 } ) ) - size, 0 );
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

bool operator==( const int_id<oter_t> &lhs, const char *rhs )
{
    return lhs.id().str() == rhs;
}

bool operator!=( const int_id<oter_t> &lhs, const char *rhs )
{
    return !( lhs == rhs );
}

static void set_oter_ids()   // FIXME: constify
{
    ot_null         = oter_str_id::NULL_ID();
    // NOT required.
    ot_crater       = oter_id( "crater" );
    ot_field        = oter_id( "field" );
    ot_forest       = oter_id( "forest" );
    ot_forest_thick = oter_id( "forest_thick" );
    ot_forest_water = oter_id( "forest_water" );
    ot_river_center = oter_id( "river_center" );
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

    assign( jo, "color", color, strict );

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
    for( const auto &elem : land_use_codes.get_all() ) {
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
    for( const auto &elem : specials.get_all() ) {
        const_cast<overmap_special &>( elem ).finalize(); // This cast is ugly, but safe.
    }
}

void overmap_specials::check_consistency()
{
    const size_t max_count = ( OMAPX / OMSPEC_FREQ ) * ( OMAPY / OMSPEC_FREQ );
    const size_t actual_count = std::accumulate( specials.get_all().begin(), specials.get_all().end(),
                                static_cast< size_t >( 0 ),
    []( size_t sum, const overmap_special & elem ) {
        return sum + ( elem.flags.count( "UNIQUE" ) ? static_cast<size_t>( 0 )  : static_cast<size_t>( (
                           std::max( elem.occurrences.min, 0 ) ) ) ) ;
    } );

    if( actual_count > max_count ) {
        debugmsg( "There are too many mandatory overmap specials (%d > %d). Some of them may not be placed.",
                  actual_count, max_count );
    }

    specials.check();
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
    const int city_size = get_option<int>( "CITY_SIZE" );
    std::vector<const overmap_special *> res;

    res.reserve( specials.size() );
    for( const overmap_special &elem : specials.get_all() ) {
        if( elem.occurrences.empty() ||
        std::any_of( elem.terrains.begin(), elem.terrains.end(), []( const overmap_special_terrain & t ) {
        return t.locations.empty();
        } ) ) {
            continue;
        }

        if( city_size == 0 && elem.city_size.min > city_size ) {
            continue;
        }

        res.push_back( &elem );
    }

    return overmap_special_batch( origin, res );
}

bool is_river( const oter_id &ter )
{
    return ter->is_river();
}

bool is_river_or_lake( const oter_id &ter )
{
    return ter->is_river() || ter->is_lake() || ter->is_lake_shore();
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

    static const auto is_ot_subtype = []( const std::string & otype, const oter_id & oter ) {
        // Checks for any partial match.
        return strstr( oter.id().c_str(), otype.c_str() );
    };

    switch( match_type ) {
        case ot_match_type::exact:
            return is_ot( name, oter );
        case ot_match_type::type:
            return is_ot_type( name, oter );
        case ot_match_type::prefix:
            return is_ot_prefix( name, oter );
        case ot_match_type::contains:
            return is_ot_subtype( name, oter );
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
            load_mapgen_function( jio, fmapkey, point_zero );
        }
    }
}

std::string oter_type_t::get_symbol() const
{
    return utf32_to_utf8( symbol );
}

void oter_type_t::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    optional( jo, was_loaded, "sym", symbol, unicode_codepoint_from_symbol_reader, NULL_UNICODE );

    assign( jo, "name", name, strict );
    assign( jo, "see_cost", see_cost, strict );
    assign( jo, "travel_cost", travel_cost, strict );
    assign( jo, "extras", extras, strict );
    assign( jo, "mondensity", mondensity, strict );
    assign( jo, "spawns", static_spawns, strict );
    assign( jo, "color", color, strict );
    assign( jo, "land_use_code", land_use_code, strict );

    const auto flag_reader = make_flag_reader( oter_flags_map, "overmap terrain flag" );
    optional( jo, was_loaded, "flags", flags, flag_reader );

    if( has_flag( oter_flags::line_drawing ) ) {
        if( has_flag( oter_flags::no_rotate ) ) {
            jo.throw_error( R"(Mutually exclusive flags: "NO_ROTATE" and "LINEAR".)" );
        }

        for( const auto &elem : om_lines::mapgen_suffixes ) {
            load_overmap_terrain_mapgens( jo, id.str(), elem );
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
    id( type.id.str() + "_" + om_direction::id( dir ) ),
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
    static const oter_str_id road_manhole( "road_nesw_manhole" );
    if( id == road_manhole ) {
        return true;
    }
    return om_lines::has_segment( line, dir );
}

bool oter_t::is_hardcoded() const
{
    // TODO: This set only exists because so does the monstrous 'if-else' statement in @ref map::draw_map(). Get rid of both.
    static const std::set<std::string> hardcoded_mapgen = {
        "acid_anthill",
        "anthill",
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
        "looted_building",  // pseudo-terrain
        "mine",
        "mine_down",
        "mine_entrance",
        "mine_finale",
        "mine_shaft",
        "office_tower_1",
        "office_tower_1_entrance",
        "office_tower_b",
        "office_tower_b_entrance",
        "slimepit",
        "slimepit_down",
        "spider_pit_under",
        "spiral",
        "spiral_hub",
        "temple",
        "temple_finale",
        "temple_stairs",
        "triffid_finale",
        "triffid_roots"
    };

    return hardcoded_mapgen.find( get_mapgen_id() ) != hardcoded_mapgen.end();
}

void overmap_terrains::load( const JsonObject &jo, const std::string &src )
{
    terrain_types.load( jo, src );
}

void overmap_terrains::check_consistency()
{
    for( const auto &elem : terrain_types.get_all() ) {
        elem.check();
        if( elem.static_spawns.group && !elem.static_spawns.group.is_valid() ) {
            debugmsg( "Invalid monster group \"%s\" in spawns of \"%s\".", elem.static_spawns.group.c_str(),
                      elem.id.c_str() );
        }
    }

    for( const auto &elem : terrains.get_all() ) {
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
        } else if( !exists_hardcoded ) {
            debugmsg( "No mapgen terrain exists for \"%s\".", mid.c_str() );
        }
    }
}

void overmap_terrains::finalize()
{
    terrain_types.finalize();

    for( const auto &elem : terrain_types.get_all() ) {
        const_cast<oter_type_t &>( elem ).finalize(); // This cast is ugly, but safe.
    }

    if( region_settings_map.find( "default" ) == region_settings_map.end() ) {
        debugmsg( "ERROR: can't find default overmap settings (region_map_settings 'default'), "
                  "cataclysm pending.  And not the fun kind." );
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

bool overmap_special_terrain::can_be_placed_on( const oter_id &oter ) const
{
    return std::any_of( locations.begin(), locations.end(),
    [&oter]( const string_id<overmap_location> &loc ) {
        return loc->test( oter );
    } );
}

const overmap_special_terrain &overmap_special::get_terrain_at( const tripoint &p ) const
{
    const auto iter = std::find_if( terrains.begin(),
    terrains.end(), [ &p ]( const overmap_special_terrain & elem ) {
        return elem.p == p;
    } );
    if( iter == terrains.end() ) {
        static const overmap_special_terrain null_terrain{};
        return null_terrain;
    }
    return *iter;
}

bool overmap_special::requires_city() const
{
    return city_size.min > 0 || city_distance.max < std::max( OMAPX, OMAPY );
}

bool overmap_special::can_belong_to_city( const tripoint_om_omt &p, const city &cit ) const
{
    if( !requires_city() ) {
        return true;
    }
    if( !cit || !city_size.contains( cit.size ) ) {
        return false;
    }
    return city_distance.contains( cit.get_distance_from( p ) );
}

void overmap_special::load( const JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";
    // city_building is just an alias of overmap_special
    // TODO: This comparison is a hack. Separate them properly.
    const bool is_special = jo.get_string( "type", "" ) == "overmap_special";

    mandatory( jo, was_loaded, "overmaps", terrains );
    optional( jo, was_loaded, "locations", default_locations );

    if( is_special ) {
        mandatory( jo, was_loaded, "occurrences", occurrences );

        optional( jo, was_loaded, "connections", connections );

        assign( jo, "city_sizes", city_size, strict );
        assign( jo, "city_distance", city_distance, strict );
    }

    assign( jo, "spawns", spawns, strict );

    assign( jo, "rotate", rotatable, strict );
    assign( jo, "flags", flags, strict );
}

void overmap_special::finalize()
{
    // If the special has default locations, then add those to the locations
    // of each of the terrains IF the terrain has no locations already.
    if( !default_locations.empty() ) {
        for( auto &t : terrains ) {
            if( t.locations.empty() ) {
                t.locations.insert( default_locations.begin(), default_locations.end() );
            }
        }
    }

    for( auto &elem : connections ) {
        const auto &oter = get_terrain_at( elem.p );
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
        // and use that as the intial direction to be passed off to the connection
        // building code.
        if( elem.from ) {
            const direction calculated_direction = direction_from( *elem.from, elem.p );
            switch( calculated_direction ) {
                case direction::NORTH:
                    elem.initial_dir = om_direction::type::north;
                    break;
                case direction::EAST:
                    elem.initial_dir = om_direction::type::east;
                    break;
                case direction::SOUTH:
                    elem.initial_dir = om_direction::type::south;
                    break;
                case direction::WEST:
                    elem.initial_dir = om_direction::type::west;
                    break;
                default:
                    // The only supported directions are north/east/south/west
                    // as those are the four directions that overmap connections
                    // can be made in. If the direction we figured out wasn't
                    // one of those, just set this as invalid. We'll provide
                    // a warning to the user/developer in overmap_special::check().
                    elem.initial_dir = om_direction::type::invalid;
                    break;
            }
        }
    }
}

void overmap_special::check() const
{
    std::set<oter_str_id> invalid_terrains;
    std::set<tripoint> points;

    for( const overmap_special_terrain &elem : terrains ) {
        const oter_str_id &oter = elem.terrain;

        if( !oter.is_valid() ) {
            if( invalid_terrains.count( oter ) == 0 ) {
                // Not a huge fan of the the direct id manipulation here, but I don't know
                // how else to do this
                oter_str_id invalid( oter.str() + "_north" );
                if( invalid.is_valid() ) {
                    debugmsg( "In overmap special \"%s\", terrain \"%s\" rotates, but is specified without a rotation.",
                              id.str(), oter.str() );
                } else  {
                    debugmsg( "In overmap special \"%s\", terrain \"%s\" is invalid.",
                              id.c_str(), oter.c_str() );
                }
                invalid_terrains.insert( oter );
            }
        }

        const auto &pos = elem.p;

        if( points.count( pos ) > 0 ) {
            debugmsg( "In overmap special \"%s\", point [%d,%d,%d] is duplicated.",
                      id.c_str(), pos.x, pos.y, pos.z );
        } else {
            points.insert( pos );
        }

        if( elem.locations.empty() ) {
            debugmsg( "In overmap special \"%s\", no location is defined for point [%d,%d,%d] or the overall special.",
                      id.c_str(), pos.x, pos.y, pos.z );
        }

        for( const auto &l : elem.locations ) {
            if( !l.is_valid() ) {
                debugmsg( "In overmap special \"%s\", point [%d,%d,%d], location \"%s\" is invalid.",
                          id.c_str(), pos.x, pos.y, pos.z, l.c_str() );
            }
        }
    }

    for( const auto &elem : connections ) {
        const auto &oter = get_terrain_at( elem.p );
        if( !elem.terrain ) {
            debugmsg( "In overmap special \"%s\", connection [%d,%d,%d] doesn't have a terrain.",
                      id.c_str(), elem.p.x, elem.p.y, elem.p.z );
        } else if( !elem.existing && !elem.terrain->has_flag( oter_flags::line_drawing ) ) {
            debugmsg( "In overmap special \"%s\", connection [%d,%d,%d] \"%s\" isn't drawn with lines.",
                      id.c_str(), elem.p.x, elem.p.y, elem.p.z, elem.terrain.c_str() );
        } else if( !elem.existing && oter.terrain && !oter.terrain->type_is( elem.terrain ) ) {
            debugmsg( "In overmap special \"%s\", connection [%d,%d,%d] overwrites \"%s\".",
                      id.c_str(), elem.p.x, elem.p.y, elem.p.z, oter.terrain.c_str() );
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
                    debugmsg( "In overmap special \"%s\", connection [%d,%d,%d] is not directly north, east, south or west of the defined \"from\" [%d,%d,%d].",
                              id.c_str(), elem.p.x, elem.p.y, elem.p.z, elem.from->x, elem.from->y, elem.from->z );
                    break;
            }
        }
    }
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
    settings = rsit->second;

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

    const bool should_blacklist = !settings.overmap_feature_flag.blacklist.empty();
    const bool should_whitelist = !settings.overmap_feature_flag.whitelist.empty();

    // If this region's settings has blacklisted or whitelisted overmap feature flags, let's
    // filter our default batch.

    // Remove any items that have a flag that is present in the blacklist.
    if( should_blacklist ) {
        for( auto it = enabled_specials.begin(); it != enabled_specials.end(); ) {
            std::vector<std::string> common;
            std::set_intersection( settings.overmap_feature_flag.blacklist.begin(),
                                   settings.overmap_feature_flag.blacklist.end(),
                                   it->special_details->flags.begin(), it->special_details->flags.end(),
                                   std::back_inserter( common )
                                 );
            if( !common.empty() ) {
                it = enabled_specials.erase( it );
            } else {
                ++it;
            }
        }
    }

    // Remove any items which do not have any of the flags from the whitelist.
    if( should_whitelist ) {
        for( auto it = enabled_specials.begin(); it != enabled_specials.end(); ) {
            std::vector<std::string> common;
            std::set_intersection( settings.overmap_feature_flag.whitelist.begin(),
                                   settings.overmap_feature_flag.whitelist.end(),
                                   it->special_details->flags.begin(), it->special_details->flags.end(),
                                   std::back_inserter( common )
                                 );
            if( common.empty() ) {
                it = enabled_specials.erase( it );
            } else {
                ++it;
            }
        }
    }

    populate( enabled_specials );
}

oter_id overmap::get_default_terrain( int z ) const
{
    if( z == 0 ) {
        return settings.default_oter.id();
    } else {
        // // TODO: Get rid of the hard-coded ids.
        static const oter_str_id open_air( "open_air" );
        static const oter_str_id empty_rock( "empty_rock" );

        return z > 0 ? open_air.id() : empty_rock.id();
    }
}

void overmap::init_layers()
{
    for( int k = 0; k < OVERMAP_LAYERS; ++k ) {
        const oter_id tid = get_default_terrain( k - OVERMAP_DEPTH );

        for( int i = 0; i < OMAPX; ++i ) {
            for( int j = 0; j < OMAPY; ++j ) {
                layer[k].terrain[i][j] = tid;
                layer[k].visible[i][j] = false;
                layer[k].explored[i][j] = false;
            }
        }
    }
}

void overmap::ter_set( const tripoint_om_omt &p, const oter_id &id )
{
    if( !inbounds( p ) ) {
        /// TODO: Add a debug message reporting this, but currently there are way too many place that would trigger it.
        return;
    }

    layer[p.z() + OVERMAP_DEPTH].terrain[p.x()][p.y()] = id;
}

const oter_id &overmap::ter( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {
        /// TODO: Add a debug message reporting this, but currently there are way too many place that would trigger it.
        return ot_null;
    }

    return layer[p.z() + OVERMAP_DEPTH].terrain[p.x()][p.y()];
}

bool &overmap::seen( const tripoint_om_omt &p )
{
    if( !inbounds( p ) ) {
        nullbool = false;
        return nullbool;
    }
    return layer[p.z() + OVERMAP_DEPTH].visible[p.x()][p.y()];
}

bool overmap::seen( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    return layer[p.z() + OVERMAP_DEPTH].visible[p.x()][p.y()];
}

bool &overmap::explored( const tripoint_om_omt &p )
{
    if( !inbounds( p ) ) {
        nullbool = false;
        return nullbool;
    }
    return layer[p.z() + OVERMAP_DEPTH].explored[p.x()][p.y()];
}

bool overmap::is_explored( const tripoint_om_omt &p ) const
{
    if( !inbounds( p ) ) {
        return false;
    }
    return layer[p.z() + OVERMAP_DEPTH].explored[p.x()][p.y()];
}

bool overmap::mongroup_check( const mongroup &candidate ) const
{
    const auto matching_range = zg.equal_range( candidate.pos );
    return std::find_if( matching_range.first, matching_range.second,
    [candidate]( const std::pair<tripoint_om_sm, mongroup> &match ) {
        // This is extra strict since we're using it to test serialization.
        return candidate.type == match.second.type && candidate.pos == match.second.pos &&
               candidate.radius == match.second.radius &&
               candidate.population == match.second.population &&
               candidate.target == match.second.target &&
               candidate.interest == match.second.interest &&
               candidate.dying == match.second.dying &&
               candidate.horde == match.second.horde &&
               candidate.diffuse == match.second.diffuse;
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
    for( auto &i : layer[p.z() + OVERMAP_DEPTH].notes ) {
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
    for( const auto &note : this_layer.notes ) {
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

const string_id<map_extra> &overmap::extra( const tripoint_om_omt &p ) const
{
    static const string_id<map_extra> fallback{};

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

void overmap::add_extra( const tripoint_om_omt &p, const string_id<map_extra> &id )
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
    } else if( !id.is_null() ) {
        it->id = id;
    } else {
        extras.erase( it );
    }
}

void overmap::delete_extra( const tripoint_om_omt &p )
{
    add_extra( p, string_id<map_extra>::NULL_ID() );
}

std::vector<point_abs_omt> overmap::find_extras( const int z, const std::string &text )
{
    std::vector<point_abs_omt> extra_locations;
    map_layer &this_layer = layer[z + OVERMAP_DEPTH];
    for( const auto &extra : this_layer.extras ) {
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
    if( g->gametype() == special_game_type::DEFENSE ) {
        dbg( D_INFO ) << "overmap::generate skipped in Defense special game mode!";
        return;
    }

    dbg( D_INFO ) << "overmap::generate start…";

    populate_connections_out_from_neighbors( north, east, south, west );

    place_rivers( north, east, south, west );
    place_lakes();
    place_forests();
    place_swamps();
    place_ravines();
    place_cities();
    place_forest_trails();
    place_roads( north, east, south, west );
    place_specials( enabled_specials );
    place_forest_trailheads();

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
    std::vector<point_om_omt> shaft_points;
    std::vector<city> mine_points;
    // These are so common that it's worth checking first as int.
    const oter_id skip_above[5] = {
        oter_id( "empty_rock" ), oter_id( "forest" ), oter_id( "field" ),
        oter_id( "forest_thick" ), oter_id( "forest_water" )
    };

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            tripoint_om_omt p( i, j, z );
            const oter_id oter_above = ter( p + tripoint_above );
            const oter_id oter_ground = ter( tripoint_om_omt( p.xy(), 0 ) );

            if( is_ot_match( "microlab_sub_connector", ter( p ), ot_match_type::type ) ) {
                om_direction::type rotation = ter( p )->get_dir();
                ter_set( p, oter_id( "subway_end_north" )->get_rotated( rotation ) );
                subway_points.emplace_back( p.xy() );
            }

            // implicitly skip skip_above oter_ids
            bool skipme = false;
            for( const oter_id &elem : skip_above ) {
                if( oter_above == elem ) {
                    skipme = true;
                    break;
                }
            }
            if( skipme ) {
                continue;
            }

            if( is_ot_match( "sub_station", oter_ground, ot_match_type::type ) && z == -1 ) {
                ter_set( p, oter_id( "sewer_sub_station" ) );
                requires_sub = true;
            } else if( is_ot_match( "sub_station", oter_ground, ot_match_type::type ) && z == -2 ) {
                ter_set( p, oter_id( "subway_isolated" ) );
                subway_points.emplace_back( i, j - 1 );
                subway_points.emplace_back( i, j );
                subway_points.emplace_back( i, j + 1 );
            } else if( oter_above == "road_nesw_manhole" ) {
                ter_set( p, oter_id( "sewer_isolated" ) );
                sewer_points.emplace_back( i, j );
            } else if( oter_above == "sewage_treatment" ) {
                sewer_points.emplace_back( i, j );
            } else if( oter_above == "anthill" || oter_above == "acid_anthill" ) {
                const int size = rng( MIN_ANT_SIZE, MAX_ANT_SIZE );
                ant_points.push_back( city( p.xy(), size ) );
            } else if( oter_above == "slimepit_down" ) {
                const int size = rng( MIN_GOO_SIZE, MAX_GOO_SIZE );
                goo_points.push_back( city( p.xy(), size ) );
            } else if( oter_above == "forest_water" ) {
                ter_set( p, oter_id( "cavern" ) );
                chip_rock( p );
            } else if( oter_above == "lab_core" ||
                       ( z == -1 && oter_above == "lab_stairs" ) ) {
                lab_points.push_back( city( p.xy(), rng( 1, 5 + z ) ) );
            } else if( oter_above == "lab_stairs" ) {
                ter_set( p, oter_id( "lab" ) );
            } else if( oter_above == "ice_lab_core" ||
                       ( z == -1 && oter_above == "ice_lab_stairs" ) ) {
                ice_lab_points.push_back( city( p.xy(), rng( 1, 5 + z ) ) );
            } else if( oter_above == "ice_lab_stairs" ) {
                ter_set( p, oter_id( "ice_lab" ) );
            } else if( oter_above == "central_lab_core" ) {
                central_lab_points.push_back( city( p.xy(), rng( std::max( 1, 7 + z ), 9 + z ) ) );
            } else if( oter_above == "central_lab_stairs" ) {
                ter_set( p, oter_id( "central_lab" ) );
            } else if( is_ot_match( "hidden_lab_stairs", oter_above, ot_match_type::contains ) ) {
                lab_points.push_back( city( p.xy(), rng( 1, 5 + z ) ) );
            } else if( oter_above == "mine_entrance" ) {
                shaft_points.push_back( p.xy() );
            } else if( oter_above == "mine_shaft" ||
                       oter_above == "mine_down" ) {
                ter_set( p, oter_id( "mine" ) );
                mine_points.push_back( city( p.xy(), rng( 6 + z, 10 + z ) ) );
                // technically not all finales need a sub level,
                // but at this point we don't know
                requires_sub = true;
            } else if( oter_above == "mine_finale" ) {
                for( const tripoint_om_omt &q : points_in_radius( p, 1, 0 ) ) {
                    ter_set( q, oter_id( "spiral" ) );
                }
                ter_set( p, oter_id( "spiral_hub" ) );
                add_mon_group( mongroup( GROUP_SPIRAL, tripoint( i * 2, j * 2, z ), 2, 200 ) );
            }
        }
    }

    for( auto &i : goo_points ) {
        requires_sub |= build_slimepit( tripoint_om_omt( i.pos, z ), i.size );
    }
    const string_id<overmap_connection> sewer_tunnel( "sewer_tunnel" );
    connect_closest_points( sewer_points, z, *sewer_tunnel );

    // A third of overmaps have labs with a 1-in-2 chance of being subway connected.
    // If the central lab exists, all labs which go down to z=4 will have a subway to central.
    int lab_train_odds = 0;
    if( z == -2 && one_in( 3 ) ) {
        lab_train_odds = 2;
    }
    if( z == -4 && !central_lab_points.empty() ) {
        lab_train_odds = 1;
    }

    for( auto &i : lab_points ) {
        bool lab = build_lab( tripoint_om_omt( i.pos, z ), i.size, &lab_train_points, "", lab_train_odds );
        requires_sub |= lab;
        if( !lab && ter( tripoint_om_omt( i.pos, z ) ) == "lab_core" ) {
            ter_set( tripoint_om_omt( i.pos, z ), oter_id( "lab" ) );
        }
    }
    for( auto &i : ice_lab_points ) {
        bool ice_lab = build_lab( tripoint_om_omt( i.pos, z ), i.size, &lab_train_points, "ice_",
                                  lab_train_odds );
        requires_sub |= ice_lab;
        if( !ice_lab && ter( tripoint_om_omt( i.pos, z ) ) == "ice_lab_core" ) {
            ter_set( tripoint_om_omt( i.pos, z ), oter_id( "ice_lab" ) );
        }
    }
    for( auto &i : central_lab_points ) {
        bool central_lab = build_lab( tripoint_om_omt( i.pos, z ), i.size, &central_lab_train_points,
                                      "central_", lab_train_odds );
        requires_sub |= central_lab;
        if( !central_lab && ter( tripoint_om_omt( i.pos, z ) ) == "central_lab_core" ) {
            ter_set( tripoint_om_omt( i.pos, z ), oter_id( "central_lab" ) );
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
                ter_set( i, oter_id( "open_air" ) ); // mark tile to prevent subway gen

                for( const tripoint_om_omt &nearby_loc : nearby_locations ) {
                    if( is_ot_match( "empty_rock", ter( nearby_loc ), ot_match_type::contains ) ) {
                        // mark tile to prevent subway gen
                        ter_set( nearby_loc, oter_id( "open_air" ) );
                    }
                }
            } else {
                // change train connection point back to rock to allow gen
                if( is_ot_match( "open_air", ter( i ), ot_match_type::contains ) ) {
                    ter_set( i, oter_id( "empty_rock" ) );
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

    const string_id<overmap_connection> subway_tunnel( "subway_tunnel" );

    subway_points.insert( subway_points.end(), subway_lab_train_points.begin(),
                          subway_lab_train_points.end() );
    connect_closest_points( subway_points, z, *subway_tunnel );

    for( auto &i : subway_points ) {
        if( is_ot_match( "sub_station", ter( tripoint_om_omt( i, z + 2 ) ), ot_match_type::type ) ) {
            ter_set( tripoint_om_omt( i, z ), oter_id( "underground_sub_station" ) );
        }
    }

    // The first lab point is adjacent to a lab, set it a depot (as long as track was actually laid).
    const auto create_train_depots = [this, z, &subway_tunnel]( const oter_id & train_type,
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
                ter_set( i, oter_id( "empty_rock" ) ); // this clears marked tiles
                bool is_depot_generated = false;
                for( const tripoint_om_omt &subway_loc : subway_possible_loc ) {
                    if( !is_depot_generated &&
                        is_ot_match( "subway", ter( subway_loc ), ot_match_type::contains ) ) {
                        extra_route.push_back( i.xy() );
                        extra_route.push_back( subway_loc.xy() );
                        connect_closest_points( extra_route, z, *subway_tunnel );

                        ter_set( i, train_type );
                        is_depot_generated = true; // only one connection to depot
                    } else if( is_ot_match( "open_air", ter( subway_loc ),
                                            ot_match_type::contains ) ) {
                        // clear marked
                        ter_set( subway_loc, oter_id( "empty_rock" ) );
                    }
                }
            }
            is_first_in_pair = !is_first_in_pair;
        }
    };
    create_train_depots( oter_id( "lab_train_depot" ), lab_train_points );
    create_train_depots( oter_id( "central_lab_train_depot" ), central_lab_train_points );

    for( auto &i : cities ) {
        tripoint_om_omt omt_pos( i.pos, z );
        tripoint_om_sm sm_pos = project_to<coords::sm>( omt_pos );
        // Sewers and city subways are present at z == -1 and z == -2. Don't spawn CHUD on other z-levels.
        if( ( z == -1 || z == -2 ) && one_in( 3 ) ) {
            add_mon_group( mongroup( GROUP_CHUD,
                                     sm_pos, i.size, i.size * 20 ) );
        }
        // Sewers are present at z == -1. Don't spawn sewer monsters on other z-levels.
        if( z == -1 && !one_in( 8 ) ) {
            add_mon_group( mongroup( GROUP_SEWER,
                                     sm_pos, ( i.size * 7 ) / 2, i.size * 70 ) );
        }
    }

    for( auto &i : mine_points ) {
        build_mine( tripoint_om_omt( i.pos, z ), i.size );
    }

    for( auto &i : shaft_points ) {
        ter_set( tripoint_om_omt( i, z ), oter_id( "mine_shaft" ) );
        requires_sub = true;
    }
    for( auto &i : ant_points ) {
        const tripoint_om_omt p_loc( i.pos, z );
        if( ter( p_loc ) != "empty_rock" ) {
            continue;
        }
        mongroup_id ant_group( ter( p_loc + tripoint_above ) == "anthill" ?
                               "GROUP_ANT" : "GROUP_ANT_ACID" );
        add_mon_group(
            mongroup( ant_group, tripoint_om_sm( project_to<coords::sm>( i.pos ), z ),
                      ( i.size * 3 ) / 2, rng( 6000, 8000 ) ) );
        build_anthill( p_loc, i.size, ter( p_loc + tripoint_above ) == "anthill" );
    }

    return requires_sub;
}

bool overmap::generate_over( const int z )
{
    bool requires_over = false;
    std::vector<point_om_omt> bridge_points;

    // These are so common that it's worth checking first as int.
    const std::set<oter_id> skip_below = {
        oter_id( "empty_rock" ), oter_id( "forest" ), oter_id( "field" ),
        oter_id( "forest_thick" ), oter_id( "forest_water" )
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

                if( is_ot_match( "bridge", oter_ground, ot_match_type::type ) ) {
                    ter_set( p, oter_id( "bridge_road" + oter_get_rotation_string( oter_ground ) ) );
                    bridge_points.push_back( p.xy() );
                }
            }
        }
    }

    generate_bridgeheads( bridge_points );

    return requires_over;
}

void overmap::generate_bridgeheads( const std::vector<point_om_omt> &bridge_points )
{
    std::vector<std::pair<point_om_omt, std::string>> bridgehead_points;
    for( const point_om_omt &bp : bridge_points ) {
        //const oter_id oter_ground = ter( tripoint_om_omt( bp, 0 ) );
        const oter_id oter_ground_north = ter( tripoint_om_omt( bp, 0 ) + tripoint_north );
        const oter_id oter_ground_south = ter( tripoint_om_omt( bp, 0 ) + tripoint_south );
        const oter_id oter_ground_east = ter( tripoint_om_omt( bp, 0 ) + tripoint_east );
        const oter_id oter_ground_west = ter( tripoint_om_omt( bp, 0 ) + tripoint_west );
        const bool is_bridge_north = is_ot_match( "bridge", oter_ground_north, ot_match_type::type )
                                     && ( oter_get_rotation( oter_ground_north ) % 2 == 0 );
        const bool is_bridge_south = is_ot_match( "bridge", oter_ground_south, ot_match_type::type )
                                     && ( oter_get_rotation( oter_ground_south ) % 2 == 0 );
        const bool is_bridge_east = is_ot_match( "bridge", oter_ground_east, ot_match_type::type )
                                    && ( oter_get_rotation( oter_ground_east ) % 2 == 1 );
        const bool is_bridge_west = is_ot_match( "bridge", oter_ground_west, ot_match_type::type )
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
        ter_set( p, oter_id( "bridgehead_ground" + bhp.second ) );
        ter_set( p + tripoint_above, oter_id( "bridgehead_ramp" + bhp.second ) );
    }
}

std::vector<point_abs_omt> overmap::find_terrain( const std::string &term, int zlevel )
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
    for( const auto &elem : cities ) {
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

tripoint_om_omt overmap::find_random_omt( const std::pair<std::string, ot_match_type> &target )
const
{
    std::vector<tripoint_om_omt> valid;
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            for( int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++ ) {
                tripoint_om_omt p( i, j, k );
                if( is_ot_match( target.first, ter( p ), target.second ) ) {
                    valid.push_back( p );
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
            mg.radius = ( mg.radius * 9 ) / 10;
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

void overmap::place_special_forced( const overmap_special_id &special_id, const tripoint_om_omt &p,
                                    om_direction::type dir )
{
    static city invalid_city;
    place_special( *special_id, p, dir, invalid_city, false, false );
}

void mongroup::wander( const overmap &om )
{
    const city *target_city = nullptr;
    int target_distance = 0;

    if( horde_behaviour == "city" ) {
        // Find a nearby city to return to..
        for( const city &check_city : om.cities ) {
            // Check if this is the nearest city so far.
            int distance = rl_dist( project_to<coords::sm>( check_city.pos ),
                                    pos.xy() );
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
        target.x() = target_city->pos.x() * 2 + rng( -target_city->size * 2, target_city->size * 2 );
        target.y() = target_city->pos.y() * 2 + rng( -target_city->size * 2, target_city->size * 2 );
        interest = 100;
    } else {
        target.x() = pos.x() + rng( -10, 10 );
        target.y() = pos.y() + rng( -10, 10 );
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
        if( !mg.horde ) {
            ++it;
            continue;
        }

        if( mg.horde_behaviour.empty() ) {
            mg.horde_behaviour = one_in( 2 ) ? "city" : "roam";
        }

        // Gradually decrease interest.
        mg.dec_interest( 1 );

        if( ( mg.pos.xy() == mg.target.xy() ) || mg.interest <= 15 ) {
            mg.wander( *this );
        }

        // Decrease movement chance according to the terrain we're currently on.
        const oter_id &walked_into = ter( project_to<coords::omt>( mg.pos ) );
        int movement_chance = 1;
        if( walked_into == ot_forest || walked_into == ot_forest_water ) {
            movement_chance = 3;
        } else if( walked_into == ot_forest_thick ) {
            movement_chance = 6;
        } else if( walked_into == ot_river_center ) {
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
            if( mg.pos.x() > mg.target.x() ) {
                mg.pos.x()--;
            }
            if( mg.pos.x() < mg.target.x() ) {
                mg.pos.x()++;
            }
            if( mg.pos.y() > mg.target.y() ) {
                mg.pos.y()--;
            }
            if( mg.pos.y() < mg.target.y() ) {
                mg.pos.y()++;
            }

            // Erase the group at it's old location, add the group with the new location
            tmpzg.insert( std::pair<tripoint_om_sm, mongroup>( mg.pos, mg ) );
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
            const auto &p = monster_map_it->first;
            auto &this_monster = monster_map_it->second;

            // Only zombies on z-level 0 may join hordes.
            if( p.z() != 0 ) {
                monster_map_it++;
                continue;
            }

            // Check if the monster is a zombie.
            const mtype &type = *this_monster.type;
            if(
                !type.species.count( species_ZOMBIE ) || // Only add zombies to hordes.
                type.id == mtype_id( "mon_jabberwock" ) || // Jabberwockies are an exception.
                this_monster.get_speed() <= 30 || // So are very slow zombies, like crawling zombies.
                !this_monster.will_join_horde( INT_MAX ) || // So are zombies who won't join a horde of any size.
                !this_monster.mission_ids.empty() // We mustn't delete monsters that are related to missions.
            ) {
                // Don't delete the monster, just increment the iterator.
                monster_map_it++;
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
                    mongroup m( GROUP_ZOMBIE, p, 1, 0 );
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

/**
* @param p location of signal relative to this overmap origin
* @param sig_power - power of signal or max distance for reaction of zombies
*/
void overmap::signal_hordes( const tripoint_rel_sm &p_rel, const int sig_power )
{
    tripoint_om_sm p( p_rel.raw() );
    for( auto &elem : zg ) {
        mongroup &mg = elem.second;
        if( !mg.horde ) {
            continue;
        }
        const int dist = rl_dist( p, mg.pos );
        if( sig_power < dist ) {
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
            const int targ_dist = rl_dist( p, mg.target );
            // TODO: Base this on targ_dist:dist ratio.
            if( targ_dist < 5 ) {  // If signal source already pursued by horde
                mg.set_target( midpoint( mg.target.xy(), p.xy() ) );
                const int min_inc_inter = 3; // Min interest increase to already targeted source
                const int inc_roll = rng( min_inc_inter, calculated_inter );
                mg.inc_interest( inc_roll );
                add_msg_debug( "horde inc interest %d dist %d", inc_roll, dist );
            } else { // New signal source
                mg.set_target( p.xy() );
                mg.set_interest( min_capped_inter );
                add_msg_debug( "horde set interest %d dist %d", min_capped_inter, dist );
            }
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
        const auto current_terrain = ter( tripoint_om_omt( p, 0 ) );
        return current_terrain == "forest" || current_terrain == "forest_thick" ||
               current_terrain == "forest_water";
    };

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
                ( settings.forest_trail.minimum_forest_size ) ) {
                continue;
            }

            // If we don't rng a forest based on our settings, move on.
            if( !one_in( settings.forest_trail.chance ) ) {
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
            int max_random_points = settings.forest_trail.random_point_min + forest_points.size() /
                                    settings.forest_trail.random_point_size_scalar;
            max_random_points = std::min( max_random_points, settings.forest_trail.random_point_max );

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
            if( one_in( settings.forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( northmost );
            }
            if( one_in( settings.forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( southmost );
            }
            if( one_in( settings.forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( westmost );
            }
            if( one_in( settings.forest_trail.border_point_chance ) ) {
                chosen_points.emplace_back( eastmost );
            }

            // Finally, connect all the points and make a forest trail out of them.
            const string_id<overmap_connection> forest_trail( "forest_trail" );
            connect_closest_points( chosen_points, 0, *forest_trail );
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
                 settings.forest_trail.trailhead_road_distance
             ) ) {
            if( check_ot( "road", ot_match_type::contains, nearby_point ) ) {
                close = true;
            }
        }
        return close;
    };

    const auto try_place_trailhead_special = [&]( const tripoint_om_omt & trail_end,
    const om_direction::type & dir ) {
        const tripoint_om_omt potential_trailhead = trail_end + om_direction::displace( dir, 1 );
        overmap_special_id trailhead = settings.forest_trail.trailheads.pick();
        if( one_in( settings.forest_trail.trailhead_chance ) &&
            trailhead_close_to_road( potential_trailhead ) &&
            can_place_special( *trailhead, potential_trailhead, dir, false ) ) {
            const city &nearest_city = get_nearest_city( potential_trailhead );
            place_special( *trailhead, potential_trailhead, dir, nearest_city, false, false );
        }
    };

    for( int i = 2; i < OMAPX - 2; i++ ) {
        for( int j = 2; j < OMAPY - 2; j++ ) {
            const tripoint_om_omt p( i, j, 0 );
            oter_id oter = ter( p );
            if( is_ot_match( "forest_trail_end", oter, ot_match_type::prefix ) ) {
                try_place_trailhead_special( p, oter->get_dir() );
            }
        }
    }
}

void overmap::place_forests()
{
    const oter_id default_oter_id( settings.default_oter );
    const oter_id forest( "forest" );
    const oter_id forest_thick( "forest_thick" );

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
            if( n > settings.overmap_forest.noise_threshold_forest_thick ) {
                ter_set( p, forest_thick );
            } else if( n > settings.overmap_forest.noise_threshold_forest ) {
                ter_set( p, forest );
            }
        }
    }
}

void overmap::place_lakes()
{
    const om_noise::om_noise_layer_lake f( global_base_point(), g->get_seed() );

    const auto is_lake = [&]( const point_om_omt & p ) {
        return f.noise_at( p ) > settings.overmap_lake.noise_threshold_lake;
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
                ( settings.overmap_lake.lake_size_min ) ) {
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
                    for( int z = -1; z > settings.overmap_lake.lake_depth; z-- ) {
                        ter_set( tripoint_om_omt( p, z ), lake_water_cube );
                    }
                    ter_set( tripoint_om_omt( p, settings.overmap_lake.lake_depth ), lake_bed );
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

void overmap::place_rivers( const overmap *north, const overmap *east, const overmap *south,
                            const overmap *west )
{
    if( settings.river_scale == 0.0 ) {
        return;
    }
    int river_chance = static_cast<int>( std::max( 1.0, 1.0 / settings.river_scale ) );
    int river_scale = static_cast<int>( std::max( 1.0, settings.river_scale ) );
    // West/North endpoints of rivers
    std::vector<point_om_omt> river_start;
    // East/South endpoints of rivers
    std::vector<point_om_omt> river_end;

    // Determine points where rivers & roads should connect w/ adjacent maps
    // optimized comparison.
    const oter_id river_center( "river_center" );

    if( north != nullptr ) {
        for( int i = 2; i < OMAPX - 2; i++ ) {
            const tripoint_om_omt p_neighbour( i, OMAPY - 1, 0 );
            const tripoint_om_omt p_mine( i, 0, 0 );

            if( is_river( north->ter( p_neighbour ) ) ) {
                ter_set( p_mine, river_center );
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
                ter_set( p_mine, river_center );
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
                ter_set( p_mine, river_center );
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
                ter_set( p_mine, river_center );
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
                new_rivers.push_back( point_om_omt( rng( 10, OMAPX - 11 ), 0 ) );
            }
            if( west == nullptr && one_in( river_chance ) ) {
                new_rivers.push_back( point_om_omt( 0, rng( 10, OMAPY - 11 ) ) );
            }
            river_start.push_back( random_entry( new_rivers ) );
        }
    }
    if( south == nullptr || east == nullptr ) {
        while( river_end.empty() || river_end.size() + 1 < river_start.size() ) {
            new_rivers.clear();
            if( south == nullptr && one_in( river_chance ) ) {
                new_rivers.push_back( point_om_omt( rng( 10, OMAPX - 11 ), OMAPY - 1 ) );
            }
            if( east == nullptr && one_in( river_chance ) ) {
                new_rivers.push_back( point_om_omt( OMAPX - 1, rng( 10, OMAPY - 11 ) ) );
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
            river_start.push_back( point_om_omt( rng( OMAPX / 4, ( OMAPX * 3 ) / 4 ),
                                                 rng( OMAPY / 4, ( OMAPY * 3 ) / 4 ) ) );
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
    std::vector<std::vector<int>> floodplain( OMAPX, std::vector<int>( OMAPY, 0 ) );
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            const tripoint_om_omt pos( x, y, 0 );
            if( is_ot_match( "river", ter( pos ), ot_match_type::contains ) ) {
                std::vector<point_om_omt> buffered_points =
                    closest_points_first(
                        pos.xy(),
                        rng( settings.overmap_forest.river_floodplain_buffer_distance_min,
                             settings.overmap_forest.river_floodplain_buffer_distance_max ) );
                for( const point_om_omt &p : buffered_points )  {
                    if( !inbounds( p ) ) {
                        continue;
                    }
                    floodplain[p.x()][p.y()] += 1;
                }
            }
        }
    }

    const oter_id forest_water( "forest_water" );

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
                                        > settings.overmap_forest.noise_threshold_swamp_adjacent_water );

            // If this location meets our isolated swamp threshold, regardless of floodplain values, we'll make it
            // into a swamp.
            const bool should_isolated_swamp = f.noise_at( pos.xy() ) >
                                               settings.overmap_forest.noise_threshold_swamp_isolated;
            if( should_flood || should_isolated_swamp )  {
                ter_set( pos, forest_water );
            }
        }
    }
}

void overmap::place_roads( const overmap *north, const overmap *east, const overmap *south,
                           const overmap *west )
{
    const string_id<overmap_connection> local_road( "local_road" );
    std::vector<tripoint_om_omt> &roads_out = connections_out[local_road];

    // At least 3 exit points, to guarantee road continuity across overmaps
    if( roads_out.size() < 3 ) {
        std::vector<tripoint_om_omt> viable_roads;
        tripoint_om_omt tmp;
        // Populate viable_roads with one point for each neighborless side.
        // Make sure these points don't conflict with rivers.

        std::array < int, OMAPX - 20 > omap_num;
        for( int i = 0; i < 160; i++ ) {
            omap_num[i] = i + 10;
        }

        if( north == nullptr ) {
            std::shuffle( omap_num.begin(), omap_num.end(), rng_get_engine() );
            for( const auto &i : omap_num ) {
                tmp = tripoint_om_omt( i, 0, 0 );
                if( !( is_river( ter( tmp ) ) || is_river( ter( tmp + point_east ) ) ||
                       is_river( ter( tmp + point_west ) ) ) ) {
                    viable_roads.push_back( tmp );
                    break;
                }
            }
        }
        if( east == nullptr ) {
            std::shuffle( omap_num.begin(), omap_num.end(), rng_get_engine() );
            for( const auto &i : omap_num ) {
                tmp = tripoint_om_omt( OMAPX - 1, i, 0 );
                if( !( is_river( ter( tmp ) ) || is_river( ter( tmp + point_north ) ) ||
                       is_river( ter( tmp + point_south ) ) ) ) {
                    viable_roads.push_back( tmp );
                    break;
                }
            }
        }
        if( south == nullptr ) {
            std::shuffle( omap_num.begin(), omap_num.end(), rng_get_engine() );
            for( const auto &i : omap_num ) {
                tmp = tripoint_om_omt( i, OMAPY - 1, 0 );
                if( !( is_river( ter( tmp ) ) || is_river( ter( tmp + point_east ) ) ||
                       is_river( ter( tmp + point_west ) ) ) ) {
                    viable_roads.push_back( tmp );
                    break;
                }
            }
        }
        if( west == nullptr ) {
            std::shuffle( omap_num.begin(), omap_num.end(), rng_get_engine() );
            for( const auto &i : omap_num ) {
                tmp = tripoint_om_omt( 0, i, 0 );
                if( !( is_river( ter( tmp ) ) || is_river( ter( tmp + point_north ) ) ||
                       is_river( ter( tmp + point_south ) ) ) ) {
                    viable_roads.push_back( tmp );
                    break;
                }
            }
        }
        while( roads_out.size() < 3 && !viable_roads.empty() ) {
            roads_out.push_back( random_entry_removed( viable_roads ) );
        }
    }

    std::vector<point_om_omt> road_points; // cities and roads_out together
    // Compile our master list of roads; it's less messy if roads_out is first
    road_points.reserve( roads_out.size() + cities.size() );
    for( const auto &elem : roads_out ) {
        road_points.emplace_back( elem.xy() );
    }
    for( const auto &elem : cities ) {
        road_points.emplace_back( elem.pos );
    }

    // And finally connect them via roads.
    connect_closest_points( road_points, 0, *local_road );
}

void overmap::place_river( const point_om_omt &pa, const point_om_omt &pb )
{
    const oter_id river_center( "river_center" );
    int river_chance = static_cast<int>( std::max( 1.0, 1.0 / settings.river_scale ) );
    int river_scale = static_cast<int>( std::max( 1.0, settings.river_scale ) );
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
                        ter_set( p, river_center );
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
                        ter_set( p, river_center );
                    }
                }
            }
        }
    } while( pb != p2 );
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
    int op_city_size = get_option<int>( "CITY_SIZE" );
    if( op_city_size <= 0 ) {
        return;
    }
    int op_city_spacing = get_option<int>( "CITY_SPACING" );

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
    const double omts_per_city = ( op_city_size * 2 + 1 ) * ( op_city_size * 2 + 1 ) * 3 / 4.0;

    // how many cities on this overmap?
    const int NUM_CITIES =
        roll_remainder( omts_per_overmap * city_map_coverage_ratio / omts_per_city );

    const string_id<overmap_connection> local_road_id( "local_road" );
    const overmap_connection &local_road( *local_road_id );

    // if there is only a single free tile, the probability of NOT finding it after MAX_PLACEMENT_ATTEMPTS attempts
    // is (1 - 1/(OMAPX * OMAPY))^MAX_PLACEMENT_ATTEMPTS ≈ 36% for the OMAPX=OMAPY=180 and MAX_PLACEMENT_ATTEMPTS=OMAPX * OMAPY
    const int MAX_PLACEMENT_ATTEMPTS = OMAPX * OMAPY;
    int placement_attempts = 0;

    // place a seed for NUM_CITIES cities, and maybe one more
    while( cities.size() < static_cast<size_t>( NUM_CITIES ) &&
           placement_attempts < MAX_PLACEMENT_ATTEMPTS ) {
        placement_attempts++;

        // randomly make some cities smaller or larger
        int size = rng( op_city_size - 1, op_city_size + 1 );
        if( one_in( 3 ) ) {      // 33% tiny
            size = size * 1 / 3;
        } else if( one_in( 2 ) ) { // 33% small
            size = size * 2 / 3;
        } else if( one_in( 2 ) ) { // 17% large
            size = size * 3 / 2;
        } else {                 // 17% huge
            size = size * 2;
        }
        size = std::max( size, 1 );

        // TODO: put cities closer to the edge when they can span overmaps
        // don't draw cities across the edge of the map, they will get clipped
        point_om_omt c( rng( size - 1, OMAPX - size ), rng( size - 1, OMAPY - size ) );
        const tripoint_om_omt p( c, 0 );

        if( ter( p ) == settings.default_oter ) {
            placement_attempts = 0;
            ter_set( p, oter_id( "road_nesw" ) ); // every city starts with an intersection
            city tmp;
            tmp.pos = p.xy();
            tmp.size = size;
            cities.push_back( tmp );

            const om_direction::type start_dir = om_direction::random();
            om_direction::type cur_dir = start_dir;

            do {
                build_city_street( local_road, tmp.pos, size, cur_dir, tmp );
            } while( ( cur_dir = om_direction::turn_right( cur_dir ) ) != start_dir );
        }
    }
}

overmap_special_id overmap::pick_random_building_to_place( int town_dist ) const
{
    int shop_radius = settings.city_spec.shop_radius;
    int park_radius = settings.city_spec.park_radius;

    int shop_sigma = settings.city_spec.shop_sigma;
    int park_sigma = settings.city_spec.park_sigma;

    //Normally distribute shops and parks
    //Clamp at 1/2 radius to prevent houses from spawning in the city center.
    //Parks are nearly guaranteed to have a non-zero chance of spawning anywhere in the city.
    int shop_normal = std::max( static_cast<int>( normal_roll( shop_radius, shop_sigma ) ),
                                shop_radius );
    int park_normal = std::max( static_cast<int>( normal_roll( park_radius, park_sigma ) ),
                                park_radius );

    if( shop_normal > town_dist ) {
        return settings.city_spec.pick_shop();
    } else if( park_normal > town_dist ) {
        return settings.city_spec.pick_park();
    } else {
        return settings.city_spec.pick_house();
    }
}

void overmap::place_building( const tripoint_om_omt &p, om_direction::type dir, const city &town )
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

    const pf::path<point_om_omt> street_path = lay_out_street( connection, p, dir, cs + 1 );

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
                ter_set( rp, oter_id( "road_nesw_manhole" ) );
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
                    if( ter( cand ) != "ants_queen" ) { // skip over a queen's chamber.
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
                if( is_ot_match( "lab", ter( train + offset ), ot_match_type::contains ) ) {
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
                if( is_ot_match( "lab", ter( train + offset ), ot_match_type::contains ) ) {
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
                if( is_ot_match( "lab", ter( cell + offset ), ot_match_type::contains ) ) {
                    ++adjacent_labs;
                }
            }
        } while( tries < 50 && (
                     ter( cell ) == labt_stairs ||
                     ter( cell ) == labt_finale ||
                     ter( cell + point_south ) != labt ||
                     adjacent_labs != 1 ) );
        if( tries < 50 ) {
            ter_set( cell, oter_id( "lab_escape_cells" ) );
            ter_set( cell + point_south, oter_id( "lab_escape_entrance" ) );
        }
    }

    return numstairs > 0;
}

void overmap::build_anthill( const tripoint_om_omt &p, int s, bool ordinary_ants )
{
    for( om_direction::type dir : om_direction::all ) {
        build_tunnel( p, s - rng( 0, 3 ), dir, ordinary_ants );
    }

    // TODO: This should follow the tunnel network,
    // as of now it can pick a tile from an adjacent ant network.
    std::vector<tripoint_om_omt> queenpoints;
    for( int i = -s; i <= s; i++ ) {
        for( int j = -s; j <= s; j++ ) {
            const tripoint_om_omt qp = p + point( i, j );
            if( check_ot( "ants", ot_match_type::type, qp ) ) {
                queenpoints.push_back( qp );
            }
        }
    }
    if( queenpoints.empty() ) {
        debugmsg( "No queenpoints when building anthill, anthill over %s", ter( p ).id().str() );
    }
    const tripoint_om_omt target = random_entry( queenpoints );
    ter_set( target, ordinary_ants ? oter_id( "ants_queen" ) : oter_id( "ants_queen_acid" ) );

    const oter_id root_id( "ants_isolated" );

    for( int i = -s; i <= s; i++ ) {
        for( int j = -s; j <= s; j++ ) {
            const tripoint_om_omt root = p + point( i, j );
            if( !inbounds( root ) ) {
                continue;
            }
            if( root_id == ter( root )->id ) {
                const oter_id &oter = ter( root );
                for( om_direction::type dir : om_direction::all ) {
                    const tripoint_om_omt neighbor = root + om_direction::displace( dir );
                    if( check_ot( "ants", ot_match_type::prefix, neighbor ) ) {
                        size_t line = oter->get_line();
                        line = om_lines::set_segment( line, dir );
                        if( line != oter->get_line() ) {
                            ter_set( root, oter->get_type_id()->get_linear( line ) );
                        }
                    }
                }
            }
        }
    }
}

void overmap::build_tunnel( const tripoint_om_omt &p, int s, om_direction::type dir,
                            bool ordinary_ants )
{
    if( s <= 0 ) {
        return;
    }
    if( !inbounds( p ) ) {
        return;
    }

    const oter_id root_id( "ants_isolated" );
    if( root_id != ter( p )->id ) {
        if( check_ot( "ants", ot_match_type::type, p ) ) {
            return;
        }
        if( !is_ot_match( "empty_rock", ter( p ), ot_match_type::type ) ) {
            return;
        }
    }

    ter_set( p, oter_id( root_id ) );

    std::vector<om_direction::type> valid;
    valid.reserve( om_direction::size );
    for( om_direction::type r : om_direction::all ) {
        const tripoint_om_omt cand = p + om_direction::displace( r );
        if( !check_ot( "ants", ot_match_type::type, cand ) &&
            is_ot_match( "empty_rock", ter( cand ), ot_match_type::type ) ) {
            valid.push_back( r );
        }
    }

    const oter_id ants_food( "ants_food" );
    const oter_id ants_larvae( "ants_larvae" );
    const oter_id ants_larvae_acid( "ants_larvae_acid" );
    const tripoint_om_omt next =
        s != 1 ? p + om_direction::displace( dir ) : tripoint_om_omt( -1, -1, -1 );

    for( om_direction::type r : valid ) {
        const tripoint_om_omt cand = p + om_direction::displace( r );
        if( !inbounds( cand ) ) {
            continue;
        }

        if( cand.xy() != next.xy() ) {
            if( one_in( s * 2 ) ) {
                // Spawn a special chamber
                if( one_in( 2 ) ) {
                    ter_set( cand, ants_food );
                } else {
                    ter_set( cand, ordinary_ants ? ants_larvae : ants_larvae_acid );
                }
            } else if( one_in( 5 ) ) {
                // Branch off a side tunnel
                build_tunnel( cand, s - rng( 1, 3 ), r, ordinary_ants );
            }
        }
    }
    build_tunnel( next, s - 1, dir, ordinary_ants );
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

void overmap::build_mine( const tripoint_om_omt &origin, int s )
{
    bool finale = s <= rng( 1, 3 );
    const oter_id mine( "mine" );
    const oter_id mine_finale_or_down( finale ? "mine_finale" : "mine_down" );
    const oter_id empty_rock( "empty_rock" );

    int built = 0;
    if( s < 2 ) {
        s = 2;
    }
    tripoint_om_omt p = origin;
    while( built < s ) {
        ter_set( p, mine );
        std::vector<tripoint_om_omt> next;
        for( const point &offset : four_adjacent_offsets ) {
            if( ter( p + offset ) == empty_rock ) {
                next.push_back( p + offset );
            }
        }
        if( next.empty() ) { // Dead end!  Go down!
            ter_set( p, mine_finale_or_down );
            return;
        }
        p = random_entry( next );
        built++;
    }
    ter_set( p, mine_finale_or_down );
}

void overmap::place_ravines()
{
    if( settings.overmap_ravine.num_ravines == 0 ) {
        return;
    }

    const oter_id rift( "ravine" );
    const oter_id rift_edge( "ravine_edge" );
    const oter_id rift_floor( "ravine_floor" );
    const oter_id rift_floor_edge( "ravine_floor_edge" );

    std::set<point_om_omt> rift_points;

    // We dont really care about the paths each ravine takes, so this can be whatever
    // The random return value was chosen because it easily produces decent looking windy ravines
    const auto estimate =
    [&]( const pf::node<point> &, const pf::node<point> * ) {
        return rng( 1, 2 );
    };
    // A path is generated for each of ravine, and all its constituent points are stored within the
    // rift_points set. In the code block below, the set is then used to determine edges and place the
    // actual terrain pieces of the ravine.
    for( int n = 0; n < settings.overmap_ravine.num_ravines; n++ ) {
        const point offset( rng( -settings.overmap_ravine.ravine_range,
                                 settings.overmap_ravine.ravine_range ),
                            rng( -settings.overmap_ravine.ravine_range, settings.overmap_ravine.ravine_range ) );
        const point origin( rng( 0, OMAPX ), rng( 0, OMAPY ) );
        const point destination = origin + offset;
        if( !inbounds( point_om_omt( destination.x, destination.y ),
                       settings.overmap_ravine.ravine_width * 3 ) ) {
            continue;
        }
        const auto path = pf::find_path( origin, destination, point( OMAPX, OMAPY ), estimate );
        for( const auto &node : path.nodes ) {
            const point_om_omt p( node.pos.x, node.pos.y );
            for( int i = 1 - settings.overmap_ravine.ravine_width; i < settings.overmap_ravine.ravine_width;
                 i++ ) {
                for( int j = 1 - settings.overmap_ravine.ravine_width; j < settings.overmap_ravine.ravine_width;
                     j++ ) {
                    const point_om_omt n = p + point( j, i );
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
                const point_om_omt n = p + point( ni, nj );
                if( rift_points.find( n ) == rift_points.end() || !inbounds( n ) ) {
                    edge = true;
                }
            }
        }
        for( int z = 0; z >= settings.overmap_ravine.ravine_depth; z-- ) {
            if( z == settings.overmap_ravine.ravine_depth ) {
                ter_set( tripoint_om_omt( p, z ), edge ? rift_floor_edge : rift_floor );
            } else {
                ter_set( tripoint_om_omt( p, z ), edge ? rift_edge : rift );
            }
        }
    }

}

pf::path<point_om_omt> overmap::lay_out_connection(
    const overmap_connection &connection, const point_om_omt &source, const point_om_omt &dest,
    int z, const bool must_be_unexplored ) const
{
    const auto estimate =
    [&]( const pf::node<point_om_omt> &cur, const pf::node<point_om_omt> *prev ) {
        const auto &id( ter( tripoint_om_omt( cur.pos, z ) ) );

        const overmap_connection::subtype *subtype = connection.pick_subtype_for( id );

        if( !subtype ) {
            return pf::rejected;  // No option for this terrain.
        }

        const bool existing_connection = connection.has( id );

        // Only do this check if it needs to be unexplored and there isn't already a connection.
        if( must_be_unexplored && !existing_connection ) {
            // If this must be unexplored, check if we've already got a submap generated.
            const bool existing_submap = is_omt_generated( tripoint_om_omt( cur.pos, z ) );

            // If there is an existing submap, this area has already been explored and this
            // isn't a valid placement.
            if( existing_submap ) {
                return pf::rejected;
            }
        }

        if( existing_connection && id->is_rotatable() &&
            !om_direction::are_parallel( id->get_dir(), static_cast<om_direction::type>( cur.dir ) ) ) {
            return pf::rejected; // Can't intersect.
        }

        if( prev && prev->dir != cur.dir ) { // Direction has changed.
            const oter_id &prev_id = ter( tripoint_om_omt( prev->pos, z ) );
            const overmap_connection::subtype *prev_subtype = connection.pick_subtype_for( prev_id );

            if( !prev_subtype || !prev_subtype->allows_turns() ) {
                return pf::rejected;
            }
        }

        const int dist = subtype->is_orthogonal() ?
                         manhattan_dist( dest, cur.pos ) :
                         trig_dist( dest, cur.pos );
        const int existency_mult = existing_connection ? 1 : 5; // Prefer existing connections.

        return existency_mult * dist + subtype->basic_cost;
    };

    return pf::find_path( source, dest, point_om_omt( OMAPX, OMAPY ), estimate );
}

pf::path<point_om_omt> overmap::lay_out_street(
    const overmap_connection &connection, const point_om_omt &source, om_direction::type dir,
    size_t len ) const
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

        if( ter_id->is_river() || !connection.pick_subtype_for( ter_id ) ) {
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
                    if( is_ot_match( "road", ter( checkp ), ot_match_type::type ) ) {
                        collisions++;
                    }
                }
            }

            //Stop roads from running right next to eachother
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

    return pf::straight_path( source, static_cast<int>( dir ), actual_len );
}

void overmap::build_connection(
    const overmap_connection &connection, const pf::path<point_om_omt> &path, int z,
    const om_direction::type &initial_dir )
{
    if( path.nodes.empty() ) {
        return;
    }

    om_direction::type prev_dir = initial_dir;

    const pf::node<point_om_omt> start = path.nodes.front();
    const pf::node<point_om_omt> end = path.nodes.back();

    for( const auto &node : path.nodes ) {
        const tripoint_om_omt pos( node.pos, z );
        const oter_id &ter_id = ter( pos );
        // TODO: Make 'node' support 'om_direction'.
        const om_direction::type new_dir( static_cast<om_direction::type>( node.dir ) );
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

void overmap::build_connection( const point_om_omt &source, const point_om_omt &dest, int z,
                                const overmap_connection &connection, const bool must_be_unexplored,
                                const om_direction::type &initial_dir )
{
    build_connection(
        connection, lay_out_connection( connection, source, dest, z, must_be_unexplored ),
        z, initial_dir );
}

void overmap::connect_closest_points( const std::vector<point_om_omt> &points, int z,
                                      const overmap_connection &connection )
{
    if( points.size() == 1 ) {
        return;
    }
    for( size_t i = 0; i < points.size(); ++i ) {
        int closest = -1;
        int k = 0;
        for( size_t j = i + 1; j < points.size(); j++ ) {
            const int distance = trig_dist( points[i], points[j] );
            if( distance < closest || closest < 0 ) {
                closest = distance;
                k = j;
            }
        }
        if( closest > 0 ) {
            build_connection( points[i], points[k], z, connection, false );
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

void overmap::good_river( const tripoint_om_omt &p )
{
    if( !is_ot_match( "river", ter( p ), ot_match_type::prefix ) ) {
        return;
    }
    if( ( p.x() == 0 ) || ( p.x() == OMAPX - 1 ) ) {
        if( !is_river_or_lake( ter( p + point_north ) ) ) {
            ter_set( p, oter_id( "river_north" ) );
        } else if( !is_river_or_lake( ter( p + point_south ) ) ) {
            ter_set( p, oter_id( "river_south" ) );
        } else {
            ter_set( p, oter_id( "river_center" ) );
        }
        return;
    }
    if( ( p.y() == 0 ) || ( p.y() == OMAPY - 1 ) ) {
        if( !is_river_or_lake( ter( p + point_west ) ) ) {
            ter_set( p, oter_id( "river_west" ) );
        } else if( !is_river_or_lake( ter( p + point_east ) ) ) {
            ter_set( p, oter_id( "river_east" ) );
        } else {
            ter_set( p, oter_id( "river_center" ) );
        }
        return;
    }
    if( is_river_or_lake( ter( p + point_west ) ) ) {
        if( is_river_or_lake( ter( p + point_north ) ) ) {
            if( is_river_or_lake( ter( p + point_south ) ) ) {
                if( is_river_or_lake( ter( p + point_east ) ) ) {
                    // River on N, S, E, W;
                    // but we might need to take a "bite" out of the corner
                    if( !is_river_or_lake( ter( p + point_north_west ) ) ) {
                        ter_set( p, oter_id( "river_c_not_nw" ) );
                    } else if( !is_river_or_lake( ter( p + point_north_east ) ) ) {
                        ter_set( p, oter_id( "river_c_not_ne" ) );
                    } else if( !is_river_or_lake( ter( p + point_south_west ) ) ) {
                        ter_set( p, oter_id( "river_c_not_sw" ) );
                    } else if( !is_river_or_lake( ter( p + point_south_east ) ) ) {
                        ter_set( p, oter_id( "river_c_not_se" ) );
                    } else {
                        ter_set( p, oter_id( "river_center" ) );
                    }
                } else {
                    ter_set( p, oter_id( "river_east" ) );
                }
            } else {
                if( is_river_or_lake( ter( p + point_east ) ) ) {
                    ter_set( p, oter_id( "river_south" ) );
                } else {
                    ter_set( p, oter_id( "river_se" ) );
                }
            }
        } else {
            if( is_river_or_lake( ter( p + point_south ) ) ) {
                if( is_river_or_lake( ter( p + point_east ) ) ) {
                    ter_set( p, oter_id( "river_north" ) );
                } else {
                    ter_set( p, oter_id( "river_ne" ) );
                }
            } else {
                if( is_river_or_lake( ter( p + point_east ) ) ) { // Means it's swampy
                    ter_set( p, oter_id( "forest_water" ) );
                }
            }
        }
    } else {
        if( is_river_or_lake( ter( p + point_north ) ) ) {
            if( is_river_or_lake( ter( p + point_south ) ) ) {
                if( is_river_or_lake( ter( p + point_east ) ) ) {
                    ter_set( p, oter_id( "river_west" ) );
                } else { // Should never happen
                    ter_set( p, oter_id( "forest_water" ) );
                }
            } else {
                if( is_river_or_lake( ter( p + point_east ) ) ) {
                    ter_set( p, oter_id( "river_sw" ) );
                } else { // Should never happen
                    ter_set( p, oter_id( "forest_water" ) );
                }
            }
        } else {
            if( is_river_or_lake( ter( p + point_south ) ) ) {
                if( is_river_or_lake( ter( p + point_east ) ) ) {
                    ter_set( p, oter_id( "river_nw" ) );
                } else { // Should never happen
                    ter_set( p, oter_id( "forest_water" ) );
                }
            } else { // Should never happen
                ter_set( p, oter_id( "forest_water" ) );
            }
        }
    }
}

const std::string &om_direction::id( type dir )
{
    static const std::array < std::string, size + 1 > ids = {{
            "", "north", "east", "south", "west"
        }
    };
    if( dir == om_direction::type::invalid ) {
        debugmsg( "Invalid direction cannot have an id." );
    }
    return ids[static_cast<size_t>( dir ) + 1];
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

static inline om_direction::type rotate_internal( om_direction::type dir, int step )
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

om_direction::type overmap::random_special_rotation( const overmap_special &special,
        const tripoint_om_omt &p, const bool must_be_unexplored ) const
{
    std::vector<om_direction::type> rotations( om_direction::size );
    const auto first = rotations.begin();
    auto last = first;

    int top_score = 0; // Maximal number of existing connections (roads).
    // Try to find the most suitable rotation: satisfy as many connections as possible with the existing terrain.
    for( om_direction::type r : om_direction::all ) {
        int score = 0; // Number of existing connections when rotated by 'r'.
        bool valid = true;

        for( const auto &con : special.connections ) {
            const tripoint_om_omt rp = p + om_direction::rotate( con.p, r );
            if( !inbounds( rp ) ) {
                valid = false;
                break;
            }
            const oter_id &oter = ter( rp );

            if( is_ot_match( con.terrain.str(), oter, ot_match_type::type ) ) {
                ++score; // Found another one satisfied connection.
            } else if( !oter || con.existing || !con.connection->pick_subtype_for( oter ) ) {
                valid = false;
                break;
            }
        }

        if( valid && score >= top_score ) {
            if( score > top_score ) {
                top_score = score;
                last = first; // New top score. Forget previous rotations.
            }
            *last = r;
            ++last;
        }

        if( !special.rotatable ) {
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

    return std::all_of( special.terrains.begin(),
    special.terrains.end(), [&]( const overmap_special_terrain & elem ) {
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
void overmap::place_special(
    const overmap_special &special, const tripoint_om_omt &p, om_direction::type dir,
    const city &cit, const bool must_be_unexplored, const bool force )
{
    cata_assert( dir != om_direction::type::invalid );
    if( !force ) {
        cata_assert( can_place_special( special, p, dir, must_be_unexplored ) );
    }

    const bool blob = special.flags.count( "BLOB" ) > 0;

    for( const auto &elem : special.terrains ) {
        const tripoint_om_omt location = p + om_direction::rotate( elem.p, dir );
        const oter_id tid = elem.terrain->get_rotated( dir );

        overmap_special_placements[location] = special.id;
        ter_set( location, tid );

        if( blob ) {
            for( int x = -2; x <= 2; x++ ) {
                for( int y = -2; y <= 2; y++ ) {
                    const tripoint_om_omt nearby_pos = location + point( x, y );
                    if( !inbounds( nearby_pos ) ) {
                        continue;
                    }
                    if( one_in( 1 + std::abs( x ) + std::abs( y ) ) && elem.can_be_placed_on( ter( nearby_pos ) ) ) {
                        ter_set( nearby_pos, tid );
                    }
                }
            }
        }
    }
    // Make connections.
    if( cit ) {
        for( const auto &elem : special.connections ) {
            if( elem.connection ) {
                const tripoint_om_omt rp = p + om_direction::rotate( elem.p, dir );
                om_direction::type initial_dir = elem.initial_dir;

                if( initial_dir != om_direction::type::invalid ) {
                    initial_dir = om_direction::add( initial_dir, dir );
                }

                build_connection( cit.pos, rp.xy(), elem.p.z, *elem.connection, must_be_unexplored,
                                  initial_dir );
            }
        }
    }
    // Place spawns.
    if( special.spawns.group ) {
        const overmap_special_spawns &spawns = special.spawns;
        const int pop = rng( spawns.population.min, spawns.population.max );
        const int rad = rng( spawns.radius.min, spawns.radius.max );
        add_mon_group( mongroup( spawns.group, project_to<coords::sm>( p ), rad, pop ) );
    }
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
    for( auto iter = enabled_specials.begin(); iter != enabled_specials.end(); ++iter ) {
        const auto &special = *iter->special_details;
        // If we haven't finished placing minimum instances of all specials,
        // skip specials that are at their minimum count already.
        if( !place_optional && iter->instances_placed >= special.occurrences.min ) {
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

        if( ++iter->instances_placed >= special.occurrences.max ) {
            enabled_specials.erase( iter );
        }

        return true;
    }

    return false;
}

void overmap::place_specials_pass( overmap_special_batch &enabled_specials,
                                   om_special_sectors &sectors, const bool place_optional, const bool must_be_unexplored )
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
    for( int z = -OVERMAP_DEPTH; z <= OVERMAP_HEIGHT && !overmap_has_lake; z++ ) {
        for( int x = 0; x < OMAPX && !overmap_has_lake; x++ ) {
            for( int y = 0; y < OMAPY && !overmap_has_lake; y++ ) {
                overmap_has_lake = ter( { x, y, z } )->is_lake();
            }
        }
    }

    for( auto iter = enabled_specials.begin(); iter != enabled_specials.end(); ) {
        // If this special has the LAKE flag and the overmap doesn't have any
        // lake terrain, then remove this special from the candidates for this
        // overmap.
        if( iter->special_details->flags.count( "LAKE" ) > 0 && !overmap_has_lake ) {
            iter = enabled_specials.erase( iter );
            continue;
        }

        if( iter->special_details->flags.count( "UNIQUE" ) > 0 ) {
            const int min = iter->special_details->occurrences.min;
            const int max = iter->special_details->occurrences.max;

            if( x_in_y( min, max ) ) {
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
    overmap_special_batch custom_overmap_specials = overmap_special_batch( enabled_specials );

    // Check for any unplaced mandatory specials, and if there are any, attempt to
    // place them on adjacent uncreated overmaps.
    if( std::any_of( custom_overmap_specials.begin(), custom_overmap_specials.end(),
    []( overmap_special_placement placement ) {
    return placement.instances_placed <
           placement.special_details->occurrences.min;
} ) ) {
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
            std::shuffle( nearest_candidates.begin(), nearest_candidates.end(), rng_get_engine() );
            point_abs_om new_om_addr = nearest_candidates.front();
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
    for( auto &elem : custom_overmap_specials ) {
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
            if( it->instances_placed >= it->special_details->occurrences.max ) {
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
    // Cities are full of zombies
    for( city &elem : cities ) {
        if( get_option<bool>( "WANDER_SPAWNS" ) ) {
            if( !one_in( 16 ) || elem.size > 5 ) {
                mongroup m( GROUP_ZOMBIE,
                            tripoint_om_sm( project_to<coords::sm>( elem.pos ), 0 ),
                            static_cast<int>( elem.size * 2.5 ),
                            elem.size * 80 );
                //                m.set_target( zg.back().posx, zg.back().posy );
                m.horde = true;
                m.wander( *this );
                add_mon_group( m );
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
                        if( ter( { sx, sy, 0 } ) == "forest_water" ) {
                            swamp_count += 2;
                        }
                    }
                }
                if( swamp_count >= 25 ) {
                    add_mon_group( mongroup( GROUP_SWAMP, tripoint( x * 2, y * 2, 0 ), 3,
                                             rng( swamp_count * 8, swamp_count * 25 ) ) );
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
                    if( is_river_or_lake( ter( { sx, sy, 0 } ) ) ) {
                        river_count++;
                    }
                }
            }
            if( river_count >= 25 ) {
                add_mon_group( mongroup( GROUP_RIVER, tripoint( x * 2, y * 2, 0 ), 3,
                                         rng( river_count * 8, river_count * 25 ) ) );
            }
        }
    }

    // Place the "put me anywhere" groups
    int numgroups = rng( 0, 3 );
    for( int i = 0; i < numgroups; i++ ) {
        add_mon_group( mongroup( GROUP_WORM, tripoint( rng( 0, OMAPX * 2 - 1 ), rng( 0,
                                 OMAPY * 2 - 1 ), 0 ),
                                 rng( 20, 40 ), rng( 30, 50 ) ) );
    }
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
                    radios.push_back( radio_tower( pos_sm, strength(), "", radio_type::WEATHER_RADIO ) );
                } else {
                    message = SNIPPET.expand( SNIPPET.random_from_category( "radio_archive" ).value_or(
                                                  translation() ).translated() );
                    radios.push_back( radio_tower( pos_sm, strength(), message ) );
                }
            } else if( is_ot_match( "lmoe", ter( pos_omt ), ot_match_type::prefix ) ) {
                message = string_format( _( "This is automated emergency shelter beacon %d%d."
                                            "  Supplies, amenities and shelter are stocked." ), i, j );
                radios.push_back( radio_tower( pos_sm, strength() / 2, message ) );
            } else if( is_ot_match( "fema_entrance", ter( pos_omt ), ot_match_type::prefix ) ) {
                message = string_format( _( "This is FEMA camp %d%d."
                                            "  Supplies are limited, please bring supplemental food, water, and bedding."
                                            "  This is FEMA camp %d%d.  A designated long-term emergency shelter." ), i, j, i, j );
                radios.push_back( radio_tower( pos_sm, strength(), message ) );
            }
        }
    }
}

void overmap::open( overmap_special_batch &enabled_specials )
{
    const std::string terfilename = overmapbuffer::terrain_filename( loc );

    using namespace std::placeholders;
    if( read_from_file_optional( terfilename, std::bind( &overmap::unserialize, this, _1 ) ) ) {
        const std::string plrfilename = overmapbuffer::player_filename( loc );
        read_from_file_optional( plrfilename, std::bind( &overmap::unserialize_view, this, _1 ) );
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

void overmap::add_mon_group( const mongroup &group )
{
    // Monster groups: the old system had large groups (radius > 1),
    // the new system transforms them into groups of radius 1, this also
    // makes the diffuse setting obsolete (as it only controls how the radius
    // is interpreted) - it's only used when adding monster groups with function.
    if( group.radius == 1 ) {
        zg.insert( std::pair<tripoint_om_sm, mongroup>( group.pos, group ) );
        return;
    }
    // diffuse groups use a circular area, non-diffuse groups use a rectangular area
    const int rad = std::max<int>( 0, group.radius );
    const double total_area = group.diffuse ? std::pow( rad + 1, 2 ) : ( rad * rad * M_PI + 1 );
    const double pop = std::max<int>( 0, group.population );
    int xpop = 0;
    for( int x = -rad; x <= rad; x++ ) {
        for( int y = -rad; y <= rad; y++ ) {
            const int dist = group.diffuse ? square_dist( point( x, y ), point_zero ) : trig_dist( point( x,
                             y ), point_zero );
            if( dist > rad ) {
                continue;
            }
            // Population on a single submap, *not* a integer
            double pop_here;
            if( rad == 0 ) {
                pop_here = pop;
            } else if( group.diffuse ) {
                pop_here = pop / total_area;
            } else {
                // non-diffuse groups are more dense towards the center.
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
            tmp.radius = 1;
            tmp.pos += point( x, y );
            tmp.population = p;
            // This *can* create groups outside of the area of this overmap.
            // As this function is called during generating the overmap, the
            // neighboring overmaps might not have been generated and one can't access
            // them through the overmapbuffer as this would trigger generating them.
            // This would in turn to lead to a call to this function again.
            // To avoid this, the overmapbuffer checks the monster groups when loading
            // an overmap and moves groups with out-of-bounds position to another overmap.
            add_mon_group( tmp );
            xpop += tmp.population;
        }
    }
}

void overmap::for_each_npc( const std::function<void( npc & )> &callback )
{
    for( auto &guy : npcs ) {
        callback( *guy );
    }
}

void overmap::for_each_npc( const std::function<void( const npc & )> &callback ) const
{
    for( const auto &guy : npcs ) {
        callback( *guy );
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

cata::optional<basecamp *> overmap::find_camp( const point_abs_omt &p )
{
    for( auto &v : camps ) {
        if( v.camp_omt_pos().xy() == p ) {
            return &v;
        }
    }
    return cata::nullopt;
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

    // TODO: fix point types
    const bool is_generated = MAPBUFFER.lookup_submap( global_sm_loc.raw() ) != nullptr;

    return is_generated;
}

overmap_special_id overmap_specials::create_building_from( const string_id<oter_type_t> &base )
{
    // TODO: Get rid of the hard-coded ids.
    static const string_id<overmap_location> land( "land" );
    static const string_id<overmap_location> swamp( "swamp" );

    overmap_special new_special;

    new_special.id = overmap_special_id( "FakeSpecial_" + base.str() );

    overmap_special_terrain ter;
    ter.terrain = base.obj().get_first().id();
    ter.locations.insert( land );
    ter.locations.insert( swamp );
    new_special.terrains.push_back( ter );

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
        case ot_match_type::prefix: return "PREFIX";
        case ot_match_type::contains: return "CONTAINS";
        // *INDENT-ON*
        case ot_match_type::num_ot_match_type:
            break;
    }
    debugmsg( "Invalid ot_match_type" );
    abort();
}
} // namespace io

constexpr tripoint_abs_omt overmap::invalid_tripoint;

std::string oter_no_dir( const oter_id &oter )
{
    std::string base_oter_id = oter.id().c_str();
    size_t oter_len = base_oter_id.size();
    if( oter_len > 7 ) {
        if( base_oter_id.substr( oter_len - 6, 6 ) == "_south" ) {
            return base_oter_id.substr( 0, oter_len - 6 );
        } else if( base_oter_id.substr( oter_len - 6, 6 ) == "_north" ) {
            return base_oter_id.substr( 0, oter_len - 6 );
        }
    }
    if( oter_len > 6 ) {
        if( base_oter_id.substr( oter_len - 5, 5 ) == "_west" ) {
            return base_oter_id.substr( 0, oter_len - 5 );
        } else if( base_oter_id.substr( oter_len - 5, 5 ) == "_east" ) {
            return base_oter_id.substr( 0, oter_len - 5 );
        }
    }
    return base_oter_id;
}

int oter_get_rotation( const oter_id &oter )
{
    std::string base_oter_id = oter.id().c_str();
    size_t oter_len = base_oter_id.size();
    if( oter_len > 7 ) {
        if( base_oter_id.substr( oter_len - 6, 6 ) == "_south" ) {
            return 2;
        } else if( base_oter_id.substr( oter_len - 6, 6 ) == "_north" ) {
            return 0;
        }
    }
    if( oter_len > 6 ) {
        if( base_oter_id.substr( oter_len - 5, 5 ) == "_west" ) {
            return 1;
        } else if( base_oter_id.substr( oter_len - 5, 5 ) == "_east" ) {
            return 3;
        }
    }
    return 0;
}

std::string oter_get_rotation_string( const oter_id &oter )
{
    std::string base_oter_id = oter.id().c_str();
    size_t oter_len = base_oter_id.size();
    if( oter_len > 7 ) {
        if( base_oter_id.substr( oter_len - 6, 6 ) == "_south" ) {
            return "_south";
        } else if( base_oter_id.substr( oter_len - 6, 6 ) == "_north" ) {
            return "_north";
        }
    }
    if( oter_len > 6 ) {
        if( base_oter_id.substr( oter_len - 5, 5 ) == "_west" ) {
            return "_west";
        } else if( base_oter_id.substr( oter_len - 5, 5 ) == "_east" ) {
            return "_east";
        }
    }
    return "";
}
