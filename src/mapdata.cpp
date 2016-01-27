#include "mapdata.h"
#include "color.h"
#include "init.h"
#include "game_constants.h"
#include "debug.h"
#include "translations.h"
#include "trap.h"
#include "output.h"
#include "item.h"
#include "item_group.h"
#include "calendar.h"

#include <unordered_map>

const std::set<std::string> classic_extras = { "mx_helicopter", "mx_military",
"mx_roadblock", "mx_drugdeal", "mx_supplydrop", "mx_minefield",
"mx_crater", "mx_collegekids"
};

std::vector<ter_t> terlist;
std::map<std::string, ter_t> termap;

std::vector<furn_t> furnlist;
std::map<std::string, furn_t> furnmap;

template<>
const ter_t &int_id<ter_t>::obj() const
{
    if( static_cast<size_t>( _id ) >= terlist.size() ) {
        debugmsg( "invalid terrain id %d", _id );
        static const ter_t dummy{};
        return dummy;
    }
    return terlist[_id];
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
    { "TFLAG_SEALED",             TFLAG_SEALED },         // Fire, acid
    { "TFLAG_ALLOW_FIELD_EFFECT", TFLAG_ALLOW_FIELD_EFFECT }, // Fire, acid
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
        ter_set = j.get_string( "ter_set" );
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
        ter_set = j.get_string( "ter_set" );
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
  new_terrain.id = "t_null";
  new_terrain.name = _("nothing");
  new_terrain.symbol_.fill( ' ' );
  new_terrain.color_.fill( c_white );
  new_terrain.movecost = 2;
  new_terrain.trap = tr_null;
  new_terrain.trap_id_str = "";
  new_terrain.transparent = true;
  new_terrain.set_flag("TRANSPARENT");
  new_terrain.set_flag("DIGGABLE");
  new_terrain.examine = iexamine_function_from_string("none");
  new_terrain.harvest_season = 0;
  new_terrain.harvestable = "";
  new_terrain.transforms_into = "t_null";
  new_terrain.roof = "t_null";
  new_terrain.loadid = ter_id( 0 );
  new_terrain.open = "";
  new_terrain.close = "";
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
  if ( terlist.empty() ) {
      ter_t new_null = null_terrain_t();
      termap[new_null.id] = new_null;
      terlist.push_back(new_null);
  }
  ter_t new_terrain;
  new_terrain.id = jsobj.get_string("id");
  if ( new_terrain.id == "t_null" ) {
      return;
  }
  new_terrain.name = _(jsobj.get_string("name").c_str());

    new_terrain.load_symbol( jsobj );

  new_terrain.movecost = jsobj.get_int("move_cost");

  if(jsobj.has_member("trap")) {
      // Store the string representation of the trap id.
      // Overwrites the trap field in set_trap_ids() once ids are assigned..
      new_terrain.trap_id_str = jsobj.get_string("trap");
  }
  new_terrain.trap = tr_null;
  new_terrain.max_volume = jsobj.get_int("max_volume", MAX_VOLUME_IN_SQUARE);

  new_terrain.transparent = false;
    new_terrain.connect_group = TERCONN_NONE;

    for( auto & flag : jsobj.get_string_array( "flags" ) ) {
        new_terrain.set_flag( flag );
    }

    // connect_group is initialised to none, then terrain flags are set, then finally
    // connections from JSON are set. This is so that wall flags can set wall connections
    // but can be overridden by explicit connections in JSON.
    if(jsobj.has_member("connects_to")) {
    new_terrain.set_connects( jsobj.get_string("connects_to") );
    }

  if(jsobj.has_member("examine_action")) {
    std::string function_name = jsobj.get_string("examine_action");
    new_terrain.examine = iexamine_function_from_string(function_name);
  } else {
    // if not specified, default to no action
    new_terrain.examine = iexamine_function_from_string("none");
  }

  // if the terrain has something harvestable
  if (jsobj.has_member("harvestable")) {
    new_terrain.harvestable = jsobj.get_string("harvestable"); // get the harvestable
  }

  if (jsobj.has_member("transforms_into")) {
    new_terrain.transforms_into = jsobj.get_string("transforms_into"); // get the terrain to transform into later on
  }

  if (jsobj.has_member("roof")) {
    new_terrain.roof = jsobj.get_string("roof"); // Get the terrain to create above this one if there would be open air otherwise
  }

  if (jsobj.has_member("harvest_season")) {
    //get the harvest season
    if (jsobj.get_string("harvest_season") == "SPRING") {new_terrain.harvest_season = 0;} // convert the season to int for calendar compare
    else if (jsobj.get_string("harvest_season") == "SUMMER") {new_terrain.harvest_season = 1;}
    else if (jsobj.get_string("harvest_season") == "AUTUMN") {new_terrain.harvest_season = 2;}
    else {new_terrain.harvest_season = 3;}
  }

  new_terrain.open = "";
  if ( jsobj.has_member("open") ) {
      new_terrain.open = jsobj.get_string("open");
  }
  new_terrain.close = "";
  if ( jsobj.has_member("close") ) {
      new_terrain.close = jsobj.get_string("close");
  }
  new_terrain.bash.load(jsobj, "bash", false);
  new_terrain.deconstruct.load(jsobj, "deconstruct", false);
  new_terrain.loadid = ter_id( terlist.size() );
  termap[new_terrain.id]=new_terrain;
  terlist.push_back(new_terrain);
}

