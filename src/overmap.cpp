#include "omdata.h" // IWYU pragma: associated
#include "overmap.h" // IWYU pragma: associated

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <numeric>
#include <ostream>
#include <queue>
#include <random>
#include <vector>

#include "cata_utility.h"
#include "coordinate_conversions.h"
#include "debug.h"
#include "game.h"
#include "generic_factory.h"
#include "json.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapbuffer.h"
#include "mapgen.h"
#include "mapgen_functions.h"
#include "messages.h"
#include "mongroup.h"
#include "mtype.h"
#include "name.h"
#include "npc.h"
#include "options.h"
#include "output.h"
#include "overmap_connection.h"
#include "overmap_location.h"
#include "overmap_types.h"
#include "overmapbuffer.h"
#include "regional_settings.h"
#include "rng.h"
#include "rotatable_symbols.h"
#include "simple_pathfinding.h"
#include "translations.h"

#define dbg(x) DebugLog((DebugLevel)(x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

#define BUILDINGCHANCE 4
#define MIN_ANT_SIZE 8
#define MAX_ANT_SIZE 20
#define MIN_GOO_SIZE 1
#define MAX_GOO_SIZE 2
#define MIN_RIFT_SIZE 6
#define MAX_RIFT_SIZE 16

const efftype_id effect_pet( "pet" );

using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

////////////////
oter_id  ot_null,
         ot_crater,
         ot_field,
         ot_forest,
         ot_forest_thick,
         ot_forest_water,
         ot_river_center;

const oter_type_t oter_type_t::null_type;

namespace om_lines
{

struct type {
    long sym;
    size_t mapgen;
    std::string suffix;
};

const std::array<std::string, 5> mapgen_suffixes = {{
        "_straight", "_curved", "_end", "_tee", "_four_way"
    }
};

const std::array < type, 1 + om_direction::bits > all = {{
        { LINE_XXXX, 4, "_isolated"  },   // 0  ----
        { LINE_XOXO, 2, "_end_south" },   // 1  ---n
        { LINE_OXOX, 2, "_end_west"  },   // 2  --e-
        { LINE_XXOO, 1, "_ne"        },   // 3  --en
        { LINE_XOXO, 2, "_end_north" },   // 4  -s--
        { LINE_XOXO, 0, "_ns"        },   // 5  -s-n
        { LINE_OXXO, 1, "_es"        },   // 6  -se-
        { LINE_XXXO, 3, "_nes"       },   // 7  -sen
        { LINE_OXOX, 2, "_end_east"  },   // 8  w---
        { LINE_XOOX, 1, "_wn"        },   // 9  w--n
        { LINE_OXOX, 0, "_ew"        },   // 10 w-e-
        { LINE_XXOX, 3, "_new"       },   // 11 w-en
        { LINE_OOXX, 1, "_sw"        },   // 12 ws--
        { LINE_XOXX, 3, "_nsw"       },   // 13 ws-n
        { LINE_OXXX, 3, "_esw"       },   // 14 wse-
        { LINE_XXXX, 4, "_nesw"      }    // 15 wsen
    }
};

const size_t size = all.size();
const size_t invalid = 0;

constexpr size_t rotate( size_t line, om_direction::type dir )
{
    // Bitwise rotation to the left.
    return ( ( ( line << static_cast<size_t>( dir ) ) |
               ( line >> ( om_direction::size - static_cast<size_t>( dir ) ) ) ) & om_direction::bits );
}

constexpr size_t set_segment( size_t line, om_direction::type dir )
{
    return line | 1 << static_cast<int>( dir );
}

constexpr bool has_segment( size_t line, om_direction::type dir )
{
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

size_t from_dir( om_direction::type dir )
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

}

//const regional_settings default_region_settings;
t_regional_settings_map region_settings_map;

namespace
{

generic_factory<overmap_land_use_code> land_use_codes( "overmap land use codes" );
generic_factory<oter_type_t> terrain_types( "overmap terrain type" );
generic_factory<oter_t> terrains( "overmap terrain" );
generic_factory<overmap_special> specials( "overmap special" );

}

static const std::map<std::string, oter_flags> oter_flags_map = {
    { "KNOWN_DOWN",     known_down     },
    { "KNOWN_UP",       known_up       },
    { "RIVER",          river_tile     },
    { "SIDEWALK",       has_sidewalk   },
    { "NO_ROTATE",      no_rotate      },
    { "LINEAR",         line_drawing   },
    { "SUBWAY",         subway_connection }
};

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

city::city( const point &P, int const S )
    : pos( P )
    , size( S )
    , name( Name::get( nameIsTownName ) )
{
}

int city::get_distance_from( const tripoint &p ) const
{
    return std::max( int( trig_dist( p, { pos.x, pos.y, 0 } ) ) - size, 0 );
}

std::map<enum radio_type, std::string> radio_type_names =
{{ {MESSAGE_BROADCAST, "broadcast"}, {WEATHER_RADIO, "weather"} }};

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
inline bool int_id<oter_t>::is_valid() const
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
    return lhs.id().str().compare( rhs ) == 0;
}

bool operator!=( const int_id<oter_t> &lhs, const char *rhs )
{
    return !( lhs == rhs );
}

void set_oter_ids()   // @todo: fixme constify
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

void overmap_land_use_code::load( JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";
    assign( jo, "land_use_code", land_use_code, strict );
    assign( jo, "name", name, strict );
    assign( jo, "detailed_definition", detailed_definition, strict );
    assign( jo, "sym", sym, strict );
    assign( jo, "color", color, strict );
}

void overmap_land_use_code::finalize()
{

}

void overmap_land_use_code::check() const
{

}

void overmap_land_use_codes::load( JsonObject &jo, const std::string &src )
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

void overmap_specials::load( JsonObject &jo, const std::string &src )
{
    specials.load( jo, src );
}

