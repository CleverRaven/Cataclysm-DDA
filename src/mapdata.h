#pragma once
#ifndef CATA_SRC_MAPDATA_H
#define CATA_SRC_MAPDATA_H

#include <algorithm>
#include <array>
#include <bitset>
#include <cstddef>
#include <set>
#include <string>
#include <vector>

#include "calendar.h"
#include "color.h"
#include "int_id.h"
#include "string_id.h"
#include "translations.h"
#include "type_id.h"
#include "units.h"
#include "units_fwd.h"
#include "value_ptr.h"

struct ter_t;
using ter_str_id = string_id<ter_t>;

class JsonObject;
class player;
struct furn_t;
struct itype;
struct ter_t;
struct tripoint;

using iexamine_function = void ( * )( player &, const tripoint & );

struct map_bash_info {
    int str_min;            // min str(*) required to bash
    int str_max;            // max str required: bash succeeds if str >= random # between str_min & str_max
    int str_min_blocked;    // same as above; alternate values for has_adjacent_furniture(...) == true
    int str_max_blocked;
    int str_min_supported;  // Alternative values for floor supported by something from below
    int str_max_supported;
    int explosive;          // Explosion on destruction
    int sound_vol;          // sound volume of breaking terrain/furniture
    int sound_fail_vol;     // sound volume on fail
    int collapse_radius;    // Radius of the tent supported by this tile
    int fd_bash_move_cost = 100; // cost to bash a field
    bool destroy_only;      // Only used for destroying, not normally bashable
    bool bash_below;        // This terrain is the roof of the tile below it, try to destroy that too
    std::string drop_group; // item group of items that are dropped when the object is bashed
    translation sound;      // sound made on success ('You hear a "smash!"')
    translation sound_fail; // sound  made on fail
    translation field_bash_msg_success; // message upon successfully bashing a field
    ter_str_id ter_set;    // terrain to set (REQUIRED for terrain))
    ter_str_id ter_set_bashed_from_above; // terrain to set if bashed from above (defaults to ter_set)
    furn_str_id furn_set;   // furniture to set (only used by furniture, not terrain)
    // ids used for the special handling of tents
    std::vector<furn_str_id> tent_centers;
    map_bash_info();
    enum map_object_type {
        furniture = 0,
        terrain,
        field
    };
    bool load( const JsonObject &jsobj, const std::string &member, map_object_type obj_type );
};
struct map_deconstruct_info {
    // Only if true, the terrain/furniture can be deconstructed
    bool can_do;
    // This terrain provided a roof, we need to tear it down now
    bool deconstruct_above;
    // items you get when deconstructing.
    std::string drop_group;
    ter_str_id ter_set;    // terrain to set (REQUIRED for terrain))
    furn_str_id furn_set;    // furniture to set (only used by furniture, not terrain)
    map_deconstruct_info();
    bool load( const JsonObject &jsobj, const std::string &member, bool is_furniture );
};
struct furn_workbench_info {
    // Base multiplier applied for crafting here
    float multiplier;
    // Mass/volume allowed before a crafting speed penalty is applied
    units::mass allowed_mass;
    units::volume allowed_volume;
    furn_workbench_info();
    bool load( const JsonObject &jsobj, const std::string &member );
};
struct plant_data {
    // What the furniture turns into when it grows or you plant seeds in it
    furn_str_id transform;
    // What the 'base' furniture of the plant is, before you plant in it, and what it turns into when eaten
    furn_str_id base;
    // At what percent speed of a normal plant this plant furniture grows at
    float growth_multiplier;
    // What percent of the normal harvest this crop gives
    float harvest_multiplier;
    plant_data();
    bool load( const JsonObject &jsobj, const std::string &member );
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
 * SHARP - May do minor damage to players/monsters passing through it
 * ROUGH - May hurt the player's feet
 * SEALED - Can't use 'e' to retrieve items, must smash open first
 * NOITEM - Items 'fall off' this space
 * NO_SIGHT - When on this tile sight is reduced to 1
 * NO_SCENT - Scent on this tile (and thus scent diffusing through it) is reduced to 0. This acts like a wall for scent
 * MOUNTABLE - Player can fire mounted weapons from here (e.g. M2 Browning)
 * DESTROY_ITEM - Items that land here are destroyed
 * GOES_DOWN - Can use '>' to go down a level
 * GOES_UP - Can use '<' to go up a level
 * CONSOLE - Used as a computer
 * ALARMED - Sets off an alarm if smashed
 * SUPPORTS_ROOF - Used as a boundary for roof construction
 * MINEABLE - Able to broken with the jackhammer/pickaxe, but does not necessarily support a roof
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
 * RAMP - Higher z-levels can be accessed from this tile
 * EASY_DECONSTRUCT - Player can deconstruct this without tools
 * HIDE_PLACE - Creature on this tile can't be seen by other creature not standing on adjacent tiles
 * BLOCK_WIND - This tile will partially block wind
 * FLAT_SURF - Furniture or terrain or vehicle part with flat hard surface (ex. table, but not chair; tree stump, etc.).
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
 * WORKBENCH1/WORKBENCH2/WORKBENCH3 - This is an adequate/good/great workbench for crafting.  Must be paired with a workbench iexamine.
 */

/*
 * Note; All flags are defined as strings dynamically in data/json/terrain.json and furniture.json. The list above
 * represent the common builtins. The enum below is an alternative means of fast-access, for those flags that are checked
 * so much that strings produce a significant performance penalty. The following are equivalent:
 *  m->has_flag("FLAMMABLE");     //
 *  m->has_flag(TFLAG_FLAMMABLE); // ~ 20 x faster than the above, ( 2.5 x faster if the above uses static const std::string str_flammable("FLAMMABLE");
 * To add a new ter_bitflag, add below and add to ter_bitflags_map in mapdata.cpp
 * Order does not matter.
 */
enum ter_bitflags : int {
    TFLAG_TRANSPARENT,
    TFLAG_FLAMMABLE,
    TFLAG_REDUCE_SCENT,
    TFLAG_SWIMMABLE,
    TFLAG_SUPPORTS_ROOF,
    TFLAG_MINEABLE,
    TFLAG_NOITEM,
    TFLAG_NO_SIGHT,
    TFLAG_NO_SCENT,
    TFLAG_SEALED,
    TFLAG_ALLOW_FIELD_EFFECT,
    TFLAG_LIQUID,
    TFLAG_COLLAPSES,
    TFLAG_FLAMMABLE_ASH,
    TFLAG_DESTROY_ITEM,
    TFLAG_INDOORS,
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
    TFLAG_SHALLOW_WATER,
    TFLAG_CURRENT,
    TFLAG_HARVESTED,
    TFLAG_PERMEABLE,
    TFLAG_AUTO_WALL_SYMBOL,
    TFLAG_CONNECT_TO_WALL,
    TFLAG_CLIMBABLE,
    TFLAG_GOES_DOWN,
    TFLAG_GOES_UP,
    TFLAG_NO_FLOOR,
    TFLAG_SEEN_FROM_ABOVE,
    TFLAG_RAMP_DOWN,
    TFLAG_RAMP_UP,
    TFLAG_RAMP,
    TFLAG_HIDE_PLACE,
    TFLAG_BLOCK_WIND,
    TFLAG_FLAT,
    TFLAG_RAIL,
    TFLAG_THIN_OBSTACLE,
    TFLAG_SMALL_PASSAGE,
    TFLAG_Z_TRANSPARENT,

