#include "mapdata.h"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>

#include "assign.h"
#include "calendar.h"
#include "color.h"
#include "debug.h"
#include "enum_conversions.h"
#include "generic_factory.h"
#include "harvest.h"
#include "iexamine.h"
#include "iexamine_actors.h"
#include "item_group.h"
#include "json.h"
#include "output.h"
#include "rng.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"

static const item_group_id Item_spawn_data_EMPTY_GROUP( "EMPTY_GROUP" );

namespace
{

const units::volume DEFAULT_MAX_VOLUME_IN_SQUARE = units::from_liter( 1000 );

generic_factory<ter_t> terrain_data( "terrain" );
generic_factory<furn_t> furniture_data( "furniture" );

} // namespace

/** @relates int_id */
template<>
inline bool int_id<ter_t>::is_valid() const
{
    return terrain_data.is_valid( *this );
}

/** @relates int_id */
template<>
const ter_t &int_id<ter_t>::obj() const
{
    return terrain_data.obj( *this );
}

/** @relates int_id */
template<>
const string_id<ter_t> &int_id<ter_t>::id() const
{
    return terrain_data.convert( *this );
}

/** @relates int_id */
template<>
int_id<ter_t> string_id<ter_t>::id() const
{
    return terrain_data.convert( *this, t_null );
}

/** @relates int_id */
template<>
int_id<ter_t>::int_id( const string_id<ter_t> &id ) : _id( id.id() )
{
}

/** @relates string_id */
template<>
const ter_t &string_id<ter_t>::obj() const
{
    return terrain_data.obj( *this );
}

/** @relates string_id */
template<>
bool string_id<ter_t>::is_valid() const
{
    return terrain_data.is_valid( *this );
}

/** @relates int_id */
template<>
bool int_id<furn_t>::is_valid() const
{
    return furniture_data.is_valid( *this );
}

/** @relates int_id */
template<>
const furn_t &int_id<furn_t>::obj() const
{
    return furniture_data.obj( *this );
}

/** @relates int_id */
template<>
const string_id<furn_t> &int_id<furn_t>::id() const
{
    return furniture_data.convert( *this );
}

/** @relates string_id */
template<>
bool string_id<furn_t>::is_valid() const
{
    return furniture_data.is_valid( *this );
}

/** @relates string_id */
template<>
const furn_t &string_id<furn_t>::obj() const
{
    return furniture_data.obj( *this );
}

/** @relates string_id */
template<>
int_id<furn_t> string_id<furn_t>::id() const
{
    return furniture_data.convert( *this, f_null );
}

/** @relates int_id */
template<>
int_id<furn_t>::int_id( const string_id<furn_t> &id ) : _id( id.id() )
{
}

namespace io
{

template<>
std::string enum_to_string<ter_furn_flag>( ter_furn_flag data )
{
    // see mapdata.h for commentary
    switch( data ) {
        // *INDENT-OFF*
        case ter_furn_flag::TFLAG_TRANSPARENT: return "TRANSPARENT";
        case ter_furn_flag::TFLAG_FLAMMABLE: return "FLAMMABLE";
        case ter_furn_flag::TFLAG_REDUCE_SCENT: return "REDUCE_SCENT";
        case ter_furn_flag::TFLAG_SWIMMABLE: return "SWIMMABLE";
        case ter_furn_flag::TFLAG_SUPPORTS_ROOF: return "SUPPORTS_ROOF";
        case ter_furn_flag::TFLAG_MINEABLE: return "MINEABLE";
        case ter_furn_flag::TFLAG_NOITEM: return "NOITEM";
        case ter_furn_flag::TFLAG_NO_SIGHT: return "NO_SIGHT";
        case ter_furn_flag::TFLAG_NO_SCENT: return "NO_SCENT";
        case ter_furn_flag::TFLAG_SEALED: return "SEALED";
        case ter_furn_flag::TFLAG_ALLOW_FIELD_EFFECT: return "ALLOW_FIELD_EFFECT";
        case ter_furn_flag::TFLAG_LIQUID: return "LIQUID";
        case ter_furn_flag::TFLAG_COLLAPSES: return "COLLAPSES";
        case ter_furn_flag::TFLAG_FLAMMABLE_ASH: return "FLAMMABLE_ASH";
        case ter_furn_flag::TFLAG_DESTROY_ITEM: return "DESTROY_ITEM";
        case ter_furn_flag::TFLAG_INDOORS: return "INDOORS";
        case ter_furn_flag::TFLAG_LIQUIDCONT: return "LIQUIDCONT";
        case ter_furn_flag::TFLAG_FIRE_CONTAINER: return "FIRE_CONTAINER";
        case ter_furn_flag::TFLAG_FLAMMABLE_HARD: return "FLAMMABLE_HARD";
        case ter_furn_flag::TFLAG_SUPPRESS_SMOKE: return "SUPPRESS_SMOKE";
        case ter_furn_flag::TFLAG_SHARP: return "SHARP";
        case ter_furn_flag::TFLAG_DIGGABLE: return "DIGGABLE";
        case ter_furn_flag::TFLAG_ROUGH: return "ROUGH";
        case ter_furn_flag::TFLAG_UNSTABLE: return "UNSTABLE";
        case ter_furn_flag::TFLAG_WALL: return "WALL";
        case ter_furn_flag::TFLAG_DEEP_WATER: return "DEEP_WATER";
        case ter_furn_flag::TFLAG_SHALLOW_WATER: return "SHALLOW_WATER";
        case ter_furn_flag::TFLAG_WATER_CUBE: return "WATER_CUBE";
        case ter_furn_flag::TFLAG_CURRENT: return "CURRENT";
        case ter_furn_flag::TFLAG_HARVESTED: return "HARVESTED";
        case ter_furn_flag::TFLAG_PERMEABLE: return "PERMEABLE";
        case ter_furn_flag::TFLAG_AUTO_WALL_SYMBOL: return "AUTO_WALL_SYMBOL";
        case ter_furn_flag::TFLAG_CONNECT_WITH_WALL: return "CONNECT_WITH_WALL";
        case ter_furn_flag::TFLAG_CLIMBABLE: return "CLIMBABLE";
        case ter_furn_flag::TFLAG_GOES_DOWN: return "GOES_DOWN";
        case ter_furn_flag::TFLAG_GOES_UP: return "GOES_UP";
        case ter_furn_flag::TFLAG_NO_FLOOR: return "NO_FLOOR";
        case ter_furn_flag::TFLAG_ALLOW_ON_OPEN_AIR: return "ALLOW_ON_OPEN_AIR";
        case ter_furn_flag::TFLAG_SEEN_FROM_ABOVE: return "SEEN_FROM_ABOVE";
        case ter_furn_flag::TFLAG_RAMP_DOWN: return "RAMP_DOWN";
        case ter_furn_flag::TFLAG_RAMP_UP: return "RAMP_UP";
        case ter_furn_flag::TFLAG_RAMP: return "RAMP";
        case ter_furn_flag::TFLAG_HIDE_PLACE: return "HIDE_PLACE";
        case ter_furn_flag::TFLAG_BLOCK_WIND: return "BLOCK_WIND";
        case ter_furn_flag::TFLAG_FLAT: return "FLAT";
        case ter_furn_flag::TFLAG_RAIL: return "RAIL";
        case ter_furn_flag::TFLAG_THIN_OBSTACLE: return "THIN_OBSTACLE";
        case ter_furn_flag::TFLAG_SMALL_PASSAGE: return "SMALL_PASSAGE";
        case ter_furn_flag::TFLAG_Z_TRANSPARENT: return "Z_TRANSPARENT";
        case ter_furn_flag::TFLAG_SUN_ROOF_ABOVE: return "SUN_ROOF_ABOVE";
        case ter_furn_flag::TFLAG_FUNGUS: return "FUNGUS";
        case ter_furn_flag::TFLAG_LOCKED: return "LOCKED";
        case ter_furn_flag::TFLAG_PICKABLE: return "PICKABLE";
        case ter_furn_flag::TFLAG_WINDOW: return "WINDOW";
        case ter_furn_flag::TFLAG_DOOR: return "DOOR";
        case ter_furn_flag::TFLAG_SHRUB: return "SHRUB";
        case ter_furn_flag::TFLAG_YOUNG: return "YOUNG";
        case ter_furn_flag::TFLAG_PLANT: return "PLANT";
        case ter_furn_flag::TFLAG_FISHABLE: return "FISHABLE";
        case ter_furn_flag::TFLAG_TREE: return "TREE";
        case ter_furn_flag::TFLAG_PLOWABLE: return "PLOWABLE";
        case ter_furn_flag::TFLAG_ORGANIC: return "ORGANIC";
        case ter_furn_flag::TFLAG_CONSOLE: return "CONSOLE";
        case ter_furn_flag::TFLAG_PLANTABLE: return "PLANTABLE";
        case ter_furn_flag::TFLAG_GROWTH_HARVEST: return "GROWTH_HARVEST";
        case ter_furn_flag::TFLAG_MOUNTABLE: return "MOUNTABLE";
        case ter_furn_flag::TFLAG_RAMP_END: return "RAMP_END";
        case ter_furn_flag::TFLAG_FLOWER: return "FLOWER";
        case ter_furn_flag::TFLAG_CAN_SIT: return "CAN_SIT";
        case ter_furn_flag::TFLAG_FLAT_SURF: return "FLAT_SURF";
        case ter_furn_flag::TFLAG_BUTCHER_EQ: return "BUTCHER_EQ";
        case ter_furn_flag::TFLAG_GROWTH_SEEDLING: return "GROWTH_SEEDLING";
        case ter_furn_flag::TFLAG_GROWTH_MATURE: return "GROWTH_MATURE";
        case ter_furn_flag::TFLAG_WORKOUT_ARMS: return "WORKOUT_ARMS";
        case ter_furn_flag::TFLAG_WORKOUT_LEGS: return "WORKOUT_LEGS";
        case ter_furn_flag::TFLAG_TRANSLOCATOR: return "TRANSLOCATOR";
        case ter_furn_flag::TFLAG_AUTODOC: return "AUTODOC";
        case ter_furn_flag::TFLAG_AUTODOC_COUCH: return "AUTODOC_COUCH";
        case ter_furn_flag::TFLAG_OPENCLOSE_INSIDE: return "OPENCLOSE_INSIDE";
        case ter_furn_flag::TFLAG_SALT_WATER: return "SALT_WATER";
        case ter_furn_flag::TFLAG_PLACE_ITEM: return "PLACE_ITEM";
        case ter_furn_flag::TFLAG_BARRICADABLE_WINDOW_CURTAINS: return "BARRICADABLE_WINDOW_CURTAINS";
        case ter_furn_flag::TFLAG_CLIMB_SIMPLE: return "CLIMB_SIMPLE";
        case ter_furn_flag::TFLAG_NANOFAB_TABLE: return "NANOFAB_TABLE";
        case ter_furn_flag::TFLAG_ROAD: return "ROAD";
        case ter_furn_flag::TFLAG_TINY: return "TINY";
        case ter_furn_flag::TFLAG_SHORT: return "SHORT";
        case ter_furn_flag::TFLAG_NOCOLLIDE: return "NOCOLLIDE";
        case ter_furn_flag::TFLAG_BARRICADABLE_DOOR: return "BARRICADABLE_DOOR";
        case ter_furn_flag::TFLAG_BARRICADABLE_DOOR_DAMAGED: return "BARRICADABLE_DOOR_DAMAGED";
        case ter_furn_flag::TFLAG_BARRICADABLE_DOOR_REINFORCED: return "BARRICADABLE_DOOR_REINFORCED";
        case ter_furn_flag::TFLAG_USABLE_FIRE: return "USABLE_FIRE";
        case ter_furn_flag::TFLAG_CONTAINER: return "CONTAINER";
        case ter_furn_flag::TFLAG_NO_PICKUP_ON_EXAMINE: return "NO_PICKUP_ON_EXAMINE";
        case ter_furn_flag::TFLAG_RUBBLE: return "RUBBLE";
        case ter_furn_flag::TFLAG_DIGGABLE_CAN_DEEPEN: return "DIGGABLE_CAN_DEEPEN";
        case ter_furn_flag::TFLAG_PIT_FILLABLE: return "PIT_FILLABLE";
        case ter_furn_flag::TFLAG_DIFFICULT_Z: return "DIFFICULT_Z";
        case ter_furn_flag::TFLAG_ALIGN_WORKBENCH: return "ALIGN_WORKBENCH";
        case ter_furn_flag::TFLAG_NO_SPOIL: return "NO_SPOIL";
        case ter_furn_flag::TFLAG_EASY_DECONSTRUCT: return "EASY_DECONSTRUCT";
        case ter_furn_flag::TFLAG_LADDER: return "LADDER";
        case ter_furn_flag::TFLAG_ALARMED: return "ALARMED";
        case ter_furn_flag::TFLAG_CHOCOLATE: return "CHOCOLATE";
        case ter_furn_flag::TFLAG_SIGN: return "SIGN";
        case ter_furn_flag::TFLAG_SIGN_ALWAYS: return "SIGN_ALWAYS";
        case ter_furn_flag::TFLAG_DONT_REMOVE_ROTTEN: return "DONT_REMOVE_ROTTEN";
        case ter_furn_flag::TFLAG_BLOCKSDOOR: return "BLOCKSDOOR";
        case ter_furn_flag::TFLAG_SMALL_HIDE: return "SMALL_HIDE";
        case ter_furn_flag::TFLAG_NO_SELF_CONNECT: return "NO_SELF_CONNECT";
        case ter_furn_flag::TFLAG_BURROWABLE: return "BURROWABLE";
        case ter_furn_flag::TFLAG_MURKY: return "MURKY";
        case ter_furn_flag::TFLAG_AMMOTYPE_RELOAD: return "AMMOTYPE_RELOAD";
        case ter_furn_flag::TFLAG_TRANSPARENT_FLOOR: return "TRANSPARENT_FLOOR";
        case ter_furn_flag::TFLAG_TOILET_WATER: return "TOILET_WATER";
        case ter_furn_flag::TFLAG_ELEVATOR: return "ELEVATOR";
		case ter_furn_flag::TFLAG_ACTIVE_GENERATOR: return "ACTIVE_GENERATOR";
		case ter_furn_flag::TFLAG_NO_FLOOR_WATER: return "NO_FLOOR_WATER";

        // *INDENT-ON*
        case ter_furn_flag::NUM_TFLAG_FLAGS:
            break;
    }
    cata_fatal( "Invalid ter_furn_flag" );
}

} // namespace io

