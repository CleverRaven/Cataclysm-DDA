#include "mapdata.h"
#include "color.h"
#include "init.h"
#include <ostream>

std::vector<ter_t> terlist;
std::map<std::string, ter_t> termap;

std::vector<furn_t> furnlist;
std::map<std::string, furn_t> furnmap;

std::map<std::string, ter_bitflags> ter_bitflags_map;

std::ostream & operator<<(std::ostream & out, const submap * sm)
{
 out << "submap(";
 if( !sm )
 {
  out << "NULL)";
  return out;
 }

 out << "\n\tter:";
 for(int x = 0; x < SEEX; ++x)
 {
  out << "\n\t" << x << ": ";
  for(int y = 0; y < SEEY; ++y)
   out << sm->ter[x][y] << ", ";
 }

 out << "\n\titm:";
 for(int x = 0; x < SEEX; ++x)
 {
  for(int y = 0; y < SEEY; ++y)
  {
   if( !sm->itm[x][y].empty() )
   {
    for( std::vector<item>::const_iterator it = sm->itm[x][y].begin(),
      end = sm->itm[x][y].end(); it != end; ++it )
    {
     out << "\n\t("<<x<<","<<y<<") ";
     out << *it << ", ";
    }
   }
  }
 }

   out << "\n\t)";
 return out;
}

std::ostream & operator<<(std::ostream & out, const submap & sm)
{
 out << (&sm);
 return out;
}

/*
 * Initialize static mapping of heavily used string flags to bitflags for load_(terrain|furniture)
 */
void init_ter_bitflags_map() {
    ter_bitflags_map["DESTROY_ITEM"]            = TFLAG_DESTROY_ITEM;   // add/spawn_item*()
    ter_bitflags_map["ROUGH"]                   = TFLAG_ROUGH;          // monmove
    ter_bitflags_map["LIQUID"]                  = TFLAG_LIQUID;         // *move(), add/spawn_item*()
    ter_bitflags_map["FIRE_CONTAINER"]          = TFLAG_FIRE_CONTAINER; // fire
    ter_bitflags_map["DIGGABLE"]                = TFLAG_DIGGABLE;       // monmove
    ter_bitflags_map["SUPPRESS_SMOKE"]          = TFLAG_SUPPRESS_SMOKE; // fire
    ter_bitflags_map["FLAMMABLE_HARD"]          = TFLAG_FLAMMABLE_HARD; // fire
    ter_bitflags_map["COLLAPSES"]               = TFLAG_COLLAPSES;      // building "remodeling"
    ter_bitflags_map["FLAMMABLE"]               = TFLAG_FLAMMABLE;      // fire bad! fire SLOW!
    ter_bitflags_map["BASHABLE"]                = TFLAG_BASHABLE;       // half the game uses this
    ter_bitflags_map["REDUCE_SCENT"]            = TFLAG_REDUCE_SCENT;   // ...and the other half is update_scent
    ter_bitflags_map["SEALED"]                  = TFLAG_SEALED;         // item list
    ter_bitflags_map["INDOORS"]                 = TFLAG_INDOORS;        // vehicle gain_moves, weather
    ter_bitflags_map["SHARP"]                   = TFLAG_SHARP;          // monmove
    ter_bitflags_map["SUPPORTS_ROOF"]           = TFLAG_SUPPORTS_ROOF;  // and by building "remodeling" I mean hulkSMASH
    ter_bitflags_map["SWIMMABLE"]               = TFLAG_SWIMMABLE;      // monmove
    ter_bitflags_map["TRANSPARENT"]             = TFLAG_TRANSPARENT;    // map::trans / lightmap
    ter_bitflags_map["NOITEM"]                  = TFLAG_NOITEM;         // add/spawn_item*()
    ter_bitflags_map["FLAMMABLE_ASH"]           = TFLAG_FLAMMABLE_ASH;  // oh hey fire. again.
    ter_bitflags_map["PLANT"]                   = TFLAG_PLANT;          // full map iteration
    ter_bitflags_map["EXPLODES"]                = TFLAG_EXPLODES;       // guess who? smokey the bear -warned- you
    ter_bitflags_map["WALL"]                    = TFLAG_WALL;           // smells
    ter_bitflags_map["DEEP_WATER"]              = TFLAG_DEEP_WATER;     // Deep enough to submerge things
}


bool jsonint(JsonObject &jsobj, std::string key, int & var) {
    if ( jsobj.has_int(key) ) {
        var = jsobj.get_int(key);
        return true;
    }
    return false;
}

