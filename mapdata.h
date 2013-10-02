#ifndef _MAPDATA_H_
#define _MAPDATA_H_

#include <vector>
#include <string>
#include "color.h"
#include "item.h"
#include "trap.h"
#include "monster.h"
#include "enums.h"
#include "computer.h"
#include "vehicle.h"
#include "graffiti.h"
#include "basecamp.h"
#include "iexamine.h"
#include "field.h"
#include "translations.h"
#include <iosfwd>

class game;
class monster;

//More importantly: SEEX defines the size of a nonant, or grid. Same with SEEY.
#ifndef SEEX    // SEEX is how far the player can see in the X direction (at
#define SEEX 12 // least, without scrolling).  All map segments will need to be
#endif          // at least this wide. The map therefore needs to be 3x as wide.

#ifndef SEEY    // Same as SEEX
#define SEEY 12 // Requires 2*SEEY+1= 25 vertical squares
#endif          // Nuts to 80x24 terms. Mostly exists in graphical clients, and
                // those fatcats can resize.

// mfb(t_flag) converts a flag to a bit for insertion into a bitfield
#ifndef mfb
#define mfb(n) static_cast <unsigned long> (1 << (n))
#endif

/*
 * List of known flags, used in both terrain.json and furniture.json.
 * TRANSPARENT - Players and monsters can see through/past it. Also sets ter_t.transparent
 * FLAT - Player can build and move furniture on
 * BASHABLE - Players + Monsters can bash this
 * CONTAINER - Items on this square are hidden until looted by the player
 * PLACE_ITEM - Valid terrain for place_item() to put items on
 * DOOR - Can be opened (used for NPC pathfinding)
 * FLAMMABLE - Can be lit on fire
 * FLAMMABLE_HARD - Harder to light on fire, but still possible
 * EXPLODES - Explodes when on fire
 * DIGGABLE - Digging monsters, seeding monsters, digging with shovel, etc
 * LIQUID - Blocks movement, but isn't a wall (lava, water, etc)
 * SWIMMABLE - Player and monsters can swim through it
 * SHARP - May do minor damage to players/monsters passing thruogh it
 * PAINFUL - May cause a small amount of pain
 * ROUGH - May hurt the player's feet
 * SEALED - Can't use 'e' to retrieve items, must smash open first
 * NOITEM - Items 'fall off' this space
 * DESTROY_ITEM - Items that land here are destroyed
 * GOES_DOWN - Can use '>' to go down a level
 * GOES_UP - Can use '<' to go up a level
 * CONSOLE - Used as a computer
 * ALARMED - Sets off an alarm if smashed
 * SUPPORTS_ROOF - Used as a boundary for roof construction
 * INDOORS - Has roof over it; blocks rain, sunlight, etc.
 * THIN_OBSTACLE - Passable by players and monsters, vehicles destroy it
 * COLLAPSES - Has a roof that can collapse
 * FLAMMABLE_ASH - Burns to ash rather than rubble.
 * DECONSTRUCT - Can be deconstructed
 * REDUCE_SCENT - Reduces scent even more, only works if also bashable
 * FIRE_CONTAINER - Stops fire from spreading (brazier, wood stove, etc)
 * SUPPRESS_SMOKE - Prevents smoke from fires, used by ventilated wood stoves etc
 * PLANT - A "furniture" that grows and fruits
 */

/*
Struct ter_t:
Short for terrain type. This struct defines all of the metadata for a given terrain id (an enum below).

*/
struct ter_t {
 std::string name; //The plaintext name of the terrain type the user would see (IE: dirt)

 /*
 The symbol drawn on the screen for the terrain. Please note that there are extensive rules as to which
 possible object/field/entity in a single square gets drawn and that some symbols are "reserved" such as * and % to do programmatic behavior.
 */
 long sym;

 nc_color color;//The color the sym will draw in on the GUI.
 unsigned char movecost; //The amount of movement points required to pass this terrain by default.
 trap_id trap; //The id of the trap located at this terrain. Limit one trap per tile currently.
 std::set<std::string> flags;// : num_t_flags; This refers to enum t_flag defined above.
 iexamine_function examine; //What happens when the terrain is examined

 bool has_flag(std::string flag) {
     return flags.count(flag) != 0;
 }

 void set_flag(std::string flag) {
     flags.insert(flag);
     if("TRANSPARENT" == flag) {
         transparent = true;
     }
 }
 bool transparent;
};

/*
enum: ter_id
Terrain id refers to a position in the terlist[] area describing, in the order of the enum, the terrain in question
through the use of a ter_t struct.
*/
/*
  On altering any entries in this enum please add or remove the appropriate entry to the terrain_names array in tile_id_data.h
*/
enum ter_id {
t_null = 0,
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
t_door_c, t_door_b, t_door_o, t_door_locked_interior, t_door_locked, t_door_locked_alarm, t_door_frame,
t_chaingate_l, t_fencegate_c, t_fencegate_o, t_chaingate_c, t_chaingate_o, t_door_boarded,
t_door_metal_c, t_door_metal_o, t_door_metal_locked,
t_door_bar_c, t_door_bar_o, t_door_bar_locked,
t_door_glass_c, t_door_glass_o,
t_portcullis,
t_recycler, t_window, t_window_taped, t_window_domestic, t_window_domestic_taped, t_window_open, t_curtains,
t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded,
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
t_marloss, t_fungus, t_tree_fungal,
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

num_terrain_types
};

