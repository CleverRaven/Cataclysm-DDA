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

struct map_bash_item_drop {
    std::string itemtype; // item id
    int amount;           // number dropped
    int minamount;        // optional: if >= amount drop is random # between minamount and amount
    int chance;           // 
    map_bash_item_drop(std::string str, int i) : itemtype(str), amount(i), minamount(-1), chance(-1) {};
    map_bash_item_drop(std::string str, int i1, int i2) : itemtype(str), amount(i1), minamount(i2), chance(-1) {};
};
struct map_bash_info {
    int str_min;          // min str(*) required to bash
    int str_max;          // max str required: bash succeeds if str >= random # between str_min_roll & str_max
    int str_min_roll;     // lower bound of success check; defaults to str_min ( may set default to 0 )
    int str_min_blocked;  // same as above; alternate values for has_adjacent_furniture(...) == true
    int str_max_blocked;  
    int num_tests;        // how many tests must succeed
    int chance;
    int explosive;        // Explosion on destruction
    std::vector<map_bash_item_drop> items; // list of items: map_bash_item_drop
    std::string sound;    // sound made on success ('You hear a "smash!"')
    std::string sound_fail; // sound  made on fail
    std::string ter_set;    // terrain to set (REQUIRED for terrain))
    std::string furn_set;    // furniture to set (only used by furniture, not terrain)
    map_bash_info() : str_min(-1), str_max(-1), str_min_roll(-1), str_min_blocked(-1), str_max_blocked(-1), num_tests(-1), chance(-1), explosive(0), ter_set(""), furn_set("") {};
    bool load(JsonObject &jsobj, std::string member, bool is_furniture);
};

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
 * MOUNTABLE - Player can fire mounted weapons from here (EG: M2 Browning)
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
 * OPENCLOSE_INSIDE - If it's a door (with an 'open' or 'close' field), it can only be opened or closed if you're inside.
 *
 * Currently only used for Fungal conversions
 * WALL - This terrain is an upright obstacle
 * ORGANIC - This furniture is partly organic
 * FLOWER - This furniture is a flower
 * SHRUB - This terrain is a shrub
 * TREE - This terrain is a tree
 * YOUNG - This terrain is a young tree
 * FUNGUS - Fungal covered
 *
 * Furniture only:
 * BLOCKSDOOR - This will boost map terrain's resistance to bashing if str_*_blocked is set (see map_bash_info)
 */

/*
 * Note; All flags are defined as strings dynamically in data/json/terrain.json and furniture.json. The list above
 * represent the common builtins. The enum below is an alternative means of fast-access, for those flags that are checked
 * so much that strings produce a significant performance penalty. The following are equivalent:
 *  m->has_flag("FLAMMABLE");     //
 *  m->has_flag(TFLAG_FLAMMABLE); // ~ 20 x faster than the above, ( 2.5 x faster if the above uses static const std::string str_flammable("FLAMMABLE");
 * To add a new ter_bitflag, add below and add to init_ter_bitflag_map() in mapdata.cpp
 * Order does not matter.
 */
enum ter_bitflags {
    TFLAG_NONE,
    TFLAG_TRANSPARENT,
    TFLAG_FLAMMABLE,
    TFLAG_BASHABLE,
    TFLAG_REDUCE_SCENT,
    TFLAG_SWIMMABLE,
    TFLAG_SUPPORTS_ROOF,
    TFLAG_NOITEM,
    TFLAG_SEALED,
    TFLAG_LIQUID,
    TFLAG_COLLAPSES,
    TFLAG_EXPLODES,
    TFLAG_FLAMMABLE_ASH,
    TFLAG_DESTROY_ITEM,
    TFLAG_INDOORS,
    TFLAG_PLANT,
    TFLAG_FIRE_CONTAINER,
    TFLAG_FLAMMABLE_HARD,
    TFLAG_SUPPRESS_SMOKE,
    TFLAG_SHARP,
    TFLAG_DIGGABLE,
    TFLAG_ROUGH,
    TFLAG_WALL,
    TFLAG_DEEP_WATER
};
extern std::map<std::string, ter_bitflags> ter_bitflags_map;
void init_ter_bitflags_map();