static std::unordered_map<std::string, connect_group> ter_connects_map;

connect_group get_connect_group( const std::string &name )
{
    return ter_connects_map[name];
}

void connect_group::load( const JsonObject &jo )
{

    connect_group result;

    result.id = connect_group_id( jo.get_string( "id" ) );
    result.index = ter_connects_map.find( result.id.str() ) == ter_connects_map.end() ?
                   ter_connects_map.size() : ter_connects_map[result.id.str()].index;
    // Check index overflow for bitsets
    if( result.index >= NUM_TERCONN ) {
        debugmsg( "Exceeded current maximum of %d connection groups.  Increase NUM_TERCONN to allow for more groups!",
                  NUM_TERCONN );
        return;
    }

    if( jo.has_string( "group_flags" ) || jo.has_array( "group_flags" ) ) {
        const std::vector<std::string> str_flags = jo.get_as_string_array( "group_flags" );
        for( const std::string &flag : str_flags ) {
            const ter_furn_flag f = io::string_to_enum<ter_furn_flag>( flag );
            result.group_flags.insert( f );
        }
    }

    if( jo.has_string( "connects_to_flags" ) || jo.has_array( "connects_to_flags" ) ) {
        const std::vector<std::string> str_flags = jo.get_as_string_array( "connects_to_flags" );
        for( const std::string &flag : str_flags ) {
            const ter_furn_flag f = io::string_to_enum<ter_furn_flag>( flag );
            result.connects_to_flags.insert( f );
        }
    }

    if( jo.has_string( "rotates_to_flags" ) || jo.has_array( "rotates_to_flags" ) ) {
        const std::vector<std::string> str_flags = jo.get_as_string_array( "rotates_to_flags" );
        for( const std::string &flag : str_flags ) {
            const ter_furn_flag f = io::string_to_enum<ter_furn_flag>( flag );
            result.rotates_to_flags.insert( f );
        }
    }

    ter_connects_map[ result.id.str() ] = result;
}

void connect_group::reset()
{
    ter_connects_map.clear();
}

static void load_map_bash_tent_centers( const JsonArray &ja, std::vector<furn_str_id> &centers )
{
    for( const std::string line : ja ) {
        centers.emplace_back( line );
    }
}

map_bash_info::map_bash_info() : str_min( -1 ), str_max( -1 ),
    str_min_blocked( -1 ), str_max_blocked( -1 ),
    str_min_supported( -1 ), str_max_supported( -1 ),
    explosive( 0 ), sound_vol( -1 ), sound_fail_vol( -1 ),
    collapse_radius( 1 ), destroy_only( false ), bash_below( false ),
    drop_group( Item_spawn_data_EMPTY_GROUP ),
    ter_set( ter_str_id::NULL_ID() ), furn_set( furn_str_id::NULL_ID() ) {}

bool map_bash_info::load( const JsonObject &jsobj, const std::string_view member,
                          map_object_type obj_type, const std::string &context )
{
    if( !jsobj.has_object( member ) ) {
        return false;
    }

    JsonObject j = jsobj.get_object( member );
    str_min = j.get_int( "str_min", 0 );
    str_max = j.get_int( "str_max", 0 );

    str_min_blocked = j.get_int( "str_min_blocked", -1 );
    str_max_blocked = j.get_int( "str_max_blocked", -1 );

    str_min_supported = j.get_int( "str_min_supported", -1 );
    str_max_supported = j.get_int( "str_max_supported", -1 );

    explosive = j.get_int( "explosive", -1 );

    sound_vol = j.get_int( "sound_vol", -1 );
    sound_fail_vol = j.get_int( "sound_fail_vol", -1 );

    collapse_radius = j.get_int( "collapse_radius", 1 );

    destroy_only = j.get_bool( "destroy_only", false );

    bash_below = j.get_bool( "bash_below", false );

    sound = to_translation( "smash!" );
    sound_fail = to_translation( "thump!" );
    j.read( "sound", sound );
    j.read( "sound_fail", sound_fail );

    switch( obj_type ) {
        case map_bash_info::furniture:
            furn_set = furn_str_id( j.get_string( "furn_set", "f_null" ) );
            break;
        case map_bash_info::terrain:
            ter_set = ter_str_id( j.get_string( "ter_set" ) );
            ter_set_bashed_from_above = ter_str_id( j.get_string( "ter_set_bashed_from_above",
                                                    ter_set.c_str() ) );
            break;
        case map_bash_info::field:
            fd_bash_move_cost = j.get_int( "move_cost", 100 );
            j.read( "msg_success", field_bash_msg_success );
            break;
    }

    if( j.has_member( "items" ) ) {
        drop_group = item_group::load_item_group( j.get_member( "items" ), "collection",
                     "map_bash_info for " + context );
    } else {
        drop_group = Item_spawn_data_EMPTY_GROUP;
    }

    if( j.has_array( "tent_centers" ) ) {
        load_map_bash_tent_centers( j.get_array( "tent_centers" ), tent_centers );
    }

    return true;
}

map_deconstruct_info::map_deconstruct_info() : can_do( false ), deconstruct_above( false ),
    ter_set( ter_str_id::NULL_ID() ), furn_set( furn_str_id::NULL_ID() ) {}

bool map_deconstruct_info::load( const JsonObject &jsobj, const std::string_view member,
                                 bool is_furniture, const std::string &context )
{
    if( !jsobj.has_object( member ) ) {
        return false;
    }
    JsonObject j = jsobj.get_object( member );
    furn_set = furn_str_id( j.get_string( "furn_set", "f_null" ) );

    if( !is_furniture ) {
        ter_set = ter_str_id( j.get_string( "ter_set" ) );
    }
    can_do = true;
    deconstruct_above = j.get_bool( "deconstruct_above", false );

    drop_group = item_group::load_item_group( j.get_member( "items" ), "collection",
                 "map_deconstruct_info for " + context );
    return true;
}

bool map_shoot_info::load( const JsonObject &jsobj, const std::string_view member, bool was_loaded )
{
    JsonObject j = jsobj.get_object( member );

    optional( j, was_loaded, "chance_to_hit", chance_to_hit, 100 );

    std::pair<int, int> reduce_damage;
    std::pair<int, int> reduce_damage_laser;
    std::pair<int, int> destroy_damage;

    mandatory( j, was_loaded, "reduce_damage", reduce_damage );
    mandatory( j, was_loaded, "reduce_damage_laser", reduce_damage_laser );
    mandatory( j, was_loaded, "destroy_damage", destroy_damage );

    reduce_dmg_min = reduce_damage.first;
    reduce_dmg_max = reduce_damage.second;
    reduce_dmg_min_laser = reduce_damage_laser.first;
    reduce_dmg_max_laser = reduce_damage_laser.second;
    destroy_dmg_min = destroy_damage.first;
    destroy_dmg_max = destroy_damage.second;

    optional( j, was_loaded, "no_laser_destroy", no_laser_destroy, false );

    return true;
}