bool jsonstring(JsonObject &jsobj, std::string key, std::string & var) {
    if ( jsobj.has_string(key) ) {
        var = jsobj.get_string(key);
        return true;
    }
    return false;
}

bool map_bash_info::load(JsonObject &jsobj, std::string member, bool isfurniture) {
    if( jsobj.has_object(member) ) {
        JsonObject j = jsobj.get_object(member);

        if ( jsonint(j, "num_tests", num_tests ) == false ) {
           if ( jsonint(j, "str_min", str_min ) && jsonint(j, "str_max", str_max ) ) {
               num_tests = 1;
           }
        } else if ( num_tests > 0 ) {
           str_min = j.get_int("str_min");
           str_max = j.get_int("str_max");
        }

        jsonint(j, "str_min_blocked", str_min_blocked );
        jsonint(j, "str_max_blocked", str_max_blocked );
        jsonint(j, "str_min_roll", str_min_roll );
        jsonint(j, "chance", chance );
        jsonstring(j, "sound", sound );
        jsonstring(j, "sound_fail", sound_fail );
        jsonstring(j, "furn_set", furn_set );

        if ( jsonstring(j, "ter_set", ter_set ) == false && isfurniture == false ) {
           ter_set = "t_rubble";
           debugmsg("terrain[\"%s\"].bash.ter_set is not set!",jsobj.get_string("id").c_str() );
        }

        if ( j.has_array("items") ) {
           JsonArray ja = j.get_array("items");
           if (ja.size() > 0) {
               int c=0;
               while ( ja.has_more() ) {
                   if ( ja.has_object(c) ) {
                       JsonObject jio = ja.next_object();
                       if ( jio.has_string("item") && jio.has_int("amount") ) {
                           if ( jio.has_int("minamount") ) {
                               map_bash_item_drop drop( jio.get_string("item"), jio.get_int("amount"), jio.get_int("minamount") );
                               jsonint(jio, "chance", drop.chance);
                               items.push_back(drop);
                           } else {
                               map_bash_item_drop drop( jio.get_string("item"), jio.get_int("amount") );
                               jsonint(jio, "chance", drop.chance);
                               items.push_back(drop);
                           }
                       } else {
                           debugmsg("terrain[\"%s\"].bash.items[%d]: invalid entry",jsobj.get_string("id").c_str(),c);
                       }
                   } else {
                       debugmsg("terrain[\"%s\"].bash.items[%d]: invalid entry",jsobj.get_string("id").c_str(),c);
                   }
                   c++;
               }
           }
        }

//debugmsg("%d/%d %s %s/%s %d",str_min,str_max, ter_set.c_str(), sound.c_str(), sound_fail.c_str(), items.size() );
    return true;
  } else {
    return false;
  }
}

furn_t null_furniture_t() {
  furn_t new_furniture;
  new_furniture.id = "f_null";
  new_furniture.name = _("nothing");
  new_furniture.sym = ' ';
  new_furniture.color = c_white;
  new_furniture.movecost = 0;
  new_furniture.move_str_req = -1;
  new_furniture.transparent = true;
  new_furniture.bitflags = 0;
  new_furniture.set_flag("TRANSPARENT");
  new_furniture.examine = iexamine_function_from_string("none");
  new_furniture.loadid = 0;
  new_furniture.open = "";
  new_furniture.close = "";
  return new_furniture;
};

