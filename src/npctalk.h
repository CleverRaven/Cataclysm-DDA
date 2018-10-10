#pragma once
#ifndef NPCTALK_H
#define NPCTALK_H

#include <memory>
#include <vector>
#include <string>
#include <functional>

namespace talk_function
{
void nothing( npc & );
void assign_mission( npc & );
void mission_success( npc & );
void mission_failure( npc & );
void clear_mission( npc & );
void mission_reward( npc & );
void mission_favor( npc & );
void give_equipment( npc & );
void give_aid( npc & );
void give_all_aid( npc & );

void bionic_install( npc & );
void bionic_remove( npc & );

void buy_haircut( npc & );
void buy_shave( npc & );
void buy_10_logs( npc & );
void buy_100_logs( npc & );
void give_equipment( npc & );
void start_trade( npc & );
void assign_base( npc & );
void assign_guard( npc & );
void stop_guard( npc & );
void end_conversation( npc & );
void insult_combat( npc & );
void reveal_stats( npc & );
void follow( npc & );                // p follows u
void deny_follow( npc & );           // p gets "asked_to_follow"
void deny_lead( npc & );             // p gets "asked_to_lead"
void deny_equipment( npc & );        // p gets "asked_for_item"
void deny_train( npc & );            // p gets "asked_to_train"
void deny_personal_info( npc & );    // p gets "asked_personal_info"
void hostile( npc & );               // p turns hostile to u
void flee( npc & );
void leave( npc & );                 // p becomes indifferent
void stranger_neutral( npc & );      // p is now neutral towards you

void start_mugging( npc & );
void player_leaving( npc & );

void start_mugging( npc & );
void player_leaving( npc & );

void drop_weapon( npc & );
void player_weapon_away( npc & );
void player_weapon_drop( npc & );

void lead_to_safety( npc & );
void start_training( npc & );

void wake_up( npc & );

};

bool trade( npc &p, int cost, const std::string &deal );
time_duration calc_skill_training_time( const npc &p, const skill_id &skill );
int calc_skill_training_cost( const npc &p, const skill_id &skill );
time_duration calc_ma_style_training_time( const npc &, const matype_id & /* id */ );
int calc_ma_style_training_cost( const npc &p, const matype_id & /* id */ );
#endif
