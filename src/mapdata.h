#ifndef MAPDATA_H
#define MAPDATA_H

#include "color.h"
#include "item.h"
#include "trap.h"
#include "monster.h"
#include "enums.h"
#include "computer.h"
#include "vehicle.h"
#include "basecamp.h"
#include "iexamine.h"
#include "field.h"
#include "translations.h"
#include "item_stack.h"
#include <iosfwd>
#include <unordered_set>
#include <vector>
#include <list>
#include <string>

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
    int str_min;            // min str(*) required to bash
    int str_max;            // max str required: bash succeeds if str >= random # between str_min_roll & str_max_roll
    int str_min_blocked;    // same as above; alternate values for has_adjacent_furniture(...) == true
    int str_max_blocked;
    int str_min_roll;       // lower bound of success check; defaults to str_min
    int str_max_roll;       // upper bound of success check; defaults to str_max
    int explosive;          // Explosion on destruction
    int sound_vol;          // sound volume of breaking terrain/furniture
    int sound_fail_vol;     // sound volume on fail
    bool destroy_only;      // Only used for destroying, not normally bashable
    std::vector<map_bash_item_drop> items; // list of items: map_bash_item_drop
    std::string sound;      // sound made on success ('You hear a "smash!"')
    std::string sound_fail; // sound  made on fail
    std::string ter_set;    // terrain to set (REQUIRED for terrain))
    std::string furn_set;   // furniture to set (only used by furniture, not terrain)
    map_bash_info() : str_min(-1), str_max(-1), str_min_blocked(-1), str_max_blocked(-1),
                      str_min_roll(-1), str_max_roll(-1), explosive(0), sound_vol(-1), sound_fail_vol(-1),
                      destroy_only(false), sound(""), sound_fail(""), ter_set(""), furn_set("") {};
    bool load(JsonObject &jsobj, std::string member, bool is_furniture);
};
struct map_deconstruct_info {
    // Only if true, the terrain/furniture can be deconstructed
    bool can_do;
    // items you get when deconstructing.
    std::vector<map_bash_item_drop> items;
    std::string ter_set;    // terrain to set (REQUIRED for terrain))
    std::string furn_set;    // furniture to set (only used by furniture, not terrain)
    map_deconstruct_info() : can_do(false), items(), ter_set(), furn_set() { }
    bool load(JsonObject &jsobj, std::string member, bool is_furniture);
};

/*
 * List of known flags, used in both terrain.json and furniture.json.
 * TRANSPARENT - Players and monsters can see through/past it. Also sets ter_t.transparent
 * FLAT - Player can build and move furniture on
 * CONTAINER - Items on this square are hidden until looted by the player
 * PLACE_ITEM - Valid terrain for place_item() to put items on
 * DOOR - Can be opened (used for NPC pathfinding)
 * FLAMMABLE - Can be lit on fire
 * FLAMMABLE_HARD - Harder to light on fire, but still possible
 * DIGGABLE - Digging monsters, seeding monsters, digging with shovel, etc
 * LIQUID - Blocks movement, but isn't a wall (lava, water, etc)
 * SWIMMABLE - Player and monsters can swim through it
 * SHARP - May do minor damage to players/monsters passing thruogh it
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
 * COLLAPSES - Has a roof that can collapse
 * FLAMMABLE_ASH - Burns to ash rather than rubble.
 * REDUCE_SCENT - Reduces scent even more, only works if also bashable
 * FIRE_CONTAINER - Stops fire from spreading (brazier, wood stove, etc)
 * SUPPRESS_SMOKE - Prevents smoke from fires, used by ventilated wood stoves etc
 * PLANT - A "furniture" that grows and fruits
 * LIQUIDCONT - Furniture that contains liquid, allows for contents to be accessed in some checks even if SEALED
 * OPENCLOSE_INSIDE - If it's a door (with an 'open' or 'close' field), it can only be opened or closed if you're inside.
 * PERMEABLE - Allows gases to flow through unimpeded.
 *
 * Currently only used for Fungal conversions
 * WALL - This terrain is an upright obstacle
 * THIN_OBSTACLE - This terrain is a thin obstacle, i.e. fence
 * ORGANIC - This furniture is partly organic
 * FLOWER - This furniture is a flower
 * SHRUB - This terrain is a shrub
 * TREE - This terrain is a tree
 * HARVESTED - This terrain has been harvested so it won't bear any fruit
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
 * To add a new ter_bitflag, add below and add to init_ter_bitflags_map() in mapdata.cpp
 * Order does not matter.
 */
