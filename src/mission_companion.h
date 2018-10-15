#pragma once
#ifndef MISSION_COMPANION_H
#define MISSION_COMPANION_H

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
struct comp_rank;
class player;
class npc_template;
template<typename T>
class string_id;


struct mission_entry {
    std::string id;
    std::string name_display;
    std::string dir;
    bool priority;
    bool possible;
};

class mission_data
{
    public:
        //see mission_key_push() for each key description
        std::vector<std::vector<mission_entry>> entries;
        std::map<std::string, std::string> text;
        mission_data();
        /**
        * Adds the id's to the correct vectors (ie tabs) in the UI.
        * @param id is the mission reference
        * @param name_display is string displayed
        * @param dir is the direction of the expansion from the central camp, ie "[N]"
        * @param priority turns the mission key yellow and pushes to front of main tab
        * @param possible grays the mission key when false
        */
        void push( const std::string &id, const std::string &name_display = "",
                   const std::string &dir = "", bool priority = false, bool possible = true );

};

namespace talk_function
{
/*mission_companion.cpp proves a set of functions that compress all the typical mission operations into a set of hard-coded
 *unique missions that don't fit well into the framework of the existing system.  These missions typically focus on
 *sending companions out on simulated adventures or tasks.  This is not meant to be a replacement for the existing system.
 */
//Identifies which mission set the NPC draws from
void companion_mission( npc & );
//Primary Loop
bool outpost_missions( mission_data &mission_keys, npc &p, const std::string &id, const std::string &title );
/**
 * Send a companion on an individual mission or attaches them to a group to depart later
 * Set @ref submap_coords and @ref pos.
 * @param desc is the description in the popup window
 * @param id is the value stored with the NPC when it is offloaded
 * @param group is whether the NPC is waiting for additional members before departing together
 * @param equipment is placed in the NPC's special inventory and dropped when they return
 * @param skill_tests is the main skill for the quest
 * @param skill_level is checked to prevent lower level NPCs from going on missions
 */
///Send a companion on an individual mission or attaches them to a group to depart later
npc *individual_mission( npc &p, const std::string &desc, const std::string &id, bool group = false,
                         const std::vector<item *> &equipment = {}, const std::string &skill_tested = "",
                         int skill_level = 0 );
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
};
#endif
