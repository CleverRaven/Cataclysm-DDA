#include "mapdata.h"

#include <algorithm>
#include <exception>
#include <iterator>
#include <map>
#include <memory>
#include <unordered_map>
#include <utility>

#include "assign.h"
#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "debug.h"
#include "enum_conversions.h"
#include "flexbuffer_json.h"
#include "generic_factory.h"
#include "harvest.h"
#include "iexamine.h"
#include "iexamine_actors.h"
#include "item.h"
#include "item_group.h"
#include "iteminfo_query.h"
#include "mod_manager.h"
#include "output.h"
#include "rng.h"
#include "skill.h"
#include "string_formatter.h"
#include "translations.h"
#include "trap.h"
#include "type_id.h"

static furn_id f_null;
static const furn_str_id furn_f_null( "f_null" );

static const item_group_id Item_spawn_data_EMPTY_GROUP( "EMPTY_GROUP" );

static const skill_id skill_survival( "survival" );

namespace
{

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
        case ter_furn_flag::TFLAG_GROWTH_OVERGROWN: return "GROWTH_OVERGROWN";
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
        case ter_furn_flag::TFLAG_TRANSLOCATOR_GREATER: return "TRANSLOCATOR_GREATER";
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
        case ter_furn_flag::TFLAG_ELEVATOR: return "ELEVATOR";
        case ter_furn_flag::TFLAG_ACTIVE_GENERATOR: return "ACTIVE_GENERATOR";
        case ter_furn_flag::TFLAG_TRANSLUCENT: return "TRANSLUCENT";
        case ter_furn_flag::TFLAG_NO_FLOOR_WATER: return "NO_FLOOR_WATER";
        case ter_furn_flag::TFLAG_GRAZABLE: return "GRAZABLE";
        case ter_furn_flag::TFLAG_GRAZER_INEDIBLE: return "GRAZER_INEDIBLE";
        case ter_furn_flag::TFLAG_BROWSABLE: return "BROWSABLE";
        case ter_furn_flag::TFLAG_SINGLE_SUPPORT: return "SINGLE_SUPPORT";
        case ter_furn_flag::TFLAG_CLIMB_ADJACENT: return "CLIMB_ADJACENT";
        case ter_furn_flag::TFLAG_FLOATS_IN_AIR: return "FLOATS_IN_AIR";
        case ter_furn_flag::TFLAG_HARVEST_REQ_CUT1: return "HARVEST_REQ_CUT1";
        case ter_furn_flag::TFLAG_NATURAL_UNDERGROUND: return "NATURAL_UNDERGROUND";

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

    ter_connects_map[ result.id.str() ] = result;
}

void connect_group::reset()
{
    ter_connects_map.clear();
}

static void load_map_bash_tent_centers( const JsonArray &ja, std::vector<furn_str_id> &centers )
{
    centers.clear();
    for( const std::string line : ja ) {
        centers.emplace_back( line );
    }
}