enum ter_bitflags {
    TFLAG_NONE,
    TFLAG_TRANSPARENT,
    TFLAG_FLAMMABLE,
    TFLAG_REDUCE_SCENT,
    TFLAG_SWIMMABLE,
    TFLAG_SUPPORTS_ROOF,
    TFLAG_NOITEM,
    TFLAG_SEALED,
    TFLAG_LIQUID,
    TFLAG_COLLAPSES,
    TFLAG_FLAMMABLE_ASH,
    TFLAG_DESTROY_ITEM,
    TFLAG_INDOORS,
    TFLAG_PLANT,
    TFLAG_LIQUIDCONT,
    TFLAG_FIRE_CONTAINER,
    TFLAG_FLAMMABLE_HARD,
    TFLAG_SUPPRESS_SMOKE,
    TFLAG_SHARP,
    TFLAG_DIGGABLE,
    TFLAG_ROUGH,
    TFLAG_UNSTABLE,
    TFLAG_WALL,
    TFLAG_DEEP_WATER,
    TFLAG_HARVESTED,
    TFLAG_PERMEABLE
};
extern std::map<std::string, ter_bitflags> ter_bitflags_map;
void init_ter_bitflags_map();

typedef int ter_id;
typedef int furn_id;

struct map_data_common_t {
    std::string id;    // The terrain's ID. Must be set, must be unique.
    std::string name;  // The plaintext name of the terrain type the user would see (i.e. dirt)
    std::string open;  // Open action: transform into terrain with matching id
    std::string close; // Close action: transform into terrain with matching id

    map_bash_info        bash;
    map_deconstruct_info deconstruct;

    std::set<std::string> flags;    // string flags which possibly refer to what's documented above.
    unsigned long         bitflags; // bitfield of -certian- string flags which are heavily checked

    /*
    * The symbol drawn on the screen for the terrain. Please note that there are extensive rules
    * as to which possible object/field/entity in a single square gets drawn and that some symbols
    * are "reserved" such as * and % to do programmatic behavior.
    */
    long sym;

    int loadid;     // This is akin to the old ter_id, however it is set at runtime.
    int movecost;   // The amount of movement points required to pass this terrain by default.
    int max_volume; // Maximal volume of items that can be stored in/on this furniture

    nc_color color; //The color the sym will draw in on the GUI.

    iexamine_function examine; //What happens when the terrain is examined

    bool transparent;

    bool has_flag(const std::string & flag) const {
        return !!flags.count(flag);
    }

    bool has_flag(const ter_bitflags flag) const {
        return (bitflags & mfb(flag));
    }

    void set_flag(std::string flag) {
        flags.insert( flag );

        if(!transparent && "TRANSPARENT" == flag) {
            transparent = true;
        }

        auto const it = ter_bitflags_map.find(flag);
        if (it != std::end(ter_bitflags_map)) {
            bitflags |= mfb(it->second);
        }
    }
};

/*
* Struct ter_t:
* Short for terrain type. This struct defines all of the metadata for a given terrain id (an enum below).
*/
struct ter_t : map_data_common_t {
    std::string trap_id_str;     // String storing the id string of the trap.
    std::string harvestable;     // What will be harvested from this terrain?
    std::string transforms_into; // Transform into what terrain?

    trap_id trap; // The id of the trap located at this terrain. Limit one trap per tile currently.

    int harvest_season; // When will this terrain get harvested?
    int bloom_season;   // When does this terrain bloom?
};

void set_ter_ids();
void set_furn_ids();

/*
 * The terrain list contains the master list of  information and metadata for a given type of terrain.
 */
extern std::vector<ter_t> terlist;
extern std::map<std::string, ter_t> termap;
ter_id terfind(const std::string & id); // lookup, carp and return null on error

struct furn_t : map_data_common_t {
    std::string crafting_pseudo_item;

    int move_str_req; //The amount of strength required to move through this terrain easily.

    // May return NULL
    itype *crafting_pseudo_item_type() const;
    // May return NULL
    itype *crafting_ammo_item_type() const;
};


extern std::vector<furn_t> furnlist;
extern std::map<std::string, furn_t> furnmap;
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
 mx_roadblock,
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
  mfb(mx_roadblock) | mfb(mx_drugdeal) | mfb(mx_supplydrop) | mfb(mx_minefield) |
  mfb(mx_crater);