void city_buildings::load( JsonObject &jo, const std::string &src )
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
    const size_t max_count = ( OMAPX / OMSPEC_FREQ ) * ( OMAPY / OMSPEC_FREQ ) / 2;
    const size_t actual_count = std::accumulate( specials.get_all().begin(), specials.get_all().end(),
                                static_cast< size_t >( 0 ),
    []( size_t sum, const overmap_special & elem ) {
        return sum + ( elem.flags.count( "UNIQUE" ) == static_cast<size_t>( 0 ) ? static_cast<size_t>
                       ( std::max(
                             elem.occurrences.min, 0 ) ) : static_cast<size_t>( 1 ) );
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

overmap_special_batch overmap_specials::get_default_batch( const point &origin )
{
    const bool only_classic = get_option<bool>( "CLASSIC_ZOMBIES" );
    const int city_size = get_option<int>( "CITY_SIZE" );
    std::vector<const overmap_special *> res;

    res.reserve( specials.size() );
    for( const overmap_special &elem : specials.get_all() ) {
        if( elem.locations.empty() || elem.occurrences.empty() ) {
            continue;
        }

        if( only_classic && elem.flags.count( "CLASSIC" ) == 0 ) {
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

bool is_ot_type( const std::string &otype, const oter_id &oter )
{
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
}

bool is_ot_subtype( const char *otype, const oter_id &oter )
{
    // Checks for any partial match.
    return strstr( oter.id().c_str(), otype );
}

/*
 * load mapgen functions from an overmap_terrain json entry
 * suffix is for roads/subways/etc which have "_straight", "_curved", "_tee", "_four_way" function mappings
 */
void load_overmap_terrain_mapgens( JsonObject &jo, const std::string &id_base,
                                   const std::string &suffix = "" )
{
    const std::string fmapkey( id_base + suffix );
    const std::string jsonkey( "mapgen" + suffix );
    bool default_mapgen = jo.get_bool( "default_mapgen", true );
    int default_idx = -1;
    if( default_mapgen ) {
        if( const auto ptr = get_mapgen_cfunction( fmapkey ) ) {
            oter_mapgen[fmapkey].push_back( std::make_shared<mapgen_function_builtin>( ptr ) );
            default_idx = oter_mapgen[fmapkey].size() - 1;
        }
    }
    if( jo.has_array( jsonkey ) ) {
        JsonArray ja = jo.get_array( jsonkey );
        int c = 0;
        while( ja.has_more() ) {
            if( ja.has_object( c ) ) {
                JsonObject jio = ja.next_object();
                load_mapgen_function( jio, fmapkey, default_idx );
            }
            c++;
        }
    }
}

void oter_type_t::load( JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    assign( jo, "sym", sym, strict );
    assign( jo, "name", name, strict );
    assign( jo, "see_cost", see_cost, strict );
    assign( jo, "extras", extras, strict );
    assign( jo, "mondensity", mondensity, strict );
    assign( jo, "spawns", static_spawns, strict );
    assign( jo, "color", color, strict );
    assign( jo, "land_use_code", land_use_code, strict );

    const typed_flag_reader<decltype( oter_flags_map )> flag_reader{ oter_flags_map, "invalid overmap terrain flag" };
    optional( jo, was_loaded, "flags", flags, flag_reader );

    if( has_flag( line_drawing ) ) {
        if( has_flag( no_rotate ) ) {
            jo.throw_error( "Mutually exclusive flags: \"NO_ROTATE\" and \"LINEAR\"." );
        }

        for( const auto &elem : om_lines::mapgen_suffixes ) {
            load_overmap_terrain_mapgens( jo, id.str(), elem );
        }
    } else {
        load_overmap_terrain_mapgens( jo, id.str() );
    }
}

void oter_type_t::finalize()
{
    directional_peers.clear();  // In case of a second finalization.

    if( is_rotatable() ) {
        for( auto dir : om_direction::all ) {
            register_terrain( oter_t( *this, dir ), static_cast<size_t>( dir ), om_direction::size );
        }
    } else if( has_flag( line_drawing ) ) {
        for( size_t i = 0; i < om_lines::size; ++i ) {
            register_terrain( oter_t( *this, i ), i, om_lines::size );
        }
    } else {
        register_terrain( oter_t( *this ), 0, 1 );
    }
}

void oter_type_t::register_terrain( const oter_t &peer, size_t n, size_t max_n )
{
    assert( n < max_n );
    assert( peer.type_is( *this ) );

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
    assert( !directional_peers.empty() );
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
    assert( directional_peers.size() == om_direction::size );
    return directional_peers[static_cast<size_t>( dir )];
}

oter_id oter_type_t::get_linear( size_t n ) const
{
    if( !has_flag( line_drawing ) ) {
        debugmsg( "Overmap terrain \"%s \" isn't drawn with lines.", id.c_str() );
        return ot_null;
    }
    if( n >= om_lines::size ) {
        debugmsg( "Invalid overmap line (%d) was asked from overmap terrain \"%s\".", n, id.c_str() );
        return ot_null;
    }
    assert( directional_peers.size() == om_lines::size );
    return directional_peers[n];
}

oter_t::oter_t() : oter_t( oter_type_t::null_type ) {}

oter_t::oter_t( const oter_type_t &type ) :
    type( &type ),
    id( type.id.str() ),
    sym( type.sym ),
    sym_alt( type.land_use_code ? type.land_use_code->sym : sym ) {}

oter_t::oter_t( const oter_type_t &type, om_direction::type dir ) :
    type( &type ),
    id( type.id.str() + "_" + om_direction::id( dir ) ),
    dir( dir ),
    sym( om_direction::rotate_symbol( type.sym, dir ) ),
    sym_alt( om_direction::rotate_symbol( type.land_use_code ? type.land_use_code->sym : type.sym,
                                          dir ) ),
    line( om_lines::from_dir( dir ) ) {}

oter_t::oter_t( const oter_type_t &type, size_t line ) :
    type( &type ),
    id( type.id.str() + om_lines::all[line].suffix ),
    sym( om_lines::all[line].sym ),
    sym_alt( om_lines::all[line].sym ),
    line( line ) {}

std::string oter_t::get_mapgen_id() const
{
    return type->has_flag( line_drawing )
           ? type->id.str() + om_lines::mapgen_suffixes[om_lines::all[line].mapgen]
           : type->id.str();
}

oter_id oter_t::get_rotated( om_direction::type dir ) const
{
    return type->has_flag( line_drawing )
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
    // @todo: It's a DAMN UGLY hack. Remove it as soon as possible.
    static const oter_str_id road_manhole( "road_nesw_manhole" );
    if( id == road_manhole ) {
        return true;
    }
    return om_lines::has_segment( line, dir );
}

bool oter_t::is_hardcoded() const
{
    // @todo: This set only exists because so does the monstrous 'if-else' statement in @ref map::draw_map(). Get rid of both.
    static const std::set<std::string> hardcoded_mapgen = {
        "acid_anthill",
        "anthill",
        "ants_lab",
        "ants_lab_stairs",
        "fema",
        "fema_entrance",
        "haz_sar",
        "haz_sar_b1",
        "haz_sar_entrance",
        "haz_sar_entrance_b1",
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
        "megastore",
        "megastore_entrance",
        "mine",
        "mine_down",
        "mine_entrance",
        "mine_finale",
        "mine_shaft",
        "office_tower_1",
        "office_tower_1_entrance",
        "office_tower_b",
        "office_tower_b_entrance",
        "outpost",
        "sewage_treatment",
        "sewage_treatment_hub",
        "sewage_treatment_under",
        "silo",
        "silo_finale",
        "slimepit",
        "slimepit_down",
        "spider_pit_under",
        "spiral",
        "spiral_hub",
        "temple",
        "temple_finale",
        "temple_stairs",
        "toxic_dump",
        "triffid_finale",
        "triffid_roots"
    };

    return hardcoded_mapgen.find( get_mapgen_id() ) != hardcoded_mapgen.end();
}

void overmap_terrains::load( JsonObject &jo, const std::string &src )
{
    terrain_types.load( jo, src );
}

void overmap_terrains::check_consistency()
{
    for( const auto &elem : terrain_types.get_all() ) {
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
        const bool exists_loaded = oter_mapgen.find( mid ) != oter_mapgen.end();

        if( exists_loaded ) {
            if( test_mode && exists_hardcoded ) {
                debugmsg( "Mapgen terrain \"%s\" exists in both JSON and a hardcoded function. Consider removing the latter.",
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
        debugmsg( "ERROR: can't find default overmap settings (region_map_settings 'default'),"
                  " cataclysm pending. And not the fun kind." );
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

const overmap_special_terrain &overmap_special::get_terrain_at( const tripoint &p ) const
{
    const auto iter = std::find_if( terrains.begin(),
    terrains.end(), [ &p ]( const overmap_special_terrain & elem ) {
        return elem.p == p;
    } );
    if( iter == terrains.end() ) {
        static const overmap_special_terrain null_terrain;
        return null_terrain;
    }
    return *iter;
}

bool overmap_special::can_be_placed_on( const oter_id &oter ) const
{
    return std::any_of( locations.begin(), locations.end(),
    [ &oter ]( const string_id<overmap_location> &loc ) {
        return loc->test( oter );
    } );
}

bool overmap_special::requires_city() const
{
    return city_size.min > 0 || city_distance.max < std::max( OMAPX, OMAPY );
}

bool overmap_special::can_belong_to_city( const tripoint &p, const city &cit ) const
{
    if( !requires_city() ) {
        return true;
    }
    if( !cit || !city_size.contains( cit.size ) ) {
        return false;
    }
    return city_distance.contains( cit.get_distance_from( p ) );
}

void overmap_special::load( JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";
    // city_building is just an alias of overmap_special
    // @todo: This comparison is a hack. Separate them properly.
    const bool is_special = jo.get_string( "type", "" ) == "overmap_special";

    mandatory( jo, was_loaded, "overmaps", terrains );
    mandatory( jo, was_loaded, "locations", locations );

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
    }
}

void overmap_special::check() const
{
    std::set<int> invalid_terrains;
    std::set<tripoint> points;

    for( const auto &element : locations ) {
        if( !element.is_valid() ) {
            debugmsg( "In overmap special \"%s\", location \"%s\" is invalid.",
                      id.c_str(), element.c_str() );
        }
    }

    for( const auto &elem : terrains ) {
        const auto &oter = elem.terrain;

        if( !oter.is_valid() ) {
            if( invalid_terrains.count( oter.id() ) == 0 ) {
                invalid_terrains.insert( oter.id() );
                debugmsg( "In overmap special \"%s\", terrain \"%s\" is invalid.",
                          id.c_str(), oter.c_str() );
            }
        }

        const auto &pos = elem.p;

        if( points.count( pos ) > 0 ) {
            debugmsg( "In overmap special \"%s\", point [%d,%d,%d] is duplicated.",
                      id.c_str(), pos.x, pos.y, pos.z );
        } else {
            points.insert( pos );
        }
    }

    for( const auto &elem : connections ) {
        const auto &oter = get_terrain_at( elem.p );
        if( !elem.terrain ) {
            debugmsg( "In overmap special \"%s\", connection [%d,%d,%d] doesn't have a terrain.",
                      id.c_str(), elem.p.x, elem.p.y, elem.p.z );
        } else if( !elem.existing && !elem.terrain->has_flag( line_drawing ) ) {
            debugmsg( "In overmap special \"%s\", connection [%d,%d,%d] \"%s\" isn't drawn with lines.",
                      id.c_str(), elem.p.x, elem.p.y, elem.p.z, elem.terrain.c_str() );
        } else if( oter.terrain && !oter.terrain->type_is( elem.terrain ) ) {
            debugmsg( "In overmap special \"%s\", connection [%d,%d,%d] overwrites \"%s\".",
                      id.c_str(), elem.p.x, elem.p.y, elem.p.z, oter.terrain.c_str() );
        }
    }
}

// *** BEGIN overmap FUNCTIONS ***
overmap::overmap( const int x, const int y ) : loc( x, y )
{
    const std::string rsettings_id = get_option<std::string>( "DEFAULT_REGION" );
    t_regional_settings_map_citr rsit = region_settings_map.find( rsettings_id );

    if( rsit == region_settings_map.end() ) {
        debugmsg( "overmap(%d,%d): can't find region '%s'", x, y,
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
        debugmsg( "overmap (%d,%d) failed to load: %s", loc.x, loc.y, err.what() );
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
        // // @todo: Get rid of the hard-coded ids.
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

oter_id &overmap::ter( const int x, const int y, const int z )
{
    if( !inbounds( tripoint( x, y, z ) ) ) {
        return ot_null;
    }

    return layer[z + OVERMAP_DEPTH].terrain[x][y];
}

oter_id &overmap::ter( const tripoint &p )
{
    return ter( p.x, p.y, p.z );
}

const oter_id overmap::get_ter( const int x, const int y, const int z ) const
{
    if( !inbounds( tripoint( x, y, z ) ) ) {
        return ot_null;
    }

    return layer[z + OVERMAP_DEPTH].terrain[x][y];
}

const oter_id overmap::get_ter( const tripoint &p ) const
{
    return get_ter( p.x, p.y, p.z );
}

bool &overmap::seen( int x, int y, int z )
{
    if( !inbounds( tripoint( x, y, z ) ) ) {
        nullbool = false;
        return nullbool;
    }
    return layer[z + OVERMAP_DEPTH].visible[x][y];
}

bool &overmap::explored( int x, int y, int z )
{
    if( !inbounds( tripoint( x, y, z ) ) ) {
        nullbool = false;
        return nullbool;
    }
    return layer[z + OVERMAP_DEPTH].explored[x][y];
}

bool overmap::is_explored( const int x, const int y, const int z ) const
{
    if( !inbounds( tripoint( x, y, z ) ) ) {
        return false;
    }
    return layer[z + OVERMAP_DEPTH].explored[x][y];
}

bool overmap::mongroup_check( const mongroup &candidate ) const
{
    const auto matching_range = zg.equal_range( candidate.pos );
    return std::find_if( matching_range.first, matching_range.second,
    [candidate]( std::pair<tripoint, mongroup> match ) {
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

bool overmap::monster_check( const std::pair<tripoint, monster> &candidate ) const
{
    const auto matching_range = monster_map.equal_range( candidate.first );
    return std::find_if( matching_range.first, matching_range.second,
    [candidate]( std::pair<tripoint, monster> match ) {
        return candidate.second.pos() == match.second.pos() &&
               candidate.second.type == match.second.type;
    } ) != matching_range.second;
}

void overmap::insert_npc( std::shared_ptr<npc> who )
{
    npcs.push_back( who );
    g->set_npcs_dirty();
}

std::shared_ptr<npc> overmap::erase_npc( const int id )
{
    const auto iter = std::find_if( npcs.begin(), npcs.end(), [id]( const std::shared_ptr<npc> &n ) {
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

std::vector<std::shared_ptr<npc>> overmap::get_npcs( const std::function<bool( const npc & )>
                               &predicate ) const
{
    std::vector<std::shared_ptr<npc>> result;
    for( const auto &g : npcs ) {
        if( predicate( *g ) ) {
            result.push_back( g );
        }
    }
    return result;
}

bool overmap::has_note( const int x, const int y, const int z ) const
{
    if( z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
        return false;
    }

    for( auto &i : layer[z + OVERMAP_DEPTH].notes ) {
        if( i.x == x && i.y == y ) {
            return true;
        }
    }
    return false;
}

const std::string &overmap::note( const int x, const int y, const int z ) const
{
    static const std::string fallback {};

    if( z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
        return fallback;
    }

    const auto &notes = layer[z + OVERMAP_DEPTH].notes;
    const auto it = std::find_if( begin( notes ), end( notes ), [&]( const om_note & n ) {
        return n.x == x && n.y == y;
    } );

    return ( it != std::end( notes ) ) ? it->text : fallback;
}

void overmap::add_note( const int x, const int y, const int z, std::string message )
{
    if( z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT ) {
        debugmsg( "Attempting to add not to overmap for blank layer %d", z );
        return;
    }

    auto &notes = layer[z + OVERMAP_DEPTH].notes;
    const auto it = std::find_if( begin( notes ), end( notes ), [&]( const om_note & n ) {
        return n.x == x && n.y == y;
    } );

    if( it == std::end( notes ) ) {
        notes.emplace_back( om_note {std::move( message ), x, y} );
    } else if( !message.empty() ) {
        it->text = std::move( message );
    } else {
        notes.erase( it );
    }
}

void overmap::delete_note( const int x, const int y, const int z )
{
    add_note( x, y, z, std::string {} );
}

std::vector<point> overmap::find_notes( const int z, const std::string &text )
{
    std::vector<point> note_locations;
    map_layer &this_layer = layer[z + OVERMAP_DEPTH];
    for( auto note : this_layer.notes ) {
        if( match_include_exclude( note.text, text ) ) {
            note_locations.push_back( global_base_point() + point( note.x, note.y ) );
        }
    }
    return note_locations;
}

bool overmap::inbounds( const tripoint &p, int clearance )
{
    const tripoint overmap_boundary_min( 0, 0, -OVERMAP_DEPTH );
    const tripoint overmap_boundary_max( OMAPX, OMAPY, OVERMAP_HEIGHT );
    const tripoint overmap_clearance_min( 0 + clearance, 0 + clearance, 0 );
    const tripoint overmap_clearance_max( 1 + clearance, 1 + clearance, 0 );

    const box overmap_boundaries( overmap_boundary_min, overmap_boundary_max );
    const box overmap_clearance( overmap_clearance_min, overmap_clearance_max );

    return generic_inbounds( p, overmap_boundaries, overmap_clearance );
}

const scent_trace &overmap::scent_at( const tripoint &loc ) const
{
    static const scent_trace null_scent;
    const auto &scent_found = scents.find( loc );
    if( scent_found != scents.end() ) {
        return scent_found->second;
    }
    return null_scent;
}

void overmap::set_scent( const tripoint &loc, scent_trace &new_scent )
{
    // @todo: increase strength of scent trace when applied repeatedly in a short timespan.
    scents[loc] = new_scent;
}

void overmap::generate( const overmap *north, const overmap *east,
                        const overmap *south, const overmap *west,
                        overmap_special_batch &enabled_specials )
{
    dbg( D_INFO ) << "overmap::generate start...";
    std::vector<point> river_start;// West/North endpoints of rivers
    std::vector<point> river_end; // East/South endpoints of rivers

    // Determine points where rivers & roads should connect w/ adjacent maps
    const oter_id river_center( "river_center" ); // optimized comparison.

    if( north != nullptr ) {
        for( int i = 2; i < OMAPX - 2; i++ ) {
            if( is_river( north->get_ter( i, OMAPY - 1, 0 ) ) ) {
                ter( i, 0, 0 ) = river_center;
            }
            if( is_river( north->get_ter( i, OMAPY - 1, 0 ) ) &&
                is_river( north->get_ter( i - 1, OMAPY - 1, 0 ) ) &&
                is_river( north->get_ter( i + 1, OMAPY - 1, 0 ) ) ) {
                if( river_start.empty() ||
                    river_start[river_start.size() - 1].x < i - 6 ) {
                    river_start.push_back( point( i, 0 ) );
                }
            }
        }
        for( auto &i : north->roads_out ) {
            if( i.pos.y == OMAPY - 1 ) {
                roads_out.push_back( city( i.pos.x, 0, 0 ) );
            }
        }
    }
    size_t rivers_from_north = river_start.size();
    if( west != nullptr ) {
        for( int i = 2; i < OMAPY - 2; i++ ) {
            if( is_river( west->get_ter( OMAPX - 1, i, 0 ) ) ) {
                ter( 0, i, 0 ) = river_center;
            }
            if( is_river( west->get_ter( OMAPX - 1, i, 0 ) ) &&
                is_river( west->get_ter( OMAPX - 1, i - 1, 0 ) ) &&
                is_river( west->get_ter( OMAPX - 1, i + 1, 0 ) ) ) {
                if( river_start.size() == rivers_from_north ||
                    river_start[river_start.size() - 1].y < i - 6 ) {
                    river_start.push_back( point( 0, i ) );
                }
            }
        }
        for( auto &i : west->roads_out ) {
            if( i.pos.x == OMAPX - 1 ) {
                roads_out.push_back( city( 0, i.pos.y, 0 ) );
            }
        }
    }
    if( south != nullptr ) {
        for( int i = 2; i < OMAPX - 2; i++ ) {
            if( is_river( south->get_ter( i, 0, 0 ) ) ) {
                ter( i, OMAPY - 1, 0 ) = river_center;
            }
            if( is_river( south->get_ter( i,     0, 0 ) ) &&
                is_river( south->get_ter( i - 1, 0, 0 ) ) &&
                is_river( south->get_ter( i + 1, 0, 0 ) ) ) {
                if( river_end.empty() ||
                    river_end[river_end.size() - 1].x < i - 6 ) {
                    river_end.push_back( point( i, OMAPY - 1 ) );
                }
            }
        }
        for( auto &i : south->roads_out ) {
            if( i.pos.y == 0 ) {
                roads_out.push_back( city( i.pos.x, OMAPY - 1, 0 ) );
            }
        }
    }
    size_t rivers_to_south = river_end.size();
    if( east != nullptr ) {
        for( int i = 2; i < OMAPY - 2; i++ ) {
            if( is_river( east->get_ter( 0, i, 0 ) ) ) {
                ter( OMAPX - 1, i, 0 ) = river_center;
            }
            if( is_river( east->get_ter( 0, i, 0 ) ) &&
                is_river( east->get_ter( 0, i - 1, 0 ) ) &&
                is_river( east->get_ter( 0, i + 1, 0 ) ) ) {
                if( river_end.size() == rivers_to_south ||
                    river_end[river_end.size() - 1].y < i - 6 ) {
                    river_end.push_back( point( OMAPX - 1, i ) );
                }
            }
        }
        for( auto &i : east->roads_out ) {
            if( i.pos.x == 0 ) {
                roads_out.push_back( city( OMAPX - 1, i.pos.y, 0 ) );
            }
        }
    }

    // Even up the start and end points of rivers. (difference of 1 is acceptable)
    // Also ensure there's at least one of each.
    std::vector<point> new_rivers;
    if( north == nullptr || west == nullptr ) {
        while( river_start.empty() || river_start.size() + 1 < river_end.size() ) {
            new_rivers.clear();
            if( north == nullptr ) {
                new_rivers.push_back( point( rng( 10, OMAPX - 11 ), 0 ) );
            }
            if( west == nullptr ) {
                new_rivers.push_back( point( 0, rng( 10, OMAPY - 11 ) ) );
            }
            river_start.push_back( random_entry( new_rivers ) );
        }
    }
    if( south == nullptr || east == nullptr ) {
        while( river_end.empty() || river_end.size() + 1 < river_start.size() ) {
            new_rivers.clear();
            if( south == nullptr ) {
                new_rivers.push_back( point( rng( 10, OMAPX - 11 ), OMAPY - 1 ) );
            }
            if( east == nullptr ) {
                new_rivers.push_back( point( OMAPX - 1, rng( 10, OMAPY - 11 ) ) );
            }
            river_end.push_back( random_entry( new_rivers ) );
        }
    }

    // Now actually place those rivers.
    if( river_start.size() > river_end.size() && !river_end.empty() ) {
        std::vector<point> river_end_copy = river_end;
        while( !river_start.empty() ) {
            const point start = random_entry_removed( river_start );
            if( !river_end.empty() ) {
                place_river( start, river_end[0] );
                river_end.erase( river_end.begin() );
            } else {
                place_river( start, random_entry( river_end_copy ) );
            }
        }
    } else if( river_end.size() > river_start.size() && !river_start.empty() ) {
        std::vector<point> river_start_copy = river_start;
        while( !river_end.empty() ) {
            const point end = random_entry_removed( river_end );
            if( !river_start.empty() ) {
                place_river( river_start[0], end );
                river_start.erase( river_start.begin() );
            } else {
                place_river( random_entry( river_start_copy ), end );
            }
        }
    } else if( !river_end.empty() ) {
        if( river_start.size() != river_end.size() )
            river_start.push_back( point( rng( OMAPX / 4, ( OMAPX * 3 ) / 4 ),
                                          rng( OMAPY / 4, ( OMAPY * 3 ) / 4 ) ) );
        for( size_t i = 0; i < river_start.size(); i++ ) {
            place_river( river_start[i], river_end[i] );
        }
    }

    // Cities and forests come next.
    // These are agnostic of adjacent maps, so it's very simple.
    place_cities();
    place_forest();

    place_forest_trails();

    // Ideally we should have at least two exit points for roads, on different sides
    if( roads_out.size() < 2 ) {
        std::vector<city> viable_roads;
        int tmp;
        // Populate viable_roads with one point for each neighborless side.
        // Make sure these points don't conflict with rivers.
        // @todo: In theory this is a potential infinite loop...
        if( north == nullptr ) {
            do {
                tmp = rng( 10, OMAPX - 11 );
            } while( is_river( ter( tmp, 0, 0 ) ) || is_river( ter( tmp - 1, 0, 0 ) ) ||
                     is_river( ter( tmp + 1, 0, 0 ) ) );
            viable_roads.push_back( city( tmp, 0, 0 ) );
        }
        if( east == nullptr ) {
            do {
                tmp = rng( 10, OMAPY - 11 );
            } while( is_river( ter( OMAPX - 1, tmp, 0 ) ) || is_river( ter( OMAPX - 1, tmp - 1, 0 ) ) ||
                     is_river( ter( OMAPX - 1, tmp + 1, 0 ) ) );
            viable_roads.push_back( city( OMAPX - 1, tmp, 0 ) );
        }
        if( south == nullptr ) {
            do {
                tmp = rng( 10, OMAPX - 11 );
            } while( is_river( ter( tmp, OMAPY - 1, 0 ) ) || is_river( ter( tmp - 1, OMAPY - 1, 0 ) ) ||
                     is_river( ter( tmp + 1, OMAPY - 1, 0 ) ) );
            viable_roads.push_back( city( tmp, OMAPY - 1, 0 ) );
        }
        if( west == nullptr ) {
            do {
                tmp = rng( 10, OMAPY - 11 );
            } while( is_river( ter( 0, tmp, 0 ) ) || is_river( ter( 0, tmp - 1, 0 ) ) ||
                     is_river( ter( 0, tmp + 1, 0 ) ) );
            viable_roads.push_back( city( 0, tmp, 0 ) );
        }
        while( roads_out.size() < 2 && !viable_roads.empty() ) {
            roads_out.push_back( random_entry_removed( viable_roads ) );
        }
    }

    std::vector<point> road_points; // cities and roads_out together
    // Compile our master list of roads; it's less messy if roads_out is first
    road_points.reserve( roads_out.size() + cities.size() );
    for( const auto &elem : roads_out ) {
        road_points.emplace_back( elem.pos );
    }
    for( const auto &elem : cities ) {
        road_points.emplace_back( elem.pos.x, elem.pos.y );
    }

    // And finally connect them via roads.
    const string_id<overmap_connection> local_road( "local_road" );
    connect_closest_points( road_points, 0, *local_road );

    place_specials( enabled_specials );

    // After we've placed all the specials and connected everything else via roads,
    // try and place some trailheads.
    place_forest_trailheads();

    polish_river();

    // @todo: there is no reason we can't generate the sublevels in one pass
    //       for that matter there is no reason we can't as we add the entrance ways either

    // Always need at least one sublevel, but how many more
    int z = -1;
    bool requires_sub = false;
    do {
        requires_sub = generate_sub( z );
    } while( requires_sub && ( --z >= -OVERMAP_DEPTH ) );

    // Place the monsters, now that the terrain is laid out
    place_mongroups();
    place_radios();
    dbg( D_INFO ) << "overmap::generate done";
}

bool overmap::generate_sub( const int z )
{
    bool requires_sub = false;
    std::vector<point> subway_points;
    std::vector<point> sewer_points;

    std::vector<city> ant_points;
    std::vector<city> goo_points;
    std::vector<city> lab_points;
    std::vector<city> ice_lab_points;
    std::vector<city> central_lab_points;
    std::vector<point> lab_train_points;
    std::vector<point> central_lab_train_points;
    std::vector<point> shaft_points;
    std::vector<city> mine_points;
    // These are so common that it's worth checking first as int.
    const oter_id skip_above[5] = {
        oter_id( "empty_rock" ), oter_id( "forest" ), oter_id( "field" ),
        oter_id( "forest_thick" ), oter_id( "forest_water" )
    };

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            oter_id oter_above = ter( i, j, z + 1 );
            oter_id oter_ground = ter( i, j, 0 );
            //oter_id oter_sewer = ter(i, j, -1);
            //oter_id oter_underground = ter(i, j, -2);

            // implicitly skip skip_above oter_ids
            bool skipme = false;
            for( auto &elem : skip_above ) {
                if( oter_above == elem ) {
                    skipme = true;
                    break;
                }
            }
            if( skipme ) {
                continue;
            }

            if( is_ot_type( "sub_station", oter_ground ) && z == -1 ) {
                ter( i, j, z ) = oter_id( "sewer_sub_station" );
                requires_sub = true;
            } else if( is_ot_type( "sub_station", oter_ground ) && z == -2 ) {
                ter( i, j, z ) = oter_id( "subway_isolated" );
                subway_points.emplace_back( i, j - 1 );
                subway_points.emplace_back( i, j );
                subway_points.emplace_back( i, j + 1 );
            } else if( oter_above == "road_nesw_manhole" ) {
                ter( i, j, z ) = oter_id( "sewer_isolated" );
                sewer_points.emplace_back( i, j );
            } else if( oter_above == "sewage_treatment" ) {
                sewer_points.emplace_back( i, j );
            } else if( oter_above == "cave" && z == -1 ) {
                if( one_in( 3 ) ) {
                    ter( i, j, z ) = oter_id( "cave_rat" );
                    requires_sub = true; // rat caves are two level
                } else {
                    ter( i, j, z ) = oter_id( "cave" );
                }
            } else if( oter_above == "cave_rat" && z == -2 ) {
                ter( i, j, z ) = oter_id( "cave_rat" );
            } else if( oter_above == "anthill" || oter_above == "acid_anthill" ) {
                mongroup_id ant_group( oter_above == "anthill" ? "GROUP_ANT" : "GROUP_ANT_ACID" );
                int size = rng( MIN_ANT_SIZE, MAX_ANT_SIZE );
                ant_points.push_back( city( i, j, size ) );
                add_mon_group( mongroup( ant_group, i * 2, j * 2, z,
                                         ( size * 3 ) / 2, rng( 6000, 8000 ) ) );
            } else if( oter_above == "slimepit_down" ) {
                int size = rng( MIN_GOO_SIZE, MAX_GOO_SIZE );
                goo_points.push_back( city( i, j, size ) );
            } else if( oter_above == "forest_water" ) {
                ter( i, j, z ) = oter_id( "cavern" );
                chip_rock( i, j, z );
            } else if( oter_above == "lab_core" ||
                       ( z == -1 && oter_above == "lab_stairs" ) ) {
                lab_points.push_back( city( i, j, rng( 1, 5 + z ) ) );
            } else if( oter_above == "lab_stairs" ) {
                ter( i, j, z ) = oter_id( "lab" );
            } else if( oter_above == "ice_lab_core" ||
                       ( z == -1 && oter_above == "ice_lab_stairs" ) ) {
                ice_lab_points.push_back( city( i, j, rng( 1, 5 + z ) ) );
            } else if( oter_above == "ice_lab_stairs" ) {
                ter( i, j, z ) = oter_id( "ice_lab" );
            } else if( oter_above == "central_lab_core" ) {
                central_lab_points.push_back( city( i, j, rng( std::max( 1, 7 + z ), 9 + z ) ) );
            } else if( oter_above == "central_lab_stairs" ) {
                ter( i, j, z ) = oter_id( "central_lab" );
            } else if( is_ot_subtype( "hidden_lab_stairs", oter_above ) ) {
                lab_points.push_back( city( i, j, rng( 1, 5 + z ) ) );
            } else if( oter_above == "mine_entrance" ) {
                shaft_points.push_back( point( i, j ) );
            } else if( oter_above == "mine_shaft" ||
                       oter_above == "mine_down" ) {
                ter( i, j, z ) = oter_id( "mine" );
                mine_points.push_back( city( i, j, rng( 6 + z, 10 + z ) ) );
                // technically not all finales need a sub level,
                // but at this point we don't know
                requires_sub = true;
            } else if( oter_above == "mine_finale" ) {
                for( auto &p : g->m.points_in_radius( tripoint( i, j, z ), 1, 0 ) ) {
                    ter( p.x, p.y, p.z ) = oter_id( "spiral" );
                }
                ter( i, j, z ) = oter_id( "spiral_hub" );
                add_mon_group( mongroup( mongroup_id( "GROUP_SPIRAL" ), i * 2, j * 2, z, 2, 200 ) );
            } else if( oter_above == "silo" ) {
                if( rng( 2, 7 ) < abs( z ) || rng( 2, 7 ) < abs( z ) ) {
                    ter( i, j, z ) = oter_id( "silo_finale" );
                } else {
                    ter( i, j, z ) = oter_id( "silo" );
                    requires_sub = true;
                }
            }
        }
    }

    for( auto &i : goo_points ) {
        requires_sub |= build_slimepit( i.pos.x, i.pos.y, z, i.size );
    }
    const string_id<overmap_connection> sewer_tunnel( "sewer_tunnel" );
    connect_closest_points( sewer_points, z, *sewer_tunnel );

    for( auto &i : ant_points ) {
        build_anthill( i.pos.x, i.pos.y, z, i.size );
    }

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
        bool lab = build_lab( i.pos.x, i.pos.y, z, i.size, &lab_train_points, "", lab_train_odds );
        requires_sub |= lab;
        if( !lab && ter( i.pos.x, i.pos.y, z ) == "lab_core" ) {
            ter( i.pos.x, i.pos.y, z ) = oter_id( "lab" );
        }
    }
    for( auto &i : ice_lab_points ) {
        bool ice_lab = build_lab( i.pos.x, i.pos.y, z, i.size, &lab_train_points, "ice_", lab_train_odds );
        requires_sub |= ice_lab;
        if( !ice_lab && ter( i.pos.x, i.pos.y, z ) == "ice_lab_core" ) {
            ter( i.pos.x, i.pos.y, z ) = oter_id( "ice_lab" );
        }
    }
    for( auto &i : central_lab_points ) {
        bool central_lab = build_lab( i.pos.x, i.pos.y, z, i.size, &central_lab_train_points, "central_",
                                      lab_train_odds );
        requires_sub |= central_lab;
        if( !central_lab && ter( i.pos.x, i.pos.y, z ) == "central_lab_core" ) {
            ter( i.pos.x, i.pos.y, z ) = oter_id( "central_lab" );
        }
    }

    const string_id<overmap_connection> subway_tunnel( "subway_tunnel" );

    subway_points.insert( subway_points.end(), lab_train_points.begin(), lab_train_points.end() );
    subway_points.insert( subway_points.end(), central_lab_train_points.begin(),
                          central_lab_train_points.end() );
    connect_closest_points( subway_points, z, *subway_tunnel );
    // If on z = 4 and central lab is present, be sure to connect normal labs and central labs (just in case).
    if( z == -4 && !central_lab_points.empty() && !lab_train_points.empty() ) {
        std::vector<point> extra_route;
        extra_route.push_back( central_lab_train_points.back() );
        extra_route.push_back( lab_train_points.back() );
        connect_closest_points( extra_route, z, *subway_tunnel );
    }

    for( auto &i : subway_points ) {
        if( is_ot_type( "sub_station", ter( i.x, i.y, z + 2 ) ) ) {
            ter( i.x, i.y, z ) = oter_id( "underground_sub_station" );
        }
    }

    // The first lab point is adjacent to a lab, set it a depot (as long as track was actually laid).
    const auto create_train_depots = [this, z]( const oter_id & train_type,
    const std::vector<point> &train_points ) {
        bool is_first_in_pair = true;
        for( auto &i : train_points ) {
            if( is_first_in_pair ) {
                if( is_ot_subtype( "subway", ter( i.x + 1, i.y, z ) ) ||
                    is_ot_subtype( "subway", ter( i.x - 1, i.y, z ) ) ||
                    is_ot_subtype( "subway", ter( i.x, i.y + 1, z ) ) ||
                    is_ot_subtype( "subway", ter( i.x, i.y - 1, z ) ) ) {
                    ter( i.x, i.y, z ) = train_type;
                } else {
                    ter( i.x, i.y, z ) = oter_id( "empty_rock" );
                }
            }
            is_first_in_pair = !is_first_in_pair;
        }
    };
    create_train_depots( oter_id( "lab_train_depot" ), lab_train_points );
    create_train_depots( oter_id( "central_lab_train_depot" ), central_lab_train_points );

    for( auto &i : cities ) {
        // Sewers and city subways are present at z == -1 and z == -2. Don't spawn CHUD on other z-levels.
        if( ( z == -1 || z == -2 ) && one_in( 3 ) ) {
            add_mon_group( mongroup( mongroup_id( "GROUP_CHUD" ),
                                     i.pos.x * 2, i.pos.y * 2, z, i.size, i.size * 20 ) );
        }
        // Sewers are present at z == -1. Don't spawn sewer monsters on other z-levels.
        if( z == -1 && !one_in( 8 ) ) {
            add_mon_group( mongroup( mongroup_id( "GROUP_SEWER" ),
                                     i.pos.x * 2, i.pos.y * 2, z, ( i.size * 7 ) / 2, i.size * 70 ) );
        }
    }

    // Disable rifts when they can interfere with subways and sewers.
    if( z < -4 ) {
        place_rifts( z );
    }
    for( auto &i : mine_points ) {
        build_mine( i.pos.x, i.pos.y, z, i.size );
    }

    for( auto &i : shaft_points ) {
        ter( i.x, i.y, z ) = oter_id( "mine_shaft" );
        requires_sub = true;
    }
    return requires_sub;
}

std::vector<point> overmap::find_terrain( const std::string &term, int zlevel )
{
    std::vector<point> found;
    for( int x = 0; x < OMAPX; x++ ) {
        for( int y = 0; y < OMAPY; y++ ) {
            if( seen( x, y, zlevel ) &&
                lcmatch( ter( x, y, zlevel )->get_name(), term ) ) {
                found.push_back( global_base_point() + point( x, y ) );
            }
        }
    }
    return found;
}

const city &overmap::get_nearest_city( const tripoint &p ) const
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

tripoint overmap::find_random_omt( const std::string &omt_base_type ) const
{
    std::vector<tripoint> valid;
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            for( int k = -OVERMAP_DEPTH; k <= OVERMAP_HEIGHT; k++ ) {
                if( get_ter( i, j, k )->get_type_id().str() == omt_base_type ) {
                    valid.push_back( tripoint( i, j, k ) );
                }
            }
        }
    }
    return random_entry( valid, invalid_tripoint );
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

void mongroup::wander( overmap &om )
{
    const city *target_city = nullptr;
    int target_distance = 0;

    if( horde_behaviour == "city" ) {
        // Find a nearby city to return to..
        for( const city &check_city : om.cities ) {
            // Check if this is the nearest city so far.
            int distance = rl_dist( check_city.pos.x * 2, check_city.pos.y * 2, pos.x, pos.y );
            if( !target_city || distance < target_distance ) {
                target_distance = distance;
                target_city = &check_city;
            }
        }
    }

    if( target_city ) {
        // @todo: somehow use the same algorithm that distributes zombie
        // density at world gen to spread the hordes over the actual
        // city, rather than the center city tile
        target.x = target_city->pos.x * 2 + rng( -5, 5 );
        target.y = target_city->pos.y * 2 + rng( -5, 5 );
        interest = 100;
    } else {
        target.x = pos.x + rng( -10, 10 );
        target.y = pos.y + rng( -10, 10 );
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

        if( ( mg.pos.x == mg.target.x && mg.pos.y == mg.target.y ) || mg.interest <= 15 ) {
            mg.wander( *this );
        }

        // Decrease movement chance according to the terrain we're currently on.
        const oter_id &walked_into = ter( mg.pos.x, mg.pos.y, mg.pos.z );
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
            // @todo: Handle moving to adjacent overmaps.
            if( mg.pos.x > mg.target.x ) {
                mg.pos.x--;
            }
            if( mg.pos.x < mg.target.x ) {
                mg.pos.x++;
            }
            if( mg.pos.y > mg.target.y ) {
                mg.pos.y--;
            }
            if( mg.pos.y < mg.target.y ) {
                mg.pos.y++;
            }

            // Erase the group at it's old location, add the group with the new location
            tmpzg.insert( std::pair<tripoint, mongroup>( mg.pos, mg ) );
            zg.erase( it++ );
        } else {
            ++it;
        }
    }
    // and now back into the monster group map.
    zg.insert( tmpzg.begin(), tmpzg.end() );

    if( get_option<bool>( "WANDER_SPAWNS" ) ) {
        static const mongroup_id GROUP_ZOMBIE( "GROUP_ZOMBIE" );

        // Re-absorb zombies into hordes.
        // Scan over monsters outside the player's view and place them back into hordes.
        auto monster_map_it = monster_map.begin();
        while( monster_map_it != monster_map.end() ) {
            const auto &p = monster_map_it->first;
            auto &this_monster = monster_map_it->second;

            // Only zombies on z-level 0 may join hordes.
            if( p.z != 0 ) {
                monster_map_it++;
                continue;
            }

            // Check if the monster is a zombie.
            auto &type = *( this_monster.type );
            if(
                !type.species.count( species_id( "ZOMBIE" ) ) || // Only add zombies to hordes.
                type.id == mtype_id( "mon_jabberwock" ) || // Jabberwockies are an exception.
                this_monster.get_speed() <= 30 || // So are very slow zombies, like crawling zombies.
                this_monster.has_effect( effect_pet ) || // "Zombie pet" zlaves are, too.
                !this_monster.will_join_horde( INT_MAX ) || // So are zombies who won't join a horde of any size.
                this_monster.mission_id != -1 // We mustn't delete monsters that are related to missions.
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
            [&]( std::pair<const tripoint, mongroup> &horde_entry ) {
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
                    mongroup m( GROUP_ZOMBIE, p.x, p.y, p.z, 1, 0 );
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
* @param p location of signal
* @param sig_power - power of signal or max distance for reaction of zombies
*/
void overmap::signal_hordes( const tripoint &p, const int sig_power )
{
    for( auto &elem : zg ) {
        mongroup &mg = elem.second;
        if( !mg.horde ) {
            continue;
        }
        const int dist = rl_dist( p, mg.pos );
        if( sig_power < dist ) {
            continue;
        }
        // @todo: base this in monster attributes, foremost GOODHEARING.
        const int inter_per_sig_power = 15; //Interest per signal value
        const int min_initial_inter = 30; //Min initial interest for horde
        const int calculated_inter = ( sig_power + 1 - dist ) * inter_per_sig_power; // Calculated interest
        const int roll = rng( 0, mg.interest );
        // Minimum capped calculated interest. Used to give horde enough interest to really investigate the target at start.
        const int min_capped_inter = std::max( min_initial_inter, calculated_inter );
        if( roll < min_capped_inter ) { //Rolling if horde interested in new signal
            // TODO: Z-coordinate for mongroup targets
            const int targ_dist = rl_dist( p, mg.target );
            // @todo: Base this on targ_dist:dist ratio.
            if( targ_dist < 5 ) {  // If signal source already pursued by horde
                mg.set_target( ( mg.target.x + p.x ) / 2, ( mg.target.y + p.y ) / 2 );
                const int min_inc_inter = 3; // Min interest increase to already targeted source
                const int inc_roll = rng( min_inc_inter, calculated_inter );
                mg.inc_interest( inc_roll );
                add_msg( m_debug, "horde inc interest %d dist %d", inc_roll, dist ) ;
            } else { // New signal source
                mg.set_target( p.x, p.y );
                mg.set_interest( min_capped_inter );
                add_msg( m_debug, "horde set interest %d dist %d", min_capped_inter, dist );
            }
        }
    }
}

void grow_forest_oter_id( oter_id &oid, bool swampy )
{
    if( swampy && ( oid == ot_field || oid == ot_forest ) ) {
        oid = ot_forest_water;
    } else if( oid == ot_forest ) {
        oid = ot_forest_thick;
    } else if( oid == ot_field ) {
        oid = ot_forest;
    }
}

void overmap::place_forest()
{
    int forests_placed = 0;
    for( int i = 0; i < settings.num_forests; i++ ) {
        int forx = 0;
        int fory = 0;
        int fors = 0;
        // try to place this forest
        int tries = 100;
        do {
            // forx and fory determine the epicenter of the forest
            forx = rng( 0, OMAPX - 1 );
            fory = rng( 0, OMAPY - 1 );
            // fors determines its basic size
            fors = rng( settings.forest_size_min, settings.forest_size_max );
            const auto iter = std::find_if(
                                  cities.begin(),
                                  cities.end(),
            [&]( const city & c ) {
                return
                    // is this city too close?
                    trig_dist( forx, fory, c.pos.x, c.pos.y ) - fors / 2 < c.size &&
                    // occasionally accept near a city if we've been failing
                    tries > rng( -1000 / ( i - forests_placed + 1 ), 2 );
            }
                              );
            if( iter == cities.end() ) { // every city was too close
                break;
            }
        } while( tries-- );

        // if we gave up, don't bother trying to place another forest
        if( tries == 0 ) {
            break;
        }

        forests_placed++;

        int swamps = settings.swamp_maxsize; // How big the swamp may be...
        int x = forx;
        int y = fory;

        // Depending on the size on the forest...
        for( int j = 0; j < fors; j++ ) {
            int swamp_chance = 0;
            for( int k = -2; k <= 2; k++ ) {
                for( int l = -2; l <= 2; l++ ) {
                    if( ter( x + k, y + l, 0 ) == "forest_water" ||
                        check_ot_type( "river", x + k, y + l, 0 ) ) {
                        swamp_chance += settings.swamp_river_influence;
                    }
                }
            }
            bool swampy = false;
            if( swamps > 0 && swamp_chance > 0 && !one_in( swamp_chance ) &&
                ( ter( x, y, 0 ) == "forest" || ter( x, y, 0 ) == "forest_thick" ||
                  ter( x, y, 0 ) == "field" || one_in( settings.swamp_spread_chance ) ) ) {
                // ...and make a swamp.
                ter( x, y, 0 ) = oter_id( "forest_water" );
                swampy = true;
                swamps--;
            } else if( swamp_chance == 0 ) {
                swamps = settings.swamp_maxsize;
            }

            // Place or enlarge forest
            for( int mx = -1; mx < 2; mx++ ) {
                for( int my = -1; my < 2; my++ ) {
                    grow_forest_oter_id( ter( x + mx, y + my, 0 ),
                                         ( mx == 0 && my == 0 ? false : swampy ) );
                }
            }
            // Random walk our forest
            x += rng( -2, 2 );
            if( x < 0 ) {
                x = 0;
            }
            if( x > OMAPX ) {
                x = OMAPX;
            }
            y += rng( -2, 2 );
            if( y < 0 ) {
                y = 0;
            }
            if( y > OMAPY ) {
                y = OMAPY;
            }
        }
    }
}

void overmap::place_forest_trails()
{
    std::unordered_set<point> visited;

    const auto get_forest = [&]( point starting_point, std::vector<point> &forest_points ) {
        std::queue<point> to_check;
        to_check.push( starting_point );
        while( !to_check.empty() ) {
            const point current_point = to_check.front();
            to_check.pop();

            // We've been here before, so bail.
            if( visited.find( current_point ) != visited.end() ) {
                continue;
            }

            // This point is out of bounds, so bail.
            if( !inbounds( current_point, 1 ) ) {
                continue;
            }

            // Mark this point as visited.
            visited.emplace( current_point );

            // If this point is a valid forest type, then add it to our collection
            // of forest points.
            const auto current_terrain = ter( current_point.x, current_point.y, 0 );
            if( current_terrain == "forest" || current_terrain == "forest_thick" ||
                current_terrain == "forest_water" ) {
                forest_points.emplace_back( current_point );
                to_check.push( point( current_point.x, current_point.y + 1 ) );
                to_check.push( point( current_point.x, current_point.y - 1 ) );
                to_check.push( point( current_point.x + 1, current_point.y ) );
                to_check.push( point( current_point.x - 1, current_point.y ) );
            }
        }

        return;
    };

    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            oter_id oter = ter( i, j, 0 );
            if( !is_ot_type( "forest", oter ) ) {
                continue;
            }

            point seed_point( i, j );

            // If we've already visited this point, we don't need to
            // process it since it's already part of another forest.
            if( visited.find( seed_point ) != visited.end() ) {
                continue;
            }

            // Get the contiguous forest from this point.
            std::vector<point> forest_points;
            get_forest( seed_point, forest_points );

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
            forest_points.end(), []( const point & lhs, const point & rhs ) {
                return lhs.y < rhs.y;
            } );

            // Get the west and east most points in the forest.
            auto west_east_most = std::minmax_element( forest_points.begin(),
            forest_points.end(), []( const point & lhs, const point & rhs ) {
                return lhs.x < rhs.x;
            } );

            // We'll use these points later as points that are guaranteed to be
            // at a boundary and will form a good foundation for the trail system.
            point northmost = *north_south_most.first;
            point southmost = *north_south_most.second;
            point westmost = *west_east_most.first;
            point eastmost = *west_east_most.second;

            // Do a simplistic calculation of the center of the forest (rather than
            // calculating the actual centroid--it's not that important) to have another
            // good point to form the foundation of the trail system.
            int center_x = westmost.x + ( eastmost.x - westmost.x ) / 2;
            int center_y = northmost.y + ( southmost.y - northmost.y ) / 2;

            point center_point = point( center_x, center_y );

            // Because we didn't do the centroid of a concave polygon, there's no
            // guarantee that our center point is actually within the bounds of the
            // forest. Just find the point within our set that is closest to our
            // center point and use that.
            point actual_center_point = *std::min_element( forest_points.begin(),
            forest_points.end(), [&center_point]( const point & lhs, const point & rhs ) {
                return square_dist( lhs.x, lhs.y, center_point.x, center_point.y ) < square_dist( rhs.x, rhs.y,
                        center_point.x, center_point.y );
            } );

            // Figure out how many random points we'll add to our trail system, based on the forest
            // size and our configuration.
            int max_random_points = settings.forest_trail.random_point_min + forest_points.size() /
                                    settings.forest_trail.random_point_size_scalar;
            max_random_points = std::min( max_random_points, settings.forest_trail.random_point_max );

            // Start with the center...
            std::vector<point> chosen_points = { actual_center_point };

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

    // Add the roads out of the overmap and cities to our collection, which
    // we'll then use to connect our trailheads to the rest of the road network.
    std::vector<point> road_points;
    road_points.reserve( roads_out.size() + cities.size() );
    for( const auto &elem : roads_out ) {
        road_points.emplace_back( elem.pos );
    }
    for( const auto &elem : cities ) {
        road_points.emplace_back( elem.pos.x, elem.pos.y );
    }

    // Trailheads may be placed if all of the following are true:
    // 1. we're at a forest_trail_end_north/south/west/east,
    // 2. the next two overmap terrains continuing in the direction
    //    of the trail are fields
    // 3. we're within trailhead_road_distance from an existing road
    // 4. rng rolls a success for our trailhead_chance from the configuration

    const auto trailhead_close_to_road = [&]( const tripoint trailhead ) {
        bool close = false;
        for( point nearby_point : closest_points_first( settings.forest_trail.trailhead_road_distance,
                point( trailhead.x, trailhead.y ) ) ) {
            if( check_ot_subtype( "road", nearby_point.x, nearby_point.y, 0 ) ) {
                close = true;
            }
        }
        return close;
    };

    const auto try_place_trailhead = [&]( const tripoint trailhead, const tripoint road,
    const std::string suffix ) {
        oter_id &oter_potential_trailhead = ter( trailhead );
        oter_id &oter_potential_road = ter( road );
        if( oter_potential_trailhead == "field" && oter_potential_road == "field" &&
            one_in( settings.forest_trail.trailhead_chance ) && trailhead_close_to_road( trailhead ) ) {
            oter_potential_trailhead = oter_id( "trailhead" + suffix );
            road_points.emplace_back( road.x, road.y );
        }
    };

    for( int i = 2; i < OMAPX - 2; i++ ) {
        for( int j = 2; j < OMAPY - 2; j++ ) {
            oter_id oter = ter( i, j, 0 );
            if( oter == "forest_trail_end_north" ) {
                try_place_trailhead( tripoint( i, j - 1, 0 ), tripoint( i, j - 2, 0 ), "_north" );
            } else if( oter == "forest_trail_end_south" ) {
                try_place_trailhead( tripoint( i, j + 1, 0 ), tripoint( i, j + 2, 0 ), "_south" );
            } else if( oter == "forest_trail_end_west" ) {
                try_place_trailhead( tripoint( i - 1, j, 0 ), tripoint( i - 2, j, 0 ), "_west" );
            } else if( oter == "forest_trail_end_east" ) {
                try_place_trailhead( tripoint( i + 1, j, 0 ), tripoint( i + 2, j, 0 ), "_east" );
            } else {
                continue;
            }
        }
    }

    // If we actually added some trailheads...
    if( road_points.size() > roads_out.size() ) {
        // ...then connect our road points with local_road connections.
        const string_id<overmap_connection> local_road( "local_road" );
        connect_closest_points( road_points, 0, *local_road );
    }
}

void overmap::place_river( point pa, point pb )
{
    int x = pa.x;
    int y = pa.y;
    do {
        x += rng( -1, 1 );
        y += rng( -1, 1 );
        if( x < 0 ) {
            x = 0;
        }
        if( x > OMAPX - 1 ) {
            x = OMAPX - 1;
        }
        if( y < 0 ) {
            y = 0;
        }
        if( y > OMAPY - 1 ) {
            y = OMAPY - 1;
        }
        for( int i = -1; i <= 1; i++ ) {
            for( int j = -1; j <= 1; j++ ) {
                if( y + i >= 0 && y + i < OMAPY && x + j >= 0 && x + j < OMAPX ) {
                    ter( x + j, y + i, 0 ) = oter_id( "river_center" );
                }
            }
        }
        if( pb.x > x && ( rng( 0, int( OMAPX * 1.2 ) - 1 ) < pb.x - x ||
                          ( rng( 0, int( OMAPX * .2 ) - 1 ) > pb.x - x &&
                            rng( 0, int( OMAPY * .2 ) - 1 ) > abs( pb.y - y ) ) ) ) {
            x++;
        }
        if( pb.x < x && ( rng( 0, int( OMAPX * 1.2 ) - 1 ) < x - pb.x ||
                          ( rng( 0, int( OMAPX * .2 ) - 1 ) > x - pb.x &&
                            rng( 0, int( OMAPY * .2 ) - 1 ) > abs( pb.y - y ) ) ) ) {
            x--;
        }
        if( pb.y > y && ( rng( 0, int( OMAPY * 1.2 ) - 1 ) < pb.y - y ||
                          ( rng( 0, int( OMAPY * .2 ) - 1 ) > pb.y - y &&
                            rng( 0, int( OMAPX * .2 ) - 1 ) > abs( x - pb.x ) ) ) ) {
            y++;
        }
        if( pb.y < y && ( rng( 0, int( OMAPY * 1.2 ) - 1 ) < y - pb.y ||
                          ( rng( 0, int( OMAPY * .2 ) - 1 ) > y - pb.y &&
                            rng( 0, int( OMAPX * .2 ) - 1 ) > abs( x - pb.x ) ) ) ) {
            y--;
        }
        x += rng( -1, 1 );
        y += rng( -1, 1 );
        if( x < 0 ) {
            x = 0;
        }
        if( x > OMAPX - 1 ) {
            x = OMAPX - 2;
        }
        if( y < 0 ) {
            y = 0;
        }
        if( y > OMAPY - 1 ) {
            y = OMAPY - 1;
        }
        for( int i = -1; i <= 1; i++ ) {
            for( int j = -1; j <= 1; j++ ) {
                // We don't want our riverbanks touching the edge of the map for many reasons
                if( inbounds( tripoint( x + j, y + i, 0 ), 1 ) ||
                    // UNLESS, of course, that's where the river is headed!
                    ( abs( pb.y - ( y + i ) ) < 4 && abs( pb.x - ( x + j ) ) < 4 ) ) {
                    ter( x + j, y + i, 0 ) = oter_id( "river_center" );
                }
            }
        }
    } while( pb.x != x || pb.y != y );
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
    const double omts_per_city = ( op_city_size * 2 + 1 ) * ( op_city_size * 2 + 1 ) * 3 / 4;

    // how many cities on this overmap?
    const int NUM_CITIES =
        roll_remainder( omts_per_overmap * city_map_coverage_ratio / omts_per_city );

    const string_id<overmap_connection> local_road_id( "local_road" );
    const overmap_connection &local_road( *local_road_id );

    // place a seed for NUM_CITIES cities, and maybe one more
    while( cities.size() < size_t( NUM_CITIES ) ) {
        // randomly make some cities smaller or larger
        int size = rng( op_city_size - 1, op_city_size + 1 );
        if( one_in( 3 ) ) {      // 33% tiny
            size = 1;
        } else if( one_in( 2 ) ) { // 33% small
            size = size * 2 / 3;
        } else if( one_in( 2 ) ) { // 17% large
            size = size * 3 / 2;
        } else {                 // 17% huge
            size = size * 2;
        }
        size = std::max( size, 1 );

        // @todo put cities closer to the edge when they can span overmaps
        // don't draw cities across the edge of the map, they will get clipped
        int cx = rng( size - 1, OMAPX - size );
        int cy = rng( size - 1, OMAPY - size );
        if( ter( cx, cy, 0 ) == settings.default_oter ) {
            ter( cx, cy, 0 ) = oter_id( "road_nesw" ); // every city starts with an intersection
            city tmp;
            tmp.pos = { cx, cy };
            tmp.size = size;
            cities.push_back( tmp );

            const auto start_dir = om_direction::random();
            auto cur_dir = start_dir;

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

void overmap::place_building( const tripoint &p, om_direction::type dir, const city &town )
{
    const tripoint building_pos = p + om_direction::displace( dir );
    const om_direction::type building_dir = om_direction::opposite( dir );

    const int town_dist = ( trig_dist( building_pos.x, building_pos.y, town.pos.x,
                                       town.pos.y ) * 100 ) / std::max( town.size, 1 );

    for( size_t retries = 10; retries > 0; --retries ) {
        const overmap_special_id building_tid = pick_random_building_to_place( town_dist );

        if( can_place_special( *building_tid, building_pos, building_dir, false ) ) {
            place_special( *building_tid, building_pos, building_dir, town, false, false );
            break;
        }
    }
}

void overmap::build_city_street( const overmap_connection &connection, const point &p, int cs,
                                 om_direction::type dir, const city &town, int block_width )
{
    int c = cs;
    int croad = cs;

    if( dir == om_direction::type::invalid ) {
        debugmsg( "Invalid road direction." );
        return;
    }

    const auto street_path = lay_out_street( connection, p, dir, cs + 1 );

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

            build_city_street( connection, iter->pos(), left, om_direction::turn_left( dir ),
                               town, new_width );

            build_city_street( connection, iter->pos(), right, om_direction::turn_right( dir ),
                               town, new_width );

            auto &oter = ter( iter->x, iter->y, 0 );
            // @todo Get rid of the hardcoded terrain ids.
            if( one_in( 2 ) && oter->get_line() == 15 && oter->type_is( oter_type_id( "road" ) ) ) {
                oter = oter_id( "road_nesw_manhole" );
            }
        }
        const tripoint rp( iter->x, iter->y, 0 );

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
        const auto rnd_dir = om_direction::turn_random( dir );
        build_city_street( connection, last_node.pos(), cs, rnd_dir, town );
        if( one_in( 5 ) ) {
            build_city_street( connection, last_node.pos(), cs, om_direction::opposite( rnd_dir ),
                               town, new_width );
        }
    }
}

bool overmap::build_lab( int x, int y, int z, int s, std::vector<point> *lab_train_points,
                         const std::string &prefix, int train_odds )
{
    std::vector<point> generated_lab;
    const oter_id labt( prefix + "lab" );
    const oter_id labt_stairs( labt.id().str() + "_stairs" );
    const oter_id labt_core( labt.id().str() + "_core" );
    const oter_id labt_finale( labt.id().str() + "_finale" );
    const oter_id labt_ants( "ants_lab" );
    const oter_id labt_ants_stairs( "ants_lab_stairs" );

    ter( x, y, z ) = labt;
    generated_lab.push_back( point( x, y ) );

    // maintain a list of potential new lab maps
    // grows outwards from previously placed lab maps
    std::set<point> candidates;
    candidates.insert( {point( x - 1, y ), point( x + 1, y ), point( x, y - 1 ), point( x, y + 1 )} );
    while( !candidates.empty() ) {
        auto cand = candidates.begin();
        const int &cx = cand->x;
        const int &cy = cand->y;
        int dist = abs( x - cx ) + abs( y - cy );
        if( dist <= s * 2 ) { // increase radius to compensate for sparser new algorithm
            int dist_increment = s > 3 ? 3 : 2; // Determines at what distance the odds of placement decreases
            if( one_in( dist / dist_increment + 1 ) ) { // odds diminish farther away from the stairs
                // make an ants lab if it's a basic lab and ants were there before.
                if( prefix.empty() && check_ot_type( "ants", cx, cy, z ) ) {
                    if( ter( cx, cy, z ) != "ants_queen" ) { // skip over a queen's chamber.
                        ter( cx, cy, z ) = labt_ants;
                    }
                } else {
                    ter( cx, cy, z ) = labt;
                }
                generated_lab.push_back( *cand );
                // add new candidates, don't backtrack
                if( ter( cx - 1, cy, z ) != labt && abs( x - cx + 1 ) + abs( y - cy ) > dist ) {
                    candidates.insert( point( cx - 1, cy ) );
                }
                if( ter( cx + 1, cy, z ) != labt && abs( x - cx - 1 ) + abs( y - cy ) > dist ) {
                    candidates.insert( point( cx + 1, cy ) );
                }
                if( ter( cx, cy - 1, z ) != labt && abs( x - cx ) + abs( y - cy + 1 ) > dist ) {
                    candidates.insert( point( cx, cy - 1 ) );
                }
                if( ter( cx, cy + 1, z ) != labt && abs( x - cx ) + abs( y - cy - 1 ) > dist ) {
                    candidates.insert( point( cx, cy + 1 ) );
                }
            }
        }
        candidates.erase( cand );
    }

    bool generate_stairs = true;
    for( auto &elem : generated_lab ) {
        // Use a check for "_stairs" to catch the hidden_lab_stairs tiles.
        if( is_ot_subtype( "_stairs", ter( elem.x, elem.y, z + 1 ) ) ) {
            generate_stairs = false;
        }
    }
    if( generate_stairs && !generated_lab.empty() ) {
        std::shuffle( generated_lab.begin(), generated_lab.end(), rng_get_engine() );

        // we want a spot where labs are above, but we'll settle for the last element if necessary.
        point p;
        for( auto elem : generated_lab ) {
            p = elem;
            if( ter( p.x, p.y, z + 1 ) == labt ) {
                break;
            }
        }
        ter( p.x, p.y, z + 1 ) = labt_stairs;
    }

    ter( x, y, z ) = labt_core;
    int numstairs = 0;
    if( s > 0 ) { // Build stairs going down
        while( !one_in( 6 ) ) {
            int stairx = 0;
            int stairy = 0;
            int tries = 0;
            do {
                stairx = rng( x - s, x + s );
                stairy = rng( y - s, y + s );
                tries++;
            } while( ( ter( stairx, stairy, z ) != labt && ter( stairx, stairy, z ) != labt_ants ) &&
                     tries < 15 );
            if( tries < 15 ) {
                if( ter( stairx, stairy, z ) == labt_ants ) {
                    ter( stairx, stairy, z ) = labt_ants_stairs;
                } else {
                    ter( stairx, stairy, z ) = labt_stairs;
                }
                numstairs++;
            }
        }
    }

    // We need a finale on the bottom of labs.  Central labs have a chance of additional finales.
    if( numstairs == 0 || ( prefix == "central_" && one_in( -z - 1 ) ) ) {
        int finalex = 0;
        int finaley = 0;
        int tries = 0;
        do {
            finalex = rng( x - s, x + s );
            finaley = rng( y - s, y + s );
            tries++;
        } while( tries < 15 && ter( finalex, finaley, z ) != labt
                 && ter( finalex, finaley, z ) != labt_core );
        ter( finalex, finaley, z ) = labt_finale;
    }

    if( train_odds > 0 && one_in( train_odds ) ) {
        int trainx = 0;
        int trainy = 0;
        int tries = 0;
        int adjacent_labs = 0;

        do {
            trainx = rng( x - s * 1.5 - 1, x + s * 1.5 + 1 );
            trainy = rng( y - s * 1.5 - 1, y + s * 1.5 + 1 );
            tries++;

            adjacent_labs = ( is_ot_subtype( "lab", ter( trainx, trainy - 1, z ) ) ? 1 : 0 ) +
                            ( is_ot_subtype( "lab", ter( trainx - 1, trainy, z ) ) ? 1 : 0 ) +
                            ( is_ot_subtype( "lab", ter( trainx, trainy + 1, z ) ) ? 1 : 0 ) +
                            ( is_ot_subtype( "lab", ter( trainx + 1, trainy, z ) ) ? 1 : 0 );
        } while( tries < 50 && (
                     ter( trainx, trainy, z ) == labt ||
                     ter( trainx, trainy, z ) == labt_stairs ||
                     ter( trainx, trainy, z ) == labt_finale ||
                     adjacent_labs != 1 ) );
        if( tries < 50 ) {
            lab_train_points->push_back( point( trainx, trainy ) );
            if( is_ot_subtype( "lab", ter( trainx, trainy - 1, z ) ) ) {
                lab_train_points->push_back( point( trainx, trainy + 1 ) );
            } else if( is_ot_subtype( "lab", ter( trainx, trainy + 1, z ) ) ) {
                lab_train_points->push_back( point( trainx, trainy - 1 ) );
            } else if( is_ot_subtype( "lab", ter( trainx + 1, trainy, z ) ) ) {
                lab_train_points->push_back( point( trainx - 1, trainy ) );
            } else if( is_ot_subtype( "lab", ter( trainx - 1, trainy, z ) ) ) {
                lab_train_points->push_back( point( trainx + 1, trainy ) );
            }
        }
    }

    // 4th story of labs is a candidate for lab escape, as long as there's no train or finale.
    if( prefix.empty() && z == -4 && train_odds == 0 && numstairs > 0 ) {
        int cellx = 0;
        int celly = 0;
        int tries = 0;
        int adjacent_labs = 0;

        // Find a space bordering just one lab to the south.
        do {
            cellx = rng( x - s * 1.5 - 1, x + s * 1.5 + 1 );
            celly = rng( y - s * 1.5 - 1, y + s * 1.5 + 1 );
            tries++;

            adjacent_labs = ( is_ot_subtype( "lab", ter( cellx, celly - 1, z ) ) ? 1 : 0 ) +
                            ( is_ot_subtype( "lab", ter( cellx - 1, celly, z ) ) ? 1 : 0 ) +
                            ( is_ot_subtype( "lab", ter( cellx, celly + 1, z ) ) ? 1 : 0 ) +
                            ( is_ot_subtype( "lab", ter( cellx + 1, celly, z ) ) ? 1 : 0 );
        } while( tries < 50 && (
                     ter( cellx, celly, z ) == labt_stairs ||
                     ter( cellx, celly, z ) == labt_finale ||
                     ter( cellx, celly + 1, z ) != labt ||
                     adjacent_labs != 1 ) );
        if( tries < 50 ) {
            ter( cellx, celly, z ) = oter_id( "lab_escape_cells" );
            ter( cellx, celly + 1, z ) = oter_id( "lab_escape_entrance" );
        }
    }

    return numstairs > 0;
}

void overmap::build_anthill( int x, int y, int z, int s )
{
    for( auto dir : om_direction::all ) {
        build_tunnel( x, y, z, s - rng( 0, 3 ), dir );
    }

    std::vector<point> queenpoints;
    for( int i = x - s; i <= x + s; i++ ) {
        for( int j = y - s; j <= y + s; j++ ) {
            if( check_ot_type( "ants", i, j, z ) ) {
                queenpoints.push_back( point( i, j ) );
            }
        }
    }
    const point target = random_entry( queenpoints );
    ter( target.x, target.y, z ) = oter_id( "ants_queen" );

    const oter_id root_id( "ants_isolated" );

    for( int i = x - s; i <= x + s; i++ ) {
        for( int j = y - s; j <= y + s; j++ ) {
            if( root_id == get_ter( i, j, z )->id ) {
                auto &oter = ter( i, j, z );
                for( auto dir : om_direction::all ) {
                    const point p = point( i, j ) + om_direction::displace( dir );
                    if( check_ot_type( "ants", p.x, p.y, z ) ) {
                        size_t line = oter->get_line();
                        line = om_lines::set_segment( line, dir );
                        if( line != oter->get_line() ) {
                            oter = oter->get_type_id()->get_linear( line );
                        }
                    }
                }
            }
        }
    }
}

void overmap::build_tunnel( int x, int y, int z, int s, om_direction::type dir )
{
    if( s <= 0 ) {
        return;
    }

    const oter_id root_id( "ants_isolated" );
    if( check_ot_type( "ants", x, y, z ) && root_id != get_ter( x, y, z )->id ) {
        return;
    }

    ter( x, y, z ) = oter_id( root_id );

    std::vector<om_direction::type> valid;
    valid.reserve( om_direction::size );
    for( auto r : om_direction::all ) {
        const point p = point( x, y ) + om_direction::displace( r );
        if( !check_ot_type( "ants", p.x, p.y, z ) ) {
            valid.push_back( r );
        }
    }

    const oter_id ants_food( "ants_food" );
    const oter_id ants_larvae( "ants_larvae" );
    const point next = s != 1 ? point( x, y ) + om_direction::displace( dir ) : point( -1, -1 );

    for( auto r : valid ) {
        const point p = point( x, y ) + om_direction::displace( r );

        if( p.x != next.x || p.y != next.y ) {
            if( one_in( s * 2 ) ) {
                // Spawn a special chamber
                if( one_in( 2 ) ) {
                    ter( p.x, p.y, z ) = ants_food;
                } else {
                    ter( p.x, p.y, z ) = ants_larvae;
                }
            } else if( one_in( 5 ) ) {
                // Branch off a side tunnel
                build_tunnel( p.x, p.y, z, s - rng( 1, 3 ), r );
            }
        }
    }
    build_tunnel( next.x, next.y, z, s - 1, dir );
}

bool overmap::build_slimepit( int x, int y, int z, int s )
{
    const oter_id slimepit_down( "slimepit_down" );
    const oter_id slimepit( "slimepit" );

    bool requires_sub = false;
    tripoint origin( x, y, z );
    for( auto p : g->m.points_in_radius( origin, s + z + 1, 0 ) ) {
        int dist = square_dist( x, y, p.x, p.y );
        if( one_in( 2 * dist ) ) {
            chip_rock( p.x, p.y, p.z );
            if( one_in( 8 ) && z > -OVERMAP_DEPTH ) {
                ter( p.x, p.y, p.z ) = slimepit_down;
                requires_sub = true;
            } else {
                ter( p.x, p.y, p.z ) = slimepit;
            }
        }
    }

    return requires_sub;
}

void overmap::build_mine( int x, int y, int z, int s )
{
    bool finale = s <= rng( 1, 3 );
    const oter_id mine( "mine" );
    const oter_id mine_finale_or_down( finale ? "mine_finale" : "mine_down" );
    const oter_id empty_rock( "empty_rock" );

    int built = 0;
    if( s < 2 ) {
        s = 2;
    }
    while( built < s ) {
        ter( x, y, z ) = mine;
        std::vector<point> next;
        for( int i = -1; i <= 1; i += 2 ) {
            if( ter( x, y + i, z ) == empty_rock ) {
                next.push_back( point( x, y + i ) );
            }
            if( ter( x + i, y, z ) == empty_rock ) {
                next.push_back( point( x + i, y ) );
            }
        }
        if( next.empty() ) { // Dead end!  Go down!
            ter( x, y, z ) = mine_finale_or_down;
            return;
        }
        const point p = random_entry( next );
        x = p.x;
        y = p.y;
        built++;
    }
    ter( x, y, z ) = mine_finale_or_down;
}

void overmap::place_rifts( const int z )
{
    int num_rifts = rng( 0, 2 ) * rng( 0, 2 );
    std::vector<point> riftline;
    if( !one_in( 4 ) ) {
        num_rifts++;
    }
    const oter_id hellmouth( "hellmouth" );
    const oter_id rift( "rift" );

    for( int n = 0; n < num_rifts; n++ ) {
        int x = rng( MAX_RIFT_SIZE, OMAPX - MAX_RIFT_SIZE );
        int y = rng( MAX_RIFT_SIZE, OMAPY - MAX_RIFT_SIZE );
        int xdist = rng( MIN_RIFT_SIZE, MAX_RIFT_SIZE ),
            ydist = rng( MIN_RIFT_SIZE, MAX_RIFT_SIZE );
        // We use rng(0, 10) as the t-value for this Bresenham Line, because by
        // repeating this twice, we can get a thick line, and a more interesting rift.
        for( int o = 0; o < 3; o++ ) {
            if( xdist > ydist ) {
                riftline = line_to( x - xdist, y - ydist + o, x + xdist, y + ydist, rng( 0, 10 ) );
            } else {
                riftline = line_to( x - xdist + o, y - ydist, x + xdist, y + ydist, rng( 0, 10 ) );
            }
            for( size_t i = 0; i < riftline.size(); i++ ) {
                if( i == riftline.size() / 2 && !one_in( 3 ) ) {
                    ter( riftline[i].x, riftline[i].y, z ) = hellmouth;
                } else {
                    ter( riftline[i].x, riftline[i].y, z ) = rift;
                }
            }
        }
    }
}

pf::path overmap::lay_out_connection( const overmap_connection &connection, const point &source,
                                      const point &dest, int z, const bool must_be_unexplored ) const
{
    const auto estimate = [&]( const pf::node & cur, const pf::node * prev ) {
        const auto &id( get_ter( cur.x, cur.y, z ) );

        const overmap_connection::subtype *subtype = connection.pick_subtype_for( id );

        if( !subtype ) {
            return pf::rejected;  // No option for this terrain.
        }

        const bool existing_connection = connection.has( id );

        // Only do this check if it needs to be unexplored and there isn't already a connection.
        if( must_be_unexplored && !existing_connection ) {
            // If this must be unexplored, check if we've already got a submap generated.
            const bool existing_submap = is_omt_generated( tripoint( cur.x, cur.y, z ) );

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
            const auto &prev_id( get_ter( prev->x, prev->y, z ) );
            const overmap_connection::subtype *prev_subtype = connection.pick_subtype_for( prev_id );

            if( !prev_subtype || !prev_subtype->allows_turns() ) {
                return pf::rejected;
            }
        }

        const int dx = dest.x - cur.x;
        const int dy = dest.y - cur.y;
        const int dist = subtype->is_orthogonal() ? std::abs( dx ) + std::abs( dy ) : std::sqrt(
                             dx * dx + dy * dy );
        const int existency_mult = existing_connection ? 1 : 5; // Prefer existing connections.

        return existency_mult * dist + subtype->basic_cost;
    };

    return pf::find_path( source, dest, OMAPX, OMAPY, estimate );
}

pf::path overmap::lay_out_street( const overmap_connection &connection, const point &source,
                                  om_direction::type dir, size_t len ) const
{
    const tripoint from( source, 0 );
    // See if we need to make another one "step" further.
    const tripoint en_pos = from + om_direction::displace( dir, len + 1 );
    if( inbounds( en_pos, 1 ) && connection.has( get_ter( en_pos ) ) ) {
        ++len;
    }

    size_t actual_len = 0;

    while( actual_len < len ) {
        const tripoint pos = from + om_direction::displace( dir, actual_len );

        if( !inbounds( pos, 1 ) ) {
            break;  // Don't approach overmap bounds.
        }

        const auto &ter_id( get_ter( pos ) );

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
                const tripoint checkp = pos + tripoint( i, j, 0 );

                if( checkp != pos + om_direction::displace( dir, 1 ) &&
                    checkp != pos + om_direction::displace( om_direction::opposite( dir ), 1 ) &&
                    checkp != pos ) {
                    if( is_ot_type( "road", get_ter( checkp ) ) ) {
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

void overmap::build_connection( const overmap_connection &connection, const pf::path &path, int z )
{
    om_direction::type prev_dir = om_direction::type::invalid;

    for( const auto &node : path.nodes ) {
        const tripoint pos( node.x, node.y, z );
        auto &ter_id( ter( pos ) );
        // @todo: Make 'node' support 'om_direction'.
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

            for( const auto dir : om_direction::all ) {
                const tripoint np( pos + om_direction::displace( dir ) );

                if( inbounds( np ) ) {
                    auto &near_id( ter( np ) );

                    if( connection.has( near_id ) ) {
                        if( near_id->is_linear() ) {
                            const size_t near_line = near_id->get_line();

                            if( om_lines::is_straight( near_line ) || om_lines::has_segment( near_line, new_dir ) ) {
                                // Mutual connection.
                                const size_t new_near_line = om_lines::set_segment( near_line, om_direction::opposite( dir ) );
                                near_id = near_id->get_type_id()->get_linear( new_near_line );
                                new_line = om_lines::set_segment( new_line, dir );
                            }
                        } else if( near_id->is_rotatable() && om_direction::are_parallel( dir, near_id->get_dir() ) ) {
                            new_line = om_lines::set_segment( new_line, dir );
                        }
                    }
                } else {
                    // Always connect to outbound tiles.
                    new_line = om_lines::set_segment( new_line, dir );
                }
            }

            if( new_line == om_lines::invalid ) {
                debugmsg( "Invalid path for connection \"%s\".", connection.id.c_str() );
                return;
            }

            ter_id = subtype->terrain->get_linear( new_line );
        } else if( new_dir != om_direction::type::invalid ) {
            ter_id = subtype->terrain->get_rotated( new_dir );
        }

        prev_dir = new_dir;
    }
}

void overmap::build_connection( const point &source, const point &dest, int z,
                                const overmap_connection &connection, const bool must_be_unexplored )
{
    build_connection( connection, lay_out_connection( connection, source, dest, z, must_be_unexplored ),
                      z );
}

void overmap::connect_closest_points( const std::vector<point> &points, int z,
                                      const overmap_connection &connection )
{
    if( points.size() == 1 ) {
        return;
    }
    for( size_t i = 0; i < points.size(); ++i ) {
        int closest = -1;
        int k = 0;
        for( size_t j = i + 1; j < points.size(); j++ ) {
            const int distance = trig_dist( points[i].x, points[i].y, points[j].x, points[j].y );
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
            good_river( x, y, 0 );
        }
    }
}

// Changes neighboring empty rock to partial rock
void overmap::chip_rock( int x, int y, int z )
{
    const oter_id rock( "rock" );
    const oter_id empty_rock( "empty_rock" );

    if( ter( x - 1, y, z ) == empty_rock ) {
        ter( x - 1, y, z ) = rock;
    }

    if( ter( x + 1, y, z ) == empty_rock ) {
        ter( x + 1, y, z ) = rock;
    }

    if( ter( x, y - 1, z ) == empty_rock ) {
        ter( x, y - 1, z ) = rock;
    }

    if( ter( x, y + 1, z ) == empty_rock ) {
        ter( x, y + 1, z ) = rock;
    }
}

bool overmap::check_ot_type( const std::string &otype, int x, int y, int z ) const
{
    const oter_id oter = get_ter( x, y, z );
    return is_ot_type( otype, oter );
}

bool overmap::check_ot_subtype( const std::string &otype, int x, int y, int z ) const
{
    const oter_id oter = get_ter( x, y, z );
    return is_ot_subtype( otype.c_str(), oter );
}

bool overmap::check_overmap_special_type( const overmap_special_id &id,
        const tripoint &location ) const
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

void overmap::good_river( int x, int y, int z )
{
    if( !is_ot_type( "river", get_ter( x, y, z ) ) ) {
        return;
    }
    if( ( x == 0 ) || ( x == OMAPX - 1 ) ) {
        if( !is_river( ter( x, y - 1, z ) ) ) {
            ter( x, y, z ) = oter_id( "river_north" );
        } else if( !is_river( ter( x, y + 1, z ) ) ) {
            ter( x, y, z ) = oter_id( "river_south" );
        } else {
            ter( x, y, z ) = oter_id( "river_center" );
        }
        return;
    }
    if( ( y == 0 ) || ( y == OMAPY - 1 ) ) {
        if( !is_river( ter( x - 1, y, z ) ) ) {
            ter( x, y, z ) = oter_id( "river_west" );
        } else if( !is_river( ter( x + 1, y, z ) ) ) {
            ter( x, y, z ) = oter_id( "river_east" );
        } else {
            ter( x, y, z ) = oter_id( "river_center" );
        }
        return;
    }
    if( is_river( ter( x - 1, y, z ) ) ) {
        if( is_river( ter( x, y - 1, z ) ) ) {
            if( is_river( ter( x, y + 1, z ) ) ) {
                if( is_river( ter( x + 1, y, z ) ) ) {
                    // River on N, S, E, W;
                    // but we might need to take a "bite" out of the corner
                    if( !is_river( ter( x - 1, y - 1, z ) ) ) {
                        ter( x, y, z ) = oter_id( "river_c_not_nw" );
                    } else if( !is_river( ter( x + 1, y - 1, z ) ) ) {
                        ter( x, y, z ) = oter_id( "river_c_not_ne" );
                    } else if( !is_river( ter( x - 1, y + 1, z ) ) ) {
                        ter( x, y, z ) = oter_id( "river_c_not_sw" );
                    } else if( !is_river( ter( x + 1, y + 1, z ) ) ) {
                        ter( x, y, z ) = oter_id( "river_c_not_se" );
                    } else {
                        ter( x, y, z ) = oter_id( "river_center" );
                    }
                } else {
                    ter( x, y, z ) = oter_id( "river_east" );
                }
            } else {
                if( is_river( ter( x + 1, y, z ) ) ) {
                    ter( x, y, z ) = oter_id( "river_south" );
                } else {
                    ter( x, y, z ) = oter_id( "river_se" );
                }
            }
        } else {
            if( is_river( ter( x, y + 1, z ) ) ) {
                if( is_river( ter( x + 1, y, z ) ) ) {
                    ter( x, y, z ) = oter_id( "river_north" );
                } else {
                    ter( x, y, z ) = oter_id( "river_ne" );
                }
            } else {
                if( is_river( ter( x + 1, y, z ) ) ) { // Means it's swampy
                    ter( x, y, z ) = oter_id( "forest_water" );
                }
            }
        }
    } else {
        if( is_river( ter( x, y - 1, z ) ) ) {
            if( is_river( ter( x, y + 1, z ) ) ) {
                if( is_river( ter( x + 1, y, z ) ) ) {
                    ter( x, y, z ) = oter_id( "river_west" );
                } else { // Should never happen
                    ter( x, y, z ) = oter_id( "forest_water" );
                }
            } else {
                if( is_river( ter( x + 1, y, z ) ) ) {
                    ter( x, y, z ) = oter_id( "river_sw" );
                } else { // Should never happen
                    ter( x, y, z ) = oter_id( "forest_water" );
                }
            }
        } else {
            if( is_river( ter( x, y + 1, z ) ) ) {
                if( is_river( ter( x + 1, y, z ) ) ) {
                    ter( x, y, z ) = oter_id( "river_nw" );
                } else { // Should never happen
                    ter( x, y, z ) = oter_id( "forest_water" );
                }
            } else { // Should never happen
                ter( x, y, z ) = oter_id( "forest_water" );
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
    if( dir == type::invalid ) {
        debugmsg( "Invalid direction cannot have an id." );
    }
    return ids[static_cast<size_t>( dir ) + 1];
}

const std::string &om_direction::name( type dir )
{
    static const std::array < std::string, size + 1 > names = {{
            _( "invalid" ), _( "north" ), _( "east" ), _( "south" ), _( "west" )
        }
    };
    return names[static_cast<size_t>( dir ) + 1];
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
        case type::invalid:
            debugmsg( "Invalid overmap rotation (%d).", static_cast<int>( dir ) );
        // Intentional fallthrough.
        case type::north:
            break;  // No need to do anything.
        case type::east:
            return point( -p.y, p.x );
        case type::south:
            return point( -p.x, -p.y );
        case type::west:
            return point( p.y, -p.x );
    }
    return p;
}

tripoint om_direction::rotate( const tripoint &p, type dir )
{
    return tripoint( rotate( { p.x, p.y }, dir ), p.z );
}

long om_direction::rotate_symbol( long sym, type dir )
{
    return rotatable_symbols::get( sym, static_cast<int>( dir ) );
}

point om_direction::displace( type dir, int dist )
{
    return rotate( { 0, -dist }, dir );
}

inline om_direction::type rotate_internal( om_direction::type dir, int step )
{
    using namespace om_direction;
    if( dir == type::invalid ) {
        debugmsg( "Can't rotate an invalid overmap rotation." );
        return dir;
    }
    step = step % size;
    if( step < 0 ) {
        step += size;
    }
    return static_cast<type>( ( static_cast<int>( dir ) + step ) % size );
}

om_direction::type om_direction::add( type dir1, type dir2 )
{
    return rotate_internal( dir1, static_cast<int>( dir2 ) );
}

om_direction::type om_direction::turn_left( type dir )
{
    return rotate_internal( dir, -int( size ) / 4 );
}

om_direction::type om_direction::turn_right( type dir )
{
    return rotate_internal( dir, int( size ) / 4 );
}

om_direction::type om_direction::turn_random( type dir )
{
    return rng( 0, 1 ) ? turn_left( dir ) : turn_right( dir );
}

om_direction::type om_direction::opposite( type dir )
{
    return rotate_internal( dir, int( size ) / 2 );
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
        const tripoint &p, const bool must_be_unexplored ) const
{
    std::vector<om_direction::type> rotations( om_direction::size );
    const auto first = rotations.begin();
    auto last = first;

    int top_score = 0; // Maximal number of existing connections (roads).
    // Try to find the most suitable rotation: satisfy as many connections as possible with the existing terrain.
    for( auto r : om_direction::all ) {
        int score = 0; // Number of existing connections when rotated by 'r'.
        bool valid = true;

        for( const auto &con : special.connections ) {
            const tripoint rp = p + om_direction::rotate( con.p, r );
            const oter_id &oter = get_ter( rp );

            if( is_ot_type( con.terrain.str(), oter ) ) {
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

bool overmap::can_place_special( const overmap_special &special, const tripoint &p,
                                 om_direction::type dir, const bool must_be_unexplored ) const
{
    assert( p != invalid_tripoint );
    assert( dir != om_direction::type::invalid );

    if( !special.id ) {
        return false;
    }

    return std::all_of( special.terrains.begin(),
    special.terrains.end(), [&]( const overmap_special_terrain & elem ) {
        const tripoint rp = p + om_direction::rotate( elem.p, dir );

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

        const oter_id tid = get_ter( rp );

        if( rp.z == 0 ) {
            return special.can_be_placed_on( tid );
        } else {
            return tid == get_default_terrain( rp.z );
        }
    } );
}

// checks around the selected point to see if the special can be placed there
void overmap::place_special( const overmap_special &special, const tripoint &p,
                             om_direction::type dir, const city &cit, const bool must_be_unexplored, const bool force )
{
    assert( p != invalid_tripoint );
    assert( dir != om_direction::type::invalid );
    if( !force ) {
        assert( can_place_special( special, p, dir, must_be_unexplored ) );
    }

    const bool blob = special.flags.count( "BLOB" ) > 0;

    for( const auto &elem : special.terrains ) {
        const tripoint location = p + om_direction::rotate( elem.p, dir );
        const oter_id tid = elem.terrain->get_rotated( dir );

        overmap_special_placements[location] = special.id;
        ter( location.x, location.y, location.z ) = tid;

        if( blob ) {
            for( int x = -2; x <= 2; x++ ) {
                for( int y = -2; y <= 2; y++ ) {
                    auto &cur_ter = ter( location.x + x, location.y + y, location.z );
                    if( one_in( 1 + abs( x ) + abs( y ) ) && special.can_be_placed_on( cur_ter ) ) {
                        cur_ter = tid;
                    }
                }
            }
        }
    }
    // Make connections.
    if( cit ) {
        for( const auto &elem : special.connections ) {
            if( elem.connection ) {
                const tripoint rp( p + om_direction::rotate( elem.p, dir ) );
                build_connection( cit.pos, point( rp.x, rp.y ), elem.p.z, *elem.connection, must_be_unexplored );
            }
        }
    }
    // Place spawns.
    if( special.spawns.group ) {
        const overmap_special_spawns &spawns = special.spawns;
        const int pop = rng( spawns.population.min, spawns.population.max );
        const int rad = rng( spawns.radius.min, spawns.radius.max );
        add_mon_group( mongroup( spawns.group, p.x * 2, p.y * 2, p.z, rad, pop ) );
    }
    // Place basement for houses.
    if( special.id == "FakeSpecial_house" && one_in( settings.city_spec.house_basement_chance ) ) {
        const overmap_special_id basement_tid = settings.city_spec.pick_basement();
        const tripoint basement_p = tripoint( p.x, p.y, p.z - 1 );

        // This basement isn't part of the special that we asserted we could place at
        // the top of this function, so we need to make sure we can place the basement
        // special before doing so.
        if( can_place_special( *basement_tid, basement_p, dir, must_be_unexplored ) || force ) {
            place_special( *basement_tid, basement_p, dir, cit, force, must_be_unexplored );
        }
    }
}

om_special_sectors get_sectors( const int sector_width )
{
    std::vector<point> res;

    res.reserve( ( OMAPX / sector_width ) * ( OMAPY / sector_width ) );
    for( int x = 0; x < OMAPX; x += sector_width ) {
        for( int y = 0; y < OMAPY; y += sector_width ) {
            res.emplace_back( x, y );
        }
    }
    std::shuffle( res.begin(), res.end(), rng_get_engine() );
    return { res, sector_width };
}

bool overmap::place_special_attempt( overmap_special_batch &enabled_specials,
                                     const point &sector, const int sector_width, const bool place_optional,
                                     const bool must_be_unexplored )
{
    const int x = sector.x;
    const int y = sector.y;

    const tripoint p( rng( x, x + sector_width - 1 ), rng( y, y + sector_width - 1 ), 0 );
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
        const auto rotation = random_special_rotation( special, p, must_be_unexplored );
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
    for( auto iter = enabled_specials.begin(); iter != enabled_specials.end(); ) {
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
    // place them on adajacent uncreated overmaps.
    if( std::any_of( custom_overmap_specials.begin(), custom_overmap_specials.end(),
    []( overmap_special_placement placement ) {
    return placement.instances_placed <
           placement.special_details->occurrences.min;
} ) ) {
        // Randomly select from among the nearest uninitialized overmap positions.
        int previous_distance = 0;
        std::vector<point> nearest_candidates;
        // Since this starts at enabled_specials::origin, it will only place new overmaps
        // in the 5x5 area surrounding the initial overmap, bounding the amount of work we will do.
        for( point candidate_addr : closest_points_first( 2, custom_overmap_specials.get_origin() ) ) {
            if( !overmap_buffer.has( candidate_addr.x, candidate_addr.y ) ) {
                int current_distance = square_dist( pos().x, pos().y,
                                                    candidate_addr.x, candidate_addr.y );
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
            point new_om_addr = nearest_candidates.front();
            overmap_buffer.create_custom_overmap( new_om_addr.x, new_om_addr.y, custom_overmap_specials );
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
    std::map<overmap_special_id, int>::iterator iter;
    for( auto &elem : custom_overmap_specials ) {
        processed_specials[elem.special_details->id] = elem.instances_placed;
    }

    // Loop through the specials we started with.
    for( auto it = enabled_specials.begin(); it != enabled_specials.end(); ) {
        // Determine if this special is still in our callee's list of specials...
        iter = processed_specials.find( it->special_details->id );
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
    for( auto &elem : cities ) {
        if( get_option<bool>( "WANDER_SPAWNS" ) ) {
            if( !one_in( 16 ) || elem.size > 5 ) {
                mongroup m( mongroup_id( "GROUP_ZOMBIE" ), ( elem.pos.x * 2 ), ( elem.pos.y * 2 ), 0,
                            int( elem.size * 2.5 ),
                            elem.size * 80 );
                //                m.set_target( zg.back().posx, zg.back().posy );
                m.horde = true;
                m.wander( *this );
                add_mon_group( m );
            }
        }
    }

    if( !get_option<bool>( "CLASSIC_ZOMBIES" ) ) {
        // Figure out where swamps are, and place swamp monsters
        for( int x = 3; x < OMAPX - 3; x += 7 ) {
            for( int y = 3; y < OMAPY - 3; y += 7 ) {
                int swamp_count = 0;
                for( int sx = x - 3; sx <= x + 3; sx++ ) {
                    for( int sy = y - 3; sy <= y + 3; sy++ ) {
                        if( ter( sx, sy, 0 ) == "forest_water" ) {
                            swamp_count += 2;
                        }
                    }
                }
                if( swamp_count >= 25 )
                    add_mon_group( mongroup( mongroup_id( "GROUP_SWAMP" ), x * 2, y * 2, 0, 3,
                                             rng( swamp_count * 8, swamp_count * 25 ) ) );
            }
        }
    }

    if( !get_option<bool>( "CLASSIC_ZOMBIES" ) ) {
        // Figure out where rivers are, and place swamp monsters
        for( int x = 3; x < OMAPX - 3; x += 7 ) {
            for( int y = 3; y < OMAPY - 3; y += 7 ) {
                int river_count = 0;
                for( int sx = x - 3; sx <= x + 3; sx++ ) {
                    for( int sy = y - 3; sy <= y + 3; sy++ ) {
                        if( is_river( ter( sx, sy, 0 ) ) ) {
                            river_count++;
                        }
                    }
                }
                if( river_count >= 25 )
                    add_mon_group( mongroup( mongroup_id( "GROUP_RIVER" ), x * 2, y * 2, 0, 3,
                                             rng( river_count * 8, river_count * 25 ) ) );
            }
        }
    }

    if( !get_option<bool>( "CLASSIC_ZOMBIES" ) ) {
        // Place the "put me anywhere" groups
        int numgroups = rng( 0, 3 );
        for( int i = 0; i < numgroups; i++ ) {
            add_mon_group( mongroup( mongroup_id( "GROUP_WORM" ), rng( 0, OMAPX * 2 - 1 ), rng( 0,
                                     OMAPY * 2 - 1 ), 0,
                                     rng( 20, 40 ), rng( 30, 50 ) ) );
        }
    }
}

point overmap::global_base_point() const
{
    return point( loc.x * OMAPX, loc.y * OMAPY );
}

void overmap::place_radios()
{
    std::string message;
    for( int i = 0; i < OMAPX; i++ ) {
        for( int j = 0; j < OMAPY; j++ ) {
            if( ter( i, j, 0 ) == "radio_tower" ) {
                int choice = rng( 0, 2 );
                switch( choice ) {
                    case 0:
                        message = string_format( _( "This is emergency broadcast station %d%d.\
  Please proceed quickly and calmly to your designated evacuation point." ), i, j );
                        radios.push_back( radio_tower( i * 2, j * 2, rng( RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH ),
                                                       message ) );
                        break;
                    case 1:
                        radios.push_back( radio_tower( i * 2, j * 2, rng( RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH ),
                                                       _( "Head West.  All survivors, head West.  Help is waiting." ) ) );
                        break;
                    case 2:
                        radios.push_back( radio_tower( i * 2, j * 2, rng( RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH ), "",
                                                       WEATHER_RADIO ) );
                        break;
                }
            } else if( ter( i, j, 0 ) == "lmoe" ) {
                message = string_format( _( "This is automated emergency shelter beacon %d%d.\
  Supplies, amenities and shelter are stocked." ), i, j );
                radios.push_back( radio_tower( i * 2, j * 2, rng( RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH ) / 2,
                                               message ) );
            } else if( ter( i, j, 0 ) == "fema_entrance" ) {
                message = string_format( _( "This is FEMA camp %d%d.\
  Supplies are limited, please bring supplemental food, water, and bedding.\
  This is FEMA camp %d%d.  A designated long-term emergency shelter." ), i, j, i, j );
                radios.push_back( radio_tower( i * 2, j * 2, rng( RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH ),
                                               message ) );
            }
        }
    }
}

void overmap::open( overmap_special_batch &enabled_specials )
{
    const std::string plrfilename = overmapbuffer::player_filename( loc.x, loc.y );
    const std::string terfilename = overmapbuffer::terrain_filename( loc.x, loc.y );

    using namespace std::placeholders;
    if( read_from_file_optional( terfilename, std::bind( &overmap::unserialize, this, _1 ) ) ) {
        read_from_file_optional( plrfilename, std::bind( &overmap::unserialize_view, this, _1 ) );
    } else { // No map exists!  Prepare neighbors, and generate one.
        std::vector<const overmap *> pointers;
        // Fetch south and north
        for( int i = -1; i <= 1; i += 2 ) {
            pointers.push_back( overmap_buffer.get_existing( loc.x, loc.y + i ) );
        }
        // Fetch east and west
        for( int i = -1; i <= 1; i += 2 ) {
            pointers.push_back( overmap_buffer.get_existing( loc.x + i, loc.y ) );
        }

        // pointers looks like (north, south, west, east)
        generate( pointers[0], pointers[3], pointers[1], pointers[2], enabled_specials );
    }
}

// Note: this may throw io errors from std::ofstream
void overmap::save() const
{
    const std::string plrfilename = overmapbuffer::player_filename( loc.x, loc.y );
    const std::string terfilename = overmapbuffer::terrain_filename( loc.x, loc.y );

    ofstream_wrapper fout_player( plrfilename );
    serialize_view( fout_player );
    fout_player.close();

    ofstream_wrapper_exclusive fout_terrain( terfilename );
    serialize( fout_terrain );
    fout_terrain.close();
}

void overmap::add_mon_group( const mongroup &group )
{
    // Monster groups: the old system had large groups (radius > 1),
    // the new system transforms them into groups of radius 1, this also
    // makes the diffuse setting obsolete (as it only controls how the radius
    // is interpreted) - it's only used when adding monster groups with function.
    if( group.radius == 1 ) {
        zg.insert( std::pair<tripoint, mongroup>( group.pos, group ) );
        return;
    }
    // diffuse groups use a circular area, non-diffuse groups use a rectangular area
    const int rad = std::max<int>( 0, group.radius );
    const double total_area = group.diffuse ? std::pow( rad + 1, 2 ) : ( rad * rad * M_PI + 1 );
    const double pop = std::max<int>( 0, group.population );
    int xpop = 0;
    for( int x = -rad; x <= rad; x++ ) {
        for( int y = -rad; y <= rad; y++ ) {
            const int dist = group.diffuse ? square_dist( x, y, 0, 0 ) : trig_dist( x, y, 0, 0 );
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
            tmp.pos.x += x;
            tmp.pos.y += y;
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

void overmap::for_each_npc( const std::function<void( npc & )> callback )
{
    for( auto &guy : npcs ) {
        callback( *guy );
    }
}

void overmap::for_each_npc( const std::function<void( const npc & )> callback ) const
{
    for( auto &guy : npcs ) {
        callback( *guy );
    }
}

std::shared_ptr<npc> overmap::find_npc( const int id ) const
{
    for( const auto &guy : npcs ) {
        if( guy->getID() == id ) {
            return guy;
        }
    }
    return nullptr;
}

bool overmap::is_omt_generated( const tripoint &loc ) const
{
    if( !inbounds( loc ) ) {
        return false;
    }

    // Location is local to this overmap, but we need global submap coordinates
    // for the mapbuffer lookup.
    tripoint global_sm_loc = omt_to_sm_copy( loc ) + om_to_sm_copy( tripoint( pos().x, pos().y,
                             loc.z ) );

    const bool is_generated = MAPBUFFER.lookup_submap( global_sm_loc ) != nullptr;

    return is_generated;
}

overmap_special_id overmap_specials::create_building_from( const string_id<oter_type_t> &base )
{
    // @todo: Get rid of the hard-coded ids.
    static const string_id<overmap_location> land( "land" );
    static const string_id<overmap_location> swamp( "swamp" );

    overmap_special new_special;

    new_special.id = overmap_special_id( "FakeSpecial_" + base.str() );
    new_special.locations.insert( land );
    new_special.locations.insert( swamp );

    overmap_special_terrain ter;
    ter.terrain = base.obj().get_first().id();
    new_special.terrains.push_back( ter );

    return specials.insert( new_special ).id;
}

constexpr tripoint overmap::invalid_tripoint;