furn_workbench_info::furn_workbench_info() : multiplier( 1.0f ), allowed_mass( units::mass_max ),
    allowed_volume( units::volume_max ) {}

bool furn_workbench_info::load( const JsonObject &jsobj, const std::string_view member )
{
    JsonObject j = jsobj.get_object( member );

    assign( j, "multiplier", multiplier );
    assign( j, "mass", allowed_mass );
    assign( j, "volume", allowed_volume );

    return true;
}

plant_data::plant_data() : transform( furn_str_id::NULL_ID() ), base( furn_str_id::NULL_ID() ),
    growth_multiplier( 1.0f ), harvest_multiplier( 1.0f ) {}

bool plant_data::load( const JsonObject &jsobj, const std::string_view member )
{
    JsonObject j = jsobj.get_object( member );

    assign( j, "transform", transform );
    assign( j, "base", base );
    assign( j, "growth_multiplier", growth_multiplier );
    assign( j, "harvest_multiplier", harvest_multiplier );

    return true;
}

furn_t null_furniture_t()
{
    furn_t new_furniture;
    new_furniture.id = furn_str_id::NULL_ID();
    new_furniture.name_ = to_translation( "nothing" );
    new_furniture.symbol_.fill( ' ' );
    new_furniture.color_.fill( c_white );
    new_furniture.light_emitted = 0;
    new_furniture.movecost = 0;
    new_furniture.move_str_req = -1;
    new_furniture.transparent = true;
    new_furniture.set_flag( ter_furn_flag::TFLAG_TRANSPARENT );
    new_furniture.examine_func = iexamine_functions_from_string( "none" );
    new_furniture.max_volume = DEFAULT_MAX_VOLUME_IN_SQUARE;
    return new_furniture;
}

ter_t::ter_t() : open( ter_str_id::NULL_ID() ), close( ter_str_id::NULL_ID() ),
    transforms_into( ter_str_id::NULL_ID() ),
    roof( ter_str_id::NULL_ID() ), trap( tr_null ) {}

ter_t null_terrain_t()
{
    ter_t new_terrain;

    new_terrain.id = ter_str_id::NULL_ID();
    new_terrain.name_ = to_translation( "nothing" );
    new_terrain.symbol_.fill( ' ' );
    new_terrain.color_.fill( c_white );
    new_terrain.light_emitted = 0;
    new_terrain.movecost = 0;
    new_terrain.transparent = true;
    new_terrain.set_flag( ter_furn_flag::TFLAG_TRANSPARENT );
    new_terrain.set_flag( ter_furn_flag::TFLAG_DIGGABLE );
    new_terrain.examine_func = iexamine_functions_from_string( "none" );
    new_terrain.max_volume = DEFAULT_MAX_VOLUME_IN_SQUARE;
    return new_terrain;
}

template<typename C, typename F>
void load_season_array( const JsonObject &jo, const std::string &key, const std::string &context,
                        C &container, F load_func )
{
    if( jo.has_string( key ) ) {
        container.fill( load_func( jo.get_string( key ) ) );

    } else if( jo.has_array( key ) ) {
        JsonArray arr = jo.get_array( key );
        if( arr.size() == 1 ) {
            container.fill( load_func( arr.get_string( 0 ) ) );

        } else if( arr.size() == container.size() ) {
            for( auto &e : container ) {
                e = load_func( arr.next_string() );
            }

        } else {
            jo.throw_error_at( key, "Incorrect number of entries" );
        }

    } else if( jo.has_member( key ) ) {
        jo.throw_error_at(
            key, string_format( "Expected '%s' member to be string or array", key ) );
    } else {
        jo.throw_error(
            string_format( "Expected '%s' member in %s but none was found", key, context ) );
    }
}

std::string map_data_common_t::name() const
{
    return name_.translated();
}

bool map_data_common_t::can_examine( const tripoint &examp ) const
{
    return examine_actor || examine_func.can_examine( examp );
}

bool map_data_common_t::has_examine( iexamine_examine_function func ) const
{
    return examine_func.examine == func;
}

bool map_data_common_t::has_examine( const std::string &action ) const
{
    return examine_actor && examine_actor->type == action;
}

void map_data_common_t::set_examine( iexamine_functions func )
{
    examine_func = func;
}

void map_data_common_t::examine( Character &you, const tripoint &examp ) const
{
    if( !examine_actor ) {
        examine_func.examine( you, examp );
        return;
    }
    examine_actor->call( you, examp );
}

void map_data_common_t::load_symbol( const JsonObject &jo, const std::string &context )
{
    if( jo.has_member( "copy-from" ) && looks_like.empty() ) {
        looks_like = jo.get_string( "copy-from" );
    }
    jo.read( "looks_like", looks_like );

    load_season_array( jo, "symbol", context, symbol_, [&jo]( const std::string & str ) {
        if( str == "LINE_XOXO" ) {
            return LINE_XOXO;
        } else if( str == "LINE_OXOX" ) {
            return LINE_OXOX;
        } else if( str.length() != 1 ) {
            jo.throw_error_at( "symbol", "Symbol string must be exactly 1 character long." );
        }
        return static_cast<int>( str[0] );
    } );

    const bool has_color = jo.has_member( "color" );
    const bool has_bgcolor = jo.has_member( "bgcolor" );
    if( has_color && has_bgcolor ) {
        jo.throw_error( "Found both color and bgcolor, only one of these is allowed." );
    } else if( has_color ) {
        load_season_array( jo, "color", context, color_, []( const std::string_view str ) {
            // has to use a lambda because of default params
            return color_from_string( str );
        } );
    } else if( has_bgcolor ) {
        load_season_array( jo, "bgcolor", context, color_, bgcolor_from_string );
    } else {
        jo.throw_error( R"(Missing member: one of: "color", "bgcolor" must exist.)" );
    }
}

int map_data_common_t::symbol() const
{
    return symbol_[season_of_year( calendar::turn )];
}

nc_color map_data_common_t::color() const
{
    return color_[season_of_year( calendar::turn )];
}

const harvest_id &map_data_common_t::get_harvest() const
{
    return harvest_by_season[season_of_year( calendar::turn )];
}

const std::set<std::string> &map_data_common_t::get_harvest_names() const
{
    static const std::set<std::string> null_names = {};
    const harvest_id &hid = get_harvest();
    return hid.is_null() ? null_names : hid->names();
}

void load_furniture( const JsonObject &jo, const std::string &src )
{
    if( furniture_data.empty() ) {
        furniture_data.insert( null_furniture_t() );
    }
    furniture_data.load( jo, src );
}

void load_terrain( const JsonObject &jo, const std::string &src )
{
    if( terrain_data.empty() ) { // TODO: This shouldn't live here
        terrain_data.insert( null_terrain_t() );
    }
    terrain_data.load( jo, src );
}

void map_data_common_t::extraprocess_flags( const ter_furn_flag flag )
{
    if( !transparent && flag == ter_furn_flag::TFLAG_TRANSPARENT ) {
        transparent = true;
    }

    for( std::pair<const std::string, connect_group> &item : ter_connects_map ) {
        if( item.second.group_flags.find( flag ) != item.second.group_flags.end() ) {
            set_connect_groups( { item.second.id.str() } );
        }
        if( item.second.connects_to_flags.find( flag ) != item.second.connects_to_flags.end() ) {
            set_connects_to( { item.second.id.str() } );
        }
        if( item.second.rotates_to_flags.find( flag ) != item.second.rotates_to_flags.end() ) {
            set_rotates_to( { item.second.id.str() } );
        }
    }
}

void map_data_common_t::set_flag( const std::string &flag )
{
    flags.insert( flag );
    std::optional<ter_furn_flag> f = io::string_to_enum_optional<ter_furn_flag>( flag );
    if( f.has_value() ) {
        bitflags.set( f.value() );
        extraprocess_flags( f.value() );
    }
}

void map_data_common_t::set_flag( const ter_furn_flag flag )
{
    flags.insert( io::enum_to_string<ter_furn_flag>( flag ) );
    bitflags.set( flag );
    extraprocess_flags( flag );
}

void map_data_common_t::set_connect_groups( const std::vector<std::string>
        &connect_groups_vec )
{
    set_groups( connect_groups, connect_groups_vec );
}

void map_data_common_t::set_connects_to( const std::vector<std::string> &connect_groups_vec )
{
    set_groups( connect_to_groups, connect_groups_vec );
}

void map_data_common_t::set_rotates_to( const std::vector<std::string> &connect_groups_vec )
{
    set_groups( rotate_to_groups, connect_groups_vec );
}

void map_data_common_t::set_groups( std::bitset<NUM_TERCONN> &bits,
                                    const std::vector<std::string> &connect_groups_vec )
{
    for( const std::string &group : connect_groups_vec ) {
        if( group.empty() ) {
            debugmsg( "Can't use empty string for terrain groups" );
            continue;
        }
        std::string grp = group;
        bool remove = false;
        if( grp.at( 0 ) == '~' ) {
            grp = grp.substr( 1 );
            remove = true;
        }
        const auto it = ter_connects_map.find( grp );
        if( it != ter_connects_map.end() ) {
            if( remove ) {
                bits.reset( it->second.index );
            } else {
                bits.set( it->second.index );
            }
        } else {
            debugmsg( "can't find terrain group %s", group.c_str() );
        }
    }
}

