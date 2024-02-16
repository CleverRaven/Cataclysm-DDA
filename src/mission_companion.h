#pragma once
#ifndef CATA_SRC_MISSION_COMPANION_H
#define CATA_SRC_MISSION_COMPANION_H

#include <iosfwd>
#include <map>
#include <new>
#include <optional>
#include <string>
#include <vector>

#include "calendar.h"
#include "coordinates.h"
#include "mapgendata.h"
#include "memory_fast.h"
#include "point.h"
#include "type_id.h"

class item;
class monster;
class npc;
class npc_template;
struct comp_rank;

using npc_ptr = shared_ptr_fast<npc>;
using comp_list = std::vector<npc_ptr>;

//  The different missions that are supported by the code. New missions have to get a new value entered to
//  this enum (and added as appropriate in all places the enum is used). Note that there are currently
//  two missions that take parameters based on (ultimately) JSON data, namely Camp_Crafting, that takes
//  a recipe from a recipe group added as a "provides" in faction camp hubs/expansions, and Camp_Upgrade,
//  that take a blueprint provided by a hub/expansion construction recipe.
//  Camp_Survey_Expansion allows base camps to expand into surrounding tiles based on JSON definitions,
//  but this selection is done directly as part of the finalization of the mission, rather than provided
//  as a goal at the outset, and so only counts partially.
//
enum mission_kind : int {
    No_Mission,  //  Null value

    //  Jobs assigned to companions for external parties
    Scavenging_Patrol_Job,
    Scavenging_Raid_Job,
    Hospital_Raid_Job,
    Menial_Job,
    Carpentry_Job,
    Forage_Job,
    Caravan_Commune_Center_Job,

    //  Faction camp tasks
    Camp_Distribute_Food,        //  Direct action, not serialized
    Camp_Determine_Leadership,   //  Direct action, not serialized
    Camp_Have_Meal,              //  Direct action, not serialized
    Camp_Hide_Mission,           //  Direct action, not serialized
    Camp_Reveal_Mission,         //  Direct action, not serialized
    Camp_Assign_Jobs,
    Camp_Assign_Workers,
    Camp_Abandon,
    Camp_Upgrade,
    Camp_Emergency_Recall,
    Camp_Crafting,
    Camp_Gather_Materials,
    Camp_Collect_Firewood,
    Camp_Menial,
    Camp_Survey_Field,
    Camp_Survey_Expansion,
    Camp_Cut_Logs,
    Camp_Clearcut,
    Camp_Setup_Hide_Site,
    Camp_Relay_Hide_Site,
    Camp_Foraging,
    Camp_Trapping,
    Camp_Hunting,
    Camp_OM_Fortifications,
    Camp_Recruiting,
    Camp_Scouting,
    Camp_Combat_Patrol,
    Camp_Chop_Shop,  //  Obsolete removed during 0.E
    Camp_Plow,
    Camp_Plant,
    Camp_Harvest,

    last_mission_kind
};

template<>
struct enum_traits<mission_kind> {
    static constexpr mission_kind last = mission_kind::last_mission_kind;
};

//  Operation to get translated UI strings for the corresponding misison
std::string action_of( mission_kind kind );

//  id is the identifier for the mission.
//  parameters contains any additional modifiers, such as recipes or blueprints.
//  dir indicates the hub/expansion the mission refers to if a base camp mission
//  and not set for non base camp missions.
//  The struct is used in companions (npc) to keep track of active missions, as
//  well as in internal structures used to populate the UI with available,
//  ongoing, and unavailable missions.
//  It can be noted that only npcs serialize/deserialize mission_id, while the
//  other usages are internal and not saved when the game is saved.
//
struct mission_id {
    mission_kind id = No_Mission;
    std::string parameters;
    mapgen_arguments mapgen_args;
    std::optional<point> dir;

    void serialize( JsonOut & ) const;
    void deserialize( const JsonValue & );
};

bool is_equal( const mission_id &first, const mission_id &second );
void reset_miss_id( mission_id &miss_id );

//  ret determines whether the mission is for the start of a mission or the return of the companion(s).
//  Used in the UI mission generation process to distinguish active missions from available ones.
struct ui_mission_id {
    mission_id id;
    bool ret;
};