ter_t null_terrain_t() {
  ter_t new_terrain;
  new_terrain.id = "t_null";
  new_terrain.name = _("nothing");
  new_terrain.sym = ' ';
  new_terrain.color = c_white;
  new_terrain.movecost = 2;
  new_terrain.transparent = true;
  new_terrain.bitflags = 0;
  new_terrain.set_flag("TRANSPARENT");
  new_terrain.set_flag("DIGGABLE");
  new_terrain.examine = iexamine_function_from_string("none");
  new_terrain.loadid = 0;
  new_terrain.open = "";
  new_terrain.close = "";
  return new_terrain;
};

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
  new_furniture.sym = jsobj.get_string("symbol").c_str()[0];

  bool has_color = jsobj.has_member("color");
  bool has_bgcolor = jsobj.has_member("bgcolor");
  if(has_color && has_bgcolor) {
    debugmsg("Found both color and bgcolor for %s, use only one of these.", new_furniture.name.c_str());
    new_furniture.color = c_white;
  } else if(has_color) {
    new_furniture.color = color_from_string(jsobj.get_string("color"));
  } else if(has_bgcolor) {
    new_furniture.color = bgcolor_from_string(jsobj.get_string("bgcolor"));
  } else {
    debugmsg("Furniture %s needs at least one of: color, bgcolor.", new_furniture.name.c_str());
  }

  new_furniture.movecost = jsobj.get_int("move_cost_mod");
  new_furniture.move_str_req = jsobj.get_int("required_str");

  new_furniture.transparent = false;
  new_furniture.bitflags = 0;
  JsonArray flags = jsobj.get_array("flags");
  while(flags.has_more()) {
    new_furniture.set_flag(flags.next_string());
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

  new_furniture.loadid = furnlist.size();
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

  //Special case for the LINE_ symbols
  std::string symbol = jsobj.get_string("symbol");
  if("LINE_XOXO" == symbol) {
    new_terrain.sym = LINE_XOXO;
  } else if("LINE_OXOX" == symbol) {
    new_terrain.sym = LINE_OXOX;
  } else {
    new_terrain.sym = symbol.c_str()[0];
  }

  new_terrain.color = color_from_string(jsobj.get_string("color"));
  new_terrain.movecost = jsobj.get_int("move_cost");

  if(jsobj.has_member("trap")) {
      // Store the string representation of the trap id.
      // Overwrites the trap field in set_trap_ids() once ids are assigned..
      new_terrain.trap_id_str = jsobj.get_string("trap");
  }
  new_terrain.trap = tr_null;

  new_terrain.transparent = false;
  new_terrain.bitflags = 0;
  JsonArray flags = jsobj.get_array("flags");
  while(flags.has_more()) {
    new_terrain.set_flag(flags.next_string());
  }

  if(jsobj.has_member("examine_action")) {
    std::string function_name = jsobj.get_string("examine_action");
    new_terrain.examine = iexamine_function_from_string(function_name);
  } else {
    //If not specified, default to no action
    new_terrain.examine = iexamine_function_from_string("none");
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
  new_terrain.loadid=terlist.size();
  termap[new_terrain.id]=new_terrain;
  terlist.push_back(new_terrain);
}


ter_id terfind(const std::string & id) {
    if( termap.find(id) == termap.end() ) {
         debugmsg("Can't find %s",id.c_str());
         return 0;
    }
    return termap[id].loadid;
};

ter_id t_null,
    t_hole, // Real nothingness; makes you fall a z-level
    // Ground
    t_dirt, t_sand, t_dirtmound, t_pit_shallow, t_pit,
    t_pit_corpsed, t_pit_covered, t_pit_spiked, t_pit_spiked_covered,
    t_rock_floor, t_rubble, t_ash, t_metal, t_wreckage,
    t_grass,
    t_metal_floor,
    t_pavement, t_pavement_y, t_sidewalk, t_concrete,
    t_floor,
    t_dirtfloor,//Dirt floor(Has roof)
    t_grate,
    t_slime,
    t_bridge,
    // Lighting related
    t_skylight, t_emergency_light_flicker, t_emergency_light,
    // Walls
    t_wall_log_half, t_wall_log, t_wall_log_chipped, t_wall_log_broken, t_palisade, t_palisade_gate, t_palisade_gate_o,
    t_wall_half, t_wall_wood, t_wall_wood_chipped, t_wall_wood_broken,
    t_wall_v, t_wall_h, t_concrete_v, t_concrete_h,
    t_wall_metal_v, t_wall_metal_h,
    t_wall_glass_v, t_wall_glass_h,
    t_wall_glass_v_alarm, t_wall_glass_h_alarm,
    t_reinforced_glass_v, t_reinforced_glass_h,
    t_bars,
    t_door_c, t_door_b, t_door_o, t_rdoor_c, t_rdoor_b, t_rdoor_o,t_door_locked_interior, t_door_locked, t_door_locked_alarm, t_door_frame,
    t_chaingate_l, t_fencegate_c, t_fencegate_o, t_chaingate_c, t_chaingate_o, t_door_boarded,
    t_door_metal_c, t_door_metal_o, t_door_metal_locked,
    t_door_bar_c, t_door_bar_o, t_door_bar_locked,
    t_door_glass_c, t_door_glass_o,
    t_portcullis,
    t_recycler, t_window, t_window_taped, t_window_domestic, t_window_domestic_taped, t_window_open, t_curtains,
    t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded,
    t_window_boarded_noglass, t_window_reinforced, t_window_reinforced_noglass, t_window_enhanced, t_window_enhanced_noglass,
    t_window_stained_green, t_window_stained_red, t_window_stained_blue,
    t_rock, t_fault,
    t_paper,
    // Tree
    t_tree, t_tree_young, t_tree_apple, t_underbrush, t_shrub, t_shrub_blueberry, t_shrub_strawberry, t_trunk,
    t_root_wall,
    t_wax, t_floor_wax,
    t_fence_v, t_fence_h, t_chainfence_v, t_chainfence_h, t_chainfence_posts,
    t_fence_post, t_fence_wire, t_fence_barbed, t_fence_rope,
    t_railing_v, t_railing_h,
    // Nether
    t_marloss, t_fungus_floor_in, t_fungus_floor_sup, t_fungus_floor_out, t_fungus_wall, t_fungus_wall_v,
    t_fungus_wall_h, t_fungus_mound, t_fungus, t_shrub_fungal, t_tree_fungal, t_tree_fungal_young,
    // Water, lava, etc.
    t_water_sh, t_water_dp, t_water_pool, t_sewage,
    t_lava,
    // More embellishments than you can shake a stick at.
    t_sandbox, t_slide, t_monkey_bars, t_backboard,
    t_gas_pump, t_gas_pump_smashed,
    t_generator_broken,
    t_missile, t_missile_exploded,
    t_radio_tower, t_radio_controls,
    t_console_broken, t_console, t_gates_mech_control, t_gates_control_concrete, t_barndoor, t_palisade_pulley,
    t_sewage_pipe, t_sewage_pump,
    t_centrifuge,
    t_column,
    t_vat,
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
     t_switch_rg, t_switch_gb, t_switch_rb, t_switch_even,
    num_terrain_types;

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
    t_rock_floor=terfind("t_rock_floor");
    t_rubble=terfind("t_rubble");
    t_ash=terfind("t_ash");
    t_metal=terfind("t_metal");
    t_wreckage=terfind("t_wreckage");
    t_grass=terfind("t_grass");
    t_metal_floor=terfind("t_metal_floor");
    t_pavement=terfind("t_pavement");
    t_pavement_y=terfind("t_pavement_y");
    t_sidewalk=terfind("t_sidewalk");
    t_concrete=terfind("t_concrete");
    t_floor=terfind("t_floor");
    t_dirtfloor=terfind("t_dirtfloor");
    t_grate=terfind("t_grate");
    t_slime=terfind("t_slime");
    t_bridge=terfind("t_bridge");
    t_skylight=terfind("t_skylight");
    t_emergency_light_flicker=terfind("t_emergency_light_flicker");
    t_emergency_light=terfind("t_emergency_light");
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
    t_wall_v=terfind("t_wall_v");
    t_wall_h=terfind("t_wall_h");
    t_concrete_v=terfind("t_concrete_v");
    t_concrete_h=terfind("t_concrete_h");
    t_wall_metal_v=terfind("t_wall_metal_v");
    t_wall_metal_h=terfind("t_wall_metal_h");
    t_wall_glass_v=terfind("t_wall_glass_v");
    t_wall_glass_h=terfind("t_wall_glass_h");
    t_wall_glass_v_alarm=terfind("t_wall_glass_v_alarm");
    t_wall_glass_h_alarm=terfind("t_wall_glass_h_alarm");
    t_reinforced_glass_v=terfind("t_reinforced_glass_v");
    t_reinforced_glass_h=terfind("t_reinforced_glass_h");
    t_bars=terfind("t_bars");
    t_door_c=terfind("t_door_c");
    t_door_b=terfind("t_door_b");
    t_door_o=terfind("t_door_o");
    t_rdoor_c=terfind("t_rdoor_c");
    t_rdoor_b=terfind("t_rdoor_b");
    t_rdoor_o=terfind("t_rdoor_o");
    t_door_locked_interior=terfind("t_door_locked_interior");
    t_door_locked=terfind("t_door_locked");
    t_door_locked_alarm=terfind("t_door_locked_alarm");
    t_door_frame=terfind("t_door_frame");
    t_door_frame=terfind("t_mdoor_frame");
    t_chaingate_l=terfind("t_chaingate_l");
    t_fencegate_c=terfind("t_fencegate_c");
    t_fencegate_o=terfind("t_fencegate_o");
    t_chaingate_c=terfind("t_chaingate_c");
    t_chaingate_o=terfind("t_chaingate_o");
    t_door_boarded=terfind("t_door_boarded");
    t_door_metal_c=terfind("t_door_metal_c");
    t_door_metal_o=terfind("t_door_metal_o");
    t_door_metal_locked=terfind("t_door_metal_locked");
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
    t_window_stained_green=terfind("t_window_stained_green");
    t_window_stained_red=terfind("t_window_stained_red");
    t_window_stained_blue=terfind("t_window_stained_blue");
    t_rock=terfind("t_rock");
    t_fault=terfind("t_fault");
    t_paper=terfind("t_paper");
    t_tree=terfind("t_tree");
    t_tree_young=terfind("t_tree_young");
    t_tree_apple=terfind("t_tree_apple");
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
    t_fungus_wall_v=terfind("t_fungus_wall_v");
    t_fungus_wall_h=terfind("t_fungus_wall_h");
    t_fungus_mound=terfind("t_fungus_mound");
    t_fungus=terfind("t_fungus");
    t_shrub_fungal=terfind("t_shrub_fungal");
    t_tree_fungal=terfind("t_tree_fungal");
    t_tree_fungal_young=terfind("t_tree_fungal_young");
    t_water_sh=terfind("t_water_sh");
    t_water_dp=terfind("t_water_dp");
    t_water_pool=terfind("t_water_pool");
    t_sewage=terfind("t_sewage");
    t_lava=terfind("t_lava");
    t_sandbox=terfind("t_sandbox");
    t_slide=terfind("t_slide");
    t_monkey_bars=terfind("t_monkey_bars");
    t_backboard=terfind("t_backboard");
    t_gas_pump=terfind("t_gas_pump");
    t_gas_pump_smashed=terfind("t_gas_pump_smashed");
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
    t_sewage_pipe=terfind("t_sewage_pipe");
    t_sewage_pump=terfind("t_sewage_pump");
    t_centrifuge=terfind("t_centrifuge");
    t_column=terfind("t_column");
    t_vat=terfind("t_vat");
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
    num_terrain_types = terlist.size();
};