static const std::unordered_map<std::string, std::string> ter_type_conversion_map = { {
    { "t_wall_h", "t_wall" },
    { "t_wall_v", "t_wall" },
    { "t_concrete_h", "t_concrete_wall" },
    { "t_concrete_v", "t_concrete_wall" },
    { "t_wall_metal_h", "t_wall_metal" },
    { "t_wall_metal_v", "t_wall_metal" },
    { "t_wall_glass_h", "t_wall_glass" },
    { "t_wall_glass_v", "t_wall_glass" },
    { "t_wall_glass_h_alarm", "t_wall_glass_alarm" },
    { "t_wall_glass_v_alarm", "t_wall_glass_alarm" },
    { "t_reinforced_glass_h", "t_reinforced_glass" },
    { "t_reinforced_glass_v", "t_reinforced_glass" },
    { "t_fungus_wall_h", "t_fungus_wall" },
    { "t_fungus_wall_v", "t_fungus_wall" },
    { "t_wall_h_r", "t_wall_r" },
    { "t_wall_v_r", "t_wall_r" },
    { "t_wall_h_w", "t_wall_w" },
    { "t_wall_v_w", "t_wall_w" },
    { "t_wall_h_b", "t_wall_b" },
    { "t_wall_v_b", "t_wall_b" },
    { "t_wall_h_g", "t_wall_g" },
    { "t_wall_v_g", "t_wall_g" },
    { "t_wall_h_y", "t_wall_y" },
    { "t_wall_v_y", "t_wall_y" },
    { "t_wall_h_p", "t_wall_p" },
    { "t_wall_v_p", "t_wall_p" },
} };

std::string convert_terrain_type( const std::string &t )
{
    const auto iter = ter_type_conversion_map.find( t );
    if( iter == ter_type_conversion_map.end() ) {
        return t;
    }
    return iter->second;
}

