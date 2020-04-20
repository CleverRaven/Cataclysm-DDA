#pragma once
#ifndef CATA_SRC_MISSION_COMPANION_H
#define CATA_SRC_MISSION_COMPANION_H

#include <map>
#include <string>
#include <vector>

#include "calendar.h"
#include "memory_fast.h"
#include "optional.h"
#include "point.h"
#include "type_id.h"

class item;
class monster;
class npc;
class npc_template;
struct comp_rank;
template<typename T>
class string_id;

using npc_ptr = shared_ptr_fast<npc>;
using comp_list = std::vector<npc_ptr>;

struct mission_entry {
    std::string id;
    std::string name_display;
    cata::optional<point> dir;
    std::string text;
    bool priority = false;
    bool possible = false;
};

class mission_data
{
    public:
        std::vector<std::vector<mission_entry>> entries;
        mission_entry cur_key;

        mission_data();
        /**
        * Adds the id's to the correct vectors (i.e. tabs) in the UI.
        * @param id is the mission reference
        * @param name_display is string displayed
        * @param dir is the direction of the expansion from the central camp, i.e. "[N]"
        * @param priority turns the mission key yellow and pushes to front of main tab
        * @param possible grays the mission key when false and makes it impossible to select
        */
        void add( const std::string &id, const std::string &name_display,
                  const cata::optional<point> &dir, const std::string &text,
                  bool priority = false, bool possible = true );
        void add_return( const std::string &id, const std::string &name_display,
                         const cata::optional<point> &dir, const std::string &text, bool possible = true );
        void add_start( const std::string &id, const std::string &name_display,
                        const cata::optional<point> &dir, const std::string &text, bool possible = true );
        void add( const std::string &id, const std::string &name_display = "",
                  const std::string &text = "" );
};

namespace talk_function
{
/*
 * mission_companion.cpp proves a set of functions that compress all the typical mission
 * operations into a set of hard-coded unique missions that don't fit well into the framework of
 * the existing system.  These missions typically focus on sending companions out on simulated
 * adventures or tasks.  This is not meant to be a replacement for the existing system.
 */

//Identifies which mission set the NPC draws from
void companion_mission( npc &p );

// Display the available missions and let the player choose one
bool display_and_choose_opts( mission_data &mission_key, const tripoint &omt_pos,
                              const std::string &role_id, const std::string &title );

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
npc_ptr individual_mission( npc &p, const std::string &desc, const std::string &miss_id,
                            bool group = false, const std::vector<item *> &equipment = {},
                            const std::map<skill_id, int> &required_skills = {} );
npc_ptr individual_mission( const tripoint &omt_pos, const std::string &role_id,
                            const std::string &desc, const std::string &miss_id,
                            bool group = false, const std::vector<item *> &equipment = {},
                            const std::map<skill_id, int> &required_skills = {} );

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
bool companion_om_combat_check( const comp_list &group, const tripoint &om_tgt,
                                bool try_engage = false );
void force_on_force( const comp_list &defender, const std::string &def_desc,
                     const comp_list &attacker, const std::string &att_desc, int advantage );
bool force_on_force( const comp_list &defender, const std::string &def_desc,
                     const std::vector< monster * > &monsters_fighting,
                     const std::string &att_desc, int advantage );
int combat_score( const comp_list &group );    //Used to determine retreat
int combat_score( const std::vector< monster * > &group );
void attack_random( const comp_list &attacker, const comp_list &defender );
void attack_random( const std::vector< monster * > &group, const comp_list &defender );
void attack_random( const comp_list &attacker, const std::vector< monster * > &group );
npc_ptr temp_npc( const string_id<npc_template> &type );

//Utility functions
/// Returns npcs that have the given companion mission.
comp_list companion_list( const npc &p, const std::string &mission_id, bool contains = false );
comp_list companion_list( const tripoint &omt_pos, const std::string &role_id,
                          const std::string &mission_id, bool contains = false );
comp_list companion_sort( comp_list available,
                          const std::map<skill_id, int> &required_skills = {} );
std::vector<comp_rank> companion_rank( const comp_list &available, bool adj = true );
npc_ptr companion_choose( const std::map<skill_id, int> &required_skills = {} );
npc_ptr companion_choose_return( const npc &p, const std::string &mission_id,
                                 const time_point &deadline );
npc_ptr companion_choose_return( const tripoint &omt_pos, const std::string &role_id,
                                 const std::string &mission_id, const time_point &deadline,
                                 bool by_mission = true );

//Return NPC to your party
void companion_return( npc &comp );
//Smash stuff, steal valuables, and change map maker
// TODO: Make this return the loot gained
void loot_building( const tripoint &site );

} // namespace talk_function
#endif // CATA_SRC_MISSION_COMPANION_H
