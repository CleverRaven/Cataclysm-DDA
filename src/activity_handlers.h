#pragma once
#ifndef CATA_SRC_ACTIVITY_HANDLERS_H
#define CATA_SRC_ACTIVITY_HANDLERS_H

#include <functional>
#include <list>
#include <map>
#include <new>
#include <unordered_set>
#include <vector>

#include "optional.h"
#include "type_id.h"
#include "requirements.h"

class Character;
class item;
class player_activity;
struct tripoint;

std::vector<tripoint> get_sorted_tiles_by_distance( const tripoint &abspos,
        const std::unordered_set<tripoint> &tiles );
std::vector<tripoint> route_adjacent( const Character &you, const tripoint &dest );

std::vector<tripoint> route_best_workbench( const Character &you, const tripoint &dest );

enum class requirement_check_result : int {
    SKIP_LOCATION = 0,
    CAN_DO_LOCATION,
    RETURN_EARLY       //another activity like a fetch activity has been started.
};

enum class butcher_type : int {
    BLEED,          // bleeding a corpse
    QUICK,          // quick butchery
    FULL,           // full workshop butchery
    FIELD_DRESS,    // field dressing a corpse
    SKIN,           // skinning a corpse
    QUARTER,        // quarter a corpse
    DISMEMBER,      // destroy a corpse
    DISSECT,        // dissect a corpse for CBMs
    NUM_TYPES       // always keep at the end, number of butchery types
};

enum class do_activity_reason : int {
    CAN_DO_CONSTRUCTION,    // Can do construction.
    CAN_DO_FETCH,           // Can do fetch - this is usually the default result for fetch task
    CAN_DO_PREREQ,          // for constructions - can't build the main construction, but can build the pre-req
    CAN_DO_PREREQ_2,        // Can do the second pre-req deep below the desired one.
    NO_COMPONENTS,          // can't do the activity there due to lack of components /tools
    NO_COMPONENTS_PREREQ,   // need components to build the pre-requisite for the actual desired construction
    NO_COMPONENTS_PREREQ_2, // need components to the second pre-req deep.
    DONT_HAVE_SKILL,        // don't have the required skill
    NO_ZONE,                // There is no required zone anymore
    ALREADY_DONE,           // the activity is done already ( maybe by someone else )
    UNKNOWN_ACTIVITY,       // This is probably an error - got to the end of function with no previous reason
    NEEDS_HARVESTING,       // For farming - tile is harvestable now.
    NEEDS_PLANTING,         // For farming - tile can be planted
    NEEDS_TILLING,          // For farming - tile can be tilled
    BLOCKING_TILE,           // Something has made it's way onto the tile, so the activity cannot proceed
    NEEDS_CHOPPING,         // There is wood there to be chopped
    NEEDS_TREE_CHOPPING,    // There is a tree there that needs to be chopped
    NEEDS_BIG_BUTCHERING,   // There is at least one corpse there to butcher, and it's a big one
    NEEDS_BUTCHERING,       // THere is at least one corpse there to butcher, and there's no need for additional tools
    ALREADY_WORKING,        // somebody is already working there
    NEEDS_VEH_DECONST,       // There is a vehicle part there that we can deconstruct, given the right tools.
    NEEDS_VEH_REPAIR,       // There is a vehicle part there that can be repaired, given the right tools.
    WOULD_PREVENT_VEH_FLYING, // Attempting to perform this activity on a vehicle would prevent it from flying
    NEEDS_MINING,           // This spot can be mined, if the right tool is present.
    NEEDS_FISHING,           // This spot can be fished, if the right tool is present.
    NEEDS_DISASSEMBLE        // There is at least one item to disassemble.

};

struct activity_reason_info {
    //reason for success or fail
    do_activity_reason reason;
    //is it possible to do this
    bool can_do;
    //construction index
    cata::optional<construction_id> con_idx;