ter_id t_null,
       t_hole, // Real nothingness; makes you fall a z-level
       // Ground
       t_dirt, t_sand, t_clay, t_dirtmound, t_pit_shallow, t_pit, t_grave, t_grave_new,
       t_pit_corpsed, t_pit_covered, t_pit_spiked, t_pit_spiked_covered, t_pit_glass, t_pit_glass_covered,
       t_rock_floor,
       t_grass, t_grass_long, t_grass_tall, t_grass_golf, t_grass_dead, t_grass_white, t_moss,
       t_metal_floor,
       t_pavement, t_pavement_y, t_sidewalk, t_concrete, t_zebra,
       t_thconc_floor, t_thconc_floor_olight, t_strconc_floor,
       t_floor, t_floor_waxed,
       t_dirtfloor,//Dirt floor(Has roof)
       t_carpet_red, t_carpet_yellow, t_carpet_purple, t_carpet_green,
       t_linoleum_white, t_linoleum_gray,
       t_grate,
       t_slime,
       t_bridge,
       t_covered_well,
       // Lighting related
       t_utility_light,
       // Walls
       t_wall_log_half, t_wall_log, t_wall_log_chipped, t_wall_log_broken, t_palisade, t_palisade_gate,
       t_palisade_gate_o,
       t_wall_half, t_wall_wood, t_wall_wood_chipped, t_wall_wood_broken,
       t_wall, t_concrete_wall, t_brick_wall,
       t_wall_metal,
       t_scrap_wall,
       t_scrap_wall_halfway,
       t_wall_glass,
       t_wall_glass_alarm,
       t_reinforced_glass, t_reinforced_glass_shutter, t_reinforced_glass_shutter_open,
       t_laminated_glass, t_ballistic_glass,
       t_reinforced_door_glass_o, t_reinforced_door_glass_c,
       t_bars,
       t_reb_cage,
       t_door_c, t_door_c_peep, t_door_b, t_door_b_peep, t_door_o, t_door_o_peep, t_rdoor_c, t_rdoor_b,
       t_rdoor_o, t_door_locked_interior, t_door_locked, t_door_locked_peep, t_door_locked_alarm,
       t_door_frame,
       t_chaingate_l, t_fencegate_c, t_fencegate_o, t_chaingate_c, t_chaingate_o,
       t_retractable_gate_c, t_retractable_gate_l, t_retractable_gate_o,
       t_door_boarded, t_door_boarded_damaged, t_door_boarded_peep, t_rdoor_boarded,
       t_rdoor_boarded_damaged, t_door_boarded_damaged_peep,
       t_door_metal_c, t_door_metal_o, t_door_metal_locked, t_door_metal_pickable, t_mdoor_frame,
       t_door_bar_c, t_door_bar_o, t_door_bar_locked,
       t_door_glass_c, t_door_glass_o, t_door_glass_frosted_c, t_door_glass_frosted_o,
       t_portcullis,
       t_recycler, t_window, t_window_taped, t_window_domestic, t_window_domestic_taped, t_window_open,
       t_curtains, t_window_bars_curtains, t_window_bars_domestic,
       t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded,
       t_window_boarded_noglass, t_window_reinforced, t_window_reinforced_noglass, t_window_enhanced,
       t_window_enhanced_noglass, t_window_bars_alarm, t_window_bars,
       t_metal_grate_window, t_metal_grate_window_with_curtain, t_metal_grate_window_with_curtain_open,
       t_metal_grate_window_noglass, t_metal_grate_window_with_curtain_noglass,
       t_metal_grate_window_with_curtain_open_noglass,
       t_window_stained_green, t_window_stained_red, t_window_stained_blue,
       t_window_no_curtains, t_window_no_curtains_open, t_window_no_curtains_taped,
       t_rock, t_fault,
       t_paper,
       t_rock_wall, t_rock_wall_half,
       // Tree
       t_tree, t_tree_young, t_tree_apple, t_tree_apple_harvested, t_tree_coffee, t_tree_coffee_harvested,
       t_tree_pear, t_tree_pear_harvested, t_tree_cherry, t_tree_cherry_harvested,
       t_tree_peach, t_tree_peach_harvested, t_tree_apricot, t_tree_apricot_harvested, t_tree_plum,
       t_tree_plum_harvested,
       t_tree_pine, t_tree_blackjack, t_tree_birch, t_tree_willow, t_tree_maple, t_tree_maple_tapped,
       t_tree_hickory, t_tree_hickory_dead, t_tree_hickory_harvested, t_tree_deadpine, t_underbrush,
       t_shrub, t_shrub_blueberry, t_shrub_strawberry, t_trunk, t_stump,
       t_root_wall,
       t_wax, t_floor_wax,
       t_fence, t_chainfence, t_chainfence_posts,
       t_fence_post, t_fence_wire, t_fence_barbed, t_fence_rope,
       t_railing,
       // Nether
       t_marloss, t_fungus_floor_in, t_fungus_floor_sup, t_fungus_floor_out, t_fungus_wall,
       t_fungus_mound, t_fungus, t_shrub_fungal, t_tree_fungal, t_tree_fungal_young, t_marloss_tree,
       // Water, lava, etc.
       t_water_moving_dp, t_water_moving_sh, t_water_sh, t_water_dp, t_swater_sh, t_swater_dp,
       t_water_pool, t_sewage,
       t_lava,
       // More embellishments than you can shake a stick at.
       t_sandbox, t_slide, t_monkey_bars, t_backboard,
       t_gas_pump, t_gas_pump_smashed,
       t_diesel_pump, t_diesel_pump_smashed,
       t_atm,
       t_missile, t_missile_exploded,
       t_radio_tower, t_radio_controls,
       t_gates_mech_control, t_gates_control_concrete, t_gates_control_brick,
       t_barndoor, t_palisade_pulley,
       t_gates_control_metal,
       t_sewage_pipe, t_sewage_pump,
       t_column,
       t_vat,
       t_rootcellar,
       t_cvdbody, t_cvdmachine,
       t_water_pump,
       t_conveyor,
       t_improvised_shelter,
       // Staircases etc.
       t_stairs_down, t_stairs_up, t_manhole, t_ladder_up, t_ladder_down, t_slope_down,
       t_slope_up, t_rope_up,
       t_manhole_cover,
       // Special
       t_card_science, t_card_military, t_card_industrial, t_card_reader_broken, t_slot_machine,
       t_elevator_control, t_elevator_control_off, t_elevator, t_pedestal_wyrm,
       t_pedestal_temple,
       // Temple tiles
       t_rock_red, t_rock_green, t_rock_blue, t_floor_red, t_floor_green, t_floor_blue,
       t_switch_rg, t_switch_gb, t_switch_rb, t_switch_even, t_open_air,
       t_pavement_bg_dp, t_pavement_y_bg_dp, t_sidewalk_bg_dp, t_guardrail_bg_dp,
       t_rad_platform,
       // Railroad and subway
       t_railroad_rubble,
       t_buffer_stop, t_railroad_crossing_signal, t_crossbuck_wood, t_crossbuck_metal,
       t_railroad_tie, t_railroad_tie_h, t_railroad_tie_v, t_railroad_tie_d,
       t_railroad_track, t_railroad_track_h, t_railroad_track_v, t_railroad_track_d, t_railroad_track_d1,
       t_railroad_track_d2,
       t_railroad_track_on_tie, t_railroad_track_h_on_tie, t_railroad_track_v_on_tie,
       t_railroad_track_d_on_tie;

// TODO: Put this crap into an inclusion, which should be generated automatically using JSON data