bool is_equal( const ui_mission_id &first, const ui_mission_id &second );

struct mission_entry {
    ui_mission_id id;
    std::string name_display;
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
        * @param priority turns the mission key yellow and pushes to front of main tab
        * @param possible grays the mission key when false and makes it impossible to select
        */
        void add( const ui_mission_id &id, const std::string &name_display = "",
                  const std::string &text = "",
                  bool priority = false, bool possible = true );
        void add_return( const mission_id &id, const std::string &name_display,
                         const std::string &text, bool possible = true );
        void add_start( const mission_id &id, const std::string &name_display,
                        const std::string &text, bool possible = true );
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
bool display_and_choose_opts( mission_data &mission_key, const tripoint_abs_omt &omt_pos,
                              const std::string &role_id, const std::string &title );

/**
 * Send a companion on an individual mission or attaches them to a group to depart later
 * Set @ref submap_coords and @ref pos.
 * @param desc is the description in the popup window
 * @param id is the value stored with the NPC when it is offloaded
 * @param group is whether the NPC is waiting for additional members before departing together
 * @param equipment is placed in the NPC's special inventory and dropped when they return
 * @param silent_failure is used to add an additional companion to a mission.
 */
///Send a companion on an individual mission or attaches them to a group to depart later
npc_ptr individual_mission( npc &p, const std::string &desc, const mission_id &miss_id,
                            bool group = false, const std::vector<item *> &equipment = {},
                            const std::map<skill_id, int> &required_skills = {}, bool silent_failure = false );
npc_ptr individual_mission( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                            const std::string &desc, const mission_id &miss_id,
                            bool group = false, const std::vector<item *> &equipment = {},
                            const std::map<skill_id, int> &required_skills = {}, bool silent_failure = false );

///All of these missions are associated with the ranch camp and need to be updated/merged into the new ones
void caravan_return( npc &p, const std::string &dest, const mission_id &miss_id );
void caravan_depart( npc &p, const std::string &dest, const mission_id &miss_id );
int caravan_dist( const std::string &dest );

void field_plant( npc &p, const std::string &place );
void field_harvest( npc &p, const std::string &place );
bool scavenging_patrol_return( npc &p );
bool scavenging_raid_return( npc &p );
bool hospital_raid_return( npc &p );
bool labor_return( npc &p );
bool carpenter_return( npc &p );
bool forage_return( npc &p );

/// Trains NPC @ref comp, in skill_tested for duration time_worked at difficulty 1, several groups of skills can also be input
void companion_skill_trainer( npc &comp, const std::string &skill_tested = "",
                              time_duration time_worked = 1_hours, int difficulty = 1 );
void companion_skill_trainer( npc &comp, const skill_id &skill_tested,
                              time_duration time_worked = 1_hours, int difficulty = 1 );
//Combat functions
bool companion_om_combat_check( const comp_list &group, const tripoint_abs_omt &om_tgt,
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
comp_list companion_list( const npc &p, const mission_id &miss_id, bool contains = false );
comp_list companion_list( const tripoint &omt_pos, const std::string &role_id,
                          const mission_id &miss_id, bool contains = false );
comp_list companion_sort( comp_list available,
                          const std::map<skill_id, int> &required_skills = {} );
std::vector<comp_rank> companion_rank( const comp_list &available, bool adj = true );
// silent_failure is used when an additional companion is requested to modify the query and just return
// without a popup when no companion is found/selected.
npc_ptr companion_choose( const std::map<skill_id, int> &required_skills = {}, bool silent_failure =
                              false );
npc_ptr companion_choose_return( const npc &p, const mission_id &miss_id,
                                 const time_point &deadline, bool ignore_parameters = false );
npc_ptr companion_choose_return( const tripoint_abs_omt &omt_pos, const std::string &role_id,
                                 const mission_id &miss_id, const time_point &deadline,
                                 bool by_mission = true, bool ignore_parameters = false );
npc_ptr companion_choose_return( comp_list &npc_list );

//Return NPC to your party
void companion_return( npc &comp );
//Smash stuff, steal valuables, and change map marker
std::set<item> loot_building( const tripoint_abs_omt &site, const oter_str_id &looted_replacement );

} // namespace talk_function
#endif // CATA_SRC_MISSION_COMPANION_H