    NUM_TERFLAGS
};

/*
 * Terrain groups which affect whether the terrain connects visually.
 * Groups are also defined in ter_connects_map() in mapdata.cpp which matches group to JSON string.
 */
enum ter_connects : int {
    TERCONN_NONE,
    TERCONN_WALL,
    TERCONN_CHAINFENCE,
    TERCONN_WOODFENCE,
    TERCONN_RAILING,
    TERCONN_POOLWATER,
    TERCONN_WATER,
    TERCONN_PAVEMENT,
    TERCONN_RAIL,
    TERCONN_COUNTER,
    TERCONN_CANVAS_WALL,
};

struct map_data_common_t {
        map_bash_info        bash;
        map_deconstruct_info deconstruct;

    public:
        virtual ~map_data_common_t() = default;

    protected:
        friend furn_t null_furniture_t();
        friend ter_t null_terrain_t();
        // The (untranslated) plaintext name of the terrain type the user would see (i.e. dirt)
        std::string name_;

    private:
        std::set<std::string> flags;    // string flags which possibly refer to what's documented above.
        std::bitset<NUM_TERFLAGS> bitflags; // bitfield of -certain- string flags which are heavily checked

    public:
        ter_str_id curtain_transform;

        bool has_curtains() const {
            return !( curtain_transform.is_empty() || curtain_transform.is_null() );
        }