void set_ter_ids()
{
    t_null = ter_id( "t_null" );
    t_hole = ter_id( "t_hole" );
    t_dirt = ter_id( "t_dirt" );
    t_sand = ter_id( "t_sand" );
    t_clay = ter_id( "t_clay" );
    t_dirtmound = ter_id( "t_dirtmound" );
    t_grave = ter_id( "t_grave" );
    t_grave_new = ter_id( "t_grave_new" );
    t_pit_shallow = ter_id( "t_pit_shallow" );
    t_pit = ter_id( "t_pit" );
    t_pit_corpsed = ter_id( "t_pit_corpsed" );
    t_pit_covered = ter_id( "t_pit_covered" );
    t_pit_spiked = ter_id( "t_pit_spiked" );
    t_pit_spiked_covered = ter_id( "t_pit_spiked_covered" );
    t_pit_glass = ter_id( "t_pit_glass" );
    t_pit_glass_covered = ter_id( "t_pit_glass_covered" );
    t_rock_floor = ter_id( "t_rock_floor" );
    t_grass = ter_id( "t_grass" );
    t_grass_dead = ter_id( "t_grass_dead" );
    t_grass_long = ter_id( "t_grass_long" );
    t_grass_tall = ter_id( "t_grass_tall" );
    t_moss = ter_id( "t_moss" );
    t_metal_floor = ter_id( "t_metal_floor" );
    t_pavement = ter_id( "t_pavement" );
    t_pavement_y = ter_id( "t_pavement_y" );
    t_zebra = ter_id( "t_zebra" );
    t_sidewalk = ter_id( "t_sidewalk" );
    t_concrete = ter_id( "t_concrete" );
    t_thconc_floor = ter_id( "t_thconc_floor" );
    t_thconc_floor_olight = ter_id( "t_thconc_floor_olight" );
    t_strconc_floor = ter_id( "t_strconc_floor" );
    t_floor = ter_id( "t_floor" );
    t_floor_waxed = ter_id( "t_floor_waxed" );
    t_dirtfloor = ter_id( "t_dirtfloor" );
    t_carpet_red = ter_id( "t_carpet_red" );
    t_carpet_yellow = ter_id( "t_carpet_yellow" );
    t_carpet_purple = ter_id( "t_carpet_purple" );
    t_carpet_green = ter_id( "t_carpet_green" );
    t_linoleum_white = ter_id( "t_linoleum_white" );
    t_linoleum_gray = ter_id( "t_linoleum_gray" );
    t_grate = ter_id( "t_grate" );
    t_slime = ter_id( "t_slime" );
    t_bridge = ter_id( "t_bridge" );
    t_utility_light = ter_id( "t_utility_light" );
    t_wall_log_half = ter_id( "t_wall_log_half" );
    t_wall_log = ter_id( "t_wall_log" );
    t_wall_log_chipped = ter_id( "t_wall_log_chipped" );
    t_wall_log_broken = ter_id( "t_wall_log_broken" );
    t_palisade = ter_id( "t_palisade" );
    t_palisade_gate = ter_id( "t_palisade_gate" );
    t_palisade_gate_o = ter_id( "t_palisade_gate_o" );
    t_wall_half = ter_id( "t_wall_half" );
    t_wall_wood = ter_id( "t_wall_wood" );
    t_wall_wood_chipped = ter_id( "t_wall_wood_chipped" );
    t_wall_wood_broken = ter_id( "t_wall_wood_broken" );
    t_wall = ter_id( "t_wall" );
    t_concrete_wall = ter_id( "t_concrete_wall" );
    t_brick_wall = ter_id( "t_brick_wall" );
    t_wall_metal = ter_id( "t_wall_metal" );
    t_scrap_wall = ter_id( "t_scrap_wall" );
    t_scrap_wall_halfway = ter_id( "t_scrap_wall_halfway" );
    t_wall_glass = ter_id( "t_wall_glass" );
    t_wall_glass_alarm = ter_id( "t_wall_glass_alarm" );
    t_reinforced_glass = ter_id( "t_reinforced_glass" );
    t_reinforced_glass_shutter = ter_id( "t_reinforced_glass_shutter" );
    t_reinforced_glass_shutter_open = ter_id( "t_reinforced_glass_shutter_open" );
    t_laminated_glass = ter_id( "t_laminated_glass" );
    t_ballistic_glass = ter_id( "t_ballistic_glass" );
    t_reinforced_door_glass_c = ter_id( "t_reinforced_door_glass_c" );
    t_reinforced_door_glass_o = ter_id( "t_reinforced_door_glass_o" );
    t_bars = ter_id( "t_bars" );
    t_reb_cage = ter_id( "t_reb_cage" );
    t_door_c = ter_id( "t_door_c" );
    t_door_c_peep = ter_id( "t_door_c_peep" );
    t_door_b = ter_id( "t_door_b" );
    t_door_b_peep = ter_id( "t_door_b_peep" );
    t_door_o = ter_id( "t_door_o" );
    t_door_o_peep = ter_id( "t_door_o_peep" );
    t_rdoor_c = ter_id( "t_rdoor_c" );
    t_rdoor_b = ter_id( "t_rdoor_b" );
    t_rdoor_o = ter_id( "t_rdoor_o" );
    t_door_locked_interior = ter_id( "t_door_locked_interior" );
    t_door_locked = ter_id( "t_door_locked" );
    t_door_locked_peep = ter_id( "t_door_locked_peep" );
    t_door_locked_alarm = ter_id( "t_door_locked_alarm" );
    t_door_frame = ter_id( "t_door_frame" );
    t_mdoor_frame = ter_id( "t_mdoor_frame" );
    t_chaingate_l = ter_id( "t_chaingate_l" );
    t_fencegate_c = ter_id( "t_fencegate_c" );
    t_fencegate_o = ter_id( "t_fencegate_o" );
    t_chaingate_c = ter_id( "t_chaingate_c" );
    t_chaingate_o = ter_id( "t_chaingate_o" );
    t_retractable_gate_l = ter_id( "t_retractable_gate_l" );
    t_retractable_gate_c = ter_id( "t_retractable_gate_c" );
    t_retractable_gate_o = ter_id( "t_retractable_gate_o" );
    t_door_boarded = ter_id( "t_door_boarded" );
    t_door_boarded_damaged = ter_id( "t_door_boarded_damaged" );
    t_door_boarded_peep = ter_id( "t_door_boarded_peep" );
    t_rdoor_boarded = ter_id( "t_rdoor_boarded" );
    t_rdoor_boarded_damaged = ter_id( "t_rdoor_boarded_damaged" );
    t_door_boarded_damaged_peep = ter_id( "t_door_boarded_damaged_peep" );
    t_door_metal_c = ter_id( "t_door_metal_c" );
    t_door_metal_o = ter_id( "t_door_metal_o" );
    t_door_metal_locked = ter_id( "t_door_metal_locked" );
    t_door_metal_pickable = ter_id( "t_door_metal_pickable" );
    t_door_bar_c = ter_id( "t_door_bar_c" );
    t_door_bar_o = ter_id( "t_door_bar_o" );
    t_door_bar_locked = ter_id( "t_door_bar_locked" );
    t_door_glass_c = ter_id( "t_door_glass_c" );
    t_door_glass_o = ter_id( "t_door_glass_o" );
    t_door_glass_frosted_c = ter_id( "t_door_glass_frosted_c" );
    t_door_glass_frosted_o = ter_id( "t_door_glass_frosted_o" );
    t_portcullis = ter_id( "t_portcullis" );
    t_recycler = ter_id( "t_recycler" );
    t_window = ter_id( "t_window" );
    t_window_taped = ter_id( "t_window_taped" );
    t_window_domestic = ter_id( "t_window_domestic" );
    t_window_domestic_taped = ter_id( "t_window_domestic_taped" );
    t_window_bars_domestic = ter_id( "t_window_bars_domestic" );
    t_window_open = ter_id( "t_window_open" );
    t_curtains = ter_id( "t_curtains" );
    t_window_bars_curtains = ter_id( "t_window_bars_curtains" );
    t_window_alarm = ter_id( "t_window_alarm" );
    t_window_alarm_taped = ter_id( "t_window_alarm_taped" );
    t_window_empty = ter_id( "t_window_empty" );
    t_window_frame = ter_id( "t_window_frame" );
    t_window_boarded = ter_id( "t_window_boarded" );
    t_window_boarded_noglass = ter_id( "t_window_boarded_noglass" );
    t_window_reinforced = ter_id( "t_window_reinforced" );
    t_window_reinforced_noglass = ter_id( "t_window_reinforced_noglass" );
    t_window_enhanced = ter_id( "t_window_enhanced" );
    t_window_enhanced_noglass = ter_id( "t_window_enhanced_noglass" );
    t_window_bars_alarm = ter_id( "t_window_bars_alarm" );
    t_window_bars = ter_id( "t_window_bars" );
    t_window_stained_green = ter_id( "t_window_stained_green" );
    t_window_stained_red = ter_id( "t_window_stained_red" );
    t_window_stained_blue = ter_id( "t_window_stained_blue" );
    t_window_no_curtains = ter_id( "t_window_no_curtains" );
    t_window_no_curtains_open = ter_id( "t_window_no_curtains_open" );
    t_window_no_curtains_taped = ter_id( "t_window_no_curtains_taped" );
    t_rock = ter_id( "t_rock" );
    t_fault = ter_id( "t_fault" );
    t_paper = ter_id( "t_paper" );
    t_rock_wall = ter_id( "t_rock_wall" );
    t_rock_wall_half = ter_id( "t_rock_wall_half" );
    t_tree = ter_id( "t_tree" );
    t_tree_young = ter_id( "t_tree_young" );
    t_tree_apple = ter_id( "t_tree_apple" );
    t_tree_apple_harvested = ter_id( "t_tree_apple_harvested" );
    t_tree_coffee = ter_id( "t_tree_coffee" );
    t_tree_coffee_harvested = ter_id( "t_tree_coffee_harvested" );
    t_tree_pear = ter_id( "t_tree_pear" );
    t_tree_pear_harvested = ter_id( "t_tree_pear_harvested" );
    t_tree_cherry = ter_id( "t_tree_cherry" );
    t_tree_cherry_harvested = ter_id( "t_tree_cherry_harvested" );
    t_tree_peach = ter_id( "t_tree_peach" );
    t_tree_peach_harvested = ter_id( "t_tree_peach_harvested" );
    t_tree_apricot = ter_id( "t_tree_apricot" );
    t_tree_apricot_harvested = ter_id( "t_tree_apricot_harvested" );
    t_tree_plum = ter_id( "t_tree_plum" );
    t_tree_plum_harvested = ter_id( "t_tree_plum_harvested" );
    t_tree_pine = ter_id( "t_tree_pine" );
    t_tree_blackjack = ter_id( "t_tree_blackjack" );
    t_tree_birch = ter_id( "t_tree_birch" );
    t_tree_willow = ter_id( "t_tree_willow" );
    t_tree_maple = ter_id( "t_tree_maple" );
    t_tree_maple_tapped = ter_id( "t_tree_maple_tapped" );
    t_tree_deadpine = ter_id( "t_tree_deadpine" );
    t_tree_hickory = ter_id( "t_tree_hickory" );
    t_tree_hickory_dead = ter_id( "t_tree_hickory_dead" );
    t_tree_hickory_harvested = ter_id( "t_tree_hickory_harvested" );
    t_underbrush = ter_id( "t_underbrush" );
    t_shrub = ter_id( "t_shrub" );
    t_shrub_blueberry = ter_id( "t_shrub_blueberry" );
    t_shrub_strawberry = ter_id( "t_shrub_strawberry" );
    t_trunk = ter_id( "t_trunk" );
    t_stump = ter_id( "t_stump" );
    t_root_wall = ter_id( "t_root_wall" );
    t_wax = ter_id( "t_wax" );
    t_floor_wax = ter_id( "t_floor_wax" );
    t_fence = ter_id( "t_fence" );
    t_chainfence = ter_id( "t_chainfence" );
    t_chainfence_posts = ter_id( "t_chainfence_posts" );
    t_fence_post = ter_id( "t_fence_post" );
    t_fence_wire = ter_id( "t_fence_wire" );
    t_fence_barbed = ter_id( "t_fence_barbed" );
    t_fence_rope = ter_id( "t_fence_rope" );
    t_railing = ter_id( "t_railing" );
    t_marloss = ter_id( "t_marloss" );
    t_fungus_floor_in = ter_id( "t_fungus_floor_in" );
    t_fungus_floor_sup = ter_id( "t_fungus_floor_sup" );
    t_fungus_floor_out = ter_id( "t_fungus_floor_out" );
    t_fungus_wall = ter_id( "t_fungus_wall" );
    t_fungus_mound = ter_id( "t_fungus_mound" );
    t_fungus = ter_id( "t_fungus" );
    t_shrub_fungal = ter_id( "t_shrub_fungal" );
    t_tree_fungal = ter_id( "t_tree_fungal" );
    t_tree_fungal_young = ter_id( "t_tree_fungal_young" );
    t_marloss_tree = ter_id( "t_marloss_tree" );
    t_water_moving_dp = ter_id( "t_water_moving_dp" );
    t_water_moving_sh = ter_id( "t_water_moving_sh" );
    t_water_sh = ter_id( "t_water_sh" );
    t_water_dp = ter_id( "t_water_dp" );
    t_swater_sh = ter_id( "t_swater_sh" );
    t_swater_dp = ter_id( "t_swater_dp" );
    t_water_pool = ter_id( "t_water_pool" );
    t_sewage = ter_id( "t_sewage" );
    t_lava = ter_id( "t_lava" );
    t_sandbox = ter_id( "t_sandbox" );
    t_slide = ter_id( "t_slide" );
    t_monkey_bars = ter_id( "t_monkey_bars" );
    t_backboard = ter_id( "t_backboard" );
    t_gas_pump = ter_id( "t_gas_pump" );
    t_gas_pump_smashed = ter_id( "t_gas_pump_smashed" );
    t_diesel_pump = ter_id( "t_diesel_pump" );
    t_diesel_pump_smashed = ter_id( "t_diesel_pump_smashed" );
    t_atm = ter_id( "t_atm" );
    t_missile = ter_id( "t_missile" );
    t_missile_exploded = ter_id( "t_missile_exploded" );
    t_radio_tower = ter_id( "t_radio_tower" );
    t_radio_controls = ter_id( "t_radio_controls" );
    t_gates_mech_control = ter_id( "t_gates_mech_control" );
    t_gates_control_brick = ter_id( "t_gates_control_brick" );
    t_gates_control_concrete = ter_id( "t_gates_control_concrete" );
    t_barndoor = ter_id( "t_barndoor" );
    t_palisade_pulley = ter_id( "t_palisade_pulley" );
    t_gates_control_metal = ter_id( "t_gates_control_metal" );
    t_sewage_pipe = ter_id( "t_sewage_pipe" );
    t_sewage_pump = ter_id( "t_sewage_pump" );
    t_column = ter_id( "t_column" );
    t_vat = ter_id( "t_vat" );
    t_rootcellar = ter_id( "t_rootcellar" );
    t_cvdbody = ter_id( "t_cvdbody" );
    t_cvdmachine = ter_id( "t_cvdmachine" );
    t_stairs_down = ter_id( "t_stairs_down" );
    t_stairs_up = ter_id( "t_stairs_up" );
    t_manhole = ter_id( "t_manhole" );
    t_ladder_up = ter_id( "t_ladder_up" );
    t_ladder_down = ter_id( "t_ladder_down" );
    t_slope_down = ter_id( "t_slope_down" );
    t_slope_up = ter_id( "t_slope_up" );
    t_rope_up = ter_id( "t_rope_up" );
    t_manhole_cover = ter_id( "t_manhole_cover" );
    t_card_science = ter_id( "t_card_science" );
    t_card_military = ter_id( "t_card_military" );
    t_card_industrial = ter_id( "t_card_industrial" );
    t_card_reader_broken = ter_id( "t_card_reader_broken" );
    t_slot_machine = ter_id( "t_slot_machine" );
    t_elevator_control = ter_id( "t_elevator_control" );
    t_elevator_control_off = ter_id( "t_elevator_control_off" );
    t_elevator = ter_id( "t_elevator" );
    t_pedestal_wyrm = ter_id( "t_pedestal_wyrm" );
    t_pedestal_temple = ter_id( "t_pedestal_temple" );
    t_rock_red = ter_id( "t_rock_red" );
    t_rock_green = ter_id( "t_rock_green" );
    t_rock_blue = ter_id( "t_rock_blue" );
    t_floor_red = ter_id( "t_floor_red" );
    t_floor_green = ter_id( "t_floor_green" );
    t_floor_blue = ter_id( "t_floor_blue" );
    t_switch_rg = ter_id( "t_switch_rg" );
    t_switch_gb = ter_id( "t_switch_gb" );
    t_switch_rb = ter_id( "t_switch_rb" );
    t_switch_even = ter_id( "t_switch_even" );
    t_covered_well = ter_id( "t_covered_well" );
    t_water_pump = ter_id( "t_water_pump" );
    t_conveyor = ter_id( "t_conveyor" );
    t_open_air = ter_id( "t_open_air" );
    t_pavement_bg_dp = ter_id( "t_pavement_bg_dp" );
    t_pavement_y_bg_dp = ter_id( "t_pavement_y_bg_dp" );
    t_sidewalk_bg_dp = ter_id( "t_sidewalk_bg_dp" );
    t_guardrail_bg_dp = ter_id( "t_guardrail_bg_dp" );
    t_rad_platform = ter_id( "t_rad_platform" );
    t_improvised_shelter = ter_id( "t_improvised_shelter" );
    t_railroad_rubble = ter_id( "t_railroad_rubble" );
    t_buffer_stop = ter_id( "t_buffer_stop" );
    t_railroad_crossing_signal = ter_id( "t_railroad_crossing_signal" );
    t_crossbuck_metal = ter_id( "t_crossbuck_metal" );
    t_crossbuck_wood = ter_id( "t_crossbuck_wood" );
    t_railroad_tie = ter_id( "t_railroad_tie" );
    t_railroad_tie_h = ter_id( "t_railroad_tie_h" );
    t_railroad_tie_v = ter_id( "t_railroad_tie_v" );
    t_railroad_tie_d = ter_id( "t_railroad_tie_d" );
    t_railroad_track = ter_id( "t_railroad_track" );
    t_railroad_track_h = ter_id( "t_railroad_track_h" );
    t_railroad_track_v = ter_id( "t_railroad_track_v" );
    t_railroad_track_d = ter_id( "t_railroad_track_d" );
    t_railroad_track_d1 = ter_id( "t_railroad_track_d1" );
    t_railroad_track_d2 = ter_id( "t_railroad_track_d2" );
    t_railroad_track_on_tie = ter_id( "t_railroad_track_on_tie" );
    t_railroad_track_h_on_tie = ter_id( "t_railroad_track_h_on_tie" );
    t_railroad_track_v_on_tie = ter_id( "t_railroad_track_v_on_tie" );
    t_railroad_track_d_on_tie = ter_id( "t_railroad_track_d_on_tie" );
    for( const ter_t &elem : terrain_data.get_all() ) {
        ter_t &ter = const_cast<ter_t &>( elem );
        if( ter.trap_id_str.empty() ) {
            ter.trap = tr_null;
        } else {
            ter.trap = trap_str_id( ter.trap_id_str );
        }
    }
}