void map_common_bash_info::load( const JsonObject &jo, const bool was_loaded,
                                 const std::string &context )
{
    optional( jo, was_loaded, "str_min", str_min, -1 );
    optional( jo, was_loaded, "str_max", str_max, -1 );

    optional( jo, was_loaded, "str_min_blocked", str_min_blocked, -1 );
    optional( jo, was_loaded, "str_max_blocked", str_max_blocked, -1 );

    optional( jo, was_loaded, "str_min_supported", str_min_supported, -1 );
    optional( jo, was_loaded, "str_max_supported", str_max_supported, -1 );

    optional( jo, was_loaded, "explosive", explosive, -1 );

    optional( jo, was_loaded, "sound_vol", sound_vol, -1 );
    optional( jo, was_loaded, "sound_fail_vol", sound_fail_vol, -1 );

    optional( jo, was_loaded, "collapse_radius", collapse_radius, 1 );

    optional( jo, was_loaded, "destroy_only", destroy_only, false );

    optional( jo, was_loaded, "bash_below", bash_below, false );

    optional( jo, was_loaded, "sound", sound, to_translation( "smash!" ) );
    optional( jo, was_loaded, "sound_fail", sound_fail, to_translation( "thump!" ) );

    if( jo.has_member( "items" ) ) {
        drop_group = item_group::load_item_group( jo.get_member( "items" ), "collection",
                     "map_bash_info for " + context );
    } else if( !was_loaded ) {
        drop_group = Item_spawn_data_EMPTY_GROUP;
    }

    if( jo.has_array( "tent_centers" ) ) {
        load_map_bash_tent_centers( jo.get_array( "tent_centers" ), tent_centers );
    }
}
void map_ter_bash_info::load( const JsonObject &jo, const bool was_loaded,
                              const std::string &context )
{
    map_common_bash_info::load( jo, was_loaded, context );
    mandatory( jo, was_loaded, "ter_set", ter_set );
    optional( jo, was_loaded, "ter_set_bashed_from_above", ter_set_bashed_from_above, ter_set );
}
void map_furn_bash_info::load( const JsonObject &jo, const bool was_loaded,
                               const std::string &context )
{
    map_common_bash_info::load( jo, was_loaded, context );
    optional( jo, was_loaded, "furn_set", furn_set, furn_f_null );
}
void map_fd_bash_info::load( const JsonObject &jo, const bool was_loaded,
                             const std::string &context )
{
    map_common_bash_info::load( jo, was_loaded, context );
    optional( jo, was_loaded, "move_cost", fd_bash_move_cost, 100 );
    optional( jo, was_loaded, "msg_success", field_bash_msg_success );
}

std::string map_common_bash_info::potential_bash_items( const std::string
        &ter_furn_name ) const
{
    //TODO: Add a descriptive indicator of vaguely how hard it is to bash?
    return string_format( _( "Bashing the %s would yield:\n%s" ),
                          ter_furn_name, item_group::potential_items( drop_group ) );
}

void map_common_deconstruct_info::load( const JsonObject &jo, const bool was_loaded,
                                        const std::string &context )
{
    if( jo.has_object( "skill" ) ) {
        JsonObject jos = jo.get_object( "skill" );
        skill = { skill_id( jos.get_string( "skill" ) ), jos.get_int( "min", 0 ), jos.get_int( "max", 10 ), jos.get_float( "multiplier", 1.0 ) };
    }
    optional( jo, was_loaded, "deconstruct_above", deconstruct_above, false );
    if( jo.has_member( "items" ) ) {
        drop_group = item_group::load_item_group( jo.get_member( "items" ), "collection",
                     "map_deconstruct_info for " + context );
    } else if( !was_loaded ) {
        drop_group = Item_spawn_data_EMPTY_GROUP;
    }
}

void map_ter_deconstruct_info::load( const JsonObject &jo, const bool was_loaded,
                                     const std::string &context )
{
    mandatory( jo, was_loaded, "ter_set", ter_set );
    map_common_deconstruct_info::load( jo, was_loaded, context );
}
void map_furn_deconstruct_info::load( const JsonObject &jo, const bool was_loaded,
                                      const std::string &context )
{
    optional( jo, was_loaded, "furn_set", furn_set, furn_f_null );
    map_common_deconstruct_info::load( jo, was_loaded, context );
}

std::string map_common_deconstruct_info::potential_deconstruct_items( const std::string
        &ter_furn_name ) const
{
    Character &who = get_avatar();
    bool will_practice_skill = !!skill && who.get_skill_level( skill->id ) >= skill->min &&
                               who.get_skill_level( skill->id ) < skill->max;
    if( will_practice_skill ) {
        return string_format(
                   _( "Deconstructing the %s would yield:\n%s\nYou feel you might also learn something about <color_cyan>%s</color>." ),
                   ter_furn_name, item_group::potential_items( drop_group ), skill->id.obj().name() );
    } else {
        return string_format( _( "Deconstructing the %s would yield:\n%s" ),
                              ter_furn_name, item_group::potential_items( drop_group ) );
    }
}