    public:
        std::string name() const;

        /*
        * The symbol drawn on the screen for the terrain. Please note that
        * there are extensive rules as to which possible object/field/entity in
        * a single square gets drawn and that some symbols are "reserved" such
        * as * and % to do programmatic behavior.
        */
        std::array<int, NUM_SEASONS> symbol_;

        int light_emitted = 0;
        // The amount of movement points required to pass this terrain by default.
        int movecost = 0;
        // The coverage percentage of a furniture piece of terrain. <30 won't cover from sight.
        int coverage = 0;
        // Maximal volume of items that can be stored in/on this furniture
        units::volume max_volume = 1000_liter;

        translation description;

        // The color the sym will draw in on the GUI.
        std::array<nc_color, NUM_SEASONS> color_;
        void load_symbol( const JsonObject &jo );

        std::string looks_like;

        iexamine_function examine; // What happens when the terrain/furniture is examined

        /**
         * When will this terrain/furniture get harvested and what will drop?
         * Note: This excludes items that take extra tools to harvest.
         */
        std::array<harvest_id, NUM_SEASONS> harvest_by_season = {{
                harvest_id::NULL_ID(), harvest_id::NULL_ID(), harvest_id::NULL_ID(), harvest_id::NULL_ID()
            }
        };

        bool transparent = false;

        const std::set<std::string> &get_flags() const {
            return flags;
        }

        bool has_flag( const std::string &flag ) const {
            return flags.count( flag ) > 0;
        }

        bool has_flag( const ter_bitflags flag ) const {
            return bitflags.test( flag );
        }

        void set_flag( const std::string &flag );

        int connect_group = 0;

        void set_connects( const std::string &connect_group_string );

        bool connects( int &ret ) const;

        bool connects_to( int test_connect_group ) const {
            return connect_group != TERCONN_NONE && connect_group == test_connect_group;
        }

        int symbol() const;
        nc_color color() const;

        const harvest_id &get_harvest() const;
        /**
         * Returns a set of names of the items that would be dropped.
         * Used for NPC whitelist checking.
         */
        const std::set<std::string> &get_harvest_names() const;

        std::string extended_description() const;

        bool was_loaded = false;

        bool is_flammable() const {
            return flags.count( "FLAMMABLE" ) > 0 || flags.count( "FLAMMABLE_ASH" ) > 0 ||
                   flags.count( "FLAMMABLE_HARD" ) > 0;
        }

        virtual void load( const JsonObject &jo, const std::string &src );
        virtual void check() const;
};

/*
* Struct ter_t:
* Short for terrain type. This struct defines all of the metadata for a given terrain id (an enum below).
*/
struct ter_t : map_data_common_t {
    ter_str_id id;    // The terrain's ID. Must be set, must be unique.
    ter_str_id open;  // Open action: transform into terrain with matching id
    ter_str_id close; // Close action: transform into terrain with matching id

    std::string trap_id_str;     // String storing the id string of the trap.
    ter_str_id transforms_into; // Transform into what terrain?
    ter_str_id roof;            // What will be the floor above this terrain

    trap_id trap; // The id of the trap located at this terrain. Limit one trap per tile currently.

    ter_t();

    static size_t count();

    void load( const JsonObject &jo, const std::string &src ) override;
    void check() const override;
};

void set_ter_ids();
void set_furn_ids();
void reset_furn_ter();

/*
 * The terrain list contains the master list of  information and metadata for a given type of terrain.
 */

struct furn_t : map_data_common_t {
    furn_str_id id;
    furn_str_id open;  // Open action: transform into furniture with matching id
    furn_str_id close; // Close action: transform into furniture with matching id
    itype_id crafting_pseudo_item;
    units::volume keg_capacity = 0_ml;
    int comfort = 0;
    int floor_bedding_warmth = 0;
    /** Emissions of furniture */
    std::set<emit_id> emissions;