void reset_furn_ter()
{
    terrain_data.reset();
    furniture_data.reset();
}

furn_id f_null, f_clear,
        f_hay,
        f_rubble, f_rubble_rock, f_wreckage, f_ash,
        f_barricade_road, f_sandbag_half, f_sandbag_wall,
        f_bulletin,
        f_indoor_plant,
        f_bed, f_toilet, f_makeshift_bed, f_straw_bed,
        f_sink, f_oven, f_woodstove, f_fireplace, f_bathtub,
        f_chair, f_armchair, f_sofa, f_cupboard, f_trashcan, f_desk, f_exercise,
        f_bench, f_table, f_pool_table,
        f_counter,
        f_fridge, f_glass_fridge, f_dresser, f_locker,
        f_rack, f_bookcase,
        f_washer, f_dryer,
        f_vending_c, f_vending_o, f_dumpster, f_dive_block,
        f_crate_c, f_crate_o, f_coffin_c, f_coffin_o,
        f_gunsafe_ml, f_gunsafe_mj, f_gun_safe_el,
        f_large_canvas_wall, f_canvas_wall, f_canvas_door, f_canvas_door_o, f_groundsheet,
        f_fema_groundsheet, f_large_groundsheet,
        f_large_canvas_door, f_large_canvas_door_o, f_center_groundsheet, f_skin_wall, f_skin_door,
        f_skin_door_o, f_skin_groundsheet,
        f_mutpoppy, f_flower_fungal, f_fungal_mass, f_fungal_clump,
        f_cattails, f_lotus, f_lilypad,
        f_safe_c, f_safe_l, f_safe_o,
        f_plant_seed, f_plant_seedling, f_plant_mature, f_plant_harvest,
        f_fvat_empty, f_fvat_full, f_fvat_wood_empty, f_fvat_wood_full,
        f_wood_keg,
        f_standing_tank,
        f_egg_sackbw, f_egg_sackcs, f_egg_sackws, f_egg_sacke,
        f_flower_marloss,
        f_tatami,
        f_kiln_empty, f_kiln_full, f_kiln_metal_empty, f_kiln_metal_full,
        f_arcfurnace_empty, f_arcfurnace_full,
        f_smoking_rack, f_smoking_rack_active, f_metal_smoking_rack, f_metal_smoking_rack_active,
        f_water_mill, f_water_mill_active,
        f_wind_mill, f_wind_mill_active,
        f_robotic_arm, f_vending_reinforced,
        f_brazier,
        f_firering,
        f_tourist_table,
        f_camp_chair,
        f_sign,
        f_stook_empty, f_stook_full,
        f_street_light, f_traffic_light, f_flagpole, f_wooden_flagpole,
        f_console, f_console_broken;

