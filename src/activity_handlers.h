#pragma once
#ifndef ACTIVITY_HANDLERS_H
#define ACTIVITY_HANDLERS_H

#include <functional>
#include <map>
#include <unordered_set>
#include <vector>

#include "player_activity.h"

class player;

std::vector<tripoint> get_sorted_tiles_by_distance( const tripoint &abspos,
        const std::unordered_set<tripoint> &tiles );
std::vector<tripoint> route_adjacent( const player &p, const tripoint &dest );

enum butcher_type : int {
    BUTCHER,        // quick butchery
    BUTCHER_FULL,   // full workshop butchery
    F_DRESS,        // field dressing a corpse
    SKIN,           // skinning a corpse
    QUARTER,        // quarter a corpse
    DISMEMBER,      // destroy a corpse
    DISSECT         // dissect a corpse for CBMs
};

int butcher_time_to_cut( const player &u, const item &corpse_item, const butcher_type action );

// activity_item_handling.cpp
void activity_on_turn_drop();
void activity_on_turn_move_items();
void activity_on_turn_move_loot( player_activity &act, player &p );
void activity_on_turn_pickup();
void activity_on_turn_wear();
void activity_on_turn_stash();
void try_refuel_fire( player &p );

enum class item_drop_reason {
    deliberate,
    too_large,
    too_heavy,
    tumbling
};

void put_into_vehicle_or_drop( Character &c, item_drop_reason, const std::list<item> &items );
void put_into_vehicle_or_drop( Character &c, item_drop_reason, const std::list<item> &items,
                               const tripoint &where, bool force_ground = false );

namespace activity_handlers
{

/** activity_do_turn functions: */
void burrow_do_turn( player_activity *act, player *p );
void craft_do_turn( player_activity *act, player *p );
void fill_liquid_do_turn( player_activity *act, player *p );
void pickaxe_do_turn( player_activity *act, player *p );
void drop_do_turn( player_activity *act, player *p );
void stash_do_turn( player_activity *act, player *p );
void pulp_do_turn( player_activity *act, player *p );
void game_do_turn( player_activity *act, player *p );
void start_fire_do_turn( player_activity *act, player *p );
void vibe_do_turn( player_activity *act, player *p );
void oxytorch_do_turn( player_activity *act, player *p );
void aim_do_turn( player_activity *act, player *p );
void pickup_do_turn( player_activity *act, player *p );
void wear_do_turn( player_activity *act, player *p );
void move_items_do_turn( player_activity *act, player *p );
void move_loot_do_turn( player_activity *act, player *p );
void adv_inventory_do_turn( player_activity *act, player *p );
void armor_layers_do_turn( player_activity *act, player *p );
void atm_do_turn( player_activity *act, player *p );
void cracking_do_turn( player_activity *act, player *p );
void repair_item_do_turn( player_activity *act, player *p );
void butcher_do_turn( player_activity *act, player *p );
void hacksaw_do_turn( player_activity *act, player *p );
void chop_tree_do_turn( player_activity *act, player *p );
void jackhammer_do_turn( player_activity *act, player *p );
void dig_do_turn( player_activity *act, player *p );
void fill_pit_do_turn( player_activity *act, player *p );
void till_plot_do_turn( player_activity *act, player *p );
void plant_plot_do_turn( player_activity *act, player *p );
void fertilize_plot_do_turn( player_activity *act, player *p );
void harvest_plot_do_turn( player_activity *act, player *p );
void try_sleep_do_turn( player_activity *act, player *p );

// defined in activity_handlers.cpp
extern const std::map< activity_id, std::function<void( player_activity *, player * )> >
do_turn_functions;

/** activity_finish functions: */
void burrow_finish( player_activity *act, player *p );
void butcher_finish( player_activity *act, player *p );
void firstaid_finish( player_activity *act, player *p );
void fish_finish( player_activity *act, player *p );
void forage_finish( player_activity *act, player *p );
void hotwire_finish( player_activity *act, player *p );
void longsalvage_finish( player_activity *act, player *p );
void make_zlave_finish( player_activity *act, player *p );
void pickaxe_finish( player_activity *act, player *p );
void reload_finish( player_activity *act, player *p );
void start_fire_finish( player_activity *act, player *p );
void train_finish( player_activity *act, player *p );
void vehicle_finish( player_activity *act, player *p );
void start_engines_finish( player_activity *act, player *p );
void oxytorch_finish( player_activity *act, player *p );
void cracking_finish( player_activity *act, player *p );
void open_gate_finish( player_activity *act, player * );
void repair_item_finish( player_activity *act, player *p );
void mend_item_finish( player_activity *act, player *p );
void gunmod_add_finish( player_activity *act, player *p );
void toolmod_add_finish( player_activity *act, player *p );
void clear_rubble_finish( player_activity *act, player *p );
void meditate_finish( player_activity *act, player *p );
void read_finish( player_activity *act, player *p );
void wait_finish( player_activity *act, player *p );
void wait_weather_finish( player_activity *act, player *p );
void wait_npc_finish( player_activity *act, player *p );
void socialize_finish( player_activity *act, player *p );
void try_sleep_finish( player_activity *act, player *p );
void craft_finish( player_activity *act, player *p );
void longcraft_finish( player_activity *act, player *p );
void disassemble_finish( player_activity *act, player *p );
void build_finish( player_activity *act, player *p );
void vibe_finish( player_activity *act, player *p );
void atm_finish( player_activity *act, player *p );
void aim_finish( player_activity *act, player *p );
void washing_finish( player_activity *act, player *p );
void hacksaw_finish( player_activity *act, player *p );
void chop_tree_finish( player_activity *act, player *p );
void chop_logs_finish( player_activity *act, player *p );
void jackhammer_finish( player_activity *act, player *p );
void dig_finish( player_activity *act, player *p );
void fill_pit_finish( player_activity *act, player *p );
void play_with_pet_finish( player_activity *act, player *p );
void shaving_finish( player_activity *act, player *p );
void haircut_finish( player_activity *act, player *p );
void unload_mag_finish( player_activity *act, player *p );
void robot_control_finish( player_activity *act, player *p );

// defined in activity_handlers.cpp
extern const std::map< activity_id, std::function<void( player_activity *, player * )> >
finish_functions;

}

#endif
