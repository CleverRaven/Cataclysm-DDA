#pragma once
#ifndef DIALOGUE_H
#define DIALOGUE_H

#include <memory>
#include <vector>
#include <string>
#include <functional>

class martialart;
class JsonObject;
class mission;
class time_point;
class npc;
class item;
struct tripoint;
class player;
class npc_template;
template<typename T>
class string_id;

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

void drop_weapon( npc & );
void player_weapon_away( npc & );
void player_weapon_drop( npc & );

void lead_to_safety( npc & );
void start_training( npc & );

void wake_up( npc & );

/*mission_companion.cpp proves a set of functions that compress all the typical mission operations into a set of hard-coded
 *unique missions that don't fit well into the framework of the existing system.  These missions typically focus on
 *sending companions out on simulated adventures or tasks.  This is not meant to be a replacement for the existing system.
 */
//Identifies which mission set the NPC draws from
void companion_mission( npc & );
//Primary Loop
bool outpost_missions( npc &p, std::string id, std::string title );
//Send a companion on an individual mission or attaches them to a group to depart later
void individual_mission( npc &p, std::string desc, std::string id, bool group = false );

void caravan_return( npc &p, std::string dest, std::string id );
void caravan_depart( npc &p, std::string dest, std::string id );
int caravan_dist( std::string dest );
void field_build_1( npc &p );
void field_build_2( npc &p );
void field_plant( npc &p, std::string place );
void field_harvest( npc &p, std::string place );
bool scavenging_patrol_return( npc &p );
bool scavenging_raid_return( npc &p );
bool labor_return( npc &p );
bool carpenter_return( npc &p );
bool forage_return( npc &p );

//Combat functions
void force_on_force( std::vector<std::shared_ptr<npc>> defender, std::string def_desc,
                     std::vector<std::shared_ptr<npc>> attacker, std::string att_desc, int advantage );
int combat_score( const std::vector<std::shared_ptr<npc>> &group );    //Used to determine retreat
void attack_random( const std::vector<std::shared_ptr<npc>> &attacker,
                    const std::vector<std::shared_ptr<npc>> &defender );
std::shared_ptr<npc> temp_npc( const string_id<npc_template> &type );

//Utility functions
/// Returns npcs that have the given companion mission.
std::vector<std::shared_ptr<npc>> companion_list( const npc &p, const std::string &id );
npc *companion_choose();
npc *companion_choose_return( std::string id, const time_point &deadline );
void companion_return( npc &comp );               //Return NPC to your party
std::vector<item *> loot_building( const tripoint
                                   site ); //Smash stuff, steal valuables, and change map maker
};

void unload_talk_topics();
void load_talk_topic( JsonObject &jo );

#endif