typedef int ter_id;
typedef int furn_id;
/*
Struct ter_t:
Short for terrain type. This struct defines all of the metadata for a given terrain id (an enum below).

*/
struct ter_t {
 std::string id;   // The terrain's ID. Must be set, must be unique.
 int loadid;       // This is akin to the old ter_id, however it is set at runtime.
 std::string name; // The plaintext name of the terrain type the user would see (IE: dirt)

 /*
 The symbol drawn on the screen for the terrain. Please note that there are extensive rules as to which
 possible object/field/entity in a single square gets drawn and that some symbols are "reserved" such as * and % to do programmatic behavior.
 */
 long sym;

 nc_color color;//The color the sym will draw in on the GUI.
 unsigned char movecost; //The amount of movement points required to pass this terrain by default.
 trap_id trap; //The id of the trap located at this terrain. Limit one trap per tile currently.
 std::string trap_id_str; // String storing the id string of the trap.
 std::set<std::string> flags;// string flags which may or may not refer to what's documented above.
 unsigned long bitflags; // bitfield of -certian- string flags which are heavily checked
 iexamine_function examine; //What happens when the terrain is examined
 std::string open;          // open action: transform into terrain with matching id
 std::string close;         // close action: transform into terrain with matching id

 map_bash_info bash;
 
 bool has_flag(const std::string & flag) const {
     return flags.count(flag) != 0;
 }

 bool has_flag(const ter_bitflags flag) const {
     return (bitflags & mfb(flag));
 }

 void set_flag(std::string flag) {
     flags.insert(flag);
     if("TRANSPARENT" == flag) {
         transparent = true;
     }
     if ( ter_bitflags_map.find( flag ) != ter_bitflags_map.end() ) {
         bitflags |= mfb ( ter_bitflags_map.find( flag )->second ); 
     }
 }
 bool transparent;
};

void set_ter_ids();
void set_furn_ids();

/*
 * The terrain list contains the master list of  information and metadata for a given type of terrain.
 */
extern std::vector<ter_t> terlist;
extern std::map<std::string, ter_t> termap;
extern std::map<int,int> reverse_legacy_ter_id;
ter_id terfind(const std::string & id); // lookup, carp and return null on error


struct furn_t {
 std::string id;
 int loadid;
 std::string name;
 long sym;
 nc_color color;
 int movecost; // Penalty to terrain
 int move_str_req; //The amount of strength required to move through this terrain easily.
 std::set<std::string> flags;// string flags which may or may not refer to what's documented above.
 unsigned long bitflags; // bitfield of -certian- string flags which are heavily checked
 iexamine_function examine;
 std::string open;
 std::string close;

 map_bash_info bash;
 
 bool has_flag(const std::string & flag) const {
     return flags.count(flag) != 0;
 }

 bool has_flag(const ter_bitflags flag) const {
     return (bitflags & mfb(flag));
 }

 void set_flag(std::string flag) {
     flags.insert(flag);
     if("TRANSPARENT" == flag) {
         transparent = true;
     }
     if ( ter_bitflags_map.find( flag ) != ter_bitflags_map.end() ) {
         bitflags |= mfb ( ter_bitflags_map.find( flag )->second ); 
     }
 }
 bool transparent;
};


extern std::vector<furn_t> furnlist;
extern std::map<std::string, furn_t> furnmap;
extern std::map<int,int> reverse_legacy_furn_id;
furn_id furnfind(const std::string & id); // lookup, carp and return null on error

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
 mx_crater,
 mx_fumarole,
 mx_portal_in,
 mx_anomaly,
 num_map_extras
};

//Classic Extras is for when you have special zombies turned off.
const int classic_extras =  mfb(mx_helicopter) | mfb(mx_military) |
  mfb(mx_stash) | mfb(mx_drugdeal) | mfb(mx_supplydrop) | mfb(mx_minefield) |
  mfb(mx_crater);

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
            int portal = 0, int minefield = 0,
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
  chances[ 9] = crater;
  chances[10] = lava;
  chances[11] = marloss;
  chances[12] = anomaly;
  chances[13] = 0;
 }
};

