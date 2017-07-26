#include "overmap.h"

#include "coordinate_conversions.h"
#include "generic_factory.h"
#include "overmap_types.h"
#include "rng.h"
#include "line.h"
#include "game.h"
#include "npc.h"
#include "debug.h"
#include "cursesdef.h"
#include "options.h"
#include "catacharset.h"
#include "overmapbuffer.h"
#include "action.h"
#include "input.h"
#include "json.h"
#include "mapdata.h"
#include "mapgen.h"
#include "cata_utility.h"
#include "uistate.h"
#include "mongroup.h"
#include "mtype.h"
#include "name.h"
#include "simple_pathfinding.h"
#include "translations.h"
#include "mapgen_functions.h"
#include "clzones.h"
#include "weather_gen.h"
#include "weather.h"
#include "ui.h"
#include "mapbuffer.h"
#include "map_iterator.h"
#include "messages.h"
#include "rotatable_symbols.h"
#include "string_input_popup.h"

#include <cassert>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <vector>
#include <sstream>
#include <cstring>
#include <ostream>
#include <algorithm>
#include <numeric>

#define dbg(x) DebugLog((DebugLevel)(x),D_MAP_GEN) << __FILE__ << ":" << __LINE__ << ": "

#define STREETCHANCE 2
#define MIN_ANT_SIZE 8
#define MAX_ANT_SIZE 20
#define MIN_GOO_SIZE 1
#define MAX_GOO_SIZE 2
#define MIN_RIFT_SIZE 6
#define MAX_RIFT_SIZE 16

const efftype_id effect_pet( "pet" );

using oter_type_id = int_id<oter_type_t>;
using oter_type_str_id = string_id<oter_type_t>;

#include "omdata.h"
////////////////
oter_id  ot_null,
         ot_crater,
         ot_field,
         ot_forest,
         ot_forest_thick,
         ot_forest_water,
         ot_river_center;

struct overmap_special_location {
    public:
        overmap_special_location( const std::vector<std::string> &allowed,
                                  const std::vector<std::string> &disallowed ) {
            const auto convert = []( const std::string &str ) {
                return string_id<oter_type_t>( str );
            };

            std::transform( allowed.begin(), allowed.end(), std::back_inserter( this->allowed ), convert );
            std::transform( disallowed.begin(), disallowed.end(), std::back_inserter( this->disallowed ), convert );
        }

        // Test if oter meets the terrain restrictions.
        bool test( const oter_id &oter ) const {
            const auto matches = [ &oter ]( const string_id<oter_type_t> &type ) {
                return oter->type_is( *type );
            };

            if( !allowed.empty() && std::none_of( allowed.begin(), allowed.end(), matches ) ) {
                return false;
            }

            return std::none_of( disallowed.begin(), disallowed.end(), matches );
        }

    private:
        std::vector<string_id<oter_type_t>> allowed;
        std::vector<string_id<oter_type_t>> disallowed;
};

// Map of allowed and disallowed terrain for locations of overmap specials.
// Format: { location, { { list of allowed terrains }, { list of disallowed terrains } } }
// @todo Jsonize this map.
static const std::map<std::string, overmap_special_location> special_locations = {
    { "field",      { { "field"            }, {                  } } },
    { "forest",     { { "forest"           }, {                  } } },
    { "swamp",      { { "forest_water"     }, {                  } } },
    { "land",       { {                    }, { "river" , "road" } } },
    { "water",      { { "river"            }, {                  } } },
    { "wilderness", { { "forest" , "field" }, {                  } } }
};

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
}};

const std::array<type, 1 + om_direction::bits> all = {{
    {         0, 4, "_isolated"  },   // 0  ----
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
}};

const size_t size = all.size();

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

}

//const regional_settings default_region_settings;
t_regional_settings_map region_settings_map;

namespace
{

generic_factory<oter_type_t> terrain_types( "overmap terrain type" );
generic_factory<oter_t> terrains( "overmap terrain" );

generic_factory<overmap_special> specials( "overmap special" );

}

static const std::map<std::string, oter_flags> oter_flags_map = {
    { "KNOWN_DOWN",     known_down     },
    { "KNOWN_UP",       known_up       },
    { "RIVER",          river_tile     },
    { "SIDEWALK",       has_sidewalk   },
    { "ALLOW_OVERRIDE", allow_override },
    { "NO_ROTATE",      no_rotate      },
    { "LINEAR",         line_drawing   }
};

/*
 * Temporary container id_or_id. Stores str for delayed lookup and conversion.
 */
struct sid_or_sid {
   std::string primary_str;   // 32
   std::string secondary_str; // 64
   int chance;                // 68
   sid_or_sid(const std::string & s1, const int i, const::std::string s2) : primary_str(s1), secondary_str(s2), chance(i) { }
};

city::city( int const X, int const Y, int const S)
: x (X)
, y (Y)
, s (S)
, name( Name::get( nameIsTownName ) )
{
}

int city::get_distance_from( const tripoint &p ) const
{
    return std::max( int( trig_dist( p, { x, y, 0 } ) ) - s, 0 );
}

std::map<enum radio_type, std::string> radio_type_names =
{{ {MESSAGE_BROADCAST, "broadcast"}, {WEATHER_RADIO, "weather"} }};

