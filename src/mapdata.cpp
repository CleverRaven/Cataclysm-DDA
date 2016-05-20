#include "mapdata.h"
#include "color.h"
#include "init.h"
#include "game_constants.h"
#include "generic_factory.h"
#include "debug.h"
#include "translations.h"
#include "output.h"
#include "item.h"
#include "item_group.h"

#include <unordered_map>

const std::set<std::string> classic_extras = { "mx_helicopter", "mx_military",
"mx_roadblock", "mx_drugdeal", "mx_supplydrop", "mx_minefield",
"mx_crater", "mx_collegekids"
};

template<>
const string_id<ter_t> string_id<ter_t>::NULL_ID( "t_null", 0 );

namespace
{

generic_factory<ter_t> terrain_data( "terrain", "id", "aliases" );

}

std::vector<furn_t> furnlist;
std::map<std::string, furn_t> furnmap;

template<>
inline bool int_id<ter_t>::is_valid() const
{
    return terrain_data.is_valid( *this );
}

template<>
const ter_t &int_id<ter_t>::obj() const
{
    return terrain_data.obj( *this );
}

template<>
const string_id<ter_t> &int_id<ter_t>::id() const
{
    return terrain_data.convert( *this );
}

template<>
int_id<ter_t> string_id<ter_t>::id() const
{
    return terrain_data.convert( *this, t_null );
}

template<>
int_id<ter_t>::int_id( const string_id<ter_t> &id ) : _id( id.id() )
{
}

template<>
const ter_t &string_id<ter_t>::obj() const
{
    return terrain_data.obj( *this );
}

template<>
bool string_id<ter_t>::is_valid() const
{
    return terrain_data.is_valid( *this );
}

template<>
const furn_t &int_id<furn_t>::obj() const
{
    if( static_cast<size_t>( _id ) >= furnlist.size() ) {
        debugmsg( "invalid furniture id %d", _id );
        static const furn_t dummy{};
        return dummy;
    }
    return furnlist[_id];
}

static const std::unordered_map<std::string, ter_bitflags> ter_bitflags_map = { {
    { "DESTROY_ITEM",             TFLAG_DESTROY_ITEM },   // add/spawn_item*()
    { "ROUGH",                    TFLAG_ROUGH },          // monmove
    { "UNSTABLE",                 TFLAG_UNSTABLE },       // monmove
    { "LIQUID",                   TFLAG_LIQUID },         // *move(), add/spawn_item*()
    { "FIRE_CONTAINER",           TFLAG_FIRE_CONTAINER }, // fire
    { "DIGGABLE",                 TFLAG_DIGGABLE },       // monmove
    { "SUPPRESS_SMOKE",           TFLAG_SUPPRESS_SMOKE }, // fire
    { "FLAMMABLE_HARD",           TFLAG_FLAMMABLE_HARD }, // fire
    { "SEALED",                   TFLAG_SEALED },         // Fire, acid
    { "ALLOW_FIELD_EFFECT",       TFLAG_ALLOW_FIELD_EFFECT }, // Fire, acid
    { "COLLAPSES",                TFLAG_COLLAPSES },      // building "remodeling"
    { "FLAMMABLE",                TFLAG_FLAMMABLE },      // fire bad! fire SLOW!
    { "REDUCE_SCENT",             TFLAG_REDUCE_SCENT },   // ...and the other half is update_scent
    { "INDOORS",                  TFLAG_INDOORS },        // vehicle gain_moves, weather
    { "SHARP",                    TFLAG_SHARP },          // monmove
    { "SUPPORTS_ROOF",            TFLAG_SUPPORTS_ROOF },  // and by building "remodeling" I mean hulkSMASH
    { "MINEABLE",                 TFLAG_MINEABLE },       // allows mining
    { "SWIMMABLE",                TFLAG_SWIMMABLE },      // monmove, many fields
    { "TRANSPARENT",              TFLAG_TRANSPARENT },    // map::trans / lightmap
    { "NOITEM",                   TFLAG_NOITEM },         // add/spawn_item*()
    { "FLAMMABLE_ASH",            TFLAG_FLAMMABLE_ASH },  // oh hey fire. again.
    { "PLANT",                    TFLAG_PLANT },          // full map iteration
    { "WALL",                     TFLAG_WALL },           // smells
    { "DEEP_WATER",               TFLAG_DEEP_WATER },     // Deep enough to submerge things
    { "HARVESTED",                TFLAG_HARVESTED },      // harvested.  will not bear fruit.
    { "PERMEABLE",                TFLAG_PERMEABLE },      // gases can flow through.
    { "AUTO_WALL_SYMBOL",         TFLAG_AUTO_WALL_SYMBOL }, // automatically create the appropriate wall
    { "CONNECT_TO_WALL",          TFLAG_CONNECT_TO_WALL }, // superseded by ter_connects, retained for json backward compatibilty
    { "CLIMBABLE",                TFLAG_CLIMBABLE },      // Can be climbed over
    { "GOES_DOWN",                TFLAG_GOES_DOWN },      // Allows non-flying creatures to move downwards
    { "GOES_UP",                  TFLAG_GOES_UP },        // Allows non-flying creatures to move upwards
    { "NO_FLOOR",                 TFLAG_NO_FLOOR },       // Things should fall when placed on this tile
    { "SEEN_FROM_ABOVE",          TFLAG_SEEN_FROM_ABOVE },// This should be visible if the tile above has no floor
    { "RAMP",                     TFLAG_RAMP },           // Can be used to move up a z-level
} };

static const std::unordered_map<std::string, ter_connects> ter_connects_map = { {
    { "WALL",                     TERCONN_WALL },         // implied by TFLAG_CONNECT_TO_WALL, TFLAG_AUTO_WALL_SYMBOL or TFLAG_WALL
    { "CHAINFENCE",               TERCONN_CHAINFENCE },
    { "WOODFENCE",                TERCONN_WOODFENCE },
    { "RAILING",                  TERCONN_RAILING },
} };

void load_map_bash_tent_centers( JsonArray ja, std::vector<std::string> &centers ) {
    while ( ja.has_more() ) {
        centers.push_back( ja.next_string() );
    }
}