/*
 * The terrain list contains the master list of  information and metadata for a given type of terrain.
 * Must match the order of the constants in the ter_list enum above!
 */
extern std::vector<ter_t> terlist;

struct furn_t {
 std::string name;
 long sym;
 nc_color color;
 int movecost; // Penalty to terrain
 int move_str_req; //The amount of strength required to move through this terrain easily.
 std::set<std::string> flags;
 iexamine_function examine;

 bool has_flag(std::string flag) {
     return flags.count(flag) != 0;
 }

 void set_flag(std::string flag) {
     flags.insert(flag);
     if("TRANSPARENT" == flag) {
         transparent = true;
     }
 }
 bool transparent;
};

/*
  On altering any entries in this enum please add or remove the appropriate entry to the furn_names array in tile_id_data.h
*/
enum furn_id {
f_null,
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
f_dumpster, f_dive_block,
f_crate_c, f_crate_o,
f_canvas_wall, f_canvas_door, f_canvas_door_o, f_groundsheet, f_fema_groundsheet,
f_skin_wall, f_skin_door, f_skin_door_o,  f_skin_groundsheet,
f_mutpoppy,
f_safe_c, f_safe_l, f_safe_o,
f_plant_seed, f_plant_seedling, f_plant_mature, f_plant_harvest,
num_furniture_types
};

//Must match enum furn_id order above!
extern std::vector<furn_t> furnlist;

/*
enum: map_extra
Map Extras are overmap specific flags that tell a submap "hey, put something extra here ontop of whats normally here".
*/
enum map_extra {
 mx_null = 0,
 mx_helicopter,
 mx_military,
 mx_science,
 mx_stash,
 mx_drugdeal,
 mx_supplydrop,
 mx_portal,
 mx_minefield,
 mx_wolfpack,
 mx_cougar,
 mx_puddle,
 mx_crater,
 mx_fumarole,
 mx_portal_in,
 mx_anomaly,
 num_map_extras
};

//Classic Extras is for when you have special zombies turned off.
const int classic_extras =  mfb(mx_helicopter) | mfb(mx_military) |
  mfb(mx_stash) | mfb(mx_drugdeal) | mfb(mx_supplydrop) | mfb(mx_minefield) |
  mfb(mx_wolfpack) | mfb(mx_cougar) | mfb(mx_puddle) | mfb(mx_crater);

// Chances are relative to eachother; e.g. a 200 chance is twice as likely
// as a 100 chance to appear.
const int map_extra_chance[num_map_extras + 1] = {
  0, // Null - 0 chance
 40, // Helicopter
 50, // Military
120, // Science
200, // Stash
 20, // Drug deal
 10, // Supply drop
  5, // Portal
 70, // Minefield
 30, // Wolf pack
 40, // Cougar
250, // Puddle
 10, // Crater
  8, // Fumarole
  7, // One-way portal into this world
 10, // Anomaly
  0  // Just a cap value; leave this as the last one
};

struct map_extras {
 unsigned int chance;
 int chances[num_map_extras + 1];
 map_extras(unsigned int embellished, int helicopter = 0, int mili = 0,
            int sci = 0, int stash = 0, int drug = 0, int supply = 0,
            int portal = 0, int minefield = 0, int wolves = 0, int cougar = 0, int puddle = 0,
            int crater = 0, int lava = 0, int marloss = 0, int anomaly = 0)
            : chance(embellished)
 {
  chances[ 0] = 0;
  chances[ 1] = helicopter;
  chances[ 2] = mili;
  chances[ 3] = sci;
  chances[ 4] = stash;
  chances[ 5] = drug;
  chances[ 6] = supply;
  chances[ 7] = portal;
  chances[ 8] = minefield;
  chances[ 9] = wolves;
  chances[10] = cougar;
  chances[11] = puddle;
  chances[12] = crater;
  chances[13] = lava;
  chances[14] = marloss;
  chances[15] = anomaly;
  chances[16] = 0;
 }
};

struct spawn_point {
 int posx, posy;
 int count;
 mon_id type;
 int faction_id;
 int mission_id;
 bool friendly;
 std::string name;
 spawn_point(mon_id T = mon_null, int C = 0, int X = -1, int Y = -1,
             int FAC = -1, int MIS = -1, bool F = false,
             std::string N = "NONE") :
             posx (X), posy (Y), count (C), type (T), faction_id (FAC),
             mission_id (MIS), friendly (F), name (N) {}
};

struct submap {
    ter_id             ter[SEEX][SEEY];  // Terrain on each square
    std::vector<item>  itm[SEEX][SEEY];  // Items on each square
    furn_id            frn[SEEX][SEEY];  // Furniture on each square
    trap_id            trp[SEEX][SEEY];  // Trap on each square
    field              fld[SEEX][SEEY];  // Field on each square
    int                rad[SEEX][SEEY];  // Irradiation of each square
    graffiti           graf[SEEX][SEEY]; // Graffiti on each square
    int active_item_count;
    int field_count;
    int turn_last_touched;
    int temperature;
    std::vector<spawn_point> spawns;
    std::vector<vehicle*> vehicles;
    computer comp;
    basecamp camp;  // only allowing one basecamp per submap
};

std::ostream & operator<<(std::ostream &, const submap *);
std::ostream & operator<<(std::ostream &, const submap &);

void load_furniture(JsonObject &jsobj);
void load_terrain(JsonObject &jsobj);

#endif