/** @relates string_id */
template<>
bool string_id<oter_type_t>::is_valid() const
{
    return terrain_types.is_valid( *this );
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

void set_oter_ids()   // fixme constify
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

void overmap_specials::load( JsonObject &jo, const std::string &src )
{
    specials.load( jo, src );
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
    const size_t actual_count = std::accumulate(  specials.get_all().begin(), specials.get_all().end(), static_cast< size_t >( 0 ),
    []( size_t sum, const overmap_special &elem ) {
        return sum + ( elem.flags.count( "UNIQUE" ) == ( size_t )0 ? ( size_t )std::max( elem.occurrences.min, 0 ) : ( size_t )1 );
    } );

    if( actual_count > max_count ) {
        debugmsg( "There are too many mandatory overmap specials (%d > %d). Some of them may not be placed.", actual_count, max_count );
    }

    specials.check();
}

void overmap_specials::reset()
{
    specials.reset();
}

bool is_river(const oter_id &ter)
{
    // if the id starts with "river" or "bridge", count as a river, but this
    // is done in data init.
    // return (ter.compare(0,5,"river",5) == 0 || ter.compare(0,6,"bridge",6) == 0);
    return ter->has_flag( river_tile );
}

bool is_ot_type(const std::string &otype, const oter_id &oter)
{
    const size_t oter_size = oter.id().str().size();
    const size_t compare_size = otype.size();
    if (compare_size > oter_size) {
        return false;
    }

    const auto &oter_str = oter.id();
    if (oter_str.str().compare(0, compare_size, otype) != 0) {
        return false;
    }

    // check if it's a full match
    if (compare_size == oter_size) {
        return true;
    }

    // only ok for partial if next char is an underscore
    return oter_str.str()[compare_size] == '_';
}

bool road_allowed(const oter_id &ter)
{
    return ter->has_flag( allow_override );
}

oter_id overmap::random_shop() const
{
    return settings.city_spec.pick_shop();
}

oter_id overmap::random_park() const
{
    return settings.city_spec.pick_park();
}

oter_id overmap::random_house() const
{
    const oter_id house( "house_north" );
    const oter_id house_base( "house_base_north" );

    return one_in( settings.house_basement_chance) ? house : house_base;
}

/*
 * load mapgen functions from an overmap_terrain json entry
 * suffix is for roads/subways/etc which have "_straight", "_curved", "_tee", "_four_way" function mappings
 */
void load_overmap_terrain_mapgens(JsonObject &jo, const std::string id_base,
                                  const std::string suffix = "")
{
    const std::string fmapkey(id_base + suffix);
    const std::string jsonkey("mapgen" + suffix);
    bool default_mapgen = jo.get_bool("default_mapgen", true);
    int default_idx = -1;
    if ( default_mapgen ) {
        auto const iter = mapgen_cfunction_map.find( fmapkey );
        if ( iter != mapgen_cfunction_map.end() ) {
            oter_mapgen[fmapkey].push_back( new mapgen_function_builtin( iter->second ) );
            default_idx = oter_mapgen[fmapkey].size() - 1;
        }
    }
    if ( jo.has_array( jsonkey ) ) {
        JsonArray ja = jo.get_array( jsonkey );
        int c = 0;
        while ( ja.has_more() ) {
            if ( ja.has_object(c) ) {
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

    optional( jo, was_loaded, "color", color, color_reader{} );

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
    sym( type.sym ) {}

oter_t::oter_t( const oter_type_t &type, om_direction::type dir ) :
    type( &type ),
    id( type.id.str() + "_" + om_direction::id( dir ) ),
    dir( dir ),
    sym( om_direction::rotate_symbol( type.sym, dir ) ) {}

oter_t::oter_t( const oter_type_t &type, size_t line ) :
    type( &type ),
    id( type.id.str() + om_lines::all[line].suffix ),
    sym( om_lines::all[line].sym ),
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

inline bool oter_t::type_is( const int_id<oter_type_t> &type_id ) const
{
    return type->id.id() == type_id;
}

inline bool oter_t::type_is( const oter_type_t &type ) const
{
    return this->type == &type;
}

bool oter_t::can_connect_to( const int_id<oter_t> &oter ) const
{
    // @todo JSONize.
    const auto is_road = []( const int_id<oter_t> &oter ) {
        return is_ot_type( "road",   oter ) ||
               is_ot_type( "bridge", oter ) ||
               is_ot_type( "hiway",  oter );
    };

    return is_road( id ) ? is_road( oter ) : type_is( *oter->type );
}

bool oter_t::has_connection( om_direction::type dir ) const
{
    // @todo It's a DAMN UGLY hack. Remove it as soon as possible.
    static const oter_str_id road_manhole( "road_nesw_manhole" );
    if( id == road_manhole ) {
        return true;
    }
    return om_lines::has_segment( line, dir );
}

bool oter_t::is_hardcoded() const
{
    // @todo This set only exists because so does the monstrous 'if-else' statement in @ref map::draw_map(). Get rid of both.
    static const std::set<std::string> hardcoded_mapgen = {
        "anthill",
        "fema",
        "fema_entrance",
        "haz_sar",
        "haz_sar_b1",
        "haz_sar_entrance",
        "haz_sar_entrance_b1",
        "hospital",
        "hospital_entrance",
        "ice_lab",
        "ice_lab_stairs",
        "ice_lab_core",
        "ice_lab_finale",
        "lab",
        "lab_core",
        "lab_stairs",
        "lab_finale",
        "looted_building",  // pseudo-terrain
        "mansion",
        "mansion_entrance",
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
        "prison_1",
        "prison_2",
        "prison_3",
        "prison_4",
        "prison_5",
        "prison_6",
        "prison_7",
        "prison_8",
        "prison_9",
        "prison_b",
        "prison_b_entrance",
        "radio_tower",
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
        "station_radio",
        "temple",
        "temple_finale",
        "temple_stairs",
        "toxic_dump",
        "triffid_finale",
        "triffid_roots",
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
            debugmsg( "Invalid monster group \"%s\" in spawns of \"%s\".", elem.static_spawns.group.c_str(), elem.id.c_str() );
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
                debugmsg( "Mapgen terrain \"%s\" exists in both JSON and a hardcoded function. Consider removing the latter.", mid.c_str() );
            }
        } else if( !exists_hardcoded ) {
            debugmsg( "No mapgen terrain exists for \"%s\".", mid.c_str() );
        }
    }
}

void overmap_terrains::finalize()
{

    for( const auto &elem : terrain_types.get_all() ) {
        const_cast<oter_type_t &>( elem ).finalize(); // This cast is ugly, but safe.
    }

    if ( region_settings_map.find("default") == region_settings_map.end() ) {
        debugmsg("ERROR: can't find default overmap settings (region_map_settings 'default'),"
                 " cataclysm pending. And not the fun kind.");
    }

    for( auto &elem : region_settings_map ) {
        elem.second.setup();
    }

    set_oter_ids();
}

void overmap_terrains::reset()
{
    terrain_types.reset();
    terrains.reset();
}

size_t overmap_terrains::count()
{
    return terrains.size();
}

void load_region_settings( JsonObject &jo )
{
    regional_settings new_region;
    if ( ! jo.read("id", new_region.id) ) {
        jo.throw_error("No 'id' field.");
    }
    bool strict = ( new_region.id == "default" );
    if ( ! jo.read("default_oter", new_region.default_oter) && strict ) {
        jo.throw_error("default_oter required for default ( though it should probably remain 'field' )");
    }
    if ( jo.has_object("default_groundcover") ) {
        JsonObject jio = jo.get_object("default_groundcover");
        new_region.default_groundcover_str.reset( new sid_or_sid("t_grass", 4, "t_dirt") );
        if ( ! jio.read("primary", new_region.default_groundcover_str->primary_str) ||
             ! jio.read("secondary", new_region.default_groundcover_str->secondary_str) ||
             ! jio.read("ratio", new_region.default_groundcover.chance) ) {
            jo.throw_error("'default_groundcover' missing one of:\n   { \"primary\": \"ter_id\", \"secondary\": \"ter_id\", \"ratio\": (number) }\n");
        }
    } else if ( strict ) {
        jo.throw_error("'default_groundcover' required for 'default'");
    }
    if ( ! jo.read("num_forests", new_region.num_forests) && strict ) {
        jo.throw_error("num_forests required for default");
    }
    if ( ! jo.read("forest_size_min", new_region.forest_size_min) && strict ) {
        jo.throw_error("forest_size_min required for default");
    }
    if ( ! jo.read("forest_size_max", new_region.forest_size_max) && strict ) {
        jo.throw_error("forest_size_max required for default");
    }
    if ( ! jo.read("house_basement_chance", new_region.house_basement_chance) && strict ) {
        jo.throw_error("house_basement_chance required for default");
    }
    if ( ! jo.read("swamp_maxsize", new_region.swamp_maxsize) && strict ) {
        jo.throw_error("swamp_maxsize required for default");
    }
    if ( ! jo.read("swamp_river_influence", new_region.swamp_river_influence) && strict ) {
        jo.throw_error("swamp_river_influence required for default");
    }
    if ( ! jo.read("swamp_spread_chance", new_region.swamp_spread_chance) && strict ) {
        jo.throw_error("swamp_spread_chance required for default");
    }

    if ( ! jo.has_object("field_coverage") ) {
        if ( strict ) {
            jo.throw_error("\"field_coverage\": { ... } required for default");
        }
    } else {
        JsonObject pjo = jo.get_object("field_coverage");
        double tmpval = 0.0f;
        if ( ! pjo.read("percent_coverage", tmpval) ) {
            pjo.throw_error("field_coverage: percent_coverage required");
        }
        new_region.field_coverage.mpercent_coverage = (int)(tmpval * 10000.0);
        if ( ! pjo.read("default_ter", new_region.field_coverage.default_ter_str) ) {
            pjo.throw_error("field_coverage: default_ter required");
        }
        tmpval = 0.0f;
        if ( pjo.has_object("other") ) {
            JsonObject opjo = pjo.get_object("other");
            std::set<std::string> keys = opjo.get_member_names();
            for( const auto &key : keys ) {
                tmpval = 0.0f;
                if( key != "//" ) {
                    if( opjo.read( key, tmpval ) ) {
                        new_region.field_coverage.percent_str[key] = tmpval;
                    }
                }
            }
        }
        if ( pjo.read("boost_chance", tmpval) && tmpval != 0.0f ) {
            new_region.field_coverage.boost_chance = (int)(tmpval * 10000.0);
            if ( ! pjo.read("boosted_percent_coverage", tmpval) ) {
                pjo.throw_error("boost_chance > 0 requires boosted_percent_coverage");
            }
            new_region.field_coverage.boosted_mpercent_coverage = (int)(tmpval * 10000.0);
            if ( ! pjo.read("boosted_other_percent", tmpval) ) {
                pjo.throw_error("boost_chance > 0 requires boosted_other_percent");
            }
            new_region.field_coverage.boosted_other_mpercent = (int)(tmpval * 10000.0);
            if ( pjo.has_object("boosted_other") ) {
                JsonObject opjo = pjo.get_object("boosted_other");
                std::set<std::string> keys = opjo.get_member_names();
                for( const auto &key : keys ) {
                    tmpval = 0.0f;
                    if( key != "//" ) {
                        if( opjo.read( key, tmpval ) ) {
                            new_region.field_coverage.boosted_percent_str[key] = tmpval;
                        }
                    }
                }
            } else {
                pjo.throw_error("boost_chance > 0 requires boosted_other { ... }");
            }
        }
    }

    if ( ! jo.has_object("map_extras") ) {
        if ( strict ) {
            jo.throw_error("\"map_extras\": { ... } required for default");
        }
    } else {
        JsonObject pjo = jo.get_object("map_extras");

        std::set<std::string> zones = pjo.get_member_names();
        for( const auto &zone : zones ) {
            if( zone != "//" ) {
                JsonObject zjo = pjo.get_object(zone);
                map_extras extras(0);

                if ( ! zjo.read("chance", extras.chance) && strict ) {
                    zjo.throw_error("chance required for default");
                }

                if ( ! zjo.has_object("extras") ) {
                    if ( strict ) {
                        zjo.throw_error("\"extras\": { ... } required for default");
                    }
                } else {
                    JsonObject exjo = zjo.get_object("extras");

                    std::set<std::string> keys = exjo.get_member_names();
                    for( const auto &key : keys ) {
                        if(key != "//" ) {
                            if (get_option<bool>( "CLASSIC_ZOMBIES" )
                                && classic_extras.count(key) == 0) {
                                continue;
                            }
                            extras.values.add(key, exjo.get_int(key, 0));
                        }
                    }
                }

                new_region.region_extras[zone] = extras;
            }
        }
    }

    if ( ! jo.has_object("city") ) {
        if ( strict ) {
            jo.throw_error("\"city\": { ... } required for default");
        }
    } else {
        JsonObject cjo = jo.get_object("city");
        if ( ! cjo.read("shop_radius", new_region.city_spec.shop_radius) && strict ) {
            jo.throw_error("city: shop_radius required for default");
        }
        if ( ! cjo.read("park_radius", new_region.city_spec.park_radius) && strict ) {
            jo.throw_error("city: park_radius required for default");
        }
        if ( ! cjo.has_object("shops") && strict ) {
            if ( strict ) {
                jo.throw_error("city: \"shops\": { ... } required for default");
            }
        } else {
            JsonObject wjo = cjo.get_object("shops");
            std::set<std::string> keys = wjo.get_member_names();
            for( const auto &key : keys ) {
                if( key != "//" ) {
                    if( wjo.has_int( key ) ) {
                        new_region.city_spec.shops.add( { oter_type_str_id( key ) }, wjo.get_int( key ) );
                    }
                }
            }
        }
        if ( ! cjo.has_object("parks") && strict ) {
            if ( strict ) {
                jo.throw_error("city: \"parks\": { ... } required for default");
            }
        } else {
            JsonObject wjo = cjo.get_object("parks");
            std::set<std::string> keys = wjo.get_member_names();
            for( const auto &key : keys ) {
                if( key != "//" ) {
                    if( wjo.has_int( key ) ) {
                        new_region.city_spec.parks.add( { oter_type_str_id( key ) }, wjo.get_int( key ) );
                    }
                }
            }
        }
    }

    if ( ! jo.has_object("weather") ) {
        if ( strict ) {
            jo.throw_error("\"weather\": { ... } required for default");
        }
    } else {
        JsonObject wjo = jo.get_object( "weather" );
        new_region.weather = weather_generator::load( wjo );
    }

    region_settings_map[new_region.id] = new_region;
}

void reset_region_settings()
{
    region_settings_map.clear();
}

/*
    Entry point for parsing "region_overlay" json objects.
        Will loop through and apply the overlay to each of the overlay's regions.
*/
void load_region_overlay(JsonObject &jo)
{
    if (jo.has_array("regions")) {
        JsonArray regions = jo.get_array("regions");

        while (regions.has_more()) {
            std::string regionid = regions.next_string();

            if(regionid == "all") {
                if(regions.size() != 1) {
                    jo.throw_error("regions: More than one region is not allowed when \"all\" is used");
                }

                for(auto &itr : region_settings_map) {
                    apply_region_overlay(jo, itr.second);
                }
            }
            else {
                auto itr = region_settings_map.find(regionid);
                if(itr == region_settings_map.end()) {
                    jo.throw_error("region: " + regionid + " not found in region_settings_map");
                }
                else {
                    apply_region_overlay(jo, itr->second);
                }
            }
        }
    }
    else {
        jo.throw_error("\"regions\" is required and must be an array");
    }
}

void apply_region_overlay(JsonObject &jo, regional_settings &region)
{
    jo.read("default_oter", region.default_oter);

    if (jo.has_object("default_groundcover")) {
        JsonObject jio = jo.get_object("default_groundcover");

        jio.read("primary", region.default_groundcover_str->primary_str);
        jio.read("secondary", region.default_groundcover_str->secondary_str);
        jio.read("ratio", region.default_groundcover.chance);
    }

    jo.read("num_forests", region.num_forests);
    jo.read("forest_size_min", region.forest_size_min);
    jo.read("forest_size_max", region.forest_size_max);
    jo.read("house_basement_chance", region.house_basement_chance);
    jo.read("swamp_maxsize", region.swamp_maxsize);
    jo.read("swamp_river_influence", region.swamp_river_influence);
    jo.read("swamp_spread_chance", region.swamp_spread_chance);

    JsonObject fieldjo = jo.get_object("field_coverage");
    double tmpval = 0.0f;
    if (fieldjo.read("percent_coverage", tmpval)) {
        region.field_coverage.mpercent_coverage = (int)(tmpval * 10000.0);
    }

    fieldjo.read("default_ter", region.field_coverage.default_ter_str);

    JsonObject otherjo = fieldjo.get_object("other");
    std::set<std::string> keys = otherjo.get_member_names();
    for( const auto &key : keys ) {
        if( key != "//" ) {
            if( otherjo.read( key, tmpval ) ) {
                region.field_coverage.percent_str[key] = tmpval;
            }
        }
    }

    if (fieldjo.read("boost_chance", tmpval)) {
        region.field_coverage.boost_chance = (int)(tmpval * 10000.0);
    }
    if (fieldjo.read("boosted_percent_coverage", tmpval)) {
        if(region.field_coverage.boost_chance > 0.0f && tmpval == 0.0f) {
            fieldjo.throw_error("boost_chance > 0 requires boosted_percent_coverage");
        }

        region.field_coverage.boosted_mpercent_coverage = (int)(tmpval * 10000.0);
    }

    if (fieldjo.read("boosted_other_percent", tmpval)) {
        if(region.field_coverage.boost_chance > 0.0f && tmpval == 0.0f) {
            fieldjo.throw_error("boost_chance > 0 requires boosted_other_percent");
        }

        region.field_coverage.boosted_other_mpercent = (int)(tmpval * 10000.0);
    }

    JsonObject boostedjo = fieldjo.get_object("boosted_other");
    std::set<std::string> boostedkeys = boostedjo.get_member_names();
    for( const auto &key : boostedkeys ) {
        if( key != "//" ) {
            if( boostedjo.read( key, tmpval ) ) {
                region.field_coverage.boosted_percent_str[key] = tmpval;
            }
        }
    }

    if(region.field_coverage.boost_chance > 0.0f && region.field_coverage.boosted_percent_str.size() == 0) {
        fieldjo.throw_error("boost_chance > 0 requires boosted_other { ... }");
    }

    JsonObject mapextrajo = jo.get_object("map_extras");
    std::set<std::string> extrazones = mapextrajo.get_member_names();
    for( const auto &zone : extrazones ) {
        if( zone != "//" ) {
            JsonObject zonejo = mapextrajo.get_object(zone);

            int tmpval = 0;
            if (zonejo.read("chance", tmpval)) {
                region.region_extras[zone].chance = tmpval;
            }

            JsonObject extrasjo = zonejo.get_object("extras");
            std::set<std::string> extrakeys = extrasjo.get_member_names();
            for( const auto &key : extrakeys ) {
                if( key != "//" ) {
                    if (get_option<bool>( "CLASSIC_ZOMBIES" )
                        && classic_extras.count(key) == 0) {
                        continue;
                    }
                    region.region_extras[zone].values.add_or_replace(key, extrasjo.get_int(key));
                }
            }
        }
    }

    JsonObject cityjo = jo.get_object("city");

    cityjo.read("shop_radius", region.city_spec.shop_radius);
    cityjo.read("park_radius", region.city_spec.park_radius);

    JsonObject shopsjo = cityjo.get_object("shops");
    std::set<std::string> shopkeys = shopsjo.get_member_names();
    for( const auto &key : shopkeys ) {
        if( key != "//" ) {
            if( shopsjo.has_int( key ) ) {
                region.city_spec.shops.add_or_replace( { oter_type_str_id( key ) }, shopsjo.get_int( key ) );
            }
        }
    }

    JsonObject parksjo = cityjo.get_object("parks");
    std::set<std::string> parkkeys = parksjo.get_member_names();
    for( const auto &key : parkkeys ) {
        if( key != "//" ) {
            if( parksjo.has_int( key ) ) {
                region.city_spec.parks.add_or_replace( { oter_type_str_id( key ) }, parksjo.get_int( key ) );
            }
        }
    }

}

const overmap_special_terrain &overmap_special::get_terrain_at( const tripoint &p ) const
{
    const auto iter = std::find_if( terrains.begin(), terrains.end(), [ &p ]( const overmap_special_terrain &elem ) {
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
    [ &oter ]( const overmap_special_location *loc ) {
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
    if( !cit || !city_size.contains( cit.s ) ) {
        return false;
    }
    return city_distance.contains( cit.get_distance_from( p ) );
}

void overmap_special::load( JsonObject &jo, const std::string &src )
{
    const bool strict = src == "dda";

    mandatory( jo, was_loaded, "overmaps", terrains );

    std::vector<std::string> tmp_locations;
    mandatory( jo, was_loaded, "locations", tmp_locations );
    for( const std::string &elem : tmp_locations ) {
        const auto iter = special_locations.find( elem );

        if( iter != special_locations.end() ) {
            locations.insert( &iter->second );
        } else {
            debugmsg( "Overmap special \"%s\" has invalid location \"%s\".",
                      id.c_str(), elem.c_str() );
        }
    }

    mandatory( jo, was_loaded, "occurrences", occurrences );

    optional( jo, was_loaded, "connections", connections );

    assign( jo, "spawns", spawns, strict );

    assign( jo, "city_sizes", city_size, strict );
    assign( jo, "city_distance", city_distance, strict );
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
    }
}

void overmap_special::check() const
{
    std::set<int> invalid_terrains;
    std::set<int> fixed_terrains;
    std::set<tripoint> points;

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

overmap::overmap( int const x, int const y ) : loc( x, y )
{
    const std::string rsettings_id = get_option<std::string>( "DEFAULT_REGION" );
    t_regional_settings_map_citr rsit = region_settings_map.find( rsettings_id );

    if ( rsit == region_settings_map.end() ) {
        debugmsg("overmap(%d,%d): can't find region '%s'", x, y, rsettings_id.c_str() ); // gonna die now =[
    }
    settings = rsit->second;

    init_layers();
    try {
        open();
    } catch( const std::exception &err ) {
        debugmsg( "overmap (%d,%d) failed to load: %s", loc.x, loc.y, err.what() );
    }

    set_validity_from_settings();
    current_validity = overmap_valid::invalid;
}

overmap::overmap()
{
    t_regional_settings_map_citr rsit = region_settings_map.find( "default" );

    if ( rsit == region_settings_map.end() ) {
        debugmsg("Test overmap: can't find region 'default'" );
    }
    settings = rsit->second;
    init_layers();

    set_validity_from_settings();
    current_validity = overmap_valid::invalid;
}

overmap::~overmap()
{
    for( npc *npc_to_delete : npcs ) {
        delete npc_to_delete;
    }
}

void overmap::set_validity_from_settings()
{
    const std::string &opt = get_option<std::string>( "ALLOW_INVALID_OVERMAPS" );
    if( opt == "allow_invalid" ) {
        minimum_validity = overmap_valid::invalid;
    } else if( opt == "ask_invalid" ) {
        minimum_validity = overmap_valid::unlimited;
    } else {
        minimum_validity = overmap_valid::valid;
    }
}

void overmap::init_layers()
{
    for(int z = 0; z < OVERMAP_LAYERS; ++z) {
        oter_str_id default_type( (z < OVERMAP_DEPTH) ? oter_str_id( "empty_rock" ) : (z == OVERMAP_DEPTH) ? settings.default_oter :
                               oter_str_id( "open_air" ) );
        for(int i = 0; i < OMAPX; ++i) {
            for(int j = 0; j < OMAPY; ++j) {
                layer[z].terrain[i][j] = default_type.id();
                layer[z].visible[i][j] = false;
                layer[z].explored[i][j] = false;
            }
        }
    }
}

oter_id &overmap::ter(const int x, const int y, const int z)
{
    if( !inbounds( x, y, z ) ) {
        return ot_null;
    }

    return layer[z + OVERMAP_DEPTH].terrain[x][y];
}

const oter_id overmap::get_ter(const int x, const int y, const int z) const
{
    if( !inbounds( x, y, z ) ) {
        return ot_null;
    }

    return layer[z + OVERMAP_DEPTH].terrain[x][y];
}

const oter_id overmap::get_ter( const tripoint &p ) const
{
    return get_ter( p.x, p.y, p.z );
}

bool &overmap::seen(int x, int y, int z)
{
    if( !inbounds( x, y, z ) ) {
        nullbool = false;
        return nullbool;
    }
    return layer[z + OVERMAP_DEPTH].visible[x][y];
}

bool &overmap::explored(int x, int y, int z)
{
    if( !inbounds( x, y, z ) ) {
        nullbool = false;
        return nullbool;
    }
    return layer[z + OVERMAP_DEPTH].explored[x][y];
}

bool overmap::is_explored(int const x, int const y, int const z) const
{
    if( !inbounds( x, y, z ) ) {
        return false;
    }
    return layer[z + OVERMAP_DEPTH].explored[x][y];
}

bool overmap::mongroup_check(const mongroup &candidate) const
{
    const auto matching_range = zg.equal_range(candidate.pos);
    return std::find_if( matching_range.first, matching_range.second,
        [candidate](std::pair<tripoint, mongroup> match) {
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

bool overmap::monster_check(const std::pair<tripoint, monster> &candidate) const
{
    const auto matching_range = monster_map.equal_range(candidate.first);
    return std::find_if( matching_range.first, matching_range.second,
        [candidate](std::pair<tripoint, monster> match) {
            return candidate.second.pos() == match.second.pos() &&
                candidate.second.type == match.second.type;
        } ) != matching_range.second;
}

void overmap::insert_npc( npc *who )
{
    npcs.push_back( who );
    g->set_npcs_dirty();
}

void overmap::erase_npc( npc *who )
{
    const auto iter = std::find( npcs.begin(), npcs.end(), who );
    assert( iter != npcs.end() );
    npcs.erase( iter );
    delete who;
    g->set_npcs_dirty();
}

bool overmap::has_note(int const x, int const y, int const z) const
{
    if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        return false;
    }

    for( auto &i : layer[z + OVERMAP_DEPTH].notes ) {
        if( i.x == x && i.y == y ) {
            return true;
        }
    }
    return false;
}

std::string const& overmap::note(int const x, int const y, int const z) const
{
    static std::string const fallback {};

    if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        return fallback;
    }

    auto const &notes = layer[z + OVERMAP_DEPTH].notes;
    auto const it = std::find_if(begin(notes), end(notes), [&](om_note const& n) {
        return n.x == x && n.y == y;
    });

    return (it != std::end(notes)) ? it->text : fallback;
}

void overmap::add_note(int const x, int const y, int const z, std::string message)
{
    if (z < -OVERMAP_DEPTH || z > OVERMAP_HEIGHT) {
        debugmsg("Attempting to add not to overmap for blank layer %d", z);
        return;
    }

    auto &notes = layer[z + OVERMAP_DEPTH].notes;
    auto const it = std::find_if(begin(notes), end(notes), [&](om_note const& n) {
        return n.x == x && n.y == y;
    });

    if (it == std::end(notes)) {
        notes.emplace_back(om_note {std::move(message), x, y});
    } else if (!message.empty()) {
        it->text = std::move(message);
    } else {
        notes.erase(it);
    }
}

void overmap::delete_note(int const x, int const y, int const z)
{
    add_note(x, y, z, std::string {});
}

std::vector<point> overmap::find_notes(int const z, std::string const &text)
{
    std::vector<point> note_locations;
    map_layer &this_layer = layer[z + OVERMAP_DEPTH];
    for( auto note : this_layer.notes ) {
        if( lcmatch( note.text, text ) ) {
            note_locations.push_back( global_base_point() + point( note.x, note.y ) );
        }
    }
    return note_locations;
}

bool overmap::inbounds( const tripoint &loc, int clearance )
{
    return ( loc.x >= clearance && loc.x < OMAPX - clearance &&
             loc.y >= clearance && loc.y < OMAPY - clearance &&
             loc.z >= -OVERMAP_DEPTH && loc.z <= OVERMAP_HEIGHT );
}

bool overmap::inbounds( int x, int y, int z, int clearance )
{
    return inbounds( tripoint( x, y, z ), clearance );
}

point overmap::display_notes(int z)
{
    const overmapbuffer::t_notes_vector notes = overmap_buffer.get_all_notes(z);
    WINDOW *w_notes = newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                              (TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0,
                              (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    draw_border(w_notes);

    const std::string title = _("Notes:");
    const std::string back_msg = _("< Prev notes");
    const std::string forward_msg = _("Next notes >");

    // Number of items to show at one time, 2 rows for border, 2 for title & bottom text
    const unsigned maxitems = FULL_SCREEN_HEIGHT - 4;
    int ch = '.';
    unsigned start = 0;
    const int back_len = utf8_width( back_msg );
    bool redraw = true;
    point result(-1, -1);

    mvwprintz(w_notes, 1, 1, c_ltgray, title.c_str());
    do {
        if (redraw) {
            for (int i = 2; i < FULL_SCREEN_HEIGHT - 1; i++) {
                for (int j = 1; j < FULL_SCREEN_WIDTH - 1; j++) {
                    mvwputch(w_notes, i, j, c_black, ' ');
                }
            }
            for (unsigned i = 0; i < maxitems; i++) {
                const unsigned cur_it = start + i;
                if (cur_it >= notes.size()) {
                    continue;
                }
                // Print letter ('a' <=> cur_it == start)
                mvwputch (w_notes, i + 2, 1, c_white, 'a' + i);
                mvwprintz(w_notes, i + 2, 3, c_ltgray, "- %s", notes[cur_it].second.c_str());
            }
            if (start >= maxitems) {
                mvwprintw(w_notes, maxitems + 2, 1, back_msg.c_str());
            }
            if (start + maxitems < notes.size()) {
                mvwprintw(w_notes, maxitems + 2, 2 + back_len, forward_msg.c_str());
            }
            mvwprintz(w_notes, 1, 40, c_white, _("Press letter to center on note"));
            mvwprintz(w_notes, FULL_SCREEN_HEIGHT - 2, 40, c_white, _("Spacebar - Return to map  "));
            wrefresh(w_notes);
            redraw = false;
        }
        // TODO: use input context
        ch = inp_mngr.get_input_event().get_first_input();
        if (ch == '<' && start >= maxitems) {
            start -= maxitems;
            redraw = true;
        } else if (ch == '>' && start + maxitems < notes.size()) {
            start += maxitems;
            redraw = true;
        } else if(ch >= 'a' && ch <= 'z') {
            const unsigned chosen = ch - 'a' + start;
            if (chosen < notes.size()) {
                result = notes[chosen].first;
                break; // -> return result
            }
        }
    } while(ch != ' ' && ch != '\n' && ch != KEY_ESCAPE);
    delwin(w_notes);
    return result;
}

const scent_trace &overmap::scent_at( const tripoint &loc ) const
{
    const static scent_trace null_scent;
    const auto &scent_found = scents.find( loc );
    if( scent_found != scents.end() ) {
        return scent_found->second;
    }
    return null_scent;
}

void overmap::set_scent( const tripoint &loc, scent_trace &new_scent )
{
    // TODO: increase strength of scent trace when applied repeatedlu in a short timespan.
    scents[loc] = new_scent;
}

void overmap::generate(const overmap *north, const overmap *east,
                       const overmap *south, const overmap *west)
{
    dbg(D_INFO) << "overmap::generate start...";
    std::vector<point> river_start;// West/North endpoints of rivers
    std::vector<point> river_end; // East/South endpoints of rivers

    // Determine points where rivers & roads should connect w/ adjacent maps
    const oter_id river_center("river_center"); // optimized comparison.

    if (north != NULL) {
        for (int i = 2; i < OMAPX - 2; i++) {
            if (is_river(north->get_ter(i, OMAPY - 1, 0))) {
                ter(i, 0, 0) = river_center;
            }
            if (is_river(north->get_ter(i, OMAPY - 1, 0)) &&
                is_river(north->get_ter(i - 1, OMAPY - 1, 0)) &&
                is_river(north->get_ter(i + 1, OMAPY - 1, 0))) {
                if (river_start.empty() ||
                    river_start[river_start.size() - 1].x < i - 6) {
                    river_start.push_back(point(i, 0));
                }
            }
        }
        for (auto &i : north->roads_out) {
            if (i.y == OMAPY - 1) {
                roads_out.push_back(city(i.x, 0, 0));
            }
        }
    }
    size_t rivers_from_north = river_start.size();
    if (west != NULL) {
        for (int i = 2; i < OMAPY - 2; i++) {
            if (is_river(west->get_ter(OMAPX - 1, i, 0))) {
                ter(0, i, 0) = river_center;
            }
            if (is_river(west->get_ter(OMAPX - 1, i, 0)) &&
                is_river(west->get_ter(OMAPX - 1, i - 1, 0)) &&
                is_river(west->get_ter(OMAPX - 1, i + 1, 0))) {
                if (river_start.size() == rivers_from_north ||
                    river_start[river_start.size() - 1].y < i - 6) {
                    river_start.push_back(point(0, i));
                }
            }
        }
        for (auto &i : west->roads_out) {
            if (i.x == OMAPX - 1) {
                roads_out.push_back(city(0, i.y, 0));
            }
        }
    }
    if (south != NULL) {
        for (int i = 2; i < OMAPX - 2; i++) {
            if (is_river(south->get_ter(i, 0, 0))) {
                ter(i, OMAPY - 1, 0) = river_center;
            }
            if (is_river(south->get_ter(i,     0, 0)) &&
                is_river(south->get_ter(i - 1, 0, 0)) &&
                is_river(south->get_ter(i + 1, 0, 0))) {
                if (river_end.empty() ||
                    river_end[river_end.size() - 1].x < i - 6) {
                    river_end.push_back(point(i, OMAPY - 1));
                }
            }
        }
        for (auto &i : south->roads_out) {
            if (i.y == 0) {
                roads_out.push_back(city(i.x, OMAPY - 1, 0));
            }
        }
    }
    size_t rivers_to_south = river_end.size();
    if (east != NULL) {
        for (int i = 2; i < OMAPY - 2; i++) {
            if (is_river(east->get_ter(0, i, 0))) {
                ter(OMAPX - 1, i, 0) = river_center;
            }
            if (is_river(east->get_ter(0, i, 0)) &&
                is_river(east->get_ter(0, i - 1, 0)) &&
                is_river(east->get_ter(0, i + 1, 0))) {
                if (river_end.size() == rivers_to_south ||
                    river_end[river_end.size() - 1].y < i - 6) {
                    river_end.push_back(point(OMAPX - 1, i));
                }
            }
        }
        for (auto &i : east->roads_out) {
            if (i.x == 0) {
                roads_out.push_back(city(OMAPX - 1, i.y, 0));
            }
        }
    }

    // Even up the start and end points of rivers. (difference of 1 is acceptable)
    // Also ensure there's at least one of each.
    std::vector<point> new_rivers;
    if (north == NULL || west == NULL) {
        while (river_start.empty() || river_start.size() + 1 < river_end.size()) {
            new_rivers.clear();
            if (north == NULL) {
                new_rivers.push_back( point(rng(10, OMAPX - 11), 0) );
            }
            if (west == NULL) {
                new_rivers.push_back( point(0, rng(10, OMAPY - 11)) );
            }
            river_start.push_back( random_entry( new_rivers ) );
        }
    }
    if (south == NULL || east == NULL) {
        while (river_end.empty() || river_end.size() + 1 < river_start.size()) {
            new_rivers.clear();
            if (south == NULL) {
                new_rivers.push_back( point(rng(10, OMAPX - 11), OMAPY - 1) );
            }
            if (east == NULL) {
                new_rivers.push_back( point(OMAPX - 1, rng(10, OMAPY - 11)) );
            }
            river_end.push_back( random_entry( new_rivers ) );
        }
    }

    // Now actually place those rivers.
    if (river_start.size() > river_end.size() && !river_end.empty()) {
        std::vector<point> river_end_copy = river_end;
        while (!river_start.empty()) {
            const point start = random_entry_removed( river_start );
            if (!river_end.empty()) {
                place_river(start, river_end[0]);
                river_end.erase(river_end.begin());
            } else {
                place_river( start, random_entry( river_end_copy ) );
            }
        }
    } else if (river_end.size() > river_start.size() && !river_start.empty()) {
        std::vector<point> river_start_copy = river_start;
        while (!river_end.empty()) {
            const point end = random_entry_removed( river_end );
            if (!river_start.empty()) {
                place_river(river_start[0], end);
                river_start.erase(river_start.begin());
            } else {
                place_river( random_entry( river_start_copy ), end );
            }
        }
    } else if (!river_end.empty()) {
        if (river_start.size() != river_end.size())
            river_start.push_back( point(rng(OMAPX / 4, (OMAPX * 3) / 4),
                                         rng(OMAPY / 4, (OMAPY * 3) / 4)));
        for (size_t i = 0; i < river_start.size(); i++) {
            place_river(river_start[i], river_end[i]);
        }
    }

    // Cities and forests come next.
    // These're agnostic of adjacent maps, so it's very simple.
    place_cities();
    place_forest();

    // Ideally we should have at least two exit points for roads, on different sides
    if (roads_out.size() < 2) {
        std::vector<city> viable_roads;
        int tmp;
        // Populate viable_roads with one point for each neighborless side.
        // Make sure these points don't conflict with rivers.
        // TODO: In theory this is a potential infinte loop...
        if (north == NULL) {
            do {
                tmp = rng(10, OMAPX - 11);
            } while (is_river(ter(tmp, 0, 0)) || is_river(ter(tmp - 1, 0, 0)) ||
                     is_river(ter(tmp + 1, 0, 0)) );
            viable_roads.push_back(city(tmp, 0, 0));
        }
        if (east == NULL) {
            do {
                tmp = rng(10, OMAPY - 11);
            } while (is_river(ter(OMAPX - 1, tmp, 0)) || is_river(ter(OMAPX - 1, tmp - 1, 0)) ||
                     is_river(ter(OMAPX - 1, tmp + 1, 0)));
            viable_roads.push_back(city(OMAPX - 1, tmp, 0));
        }
        if (south == NULL) {
            do {
                tmp = rng(10, OMAPX - 11);
            } while (is_river(ter(tmp, OMAPY - 1, 0)) || is_river(ter(tmp - 1, OMAPY - 1, 0)) ||
                     is_river(ter(tmp + 1, OMAPY - 1, 0)));
            viable_roads.push_back(city(tmp, OMAPY - 1, 0));
        }
        if (west == NULL) {
            do {
                tmp = rng(10, OMAPY - 11);
            } while (is_river(ter(0, tmp, 0)) || is_river(ter(0, tmp - 1, 0)) ||
                     is_river(ter(0, tmp + 1, 0)));
            viable_roads.push_back(city(0, tmp, 0));
        }
        while (roads_out.size() < 2 && !viable_roads.empty()) {
            roads_out.push_back( random_entry_removed( viable_roads ) );
        }
    }

    std::vector<point> road_points; // cities and roads_out together
    // Compile our master list of roads; it's less messy if roads_out is first
    road_points.reserve( roads_out.size() + cities.size() );
    for( const auto &elem : roads_out ) {
        road_points.emplace_back( elem.x, elem.y );
    }
    for( const auto &elem : cities ) {
        road_points.emplace_back( elem.x, elem.y );
    }
    // And finally connect them via roads.
    connect_closest_points( road_points, 0, oter_type_id( "road" ) );

    place_specials();
    // Clean up our roads and rivers
    polish(0);

    // TODO: there is no reason we can't generate the sublevels in one pass
    //       for that matter there is no reason we can't as we add the entrance ways either

    // Always need at least one sublevel, but how many more
    int z = -1;
    bool requires_sub = false;
    do {
        requires_sub = generate_sub(z);
    } while(requires_sub && (--z >= -OVERMAP_DEPTH));

    // Place the monsters, now that the terrain is laid out
    place_mongroups();
    place_radios();
    dbg(D_INFO) << "overmap::generate done";
}


bool overmap::generate_sub(int const z)
{
    bool requires_sub = false;
    std::vector<point> subway_points;
    std::vector<point> sewer_points;

    std::vector<city> ant_points;
    std::vector<city> goo_points;
    std::vector<city> lab_points;
    std::vector<city> ice_lab_points;
    std::vector<point> shaft_points;
    std::vector<city> mine_points;
    // These are so common that it's worth checking first as int.
    const oter_id skip_above[5] = {
        oter_id("empty_rock"), oter_id("forest"), oter_id("field"),
        oter_id("forest_thick"), oter_id("forest_water")
    };

    for (int i = 0; i < OMAPX; i++) {
        for (int j = 0; j < OMAPY; j++) {
            oter_id oter_above = ter(i, j, z + 1);

            // implicitly skip skip_above oter_ids
            bool skipme = false;
            for( auto &elem : skip_above ) {
                if( oter_above == elem ) {
                    skipme = true;
                    break;
                }
            }
            if( skipme )
            {
                continue;
            }

            if (is_ot_type("house_base", oter_above)) {
                ter(i, j, z) = oter_id( "basement" );
            } else if (is_ot_type("sub_station", oter_above)) {
                ter(i, j, z) = oter_id( "subway_isolated" );
                subway_points.emplace_back( i, j );
            } else if (oter_above == "road_nesw_manhole") {
                ter(i, j, z) = oter_id( "sewer_isolated" );
                sewer_points.emplace_back( i, j );
            } else if (oter_above == "sewage_treatment") {
                sewer_points.emplace_back( i, j );
            } else if (oter_above == "cave" && z == -1) {
                if (one_in(3)) {
                    ter(i, j, z) = oter_id( "cave_rat" );
                    requires_sub = true; // rat caves are two level
                } else {
                    ter(i, j, z) = oter_id( "cave" );
                }
            } else if (oter_above == "cave_rat" && z == -2) {
                ter(i, j, z) = oter_id( "cave_rat" );
            } else if (oter_above == "anthill") {
                int size = rng(MIN_ANT_SIZE, MAX_ANT_SIZE);
                ant_points.push_back(city(i, j, size));
                add_mon_group(mongroup( mongroup_id( "GROUP_ANT" ), i * 2, j * 2, z, (size * 3) / 2, rng(6000, 8000)));
            } else if (oter_above == "slimepit_down") {
                int size = rng(MIN_GOO_SIZE, MAX_GOO_SIZE);
                goo_points.push_back(city(i, j, size));
            } else if (oter_above == "forest_water") {
                ter(i, j, z) = oter_id( "cavern" );
                chip_rock( i, j, z );
            } else if (oter_above == "lab_core" ||
                       (z == -1 && oter_above == "lab_stairs")) {
                lab_points.push_back(city(i, j, rng(1, 5 + z)));
            } else if (oter_above == "lab_stairs") {
                ter(i, j, z) = oter_id( "lab" );
            } else if (oter_above == "ice_lab_core" ||
                       (z == -1 && oter_above == "ice_lab_stairs")) {
                ice_lab_points.push_back(city(i, j, rng(1, 5 + z)));
            } else if (oter_above == "ice_lab_stairs") {
                ter(i, j, z) = oter_id( "ice_lab" );
            } else if (oter_above == "mine_entrance") {
                shaft_points.push_back( point(i, j) );
            } else if (oter_above == "mine_shaft" ||
                       oter_above == "mine_down"    ) {
                ter(i, j, z) = oter_id( "mine" );
                mine_points.push_back(city(i, j, rng(6 + z, 10 + z)));
                // technically not all finales need a sub level,
                // but at this point we don't know
                requires_sub = true;
            } else if( oter_above == "mine_finale" ) {
                for( auto &p : g->m.points_in_radius( tripoint( i, j, z ), 1, 0 ) ) {
                    ter( p.x, p.y, p.z ) = oter_id( "spiral" );
                }
                ter( i, j, z ) = oter_id( "spiral_hub" );
                add_mon_group( mongroup( mongroup_id( "GROUP_SPIRAL" ), i * 2, j * 2, z, 2, 200 ) );
            } else if ( oter_above == "silo" ) {
                if (rng(2, 7) < abs(z) || rng(2, 7) < abs(z)) {
                    ter(i, j, z) = oter_id( "silo_finale" );
                } else {
                    ter(i, j, z) = oter_id( "silo" );
                    requires_sub = true;
                }
            }
        }
    }

    for (auto &i : goo_points) {
        requires_sub |= build_slimepit(i.x, i.y, z, i.s);
    }
    connect_closest_points( sewer_points, z, oter_type_id( "sewer") );
    polish(z, "sewer");
    connect_closest_points( subway_points, z, oter_type_id( "subway" ) );
    for (auto &i : subway_points) {
        ter(i.x, i.y, z) = oter_id( "subway_station" );
    }
    for (auto &i : lab_points) {
        bool lab = build_lab(i.x, i.y, z, i.s);
        requires_sub |= lab;
        if (!lab && ter(i.x, i.y, z) == "lab_core") {
            ter(i.x, i.y, z) = oter_id( "lab" );
        }
    }
    for (auto &i : ice_lab_points) {
        bool ice_lab = build_lab(i.x, i.y, z, i.s, true);
        requires_sub |= ice_lab;
        if (!ice_lab && ter(i.x, i.y, z) == "ice_lab_core") {
            ter(i.x, i.y, z) = oter_id( "ice_lab" );
        }
    }
    for (auto &i : ant_points) {
        build_anthill(i.x, i.y, z, i.s);
    }
    polish(z, "subway");
    polish(z, "ants");

    for (auto &i : cities) {
        if (one_in(3)) {
            add_mon_group(mongroup( mongroup_id( "GROUP_CHUD" ), i.x * 2, i.y * 2, z, i.s, i.s * 20));
        }
        if (!one_in(8)) {
            add_mon_group(mongroup( mongroup_id( "GROUP_SEWER" ), i.x * 2, i.y * 2, z, (i.s * 7) / 2, i.s * 70));
        }
    }

    place_rifts(z);
    for (auto &i : mine_points) {
        build_mine(i.x, i.y, z, i.s);
    }

    for (auto &i : shaft_points) {
        ter(i.x, i.y, z) = oter_id( "mine_shaft" );
        requires_sub = true;
    }
    return requires_sub;
}

std::vector<point> overmap::find_terrain(const std::string &term, int zlevel)
{
    std::vector<point> found;
    for (int x = 0; x < OMAPX; x++) {
        for (int y = 0; y < OMAPY; y++) {
            if (seen(x, y, zlevel) &&
                lcmatch( ter(x, y, zlevel)->get_name(), term ) ) {
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
        if (dist < distance) {
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

// {note symbol, note color, offset to text}
std::tuple<char, nc_color, size_t> get_note_display_info(std::string const &note)
{
    std::tuple<char, nc_color, size_t> result {'N', c_yellow, 0};
    bool set_color  = false;
    bool set_symbol = false;

    size_t pos = 0;
    for (int i = 0; i < 2; ++i) {
        // find the first non-whitespace non-delimiter
        pos = note.find_first_not_of(" :;", pos, 3);
        if (pos == std::string::npos) {
            return result;
        }

        // find the first following delimiter
        auto const end = note.find_first_of(" :;", pos, 3);
        if (end == std::string::npos) {
            return result;
        }

        // set color or symbol
        if (!set_symbol && note[end] == ':') {
            std::get<0>(result) = note[end - 1];
            std::get<2>(result) = end + 1;
            set_symbol = true;
        } else if (!set_color && note[end] == ';') {
            std::get<1>(result) = get_note_color(note.substr(pos, end - pos));
            std::get<2>(result) = end + 1;
            set_color = true;
        }

        pos = end + 1;
    }

    return result;
}

static bool get_weather_glyph( tripoint const &pos, nc_color &ter_color, long &ter_sym )
{
    // Weather calculation is a bit expensive, so it's cached here.
    static std::map<tripoint, weather_type> weather_cache;
    static calendar last_weather_display = calendar::turn;
    if( last_weather_display != calendar::turn ) {
        last_weather_display = calendar::turn;
        weather_cache.clear();
    }
    auto iter = weather_cache.find( pos );
    if( iter == weather_cache.end() ) {
        auto const abs_ms_pos =  tripoint( pos.x * SEEX * 2, pos.y * SEEY * 2, pos.z );
        const auto &wgen = overmap_buffer.get_settings( pos.x, pos.y, pos.z ).weather;
        auto const weather = wgen.get_weather_conditions( abs_ms_pos, calendar::turn, g->get_seed() );
        iter = weather_cache.insert( std::make_pair( pos, weather ) ).first;
    }
    switch( iter->second ) {
        case WEATHER_SUNNY:
        case WEATHER_CLEAR:
        case WEATHER_NULL:
        case NUM_WEATHER_TYPES:
            // show the terrain as usual
            return false;
        case WEATHER_CLOUDY:
            ter_color = c_white;
            ter_sym = '8';
            break;
        case WEATHER_DRIZZLE:
        case WEATHER_FLURRIES:
            ter_color = c_ltblue;
            ter_sym = '8';
            break;
        case WEATHER_ACID_DRIZZLE:
            ter_color = c_ltgreen;
            ter_sym = '8';
            break;
        case WEATHER_RAINY:
        case WEATHER_SNOW:
            ter_color = c_blue;
            ter_sym = '8';
            break;
        case WEATHER_ACID_RAIN:
            ter_color = c_green;
            ter_sym = '8';
            break;
        case WEATHER_THUNDER:
        case WEATHER_LIGHTNING:
        case WEATHER_SNOWSTORM:
            ter_color = c_dkgray;
            ter_sym = '8';
            break;
    }
    return true;
}

static bool get_scent_glyph( const tripoint &pos, nc_color &ter_color, long &ter_sym )
{
    auto possible_scent = overmap_buffer.scent_at( pos );
    if( possible_scent.creation_turn >= 0 ) {
        color_manager &color_list = get_all_colors();
        int i = 0;
        int scent_age = calendar::turn - possible_scent.creation_turn;
        while( i < num_colors && scent_age > 0 ) {
            i++;
            scent_age /= 10;
        }
        ter_color = color_list.get( (color_id)i );
        int scent_strength = possible_scent.initial_strength;
        char c = '0';
        while( c <= '9' && scent_strength > 0 ) {
            c++;
            scent_strength /= 10;
        }
        ter_sym = c;
        return true;
    }
    // but it makes no scents!
    return false;
}

void overmap::draw(WINDOW *w, WINDOW *wbar, const tripoint &center,
                   const tripoint &orig, bool blink, bool show_explored,
                   input_context *inp_ctxt, const draw_data_t &data)
{
    const int z     = center.z;
    const int cursx = center.x;
    const int cursy = center.y;
    const int om_map_width  = OVERMAP_WINDOW_WIDTH;
    const int om_map_height = OVERMAP_WINDOW_HEIGHT;
    const int om_half_width = om_map_width / 2;
    const int om_half_height = om_map_height / 2;

    // Target of current mission
    const tripoint target = g->u.get_active_mission_target();
    const bool has_target = target != overmap::invalid_tripoint;
    // seen status & terrain of center position
    bool csee = false;
    oter_id ccur_ter = ot_null;
    // Debug vision allows seeing everything
    const bool has_debug_vision = g->u.has_trait( trait_id( "DEBUG_NIGHTVISION" ) );
    // sight_points is hoisted for speed reasons.
    const int sight_points = !has_debug_vision ?
                             g->u.overmap_sight_range( g->light_level( g->u.posz() ) ) :
                             100;

    std::string sZoneName;
    tripoint tripointZone = tripoint(-1, -1, -1);
    const auto &zones = zone_manager::get_manager();

    if( data.iZoneIndex != -1 ) {
        sZoneName = zones.zones[data.iZoneIndex].get_name();
        tripointZone = ms_to_omt_copy(zones.zones[data.iZoneIndex].get_center_point());
    }

    // If we're debugging monster groups, find the monster group we've selected
    const mongroup *mgroup = nullptr;
    std::vector<mongroup *> mgroups;
    if(data.debug_mongroup) {
        mgroups = overmap_buffer.monsters_at( center.x, center.y, center.z );
        for( const auto &mgp : mgroups ) {
            mgroup = mgp;
            if( mgp->horde ) {
                break;
            }
        }
    }

    // A small LRU cache: most oter_id's occur in clumps like forests of swamps.
    // This cache helps avoid much more costly lookups in the full hashmap.
    constexpr size_t cache_size = 8; // used below to calculate the next index
    std::array<std::pair<oter_id, oter_t const*>, cache_size> cache {{}};
    size_t cache_next = 0;

    int const offset_x = cursx - om_half_width;
    int const offset_y = cursy - om_half_height;

    // For use with place_special: cache the color and symbol of each submap
    // and record the bounds to optimize lookups below
    std::unordered_map<tripoint, std::pair<long, nc_color>> special_cache;
    point s_begin, s_end = point( 0, 0 );
    if( blink && uistate.place_special ) {
        for( const auto &s_ter : uistate.place_special->terrains ) {
            if( s_ter.p.z == 0 ) {
                const tripoint rp = om_direction::rotate( s_ter.p, uistate.omedit_rotation );
                const oter_id oter =  s_ter.terrain->get_rotated( uistate.omedit_rotation );

                special_cache.insert( std::make_pair(
                    rp, std::make_pair( oter->get_sym(), oter->get_color() ) ) );

                s_begin.x = std::min( s_begin.x, rp.x );
                s_begin.y = std::min( s_begin.y, rp.y );
                s_end.x = std::max( s_end.x, rp.x );
                s_end.y = std::max( s_end.y, rp.y );
            }
        }
    }

    // Cache NPCs since time to draw them is linear (per seen tile) with their count
    struct npc_coloring {
        nc_color color;
        size_t count;
    };
    std::unordered_map<tripoint, npc_coloring> npc_color;
    if( blink ) {
        const auto &npcs = overmap_buffer.get_npcs_near_player( sight_points );
        for( const npc *np : npcs ) {
            if( np->posz() != z ) {
                continue;
            }

            const tripoint pos = np->global_omt_location();
            if( has_debug_vision || overmap_buffer.seen( pos.x, pos.y, pos.z ) ) {
                auto iter = npc_color.find( pos );
                nc_color np_color = np->basic_symbol_color();
                if( iter == npc_color.end() ) {
                    npc_color[ pos ] = { np_color, 1 };
                } else {
                    iter->second.count++;
                    // Randomly change to new NPC's color
                    if( iter->second.color != np_color && one_in( iter->second.count ) ) {
                        iter->second.color = np_color;
                    }
                }
            }
        }
    }

    for (int i = 0; i < om_map_width; ++i) {
        for (int j = 0; j < om_map_height; ++j) {
            const int omx = i + offset_x;
            const int omy = j + offset_y;

            oter_id cur_ter = ot_null;
            nc_color ter_color = c_black;
            long ter_sym = ' ';

            const bool see = has_debug_vision || overmap_buffer.seen(omx, omy, z);
            if (see) {
                // Only load terrain if we can actually see it
                cur_ter = overmap_buffer.ter(omx, omy, z);
            }

            tripoint const cur_pos {omx, omy, z};
            // Check if location is within player line-of-sight
            const bool los = see && g->u.overmap_los( cur_pos, sight_points );

            if (blink && cur_pos == orig) {
                // Display player pos, should always be visible
                ter_color = g->u.symbol_color();
                ter_sym   = '@';
            } else if( data.debug_weather && get_weather_glyph( tripoint( omx, omy, z ), ter_color, ter_sym ) ) {
                // ter_color and ter_sym have been set by get_weather_glyph
            } else if( data.debug_scent && get_scent_glyph( cur_pos, ter_color, ter_sym ) ) {
            } else if( blink && has_target && omx == target.x && omy == target.y ) {
                // Mission target, display always, player should know where it is anyway.
                ter_color = c_red;
                ter_sym   = '*';
                if( target.z > z ) {
                    ter_sym = '^';
                } else if( target.z < z ) {
                    ter_sym = 'v';
                }
            } else if (blink && overmap_buffer.has_note(cur_pos)) {
                // Display notes in all situations, even when not seen
                std::tie(ter_sym, ter_color, std::ignore) =
                    get_note_display_info(overmap_buffer.note(cur_pos));
            } else if (!see) {
                // All cases above ignore the seen-status,
                ter_color = c_dkgray;
                ter_sym   = '#';
                // All cases below assume that see is true.
            } else if( blink && npc_color.count( cur_pos ) != 0 ) {
                // Visible NPCs are cached already
                ter_color = npc_color[ cur_pos ].color;
                ter_sym   = '@';
            } else if (blink && los && overmap_buffer.has_horde(omx, omy, z)) {
                // Display Hordes only when within player line-of-sight
                ter_color = c_green;
                ter_sym   = 'Z';
            } else if (blink && overmap_buffer.has_vehicle(omx, omy, z)) {
                // Display Vehicles only when player can see the location
                ter_color = c_cyan;
                ter_sym   = 'c';
            } else if (!sZoneName.empty() && tripointZone.x == omx && tripointZone.y == omy) {
                ter_color = c_yellow;
                ter_sym   = 'Z';
            } else {
                // Nothing special, but is visible to the player.
                // First see if we have the oter_t cached
                oter_t const* info = nullptr;
                for (auto const &c : cache) {
                    if (c.first == cur_ter) {
                        info = c.second;
                        break;
                    }
                }
                // Nope, look in the hash map next
                if (!info) {
                    info = &cur_ter.obj();
                    cache[cache_next] = std::make_pair( cur_ter, info );
                    cache_next = ( cache_next + 1 ) % cache_size;
                }
                // Ok, we found something
                if (info) {
                    // Map tile marked as explored
                    bool const explored = show_explored && overmap_buffer.is_explored(omx, omy, z);
                    ter_color = explored ? c_dkgray : info->get_color();
                    ter_sym   = info->get_sym();
                }
            }

            // Are we debugging monster groups?
            if(blink && data.debug_mongroup) {
                // Check if this tile is the target of the currently selected group

                // Convert to position within overmap
                int localx = omx * 2;
                int localy = omy * 2;
                sm_to_om_remain(localx, localy);

                if(mgroup && mgroup->target.x / 2 == localx / 2 && mgroup->target.y / 2 == localy / 2) {
                    ter_color = c_red;
                    ter_sym = 'x';
                } else {
                    const auto &groups = overmap_buffer.monsters_at( omx, omy, center.z );
                    for( auto &mgp : groups ) {
                        if( mgp->type == mongroup_id( "GROUP_FOREST" ) ) {
                            // Don't flood the map with forest creatures.
                            continue;
                        }
                        if( mgp->horde ) {
                            // Hordes show as +
                            ter_sym = '+';
                            break;
                        } else {
                            // Regular groups show as -
                            ter_sym = '-';
                        }
                    }
                    // Set the color only if we encountered an eligible group.
                    if( ter_sym == '+' || ter_sym == '-' ) {
                        if( los ) {
                            ter_color = c_ltblue;
                        } else {
                            ter_color = c_blue;
                        }
                    }
                }
            }

            // Preview for place_terrain or place_special
            if( uistate.place_terrain || uistate.place_special ) {
                if( blink && uistate.place_terrain && omx == cursx && omy == cursy ) {
                    ter_color = uistate.place_terrain->get_color();
                    ter_sym = uistate.place_terrain->get_sym();
                } else if( blink && uistate.place_special ) {
                    if( omx - cursx >= s_begin.x && omx - cursx <= s_end.x &&
                        omy - cursy >= s_begin.y && omy - cursy <= s_end.y ) {
                        auto sm = special_cache.find( tripoint( omx - cursx, omy - cursy, z ) );
                        if( sm != special_cache.end() ) {
                            ter_color = sm->second.second;
                            ter_sym = sm->second.first;
                        }
                    }
                }
                // Highlight areas that already have been generated
                if( MAPBUFFER.lookup_submap(
                        omt_to_sm_copy( tripoint( omx, omy, z ) ) ) ) {
                    ter_color = red_background( ter_color );
                }
            }

            if( omx == cursx && omy == cursy && !uistate.place_special ) {
                csee = see;
                ccur_ter = cur_ter;
                mvwputch_hi(w, j, i, ter_color, ter_sym);
            } else {
                mvwputch(w, j, i, ter_color, ter_sym);
            }
        }
    }
    if (has_target && blink &&
        (target.x < offset_x ||
         target.x >= offset_x + om_map_width ||
         target.y < offset_y ||
         target.y >= offset_y + om_map_height)) {
        int marker_x = std::max( 0, std::min( om_map_width  - 1, target.x - offset_x ) );
        int marker_y = std::max( 0, std::min( om_map_height - 1, target.y - offset_y ) );
        long marker_sym = ' ';
        switch (direction_from(cursx, cursy, target.x, target.y)) {
        case NORTH:
            marker_sym = '^';
            break;
        case NORTHEAST:
            marker_sym = LINE_OOXX;
            break;
        case EAST:
            marker_sym = '>';
            break;
        case SOUTHEAST:
            marker_sym = LINE_XOOX;
            break;
        case SOUTH:
            marker_sym = 'v';
            break;
        case SOUTHWEST:
            marker_sym = LINE_XXOO;
            break;
        case WEST:
            marker_sym = '<';
            break;
        case NORTHWEST:
            marker_sym = LINE_OXXO;
            break;
        default:
            break; //Do nothing
        }
        mvwputch(w, marker_y, marker_x, c_red, marker_sym);
    }

    std::vector<std::pair<nc_color, std::string>> corner_text;

    std::string const &note_text = overmap_buffer.note(cursx, cursy, z);
    if (!note_text.empty()) {
        size_t const pos = std::get<2>(get_note_display_info(note_text));
        if (pos != std::string::npos) {
            corner_text.emplace_back( c_yellow, note_text.substr(pos) );
        }
    }

    for( const auto &npc : overmap_buffer.get_npcs_near_omt(cursx, cursy, z, 0) ) {
        if( !npc->marked_for_death ) {
            corner_text.emplace_back( npc->basic_symbol_color(), npc->name );
        }
    }

    for( auto &v : overmap_buffer.get_vehicle(cursx, cursy, z) ) {
        corner_text.emplace_back( c_white, v.name );
    }

    if( !corner_text.empty() ) {
        int maxlen = 0;
        for (auto const &line : corner_text) {
            maxlen = std::max( maxlen, utf8_width( line.second ) );
        }

        const std::string spacer(maxlen, ' ');
        for( size_t i = 0; i < corner_text.size(); i++ ) {
            const auto &pr = corner_text[ i ];
            // clear line, print line, print vertical line at the right side.
            mvwprintz( w, i, 0, c_yellow, spacer.c_str() );
            mvwprintz( w, i, 0, pr.first, "%s", pr.second.c_str() );
            mvwputch( w, i, maxlen, c_white, LINE_XOXO );
        }
        for (int i = 0; i <= maxlen; i++) {
            mvwputch( w, corner_text.size(), i, c_white, LINE_OXOX );
        }
        mvwputch( w, corner_text.size(), maxlen, c_white, LINE_XOOX );
    }

    if (sZoneName != "" && tripointZone.x == cursx && tripointZone.y == cursy) {
        std::string sTemp = _("Zone:");
        sTemp += " " + sZoneName;

        const int length = utf8_width( sTemp );
        for (int i = 0; i <= length; i++) {
            mvwputch(w, om_map_height-2, i, c_white, LINE_OXOX);
        }

        mvwprintz(w, om_map_height-1, 0, c_yellow, "%s", sTemp.c_str());
        mvwputch(w, om_map_height-2, length, c_white, LINE_OOXX);
        mvwputch(w, om_map_height-1, length, c_white, LINE_XOXO);
    }

    // Draw the vertical line
    for (int j = 0; j < TERMY; j++) {
        mvwputch(wbar, j, 0, c_white, LINE_XOXO);
    }

    // Clear the legend
    for (int i = 1; i < 55; i++) {
        for (int j = 0; j < TERMY; j++) {
            mvwputch(wbar, j, i, c_black, ' ');
        }
    }

    // Draw text describing the overmap tile at the cursor position.
    if (csee) {
        if(!mgroups.empty()) {
            int line_number = 6;
            for( const auto &mgroup : mgroups ) {
                mvwprintz(wbar, line_number++, 3,
                          c_blue, "  Species: %s", mgroup->type.c_str());
                mvwprintz(wbar, line_number++, 3,
                          c_blue, "# monsters: %d", mgroup->population + mgroup->monsters.size());
                if( !mgroup->horde ) {
                    continue;
                }
                mvwprintz(wbar, line_number++, 3,
                          c_blue, "  Interest: %d", mgroup->interest);
                mvwprintz(wbar, line_number, 3,
                          c_blue, "  Target: %d, %d", mgroup->target.x, mgroup->target.y);
                mvwprintz(wbar, line_number++, 3,
                          c_red, "x");
            }
        } else {
            const auto &ter = ccur_ter.obj();

            mvwputch( wbar, 1, 1, ter.get_color(), ter.get_sym() );
            std::vector<std::string> name = foldstring(ter.get_name(), 25);
            for (size_t i = 0; i < name.size(); i++) {
                mvwprintz(wbar, i + 1, 3, ter.get_color(), "%s", name[i].c_str());
            }
        }
    } else {
        mvwprintz(wbar, 1, 1, c_dkgray, _("# Unexplored"));
    }

    if (has_target) {
        // TODO: Add a note that the target is above/below us
        int distance = rl_dist( orig, target );
        mvwprintz(wbar, 3, 1, c_white, _("Distance to target: %d"), distance);
    }
    mvwprintz(wbar, 14, 1, c_magenta, _("Use movement keys to pan."));
    if( inp_ctxt != nullptr ) {
        int y = 16;

        const auto print_hint = [&]( const std::string &action, nc_color color = c_magenta ) {
            y += fold_and_print( wbar, y, 1, 27, color, string_format( _( "%s - %s" ),
                                 inp_ctxt->get_desc( action ).c_str(),
                                 inp_ctxt->get_action_name( action ).c_str() ) );
        };

        if( data.debug_editor ) {
            print_hint( "PLACE_TERRAIN", c_ltblue );
            print_hint( "PLACE_SPECIAL", c_ltblue );
            ++y;
        }

        print_hint( "LEVEL_UP" );
        print_hint( "LEVEL_DOWN" );
        print_hint( "CENTER" );
        print_hint( "SEARCH" );
        print_hint( "CREATE_NOTE" );
        print_hint( "DELETE_NOTE" );
        print_hint( "LIST_NOTES" );
        print_hint( "TOGGLE_BLINKING" );
        print_hint( "TOGGLE_OVERLAYS" );
        print_hint( "TOGGLE_EXPLORED" );
        print_hint( "HELP_KEYBINDINGS" );
        print_hint( "QUIT" );
    }
    point omt(cursx, cursy);
    const point om = omt_to_om_remain(omt);
    mvwprintz(wbar, getmaxy(wbar) - 1, 1, c_red,
              _("LEVEL %i, %d'%d, %d'%d"), z, om.x, omt.x, om.y, omt.y);

    // draw nice crosshair around the cursor
    if( blink && !uistate.place_terrain && !uistate.place_special ) {
        mvwputch(w, om_half_height-1, om_half_width-1, c_ltgray, LINE_OXXO);
        mvwputch(w, om_half_height-1, om_half_width+1, c_ltgray, LINE_OOXX);
        mvwputch(w, om_half_height+1, om_half_width-1, c_ltgray, LINE_XXOO);
        mvwputch(w, om_half_height+1, om_half_width+1, c_ltgray, LINE_XOOX);
    }
    // Done with all drawing!
    wrefresh(wbar);
    wmove( w, om_half_height, om_half_width );
    wrefresh(w);
}

tripoint overmap::draw_overmap()
{
    return draw_overmap(g->u.global_omt_location(), draw_data_t());
}

tripoint overmap::draw_overmap(int z)
{
    tripoint loc = g->u.global_omt_location();
    loc.z = z;
    return draw_overmap(loc, draw_data_t());
}

tripoint overmap::draw_hordes()
{
    draw_data_t data;
    data.debug_mongroup = true;
    return draw_overmap( g->u.global_omt_location(), data );
}

tripoint overmap::draw_weather()
{
    draw_data_t data;
    data.debug_weather = true;
    return draw_overmap( g->u.global_omt_location(), data );
}

tripoint overmap::draw_scents()
{
    draw_data_t data;
    data.debug_scent = true;
    return draw_overmap( g->u.global_omt_location(), data );
}

tripoint overmap::draw_editor()
{
    draw_data_t data;
    data.debug_editor = true;
    return draw_overmap( g->u.global_omt_location(), data );
}

tripoint overmap::draw_zones( tripoint const &center, tripoint const &select, int const iZoneIndex )
{
    draw_data_t data;
    data.select = select;
    data.iZoneIndex = iZoneIndex;
    return overmap::draw_overmap( center, data );
}

//Start drawing the overmap on the screen using the (m)ap command.
tripoint overmap::draw_overmap(const tripoint &orig, const draw_data_t &data)
{
    delwin(g->w_omlegend);
    g->w_omlegend = newwin(TERMY, 28, 0, TERMX - 28);
    delwin(g->w_overmap);
    g->w_overmap = newwin(OVERMAP_WINDOW_HEIGHT, OVERMAP_WINDOW_WIDTH, 0, 0);

    // Draw black padding space to avoid gap between map and legend
    // also clears the pixel minimap in TILES
    delwin(g->w_blackspace);
    g->w_blackspace = newwin(TERMY, TERMX, 0, 0);
    mvwputch(g->w_blackspace, 0, 0, c_black, ' ');
    wrefresh(g->w_blackspace);

    tripoint ret = invalid_tripoint;
    tripoint curs(orig);

    if( data.select != tripoint( -1, -1, -1 ) ) {
        curs = tripoint(data.select);
    }

    // Configure input context for navigating the map.
    input_context ictxt("OVERMAP");
    ictxt.register_action("ANY_INPUT");
    ictxt.register_directions();
    ictxt.register_action("CONFIRM");
    ictxt.register_action("LEVEL_UP");
    ictxt.register_action("LEVEL_DOWN");
    ictxt.register_action("HELP_KEYBINDINGS");

    // Actions whose keys we want to display.
    ictxt.register_action("CENTER");
    ictxt.register_action("CREATE_NOTE");
    ictxt.register_action("DELETE_NOTE");
    ictxt.register_action("SEARCH");
    ictxt.register_action("LIST_NOTES");
    ictxt.register_action("TOGGLE_BLINKING");
    ictxt.register_action("TOGGLE_OVERLAYS");
    ictxt.register_action("TOGGLE_EXPLORED");
    if( data.debug_editor ) {
        ictxt.register_action( "PLACE_TERRAIN" );
        ictxt.register_action( "PLACE_SPECIAL" );
    }
    ictxt.register_action("QUIT");
    std::string action;
    bool show_explored = true;
    do {
        draw(g->w_overmap, g->w_omlegend, curs, orig, uistate.overmap_show_overlays, show_explored, &ictxt, data);
        action = ictxt.handle_input( BLINK_SPEED );

        int dirx, diry;
        if (ictxt.get_direction(dirx, diry, action)) {
            curs.x += dirx;
            curs.y += diry;
        } else if (action == "CENTER") {
            curs = orig;
        } else if (action == "LEVEL_DOWN" && curs.z > -OVERMAP_DEPTH) {
            curs.z -= 1;
        } else if (action == "LEVEL_UP" && curs.z < OVERMAP_HEIGHT) {
            curs.z += 1;
        } else if (action == "CONFIRM") {
            ret = tripoint(curs.x, curs.y, curs.z);
        } else if (action == "QUIT") {
            ret = invalid_tripoint;
        } else if (action == "CREATE_NOTE") {
            std::string color_notes = _("Color codes: ");
            for( auto color_pair : get_note_color_names() ) {
                // The color index is not translatable, but the name is.
                color_notes += string_format( "%s:%s, ", color_pair.first.c_str(),
                                              _(color_pair.second.c_str()) );
            }
            const std::string old_note = overmap_buffer.note(curs);
            const std::string new_note = string_input_popup()
                                         .title( _("Note (X:TEXT for custom symbol, G; for color):") )
                                         .width( 45 )
                                         .text( old_note )
                                         .description( color_notes )
                                         .query_string();
            if( new_note.empty() && !old_note.empty() ) {
                // do nothing, the player should be using [D]elete
            } else if( old_note != new_note ) {
                overmap_buffer.add_note( curs, new_note );
            }
        } else if( action == "DELETE_NOTE" ) {
            if( overmap_buffer.has_note( curs ) && query_yn( _( "Really delete note?" ) ) ) {
                overmap_buffer.delete_note( curs );
            }
        } else if (action == "LIST_NOTES") {
            const point p = display_notes(curs.z);
            if (p.x != -1 && p.y != -1) {
                curs.x = p.x;
                curs.y = p.y;
            }
        } else if (action == "TOGGLE_BLINKING") {
            uistate.overmap_blinking = !uistate.overmap_blinking;
            // if we turn off overmap blinking, show overlays and explored status
            if (!uistate.overmap_blinking) {
                uistate.overmap_show_overlays = true;
            } else {
                show_explored = true;
            }
        } else if (action == "TOGGLE_OVERLAYS") {
            // if we are currently blinking, turn blinking off.
            if (uistate.overmap_blinking) {
                uistate.overmap_blinking = false;
                uistate.overmap_show_overlays = false;
                show_explored = false;
            } else {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
                show_explored = !show_explored;
            }
        } else if( action == "TOGGLE_EXPLORED" ) {
            overmap_buffer.toggle_explored( curs.x, curs.y, curs.z );
        } else if( action == "SEARCH" ) {
            std::string term = string_input_popup().title( _( "Search term:" ) ).query_string();
            if( term.empty() ) {
                continue;
            }
            std::transform( term.begin(), term.end(), term.begin(), tolower );

            // This is on purpose only the current overmap, otherwise
            // it would contain way to many entries
            overmap &om = overmap_buffer.get_om_global(point(curs.x, curs.y));
            std::vector<point> locations = om.find_notes(curs.z, term);
            std::vector<point> terlist = om.find_terrain(term, curs.z);
            locations.insert( locations.end(), terlist.begin(), terlist.end() );
            if( locations.empty() ) {
                continue;
            }
            int i = 0;
            //Navigate through results
            tripoint tmp = curs;
            WINDOW *w_search = newwin(13, 27, 3, TERMX - 27);
            input_context ctxt("OVERMAP_SEARCH");
            ctxt.register_action("NEXT_TAB", _("Next target"));
            ctxt.register_action("PREV_TAB", _("Previous target"));
            ctxt.register_action("QUIT");
            ctxt.register_action("CONFIRM");
            ctxt.register_action("HELP_KEYBINDINGS");
            ctxt.register_action("ANY_INPUT");
            do {
                tmp.x = locations[i].x;
                tmp.y = locations[i].y;
                draw(g->w_overmap, g->w_omlegend, tmp, orig, uistate.overmap_show_overlays, show_explored, NULL, draw_data_t());
                //Draw search box
                draw_border(w_search);
                mvwprintz(w_search, 1, 1, c_red, _("Find place:"));
                mvwprintz(w_search, 2, 1, c_ltblue, "                         ");
                mvwprintz(w_search, 2, 1, c_ltblue, "%s", term.c_str());
                mvwprintz(w_search, 4, 1, c_white, _("'<' '>' Cycle targets."));
                mvwprintz(w_search, 10, 1, c_white, _("Enter/Spacebar to select."));
                mvwprintz(w_search, 11, 1, c_white, _("q or ESC to return."));
                wrefresh(w_search);
                action = ctxt.handle_input( BLINK_SPEED );
                if (uistate.overmap_blinking) {
                    uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
                }
                if (action == "NEXT_TAB") {
                    i = (i + 1) % locations.size();
                } else if (action == "PREV_TAB") {
                    i = (i + locations.size() - 1) % locations.size();
                } else if (action == "CONFIRM") {
                    curs = tmp;
                }
            } while(action != "CONFIRM" && action != "QUIT");
            delwin(w_search);
            action = "";
        } else if( action == "PLACE_TERRAIN" || action == "PLACE_SPECIAL" ) {
            uimenu pmenu;
            // This simplifies overmap_special selection using uimenu
            std::vector<const overmap_special *> oslist;
            const bool terrain = action == "PLACE_TERRAIN";

            if( terrain ) {
                pmenu.title = "Select terrain to place:";
                for( const auto &oter : terrains.get_all() ) {
                    pmenu.addentry( oter.id.id(), true, 0, oter.id.str() );
                }
            } else {
                pmenu.title = "Select special to place:";
                for( const auto &elem : specials.get_all() ) {
                    oslist.push_back( &elem );
                    pmenu.addentry( oslist.size()-1, true, 0, elem.id.str() );
                }
            }
            pmenu.return_invalid = true;
            pmenu.query();

            if( pmenu.ret >= 0 ) {
                WINDOW *w_editor = newwin( 15, 27, 3, TERMX - 27 );
                input_context ctxt( "OVERMAP_EDITOR" );
                ctxt.register_directions();
                ctxt.register_action( "CONFIRM" );
                ctxt.register_action( "ROTATE" );
                ctxt.register_action( "QUIT" );
                ctxt.register_action( "ANY_INPUT" );

                if( terrain ) {
                    uistate.place_terrain = &oter_id( pmenu.ret ).obj();
                } else {
                    uistate.place_special = oslist[pmenu.ret];
                }
                // @todo Unify these things.
                const bool can_rotate = terrain ? uistate.place_terrain->is_rotatable() : uistate.place_special->rotatable;

                uistate.omedit_rotation = om_direction::type::none;
                // If user chose an already rotated submap, figure out its direction
                if( terrain && can_rotate ) {
                    for( auto r : om_direction::all ) {
                        if( uistate.place_terrain->id.id() == uistate.place_terrain->get_rotated( r ) ) {
                            uistate.omedit_rotation = r;
                            break;
                        }
                    }
                }

                do {
                    // overmap::draw will handle actually showing the preview
                    draw( g->w_overmap, g->w_omlegend, curs, orig, uistate.overmap_show_overlays,
                          show_explored, NULL, draw_data_t() );

                    draw_border( w_editor );
                    if( terrain ) {
                        mvwprintz( w_editor, 1, 1, c_white, _("Place overmap terrain:") );
                        mvwprintz( w_editor, 2, 1, c_ltblue, "                         " );
                        mvwprintz( w_editor, 2, 1, c_ltblue, uistate.place_terrain->id.c_str() );
                    } else {
                        mvwprintz( w_editor, 1, 1, c_white, _("Place overmap special:") );
                        mvwprintz( w_editor, 2, 1, c_ltblue, "                         " );
                        mvwprintz( w_editor, 2, 1, c_ltblue, uistate.place_special->id.c_str() );
                    }
                    const std::string rotation = om_direction::name( uistate.omedit_rotation );

                    mvwprintz( w_editor, 3, 1, c_ltgray, "                         " );
                    mvwprintz( w_editor, 3, 1, c_ltgray, _("Rotation: %s %s"), rotation.c_str(), can_rotate ? "" : _( "(fixed)" ) );
                    mvwprintz( w_editor, 5, 1, c_red, _("Areas highlighted in red") );
                    mvwprintz( w_editor, 6, 1, c_red, _("already have map content") );
                    mvwprintz( w_editor, 7, 1, c_red, _("generated. Their overmap") );
                    mvwprintz( w_editor, 8, 1, c_red, _("id will change, but not") );
                    mvwprintz( w_editor, 9, 1, c_red, _("their contents.") );
                    if( ( terrain && uistate.place_terrain->is_rotatable() ) ||
                        ( !terrain && uistate.place_special->rotatable ) ) {
                        mvwprintz( w_editor, 11, 1, c_white, _("[%s] Rotate"),
                                   ctxt.get_desc( "ROTATE" ).c_str() );
                    }
                    mvwprintz( w_editor, 12, 1, c_white, _("[%s] Apply"),
                               ctxt.get_desc( "CONFIRM" ).c_str() );
                    mvwprintz( w_editor, 13, 1, c_white, _("[ESCAPE/Q] Cancel") );
                    wrefresh( w_editor );

                    action = ctxt.handle_input( BLINK_SPEED );

                    if( ictxt.get_direction( dirx, diry, action ) ) {
                        curs.x += dirx;
                        curs.y += diry;
                    } else if( action == "CONFIRM" ) { // Actually modify the overmap
                        if( terrain ) {
                            overmap_buffer.ter( curs ) = uistate.place_terrain->id.id();
                            overmap_buffer.set_seen( curs.x, curs.y, curs.z, true );
                        } else {
                            for( const auto &s_ter : uistate.place_special->terrains ) {
                                const tripoint pos = curs + om_direction::rotate( s_ter.p, uistate.omedit_rotation );

                                overmap_buffer.ter( pos ) = s_ter.terrain->get_rotated( uistate.omedit_rotation );
                                overmap_buffer.set_seen( pos.x, pos.y, pos.z, true );
                            }
                        }
                        break;
                    } else if( action == "ROTATE" && can_rotate ) {
                        uistate.omedit_rotation = om_direction::turn_right( uistate.omedit_rotation );
                        if( terrain ) {
                            uistate.place_terrain = &uistate.place_terrain->get_rotated( uistate.omedit_rotation ).obj();
                        }
                    }
                    if( uistate.overmap_blinking ) {
                        uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
                    }
                } while( action != "QUIT" );

                uistate.place_terrain = nullptr;
                uistate.place_special = nullptr;
                delwin( w_editor );
                action = "";
            }
        } else if (action == "TIMEOUT") {
            if (uistate.overmap_blinking) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
            }
        } else if (action == "ANY_INPUT") {
            if (uistate.overmap_blinking) {
                uistate.overmap_show_overlays = !uistate.overmap_show_overlays;
            }
        }
    } while (action != "QUIT" && action != "CONFIRM");
    werase(g->w_overmap);
    werase(g->w_omlegend);
    erase();
    g->refresh_all();
    return ret;
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
            mg.population = (mg.population * 4) / 5;
            mg.radius = (mg.radius * 9) / 10;
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
        for(const city &check_city : om.cities ) {
            // Check if this is the nearest city so far.
            int distance = rl_dist( check_city.x * 2, check_city.y * 2, pos.x, pos.y );
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
        target.x = target_city->x * 2 + rng( -5, 5 );
        target.y = target_city->y * 2 + rng( -5, 5 );
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
    decltype(zg) tmpzg;
    //MOVE ZOMBIE GROUPS
    for( auto it = zg.begin(); it != zg.end(); ) {
        mongroup &mg = it->second;
        if( !mg.horde ) {
            ++it;
            continue;
        }

        if(mg.horde_behaviour == "") {
            mg.horde_behaviour = one_in(2) ? "city" : "roam";
        }

        // Gradually decrease interest.
        mg.dec_interest( 1 );

        if( (mg.pos.x == mg.target.x && mg.pos.y == mg.target.y) || mg.interest <= 15 ) {
            mg.wander(*this);
        }

        // Decrease movement chance according to the terrain we're currently on.
        const oter_id& walked_into = ter(mg.pos.x, mg.pos.y, mg.pos.z);
        int movement_chance = 1;
        if(walked_into == ot_forest || walked_into == ot_forest_water) {
            movement_chance = 3;
        } else if(walked_into == ot_forest_thick) {
            movement_chance = 6;
        } else if(walked_into == ot_river_center) {
            movement_chance = 10;
        }

        if( one_in(movement_chance) && rng(0, 100) < mg.interest ) {
            // TODO: Adjust for monster speed.
            // TODO: Handle moving to adjacent overmaps.
            if( mg.pos.x > mg.target.x) {
                mg.pos.x--;
            }
            if( mg.pos.x < mg.target.x) {
                mg.pos.x++;
            }
            if( mg.pos.y > mg.target.y) {
                mg.pos.y--;
            }
            if( mg.pos.y < mg.target.y) {
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


    if(get_option<bool>( "WANDER_SPAWNS" ) ) {
        static const mongroup_id GROUP_ZOMBIE("GROUP_ZOMBIE");

        // Re-absorb zombies into hordes.
        // Scan over monsters outside the player's view and place them back into hordes.
        auto monster_map_it = monster_map.begin();
        while(monster_map_it != monster_map.end()) {
            const auto& p = monster_map_it->first;
            auto& this_monster = monster_map_it->second;

            // Only zombies on z-level 0 may join hordes.
            if( p.z != 0 ) {
                monster_map_it++;
                continue;
            }

            // Check if the monster is a zombie.
            auto& type = *(this_monster.type);
            if(
                !type.species.count(species_id("ZOMBIE")) || // Only add zombies to hordes.
                type.id == mtype_id("mon_jabberwock") || // Jabberwockies are an exception.
                this_monster.has_effect( effect_pet ) || // "Zombie pet" zlaves are, too.
                this_monster.mission_id != -1 // We mustn't delete monsters that are related to missions.
            ) {
                // Don't delete the monster, just increment the iterator.
                monster_map_it++;
                continue;
            }

            // Scan for compatible hordes in this area.
            mongroup *add_to_group = NULL;
            auto group_bucket = zg.equal_range(p);
            std::for_each( group_bucket.first, group_bucket.second,
                [&](std::pair<const tripoint, mongroup> &horde_entry ) {
                mongroup &horde = horde_entry.second;

                // We only absorb zombies into GROUP_ZOMBIE hordes
                if(horde.horde && !horde.monsters.empty() && horde.type == GROUP_ZOMBIE) {
                    add_to_group = &horde;
                }
            });

            // If there is no horde to add the monster to, create one.
            if(add_to_group == NULL) {
                mongroup m(GROUP_ZOMBIE, p.x, p.y, p.z, 1, 0);
                m.horde = true;
                m.monsters.push_back(this_monster);
                m.interest = 0; // Ensures that we will select a new target.
                add_mon_group( m );
            } else {
                add_to_group->monsters.push_back(this_monster);
            }

            // Delete the monster, continue iterating.
            monster_map_it = monster_map.erase(monster_map_it);
        }
    }
}

/**
* @param p location of signal
* @param sig_power - power of signal or max distantion for reaction of zombies
*/
void overmap::signal_hordes( const tripoint &p, const int sig_power)
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
            // TODO: base this in monster attributes, foremost GOODHEARING.
            const int inter_per_sig_power = 15; //Interest per signal value
            const int min_initial_inter = 30; //Min initial interest for horde
            const int calculated_inter = ( sig_power + 1 - dist ) * inter_per_sig_power; // Calculated interest
            const int roll = rng( 0, mg.interest );
            // Minimum capped calculated interest. Used to give horde enough interest to really investigate the target at start.
            const int min_capped_inter = std::max( min_initial_inter, calculated_inter );
            if( roll < min_capped_inter ) { //Rolling if horde interested in new signal
                // TODO: Z coord for mongroup targets
                const int targ_dist = rl_dist( p, mg.target );
                // TODO: Base this on targ_dist:dist ratio.
                if ( targ_dist < 5 ) { // If signal source already pursued by horde
                    mg.set_target( (mg.target.x + p.x) / 2, (mg.target.y + p.y) / 2 );
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

void grow_forest_oter_id(oter_id &oid, bool swampy)
{
    if (swampy && ( oid == ot_field || oid == ot_forest ) ) {
        oid = ot_forest_water;
    } else if ( oid == ot_forest ) {
        oid = ot_forest_thick;
    } else if ( oid == ot_field ) {
        oid = ot_forest;
    }
}

void overmap::place_forest()
{
    int forests_placed = 0;
    for (int i = 0; i < settings.num_forests; i++) {
        int forx, fory, fors;
        // try to place this forest
        int tries = 100;
        do {
            // forx and fory determine the epicenter of the forest
            forx = rng(0, OMAPX - 1);
            fory = rng(0, OMAPY - 1);
            // fors determinds its basic size
            fors = rng(settings.forest_size_min, settings.forest_size_max);
            const auto iter = std::find_if(
                cities.begin(),
                cities.end(),
                [&](const city &c) {
                    return
                        // is this city too close?
                        trig_dist(forx, fory, c.x, c.y) - fors / 2 < c.s &&
                        // occasionally accept near a city if we've been failing
                        tries > rng(-1000/(i-forests_placed+1),2);
                }
            );
            if(iter == cities.end()) { // every city was too close
                break;
            }
        } while( tries-- );

        // if we gave up, don't bother trying to place another forest
        if (tries == 0) {
            break;
        }

        forests_placed++;

        int swamps = settings.swamp_maxsize; // How big the swamp may be...
        int x = forx;
        int y = fory;

        // Depending on the size on the forest...
        for (int j = 0; j < fors; j++) {
            int swamp_chance = 0;
            for (int k = -2; k <= 2; k++) {
                for (int l = -2; l <= 2; l++) {
                    if (ter(x + k, y + l, 0) == "forest_water" ||
                        check_ot_type("river", x + k, y + l, 0)) {
                        swamp_chance += settings.swamp_river_influence;
                    }
                }
            }
            bool swampy = false;
            if (swamps > 0 && swamp_chance > 0 && !one_in(swamp_chance) &&
                (ter(x, y, 0) == "forest" || ter(x, y, 0) == "forest_thick" ||
                 ter(x, y, 0) == "field" || one_in( settings.swamp_spread_chance ))) {
                // ...and make a swamp.
                ter(x, y, 0) = oter_id( "forest_water" );
                swampy = true;
                swamps--;
            } else if (swamp_chance == 0) {
                swamps = settings.swamp_maxsize;
            }

            // Place or embiggen forest
            for ( int mx = -1; mx < 2; mx++ ) {
                for ( int my = -1; my < 2; my++ ) {
                    grow_forest_oter_id( ter(x + mx, y + my, 0),
                                         ( mx == 0 && my == 0 ? false : swampy ) );
                }
            }
            // Random walk our forest
            x += rng(-2, 2);
            if (x < 0    ) {
                x = 0;
            }
            if (x > OMAPX) {
                x = OMAPX;
            }
            y += rng(-2, 2);
            if (y < 0    ) {
                y = 0;
            }
            if (y > OMAPY) {
                y = OMAPY;
            }
        }
    }
}

void overmap::place_river(point pa, point pb)
{
    int x = pa.x, y = pa.y;
    do {
        x += rng(-1, 1);
        y += rng(-1, 1);
        if (x < 0) {
            x = 0;
        }
        if (x > OMAPX - 1) {
            x = OMAPX - 1;
        }
        if (y < 0) {
            y = 0;
        }
        if (y > OMAPY - 1) {
            y = OMAPY - 1;
        }
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                if (y + i >= 0 && y + i < OMAPY && x + j >= 0 && x + j < OMAPX) {
                    ter(x + j, y + i, 0) = oter_id( "river_center" );
                }
            }
        }
        if (pb.x > x && (rng(0, int(OMAPX * 1.2) - 1) < pb.x - x ||
                         (rng(0, int(OMAPX * .2) - 1) > pb.x - x &&
                          rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y)))) {
            x++;
        }
        if (pb.x < x && (rng(0, int(OMAPX * 1.2) - 1) < x - pb.x ||
                         (rng(0, int(OMAPX * .2) - 1) > x - pb.x &&
                          rng(0, int(OMAPY * .2) - 1) > abs(pb.y - y)))) {
            x--;
        }
        if (pb.y > y && (rng(0, int(OMAPY * 1.2) - 1) < pb.y - y ||
                         (rng(0, int(OMAPY * .2) - 1) > pb.y - y &&
                          rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x)))) {
            y++;
        }
        if (pb.y < y && (rng(0, int(OMAPY * 1.2) - 1) < y - pb.y ||
                         (rng(0, int(OMAPY * .2) - 1) > y - pb.y &&
                          rng(0, int(OMAPX * .2) - 1) > abs(x - pb.x)))) {
            y--;
        }
        x += rng(-1, 1);
        y += rng(-1, 1);
        if (x < 0) {
            x = 0;
        }
        if (x > OMAPX - 1) {
            x = OMAPX - 2;
        }
        if (y < 0) {
            y = 0;
        }
        if (y > OMAPY - 1) {
            y = OMAPY - 1;
        }
        for (int i = -1; i <= 1; i++) {
            for (int j = -1; j <= 1; j++) {
                // We don't want our riverbanks touching the edge of the map for many reasons
                if( inbounds( x + j, y + i, 0, 1 ) ||
                    // UNLESS, of course, that's where the river is headed!
                    (abs(pb.y - (y + i)) < 4 && abs(pb.x - (x + j)) < 4)) {
                    ter(x + j, y + i, 0) = oter_id( "river_center" );
                }
            }
        }
    } while (pb.x != x || pb.y != y);
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
    const double city_map_coverage_ratio = 1.0/std::pow(2.0, op_city_spacing);
    const double omts_per_city = (op_city_size*2+1) * (op_city_size*2+1) * 3 / 4;

    // how many cities on this overmap?
    const int NUM_CITIES =
        roll_remainder(omts_per_overmap * city_map_coverage_ratio / omts_per_city);

    // place a seed for NUM_CITIES cities, and maybe one more
    while ( cities.size() < size_t(NUM_CITIES) ) {
        // randomly make some cities smaller or larger
        int size = rng(op_city_size-1, op_city_size+1);
        if(one_in(3)) {          // 33% tiny
            size = 1;
        } else if (one_in(2)) {  // 33% small
            size = size * 2 / 3;
        } else if (one_in(2)) {  // 17% large
            size = size * 3 / 2;
        } else {                 // 17% huge
            size = size * 2;
        }
        size = std::max(size,1);

        // TODO put cities closer to the edge when they can span overmaps
        // don't draw cities across the edge of the map, they will get clipped
        int cx = rng(size - 1, OMAPX - size);
        int cy = rng(size - 1, OMAPY - size);
        if (ter(cx, cy, 0) == settings.default_oter ) {
            ter(cx, cy, 0) = oter_id( "road_nesw" ); // every city starts with an intersection
            city tmp;
            tmp.x = cx;
            tmp.y = cy;
            tmp.s = size;
            cities.push_back(tmp);

            const auto start_dir = om_direction::random();
            auto cur_dir = start_dir;

            do {
                build_city_street( cx, cy, size, cur_dir, tmp );
            } while( ( cur_dir = om_direction::turn_right( cur_dir ) ) != start_dir );
        }
    }
}

void overmap::put_building( int x, int y, om_direction::type dir, const city &town )
{
    const point p = om_direction::displace( dir );
    auto &tid = ter( x + p.x, y + p.y, 0 );

    if( tid != settings.default_oter  ) {
        return;
    }

    const int town_dist = trig_dist( x, y, town.x, town.y ) / ( town.s > 0 ? town.s : 1 );
    oter_id building_tid;

    if( rng( 0, 99 ) > 80 * town_dist ) {
        building_tid = random_shop();
    } else if( rng( 0, 99 ) > 130 * town_dist ) {
        building_tid = random_park();
    } else {
        building_tid = random_house();
    }

    tid = building_tid->get_rotated( om_direction::opposite( dir ) );
}

void overmap::build_city_street( int x, int y, int cs, om_direction::type dir, const city &town )
{
    const oter_type_id road( "road" );

    int c = cs;
    int croad = cs;

    if( dir == om_direction::type::invalid ) {
        debugmsg( "Invalid road direction." );
        return;
    }

    const point bias = om_direction::displace( dir );  // Northern vector.

    // Grow in the stated direction, sprouting off sub-roads and placing buildings as we go.
    while( c > 0 && inbounds( x, y, 0, 1 ) &&
           (ter(x + bias.x, y + bias.y, 0)->has_flag( allow_override ) || c == cs) ) {
        x += bias.x;
        y += bias.y;
        c--;
        ter( x, y, 0 ) = road->get_first();
        // Look for a crossroad or a road ahead, if we find one,
        // set current tile to be road_null and c to -1 to prevent further branching.
        if( ter( x + bias.x, y + bias.y, 0 )->type_is( road ) ||
            ter( x + bias.y, y + bias.x, 0 )->type_is( road ) ||
            ter( x - bias.y, y - bias.x, 0 )->type_is( road ) ) {

            c = -1;
        }

        if( !one_in( STREETCHANCE ) ) {
            put_building( x, y, om_direction::turn_left( dir ), town );
        }
        if( !one_in( STREETCHANCE ) ) {
            put_building( x, y, om_direction::turn_right( dir ), town );
        }

        // Look to each side, and branch if the way is clear.
        if (c < croad - 1 && c >= 2 && ( ter(x + bias.y, y + bias.x, 0)->has_flag( allow_override ) &&
                                         ter(x - bias.y, y - bias.x, 0)->has_flag( allow_override ) ) ) {
            croad = c;
            build_city_street( x, y, cs - rng( 1, 3 ), om_direction::turn_left( dir ), town );
            build_city_street( x, y, cs - rng( 1, 3 ), om_direction::turn_right( dir ), town );
        }
    }
    // Now we're done growing, if there's a road ahead, add one more road segment to meet it.
    if( get_ter( x + bias.x, y + bias.y, 0 )->has_flag( allow_override ) &&
        get_ter( x + 2 * bias.x, y + 2 * bias.y, 0 )->can_connect_to( road->get_first() ) ) {

        ter( x + bias.x, y + bias.y, 0 ) = road->get_first();
    }

    // If we're big, make a right turn at the edge of town.
    // Seems to make little neighborhoods.
    cs -= rng(1, 3);
    if (cs >= 2 && c == 0) {
        const auto rnd_dir = om_direction::turn_random( dir );
        build_city_street( x, y, cs, rnd_dir, town );
        if(one_in(5)) {
            build_city_street( x, y, cs, om_direction::opposite( rnd_dir ), town );
        }
    }
}

bool overmap::build_lab( int x, int y, int z, int s, bool ice )
{
    std::vector<point> generated_lab;
    const oter_id labt( ice ? "ice_lab" : "lab" );
    const oter_id labt_stairs( labt.id().str() + "_stairs" );
    const oter_id labt_core( labt.id().str() + "_core" );
    const oter_id labt_finale( labt.id().str() + "_finale" );

    ter( x, y, z ) = labt;
    generated_lab.push_back( point( x, y ) );

    // maintain a list of potential new lab maps
    // grows outwards from previously placed lab maps
    std::set<point> candidates;
    candidates.insert( {point( x - 1, y ), point( x + 1, y ), point( x, y - 1 ), point( x, y + 1 )} );
    while( candidates.size() ) {
        auto cand = candidates.begin();
        const int &cx = cand->x;
        const int &cy = cand->y;
        int dist = abs( x - cx ) + abs( y - cy );
        if( dist <= s * 2 ) { // increase radius to compensate for sparser new algorithm
            if( one_in( dist / 2 + 1 ) ) { // odds diminish farther away from the stairs
                ter( cx, cy, z ) = labt;
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
        if( ter( elem.x, elem.y, z + 1 ) == labt_stairs ) {
            generate_stairs = false;
        }
    }
    if( generate_stairs && !generated_lab.empty() ) {
        const point p = random_entry( generated_lab );
        ter( p.x, p.y, z + 1 ) = labt_stairs;
    }

    ter( x, y, z ) = labt_core;
    int numstairs = 0;
    if( s > 0 ) { // Build stairs going down
        while( !one_in( 6 ) ) {
            int stairx, stairy;
            int tries = 0;
            do {
                stairx = rng( x - s, x + s );
                stairy = rng( y - s, y + s );
                tries++;
            } while( ter( stairx, stairy, z ) != labt && tries < 15 );
            if( tries < 15 ) {
                ter( stairx, stairy, z ) = labt_stairs;
                numstairs++;
            }
        }
    }
    if( numstairs == 0 ) { // This is the bottom of the lab;  We need a finale
        int finalex, finaley;
        int tries = 0;
        do {
            finalex = rng( x - s, x + s );
            finaley = rng( y - s, y + s );
            tries++;
        } while( tries < 15 && ter( finalex, finaley, z ) != labt
                  && ter( finalex, finaley, z ) != labt_core );
        ter( finalex, finaley, z ) = labt_finale;
    }

    return numstairs > 0;
}

void overmap::build_anthill(int x, int y, int z, int s)
{
    for( auto dir : om_direction::all ) {
        build_tunnel( x, y, z, s - rng(0, 3), dir );
    }

    std::vector<point> queenpoints;
    for (int i = x - s; i <= x + s; i++) {
        for (int j = y - s; j <= y + s; j++) {
            if (check_ot_type("ants", i, j, z)) {
                queenpoints.push_back(point(i, j));
            }
        }
    }
    const point target = random_entry( queenpoints );
    ter(target.x, target.y, z) = oter_id( "ants_queen" );

    // Connect the queen chamber, as it gets placed before polish()
    for( auto dir : om_direction::all ) {
        const point p = point( target.x, target.y ) + om_direction::displace( dir );
        if( check_ot_type( "ants", p.x, p.y, z ) ) {
            auto &neighbor = ter( p.x, p.y, z );
            if( neighbor->has_flag( line_drawing ) ) {
                size_t line = neighbor->get_line();
                line = om_lines::set_segment( line, om_direction::opposite( dir ) );
                if( line != neighbor->get_line() ) {
                    neighbor = neighbor->get_type_id()->get_linear( line );
                }
            }
        }
    }
}

void overmap::build_tunnel( int x, int y, int z, int s, om_direction::type dir )
{
    if (s <= 0) {
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
            if (one_in(s * 2)) {
                // Spawn a special chamber
                if (one_in(2)) {
                    ter( p.x, p.y, z ) = ants_food;
                } else {
                    ter( p.x, p.y, z ) = ants_larvae;
                }

                // Connect newly-spawned chamber to this tunnel segment
                auto &oter = ter( x, y, z );
                size_t line = oter->get_line();
                line = om_lines::set_segment( line, r );
                if( line != oter->get_line() ) {
                    oter = oter->get_type_id()->get_linear( line );
                }

            } else if (one_in(5)) {
                // Branch off a side tunnel
                build_tunnel( p.x, p.y, z, s - rng( 0, 3 ), r );
            }
        }
    }
    build_tunnel(next.x, next.y, z, s - 1, dir);
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

void overmap::build_mine(int x, int y, int z, int s)
{
    bool finale = s <= rng(1, 3);
    const oter_id mine( "mine" );
    const oter_id mine_finale_or_down( finale ? "mine_finale" : "mine_down" );
    const oter_id empty_rock( "empty_rock" );

    int built = 0;
    if (s < 2) {
        s = 2;
    }
    while (built < s) {
        ter(x, y, z) = mine;
        std::vector<point> next;
        for (int i = -1; i <= 1; i += 2) {
            if( ter( x, y + i, z ) == empty_rock ) {
                next.push_back( point(x, y + i) );
            }
            if( ter( x + i, y, z ) == empty_rock ) {
                next.push_back( point(x + i, y) );
            }
        }
        if (next.empty()) { // Dead end!  Go down!
            ter(x, y, z) = mine_finale_or_down;
            return;
        }
        const point p = random_entry( next );
        x = p.x;
        y = p.y;
        built++;
    }
    ter(x, y, z) = mine_finale_or_down;
}

void overmap::place_rifts(int const z)
{
    int num_rifts = rng(0, 2) * rng(0, 2);
    std::vector<point> riftline;
    if (!one_in(4)) {
        num_rifts++;
    }
    const oter_id hellmouth( "hellmouth" );
    const oter_id rift( "rift" );

    for (int n = 0; n < num_rifts; n++) {
        int x = rng(MAX_RIFT_SIZE, OMAPX - MAX_RIFT_SIZE);
        int y = rng(MAX_RIFT_SIZE, OMAPY - MAX_RIFT_SIZE);
        int xdist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE),
            ydist = rng(MIN_RIFT_SIZE, MAX_RIFT_SIZE);
        // We use rng(0, 10) as the t-value for this Bresenham Line, because by
        // repeating this twice, we can get a thick line, and a more interesting rift.
        for (int o = 0; o < 3; o++) {
            if (xdist > ydist) {
                riftline = line_to(x - xdist, y - ydist + o, x + xdist, y + ydist, rng(0, 10));
            } else {
                riftline = line_to(x - xdist + o, y - ydist, x + xdist, y + ydist, rng(0, 10));
            }
            for (size_t i = 0; i < riftline.size(); i++) {
                if (i == riftline.size() / 2 && !one_in(3)) {
                    ter(riftline[i].x, riftline[i].y, z) = hellmouth;
                } else {
                    ter(riftline[i].x, riftline[i].y, z) = rift;
                }
            }
        }
    }
}

void overmap::build_connection( const point &source, const point &dest, int z, const int_id<oter_type_t> &type_id )
{
    const int disp = type_id == oter_type_id( "road" ) ? 5 : 2;

    const auto estimate = [&]( const pf::node &prev, const pf::node &cur ) {
        // Reject nodes that don't allow roads to cross them (e.g. buildings)
        const auto &id( ter( cur.x, cur.y, z ) );

        if( !road_allowed( id ) ) {
            return -1;
        }
        // Reject nodes that make corners on the river
        if( prev.dir != cur.dir && ( is_river( ter( prev.x, prev.y, z ) ) ||
                                     is_river( ter( cur.x, cur.y, z ) ) ) ) {
            return -1;
        }

        int res = ( std::abs( dest.x - cur.x ) + std::abs( dest.y - cur.y ) ) / disp;
        // Prefer existing roads.
        res += id->type_is( type_id ) ? 0 : 3;
        // Prefer flat land over bridges
        res += !is_river( id ) ? 0 : 2;
        // Try not to turn too much
        //res += (mn.d == d) ? 0 : 1;
        return res;
    };

    const oter_id bridge_ns( "bridge_ns" );
    const oter_id bridge_ew( "bridge_ew" );

    for( const auto &node : pf::find_path( source, dest, OMAPX, OMAPY, estimate ) ) {
        auto &id( ter( node.x, node.y, z ) );

        if( is_river( id ) ) {
            id = node.dir == 1 || node.dir == 3 ? bridge_ns : bridge_ew;
        } else {
            // @todo Eliminate discrepancy which requires casting and rotation. That is, make 'node' support 'om_direction'.
            const om_direction::type dir( om_direction::turn_left( static_cast<om_direction::type>( node.dir ) ) );
            const size_t prev_line( id->type_is( type_id ) ? id->get_line() : 0 );

            id = type_id->get_linear( om_lines::set_segment( prev_line, dir ) );
        }
    }
}

void overmap::connect_closest_points( const std::vector<point> &points, int z, const int_id<oter_type_t> &type_id )
{
    if( points.size() == 1 ) {
        return;
    }
    for( size_t i = 0; i < points.size(); ++i ) {
        int closest = -1;
        int k;
        for( size_t j = i + 1; j < points.size(); j++ ) {
            const int distance = trig_dist( points[i].x, points[i].y, points[j].x, points[j].y );
            if( distance < closest || closest < 0) {
                closest = distance;
                k = j;
            }
        }
        if( closest > 0 ) {
            build_connection( points[i], points[k], z, type_id );
        }
    }
}

// Polish does both good_roads and good_rivers (and any future polishing) in
// a single loop; much more efficient
void overmap::polish(const int z, const std::string &terrain_type)
{
    const bool check_all = (terrain_type == "all");

    const oter_type_t &target_type( *oter_type_id( check_all ? std::string() : terrain_type ) );
    const oter_type_t &road_type( *oter_type_id( "road" ) );

    const oter_type_t &bridge_ns_type( *oter_type_id( "bridge_ns" ) );
    const oter_type_t &bridge_ew_type( *oter_type_id( "bridge_ew" ) );

    const oter_id road_nesw( "road_nesw" );
    const oter_id road_mahole( "road_nesw_manhole" );

    // @todo Get rid of the special case.
    const auto is_bridge = [&]( const oter_t &ter ) {
        return ter.type_is( bridge_ns_type ) || ter.type_is( bridge_ew_type );
    };

    // Main loop--checks roads and rivers that aren't on the borders of the map
    for (int x = 0; x < OMAPX; x++) {
        for (int y = 0; y < OMAPY; y++) {
            auto &oter = ter( x, y, z );
            auto &oter_obj = *oter;

            if( check_all || oter_obj.type_is( target_type ) ) {
                if( oter_obj.has_flag( line_drawing ) ) {
                    oter = good_connection( oter_obj, tripoint( x, y, z ) );

                    if( one_in( 4 ) && oter == road_nesw ) {
                        oter = road_mahole;
                    }
                } else if( is_bridge( oter_obj ) ) {
                    if( is_bridge( *ter( x - 1, y, z ) ) &&
                        is_bridge( *ter( x + 1, y, z ) ) &&
                        is_bridge( *ter( x, y - 1, z ) ) &&
                        is_bridge( *ter( x, y + 1, z ) ) ) {

                        oter = road_nesw;
                    } else if( ( oter_obj.type_is( bridge_ns_type ) && ( !is_river( ter( x - 1, y, z ) ) ||
                                                                         !is_river( ter( x + 1, y, z ) ) ) ) ||
                               ( oter_obj.type_is( bridge_ew_type ) && ( !is_river( ter( x, y - 1, z ) ) ||
                                                                         !is_river( ter( x, y + 1, z ) ) ) ) ) {
                        // Sometimes a bridge will start at the edge of a river,
                        // and this looks ugly.
                        // So, fix it by making that square normal road;
                        // also taking other road pieces that may be next
                        // to it into account. A bit of a kludge but it works.
                        oter = good_connection( *oter_id( "road_isolated" ), tripoint( x, y, z ) );
                    }
                } else if( is_ot_type( "river", oter ) ) {
                    good_river(x, y, z);
                }
            }
        }
    }

    if( !check_all && target_type.id != road_type.id ) {
        return; // Nothing to do here anymore.
    }

    // Fixes stretches of parallel roads--turns them into two-lane highways
    // Note that this fixes 2x2 areas...
    // a "tail" of 1x2 parallel roads may be left.
    // This can actually be a good thing; it ensures nice connections
    // Also, this leaves, say, 3x3 areas of road.
    // TODO: fix this?  courtyards etc?
    for (int y = 0; y < OMAPY - 1; y++) {
        for (int x = 0; x < OMAPX - 1; x++) {
            auto &oter = ter( x, y, z );
            auto &oter_obj = *oter;

            if( check_all || oter_obj.type_is( target_type ) ) {
                if ( oter == "road_nes"
                    && ter(x + 1, y, z) == "road_nsw"
                    && ter(x, y + 1, z) == "road_nes"
                    && ter(x + 1, y + 1, z) == "road_nsw") {
                    oter = oter_id( "hiway_ns" );
                    ter(x + 1, y, z) = oter_id( "hiway_ns" );
                    ter(x, y + 1, z) = oter_id( "hiway_ns" );
                    ter(x + 1, y + 1, z) = oter_id( "hiway_ns" );
                } else if ( oter == "road_esw"
                           && ter(x + 1, y, z) == "road_esw"
                           && ter(x, y + 1, z) == "road_new"
                           && ter(x + 1, y + 1, z) == "road_new" ) {
                    oter = oter_id( "hiway_ew" );
                    ter(x + 1, y, z) = oter_id( "hiway_ew" );
                    ter(x, y + 1, z) = oter_id( "hiway_ew" );
                    ter(x + 1, y + 1, z) = oter_id( "hiway_ew" );
                }
            }
        }
    }
}

// Changes neighboring empty rock to partial rock
void overmap::chip_rock(int x, int y, int z)
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

bool overmap::check_ot_type(const std::string &otype, int x, int y, int z) const
{
    const oter_id oter = get_ter(x, y, z);
    return is_ot_type(otype, oter);
}

oter_id overmap::good_connection( const oter_t &oter, const tripoint &p )
{
    size_t line = oter.get_line();

    for( const auto dir : om_direction::all ) {
        const tripoint np( p + om_direction::displace( dir ) );
        // Always connect to outbound tiles.
        if( !inbounds( np ) || oter.can_connect_to( get_ter( np ) ) ) {
            line = om_lines::set_segment( line, dir );
        }
    }

    return line == oter.get_line() ? oter.id : oter.get_type_id()->get_linear( line );
}

void overmap::good_river(int x, int y, int z)
{
    if((x == 0) || (x == OMAPX-1)) {
        if(!is_river(ter(x, y - 1, z))) {
            ter(x, y, z) = oter_id( "river_north" );
        } else if(!is_river(ter(x, y + 1, z))) {
            ter(x, y, z) = oter_id( "river_south" );
        } else {
            ter(x, y, z) = oter_id( "river_center" );
        }
        return;
    }
    if((y == 0) || (y == OMAPY-1)) {
        if(!is_river(ter(x - 1, y, z))) {
            ter(x, y, z) = oter_id( "river_west" );
        } else if(!is_river(ter(x + 1, y, z))) {
            ter(x, y, z) = oter_id( "river_east" );
        } else {
            ter(x, y, z) = oter_id( "river_center" );
        }
        return;
    }
    if (is_river(ter(x - 1, y, z))) {
        if (is_river(ter(x, y - 1, z))) {
            if (is_river(ter(x, y + 1, z))) {
                if (is_river(ter(x + 1, y, z))) {
                    // River on N, S, E, W;
                    // but we might need to take a "bite" out of the corner
                    if (!is_river(ter(x - 1, y - 1, z))) {
                        ter(x, y, z) = oter_id( "river_c_not_nw" );
                    } else if (!is_river(ter(x + 1, y - 1, z))) {
                        ter(x, y, z) = oter_id( "river_c_not_ne");
                    } else if (!is_river(ter(x - 1, y + 1, z))) {
                        ter(x, y, z) = oter_id( "river_c_not_sw" );
                    } else if (!is_river(ter(x + 1, y + 1, z))) {
                        ter(x, y, z) = oter_id( "river_c_not_se" );
                    } else {
                        ter(x, y, z) = oter_id( "river_center" );
                    }
                } else {
                    ter(x, y, z) = oter_id( "river_east" );
                }
            } else {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = oter_id( "river_south" );
                } else {
                    ter(x, y, z) = oter_id( "river_se" );
                }
            }
        } else {
            if (is_river(ter(x, y + 1, z))) {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = oter_id( "river_north" );
                } else {
                    ter(x, y, z) = oter_id( "river_ne" );
                }
            } else {
                if (is_river(ter(x + 1, y, z))) { // Means it's swampy
                    ter(x, y, z) = oter_id( "forest_water" );
                }
            }
        }
    } else {
        if (is_river(ter(x, y - 1, z))) {
            if (is_river(ter(x, y + 1, z))) {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = oter_id( "river_west" );
                } else { // Should never happen
                    ter(x, y, z) = oter_id( "forest_water" );
                }
            } else {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = oter_id( "river_sw" );
                } else { // Should never happen
                    ter(x, y, z) = oter_id( "forest_water" );
                }
            }
        } else {
            if (is_river(ter(x, y + 1, z))) {
                if (is_river(ter(x + 1, y, z))) {
                    ter(x, y, z) = oter_id( "river_nw" );
                } else { // Should never happen
                    ter(x, y, z) = oter_id( "forest_water" );
                }
            } else { // Should never happen
                ter(x, y, z) = oter_id( "forest_water" );
            }
        }
    }

}

const std::string &om_direction::id( type dir )
{
    static const std::array<std::string, size + 1> ids = {{
       "", "north", "east", "south", "west"
    }};
    if( dir == type::invalid ) {
        debugmsg( "Invalid direction cannot have an id." );
    }
    return ids[static_cast<size_t>( dir ) + 1];
}

const std::string &om_direction::name( type dir )
{
    static const std::array<std::string, size + 1> names = {{
       _( "invalid" ), _( "north" ), _( "east" ), _( "south" ), _( "west" )
    }};
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
            debugmsg( "Invalid overmap rotation (%d).", dir );
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

om_direction::type overmap::random_special_rotation( const overmap_special &special, const tripoint &p ) const
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
            } else if( !oter || con.existing || !road_allowed( oter ) ) {
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
    std::random_shuffle( first, last );
    const auto rotation = find_if( first, last, [&]( om_direction::type r ) {
        for( const auto &elem : special.terrains ) {
            const tripoint rp = p + om_direction::rotate( elem.p, r );
            if( !inbounds( rp, 1 ) || ( rp.z == 0 && !special.can_be_placed_on( get_ter( rp ) ) ) ) {
                return false;
            }
        }
        return true;
    } );

    return rotation != last ? *rotation : om_direction::type::invalid;
}

// checks around the selected point to see if the special can be placed there
void overmap::place_special( const overmap_special &special, const tripoint &p, om_direction::type dir, const city &cit )
{
    assert( p != invalid_tripoint );
    assert( dir != om_direction::type::invalid );

    const bool blob = special.flags.count( "BLOB" ) > 0;

    for( const auto &elem : special.terrains ) {
        const tripoint location = p + om_direction::rotate( elem.p, dir );
        const oter_id tid = elem.terrain->get_rotated( dir );

        ter( location.x, location.y, location.z ) = tid;

        if( blob ) {
            for (int x = -2; x <= 2; x++) {
                for (int y = -2; y <= 2; y++) {
                    if (one_in(1 + abs(x) + abs(y))) {
                        ter( location.x + x, location.y + y, location.z ) = tid;
                    }
                }
            }
        }
    }
    // Make connections.
    if( cit ) {
        for( const auto &elem : special.connections ) {
            const tripoint rp( p + om_direction::rotate( elem.p, dir ) );
            build_connection( point( rp.x, rp.y ), point( cit.x, cit.y ), elem.p.z, elem.terrain.id() );
        }
    }
    // Place spawns.
    if( special.spawns.group ) {
        const overmap_special_spawns& spawns = special.spawns;
        const int pop = rng( spawns.population.min, spawns.population.max );
        const int rad = rng( spawns.radius.min, spawns.radius.max );
        add_mon_group(mongroup(spawns.group, p.x * 2, p.y * 2, p.z, rad, pop));
    }
}

std::vector<const overmap_special *> overmap::get_enabled_specials() const
{
    const bool only_classic = get_option<bool>( "CLASSIC_ZOMBIES" );
    std::vector<const overmap_special *> res;

    res.reserve( specials.size() );
    for( const auto &elem : specials.get_all() ) {
        if( !elem.locations.empty() && ( !only_classic || elem.flags.count( "CLASSIC" ) > 0 ) ) {
            res.push_back( &elem );
        }
    }
    return res;
}

std::vector<point> overmap::get_sectors() const
{
    std::vector<point> res;

    res.reserve( ( OMAPX / OMSPEC_FREQ ) * ( OMAPY / OMSPEC_FREQ ) );
    for( int x = 0; x < OMAPX; x += OMSPEC_FREQ ) {
        for( int y = 0; y < OMAPY; y += OMSPEC_FREQ ) {
            res.emplace_back( x, y );
        }
    }
    std::random_shuffle( res.begin(), res.end() );
    return res;
}

bool overmap::place_special_attempt( std::vector<std::pair<const overmap_special *, int>> &candidates,
                                     const point &sector, bool check_cities )
{
    const int x = sector.x;
    const int y = sector.y;

    const tripoint p( rng( x, x + OMSPEC_FREQ - 1 ), rng( y, y + OMSPEC_FREQ - 1 ), 0 );
    const city &nearest_city = get_nearest_city( p );

    for( auto iter = candidates.begin(); iter != candidates.end(); ++iter ) {
        const auto &special = *iter->first;
        // City check is the fastest => it goes first.
        if( check_cities && !special.can_belong_to_city( p, nearest_city ) ) {
            continue;
        }
        // See if we can actually place the special there.
        const auto rotation = random_special_rotation( special, p );
        if( rotation == om_direction::type::invalid ) {
            continue;
        }

        place_special( special, p, rotation, nearest_city );

        if( --iter->second == 0 ) {
            if( candidates.empty() ) {
                return true; // Job done. Bail out.
            }
            iter = candidates.erase( iter );
        }

        return true;
    }

    return false;
}

void overmap::place_specials_pass( std::vector<std::pair<const overmap_special *, int>> &to_place,
                                   std::vector<point> &sectors, bool check_cities )
{
    // Walk over sectors in random order, to minimize "clumping"
    std::random_shuffle( sectors.begin(), sectors.end() );
    for( auto it = sectors.begin(); it != sectors.end(); ) {
        const point &cur_sector = *it;
        const size_t attempts = 10;
        bool placed = false;
        for( size_t i = 0; i < attempts; ++i ) {
            if( place_special_attempt( to_place, cur_sector, check_cities ) ) {
                placed = true;
                it = sectors.erase( it );
                if( to_place.empty() ) {
                    return; // Job done. Bail out.
                }

                // Refresh the permutation.
                std::random_shuffle( to_place.begin(), to_place.end() );
                break;
            }
        }

        if( !placed ) {
            it++;
        }
    }
}

// should work essentially the same as previously
// split map into sections, iterate through sections
// iterate through specials, check if special is valid
// pick & place special
void overmap::place_specials()
{
    /*
    This function uses pointers in to the @ref overmap_specials container.
    The pointers are assumed to be stable (overmap_specials should not be change during this
    function). Using pointers here is faster and has the same behavior as using the id of
    the special (which is supposed to be unique, like the pointers are).
    However, make sure to never copy the overmap_special object and to use the address of the copy,
    as this won't work as desired.
    */
    // Vectors of mandatory and optional specials. Int means number to place.
    std::vector<std::pair<const overmap_special *, int>> mandatory;
    std::vector<std::pair<const overmap_special *, int>> optional;

    // Fill the vectors with valid specials.
    for( const auto &elem : get_enabled_specials() ) {
        const int min = elem->occurrences.min;
        const int max = elem->occurrences.max;

        if( max == 0 || min > max ) {
            continue;
        }

        if( elem->flags.count( "UNIQUE" ) > 0 ) {
            if( rand() % max <= min ) {
                mandatory.emplace_back( elem, 1 );
            }
        } else {
            if( min > 0 ) {
                mandatory.emplace_back( elem, min );
            }
            // Put the rest to optionals.
            if( max > min ) {
                optional.emplace_back( elem, max - min );
            }
        }
    }

    // See if we mined anything.
    if( mandatory.empty() && optional.empty() ) {
        return; // Nothing to do.
    }
    // Make random permutations.
    std::random_shuffle( mandatory.begin(), mandatory.end() );
    std::random_shuffle( optional.begin(), optional.end() );
    std::vector<point> sectors = get_sectors();

    // First a regular mandatory pass
    place_specials_pass( mandatory, sectors, true );
    // Keep the mandatory vector to inform player about limits properly
    // Otherwise unplaced vector can be empty while the map is invalid, leading to WTF
    std::vector<std::pair<const overmap_special *, int>> mandatory_unlimited = mandatory;
    if( mandatory.empty() ) {
        current_validity = overmap_valid::valid;
    } else if( minimum_validity >= overmap_valid::unlimited ) {
        const std::string unplaced = enumerate_as_string( mandatory.begin(), mandatory.end(),
        []( const std::pair<const overmap_special *, int> &elem ) {
            return string_format( "%s (%d)", elem.first->id.c_str(), elem.second );
        } );
        dbg( D_WARNING ) << string_format( "couldn't place mandatory overmap specials during first pass (with city checks): %s.", unplaced.c_str() );

        // Couldn't place mandatory specials, try without city checks
        place_specials_pass( mandatory, sectors, false );
        current_validity = overmap_valid::unlimited;
    }

    place_specials_pass( optional, sectors, true );

    unplaced_mandatory_specials.clear();
    if( !mandatory.empty() ) {
        current_validity = overmap_valid::invalid;
    }

    const auto &unplaced = mandatory.empty() ? mandatory_unlimited : mandatory;
    if( !unplaced.empty() ) {
        const std::string unplaced_s = enumerate_as_string( mandatory.begin(), mandatory.end(),
        []( const std::pair<const overmap_special *, int> &elem ) {
            return string_format( "%s (%d)", elem.first->id.c_str(), elem.second );
        } );
        dbg( D_WARNING ) << string_format( "couldn't place mandatory overmap specials: %s.", unplaced_s.c_str() );
        std::transform( unplaced.begin(), unplaced.end(), std::back_inserter( unplaced_mandatory_specials ),
        []( const std::pair<const overmap_special *, int> &pr ) {
            return pr.first;
        } );
    }
}

void overmap::place_mongroups()
{
    // Cities are full of zombies
    for( auto &elem : cities ) {
        if( get_option<bool>( "WANDER_SPAWNS" ) ) {
            if( !one_in( 16 ) || elem.s > 5 ) {
                mongroup m( mongroup_id( "GROUP_ZOMBIE" ), ( elem.x * 2 ), ( elem.y * 2 ), 0, int( elem.s * 2.5 ),
                            elem.s * 80 );
//                m.set_target( zg.back().posx, zg.back().posy );
                m.horde = true;
                m.wander(*this);
                add_mon_group( m );
            }
        }
    }

    if (!get_option<bool>( "CLASSIC_ZOMBIES" ) ) {
        // Figure out where swamps are, and place swamp monsters
        for (int x = 3; x < OMAPX - 3; x += 7) {
            for (int y = 3; y < OMAPY - 3; y += 7) {
                int swamp_count = 0;
                for (int sx = x - 3; sx <= x + 3; sx++) {
                    for (int sy = y - 3; sy <= y + 3; sy++) {
                        if (ter(sx, sy, 0) == "forest_water") {
                            swamp_count += 2;
                        }
                    }
                }
                if (swamp_count >= 25)
                    add_mon_group(mongroup( mongroup_id( "GROUP_SWAMP" ), x * 2, y * 2, 0, 3,
                                          rng(swamp_count * 8, swamp_count * 25)));
            }
        }
    }

    if (!get_option<bool>( "CLASSIC_ZOMBIES" ) ) {
        // Figure out where rivers are, and place swamp monsters
        for (int x = 3; x < OMAPX - 3; x += 7) {
            for (int y = 3; y < OMAPY - 3; y += 7) {
                int river_count = 0;
                for (int sx = x - 3; sx <= x + 3; sx++) {
                    for (int sy = y - 3; sy <= y + 3; sy++) {
                        if (is_river(ter(sx, sy, 0))) {
                            river_count++;
                        }
                    }
                }
                if (river_count >= 25)
                    add_mon_group(mongroup( mongroup_id( "GROUP_RIVER" ), x * 2, y * 2, 0, 3,
                                          rng(river_count * 8, river_count * 25)));
            }
        }
    }

    if (!get_option<bool>( "CLASSIC_ZOMBIES" ) ) {
        // Place the "put me anywhere" groups
        int numgroups = rng(0, 3);
        for (int i = 0; i < numgroups; i++) {
            add_mon_group(mongroup( mongroup_id( "GROUP_WORM" ), rng(0, OMAPX * 2 - 1), rng(0, OMAPY * 2 - 1), 0,
                         rng(20, 40), rng(30, 50)));
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
    for (int i = 0; i < OMAPX; i++) {
        for (int j = 0; j < OMAPY; j++) {
            if (ter(i, j, 0) == "radio_tower") {
                int choice = rng(0, 2);
                switch(choice) {
                case 0:
                    message = string_format(_("This is emergency broadcast station %d%d.\
  Please proceed quickly and calmly to your designated evacuation point."), i, j);
                    radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH), message));
                    break;
                case 1:
                    radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH),
                                                 _("Head West.  All survivors, head West.  Help is waiting.")));
                    break;
                case 2:
                    radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH), "",
                                                 WEATHER_RADIO));
                    break;
                }
            } else if (ter(i, j, 0) == "lmoe") {
                message = string_format(_("This is automated emergency shelter beacon %d%d.\
  Supplies, amenities and shelter are stocked."), i, j);
                radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH) / 2,
                                             message));
            } else if (ter(i, j, 0) == "fema_entrance") {
                message = string_format(_("This is FEMA camp %d%d.\
  Supplies are limited, please bring supplemental food, water, and bedding.\
  This is FEMA camp %d%d.  A designated long-term emergency shelter."), i, j, i, j);
                radios.push_back(radio_tower(i * 2, j * 2, rng(RADIO_MIN_STRENGTH, RADIO_MAX_STRENGTH), message));
            }
        }
    }
}

void overmap::clear()
{
    clear_mon_groups();
    init_layers();
    radios.clear();
    for( npc *np : npcs ) {
        delete np;
    }

    npcs.clear();

    vehicles.clear();
    cities.clear();
    roads_out.clear();
    unplaced_mandatory_specials.clear();
    scents.clear();
    monster_map.clear();
    set_validity_from_settings();
    current_validity = overmap_valid::invalid;
}

void overmap::open()
{
    std::string const plrfilename = overmapbuffer::player_filename(loc.x, loc.y);
    std::string const terfilename = overmapbuffer::terrain_filename(loc.x, loc.y);

    using namespace std::placeholders;
    if( read_from_file_optional( terfilename, std::bind( &overmap::unserialize, this, _1 ) ) ) {
        read_from_file_optional( plrfilename, std::bind( &overmap::unserialize_view, this, _1 ) );
    } else { // No map exists!  Prepare neighbors, and generate one.
        std::vector<const overmap*> pointers;
        // Fetch south and north
        for (int i = -1; i <= 1; i += 2) {
            pointers.push_back(overmap_buffer.get_existing(loc.x, loc.y+i));
        }
        // Fetch east and west
        for (int i = -1; i <= 1; i += 2) {
            pointers.push_back(overmap_buffer.get_existing(loc.x+i, loc.y));
        }

        // pointers looks like (north, south, west, east)
        generate_outer( pointers[0], pointers[3], pointers[1], pointers[2] );
    }
}

void overmap::generate_outer( const overmap* north, const overmap* east, const overmap* south, const overmap* west )
{
    // This string is here because it is long and we don't want indents eating up precious space
    static const std::string menu_s = _(
"Couldn't generate overmap with current settings.\n"
"The following specials could not be placed:\n%s");
    current_validity = overmap_valid::invalid;
    do {
        int try_num = std::max( 1, get_option<int>( "OVERMAP_GENERATION_TRIES" ) );
        for( int i = 0; i < try_num && current_validity != overmap_valid::valid; i++ ) {
            clear();
            generate( north, east, south, west );
        }

        // Note: this should not be checked in a loop - only totally valid maps should exit the loop early
        if( current_validity >= minimum_validity ) {
            // Good enough
            break;
        }

        const auto &opt = get_option<std::string>( "ALLOW_INVALID_OVERMAPS" );
        if( g != nullptr && !test_mode && ( opt == "ask_invalid" || opt == "ask_unlimited" ) ) {
            uimenu askmenu;
            if( unplaced_mandatory_specials.empty() ) {
                debugmsg( "Map is invalid (%d < %d) but all mandatory specials are placed", (int)current_validity, (int)minimum_validity );
            }
            const std::string unplaced = enumerate_as_string( unplaced_mandatory_specials.begin(),
                unplaced_mandatory_specials.end(), []( const overmap_special *special ) {
                return special->id.str();
            } );
            askmenu.text = string_format( menu_s.c_str(), unplaced.c_str() );
            askmenu.return_invalid = true;
            enum aksmenu_rval : int {
                // Retry as many times as in settings
                RETRY = 0,
                // Lift restrictions
                LIFT = 1,
                // Open options screen, to allow user to change the world settings
                SETTINGS = 2,
                // Just allow bugged map
                ALLOW = 3
            };
            askmenu.addentry( RETRY, true, 'r', _( "Retry %d times" ), try_num );
            if( minimum_validity != overmap_valid::unlimited ) {
                askmenu.addentry( LIFT, true, 'l', _( "Generate unlimited map" ) );
            }

            askmenu.addentry( SETTINGS, true, 's', _( "Change settings" ) );
            askmenu.addentry( ALLOW, true, 'a',
                              current_validity == overmap_valid::unlimited ?
                                                  _( "Allow current unlimited map" ) :
                                                  _( "Allow current invalid map" ) );
            askmenu.query();
            switch( askmenu.ret ) {
                case RETRY:
                    break;
                case LIFT:
                    minimum_validity = overmap_valid::unlimited;
                    break;
                case SETTINGS:
                    get_options().show( true );
                    break;
                case ALLOW:
                    minimum_validity = current_validity;
                    break;
                default:
                    break;
            }
        }
    } while( current_validity < minimum_validity );
}

// Note: this may throw io errors from std::ofstream
void overmap::save() const
{
    std::string const plrfilename = overmapbuffer::player_filename(loc.x, loc.y);
    std::string const terfilename = overmapbuffer::terrain_filename(loc.x, loc.y);

    ofstream_wrapper fout_player( plrfilename );
    serialize_view( fout_player );
    fout_player.close();

    ofstream_wrapper_exclusive fout_terrain( terfilename );
    serialize( fout_terrain );
    fout_terrain.close();
}


//////////////////////////

void groundcover_extra::setup()   // fixme return bool for failure
{
    default_ter = ter_id( default_ter_str );

    ter_furn_id tf_id;
    int wtotal = 0;
    int btotal = 0;

    for ( std::map<std::string, double>::const_iterator it = percent_str.begin();
          it != percent_str.end(); ++it ) {
        tf_id.ter = t_null;
        tf_id.furn = f_null;
        if ( it->second < 0.0001 ) {
            continue;
        }
        const ter_str_id tid( it->first );
        const furn_str_id fid( it->first );
        if( tid.is_valid() ) {
            tf_id.ter = tid.id();
        } else if( fid.is_valid() ) {
            tf_id.furn = fid.id();
        } else {
            debugmsg("No clue what '%s' is! No such terrain or furniture", it->first.c_str() );
            continue;
        }
        wtotal += (int)(it->second * 10000.0);
        weightlist[ wtotal ] = tf_id;
    }

    for ( std::map<std::string, double>::const_iterator it = boosted_percent_str.begin();
          it != boosted_percent_str.end(); ++it ) {
        tf_id.ter = t_null;
        tf_id.furn = f_null;
        if ( it->second < 0.0001 ) {
            continue;
        }
        const ter_str_id tid( it->first );
        const furn_str_id fid( it->first );

        if( tid.is_valid() ) {
            tf_id.ter = tid.id();
        } else if( fid.is_valid() ) {
            tf_id.furn = fid.id();
        } else {
            debugmsg("No clue what '%s' is! No such terrain or furniture", it->first.c_str() );
            continue;
        }
        btotal += (int)(it->second * 10000.0);
        boosted_weightlist[ btotal ] = tf_id;
    }

    if ( wtotal > 1000000 ) {
        debugmsg("plant coverage total exceeds 100%%");
    }
    if ( btotal > 1000000 ) {
        debugmsg("boosted plant coverage total exceeds 100%%");
    }

    tf_id.furn = f_null;
    tf_id.ter = default_ter;
    weightlist[ 1000000 ] = tf_id;
    boosted_weightlist[ 1000000 ] = tf_id;

    percent_str.clear();
    boosted_percent_str.clear();
}

ter_furn_id groundcover_extra::pick( bool boosted ) const
{
    if ( boosted ) {
        return boosted_weightlist.lower_bound( rng( 0, 1000000 ) )->second;
    }
    return weightlist.lower_bound( rng( 0, 1000000 ) )->second;
}

void regional_settings::setup()
{
    if ( default_groundcover_str ) {
        const ter_str_id primary( default_groundcover_str->primary_str );
        const ter_str_id secondary( default_groundcover_str->secondary_str );

        default_groundcover.primary = primary.id();
        default_groundcover.secondary = secondary.id();
        field_coverage.setup();
        default_groundcover_str.reset();
        get_options().add_value("DEFAULT_REGION", id );
    }
}

void overmap::add_mon_group(const mongroup &group)
{
    // Monster groups: the old system had large groups (radius > 1),
    // the new system transforms them into groups of radius 1, this also
    // makes the diffuse setting obsolete (as it only controls how the radius
    // is interpreted) - it's only used when adding monster groups with function.
    if( group.radius == 1 ) {
        zg.insert(std::pair<tripoint, mongroup>( group.pos, group ) );
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
                pop_here = ( 1.0 - static_cast<double>( dist ) / rad ) * pop / total_area;
            }
            if( pop_here > pop || pop_here < 0 ) {
                DebugLog( D_ERROR, D_GAME ) << group.type.str() << ": invalid population here: " << pop_here;
            }
            int p = std::max( 0, static_cast<int>(std::floor( pop_here )) );
            if( pop_here - p != 0 ) {
                // in case the population is something like 0.2, randomly add a
                // single population unit, this *should* on average give the correct
                // total population.
                const int mod = static_cast<int>(10000.0 * ( pop_here - p ));
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
            // To avoid this, the overmapbufer checks the monster groups when loading
            // an overmap and moves groups with out-of-bounds position to another overmap.
            add_mon_group( tmp );
            xpop += tmp.population;
        }
    }
    DebugLog( D_ERROR, D_GAME ) << group.type.str() << ": " << group.population << " => " << xpop;
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

npc* overmap::find_npc( const int id )
{
    for( auto &guy : npcs ) {
        if( guy->getID() == id ) {
            return guy;
        }
    }
    return nullptr;
}

const tripoint overmap::invalid_tripoint = tripoint(INT_MIN, INT_MIN, INT_MIN);