bool map_shoot_info::load( const JsonObject &jsobj, std::string_view member, bool was_loaded )
{
    JsonObject j = jsobj.get_object( member );

    optional( j, false, "chance_to_hit", chance_to_hit, 100 );

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

    optional( j, false, "no_laser_destroy", no_laser_destroy, false );

    return true;
}

furn_workbench_info::furn_workbench_info() : multiplier( 1.0f ), allowed_mass( units::mass::max() ),
    allowed_volume( units::volume::max() ) {}

bool furn_workbench_info::load( const JsonObject &jsobj, std::string_view member )
{
    JsonObject j = jsobj.get_object( member );

    assign( j, "multiplier", multiplier );
    assign( j, "mass", allowed_mass );
    assign( j, "volume", allowed_volume );

    return true;
}

plant_data::plant_data() : transform( furn_str_id::NULL_ID() ), base( furn_str_id::NULL_ID() ),
    growth_multiplier( 1.0f ), harvest_multiplier( 1.0f ) {}

bool plant_data::load( const JsonObject &jsobj, std::string_view member )
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
    new_furniture.max_volume = DEFAULT_TILE_VOLUME;
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
    new_terrain.max_volume = DEFAULT_TILE_VOLUME;
    return new_terrain;
}

template<typename C, typename F>
void load_season_array( const JsonObject &jo, const std::string &key, const std::string &context,
                        const bool ignore_absent_key, C &container, F load_func )
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
    } else if( !ignore_absent_key ) {
        jo.throw_error(
            string_format( "Expected '%s' member in %s but none was found", key, context ) );
    }
}

std::string map_data_common_t::name() const
{
    return name_.translated();
}

bool map_data_common_t::can_examine( const tripoint_bub_ms &examp ) const
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

void map_data_common_t::examine( Character &you, const tripoint_bub_ms &examp ) const
{
    if( !examine_actor ) {
        examine_func.examine( you, examp );
        return;
    }
    examine_actor->call( you, examp );
}