void set_furn_ids()
{
    f_null = furn_id( "f_null" );
    f_clear = furn_id( "f_clear" );
    f_hay = furn_id( "f_hay" );
    f_rubble = furn_id( "f_rubble" );
    f_rubble_rock = furn_id( "f_rubble_rock" );
    f_wreckage = furn_id( "f_wreckage" );
    f_ash = furn_id( "f_ash" );
    f_barricade_road = furn_id( "f_barricade_road" );
    f_sandbag_half = furn_id( "f_sandbag_half" );
    f_sandbag_wall = furn_id( "f_sandbag_wall" );
    f_stook_empty = furn_id( "f_stook_empty" );
    f_stook_full = furn_id( "f_stook_full" );
    f_bulletin = furn_id( "f_bulletin" );
    f_indoor_plant = furn_id( "f_indoor_plant" );
    f_bed = furn_id( "f_bed" );
    f_toilet = furn_id( "f_toilet" );
    f_makeshift_bed = furn_id( "f_makeshift_bed" );
    f_straw_bed = furn_id( "f_straw_bed" );
    f_sink = furn_id( "f_sink" );
    f_oven = furn_id( "f_oven" );
    f_woodstove = furn_id( "f_woodstove" );
    f_fireplace = furn_id( "f_fireplace" );
    f_bathtub = furn_id( "f_bathtub" );
    f_chair = furn_id( "f_chair" );
    f_armchair = furn_id( "f_armchair" );
    f_sofa = furn_id( "f_sofa" );
    f_cupboard = furn_id( "f_cupboard" );
    f_trashcan = furn_id( "f_trashcan" );
    f_desk = furn_id( "f_desk" );
    f_exercise = furn_id( "f_exercise" );
    f_bench = furn_id( "f_bench" );
    f_table = furn_id( "f_table" );
    f_pool_table = furn_id( "f_pool_table" );
    f_counter = furn_id( "f_counter" );
    f_fridge = furn_id( "f_fridge" );
    f_glass_fridge = furn_id( "f_glass_fridge" );
    f_dresser = furn_id( "f_dresser" );
    f_locker = furn_id( "f_locker" );
    f_rack = furn_id( "f_rack" );
    f_bookcase = furn_id( "f_bookcase" );
    f_washer = furn_id( "f_washer" );
    f_dryer = furn_id( "f_dryer" );
    f_vending_c = furn_id( "f_vending_c" );
    f_vending_o = furn_id( "f_vending_o" );
    f_vending_reinforced = furn_id( "f_vending_reinforced" );
    f_dumpster = furn_id( "f_dumpster" );
    f_dive_block = furn_id( "f_dive_block" );
    f_crate_c = furn_id( "f_crate_c" );
    f_crate_o = furn_id( "f_crate_o" );
    f_coffin_c = furn_id( "f_coffin_c" );
    f_coffin_o = furn_id( "f_coffin_o" );
    f_canvas_wall = furn_id( "f_canvas_wall" );
    f_large_canvas_wall = furn_id( "f_large_canvas_wall" );
    f_canvas_door = furn_id( "f_canvas_door" );
    f_large_canvas_door = furn_id( "f_large_canvas_door" );
    f_canvas_door_o = furn_id( "f_canvas_door_o" );
    f_large_canvas_door_o = furn_id( "f_large_canvas_door_o" );
    f_groundsheet = furn_id( "f_groundsheet" );
    f_large_groundsheet = furn_id( "f_large_groundsheet" );
    f_center_groundsheet = furn_id( "f_center_groundsheet" );
    f_fema_groundsheet = furn_id( "f_fema_groundsheet" );
    f_skin_wall = furn_id( "f_skin_wall" );
    f_skin_door = furn_id( "f_skin_door" );
    f_skin_door_o = furn_id( "f_skin_door_o" );
    f_skin_groundsheet = furn_id( "f_skin_groundsheet" );
    f_mutpoppy = furn_id( "f_mutpoppy" );
    f_fungal_mass = furn_id( "f_fungal_mass" );
    f_fungal_clump = furn_id( "f_fungal_clump" );
    f_flower_fungal = furn_id( "f_flower_fungal" );
    f_cattails = furn_id( "f_cattails" );
    f_lilypad = furn_id( "f_lilypad" );
    f_lotus = furn_id( "f_lotus" );
    f_safe_c = furn_id( "f_safe_c" );
    f_safe_l = furn_id( "f_safe_l" );
    f_safe_o = furn_id( "f_safe_o" );
    f_plant_seed = furn_id( "f_plant_seed" );
    f_plant_seedling = furn_id( "f_plant_seedling" );
    f_plant_mature = furn_id( "f_plant_mature" );
    f_plant_harvest = furn_id( "f_plant_harvest" );
    f_fvat_empty = furn_id( "f_fvat_empty" );
    f_fvat_full = furn_id( "f_fvat_full" );
    f_fvat_wood_empty = furn_id( "f_fvat_wood_empty" );
    f_fvat_wood_full = furn_id( "f_fvat_wood_full" );
    f_wood_keg = furn_id( "f_wood_keg" );
    f_standing_tank = furn_id( "f_standing_tank" );
    f_egg_sackbw = furn_id( "f_egg_sackbw" );
    f_egg_sackcs = furn_id( "f_egg_sackcs" );
    f_egg_sackws = furn_id( "f_egg_sackws" );
    f_egg_sacke = furn_id( "f_egg_sacke" );
    f_flower_marloss = furn_id( "f_flower_marloss" );
    f_kiln_empty = furn_id( "f_kiln_empty" );
    f_kiln_full = furn_id( "f_kiln_full" );
    f_kiln_metal_empty = furn_id( "f_kiln_metal_empty" );
    f_kiln_metal_full = furn_id( "f_kiln_metal_full" );
    f_arcfurnace_empty = furn_id( "f_arcfurnace_empty" );
    f_arcfurnace_full = furn_id( "f_arcfurnace_full" );
    f_smoking_rack = furn_id( "f_smoking_rack" );
    f_smoking_rack_active = furn_id( "f_smoking_rack_active" );
    f_metal_smoking_rack = furn_id( "f_metal_smoking_rack" );
    f_metal_smoking_rack_active = furn_id( "f_metal_smoking_rack_active" );
    f_water_mill = furn_id( "f_water_mill" );
    f_water_mill_active = furn_id( "f_water_mill_active" );
    f_wind_mill = furn_id( "f_wind_mill" );
    f_wind_mill_active = furn_id( "f_wind_mill_active" );
    f_robotic_arm = furn_id( "f_robotic_arm" );
    f_brazier = furn_id( "f_brazier" );
    f_firering = furn_id( "f_firering" );
    f_tourist_table = furn_id( "f_tourist_table" );
    f_camp_chair = furn_id( "f_camp_chair" );
    f_sign = furn_id( "f_sign" );
    f_gunsafe_ml = furn_id( "f_gunsafe_ml" );
    f_gunsafe_mj = furn_id( "f_gunsafe_mj" );
    f_gun_safe_el = furn_id( "f_gun_safe_el" );
    f_street_light = furn_id( "f_street_light" );
    f_traffic_light = furn_id( "f_traffic_light" );
    f_flagpole = furn_id( "f_flagpole" );
    f_wooden_flagpole = furn_id( "f_wooden_flagpole" );
    f_console_broken = furn_id( "f_console_broken" );
    f_console = furn_id( "f_console" );
}

size_t ter_t::count()
{
    return terrain_data.size();
}

namespace io
{
template<>
std::string enum_to_string<season_type>( season_type data )
{
    switch( data ) {
        // *INDENT-OFF*
        case season_type::SPRING: return "spring";
        case season_type::SUMMER: return "summer";
        case season_type::AUTUMN: return "autumn";
        case season_type::WINTER: return "winter";
        // *INDENT-ON*
        case season_type::NUM_SEASONS:
            break;
    }
    cata_fatal( "Invalid season_type" );
}
} // namespace io

static std::map<std::string, cata::clone_ptr<iexamine_actor>> examine_actors;

static void add_actor( std::unique_ptr<iexamine_actor> ptr )
{
    std::string type = ptr->type;
    examine_actors[type] = cata::clone_ptr<iexamine_actor>( std::move( ptr ) );
}

static cata::clone_ptr<iexamine_actor> iexamine_actor_from_jsobj( const JsonObject &jo )
{
    std::string type = jo.get_string( "type" );
    try {
        return examine_actors.at( type );
    } catch( const std::exception & ) {
        jo.throw_error( string_format( "Invalid iexamine actor %s", type ) );
    }
}

void init_mapdata()
{
    add_actor( std::make_unique<appliance_convert_examine_actor>() );
    add_actor( std::make_unique<cardreader_examine_actor>() );
    add_actor( std::make_unique<eoc_examine_actor>() );
}

void map_data_common_t::load( const JsonObject &jo, const std::string & )
{
    if( jo.has_string( "examine_action" ) ) {
        examine_actor = nullptr;
        examine_func = iexamine_functions_from_string( jo.get_string( "examine_action" ) );
    } else if( jo.has_object( "examine_action" ) ) {
        JsonObject data = jo.get_object( "examine_action" );
        examine_actor = iexamine_actor_from_jsobj( data );
        examine_actor->load( data );
        examine_func = iexamine_functions_from_string( "invalid" );
    } else if( !was_loaded ) {
        examine_actor = nullptr;
        examine_func = iexamine_functions_from_string( "none" );
    }

    if( jo.has_array( "harvest_by_season" ) ) {
        for( JsonObject harvest_jo : jo.get_array( "harvest_by_season" ) ) {
            auto season_strings = harvest_jo.get_tags( "seasons" );
            std::set<season_type> seasons;
            std::transform( season_strings.begin(), season_strings.end(), std::inserter( seasons,
                            seasons.begin() ), io::string_to_enum<season_type> );

            harvest_id hl;
            harvest_jo.read( "id", hl );

            for( season_type s : seasons ) {
                harvest_by_season[ s ] = hl;
            }
        }
    }

    mandatory( jo, was_loaded, "description", description );
    optional( jo, was_loaded, "curtain_transform", curtain_transform );

    if( jo.has_object( "shoot" ) ) {
        shoot = cata::make_value<map_shoot_info>();
        shoot->load( jo, "shoot", was_loaded );
    }
}

bool ter_t::is_null() const
{
    return id == ter_str_id::NULL_ID();
}

void ter_t::load( const JsonObject &jo, const std::string &src )
{
    map_data_common_t::load( jo, src );
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "move_cost", movecost );
    optional( jo, was_loaded, "coverage", coverage );
    assign( jo, "max_volume", max_volume, src == "dda" );
    optional( jo, was_loaded, "trap", trap_id_str );
    optional( jo, was_loaded, "heat_radiation", heat_radiation );
    optional( jo, was_loaded, "light_emitted", light_emitted );
    int legacy_floor_bedding_warmth = units::to_legacy_bodypart_temp_delta( floor_bedding_warmth );
    optional( jo, was_loaded, "floor_bedding_warmth", legacy_floor_bedding_warmth, 0 );
    floor_bedding_warmth = units::from_legacy_bodypart_temp_delta( legacy_floor_bedding_warmth );
    optional( jo, was_loaded, "comfort", comfort, 0 );

    load_symbol( jo, "terrain " + id.str() );

    trap = tr_null;
    transparent = false;
    connect_groups.reset();
    connect_to_groups.reset();
    rotate_to_groups.reset();

    for( auto &flag : jo.get_string_array( "flags" ) ) {
        set_flag( flag );
    }
    // connect_to_groups is initialized to none, then terrain flags are set, then finally
    // connections from JSON are set. This is so that wall flags can set wall connections
    // but can be overridden by explicit connections in JSON.
    if( jo.has_member( "connect_groups" ) ) {
        set_connect_groups( jo.get_as_string_array( "connect_groups" ) );
    }
    if( jo.has_member( "connects_to" ) ) {
        set_connects_to( jo.get_as_string_array( "connects_to" ) );
    }
    if( jo.has_member( "rotates_to" ) ) {
        set_rotates_to( jo.get_as_string_array( "rotates_to" ) );
    }

    optional( jo, was_loaded, "allowed_template_ids", allowed_template_id );

    optional( jo, was_loaded, "open", open, ter_str_id::NULL_ID() );
    optional( jo, was_loaded, "close", close, ter_str_id::NULL_ID() );
    optional( jo, was_loaded, "transforms_into", transforms_into, ter_str_id::NULL_ID() );
    optional( jo, was_loaded, "roof", roof, ter_str_id::NULL_ID() );

    optional( jo, was_loaded, "lockpick_result", lockpick_result, ter_str_id::NULL_ID() );
    optional( jo, was_loaded, "lockpick_message", lockpick_message, translation() );

    oxytorch = cata::make_value<activity_data_ter>();
    if( jo.has_object( "oxytorch" ) ) {
        oxytorch->load( jo.get_object( "oxytorch" ) );
    }

    boltcut = cata::make_value<activity_data_ter>();
    if( jo.has_object( "boltcut" ) ) {
        boltcut->load( jo.get_object( "boltcut" ) );
    }

    hacksaw = cata::make_value<activity_data_ter>();
    if( jo.has_object( "hacksaw" ) ) {
        hacksaw->load( jo.get_object( "hacksaw" ) );
    }

    prying = cata::make_value<activity_data_ter>();
    if( jo.has_object( "prying" ) ) {
        prying->load( jo.get_object( "prying" ) );
    }

    optional( jo, was_loaded, "emissions", emissions );

    bash.load( jo, "bash", map_bash_info::terrain, "terrain " + id.str() );
    deconstruct.load( jo, "deconstruct", false, "terrain " + id.str() );
}