furn_id furnfind(const std::string & id) {
    if( furnmap.find(id) == furnmap.end() ) {
         debugmsg("Can't find %s",id.c_str());
         return 0;
    }
    return furnmap[id].loadid;
};

furn_id f_null,
    f_hay,
    f_bulletin,
    f_indoor_plant,
    f_bed, f_toilet, f_makeshift_bed,
    f_sink, f_oven, f_woodstove, f_fireplace, f_bathtub,
    f_chair, f_armchair, f_sofa, f_cupboard, f_trashcan, f_desk, f_exercise,
    f_bench, f_table, f_pool_table,
    f_counter,
    f_fridge, f_glass_fridge, f_dresser, f_locker,
    f_rack, f_bookcase,
    f_washer, f_dryer,
    f_vending_c, f_vending_o, f_dumpster, f_dive_block,
    f_crate_c, f_crate_o,
    f_canvas_wall, f_canvas_door, f_canvas_door_o, f_groundsheet, f_fema_groundsheet,
    f_skin_wall, f_skin_door, f_skin_door_o,  f_skin_groundsheet,
    f_mutpoppy, f_flower_fungal, f_fungal_mass, f_fungal_clump,
    f_safe_c, f_safe_l, f_safe_o,
    f_plant_seed, f_plant_seedling, f_plant_mature, f_plant_harvest,
    num_furniture_types;