    activity_reason_info( do_activity_reason reason_, bool can_do_,
                          const cata::optional<construction_id> &con_idx_ = cata::nullopt ) :
        reason( reason_ ),
        can_do( can_do_ ),
        con_idx( con_idx_ )
    { }
    activity_reason_info( do_activity_reason reason_, bool can_do_, const requirement_data &req ):
        reason( reason_ ),
        can_do( can_do_ ),
        req( req )
    { }

    const requirement_data req;

    static activity_reason_info ok( const do_activity_reason &reason_ ) {
        return activity_reason_info( reason_, true );
    }

    static activity_reason_info build( const do_activity_reason &reason_, bool can_do_,
                                       const construction_id &con_idx_ ) {
        return activity_reason_info( reason_, can_do_, con_idx_ );
    }

    static activity_reason_info fail( const do_activity_reason &reason_ ) {
        return activity_reason_info( reason_, false );
    }
};

int butcher_time_to_cut( Character &you, const item &corpse_item, butcher_type action );

// activity_item_handling.cpp
void activity_on_turn_drop();
void activity_on_turn_move_loot( player_activity &act, Character &you );
//return true if there is an activity that can be done potentially, return false if no work can be found.
bool generic_multi_activity_handler( player_activity &act, Character &you,
                                     bool check_only = false );
void activity_on_turn_fetch( player_activity &, Character *you );
void activity_on_turn_wear( player_activity &act, Character &you );
int get_auto_consume_moves( Character &you, bool food );
bool try_fuel_fire( player_activity &act, Character &you, bool starting_fire = false );

enum class item_drop_reason : int {
    deliberate,
    too_large,
    too_heavy,
    tumbling
};

void put_into_vehicle_or_drop( Character &you, item_drop_reason, const std::list<item> &items );
void put_into_vehicle_or_drop( Character &you, item_drop_reason, const std::list<item> &items,
                               const tripoint &where, bool force_ground = false );
void drop_on_map( Character &you, item_drop_reason reason, const std::list<item> &items,
                  const tripoint &where );