bool map_bash_info::load(JsonObject &jsobj, std::string member, bool isfurniture) {
    if( !jsobj.has_object(member) ) {
        return false;
    }

    JsonObject j = jsobj.get_object(member);
    str_min = j.get_int("str_min", 0);
    str_max = j.get_int("str_max", 0);

    str_min_blocked = j.get_int("str_min_blocked", -1);
    str_max_blocked = j.get_int("str_max_blocked", -1);

    str_min_supported = j.get_int("str_min_supported", -1);
    str_max_supported = j.get_int("str_max_supported", -1);

    explosive = j.get_int("explosive", -1);

    sound_vol = j.get_int("sound_vol", -1);
    sound_fail_vol = j.get_int("sound_fail_vol", -1);

    collapse_radius = j.get_int( "collapse_radius", 1 );

    destroy_only = j.get_bool("destroy_only", false);

    bash_below = j.get_bool("bash_below", false);

    sound = j.get_string("sound", _("smash!"));
    sound_fail = j.get_string("sound_fail", _("thump!"));

    if( isfurniture ) {
        furn_set = j.get_string("furn_set", "f_null");
    } else {
        ter_set = ter_str_id( j.get_string( "ter_set" ) );
    }

    if( j.has_member( "items" ) ) {
        JsonIn& stream = *j.get_raw( "items" );
        drop_group = item_group::load_item_group( stream, "collection" );
    } else {
        drop_group = "EMPTY_GROUP";
    }

    if( j.has_array("tent_centers") ) {
        load_map_bash_tent_centers( j.get_array("tent_centers"), tent_centers );
    }

    return true;
}

bool map_deconstruct_info::load(JsonObject &jsobj, std::string member, bool isfurniture)
{
    if (!jsobj.has_object(member)) {
        return false;
    }
    JsonObject j = jsobj.get_object(member);
    furn_set = j.get_string("furn_set", "");

    if (!isfurniture) {
        ter_set = ter_str_id( j.get_string( "ter_set" ) );
    }
    can_do = true;

    JsonIn& stream = *j.get_raw( "items" );
    drop_group = item_group::load_item_group( stream, "collection" );
    return true;
}

furn_t null_furniture_t() {
  furn_t new_furniture;
  new_furniture.id = "f_null";
  new_furniture.name = _("nothing");
  new_furniture.symbol_.fill( ' ' );
  new_furniture.color_.fill( c_white );
  new_furniture.movecost = 0;
  new_furniture.move_str_req = -1;
  new_furniture.transparent = true;
  new_furniture.set_flag("TRANSPARENT");
  new_furniture.examine = iexamine_function_from_string("none");
  new_furniture.loadid = furn_id( 0 );
  new_furniture.open = "";
  new_furniture.close = "";
  new_furniture.max_volume = MAX_VOLUME_IN_SQUARE;
  return new_furniture;
}

ter_t null_terrain_t() {
  ter_t new_terrain;

  new_terrain.id = NULL_ID;
  new_terrain.name = _("nothing");
  new_terrain.symbol_.fill( ' ' );
  new_terrain.color_.fill( c_white );
  new_terrain.movecost = 2;
  new_terrain.transparent = true;
  new_terrain.set_flag("TRANSPARENT");
  new_terrain.set_flag("DIGGABLE");
  new_terrain.examine = iexamine_function_from_string("none");
  new_terrain.max_volume = MAX_VOLUME_IN_SQUARE;
  return new_terrain;
}

long string_to_symbol( JsonIn &js )
{
    const std::string s = js.get_string();
    if( s == "LINE_XOXO" ) {
        return LINE_XOXO;
    } else if( s == "LINE_OXOX" ) {
        return LINE_OXOX;
    } else if( s.length() != 1 ) {
        js.error( "Symbol string must be exactly 1 character long." );
    }
    return s[0];
}

template<typename C, typename F>
void load_season_array( JsonIn &js, C &container, F load_func )
{
    if( js.test_array() ) {
        js.start_array();
        for( auto &season_entry : container ) {
            season_entry = load_func( js );
            js.end_array(); // consume separator
        }
    } else {
        container.fill( load_func( js ) );
    }
}

nc_color bgcolor_from_json( JsonIn &js)
{
    return bgcolor_from_string( js.get_string() );
}

nc_color color_from_json( JsonIn &js)
{
    return color_from_string( js.get_string() );
}

void map_data_common_t::load_symbol( JsonObject &jo )
{
    load_season_array( *jo.get_raw( "symbol" ), symbol_, string_to_symbol );

    const bool has_color = jo.has_member( "color" );
    const bool has_bgcolor = jo.has_member( "bgcolor" );
    if( has_color && has_bgcolor ) {
        jo.throw_error( "Found both color and bgcolor, only one of these is allowed." );
    } else if( has_color ) {
        load_season_array( *jo.get_raw( "color" ), color_, color_from_json );
    } else if( has_bgcolor ) {
        load_season_array( *jo.get_raw( "bgcolor" ), color_, bgcolor_from_json );
    } else {
        jo.throw_error( "Missing member: one of: \"color\", \"bgcolor\" must exist." );
    }
}

long map_data_common_t::symbol() const
{
    return symbol_[calendar::turn.get_season()];
}

nc_color map_data_common_t::color() const
{
    return color_[calendar::turn.get_season()];
}

void load_furniture(JsonObject &jsobj)
{
  if ( furnlist.empty() ) {
      furn_t new_null = null_furniture_t();
      furnmap[new_null.id] = new_null;
      furnlist.push_back(new_null);
  }
  furn_t new_furniture;
  new_furniture.id = jsobj.get_string("id");
  if ( new_furniture.id == "f_null" ) {
      return;
  }
  new_furniture.name = _(jsobj.get_string("name").c_str());

    new_furniture.load_symbol( jsobj );

  new_furniture.movecost = jsobj.get_int("move_cost_mod");
  new_furniture.move_str_req = jsobj.get_int("required_str");
  new_furniture.max_volume = jsobj.get_int("max_volume", MAX_VOLUME_IN_SQUARE);

  new_furniture.crafting_pseudo_item = jsobj.get_string("crafting_pseudo_item", "");

  new_furniture.transparent = false;
    for( auto & flag : jsobj.get_string_array( "flags" ) ) {
        new_furniture.set_flag( flag );
    }

  if(jsobj.has_member("examine_action")) {
    std::string function_name = jsobj.get_string("examine_action");
    new_furniture.examine = iexamine_function_from_string(function_name);
  } else {
    //If not specified, default to no action
    new_furniture.examine = iexamine_function_from_string("none");
  }

  new_furniture.open = "";
  if ( jsobj.has_member("open") ) {
      new_furniture.open = jsobj.get_string("open");
  }
  new_furniture.close = "";
  if ( jsobj.has_member("close") ) {
      new_furniture.close = jsobj.get_string("close");
  }
  new_furniture.bash.load(jsobj, "bash", true);
  new_furniture.deconstruct.load(jsobj, "deconstruct", true);

  new_furniture.loadid = furn_id( furnlist.size() );
  furnmap[new_furniture.id] = new_furniture;
  furnlist.push_back(new_furniture);
}