static void check_bash_items( const map_bash_info &mbi, const std::string &id, bool is_terrain )
{
    if( !item_group::group_is_defined( mbi.drop_group ) ) {
        debugmsg( "%s: bash result item group %s does not exist", id.c_str(), mbi.drop_group.c_str() );
    }
    if( mbi.str_max != -1 ) {
        if( is_terrain && mbi.ter_set.is_empty() ) { // Some tiles specify t_null explicitly
            debugmsg( "bash result terrain of %s is undefined/empty", id.c_str() );
        }
        if( !mbi.ter_set.is_valid() ) {
            debugmsg( "bash result terrain %s of %s does not exist", mbi.ter_set.c_str(), id.c_str() );
        }
        if( !mbi.furn_set.is_valid() ) {
            debugmsg( "bash result furniture %s of %s does not exist", mbi.furn_set.c_str(), id.c_str() );
        }
    }
}

static void check_decon_items( const map_deconstruct_info &mbi, const std::string &id,
                               bool is_terrain )
{
    if( !mbi.can_do ) {
        return;
    }
    if( !item_group::group_is_defined( mbi.drop_group ) ) {
        debugmsg( "%s: deconstruct result item group %s does not exist", id.c_str(),
                  mbi.drop_group.c_str() );
    }
    if( is_terrain && mbi.ter_set.is_empty() ) { // Some tiles specify t_null explicitly
        debugmsg( "deconstruct result terrain of %s is undefined/empty", id.c_str() );
    }
    if( !mbi.ter_set.is_valid() ) {
        debugmsg( "deconstruct result terrain %s of %s does not exist", mbi.ter_set.c_str(), id.c_str() );
    }
    if( !mbi.furn_set.is_valid() ) {
        debugmsg( "deconstruct result furniture %s of %s does not exist", mbi.furn_set.c_str(),
                  id.c_str() );
    }
}

void ter_t::check() const
{
    map_data_common_t::check();
    check_bash_items( bash, id.str(), true );
    check_decon_items( deconstruct, id.str(), true );

    if( !transforms_into.is_valid() ) {
        debugmsg( "invalid transforms_into %s for %s", transforms_into.c_str(), id.c_str() );
    }

    // Validate open/close transforms
    if( !open.is_valid() ) {
        debugmsg( "invalid terrain %s for opening %s", open.c_str(), id.c_str() );
    }
    if( !close.is_valid() ) {
        debugmsg( "invalid terrain %s for closing %s", close.c_str(), id.c_str() );
    }
    // Check transition consistency for opening/closing terrain. Has an obvious
    // exception for locked terrains - those aren't expected to be locked again
    if( open && open->close && open->close != id && !has_flag( ter_furn_flag::TFLAG_LOCKED ) ) {
        debugmsg( "opening terrain %s for %s doesn't reciprocate", open.c_str(), id.c_str() );
    }
    if( close && close->open && close->open != id && !has_flag( ter_furn_flag::TFLAG_LOCKED ) ) {
        debugmsg( "closing terrain %s for %s doesn't reciprocate", close.c_str(), id.c_str() );
    }

    // Validate curtain transforms
    if( has_examine( iexamine::curtains ) && !has_curtains() ) {
        debugmsg( "%s is a curtain, but has no curtain_transform", id.c_str() );
    }
    if( !has_examine( iexamine::curtains ) && has_curtains() ) {
        debugmsg( "%s is not a curtain, but has curtain_transform", id.c_str() );
    }
    if( !curtain_transform.is_empty() && !curtain_transform.is_valid() ) {
        debugmsg( "%s has invalid curtain transform target %s", id.c_str(), curtain_transform.c_str() );
    }

    // Validate generic transforms
    if( transforms_into && transforms_into == id ) {
        debugmsg( "%s transforms_into itself", id.c_str() );
    }

    for( const emit_id &e : emissions ) {
        if( !e.is_valid() ) {
            debugmsg( "ter %s has invalid emission %s set", id.c_str(), e.str().c_str() );
        }
    }
}

furn_t::furn_t() : open( furn_str_id::NULL_ID() ), close( furn_str_id::NULL_ID() ) {}

size_t furn_t::count()
{
    return furniture_data.size();
}

bool furn_t::is_movable() const
{
    return move_str_req >= 0;
}

void furn_t::load( const JsonObject &jo, const std::string &src )
{
    map_data_common_t::load( jo, src );
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "move_cost_mod", movecost );
    optional( jo, was_loaded, "coverage", coverage );
    optional( jo, was_loaded, "comfort", comfort, 0 );
    int legacy_floor_bedding_warmth = units::to_legacy_bodypart_temp_delta( floor_bedding_warmth );
    optional( jo, was_loaded, "floor_bedding_warmth", legacy_floor_bedding_warmth, 0 );
    floor_bedding_warmth = units::from_legacy_bodypart_temp_delta( legacy_floor_bedding_warmth );
    optional( jo, was_loaded, "emissions", emissions );
    int legacy_bonus_fire_warmth_feet = units::to_legacy_bodypart_temp_delta( bonus_fire_warmth_feet );
    optional( jo, was_loaded, "bonus_fire_warmth_feet", legacy_bonus_fire_warmth_feet, 300 );
    bonus_fire_warmth_feet = units::from_legacy_bodypart_temp_delta( legacy_bonus_fire_warmth_feet );
    optional( jo, was_loaded, "keg_capacity", keg_capacity, legacy_volume_reader, 0_ml );
    mandatory( jo, was_loaded, "required_str", move_str_req );
    optional( jo, was_loaded, "max_volume", max_volume, volume_reader(), DEFAULT_MAX_VOLUME_IN_SQUARE );
    optional( jo, was_loaded, "crafting_pseudo_item", crafting_pseudo_item, itype_id() );
    optional( jo, was_loaded, "deployed_item", deployed_item );
    load_symbol( jo, "furniture " + id.str() );
    transparent = false;

    optional( jo, was_loaded, "light_emitted", light_emitted );

    // see the comment in ter_id::load for connect_group handling
    connect_groups.reset();
    connect_to_groups.reset();
    rotate_to_groups.reset();

    for( auto &flag : jo.get_string_array( "flags" ) ) {
        set_flag( flag );
    }

    if( jo.has_member( "connect_groups" ) ) {
        set_connect_groups( jo.get_as_string_array( "connect_groups" ) );
    }
    if( jo.has_member( "connects_to" ) ) {
        set_connects_to( jo.get_as_string_array( "connects_to" ) );
    }
    if( jo.has_member( "rotates_to" ) ) {
        set_rotates_to( jo.get_as_string_array( "rotates_to" ) );
    }

    optional( jo, was_loaded, "open", open, string_id_reader<furn_t> {}, furn_str_id::NULL_ID() );
    optional( jo, was_loaded, "close", close, string_id_reader<furn_t> {}, furn_str_id::NULL_ID() );

    optional( jo, was_loaded, "lockpick_result", lockpick_result, string_id_reader<furn_t> {},
              furn_str_id::NULL_ID() );
    optional( jo, was_loaded, "lockpick_message", lockpick_message, translation() );

    oxytorch = cata::make_value<activity_data_furn>();
    if( jo.has_object( "oxytorch" ) ) {
        oxytorch->load( jo.get_object( "oxytorch" ) );
    }

    boltcut = cata::make_value<activity_data_furn>();
    if( jo.has_object( "boltcut" ) ) {
        boltcut->load( jo.get_object( "boltcut" ) );
    }

    hacksaw = cata::make_value<activity_data_furn>();
    if( jo.has_object( "hacksaw" ) ) {
        hacksaw->load( jo.get_object( "hacksaw" ) );
    }

    prying = cata::make_value<activity_data_furn>();
    if( jo.has_object( "prying" ) ) {
        prying->load( jo.get_object( "prying" ) );
    }

    bash.load( jo, "bash", map_bash_info::furniture, "furniture " + id.str() );
    deconstruct.load( jo, "deconstruct", true, "furniture " + id.str() );

    if( jo.has_object( "workbench" ) ) {
        workbench = cata::make_value<furn_workbench_info>();
        workbench->load( jo, "workbench" );
    }
    if( jo.has_object( "plant_data" ) ) {
        plant = cata::make_value<plant_data>();
        plant->load( jo, "plant_data" );
    }
    if( jo.has_float( "surgery_skill_multiplier" ) ) {
        surgery_skill_multiplier = cata::make_value<float>( jo.get_float( "surgery_skill_multiplier" ) );
    }
}

void furn_t::check() const
{
    map_data_common_t::check();
    check_bash_items( bash, id.str(), false );
    check_decon_items( deconstruct, id.str(), false );

    if( !open.is_valid() ) {
        debugmsg( "invalid furniture %s for opening %s", open.c_str(), id.c_str() );
    }
    if( !close.is_valid() ) {
        debugmsg( "invalid furniture %s for closing %s", close.c_str(), id.c_str() );
    }
    for( const emit_id &e : emissions ) {
        if( !e.is_valid() ) {
            debugmsg( "furn %s has invalid emission %s set", id.c_str(),
                      e.str().c_str() );
        }
    }
}

int activity_byproduct::roll() const
{
    return count + rng( random_min, random_max );
}

void activity_byproduct::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "item", item );

    if( jo.has_int( "count" ) ) {
        mandatory( jo, was_loaded, "count", count );
        count = std::max( 0, count );
    } else if( jo.has_array( "count" ) ) {
        std::vector<int> count_random = jo.get_int_array( "count" );
        random_min = std::max( 0, count_random[0] );
        random_max = std::max( 0, count_random[1] );
        if( random_min > random_max ) {
            std::swap( random_min, random_max );
        }
    }
}

void activity_byproduct::deserialize( const JsonObject &jo )
{
    load( jo );
}

void pry_data::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "prying_nails", prying_nails );

    optional( jo, was_loaded, "difficulty", difficulty );
    optional( jo, was_loaded, "prying_level", prying_level );

    optional( jo, was_loaded, "noisy", noisy );
    optional( jo, was_loaded, "alarm", alarm );
    optional( jo, was_loaded, "breakable", breakable );

    optional( jo, was_loaded, "failure", failure );
}

void pry_data::deserialize( const JsonObject &jo )
{
    load( jo );
}

void activity_data_common::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "duration", duration_, 1_seconds );
    optional( jo, was_loaded, "message", message_ );
    optional( jo, was_loaded, "sound", sound_ );

    optional( jo, was_loaded, "byproducts", byproducts_ );

    optional( jo, was_loaded, "prying_data", prying_data_ );
}

void activity_data_ter::load( const JsonObject &jo )
{
    mandatory( jo, was_loaded, "result", result_ );
    activity_data_common::load( jo );
    valid_ = true;
}

void activity_data_furn::load( const JsonObject &jo )
{
    optional( jo, was_loaded, "result", result_, f_null.id() );
    activity_data_common::load( jo );
    valid_ = true;
}

void check_furniture_and_terrain()
{
    terrain_data.check();
    furniture_data.check();
}