void set_furn_ids() {
    f_null=furnfind("f_null");
    f_hay=furnfind("f_hay");
    f_bulletin=furnfind("f_bulletin");
    f_indoor_plant=furnfind("f_indoor_plant");
    f_bed=furnfind("f_bed");
    f_toilet=furnfind("f_toilet");
    f_makeshift_bed=furnfind("f_makeshift_bed");
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
    f_bench=furnfind("f_bench");
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
    f_canvas_door=furnfind("f_canvas_door");
    f_canvas_door_o=furnfind("f_canvas_door_o");
    f_groundsheet=furnfind("f_groundsheet");
    f_fema_groundsheet=furnfind("f_fema_groundsheet");
    f_skin_wall=furnfind("f_skin_wall");
    f_skin_door=furnfind("f_skin_door");
    f_skin_door_o=furnfind("f_skin_door_o");
    f_skin_groundsheet=furnfind("f_skin_groundsheet");
    f_mutpoppy=furnfind("f_mutpoppy");
    f_fungal_mass=furnfind("f_fungal_mass");
    f_fungal_clump=furnfind("f_fungal_clump");
    f_flower_fungal=furnfind("f_flower_fungal");
    f_safe_c=furnfind("f_safe_c");
    f_safe_l=furnfind("f_safe_l");
    f_safe_o=furnfind("f_safe_o");
    f_plant_seed=furnfind("f_plant_seed");
    f_plant_seedling=furnfind("f_plant_seedling");
    f_plant_mature=furnfind("f_plant_mature");
    f_plant_harvest=furnfind("f_plant_harvest");
    num_furniture_types = furnlist.size();
}

/*
 * default? N O T H I N G.
 *
ter_furn_id::ter_furn_id() {
    ter = (short)t_null;
    furn = (short)t_null;
}
*/