namespace activity_handlers
{

bool resume_for_multi_activities( Character &you );
void generic_game_turn_handler( player_activity *act, Character *you, int morale_bonus,
                                int morale_max_bonus );

/** activity_do_turn functions: */
void fill_liquid_do_turn( player_activity *act, Character *you );
void pickaxe_do_turn( player_activity *act, Character *you );
void drop_do_turn( player_activity *act, Character *you );
void stash_do_turn( player_activity *act, Character *you );
void pulp_do_turn( player_activity *act, Character *you );
void game_do_turn( player_activity *act, Character *you );
void generic_game_do_turn( player_activity *act, Character *you );
void churn_do_turn( player_activity *act, Character *you );
void start_fire_do_turn( player_activity *act, Character *you );
void vibe_do_turn( player_activity *act, Character *you );
void hand_crank_do_turn( player_activity *act, Character *you );
void multiple_chop_planks_do_turn( player_activity *act, Character *you );
void wear_do_turn( player_activity *act, Character *you );
void eat_menu_do_turn( player_activity *act, Character *you );
void consume_food_menu_do_turn( player_activity *act, Character *you );
void consume_drink_menu_do_turn( player_activity *act, Character *you );
void consume_meds_menu_do_turn( player_activity *act, Character *you );
void consume_fuel_menu_do_turn( player_activity *act, Character *you );
void move_items_do_turn( player_activity *act, Character *you );
void multiple_farm_do_turn( player_activity *act, Character *you );
void multiple_fish_do_turn( player_activity *act, Character *you );
void multiple_construction_do_turn( player_activity *act, Character *you );
void multiple_mine_do_turn( player_activity *act, Character *you );
void multiple_butcher_do_turn( player_activity *act, Character *you );
void multiple_dis_do_turn( player_activity *act, Character *you );
void vehicle_deconstruction_do_turn( player_activity *act, Character *you );
void vehicle_repair_do_turn( player_activity *act, Character *you );
void chop_trees_do_turn( player_activity *act, Character *you );
void fetch_do_turn( player_activity *act, Character *you );
void move_loot_do_turn( player_activity *act, Character *you );
void travel_do_turn( player_activity *act, Character *you );
void adv_inventory_do_turn( player_activity *act, Character *you );
void armor_layers_do_turn( player_activity *act, Character *you );
void atm_do_turn( player_activity *act, Character *you );
void fish_do_turn( player_activity *act, Character *you );
void cracking_do_turn( player_activity *act, Character *you );
void repair_item_do_turn( player_activity *act, Character *you );
void butcher_do_turn( player_activity *act, Character *you );
void chop_tree_do_turn( player_activity *act, Character *you );
void jackhammer_do_turn( player_activity *act, Character *you );
void find_mount_do_turn( player_activity *act, Character *you );
void tidy_up_do_turn( player_activity *act, Character *you );
void build_do_turn( player_activity *act, Character *you );
void fill_pit_do_turn( player_activity *act, Character *you );
void fertilize_plot_do_turn( player_activity *act, Character *you );
void operation_do_turn( player_activity *act, Character *you );
void robot_control_do_turn( player_activity *act, Character *you );
void tree_communion_do_turn( player_activity *act, Character *you );
void spellcasting_do_turn( player_activity *act, Character *you );
void study_spell_do_turn( player_activity *act, Character *you );
void wait_stamina_do_turn( player_activity *act, Character *you );

// defined in activity_handlers.cpp
extern const std::map< activity_id, std::function<void( player_activity *, Character * )> >
do_turn_functions;

/** activity_finish functions: */
void butcher_finish( player_activity *act, Character *you );
void firstaid_finish( player_activity *act, Character *you );
void fish_finish( player_activity *act, Character *you );
void forage_finish( player_activity *act, Character *you );
void longsalvage_finish( player_activity *act, Character *you );
void pulp_finish( player_activity *act, Character *you );
void pickaxe_finish( player_activity *act, Character *you );
void start_fire_finish( player_activity *act, Character *you );
void train_finish( player_activity *act, Character *you );
void shear_finish( player_activity *act, Character *you );
void vehicle_finish( player_activity *act, Character *you );
void start_engines_finish( player_activity *act, Character *you );
void churn_finish( player_activity *act, Character *you );
void plant_seed_finish( player_activity *act, Character *you );
void cracking_finish( player_activity *act, Character *guy );
void repair_item_finish( player_activity *act, Character *you );
void mend_item_finish( player_activity *act, Character *you );
void gunmod_add_finish( player_activity *act, Character *you );
void toolmod_add_finish( player_activity *act, Character *you );
void clear_rubble_finish( player_activity *act, Character *you );
void heat_item_finish( player_activity *act, Character *you );
void wait_finish( player_activity *act, Character *you );
void wait_weather_finish( player_activity *act, Character *you );
void wait_npc_finish( player_activity *act, Character *you );
void wait_stamina_finish( player_activity *act, Character *you );
void socialize_finish( player_activity *act, Character *you );
void operation_finish( player_activity *act, Character *you );
void vibe_finish( player_activity *act, Character *you );
void hand_crank_finish( player_activity *act, Character *you );
void atm_finish( player_activity *act, Character *you );
void eat_menu_finish( player_activity *act, Character *you );
void washing_finish( player_activity *act, Character *you );
void chop_tree_finish( player_activity *act, Character *you );
void chop_logs_finish( player_activity *act, Character *you );
void chop_planks_finish( player_activity *act, Character *you );
void jackhammer_finish( player_activity *act, Character *you );
void fill_pit_finish( player_activity *act, Character *you );
void robot_control_finish( player_activity *act, Character *you );
void mind_splicer_finish( player_activity *act, Character *you );
void spellcasting_finish( player_activity *act, Character *you );
void study_spell_finish( player_activity *act, Character *you );

// defined in activity_handlers.cpp
extern const std::map< activity_id, std::function<void( player_activity *, Character * )> >
finish_functions;

} // namespace activity_handlers

#endif // CATA_SRC_ACTIVITY_HANDLERS_H
