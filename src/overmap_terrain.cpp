#include "omdata.h" // IWYU pragma: associated
#include "overmap.h" // IWYU pragma: associated

#include <cstring>
#include <optional>
#include <ostream>
#include <set>
#include <vector>

#include "cached_options.h"
#include "cata_assert.h"
#include "cata_utility.h"
#include "catacharset.h"
#include "coordinates.h"
#include "debug.h"
#include "generic_factory.h"
#include "mapgen.h"
#include "mapgen_post_process_generators.h"
#include "output.h"
#include "string_formatter.h"

static const oter_str_id oter_road_nesw_manhole( "road_nesw_manhole" );

static const oter_vision_id oter_vision_default( "default" );

static const string_id<map_data_summary> map_data_summary_empty_omt( "empty_omt" );
static const string_id<map_data_summary> map_data_summary_full_omt( "full_omt" );
static const string_id<map_data_summary>
map_data_summary_scattered_obstacles_omt( "scattered_obstacles_omt" );

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

void oter_vision::finalize_all()
{
    oter_vision_factory.finalize();
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

namespace
{

generic_factory<overmap_land_use_code> land_use_codes( "overmap land use codes" );
generic_factory<oter_type_t> terrain_types( "overmap terrain type" );
generic_factory<oter_t> terrains( "overmap terrain" );

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

void overmap_land_use_code::load( const JsonObject &jo, const std::string_view )
{
    optional( jo, was_loaded, "land_use_code", land_use_code );
    optional( jo, was_loaded, "name", name );
    optional( jo, was_loaded, "detailed_definition", detailed_definition );

    optional( jo, was_loaded, "sym", symbol, unicode_codepoint_from_symbol_reader, NULL_UNICODE );
    if( symbol == NULL_UNICODE ) {
        DebugLog( D_ERROR, D_GAME ) << "`sym` node is not defined properly for `land_use_code`: "
                                    << id.c_str() << " (" << name << ")";
    }

    optional( jo, was_loaded, "color", color, nc_color_reader{}, c_black );
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
    land_use_codes.finalize();
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
        case oter_flags::pp_generate_ruined: return "PP_GENERATE_RUINED";
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
        case oter_travel_cost_type::structure: return "structure";
        case oter_travel_cost_type::roof: return "roof";
        case oter_travel_cost_type::basement: return "basement";
        case oter_travel_cost_type::tunnel: return "tunnel";
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
    mandatory( jo, false, "color", color, nc_color_reader{} );
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

static string_id<map_data_summary> map_data_for_travel_cost( oter_travel_cost_type type,
        const JsonObject &/*jo*/ )
{
    // With no specific override, set based on travel_cost_type if present.
    switch( type ) {
        case oter_travel_cost_type::impassable:
        // Revisit water and air?
        case oter_travel_cost_type::water:
        case oter_travel_cost_type::air:
            return map_data_summary_full_omt;
        case oter_travel_cost_type::road:
        case oter_travel_cost_type::field:
        case oter_travel_cost_type::dirt_road:
            return map_data_summary_empty_omt;
        case oter_travel_cost_type::trail:
        case oter_travel_cost_type::forest:
        case oter_travel_cost_type::shore:
        case oter_travel_cost_type::swamp:
            return map_data_summary_scattered_obstacles_omt;
        case oter_travel_cost_type::other:
        default:
            // Should not reach, throw an error.
            //jo.throw_error( string_format( "No inferred or explicit default_map_data for %s", id.str() ) );
            break;
    }
    // Terrible hack, just mark it impssable?
    // There seem to be something like 2,500 entries that need to be annotted for this to work.
    return map_data_summary_full_omt;
}

void oter_type_t::load( const JsonObject &jo, const std::string_view )
{
    optional( jo, was_loaded, "sym", symbol, unicode_codepoint_from_symbol_reader, NULL_UNICODE );

    optional( jo, was_loaded, "name", name );
    // For some reason an enum can be read as a number??
    if( jo.has_number( "see_cost" ) ) {
        jo.throw_error( string_format( "In %s: See cost uses invalid number format", id.str() ) );
    }
    mandatory( jo, was_loaded, "see_cost", see_cost );
    optional( jo, was_loaded, "extras", extras, "none" );
    optional( jo, was_loaded, "mondensity", mondensity, 0 );
    optional( jo, was_loaded, "entry_eoc", entry_EOC );
    optional( jo, was_loaded, "exit_eoc", exit_EOC );
    optional( jo, was_loaded, "spawns", static_spawns );
    optional( jo, was_loaded, "color", color, nc_color_reader{} );
    optional( jo, was_loaded, "land_use_code", land_use_code, overmap_land_use_code_id::NULL_ID() );

    if( jo.has_member( "looks_like" ) ) {
        if( jo.has_array( "looks_like" ) ) {
            mandatory( jo, was_loaded, "looks_like", looks_like );
        } else {
            std::string one_look;
            mandatory( jo, was_loaded, "looks_like", one_look );
            looks_like = { one_look };
        }
    } else if( jo.has_member( "copy-from" ) ) {
        looks_like.insert( looks_like.begin(), jo.get_string( "copy-from" ) );
    }

    const auto flag_reader = typed_flag_reader<oter_flags>( "overmap terrain flag" );
    optional( jo, was_loaded, "flags", flags, flag_reader );

    optional( jo, was_loaded, "connect_group", connect_group, string_reader{} );
    optional( jo, was_loaded, "travel_cost_type", travel_cost_type, oter_travel_cost_type::other );
    optional( jo, was_loaded, "post_process_generators", post_processors );

    optional( jo, was_loaded, "default_map_data", default_map_data,
              map_data_for_travel_cost( travel_cost_type, jo ) );


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