void load_terrain(JsonObject &jsobj)
{
    if( terrain_data.empty() ) { // todo@ This shouldn't live here
        terrain_data.insert( null_terrain_t() );
    }
    terrain_data.load( jsobj );
}

void map_data_common_t::set_flag( const std::string &flag )
{
    flags.insert( flag );
    auto const it = ter_bitflags_map.find( flag );
    if( it != ter_bitflags_map.end() ) {
        bitflags.set( it->second );
        if( !transparent && it->second == TFLAG_TRANSPARENT ) {
            transparent = true;
        }
        // wall connection check for JSON backwards compatibility
        if( it->second == TFLAG_WALL || it->second == TFLAG_CONNECT_TO_WALL ) {
            set_connects( "WALL" );
        }
    }
}

void map_data_common_t::set_connects( const std::string &connect_group_string )
{
    auto const it = ter_connects_map.find( connect_group_string );
    if( it != ter_connects_map.end() ) {
        connect_group = it->second;
    }
    else { // arbitrary connect groups are a bad idea for optimisation reasons
        debugmsg( "can't find terrain connection group %s", connect_group_string.c_str() );
    }
}

bool map_data_common_t::connects( int &ret ) const {
    if ( connect_group != TERCONN_NONE ) {
        ret = connect_group;
        return true;
    }
    return false;
}

ter_id t_null,
    t_hole, // Real nothingness; makes you fall a z-level
    // Ground
    t_dirt, t_sand, t_dirtmound, t_pit_shallow, t_pit,
    t_pit_corpsed, t_pit_covered, t_pit_spiked, t_pit_spiked_covered, t_pit_glass, t_pit_glass_covered,
    t_rock_floor,
    t_grass,
    t_metal_floor,
    t_pavement, t_pavement_y, t_sidewalk, t_concrete,
    t_floor, t_floor_waxed,
    t_dirtfloor,//Dirt floor(Has roof)
    t_carpet_red,t_carpet_yellow,t_carpet_purple,t_carpet_green,
    t_linoleum_white, t_linoleum_gray,
    t_grate,
    t_slime,
    t_bridge,
    t_covered_well,
    // Lighting related
    t_utility_light,
    // Walls
    t_wall_log_half, t_wall_log, t_wall_log_chipped, t_wall_log_broken, t_palisade, t_palisade_gate, t_palisade_gate_o,
    t_wall_half, t_wall_wood, t_wall_wood_chipped, t_wall_wood_broken,
    t_wall, t_concrete_wall, t_brick_wall,
    t_wall_metal,
    t_wall_glass,
    t_wall_glass_alarm,
    t_reinforced_glass,
    t_bars,
    t_wall_r,t_wall_w,t_wall_b,t_wall_g,t_wall_p,t_wall_y,
    t_door_c, t_door_c_peep, t_door_b, t_door_b_peep, t_door_o, t_door_o_peep, t_rdoor_c, t_rdoor_b, t_rdoor_o,t_door_locked_interior, t_door_locked, t_door_locked_peep, t_door_locked_alarm, t_door_frame,
    t_chaingate_l, t_fencegate_c, t_fencegate_o, t_chaingate_c, t_chaingate_o,
    t_door_boarded, t_door_boarded_damaged, t_door_boarded_peep, t_rdoor_boarded, t_rdoor_boarded_damaged, t_door_boarded_damaged_peep,
    t_door_metal_c, t_door_metal_o, t_door_metal_locked, t_door_metal_pickable, t_mdoor_frame,
    t_door_bar_c, t_door_bar_o, t_door_bar_locked,
    t_door_glass_c, t_door_glass_o,
    t_portcullis,
    t_recycler, t_window, t_window_taped, t_window_domestic, t_window_domestic_taped, t_window_open, t_curtains,
    t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded,
    t_window_boarded_noglass, t_window_reinforced, t_window_reinforced_noglass, t_window_enhanced, t_window_enhanced_noglass, t_window_bars_alarm, t_window_bars,
    t_window_stained_green, t_window_stained_red, t_window_stained_blue,
    t_window_no_curtains, t_window_no_curtains_open, t_window_no_curtains_taped,
    t_rock, t_fault,
    t_paper,
    t_rock_wall, t_rock_wall_half,
    // Tree
    t_tree, t_tree_young, t_tree_apple, t_tree_apple_harvested, t_tree_pear, t_tree_pear_harvested, t_tree_cherry, t_tree_cherry_harvested,
    t_tree_peach, t_tree_peach_harvested, t_tree_apricot, t_tree_apricot_harvested, t_tree_plum, t_tree_plum_harvested,
    t_tree_pine, t_tree_blackjack, t_tree_birch, t_tree_willow, t_tree_maple, t_tree_maple_tapped, t_tree_hickory, t_tree_hickory_dead, t_tree_hickory_harvested, t_tree_deadpine, t_underbrush, t_shrub, t_shrub_blueberry, t_shrub_strawberry, t_trunk,
    t_root_wall,
    t_wax, t_floor_wax,
    t_fence_v, t_fence_h, t_chainfence_v, t_chainfence_h, t_chainfence_posts,
    t_fence_post, t_fence_wire, t_fence_barbed, t_fence_rope,
    t_railing_v, t_railing_h,
    // Nether
    t_marloss, t_fungus_floor_in, t_fungus_floor_sup, t_fungus_floor_out, t_fungus_wall,
    t_fungus_mound, t_fungus, t_shrub_fungal, t_tree_fungal, t_tree_fungal_young, t_marloss_tree,
    // Water, lava, etc.
    t_water_sh, t_water_dp, t_swater_sh, t_swater_dp, t_water_pool, t_sewage,
    t_lava,
    // More embellishments than you can shake a stick at.
    t_sandbox, t_slide, t_monkey_bars, t_backboard,
    t_gas_pump, t_gas_pump_smashed,
    t_diesel_pump, t_diesel_pump_smashed,
    t_atm,
    t_generator_broken,
    t_missile, t_missile_exploded,
    t_radio_tower, t_radio_controls,
    t_console_broken, t_console, t_gates_mech_control, t_gates_control_concrete, t_gates_control_brick, t_barndoor, t_palisade_pulley,
    t_gates_control_metal,
    t_sewage_pipe, t_sewage_pump,
    t_centrifuge,
    t_column,
    t_vat,
    t_cvdbody, t_cvdmachine,
    t_water_pump,
    t_conveyor, t_machinery_light, t_machinery_heavy, t_machinery_old, t_machinery_electronic,
    t_improvised_shelter,
    // Staircases etc.
    t_stairs_down, t_stairs_up, t_manhole, t_ladder_up, t_ladder_down, t_slope_down,
     t_slope_up, t_rope_up,
    t_manhole_cover,
    // Special
    t_card_science, t_card_military, t_card_reader_broken, t_slot_machine,
     t_elevator_control, t_elevator_control_off, t_elevator, t_pedestal_wyrm,
     t_pedestal_temple,
    // Temple tiles
    t_rock_red, t_rock_green, t_rock_blue, t_floor_red, t_floor_green, t_floor_blue,
    t_switch_rg, t_switch_gb, t_switch_rb, t_switch_even, t_open_air, t_plut_generator,
    t_pavement_bg_dp, t_pavement_y_bg_dp, t_sidewalk_bg_dp, t_guardrail_bg_dp;