void map_data_common_t::load_symbol_color( const JsonObject &jo, const std::string &context )
{
    const bool no_copy_symbol_color = jo.has_member( "copy-from" );

    load_season_array( jo, "symbol", context, no_copy_symbol_color,
    symbol_, [&jo]( std::string_view str ) {
        if( str.length() != 1 ) {
            jo.throw_error_at( "symbol", "Symbol string must be exactly 1 character long." );
        }
        return static_cast<int>( str[0] );
    } );

    const bool has_color = jo.has_member( "color" );
    const bool has_bgcolor = jo.has_member( "bgcolor" );
    if( has_color && has_bgcolor ) {
        jo.throw_error( "Found both color and bgcolor, only one of these is allowed." );
    } else if( has_color ) {
        load_season_array( jo, "color", context, no_copy_symbol_color,
        color_, []( std::string_view str ) {
            // has to use a lambda because of default params
            return color_from_string( str );
        } );
    } else if( has_bgcolor ) {
        load_season_array( jo, "bgcolor", context, no_copy_symbol_color, color_, bgcolor_from_string );
    } else if( !no_copy_symbol_color ) {
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

std::vector<std::string> ter_t::extended_description() const
{
    std::vector<std::string> ret;
    ret.emplace_back( get_origin( src ) );
    ret.emplace_back( "--" );

    std::vector<std::string> tmp = map_data_common_t::extended_description();
    ret.insert( ret.end(), tmp.begin(), tmp.end() );

    if( deconstruct ) {
        ret.emplace_back( "--" );
        ret.emplace_back( deconstruct->potential_deconstruct_items( name() ) );
    }

    if( is_smashable() ) {
        ret.emplace_back( "--" );
        ret.emplace_back( bash->potential_bash_items( name() ) );
    }

    return ret;
}

std::vector<std::string> furn_t::extended_description() const
{
    std::vector<std::string> ret;
    ret.emplace_back( get_origin( src ) );
    ret.emplace_back( "--" );

    std::vector<std::string> tmp = map_data_common_t::extended_description();
    ret.insert( ret.end(), tmp.begin(), tmp.end() );

    // If this furniture has a crafting pseudo item, check for tool qualities and print them
    if( !crafting_pseudo_item.is_empty() ) {
        const item pseudo( crafting_pseudo_item );
        std::vector<iteminfo_parts> quality_part = { iteminfo_parts::QUALITIES };
        const iteminfo_query quality_query( quality_part );
        std::vector<iteminfo> info_vec;
        pseudo.qualities_info( info_vec, &quality_query, 1, false );
        // A bit of cargo-culting here, pre-imgui printing code was adapted
        // to split string with line breaks into single-line strings
        std::string quality_string = format_item_info( info_vec, {} );
        size_t strpos = 0;
        while( ( strpos = quality_string.find( '\n' ) ) != std::string::npos ) {
            // \n character is skipped
            ret.emplace_back( quality_string.substr( 0, strpos ) );
            quality_string.erase( 0, strpos + 1 );
        }
    }

    if( deconstruct ) {
        ret.emplace_back( "--" );
        ret.emplace_back( deconstruct->potential_deconstruct_items( name() ) );
    }

    if( is_smashable() ) {
        ret.emplace_back( "--" );
        ret.emplace_back( bash->potential_bash_items( name() ) );
    }

    return ret;
}

std::vector<std::string> map_data_common_t::extended_description() const
{
    std::vector<std::string> tmp;

    tmp.emplace_back( string_format( _( "<header>That is a %s.</header>" ), name() ) );
    tmp.emplace_back( description.translated() );
    bool has_any_harvest = std::any_of( harvest_by_season.begin(), harvest_by_season.end(),
    []( const harvest_id & hv ) {
        return !hv.obj().empty();
    } );

    if( has_any_harvest ) {
        tmp.emplace_back( "--" );
        int player_skill = get_player_character().get_greater_skill_or_knowledge_level( skill_survival );
        tmp.emplace_back( _( "You could harvest the following things from it:" ) );
        // Group them by identical ids to avoid repeating same blocks of data
        // First, invert the mapping: season->id to id->seasons
        std::multimap<harvest_id, season_type> identical_harvest;
        for( size_t season = SPRING; season <= WINTER; season++ ) {
            const auto &hv = harvest_by_season[season];
            if( hv.obj().empty() ) {
                continue;
            }

            identical_harvest.insert( std::make_pair( hv, static_cast<season_type>( season ) ) );
        }
        // Now print them in order of seasons
        // TODO: Highlight current season
        for( size_t season = SPRING; season <= WINTER; season++ ) {
            const auto range = identical_harvest.equal_range( harvest_by_season[season] );
            if( range.first == range.second ) {
                continue;
            }

            // List the seasons first
            std::string seasons = enumerate_as_string( range.first, range.second,
            []( const std::pair<harvest_id, season_type> &pr ) {
                if( pr.second == season_of_year( calendar::turn ) ) {
                    return "<good>" + calendar::name_season( pr.second ) + "</good>";
                }

                return "<dark>" + calendar::name_season( pr.second ) + "</dark>";
            } );
            seasons += ":";
            tmp.emplace_back( seasons );
            // List the drops
            // They actually describe what player can get from it now, so it isn't spoily
            // TODO: Allow spoily listing of everything
            tmp.emplace_back( range.first->first.obj().describe( player_skill ) );
            // Remove the range from the multimap so that it isn't listed twice
            identical_harvest.erase( range.first, range.second );
        }
    }

    tmp.emplace_back( "--" );
    tmp.emplace_back( string_format( _( "Concealment: %d%%" ), coverage ) );
    if( has_flag( ter_furn_flag::TFLAG_TREE ) ) {
        tmp.emplace_back( _( "Can be <color_green>cut down</color> with the right tools." ) );
    }

    // todo: generalize, copied from map::features which combines terrain and furniture info
    std::string result;
    const auto add = [&]( const std::string & text ) {
        if( !result.empty() ) {
            result += " ";
        }
        result += text;
    };
    const auto add_if = [&]( const bool cond, const std::string & text ) {
        if( cond ) {
            add( text );
        }
    };
    add_if( has_flag( ter_furn_flag::TFLAG_DIGGABLE ), _( "Diggable." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_PLOWABLE ), _( "Plowable." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_ROUGH ), _( "Rough." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_UNSTABLE ), _( "Unstable." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_SHARP ), _( "Sharp." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_FLAT ), _( "Flat." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_EASY_DECONSTRUCT ), _( "Simple." ) );
    add_if( has_flag( ter_furn_flag::TFLAG_MOUNTABLE ), _( "Mountable." ) );
    add_if( is_flammable(), _( "Flammable." ) );
    if( !result.empty() ) {
        tmp.emplace_back( result );
    }

    std::vector<std::string> ret;
    ret.reserve( tmp.size() );
    for( const std::string &s : tmp ) {
        ret.emplace_back( replace_colors( s ) );
    }

    return ret;
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

void map_data_common_t::set_flag( const std::string &flag )
{
    flags.insert( flag );
    std::optional<ter_furn_flag> f = io::string_to_enum_optional<ter_furn_flag>( flag );
    if( f.has_value() ) {
        bitflags.set( f.value() );
        transparent |= f.value() == ter_furn_flag::TFLAG_TRANSPARENT ||
                       f.value() == ter_furn_flag::TFLAG_TRANSLUCENT;
    }
}

void map_data_common_t::set_flag( const ter_furn_flag flag )
{
    flags.insert( io::enum_to_string<ter_furn_flag>( flag ) );
    bitflags.set( flag );
    transparent |= flag == ter_furn_flag::TFLAG_TRANSPARENT || flag == ter_furn_flag::TFLAG_TRANSLUCENT;
}

void map_data_common_t::unset_flag( const std::string &flag )
{
    if( auto it_flag = flags.find( flag ); it_flag != flags.end() ) {
        flags.erase( it_flag );
    } //else return false?
    std::optional<ter_furn_flag> f = io::string_to_enum_optional<ter_furn_flag>( flag );
    if( f.has_value() ) {
        bitflags.reset( f.value() );
        if( transparent ) {
            transparent = f.value() != ter_furn_flag::TFLAG_TRANSPARENT;
        }
    }
}

void map_data_common_t::unset_flags()
{
    flags.clear();
    bitflags.reset();
    transparent = false; //?
}

void map_data_common_t::set_connect_groups( const std::vector<std::string> &connect_groups_vec )
{
    set_groups( connect_groups, connect_groups_vec );
}

void map_data_common_t::unset_connect_groups( const std::vector<std::string> &connect_groups_vec )
{
    set_groups( connect_groups, connect_groups_vec, true );
}

void map_data_common_t::set_connects_to( const std::vector<std::string> &connect_groups_vec )
{
    set_groups( connect_to_groups, connect_groups_vec );
}

void map_data_common_t::unset_connects_to( const std::vector<std::string> &connect_groups_vec )
{
    set_groups( connect_to_groups, connect_groups_vec, true );
}

void map_data_common_t::set_rotates_to( const std::vector<std::string> &connect_groups_vec )
{
    set_groups( rotate_to_groups, connect_groups_vec );
}

void map_data_common_t::unset_rotates_to( const std::vector<std::string> &connect_groups_vec )
{
    set_groups( rotate_to_groups, connect_groups_vec, true );
}

void map_data_common_t::set_groups( std::bitset<NUM_TERCONN> &bits,
                                    const std::vector<std::string> &connect_groups_vec, bool unset )
{
    for( const std::string &group : connect_groups_vec ) {
        if( group.empty() ) {
            debugmsg( "Can't use empty string for terrain groups" );
            continue;
        }
        std::string grp = group;
        bool remove = unset;
        if( grp.at( 0 ) == '~' ) {
            grp = grp.substr( 1 );
            remove = !remove;
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

ter_id t_null;

void set_ter_ids()
{
    t_null = ter_id( "t_null" );

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

void set_furn_ids()
{
    f_null = furn_id( "f_null" );
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
    add_actor( std::make_unique<mortar_examine_actor>() );
}

void map_data_common_t::load( const JsonObject &jo, const std::string &src )
{
    mandatory( jo, was_loaded, "name", name_ );
    mandatory( jo, was_loaded, "description", description );

    if( jo.has_member( "copy-from" ) ) {
        looks_like = jo.get_string( "copy-from" );
    }
    optional( jo, was_loaded, "looks_like", looks_like );
    optional( jo, was_loaded, "comfort", comfort, 0 );

    if( jo.has_member( "connect_groups" ) ) {
        connect_groups.reset();
        set_connect_groups( jo.get_as_string_array( "connect_groups" ) );
    }
    if( jo.has_member( "connects_to" ) ) {
        connect_to_groups.reset();
        set_connects_to( jo.get_as_string_array( "connects_to" ) );
    }
    if( jo.has_member( "rotates_to" ) ) {
        rotate_to_groups.reset();
        set_rotates_to( jo.get_as_string_array( "rotates_to" ) );
    }
    optional( jo, was_loaded, "coverage", coverage );
    optional( jo, was_loaded, "curtain_transform", curtain_transform );
    optional( jo, was_loaded, "emissions", emissions );

    if( jo.has_string( "examine_action" ) ) {
        examine_actor = nullptr;
        examine_func = iexamine_functions_from_string( jo.get_string( "examine_action" ) );
    } else if( jo.has_object( "examine_action" ) ) {
        JsonObject data = jo.get_object( "examine_action" );
        examine_actor = iexamine_actor_from_jsobj( data );
        examine_actor->load( data, src );
        examine_func = iexamine_functions_from_string( "invalid" );
    } else if( !was_loaded ) {
        examine_actor = nullptr;
        examine_func = iexamine_functions_from_string( "none" );
    }

    if( was_loaded && jo.has_member( "flags" ) ) {
        unset_flags();
    }
    for( auto &flag : jo.get_string_array( "flags" ) ) {
        set_flag( flag );
    }
    if( was_loaded && jo.has_member( "extend" ) ) {
        JsonObject joe = jo.get_object( "extend" );
        for( auto &flag : joe.get_string_array( "flags" ) ) {
            set_flag( flag );
        }
        if( joe.has_member( "connect_groups" ) ) {
            set_connect_groups( joe.get_as_string_array( "connect_groups" ) );
        }
        if( joe.has_member( "connects_to" ) ) {
            set_connect_groups( joe.get_as_string_array( "connects_to" ) );
        }
        if( joe.has_member( "rotates_to" ) ) {
            set_connect_groups( joe.get_as_string_array( "rotates_to" ) );
        }
    }
    if( was_loaded && jo.has_member( "delete" ) ) {
        JsonObject jod = jo.get_object( "delete" );
        for( auto &flag : jod.get_string_array( "flags" ) ) {
            unset_flag( flag );
        }
        if( jod.has_member( "connect_groups" ) ) {
            unset_connect_groups( jod.get_as_string_array( "connect_groups" ) );
        }
        if( jod.has_member( "connects_to" ) ) {
            unset_connect_groups( jod.get_as_string_array( "connects_to" ) );
        }
        if( jod.has_member( "rotates_to" ) ) {
            unset_connect_groups( jod.get_as_string_array( "rotates_to" ) );
        }
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

    if( jo.has_object( "liquid_source" ) ) {
        JsonObject liquid_source = jo.get_object( "liquid_source" );
        mandatory( liquid_source, was_loaded, "id", liquid_source_item_id );
        optional( liquid_source, was_loaded, "min_temp", liquid_source_min_temp );
        if( liquid_source.has_int( "count" ) ) {
            mandatory( liquid_source, was_loaded, "count", liquid_source_count.first );
            mandatory( liquid_source, was_loaded, "count", liquid_source_count.second );
        } else if( liquid_source.has_array( "count" ) ) {
            JsonArray ja = liquid_source.get_array( "count" );
            liquid_source_count = { ja.get_int( 0 ), ja.get_int( 1 ) };
        }
    }

    optional( jo, was_loaded, "curtain_transform", curtain_transform );
    int legacy_floor_bedding_warmth = units::to_legacy_bodypart_temp_delta(
                                          floor_bedding_warmth ); //TODO: Should be in map_data_common_t::load?
    optional( jo, was_loaded, "floor_bedding_warmth", legacy_floor_bedding_warmth, 0 );
    floor_bedding_warmth = units::from_legacy_bodypart_temp_delta( legacy_floor_bedding_warmth );

    optional( jo, was_loaded, "lockpick_message", lockpick_message, translation() );
    optional( jo, was_loaded, "light_emitted", light_emitted );

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
    optional( jo, was_loaded, "move_cost", movecost );
    assign( jo, "max_volume", max_volume, src == "dda" );
    optional( jo, was_loaded, "trap", trap_id_str );
    optional( jo, was_loaded, "heat_radiation", heat_radiation );

    load_symbol_color( jo, "terrain " + id.str() );

    trap = tr_null;

    optional( jo, was_loaded, "allowed_template_ids", allowed_template_id );

    optional( jo, was_loaded, "open", open, ter_str_id::NULL_ID() );
    optional( jo, was_loaded, "close", close, ter_str_id::NULL_ID() );
    optional( jo, was_loaded, "transforms_into", transforms_into, ter_str_id::NULL_ID() );
    optional( jo, was_loaded, "roof", roof, ter_str_id::NULL_ID() );

    optional( jo, was_loaded, "lockpick_result", lockpick_result, ter_str_id::NULL_ID() );

    oxytorch = cata::make_value<activity_data_ter>();
    if( jo.has_object( "oxytorch" ) ) { //TODO: Make overwriting these with eg "oxytorch": { } work while still allowing overwriting single values
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

    if( jo.has_object( "bash" ) ) {
        if( !bash ) {
            bash.emplace();
        }
        bash->load( jo.get_object( "bash" ), was_loaded,
                    "terrain " +
                    id.str() ); //TODO: Make overwriting these with "bash": { } works while still allowing overwriting single values ie for "ter_set"
    }
    if( jo.has_object( "deconstruct" ) ) {
        if( !deconstruct ) {
            deconstruct.emplace();
        }
        deconstruct->load( jo.get_object( "deconstruct" ), was_loaded, "terrain " + id.str() );
    }
}

void map_common_bash_info::check( const std::string &id ) const
{
    if( !drop_group.is_empty() ) {
        if( !item_group::group_is_defined( drop_group ) ) {
            debugmsg( "%s: bash result item group %s does not exist", id, drop_group.c_str() );
        }
    }
}
void map_ter_bash_info::check( const std::string &id ) const
{
    map_common_bash_info::check( id );
    if( str_max != -1 ) {
        if( ter_set.is_empty() ) { // Some tiles specify t_null explicitly
            debugmsg( "bash result terrain of %s is undefined/empty", id );
        }
        if( !ter_set.is_valid() ) {
            debugmsg( "bash result terrain %s of %s does not exist", ter_set.c_str(), id );
        }
    }
}
void map_furn_bash_info::check( const std::string &id ) const
{
    map_common_bash_info::check( id );
    if( str_max != -1 ) {
        if( !furn_set.is_valid() ) {
            debugmsg( "bash result furniture %s of %s does not exist", furn_set.c_str(), id );
        }
    }
}
void map_fd_bash_info::check( const std::string &id ) const
{
    map_common_bash_info::check( id );
}

void map_common_deconstruct_info::check( const std::string &id ) const
{
    if( !item_group::group_is_defined( drop_group ) ) {
        debugmsg( "%s: deconstruct result item group %s does not exist", id, drop_group.c_str() );
    }
}

void map_ter_deconstruct_info::check( const std::string &id ) const
{
    if( !ter_set.is_valid() ) {
        debugmsg( "deconstruct result terrain %s of %s does not exist", ter_set.c_str(), id );
    }
    map_common_deconstruct_info::check( id );
}

void map_furn_deconstruct_info::check( const std::string &id ) const
{
    if( !furn_set.is_valid() ) {
        debugmsg( "deconstruct result furniture %s of %s does not exist", furn_set.c_str(), id );
    }
    map_common_deconstruct_info::check( id );
}

bool ter_t::is_smashable() const
{
    return bash && !bash->bash_below;
}

void ter_t::check() const
{
    map_data_common_t::check();
    if( bash ) {
        bash->check( id.c_str() );
    }
    if( deconstruct ) {
        deconstruct->check( id.c_str() );
    }

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
    if( has_flag( ter_furn_flag::TFLAG_EASY_DECONSTRUCT ) && !deconstruct ) {
        debugmsg( "ter %s has EASY_DECONSTRUCT flag but cannot be deconstructed",
                  id.c_str() );
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
    mandatory( jo, was_loaded, "move_cost_mod", movecost );
    mandatory( jo, was_loaded, "required_str", move_str_req );
    optional( jo, was_loaded, "fall_damage_reduction", fall_damage_reduction, 0 );
    int legacy_bonus_fire_warmth_feet = units::to_legacy_bodypart_temp_delta( bonus_fire_warmth_feet );
    optional( jo, was_loaded, "bonus_fire_warmth_feet", legacy_bonus_fire_warmth_feet, 300 );
    bonus_fire_warmth_feet = units::from_legacy_bodypart_temp_delta( legacy_bonus_fire_warmth_feet );
    optional( jo, was_loaded, "keg_capacity", keg_capacity, volume_reader(), 0_ml );
    optional( jo, was_loaded, "max_volume", max_volume, volume_reader(), DEFAULT_TILE_VOLUME );
    optional( jo, was_loaded, "crafting_pseudo_item", crafting_pseudo_item, itype_id() );
    optional( jo, was_loaded, "deployed_item", deployed_item );
    load_symbol_color( jo, "furniture " + id.str() );

    optional( jo, was_loaded, "open", open, string_id_reader<furn_t> {}, furn_str_id::NULL_ID() );
    optional( jo, was_loaded, "close", close, string_id_reader<furn_t> {}, furn_str_id::NULL_ID() );

    optional( jo, was_loaded, "lockpick_result", lockpick_result, string_id_reader<furn_t> {},
              furn_str_id::NULL_ID() );

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

    if( jo.has_object( "bash" ) ) {
        if( !bash ) {
            bash.emplace();
        }
        bash->load( jo.get_object( "bash" ), was_loaded, "furniture " + id.str() );
    }
    if( jo.has_object( "deconstruct" ) ) {
        if( !deconstruct ) {
            deconstruct.emplace();
        }
        deconstruct->load( jo.get_object( "deconstruct" ), was_loaded, "furniture " + id.str() );
    }

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

bool furn_t::is_smashable() const
{
    return bash && !bash->bash_below;
}

void furn_t::check() const
{
    map_data_common_t::check();
    if( bash ) {
        bash->check( id.c_str() );
    }
    if( deconstruct ) {
        deconstruct->check( id.c_str() );
    }

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
    if( plant && !plant->transform.is_valid() ) {
        debugmsg( "Invalid furniture %s for plant transform in furn %s", plant->transform.c_str(),
                  id.c_str() );
    }
    if( plant && !plant->base.is_valid() ) {
        debugmsg( "Invalid furniture %s for plant base in furn %s", plant->base.c_str(), id.c_str() );
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
    optional( jo, was_loaded, "result", result_, furn_str_id::NULL_ID() );
    activity_data_common::load( jo );
    valid_ = true;
}

void check_furniture_and_terrain()
{
    terrain_data.check();
    furniture_data.check();
}