    int bonus_fire_warmth_feet = 300;
    itype_id deployed_item; // item id string used to create furniture

    int move_str_req = 0; //The amount of strength required to move through this furniture easily.

    cata::value_ptr<furn_workbench_info> workbench;

    cata::value_ptr<plant_data> plant;

    cata::value_ptr<float> surgery_skill_multiplier;

    // May return NULL
    const itype *crafting_pseudo_item_type() const;
    // May return NULL
    const itype *crafting_ammo_item_type() const;

    furn_t();

    static size_t count();

    bool is_movable() const;

    void load( const JsonObject &jo, const std::string &src ) override;
    void check() const override;
};

void load_furniture( const JsonObject &jo, const std::string &src );
void load_terrain( const JsonObject &jo, const std::string &src );

void verify_furniture();
void verify_terrain();

/*
runtime index: ter_id
ter_id refers to a position in the terlist[] where the ter_t struct is stored. These global
ints are a drop-in replacement to the old enum, however they are -not- required (save for areas in
the code that can use the performance boost and refer to core terrain types), and they are -not-
provided for terrains added by mods. A string equivalent is always present, i.e.;
t_basalt
"t_basalt"
*/
extern ter_id t_null,
       t_hole, // Real nothingness; makes you fall a z-level
       // Ground
       t_dirt, t_sand, t_clay, t_dirtmound, t_pit_shallow, t_pit, t_grave, t_grave_new,
       t_pit_corpsed, t_pit_covered, t_pit_spiked, t_pit_spiked_covered, t_pit_glass, t_pit_glass_covered,
       t_rock_floor,
       t_grass, t_grass_long, t_grass_tall, t_grass_golf, t_grass_dead, t_grass_white, t_moss,
       t_metal_floor,
       t_pavement, t_pavement_y, t_sidewalk, t_concrete,
       t_thconc_floor, t_thconc_floor_olight, t_strconc_floor,
       t_floor, t_floor_waxed,
       t_dirtfloor,//Dirt floor(Has roof)
       t_carpet_red, t_carpet_yellow, t_carpet_purple, t_carpet_green,
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
       t_wall_glass,
       t_wall_glass_alarm,
       t_reinforced_glass, t_reinforced_glass_shutter, t_reinforced_glass_shutter_open,
       t_laminated_glass, t_ballistic_glass,
       t_reinforced_door_glass_o, t_reinforced_door_glass_c,
       t_bars,
       t_reb_cage,
       t_door_c, t_door_c_peep, t_door_b, t_door_b_peep, t_door_o, t_door_o_peep,
       t_door_locked_interior, t_door_locked, t_door_locked_peep, t_door_locked_alarm, t_door_frame,
       t_chaingate_l, t_fencegate_c, t_fencegate_o, t_chaingate_c, t_chaingate_o,
       t_door_boarded, t_door_boarded_damaged, t_door_boarded_peep, t_rdoor_boarded,
       t_rdoor_boarded_damaged, t_door_boarded_damaged_peep,
       t_door_metal_c, t_door_metal_o, t_door_metal_locked, t_door_metal_pickable,
       t_door_bar_c, t_door_bar_o, t_door_bar_locked,
       t_door_glass_c, t_door_glass_o, t_door_glass_frosted_c, t_door_glass_frosted_o,
       t_portcullis,
       t_recycler, t_window, t_window_taped, t_window_domestic, t_window_domestic_taped, t_window_open,
       t_curtains, t_window_bars_curtains, t_window_bars_domestic,
       t_window_alarm, t_window_alarm_taped, t_window_empty, t_window_frame, t_window_boarded,
       t_window_boarded_noglass, t_window_bars_alarm, t_window_bars,
       t_window_stained_green, t_window_stained_red, t_window_stained_blue,
       t_window_no_curtains, t_window_no_curtains_open, t_window_no_curtains_taped,
       t_rock, t_fault,
       t_paper,
       t_rock_wall, t_rock_wall_half,
       // Tree
       t_tree, t_tree_young, t_tree_apple, t_tree_apple_harvested, t_tree_coffee, t_tree_coffee_harvested,
       t_tree_pear, t_tree_pear_harvested,
       t_tree_cherry, t_tree_cherry_harvested, t_tree_peach, t_tree_peach_harvested, t_tree_apricot,
       t_tree_apricot_harvested,
       t_tree_plum, t_tree_plum_harvested, t_tree_pine, t_tree_blackjack, t_tree_birch,
       t_tree_birch_harvested, t_tree_willow, t_tree_willow_harvested, t_tree_maple, t_tree_maple_tapped,
       t_tree_deadpine, t_tree_hickory, t_tree_hickory_dead, t_tree_hickory_harvested, t_underbrush,
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
       t_water_moving_dp, t_water_moving_sh, t_water_sh, t_swater_sh, t_water_dp, t_swater_dp,
       t_water_pool, t_sewage,
       t_lava,
       // More embellishments than you can shake a stick at.
       t_sandbox, t_slide, t_monkey_bars, t_backboard,
       t_gas_pump, t_gas_pump_smashed,
       t_diesel_pump, t_diesel_pump_smashed,
       t_atm,
       t_generator_broken,
       t_missile, t_missile_exploded,
       t_radio_tower, t_radio_controls,
       t_console_broken, t_console, t_gates_mech_control, t_gates_control_concrete, t_gates_control_brick,
       t_barndoor, t_palisade_pulley,
       t_gates_control_metal,
       t_sewage_pipe, t_sewage_pump,
       t_centrifuge,
       t_column,
       t_vat,
       t_rootcellar,
       t_cvdbody, t_cvdmachine,
       t_water_pump,
       t_conveyor, t_machinery_light, t_machinery_heavy, t_machinery_old, t_machinery_electronic,
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
       t_switch_rg, t_switch_gb, t_switch_rb, t_switch_even,
       t_rdoor_c, t_rdoor_b, t_rdoor_o, t_mdoor_frame, t_window_reinforced, t_window_reinforced_noglass,
       t_window_enhanced, t_window_enhanced_noglass, t_open_air, t_plut_generator,
       t_pavement_bg_dp, t_pavement_y_bg_dp, t_sidewalk_bg_dp, t_guardrail_bg_dp,
       t_linoleum_white, t_linoleum_gray, t_rad_platform,
       // Railroad and subway
       t_railroad_rubble,
       t_buffer_stop, t_railroad_crossing_signal, t_crossbuck_wood, t_crossbuck_metal,
       t_railroad_tie, t_railroad_tie_h, t_railroad_tie_v, t_railroad_tie_d,
       t_railroad_track, t_railroad_track_h, t_railroad_track_v, t_railroad_track_d, t_railroad_track_d1,
       t_railroad_track_d2,
       t_railroad_track_on_tie, t_railroad_track_h_on_tie, t_railroad_track_v_on_tie,
       t_railroad_track_d_on_tie;

/*
runtime index: furn_id
furn_id refers to a position in the furnlist[] where the furn_t struct is stored. See note
about ter_id above.
*/
extern furn_id f_null,
       f_hay, f_cattails, f_lotus, f_lilypad,
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
       f_large_canvas_wall, f_canvas_wall, f_canvas_door, f_canvas_door_o, f_groundsheet,
       f_fema_groundsheet, f_large_groundsheet,
       f_large_canvas_door, f_large_canvas_door_o, f_center_groundsheet, f_skin_wall, f_skin_door,
       f_skin_door_o,  f_skin_groundsheet,
       f_mutpoppy, f_flower_fungal, f_fungal_mass, f_fungal_clump,
       f_safe_c, f_safe_l, f_safe_o,
       f_plant_seed, f_plant_seedling, f_plant_mature, f_plant_harvest,
       f_fvat_empty, f_fvat_full,
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
       f_gunsafe_ml, f_gunsafe_mj, f_gun_safe_el;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//// These are on their way OUT and only used in certain switch statements until they are rewritten.

// consistency checking of terlist & furnlist.
void check_furniture_and_terrain();

void finalize_furniture_and_terrain();

#endif // CATA_SRC_MAPDATA_H