// @todo Put this crap into an inclusion, which should be generated automatically using JSON data

void set_ter_ids() {
    t_null                      = ter_id( "t_null" );
    t_hole                      = ter_id( "t_hole" );
    t_dirt                      = ter_id( "t_dirt" );
    t_sand                      = ter_id( "t_sand" );
    t_dirtmound                 = ter_id( "t_dirtmound" );
    t_pit_shallow               = ter_id( "t_pit_shallow" );
    t_pit                       = ter_id( "t_pit" );
    t_pit_corpsed               = ter_id( "t_pit_corpsed" );
    t_pit_covered               = ter_id( "t_pit_covered" );
    t_pit_spiked                = ter_id( "t_pit_spiked" );
    t_pit_spiked_covered        = ter_id( "t_pit_spiked_covered" );
    t_pit_glass                 = ter_id( "t_pit_glass" );
    t_pit_glass_covered         = ter_id( "t_pit_glass_covered" );
    t_rock_floor                = ter_id( "t_rock_floor" );
    t_grass                     = ter_id( "t_grass" );
    t_metal_floor               = ter_id( "t_metal_floor" );
    t_pavement                  = ter_id( "t_pavement" );
    t_pavement_y                = ter_id( "t_pavement_y" );
    t_sidewalk                  = ter_id( "t_sidewalk" );
    t_concrete                  = ter_id( "t_concrete" );
    t_floor                     = ter_id( "t_floor" );
    t_floor_waxed               = ter_id( "t_floor_waxed" );
    t_dirtfloor                 = ter_id( "t_dirtfloor" );
    t_carpet_red                = ter_id( "t_carpet_red" );
    t_carpet_yellow             = ter_id( "t_carpet_yellow" );
    t_carpet_purple             = ter_id( "t_carpet_purple" );
    t_carpet_green              = ter_id( "t_carpet_green" );
    t_linoleum_white            = ter_id( "t_linoleum_white" );
    t_linoleum_gray             = ter_id( "t_linoleum_gray" );
    t_grate                     = ter_id( "t_grate" );
    t_slime                     = ter_id( "t_slime" );
    t_bridge                    = ter_id( "t_bridge" );
    t_utility_light             = ter_id( "t_utility_light" );
    t_wall_log_half             = ter_id( "t_wall_log_half" );
    t_wall_log                  = ter_id( "t_wall_log" );
    t_wall_log_chipped          = ter_id( "t_wall_log_chipped" );
    t_wall_log_broken           = ter_id( "t_wall_log_broken" );
    t_palisade                  = ter_id( "t_palisade" );
    t_palisade_gate             = ter_id( "t_palisade_gate" );
    t_palisade_gate_o           = ter_id( "t_palisade_gate_o" );
    t_wall_half                 = ter_id( "t_wall_half" );
    t_wall_wood                 = ter_id( "t_wall_wood" );
    t_wall_wood_chipped         = ter_id( "t_wall_wood_chipped" );
    t_wall_wood_broken          = ter_id( "t_wall_wood_broken" );
    t_wall                      = ter_id( "t_wall" );
    t_concrete_wall             = ter_id( "t_concrete_wall" );
    t_brick_wall                = ter_id( "t_brick_wall" );
    t_wall_metal                = ter_id( "t_wall_metal" );
    t_wall_glass                = ter_id( "t_wall_glass" );
    t_wall_glass_alarm          = ter_id( "t_wall_glass_alarm" );
    t_reinforced_glass          = ter_id( "t_reinforced_glass" );
    t_bars                      = ter_id( "t_bars" );
    t_wall_b                    = ter_id( "t_wall_b" );
    t_wall_g                    = ter_id( "t_wall_g" );
    t_wall_p                    = ter_id( "t_wall_p" );
    t_wall_r                    = ter_id( "t_wall_r" );
    t_wall_w                    = ter_id( "t_wall_w" );
    t_door_c                    = ter_id( "t_door_c" );
    t_door_c_peep               = ter_id( "t_door_c_peep" );
    t_door_b                    = ter_id( "t_door_b" );
    t_door_b_peep               = ter_id( "t_door_b_peep" );
    t_door_o                    = ter_id( "t_door_o" );
    t_door_o_peep               = ter_id( "t_door_o_peep" );
    t_rdoor_c                   = ter_id( "t_rdoor_c" );
    t_rdoor_b                   = ter_id( "t_rdoor_b" );
    t_rdoor_o                   = ter_id( "t_rdoor_o" );
    t_door_locked_interior      = ter_id( "t_door_locked_interior" );
    t_door_locked               = ter_id( "t_door_locked" );
    t_door_locked_peep          = ter_id( "t_door_locked_peep" );
    t_door_locked_alarm         = ter_id( "t_door_locked_alarm" );
    t_door_frame                = ter_id( "t_door_frame" );
    t_mdoor_frame               = ter_id( "t_mdoor_frame" );
    t_chaingate_l               = ter_id( "t_chaingate_l" );
    t_fencegate_c               = ter_id( "t_fencegate_c" );
    t_fencegate_o               = ter_id( "t_fencegate_o" );
    t_chaingate_c               = ter_id( "t_chaingate_c" );
    t_chaingate_o               = ter_id( "t_chaingate_o" );
    t_door_boarded              = ter_id( "t_door_boarded" );
    t_door_boarded_damaged      = ter_id( "t_door_boarded_damaged" );
    t_door_boarded_peep         = ter_id( "t_door_boarded_peep" );
    t_rdoor_boarded             = ter_id( "t_rdoor_boarded" );
    t_rdoor_boarded_damaged     = ter_id( "t_rdoor_boarded_damaged" );
    t_door_boarded_damaged_peep = ter_id( "t_door_boarded_damaged_peep" );
    t_door_metal_c              = ter_id( "t_door_metal_c" );
    t_door_metal_o              = ter_id( "t_door_metal_o" );
    t_door_metal_locked         = ter_id( "t_door_metal_locked" );
    t_door_metal_pickable       = ter_id( "t_door_metal_pickable" );
    t_door_bar_c                = ter_id( "t_door_bar_c" );
    t_door_bar_o                = ter_id( "t_door_bar_o" );
    t_door_bar_locked           = ter_id( "t_door_bar_locked" );
    t_door_glass_c              = ter_id( "t_door_glass_c" );
    t_door_glass_o              = ter_id( "t_door_glass_o" );
    t_portcullis                = ter_id( "t_portcullis" );
    t_recycler                  = ter_id( "t_recycler" );
    t_window                    = ter_id( "t_window" );
    t_window_taped              = ter_id( "t_window_taped" );
    t_window_domestic           = ter_id( "t_window_domestic" );
    t_window_domestic_taped     = ter_id( "t_window_domestic_taped" );
    t_window_open               = ter_id( "t_window_open" );
    t_curtains                  = ter_id( "t_curtains" );
    t_window_alarm              = ter_id( "t_window_alarm" );
    t_window_alarm_taped        = ter_id( "t_window_alarm_taped" );
    t_window_empty              = ter_id( "t_window_empty" );
    t_window_frame              = ter_id( "t_window_frame" );
    t_window_boarded            = ter_id( "t_window_boarded" );
    t_window_boarded_noglass    = ter_id( "t_window_boarded_noglass" );
    t_window_reinforced         = ter_id( "t_window_reinforced" );
    t_window_reinforced_noglass = ter_id( "t_window_reinforced_noglass" );
    t_window_enhanced           = ter_id( "t_window_enhanced" );
    t_window_enhanced_noglass   = ter_id( "t_window_enhanced_noglass" );
    t_window_bars_alarm         = ter_id( "t_window_bars_alarm" );
    t_window_bars               = ter_id( "t_window_bars" );
    t_window_stained_green      = ter_id( "t_window_stained_green" );
    t_window_stained_red        = ter_id( "t_window_stained_red" );
    t_window_stained_blue       = ter_id( "t_window_stained_blue" );
    t_window_no_curtains        = ter_id( "t_window_no_curtains" );
    t_window_no_curtains_open   = ter_id( "t_window_no_curtains_open" );
    t_window_no_curtains_taped  = ter_id( "t_window_no_curtains_taped" );
    t_rock                      = ter_id( "t_rock" );
    t_fault                     = ter_id( "t_fault" );
    t_paper                     = ter_id( "t_paper" );
    t_rock_wall                 = ter_id( "t_rock_wall" );
    t_rock_wall_half            = ter_id( "t_rock_wall_half" );
    t_tree                      = ter_id( "t_tree" );
    t_tree_young                = ter_id( "t_tree_young" );
    t_tree_apple                = ter_id( "t_tree_apple" );
    t_tree_apple_harvested      = ter_id( "t_tree_apple_harvested" );
    t_tree_pear                 = ter_id( "t_tree_pear" );
    t_tree_pear_harvested       = ter_id( "t_tree_pear_harvested" );
    t_tree_cherry               = ter_id( "t_tree_cherry" );
    t_tree_cherry_harvested     = ter_id( "t_tree_cherry_harvested" );
    t_tree_peach                = ter_id( "t_tree_peach" );
    t_tree_peach_harvested      = ter_id( "t_tree_peach_harvested" );
    t_tree_apricot              = ter_id( "t_tree_apricot" );
    t_tree_apricot_harvested    = ter_id( "t_tree_apricot_harvested" );
    t_tree_plum                 = ter_id( "t_tree_plum" );
    t_tree_plum_harvested       = ter_id( "t_tree_plum_harvested" );
    t_tree_pine                 = ter_id( "t_tree_pine" );
    t_tree_blackjack            = ter_id( "t_tree_blackjack" );
    t_tree_birch                = ter_id( "t_tree_birch" );
    t_tree_willow               = ter_id( "t_tree_willow" );
    t_tree_maple                = ter_id( "t_tree_maple" );
    t_tree_maple_tapped         = ter_id( "t_tree_maple_tapped" );
    t_tree_deadpine             = ter_id( "t_tree_deadpine" );
    t_tree_hickory              = ter_id( "t_tree_hickory" );
    t_tree_hickory_dead         = ter_id( "t_tree_hickory_dead" );
    t_tree_hickory_harvested    = ter_id( "t_tree_hickory_harvested" );
    t_underbrush                = ter_id( "t_underbrush" );
    t_shrub                     = ter_id( "t_shrub" );
    t_shrub_blueberry           = ter_id( "t_shrub_blueberry" );
    t_shrub_strawberry          = ter_id( "t_shrub_strawberry" );
    t_trunk                     = ter_id( "t_trunk" );
    t_root_wall                 = ter_id( "t_root_wall" );
    t_wax                       = ter_id( "t_wax" );
    t_floor_wax                 = ter_id( "t_floor_wax" );
    t_fence_v                   = ter_id( "t_fence_v" );
    t_fence_h                   = ter_id( "t_fence_h" );
    t_chainfence_v              = ter_id( "t_chainfence_v" );
    t_chainfence_h              = ter_id( "t_chainfence_h" );
    t_chainfence_posts          = ter_id( "t_chainfence_posts" );
    t_fence_post                = ter_id( "t_fence_post" );
    t_fence_wire                = ter_id( "t_fence_wire" );
    t_fence_barbed              = ter_id( "t_fence_barbed" );
    t_fence_rope                = ter_id( "t_fence_rope" );
    t_railing_v                 = ter_id( "t_railing_v" );
    t_railing_h                 = ter_id( "t_railing_h" );
    t_marloss                   = ter_id( "t_marloss" );
    t_fungus_floor_in           = ter_id( "t_fungus_floor_in" );
    t_fungus_floor_sup          = ter_id( "t_fungus_floor_sup" );
    t_fungus_floor_out          = ter_id( "t_fungus_floor_out" );
    t_fungus_wall               = ter_id( "t_fungus_wall" );
    t_fungus_mound              = ter_id( "t_fungus_mound" );
    t_fungus                    = ter_id( "t_fungus" );
    t_shrub_fungal              = ter_id( "t_shrub_fungal" );
    t_tree_fungal               = ter_id( "t_tree_fungal" );
    t_tree_fungal_young         = ter_id( "t_tree_fungal_young" );
    t_marloss_tree              = ter_id( "t_marloss_tree" );
    t_water_sh                  = ter_id( "t_water_sh" );
    t_water_dp                  = ter_id( "t_water_dp" );
    t_swater_sh                 = ter_id( "t_swater_sh" );
    t_swater_dp                 = ter_id( "t_swater_dp" );
    t_water_pool                = ter_id( "t_water_pool" );
    t_sewage                    = ter_id( "t_sewage" );
    t_lava                      = ter_id( "t_lava" );
    t_sandbox                   = ter_id( "t_sandbox" );
    t_slide                     = ter_id( "t_slide" );
    t_monkey_bars               = ter_id( "t_monkey_bars" );
    t_backboard                 = ter_id( "t_backboard" );
    t_gas_pump                  = ter_id( "t_gas_pump" );
    t_gas_pump_smashed          = ter_id( "t_gas_pump_smashed" );
    t_diesel_pump               = ter_id( "t_diesel_pump" );
    t_diesel_pump_smashed       = ter_id( "t_diesel_pump_smashed" );
    t_atm                       = ter_id( "t_atm" );
    t_generator_broken          = ter_id( "t_generator_broken" );
    t_missile                   = ter_id( "t_missile" );
    t_missile_exploded          = ter_id( "t_missile_exploded" );
    t_radio_tower               = ter_id( "t_radio_tower" );
    t_radio_controls            = ter_id( "t_radio_controls" );
    t_console_broken            = ter_id( "t_console_broken" );
    t_console                   = ter_id( "t_console" );
    t_gates_mech_control        = ter_id( "t_gates_mech_control" );
    t_gates_control_brick       = ter_id( "t_gates_control_brick" );
    t_gates_control_concrete    = ter_id( "t_gates_control_concrete" );
    t_barndoor                  = ter_id( "t_barndoor" );
    t_palisade_pulley           = ter_id( "t_palisade_pulley" );
    t_gates_control_metal       = ter_id( "t_gates_control_metal" );
    t_sewage_pipe               = ter_id( "t_sewage_pipe" );
    t_sewage_pump               = ter_id( "t_sewage_pump" );
    t_centrifuge                = ter_id( "t_centrifuge" );
    t_column                    = ter_id( "t_column" );
    t_vat                       = ter_id( "t_vat" );
    t_cvdbody                   = ter_id( "t_cvdbody" );
    t_cvdmachine                = ter_id( "t_cvdmachine" );
    t_stairs_down               = ter_id( "t_stairs_down" );
    t_stairs_up                 = ter_id( "t_stairs_up" );
    t_manhole                   = ter_id( "t_manhole" );
    t_ladder_up                 = ter_id( "t_ladder_up" );
    t_ladder_down               = ter_id( "t_ladder_down" );
    t_slope_down                = ter_id( "t_slope_down" );
    t_slope_up                  = ter_id( "t_slope_up" );
    t_rope_up                   = ter_id( "t_rope_up" );
    t_manhole_cover             = ter_id( "t_manhole_cover" );
    t_card_science              = ter_id( "t_card_science" );
    t_card_military             = ter_id( "t_card_military" );
    t_card_reader_broken        = ter_id( "t_card_reader_broken" );
    t_slot_machine              = ter_id( "t_slot_machine" );
    t_elevator_control          = ter_id( "t_elevator_control" );
    t_elevator_control_off      = ter_id( "t_elevator_control_off" );
    t_elevator                  = ter_id( "t_elevator" );
    t_pedestal_wyrm             = ter_id( "t_pedestal_wyrm" );
    t_pedestal_temple           = ter_id( "t_pedestal_temple" );
    t_rock_red                  = ter_id( "t_rock_red" );
    t_rock_green                = ter_id( "t_rock_green" );
    t_rock_blue                 = ter_id( "t_rock_blue" );
    t_floor_red                 = ter_id( "t_floor_red" );
    t_floor_green               = ter_id( "t_floor_green" );
    t_floor_blue                = ter_id( "t_floor_blue" );
    t_switch_rg                 = ter_id( "t_switch_rg" );
    t_switch_gb                 = ter_id( "t_switch_gb" );
    t_switch_rb                 = ter_id( "t_switch_rb" );
    t_switch_even               = ter_id( "t_switch_even" );
    t_covered_well              = ter_id( "t_covered_well" );
    t_water_pump                = ter_id( "t_water_pump" );
    t_conveyor                  = ter_id( "t_conveyor" );
    t_machinery_light           = ter_id( "t_machinery_light" );
    t_machinery_heavy           = ter_id( "t_machinery_heavy" );
    t_machinery_old             = ter_id( "t_machinery_old" );
    t_machinery_electronic      = ter_id( "t_machinery_electronic" );
    t_open_air                  = ter_id( "t_open_air" );
    t_plut_generator            = ter_id( "t_plut_generator" );
    t_pavement_bg_dp            = ter_id( "t_pavement_bg_dp" );
    t_pavement_y_bg_dp          = ter_id( "t_pavement_y_bg_dp" );
    t_sidewalk_bg_dp            = ter_id( "t_sidewalk_bg_dp" );
    t_guardrail_bg_dp           = ter_id( "t_guardrail_bg_dp" );
    t_improvised_shelter        = ter_id( "t_improvised_shelter" );

    for( auto &elem : terrain_data.get_all() ) {
        ter_t &ter = const_cast<ter_t&>( elem );
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

    furnmap.clear();
    furnlist.clear();
}

furn_id furnfind(const std::string & id) {
    if( furnmap.find(id) == furnmap.end() ) {
         debugmsg("Can't find %s",id.c_str());
         return furn_id( 0 );
    }
    return furnmap[id].loadid;
}

furn_id f_null,
    f_hay,
    f_rubble, f_rubble_rock, f_wreckage, f_ash,
    f_barricade_road, f_sandbag_half, f_sandbag_wall,
    f_bulletin,
    f_indoor_plant,f_indoor_plant_y,
    f_bed, f_toilet, f_makeshift_bed, f_straw_bed,
    f_sink, f_oven, f_woodstove, f_fireplace, f_bathtub,
    f_chair, f_armchair, f_sofa, f_cupboard, f_trashcan, f_desk, f_exercise,
    f_ball_mach, f_bench, f_lane, f_table, f_pool_table,
    f_counter,
    f_fridge, f_glass_fridge, f_dresser, f_locker,
    f_rack, f_bookcase,
    f_washer, f_dryer,
    f_vending_c, f_vending_o, f_dumpster, f_dive_block,
    f_crate_c, f_crate_o,
    f_large_canvas_wall, f_canvas_wall, f_canvas_door, f_canvas_door_o, f_groundsheet, f_fema_groundsheet, f_large_groundsheet,
    f_large_canvas_door, f_large_canvas_door_o, f_center_groundsheet, f_skin_wall, f_skin_door, f_skin_door_o,  f_skin_groundsheet,
    f_mutpoppy, f_flower_fungal, f_fungal_mass, f_fungal_clump,f_dahlia,f_datura,f_dandelion,f_cattails,f_bluebell,
    f_safe_c, f_safe_l, f_safe_o,
    f_plant_seed, f_plant_seedling, f_plant_mature, f_plant_harvest,
    f_fvat_empty, f_fvat_full,
    f_wood_keg,
    f_standing_tank,
    f_statue, f_egg_sackbw, f_egg_sackcs, f_egg_sackws, f_egg_sacke,
    f_flower_marloss,
    f_floor_canvas,
    f_tatami,
    f_kiln_empty, f_kiln_full, f_kiln_metal_empty, f_kiln_metal_full,
    f_robotic_arm;

void set_furn_ids() {
    f_null=furnfind("f_null");
    f_hay=furnfind("f_hay");
    f_rubble=furnfind("f_rubble");
    f_rubble_rock=furnfind("f_rubble_rock");
    f_wreckage=furnfind("f_wreckage");
    f_ash=furnfind("f_ash");
    f_barricade_road=furnfind("f_barricade_road");
    f_sandbag_half=furnfind("f_sandbag_half");
    f_sandbag_wall=furnfind("f_sandbag_wall");
    f_bulletin=furnfind("f_bulletin");
    f_indoor_plant=furnfind("f_indoor_plant");
    f_indoor_plant_y=furnfind("f_indoor_plant_y");
    f_bed=furnfind("f_bed");
    f_toilet=furnfind("f_toilet");
    f_makeshift_bed=furnfind("f_makeshift_bed");
    f_straw_bed=furnfind("f_straw_bed");
    f_sink=furnfind("f_sink");
    f_oven=furnfind("f_oven");
    f_woodstove=furnfind("f_woodstove");
    f_fireplace=furnfind("f_fireplace");
    f_bathtub=furnfind("f_bathtub");
    f_chair=furnfind("f_chair");
    f_armchair=furnfind("f_armchair");
    f_sofa=furnfind("f_sofa");
    f_cupboard=furnfind("f_cupboard");
    f_trashcan=furnfind("f_trashcan");
    f_desk=furnfind("f_desk");
    f_exercise=furnfind("f_exercise");
    f_ball_mach=furnfind("f_ball_mach");
    f_bench=furnfind("f_bench");
    f_lane=furnfind("f_lane");
    f_table=furnfind("f_table");
    f_pool_table=furnfind("f_pool_table");
    f_counter=furnfind("f_counter");
    f_fridge=furnfind("f_fridge");
    f_glass_fridge=furnfind("f_glass_fridge");
    f_dresser=furnfind("f_dresser");
    f_locker=furnfind("f_locker");
    f_rack=furnfind("f_rack");
    f_bookcase=furnfind("f_bookcase");
    f_washer=furnfind("f_washer");
    f_dryer=furnfind("f_dryer");
    f_vending_c=furnfind("f_vending_c");
    f_vending_o=furnfind("f_vending_o");
    f_dumpster=furnfind("f_dumpster");
    f_dive_block=furnfind("f_dive_block");
    f_crate_c=furnfind("f_crate_c");
    f_crate_o=furnfind("f_crate_o");
    f_canvas_wall=furnfind("f_canvas_wall");
    f_large_canvas_wall=furnfind("f_large_canvas_wall");
    f_canvas_door=furnfind("f_canvas_door");
    f_large_canvas_door=furnfind("f_large_canvas_door");
    f_canvas_door_o=furnfind("f_canvas_door_o");
    f_large_canvas_door_o=furnfind("f_large_canvas_door_o");
    f_groundsheet=furnfind("f_groundsheet");
    f_large_groundsheet=furnfind("f_large_groundsheet");
    f_center_groundsheet=furnfind("f_center_groundsheet");
    f_fema_groundsheet=furnfind("f_fema_groundsheet");
    f_skin_wall=furnfind("f_skin_wall");
    f_skin_door=furnfind("f_skin_door");
    f_skin_door_o=furnfind("f_skin_door_o");
    f_skin_groundsheet=furnfind("f_skin_groundsheet");
    f_mutpoppy=furnfind("f_mutpoppy");
    f_fungal_mass=furnfind("f_fungal_mass");
    f_fungal_clump=furnfind("f_fungal_clump");
    f_flower_fungal=furnfind("f_flower_fungal");
    f_bluebell=furnfind("f_bluebell");
    f_dahlia=furnfind("f_dahlia");
    f_datura=furnfind("f_datura");
    f_dandelion=furnfind("f_dandelion");
    f_cattails=furnfind("f_cattails");
    f_safe_c=furnfind("f_safe_c");
    f_safe_l=furnfind("f_safe_l");
    f_safe_o=furnfind("f_safe_o");
    f_plant_seed=furnfind("f_plant_seed");
    f_plant_seedling=furnfind("f_plant_seedling");
    f_plant_mature=furnfind("f_plant_mature");
    f_plant_harvest=furnfind("f_plant_harvest");
    f_fvat_empty=furnfind("f_fvat_empty");
    f_fvat_full=furnfind("f_fvat_full");
    f_wood_keg=furnfind("f_wood_keg");
    f_standing_tank=furnfind("f_standing_tank");
    f_statue=furnfind("f_statue");
    f_egg_sackbw=furnfind("f_egg_sackbw");
    f_egg_sackcs=furnfind("f_egg_sackcs");
    f_egg_sackws=furnfind("f_egg_sackws");
    f_egg_sacke=furnfind("f_egg_sacke");
    f_flower_marloss=furnfind("f_flower_marloss");
    f_floor_canvas=furnfind("f_floor_canvas");
    f_kiln_empty=furnfind("f_kiln_empty");
    f_kiln_full=furnfind("f_kiln_full");
    f_kiln_metal_empty=furnfind("f_kiln_metal_empty");
    f_kiln_metal_full=furnfind("f_kiln_metal_full");
    f_robotic_arm=furnfind("f_robotic_arm");
}

size_t ter_t::count()
{
    return terrain_data.size();
}

void ter_t::load( JsonObject &jo )
{
    mandatory( jo, was_loaded, "name", name, translated_string_reader );
    mandatory( jo, was_loaded, "move_cost", movecost );
    optional( jo, was_loaded, "max_volume", max_volume, MAX_VOLUME_IN_SQUARE );
    optional( jo, was_loaded, "trap", trap_id_str );

    load_symbol( jo );

    trap = tr_null;
    transparent = false;
    connect_group = TERCONN_NONE;

    for( auto &flag : jo.get_string_array( "flags" ) ) {
        set_flag( flag );
    }
    // connect_group is initialised to none, then terrain flags are set, then finally
    // connections from JSON are set. This is so that wall flags can set wall connections
    // but can be overridden by explicit connections in JSON.
    if( jo.has_member( "connects_to" ) ) {
        set_connects( jo.get_string( "connects_to" ) );
    }

    if( jo.has_member( "examine_action" ) ) {
        examine = iexamine_function_from_string( jo.get_string( "examine_action" ) );
    } else {
        examine = iexamine_function_from_string( "none" );
    }

    optional( jo, was_loaded, "harvestable", harvestable );
    optional( jo, was_loaded, "open", open, NULL_ID );
    optional( jo, was_loaded, "close", close, NULL_ID );
    optional( jo, was_loaded, "transforms_into", transforms_into, NULL_ID );
    optional( jo, was_loaded, "roof", roof, NULL_ID );

    if( jo.has_member("harvest_season") ) {
        const std::string season = jo.get_string( "harvest_season" );

        if( season == "SPRING" ) {
            harvest_season = season_type::SPRING;
        } else if( season == "SUMMER" ) {
            harvest_season = season_type::SUMMER;
        } else if( season == "AUTUMN" ) {
            harvest_season = season_type::AUTUMN;
        } else if( season == "WINTER" ) {
            harvest_season = season_type::WINTER;
        } else {
            harvest_season = season_type::AUTUMN;
            debugmsg( "Invalid harvest season \"%s\" in \"%s\".", season.c_str(), id.c_str() );
        }
    }

    bash.load( jo, "bash", false );
    deconstruct.load( jo, "deconstruct", false );
}

void check_bash_items(const map_bash_info &mbi, const std::string &id, bool is_terrain)
{
    if( !item_group::group_is_defined( mbi.drop_group ) ) {
        debugmsg( "%s: bash result item group %s does not exist", id.c_str(), mbi.drop_group.c_str() );
    }
    if (mbi.str_max != -1) {
        if (is_terrain && mbi.ter_set.is_empty()) { // Some tiles specify t_null explicitly
            debugmsg("bash result terrain of %s is undefined/empty", id.c_str());
        }
        if ( !mbi.ter_set.is_valid() ) {
            debugmsg("bash result terrain %s of %s does not exist", mbi.ter_set.c_str(), id.c_str());
        }
        if (!mbi.furn_set.empty() && furnmap.count(mbi.furn_set) == 0) {
            debugmsg("bash result furniture %s of %s does not exist", mbi.furn_set.c_str(), id.c_str());
        }
    }
}

void check_decon_items(const map_deconstruct_info &mbi, const std::string &id, bool is_terrain)
{
    if (!mbi.can_do) {
        return;
    }
    if( !item_group::group_is_defined( mbi.drop_group ) ) {
        debugmsg( "%s: deconstruct result item group %s does not exist", id.c_str(), mbi.drop_group.c_str() );
    }
    if (is_terrain && mbi.ter_set.is_empty()) { // Some tiles specify t_null explicitly
        debugmsg("deconstruct result terrain of %s is undefined/empty", id.c_str());
    }
    if ( !mbi.ter_set.is_valid() ) {
        debugmsg("deconstruct result terrain %s of %s does not exist", mbi.ter_set.c_str(), id.c_str());
    }
    if (!mbi.furn_set.empty() && furnmap.count(mbi.furn_set) == 0) {
        debugmsg("deconstruct result furniture %s of %s does not exist", mbi.furn_set.c_str(), id.c_str());
    }
}

void ter_t::check() const
{
    check_bash_items( bash, id.str(), true );
    check_decon_items( deconstruct, id.str(), true );

    if( !transforms_into.is_valid() ) {
        debugmsg( "invalid transforms_into %s for %s", transforms_into.c_str(), id.c_str() );
    }
    if( !open.is_valid() ) {
        debugmsg( "invalid terrain %s for opening %s", open.c_str(), id.c_str() );
    }
    if( !close.is_valid() ) {
        debugmsg( "invalid terrain %s for closing %s", close.c_str(), id.c_str() );
    }
}

void check_furniture_and_terrain()
{
    for( const furn_t& f : furnlist ) {
        check_bash_items(f.bash, f.id, false);
        check_decon_items(f.deconstruct, f.id, false);
        if( !f.open.empty() && furnmap.count( f.open ) == 0 ) {
            debugmsg( "invalid furniture %s for opening %s", f.open.c_str(), f.id.c_str() );
        }
        if( !f.close.empty() && furnmap.count( f.close ) == 0 ) {
            debugmsg( "invalid furniture %s for closing %s", f.close.c_str(), f.id.c_str() );
        }
    }
    terrain_data.check();
}
