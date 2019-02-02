#pragma once
#ifndef MISSION_COMPANION_H
#define MISSION_COMPANION_H

#include <memory>
#include <string>
#include <vector>

#include "game.h"
#include "map.h"
#include "npc.h"
#include "output.h"

class martialart;
class JsonObject;
class mission;
class time_point;
class time_duration;
class npc;
class item;
struct tripoint;
struct comp_rank;
class player;
class npc_template;
template<typename T>
class string_id;

struct mission_entry {
    std::string id;
    std::string name_display;
    std::string dir;
    std::string text;
    bool priority;
    bool possible;
};

class mission_data
{
    public:
        //see mission_key_push() for each key description
        std::vector<std::vector<mission_entry>> entries;
        mission_entry cur_key;

        mission_data();
        /**
        * Adds the id's to the correct vectors (ie tabs) in the UI.
        * @param id is the mission reference
        * @param name_display is string displayed
        * @param dir is the direction of the expansion from the central camp, ie "[N]"
        * @param priority turns the mission key yellow and pushes to front of main tab
        * @param possible grays the mission key when false
        */
        void add( const std::string &id, const std::string &name_display = "",
                  const std::string &text = "" );
        void add_start( const std::string &id, const std::string &name_display,
                        const std::string &dir, const std::string &text, bool possible = true );
        void add_return( const std::string &id, const std::string &name_display,
                         const std::string &dir, const std::string &text, bool possible = true );
        void add( const std::string &id, const std::string &name_display,
                  const std::string &dir, const std::string &text,
                  bool priority = false, bool possible = true );
};

namespace talk_function
{
/*mission_companion.cpp proves a set of functions that compress all the typical mission operations into a set of hard-coded
 *unique missions that don't fit well into the framework of the existing system.  These missions typically focus on
 *sending companions out on simulated adventures or tasks.  This is not meant to be a replacement for the existing system.
 */

void camp_missions( mission_data &mission_key, npc &p );
bool handle_camp_mission( mission_entry &cur_key, npc &p );

//Identifies which mission set the NPC draws from
void companion_mission( npc & );
/**
 * Send a companion on an individual mission or attaches them to a group to depart later
 * Set @ref submap_coords and @ref pos.
 * @param desc is the description in the popup window
 * @param miss_id is the value stored with the NPC when it is offloaded
 * @param group is whether the NPC is waiting for additional members before departing together
 * @param equipment is placed in the NPC's special inventory and dropped when they return
 * @param skill_tested is the main skill for the quest
 * @param skill_level is checked to prevent lower level NPCs from going on missions
 */
///Send a companion on an individual mission or attaches them to a group to depart later
std::shared_ptr<npc> individual_mission( npc &p, const std::string &desc,
        const std::string &miss_id,
        bool group = false, const std::vector<item *> &equipment = {},
        const std::string &skill_tested = "", int skill_level = 0 );

///Display items listed in @ref equipment to let the player pick what to give the departing NPC, loops until quit or empty.
std::vector<item *> individual_mission_give_equipment( std::vector<item *> equipment,
        const std::string &message = _( "Do you wish to give your companion additional items?" ) );

///All of these missions are associated with the ranch camp and need to me updated/merged into the new ones
void caravan_return( npc &p, const std::string &dest, const std::string &id );
void caravan_depart( npc &p, const std::string &dest, const std::string &id );
int caravan_dist( const std::string &dest );

void field_build_1( npc &p );
void field_build_2( npc &p );
void field_plant( npc &p, const std::string &place );
void field_harvest( npc &p, const std::string &place );
bool scavenging_patrol_return( npc &p );
bool scavenging_raid_return( npc &p );
bool labor_return( npc &p );
bool carpenter_return( npc &p );
bool forage_return( npc &p );

/// Trains NPC @ref comp, in skill_tested for duration time_worked at difficulty 1, several groups of skills can also be input
void companion_skill_trainer( npc &comp, const std::string &skill_tested = "",
                              time_duration time_worked = 1_hours, int difficulty = 1 );
void companion_skill_trainer( npc &comp, const skill_id &skill_tested,
                              time_duration time_worked = 1_hours, int difficulty = 1 );
//Combat functions
bool companion_om_combat_check( const std::vector<std::shared_ptr<npc>> &group,
                                const tripoint &om_tgt,
                                bool try_engage = false );
void force_on_force( const std::vector<std::shared_ptr<npc>> &defender, const std::string &def_desc,
                     const std::vector<std::shared_ptr<npc>> &attacker, const std::string &att_desc, int advantage );
bool force_on_force( const std::vector<std::shared_ptr<npc>> &defender, const std::string &def_desc,
                     const std::vector< monster * > &monsters_fighting, const std::string &att_desc, int advantage );
int combat_score( const std::vector<std::shared_ptr<npc>> &group );    //Used to determine retreat
int combat_score( const std::vector< monster * > &group );
void attack_random( const std::vector<std::shared_ptr<npc>> &attacker,
                    const std::vector<std::shared_ptr<npc>> &defender );
void attack_random( const std::vector< monster * > &group,
                    const std::vector<std::shared_ptr<npc>> &defender );
void attack_random( const std::vector<std::shared_ptr<npc>> &attacker,
                    const std::vector< monster * > &group );
std::shared_ptr<npc> temp_npc( const string_id<npc_template> &type );

//Utility functions
/// Returns npcs that have the given companion mission.
std::vector<std::shared_ptr<npc>> companion_list( const npc &p, const std::string &id,
                               bool contains = false );
std::vector<std::shared_ptr<npc>> companion_sort( std::vector<std::shared_ptr<npc>> available,
                               const std::string &skill_tested = "" );
std::vector<comp_rank> companion_rank( const std::vector<std::shared_ptr<npc>> &available,
                                       bool adj = true );
std::shared_ptr<npc> companion_choose( const std::string &skill_tested = "", int skill_level = 0 );
std::shared_ptr<npc> companion_choose_return( const npc &p, const std::string &id,
        const time_point &deadline );
//Return NPC to your party
void companion_return( npc &comp );
//Smash stuff, steal valuables, and change map maker
std::vector<item *> loot_building( const tripoint &site );
}
#endif