struct spawn_point {
 int posx, posy;
 int count;
 std::string type;
 int faction_id;
 int mission_id;
 bool friendly;
 std::string name;
 spawn_point(std::string T = "mon_null", int C = 0, int X = -1, int Y = -1,
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

    submap() : active_item_count(0), field_count(0)
    {

    }
};

std::ostream & operator<<(std::ostream &, const submap *);
std::ostream & operator<<(std::ostream &, const submap &);

void load_furniture(JsonObject &jsobj);
void load_terrain(JsonObject &jsobj);

void verify_furniture();
void verify_terrain();


/*
 * Temporary container id_or_id. Stores str for delayed lookup and conversion.
 */
struct sid_or_sid {
   std::string primary_str;   // 32
   std::string secondary_str; // 64
   int chance;                // 68
   sid_or_sid(const std::string & s1, const int i, const::std::string s2) : primary_str(s1), secondary_str(s2), chance(i) { }
};

/*
 * Container for custom 'grass_or_dirt' functionality. Returns int but can store str values for delayed lookup and conversion
 */
struct id_or_id {
   int chance;                  // 8
   short primary;               // 12
   short secondary;             // 16
   id_or_id(const int id1, const int i, const int id2) : chance(i), primary(id1), secondary(id2) { }
   bool match( const int iid ) const {
       if ( iid == primary || iid == secondary ) {
           return true;
       }
       return false;
   }
   int get() const {
       return ( one_in(chance) ? secondary : primary );
   }
};

/*
 * It's a terrain! No, it's a furniture! Wait it's both!
 */
struct ter_furn_id {
   short ter;
   short furn;
   ter_furn_id() : ter(0), furn(0) {};
};

/*
runtime index: ter_id
ter_id refers to a position in the terlist[] where the ter_t struct is stored. These global
ints are a drop-in replacement to the old enum, however they are -not- required (save for areas in
the code that can use the perormance boost and refer to core terrain types), and they are -not-
provided for terrains added by mods. A string equivalent is always present, ie;
t_basalt
"t_basalt"
*/
extern ter_id t_null,
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
    t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded, t_window_boarded_noglass,
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
    t_atm,
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


/*
runtime index: furn_id
furn_id refers to a position in the furnlist[] where the furn_t struct is stored. See note
about ter_id above.
*/
extern furn_id f_null,
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

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// These are on their way OUT and only used in certain switch statements until they are rewritten.

enum old_ter_id {
old_t_null = 0,
old_t_hole, // Real nothingness; makes you fall a z-level
// Ground
old_t_dirt, old_t_sand, old_t_dirtmound, old_t_pit_shallow, old_t_pit,
old_t_pit_corpsed, old_t_pit_covered, old_t_pit_spiked, old_t_pit_spiked_covered,
old_t_rock_floor, old_t_rubble, old_t_ash, old_t_metal, old_t_wreckage,
old_t_grass,
old_t_metal_floor,
old_t_pavement, old_t_pavement_y, old_t_sidewalk, old_t_concrete,
old_t_floor,
old_t_dirtfloor,//Dirt floor(Has roof)
old_t_grate,
old_t_slime,
old_t_bridge,
// Lighting related
old_t_skylight, old_t_emergency_light_flicker, old_t_emergency_light,
// Walls
old_t_wall_log_half, old_t_wall_log, old_t_wall_log_chipped, old_t_wall_log_broken, old_t_palisade, old_t_palisade_gate, old_t_palisade_gate_o,
old_t_wall_half, old_t_wall_wood, old_t_wall_wood_chipped, old_t_wall_wood_broken,
old_t_wall_v, old_t_wall_h, old_t_concrete_v, old_t_concrete_h,
old_t_wall_metal_v, old_t_wall_metal_h,
old_t_wall_glass_v, old_t_wall_glass_h,
old_t_wall_glass_v_alarm, old_t_wall_glass_h_alarm,
old_t_reinforced_glass_v, old_t_reinforced_glass_h,
old_t_bars,
old_t_door_c, old_t_door_b, old_t_door_o, old_t_door_locked_interior, old_t_door_locked, old_t_door_locked_alarm, old_t_door_frame,
old_t_chaingate_l, old_t_fencegate_c, old_t_fencegate_o, old_t_chaingate_c, old_t_chaingate_o, old_t_door_boarded,
old_t_door_metal_c, old_t_door_metal_o, old_t_door_metal_locked,
old_t_door_bar_c, old_t_door_bar_o, old_t_door_bar_locked,
old_t_door_glass_c, old_t_door_glass_o,
old_t_portcullis,
old_t_recycler, old_t_window, old_t_window_taped, old_t_window_domestic, old_t_window_domestic_taped, old_t_window_open, old_t_curtains,
old_t_window_alarm, old_t_window_alarm_taped, old_t_window_empty, old_t_window_frame, old_t_window_boarded,
old_t_window_stained_green, old_t_window_stained_red, old_t_window_stained_blue,
old_t_rock, old_t_fault,
old_t_paper,
// Tree
old_t_tree, old_t_tree_young, old_t_tree_apple, old_t_underbrush, old_t_shrub, old_t_shrub_blueberry, old_t_shrub_strawberry, old_t_trunk,
old_t_root_wall,
old_t_wax, old_t_floor_wax,
old_t_fence_v, old_t_fence_h, old_t_chainfence_v, old_t_chainfence_h, old_t_chainfence_posts,
old_t_fence_post, old_t_fence_wire, old_t_fence_barbed, old_t_fence_rope,
old_t_railing_v, old_t_railing_h,
// Nether
old_t_marloss, old_t_fungus, old_t_tree_fungal,
// Water, lava, etc.
old_t_water_sh, old_t_water_dp, old_t_water_pool, old_t_sewage,
old_t_lava,
// More embellishments than you can shake a stick at.
old_t_sandbox, old_t_slide, old_t_monkey_bars, old_t_backboard,
old_t_gas_pump, old_t_gas_pump_smashed,
old_t_generator_broken,
old_t_missile, old_t_missile_exploded,
old_t_radio_tower, old_t_radio_controls,
old_t_console_broken, old_t_console, old_t_gates_mech_control, old_t_gates_control_concrete, old_t_barndoor, old_t_palisade_pulley,
old_t_sewage_pipe, old_t_sewage_pump,
old_t_centrifuge,
old_t_column,
old_t_vat,
// Staircases etc.
old_t_stairs_down, old_t_stairs_up, old_t_manhole, old_t_ladder_up, old_t_ladder_down, old_t_slope_down,
 old_t_slope_up, old_t_rope_up,
old_t_manhole_cover,
// Special
old_t_card_science, old_t_card_military, old_t_card_reader_broken, old_t_slot_machine,
 old_t_elevator_control, old_t_elevator_control_off, old_t_elevator, old_t_pedestal_wyrm,
 old_t_pedestal_temple,
// Temple tiles
old_t_rock_red, old_t_rock_green, old_t_rock_blue, old_t_floor_red, old_t_floor_green, old_t_floor_blue,
 old_t_switch_rg, old_t_switch_gb, old_t_switch_rb, old_t_switch_even,
old_num_terrain_types,
};

enum old_furn_id {
old_f_null,
old_f_hay,
old_f_bulletin,
old_f_indoor_plant,
old_f_bed, old_f_toilet, old_f_makeshift_bed,
old_f_sink, old_f_oven, old_f_woodstove, old_f_fireplace, old_f_bathtub,
old_f_chair, old_f_armchair, old_f_sofa, old_f_cupboard, old_f_trashcan, old_f_desk, old_f_exercise,
old_f_bench, old_f_table, old_f_pool_table,
old_f_counter,
old_f_fridge, old_f_glass_fridge, old_f_dresser, old_f_locker,
old_f_rack, old_f_bookcase,
old_f_washer, old_f_dryer,
old_f_dumpster, old_f_dive_block,
old_f_crate_c, old_f_crate_o,
old_f_canvas_wall, old_f_canvas_door, old_f_canvas_door_o, old_f_groundsheet, old_f_fema_groundsheet,
old_f_skin_wall, old_f_skin_door, old_f_skin_door_o, old_f_skin_groundsheet,
old_f_mutpoppy,
old_f_safe_c, old_f_safe_l, old_f_safe_o,
old_f_plant_seed, old_f_plant_seedling, old_f_plant_mature, old_f_plant_harvest,
old_num_furniture_types
};


#endif