struct map_extras {
 unsigned int chance;
 int chances[num_map_extras + 1];
 map_extras(unsigned int embellished, int helicopter = 0, int mili = 0,
            int sci = 0, int roadblock = 0, int drug = 0, int supply = 0,
            int portal = 0, int minefield = 0,
            int crater = 0, int lava = 0, int marloss = 0, int anomaly = 0)
            : chance(embellished)
 {
  chances[ 0] = 0;
  chances[ 1] = helicopter;
  chances[ 2] = mili;
  chances[ 3] = sci;
  chances[ 4] = roadblock;
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
    inline trap_id get_trap(int x, int y) const {
        return trp[x][y];
    }

    inline void set_trap(int x, int y, trap_id trap) {
        trp[x][y] = trap;
    }

    inline furn_id get_furn(int x, int y) const {
        return frn[x][y];
    }

    inline void set_furn(int x, int y, furn_id furn) {
        frn[x][y] = furn;
    }

    inline void set_ter(int x, int y, ter_id terr) {
        ter[x][y] = terr;
    }

    int get_radiation(int x, int y) {
        return rad[x][y];
    }

    void set_radiation(int x, int y, int radiation) {
        rad[x][y] = radiation;
    }

    void update_lum_add(item const &i, int const x, int const y) {
        if (i.is_emissive() && lum[x][y] < 255) {
            lum[x][y]++;
        }
    }

    void update_lum_rem(item const &i, int const x, int const y) {
        if (!i.is_emissive()) {
            return;
        } else if (lum[x][y] && lum[x][y] < 255) {
            lum[x][y]--;
            return;
        }

        // Have to scan through all items to be sure removing i will actally lower
        // the count below 255.
        int count = 0;
        for (auto const &it : itm[x][y]) {
            if (it.is_emissive()) {
                count++;
            }
        }

        if (count <= 256) {
            lum[x][y] = static_cast<uint8_t>(count - 1);
        }
    }

    bool has_graffiti( int x, int y ) const;
    const std::string &get_graffiti( int x, int y ) const;
    void set_graffiti( int x, int y, const std::string &new_graffiti );
    void delete_graffiti( int x, int y );

    // Signage is a pretend union between furniture on a square and stored
    // writing on the square. When both are present, we have signage.
    // Its effect is meant to be cosmetic and atmospheric only.
    inline bool has_signage(int x, int y) {
        furn_id f = frn[x][y];
        if (furnlist[f].id == "f_sign") {
            return cosmetics[x][y].find("SIGNAGE") != cosmetics[x][y].end();
        }
        return false;
    }
    // Dependent on furniture + cosmetics.
    inline const std::string get_signage(int x, int y) {
        if (has_signage(x, y)) {
            return cosmetics[x][y]["SIGNAGE"];
        }
        return "";
    }
    // Can be used anytime (prevents code from needing to place sign first.)
    inline void set_signage(int x, int y, std::string s) {
        cosmetics[x][y]["SIGNAGE"] = s;
    }
    // Can be used anytime (prevents code from needing to place sign first.)
    inline void delete_signage(int x, int y) {
        cosmetics[x][y].erase("SIGNAGE");
    }

    // TODO: make trp private once the horrible hack known as editmap is resolved
    ter_id          ter[SEEX][SEEY];  // Terrain on each square
    furn_id         frn[SEEX][SEEY];  // Furniture on each square
    std::uint8_t    lum[SEEX][SEEY];  // Number of items emitting light on each square
    std::list<item> itm[SEEX][SEEY];  // Items on each square
    field           fld[SEEX][SEEY];  // Field on each square
    trap_id         trp[SEEX][SEEY];  // Trap on each square
    int             rad[SEEX][SEEY];  // Irradiation of each square

    std::map<std::string, std::string> cosmetics[SEEX][SEEY]; // Textual "visuals" for each square.

    active_item_cache active_items;

    int field_count = 0;
    int turn_last_touched = 0;
    int temperature = 0;
    std::vector<spawn_point> spawns;
    /**
     * Vehicles on this submap (their (0,0) point is on this submap).
     * This vehicle objects are deleted by this submap when it gets
     * deleted.
     * TODO: submap owns these pointers, they ought to be unique_ptrs.
     */
    std::vector<vehicle*> vehicles;
    computer comp;
    basecamp camp;  // only allowing one basecamp per submap

    submap();
    ~submap();
    // delete vehicles and clear the vehicles vector
    void delete_vehicles();
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
   unsigned short ter;
   unsigned short furn;
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
    t_pit_corpsed, t_pit_covered, t_pit_spiked, t_pit_spiked_covered, t_pit_glass, t_pit_glass_covered,
    t_rock_floor,
    t_grass,
    t_metal_floor,
    t_pavement, t_pavement_y, t_sidewalk, t_concrete,
    t_floor, t_floor_waxed,
    t_dirtfloor,//Dirt floor(Has roof)
    t_carpet_red,t_carpet_yellow,t_carpet_purple,t_carpet_green,
    t_grate,
    t_slime,
    t_bridge,
    t_covered_well,
    // Lighting related
    t_utility_light,
    // Walls
    t_wall_log_half, t_wall_log, t_wall_log_chipped, t_wall_log_broken, t_palisade, t_palisade_gate, t_palisade_gate_o,
    t_wall_half, t_wall_wood, t_wall_wood_chipped, t_wall_wood_broken,
    t_wall_v, t_wall_h, t_concrete_v, t_concrete_h,
    t_wall_metal_v, t_wall_metal_h,
    t_wall_glass_v, t_wall_glass_h,
    t_wall_glass_v_alarm, t_wall_glass_h_alarm,
    t_reinforced_glass_v, t_reinforced_glass_h,
    t_bars,
    t_door_c, t_door_c_peep, t_door_b, t_door_b_peep, t_door_o, t_door_o_peep,
    t_door_locked_interior, t_door_locked, t_door_locked_peep, t_door_locked_alarm, t_door_frame,
    t_chaingate_l, t_fencegate_c, t_fencegate_o, t_chaingate_c, t_chaingate_o,
    t_door_boarded, t_door_boarded_damaged, t_door_boarded_peep, t_rdoor_boarded, t_rdoor_boarded_damaged, t_door_boarded_damaged_peep,
    t_door_metal_c, t_door_metal_o, t_door_metal_locked, t_door_metal_pickable,
    t_door_bar_c, t_door_bar_o, t_door_bar_locked,
    t_door_glass_c, t_door_glass_o,
    t_portcullis,
    t_recycler, t_window, t_window_taped, t_window_domestic, t_window_domestic_taped, t_window_open, t_curtains,
    t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded, t_window_boarded_noglass, t_window_bars_alarm,
    t_window_stained_green, t_window_stained_red, t_window_stained_blue,
    t_rock, t_fault,
    t_paper,
    t_rock_wall, t_rock_wall_half,
    // Tree
    t_tree, t_tree_young, t_tree_apple, t_tree_apple_harvested, t_tree_pear, t_tree_pear_harvested,
    t_tree_cherry, t_tree_cherry_harvested, t_tree_peach, t_tree_peach_harvested, t_tree_apricot, t_tree_apricot_harvested,
    t_tree_plum, t_tree_plum_harvested, t_tree_pine, t_tree_blackjack, t_tree_deadpine, t_underbrush, t_shrub, t_shrub_blueberry, t_shrub_strawberry, t_trunk,
    t_root_wall,
    t_wax, t_floor_wax,
    t_fence_v, t_fence_h, t_chainfence_v, t_chainfence_h, t_chainfence_posts,
    t_fence_post, t_fence_wire, t_fence_barbed, t_fence_rope,
    t_railing_v, t_railing_h,
    // Nether
    t_marloss, t_fungus_floor_in, t_fungus_floor_sup, t_fungus_floor_out, t_fungus_wall, t_fungus_wall_v,
    t_fungus_wall_h, t_fungus_mound, t_fungus, t_shrub_fungal, t_tree_fungal, t_tree_fungal_young, t_marloss_tree,
    // Water, lava, etc.
    t_water_sh, t_swater_sh, t_water_dp, t_swater_dp, t_water_pool, t_sewage,
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
    t_sewage_pipe, t_sewage_pump,
    t_centrifuge,
    t_column,
    t_vat,
    t_cvdbody, t_cvdmachine,
    t_water_pump, t_improvised_shelter,
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
    t_rdoor_c, t_rdoor_b, t_rdoor_o, t_mdoor_frame, t_window_reinforced, t_window_reinforced_noglass,
    t_window_enhanced, t_window_enhanced_noglass, t_open_air, t_plut_generator,
    t_pavement_bg_dp, t_pavement_y_bg_dp, t_sidewalk_bg_dp, t_guardrail_bg_dp,
    t_linoleum_white, t_linoleum_gray;


/*
runtime index: furn_id
furn_id refers to a position in the furnlist[] where the furn_t struct is stored. See note
about ter_id above.
*/
extern furn_id f_null,
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
    f_crate_c, f_crate_o,
    f_large_canvas_wall, f_canvas_wall, f_canvas_door, f_canvas_door_o, f_groundsheet, f_fema_groundsheet, f_large_groundsheet,
    f_large_canvas_door, f_large_canvas_door_o, f_center_groundsheet, f_skin_wall, f_skin_door, f_skin_door_o,  f_skin_groundsheet,
    f_mutpoppy, f_flower_fungal, f_fungal_mass, f_fungal_clump,
    f_safe_c, f_safe_l, f_safe_o,
    f_plant_seed, f_plant_seedling, f_plant_mature, f_plant_harvest,
    f_fvat_empty, f_fvat_full,
    f_wood_keg, f_egg_sackbw, f_egg_sackws, f_egg_sacke,
    f_flower_marloss,
    f_tatami,
    f_kiln_empty, f_kiln_full, f_kiln_metal_empty, f_kiln_metal_full;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// These are on their way OUT and only used in certain switch statements until they are rewritten.

// consistency checking of terlist & furnlist.
void check_furniture_and_terrain();

#endif