ter_id terfind(const std::string & id) {
    auto iter = termap.find( id );
    if( iter != termap.end() ) {
        return iter->second.loadid;
    }
    const std::string new_id = convert_terrain_type( id );
    if( new_id != id ) {
        return terfind( new_id );
    }
    debugmsg( "can't find terrain %s", id.c_str() );
    return t_null;
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
    t_wall, t_concrete_wall,
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
    t_tree_pine, t_tree_blackjack, t_tree_birch, t_tree_willow, t_tree_maple, t_tree_hickory, t_tree_hickory_dead, t_tree_hickory_harvested, t_tree_deadpine, t_underbrush, t_shrub, t_shrub_blueberry, t_shrub_strawberry, t_trunk,
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
    t_console_broken, t_console, t_gates_mech_control, t_gates_control_concrete, t_barndoor, t_palisade_pulley,
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

void set_ter_ids() {
    t_null=terfind("t_null");
    t_hole=terfind("t_hole");
    t_dirt=terfind("t_dirt");
    t_sand=terfind("t_sand");
    t_dirtmound=terfind("t_dirtmound");
    t_pit_shallow=terfind("t_pit_shallow");
    t_pit=terfind("t_pit");
    t_pit_corpsed=terfind("t_pit_corpsed");
    t_pit_covered=terfind("t_pit_covered");
    t_pit_spiked=terfind("t_pit_spiked");
    t_pit_spiked_covered=terfind("t_pit_spiked_covered");
    t_pit_glass=terfind("t_pit_glass");
    t_pit_glass_covered=terfind("t_pit_glass_covered");
    t_rock_floor=terfind("t_rock_floor");
    t_grass=terfind("t_grass");
    t_metal_floor=terfind("t_metal_floor");
    t_pavement=terfind("t_pavement");
    t_pavement_y=terfind("t_pavement_y");
    t_sidewalk=terfind("t_sidewalk");
    t_concrete=terfind("t_concrete");
    t_floor=terfind("t_floor");
    t_floor_waxed=terfind("t_floor_waxed");
    t_dirtfloor=terfind("t_dirtfloor");
    t_carpet_red=terfind("t_carpet_red");
    t_carpet_yellow=terfind("t_carpet_yellow");
    t_carpet_purple=terfind("t_carpet_purple");
    t_carpet_green=terfind("t_carpet_green");
    t_linoleum_white=terfind("t_linoleum_white");
    t_linoleum_gray=terfind("t_linoleum_gray");
    t_grate=terfind("t_grate");
    t_slime=terfind("t_slime");
    t_bridge=terfind("t_bridge");
    t_utility_light=terfind("t_utility_light");
    t_wall_log_half=terfind("t_wall_log_half");
    t_wall_log=terfind("t_wall_log");
    t_wall_log_chipped=terfind("t_wall_log_chipped");
    t_wall_log_broken=terfind("t_wall_log_broken");
    t_palisade=terfind("t_palisade");
    t_palisade_gate=terfind("t_palisade_gate");
    t_palisade_gate_o=terfind("t_palisade_gate_o");
    t_wall_half=terfind("t_wall_half");
    t_wall_wood=terfind("t_wall_wood");
    t_wall_wood_chipped=terfind("t_wall_wood_chipped");
    t_wall_wood_broken=terfind("t_wall_wood_broken");
    t_wall=terfind("t_wall");
    t_concrete_wall=terfind("t_concrete_wall");
    t_wall_metal=terfind("t_wall_metal");
    t_wall_glass=terfind("t_wall_glass");
    t_wall_glass_alarm=terfind("t_wall_glass_alarm");
    t_reinforced_glass=terfind("t_reinforced_glass");
    t_bars=terfind("t_bars");
    t_wall_b=terfind("t_wall_b");
    t_wall_g=terfind("t_wall_g");
    t_wall_p=terfind("t_wall_p");
    t_wall_r=terfind("t_wall_r");
    t_wall_w=terfind("t_wall_w");
    t_door_c=terfind("t_door_c");
    t_door_c_peep=terfind("t_door_c_peep");
    t_door_b=terfind("t_door_b");
    t_door_b_peep=terfind("t_door_b_peep");
    t_door_o=terfind("t_door_o");
    t_door_o_peep=terfind("t_door_o_peep");
    t_rdoor_c=terfind("t_rdoor_c");
    t_rdoor_b=terfind("t_rdoor_b");
    t_rdoor_o=terfind("t_rdoor_o");
    t_door_locked_interior=terfind("t_door_locked_interior");
    t_door_locked=terfind("t_door_locked");
    t_door_locked_peep=terfind("t_door_locked_peep");
    t_door_locked_alarm=terfind("t_door_locked_alarm");
    t_door_frame=terfind("t_door_frame");
    t_mdoor_frame=terfind("t_mdoor_frame");
    t_chaingate_l=terfind("t_chaingate_l");
    t_fencegate_c=terfind("t_fencegate_c");
    t_fencegate_o=terfind("t_fencegate_o");
    t_chaingate_c=terfind("t_chaingate_c");
    t_chaingate_o=terfind("t_chaingate_o");
    t_door_boarded=terfind("t_door_boarded");
    t_door_boarded_damaged=terfind("t_door_boarded_damaged");
    t_door_boarded_peep=terfind("t_door_boarded_peep");
    t_rdoor_boarded=terfind("t_rdoor_boarded");
    t_rdoor_boarded_damaged=terfind("t_rdoor_boarded_damaged");
    t_door_boarded_damaged_peep=terfind("t_door_boarded_damaged_peep");
    t_door_metal_c=terfind("t_door_metal_c");
    t_door_metal_o=terfind("t_door_metal_o");
    t_door_metal_locked=terfind("t_door_metal_locked");
    t_door_metal_pickable=terfind("t_door_metal_pickable");
    t_door_bar_c=terfind("t_door_bar_c");
    t_door_bar_o=terfind("t_door_bar_o");
    t_door_bar_locked=terfind("t_door_bar_locked");
    t_door_glass_c=terfind("t_door_glass_c");
    t_door_glass_o=terfind("t_door_glass_o");
    t_portcullis=terfind("t_portcullis");
    t_recycler=terfind("t_recycler");
    t_window=terfind("t_window");
    t_window_taped=terfind("t_window_taped");
    t_window_domestic=terfind("t_window_domestic");
    t_window_domestic_taped=terfind("t_window_domestic_taped");
    t_window_open=terfind("t_window_open");
    t_curtains=terfind("t_curtains");
    t_window_alarm=terfind("t_window_alarm");
    t_window_alarm_taped=terfind("t_window_alarm_taped");
    t_window_empty=terfind("t_window_empty");
    t_window_frame=terfind("t_window_frame");
    t_window_boarded=terfind("t_window_boarded");
    t_window_boarded_noglass=terfind("t_window_boarded_noglass");
    t_window_reinforced=terfind("t_window_reinforced");
    t_window_reinforced_noglass=terfind("t_window_reinforced_noglass");
    t_window_enhanced=terfind("t_window_enhanced");
    t_window_enhanced_noglass=terfind("t_window_enhanced_noglass");
    t_window_bars_alarm=terfind("t_window_bars_alarm");
    t_window_bars=terfind("t_window_bars");
    t_window_stained_green=terfind("t_window_stained_green");
    t_window_stained_red=terfind("t_window_stained_red");
    t_window_stained_blue=terfind("t_window_stained_blue");
    t_window_no_curtains=terfind("t_window_no_curtains");
    t_window_no_curtains_open=terfind("t_window_no_curtains_open");
    t_window_no_curtains_taped=terfind("t_window_no_curtains_taped");
    t_rock=terfind("t_rock");
    t_fault=terfind("t_fault");
    t_paper=terfind("t_paper");
    t_rock_wall=terfind("t_rock_wall");
    t_rock_wall_half=terfind("t_rock_wall_half");
    t_tree=terfind("t_tree");
    t_tree_young=terfind("t_tree_young");
    t_tree_apple=terfind("t_tree_apple");
    t_tree_apple_harvested=terfind("t_tree_apple_harvested");
    t_tree_pear=terfind("t_tree_pear");
    t_tree_pear_harvested=terfind("t_tree_pear_harvested");
    t_tree_cherry=terfind("t_tree_cherry");
    t_tree_cherry_harvested=terfind("t_tree_cherry_harvested");
    t_tree_peach=terfind("t_tree_peach");
    t_tree_peach_harvested=terfind("t_tree_peach_harvested");
    t_tree_apricot=terfind("t_tree_apricot");
    t_tree_apricot_harvested=terfind("t_tree_apricot_harvested");
    t_tree_plum=terfind("t_tree_plum");
    t_tree_plum_harvested=terfind("t_tree_plum_harvested");
    t_tree_pine=terfind("t_tree_pine");
    t_tree_blackjack=terfind("t_tree_blackjack");
    t_tree_birch=terfind("t_tree_birch");
    t_tree_willow=terfind("t_tree_willow");
    t_tree_maple=terfind("t_tree_maple");
    t_tree_deadpine=terfind("t_tree_deadpine");
    t_tree_hickory=terfind("t_tree_hickory");
    t_tree_hickory_dead=terfind("t_tree_hickory_dead");
    t_tree_hickory_harvested=terfind("t_tree_hickory_harvested");
    t_underbrush=terfind("t_underbrush");
    t_shrub=terfind("t_shrub");
    t_shrub_blueberry=terfind("t_shrub_blueberry");
    t_shrub_strawberry=terfind("t_shrub_strawberry");
    t_trunk=terfind("t_trunk");
    t_root_wall=terfind("t_root_wall");
    t_wax=terfind("t_wax");
    t_floor_wax=terfind("t_floor_wax");
    t_fence_v=terfind("t_fence_v");
    t_fence_h=terfind("t_fence_h");
    t_chainfence_v=terfind("t_chainfence_v");
    t_chainfence_h=terfind("t_chainfence_h");
    t_chainfence_posts=terfind("t_chainfence_posts");
    t_fence_post=terfind("t_fence_post");
    t_fence_wire=terfind("t_fence_wire");
    t_fence_barbed=terfind("t_fence_barbed");
    t_fence_rope=terfind("t_fence_rope");
    t_railing_v=terfind("t_railing_v");
    t_railing_h=terfind("t_railing_h");
    t_marloss=terfind("t_marloss");
    t_fungus_floor_in=terfind("t_fungus_floor_in");
    t_fungus_floor_sup=terfind("t_fungus_floor_sup");
    t_fungus_floor_out=terfind("t_fungus_floor_out");
    t_fungus_wall=terfind("t_fungus_wall");
    t_fungus_mound=terfind("t_fungus_mound");
    t_fungus=terfind("t_fungus");
    t_shrub_fungal=terfind("t_shrub_fungal");
    t_tree_fungal=terfind("t_tree_fungal");
    t_tree_fungal_young=terfind("t_tree_fungal_young");
    t_marloss_tree=terfind("t_marloss_tree");
    t_water_sh=terfind("t_water_sh");
    t_water_dp=terfind("t_water_dp");
    t_swater_sh=terfind("t_swater_sh");
    t_swater_dp=terfind("t_swater_dp");
    t_water_pool=terfind("t_water_pool");
    t_sewage=terfind("t_sewage");
    t_lava=terfind("t_lava");
    t_sandbox=terfind("t_sandbox");
    t_slide=terfind("t_slide");
    t_monkey_bars=terfind("t_monkey_bars");
    t_backboard=terfind("t_backboard");
    t_gas_pump=terfind("t_gas_pump");
    t_gas_pump_smashed=terfind("t_gas_pump_smashed");
    t_diesel_pump=terfind("t_diesel_pump");
    t_diesel_pump_smashed=terfind("t_diesel_pump_smashed");
    t_atm=terfind("t_atm");
    t_generator_broken=terfind("t_generator_broken");
    t_missile=terfind("t_missile");
    t_missile_exploded=terfind("t_missile_exploded");
    t_radio_tower=terfind("t_radio_tower");
    t_radio_controls=terfind("t_radio_controls");
    t_console_broken=terfind("t_console_broken");
    t_console=terfind("t_console");
    t_gates_mech_control=terfind("t_gates_mech_control");
    t_gates_control_concrete=terfind("t_gates_control_concrete");
    t_barndoor=terfind("t_barndoor");
    t_palisade_pulley=terfind("t_palisade_pulley");
    t_gates_control_metal=terfind("t_gates_control_metal");
    t_sewage_pipe=terfind("t_sewage_pipe");
    t_sewage_pump=terfind("t_sewage_pump");
    t_centrifuge=terfind("t_centrifuge");
    t_column=terfind("t_column");
    t_vat=terfind("t_vat");
    t_cvdbody=terfind("t_cvdbody");
    t_cvdmachine=terfind("t_cvdmachine");
    t_stairs_down=terfind("t_stairs_down");
    t_stairs_up=terfind("t_stairs_up");
    t_manhole=terfind("t_manhole");
    t_ladder_up=terfind("t_ladder_up");
    t_ladder_down=terfind("t_ladder_down");
    t_slope_down=terfind("t_slope_down");
    t_slope_up=terfind("t_slope_up");
    t_rope_up=terfind("t_rope_up");
    t_manhole_cover=terfind("t_manhole_cover");
    t_card_science=terfind("t_card_science");
    t_card_military=terfind("t_card_military");
    t_card_reader_broken=terfind("t_card_reader_broken");
    t_slot_machine=terfind("t_slot_machine");
    t_elevator_control=terfind("t_elevator_control");
    t_elevator_control_off=terfind("t_elevator_control_off");
    t_elevator=terfind("t_elevator");
    t_pedestal_wyrm=terfind("t_pedestal_wyrm");
    t_pedestal_temple=terfind("t_pedestal_temple");
    t_rock_red=terfind("t_rock_red");
    t_rock_green=terfind("t_rock_green");
    t_rock_blue=terfind("t_rock_blue");
    t_floor_red=terfind("t_floor_red");
    t_floor_green=terfind("t_floor_green");
    t_floor_blue=terfind("t_floor_blue");
    t_switch_rg=terfind("t_switch_rg");
    t_switch_gb=terfind("t_switch_gb");
    t_switch_rb=terfind("t_switch_rb");
    t_switch_even=terfind("t_switch_even");
    t_covered_well=terfind("t_covered_well");
    t_water_pump=terfind("t_water_pump");
    t_conveyor=terfind("t_conveyor");
    t_machinery_light=terfind("t_machinery_light");
    t_machinery_heavy=terfind("t_machinery_heavy");
    t_machinery_old=terfind("t_machinery_old");
    t_machinery_electronic=terfind("t_machinery_electronic");
    t_open_air=terfind("t_open_air");
    t_plut_generator = terfind("t_plut_generator");
    t_pavement_bg_dp = terfind("t_pavement_bg_dp");
    t_pavement_y_bg_dp = terfind("t_pavement_y_bg_dp");
    t_sidewalk_bg_dp = terfind("t_sidewalk_bg_dp");
    t_guardrail_bg_dp = terfind("t_guardrail_bg_dp");
    t_improvised_shelter = terfind("t_improvised_shelter");

    for( auto &elem : terlist ) {
        if( elem.trap_id_str.empty() ) {
            elem.trap = tr_null;
        } else {
            elem.trap = trap_str_id( elem.trap_id_str );
        }
    }
}

void reset_furn_ter()
{
    termap.clear();
    terlist.clear();
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
    f_statue, f_egg_sackbw, f_egg_sackws, f_egg_sacke,
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

/*
 * default? N O T H I N G.
 *
ter_furn_id::ter_furn_id() {
    ter = (short)t_null;
    furn = (short)t_null;
}
*/

void check_bash_items(const map_bash_info &mbi, const std::string &id, bool is_terrain)
{
    if( !item_group::group_is_defined( mbi.drop_group ) ) {
        debugmsg( "%s: bash result item group %s does not exist", id.c_str(), mbi.drop_group.c_str() );
    }
    if (mbi.str_max != -1) {
        if (is_terrain && mbi.ter_set.empty()) {
            debugmsg("bash result terrain of %s is undefined/empty", id.c_str());
        }
        if (!mbi.ter_set.empty() && termap.count(mbi.ter_set) == 0) {
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
    if (is_terrain && mbi.ter_set.empty()) {
        debugmsg("deconstruct result terrain of %s is undefined/empty", id.c_str());
    }
    if (!mbi.ter_set.empty() && termap.count(mbi.ter_set) == 0) {
        debugmsg("deconstruct result terrain %s of %s does not exist", mbi.ter_set.c_str(), id.c_str());
    }
    if (!mbi.furn_set.empty() && furnmap.count(mbi.furn_set) == 0) {
        debugmsg("deconstruct result furniture %s of %s does not exist", mbi.furn_set.c_str(), id.c_str());
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
    for( const ter_t& t : terlist ) {
        check_bash_items(t.bash, t.id, true);
        check_decon_items(t.deconstruct, t.id, true);
        if( !t.transforms_into.empty() && termap.count( t.transforms_into ) == 0 ) {
            debugmsg( "invalid transforms_into %s for %s", t.transforms_into.c_str(), t.id.c_str() );
        }
        if( !t.open.empty() && termap.count( t.open ) == 0 ) {
            debugmsg( "invalid terrain %s for opening %s", t.open.c_str(), t.id.c_str() );
        }
        if( !t.close.empty() && termap.count( t.close ) == 0 ) {
            debugmsg( "invalid terrain %s for closing %s", t.close.c_str(), t.id.c_str() );
        }
    }
}
