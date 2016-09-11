#ifndef ACTIVITY_HANDLERS_H
#define ACTIVITY_HANDLERS_H

class player_activity;
class player;

namespace activity_handlers
{
void burrow_do_turn( player_activity *act, player *p );
void burrow_finish( player_activity *act, player *p );
void butcher_finish( player_activity *act, player *p );
void fill_liquid_do_turn( player_activity *act, player *p );
void firstaid_finish( player_activity *act, player *p );
void fish_finish( player_activity *act, player *p );
void forage_finish( player_activity *act, player *p );
void game_do_turn( player_activity *act, player *p );
void hotwire_finish( player_activity *act, player *p );
void longsalvage_finish( player_activity *act, player *p );
void make_zlave_finish( player_activity *act, player *p );
void pickaxe_do_turn( player_activity *act, player *p );
void pickaxe_finish( player_activity *act, player *p );
void pickup_finish( player_activity *act, player *p );
void drop_do_turn( player_activity *act, player *p );
void stash_do_turn( player_activity *act, player *p );
void pulp_do_turn( player_activity *act, player *p );
void reload_finish( player_activity *act, player *p );
void start_fire_finish( player_activity *act, player *p );
void start_fire_do_turn( player_activity *act, player *p );
void train_finish( player_activity *act, player *p );
void vehicle_finish( player_activity *act, player *p );
void vibe_do_turn( player_activity *act, player *p );
void start_engines_finish( player_activity *act, player *p );
void oxytorch_do_turn( player_activity *act, player *p );
void oxytorch_finish( player_activity *act, player *p );
void cracking_finish( player_activity *act, player *p );
void open_gate_finish( player_activity *act, player * );
void repair_item_finish( player_activity *act, player *p );
void mend_item_finish( player_activity *act, player *p );
void gunmod_add_finish( player_activity *act, player *p );
void clear_rubble_finish( player_activity *act, player *p );
void meditate_finish( player_activity *act, player *p );
}

#endif
