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
struct comp_rank;
struct mission_entry;
class player;
class npc_template;
template<typename T>
class string_id;

namespace talk_function
{

enum camp_tab_mode {
    TAB_MAIN,
    TAB_N,
    TAB_NE,
    TAB_E,
    TAB_SE,
    TAB_S,
    TAB_SW,
    TAB_W,
    TAB_NW
};

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
bool outpost_missions( npc &p, const std::string &id, const std::string &title );
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
                         std::vector<item *> equipment = {}, std::string skill_tested = "", int skill_level = 0 );
///Display items listed in @ref equipment to let the player pick what to give the departing NPC, loops until quit or empty.
std::vector<item *> individual_mission_give_equipment( std::vector<item *> equipment,
        std::string message = _( "Do you wish to give your companion additional items?" ) );
/**
 * Adds the id's to the correct vectors (ie tabs) in the UI.
 * @param id is string displayed
 * @param dir is the direction of the expansion from the central camp "[N]"
 * @param priority turns the mission key yellow and pushes to front of main tab
 * @param possible grays the mission key when false
 */
void mission_key_push( std::vector<std::vector<mission_entry>> &mission_key_vectors, std::string id,
                       std::string name_display = "", std::string dir = "", bool priority = false, bool possible = true );

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

///Changes an NPC follower to a camp manager, displays camp warnings, and sets the current OM tile to a camp survey
void become_overseer( npc & );
///Changes an NPC camp manager to a follower
void remove_overseer( npc & );
/**
 * Counts or destroys and drops the bash items of all furniture that matches @ref furn_id f in the map tile
 * @param omt_tgt, the targeted OM tile
 * @param furn_id, furniture you are looking for
 * @param chance of destruction, 0 to 1.00
 * @param force_bash, whether you want to destroy the furniture and drop the items vs counting the furniture
 */
int om_harvest_furn( npc &comp, point omt_tgt, furn_id f, float chance = 1.0,
                     bool force_bash = true );
/// Exact same as om_harvest_furn but functions on terrain
int om_harvest_ter( npc &comp, point omt_tgt, ter_id f, float chance = 1.0,
                    bool force_bash = true );
/// Collects all items in @ref omt_tgt with a @ref chance between 0 - 1.0, @ref take, whether you take the item or count it
int om_harvest_itm( npc &comp, point omt_tgt, float chance = 1.0, bool take = true );
/// Counts or cuts trees into trunks and trunks into logs, if both are false it returns the total of the two combined
int om_harvest_trees( npc &comp, tripoint omt_tgt, float chance = 1.0, bool force_cut = true,
                      bool force_cut_trunk = true );
/// Creates an improvised shelter at @ref omt_tgt and dumps the @ref itms into the building
bool om_set_hide_site( npc &comp, tripoint omt_tgt, std::vector<item *> itms,
                       std::vector<item *> itms_rem = {} );
/**
 * Opens the overmap so that you can select points for missions or constructions.
 * @param omt_pos, where your camp is, used for calculating travel distances
 * @param min_range
 * @param range, max number of OM tiles the user can select
 * @param possible_om_types, requires the user to reselect if the OM picked isn't in the list
 * @param must_see, whether the user can select points in the unknown/fog of war
 * @param popup_notice, toggles if the user should be shown ranges before being allowed to pick
 * @param source, if you are selecting multiple points this is where the OM is centered to start
 */
tripoint om_target_tile( tripoint omt_pos, int min_range = 1, int range = 1,
                         std::vector<std::string> possible_om_types = {},
                         bool must_see = true, bool popup_notice = true, tripoint source = tripoint( -999, -999, -999 ),
                         bool bounce = false );
void om_range_mark( tripoint origin, int range, bool add_notes = true,
                    std::string message = "Y;X: MAX RANGE" );
void om_line_mark( tripoint origin, tripoint dest, bool add_notes = true,
                   std::string message = "R;X: PATH" );
std::vector<tripoint> om_companion_path( tripoint start, int range = 90, bool bounce = true );
/**
 * Can be used to calculate total trip time for an NPC mission or just the traveling portion.  Doesn't use the pathing
 * algorithms yet.
 * @param omt_pos, start point
 * @param omt_tgt, target point
 * @param work, how much time the NPC will stay at the target
 * @param trips, how many trips back and forth the NPC will make
 */
time_duration companion_travel_time_calc( tripoint omt_pos, tripoint omt_tgt, time_duration work,
        int trips = 1 );
time_duration companion_travel_time_calc( std::vector<tripoint> journey, time_duration work,
        int trips = 1 );
/// Formats the variables into a standard looking description to be displayed in a ynquery window
std::string camp_trip_description( time_duration total_time, time_duration working_time,
                                   time_duration travel_time,
                                   int distance, int trips, int need_food );
/// Determines how many trips a given NPC @ref comp will take to move all of the items @ref itms
int om_carry_weight_to_trips( npc &comp, std::vector<item *> itms );

/// Improve the camp tile to the next level and pushes the camp manager onto his correct position in case he moved
bool om_camp_upgrade( npc &p, const point omt_pos );
/// Returns the description for the recipe of the next building @ref bldg
std::string om_upgrade_description( std::string bldg );
/// Currently does the same as om_upgrade_description but should convert fire charges to raw charcoal needed and allow dark craft
std::string om_craft_description( std::string bldg );
/// Provides a "guess" for some of the things your gatherers will return with to upgrade the camp
std::string om_gathering_description( npc &p, std::string bldg );
/// Returns the name of the building the current @ref bldg upgrades into, "null" if there isn't one
std::string om_next_upgrade( std::string bldg );
/// Returns a vector of all upgrades the current build would have if it reached bldg level, "null" if there isn't one
std::vector<std::string> om_all_upgrade_levels( std::string bldg );
/// Is the @ref bldg of the same type as the @ref target and does the level of bldg meet or exceed the target level
bool om_min_level( std::string target, std::string bldg );
/// Is the @ref bldg of the same type as the @ref target and how many levels greater is it, -1 for no match, 0 for same
int om_over_level( std::string target, std::string bldg );
/// Called to close upgrade missions, @ref miss is the name of the mission id and @ref omt_pos is location to be upgraded
bool upgrade_return( npc &p, point omt_pos, std::string miss );
/// Popups to explain what is going on for anyone who is unsure, called only on the first camp designation for a character
void faction_camp_tutorial();
/// Called when a companion completes a gathering @ref task mission
bool camp_gathering_return( npc &p, std::string task );
/// Called on completion of recruiting, returns the new NPC.
void camp_recruit_return( npc &p, std::string task, int score );
void camp_craft_construction( npc &p, mission_entry cur_key,
                              std::map<std::string, std::string> recipes, std::string miss_id,
                              tripoint omt_pos, std::vector<std::pair<std::string, tripoint>> om_expansions );
std::string camp_recruit_evaluation( npc &p, std::string base,
                                     std::vector<std::pair<std::string, tripoint>> om_expansions,
                                     bool raw_score = false );
/// Called when a companion completes a chop shop @ref task mission
bool camp_garage_chop_start( npc &p, std::string task );
/**
 * Perform any mix of the three farm tasks.
 * @param harvest, should the NPC harvest every harvestable plant
 * @param plant, NPC will keep planting until they are out of dirt mounds or seeds in mission inventory
 * @param plow, references the farm json and plows any dirt or grass tiles that are where dirt mounds should be
 */
bool camp_farm_return( npc &p, std::string task, bool harvest, bool plant, bool plow );
/// Sorts all items within most of the confines of the camp into piles designated by the player or defaulted to
bool camp_menial_return( npc &p );
/**
 * Sets the location of the sorting piles used above.
 * @param reset_pts, reverts all previous points to defaults.  Called/checked so we can add new point with compatability
 * @param choose_pts, let the player flip through all of the points and set the ones they want
 */
bool camp_menial_sort_pts( npc &p, bool reset_pts = true, bool choose_pts = false );
/// Choose which expansion you should start, called when a survey mission is completed
bool camp_expansion_select( npc &p );
/// Takes all the food from the point set in camp_menial_sort_pts() and increases the faction food_supply
bool camp_distribute_food( npc &p );
/// Returns the OM tiles surrounding the camp, @ref purge removes all tiles that aren't expansions
std::vector<std::pair<std::string, tripoint>> om_building_region( npc &p, int range,
        bool purge = false );
/// Converts the camp and expansion points into direction strings, "[NW]"
std::string om_simple_dir( point omt_pos, tripoint omt_tar );
/// Returns a string for the number of plants that are harvestable, plots ready to plany, and ground that needs tilling
std::string camp_farm_description( point omt_pos, bool harvest = true, bool plots = true,
                                   bool plow = true );
/// Returns a string for display of the selected car so you don't chop shop the wrong one
std::string camp_car_description( vehicle *car );
/// Takes a mission line and gets the camp's direction from it "[NW]"
std::string camp_direction( std::string line );
/// Changes the faction food supply by @ref change, 0 returns total food supply, a negative total food supply hurts morale
int camp_food_supply( int change = 0, bool return_days = false );
/// Same as above but takes a time_duration and consumes from faction food supply for that duration of work
int camp_food_supply( time_duration work );
/// Returns the total charges of food time_duration @ref work costs
int time_to_food( time_duration work );
/// Changes the faction respect for you by @ref change, returns repect
int camp_discipline( int change = 0 );
/// Changes the faction opinion for you by @ref change, returns opinion
int camp_morale( int change = 0 );

void draw_camp_tabs( const catacurses::window &win, camp_tab_mode cur_tab,
                     std::vector<std::vector<mission_entry>> &mission_key_vectors );
std::string name_mission_tabs( npc &p, std::string id, std::string cur_title,
                               camp_tab_mode cur_tab );

/// Creats a map of all the recipes that are available to a building at om_cur, "ALL" for all possible
std::map<std::string, std::string> camp_recipe_deck( std::string om_cur );
/// Determines what the absolute max (out of 9999) that can be crafted using inventory and food supplies
int camp_recipe_batch_max( const recipe making, inventory total_inv );
/// Trains NPC @ref comp, in skill_tested for duration time_worked at difficulty 1, several groups of skills can also be input
int companion_skill_trainer( npc &comp, std::string skill_tested = "",
                             time_duration time_worked = 1_hours, int difficulty = 1 );
int companion_skill_trainer( npc &comp, skill_id skill_tested, time_duration time_worked = 1_hours,
                             int difficulty = 1 );

//Combat functions
bool companion_om_combat_check( std::vector<std::shared_ptr<npc>> group, tripoint om_tgt,
                                bool try_engage = false );
void force_on_force( std::vector<std::shared_ptr<npc>> defender, const std::string &def_desc,
                     std::vector<std::shared_ptr<npc>> attacker, const std::string &att_desc, int advantage );
bool force_on_force( std::vector<std::shared_ptr<npc>> defender, const std::string &def_desc,
                     std::vector< monster * > monsters_fighting, const std::string &att_desc, int advantage );
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
std::vector<npc *> companion_sort( std::vector<npc *> available, std::string skill_tested = "" );
std::vector<comp_rank> companion_rank( std::vector<npc *> available, bool adj = true );
npc *companion_choose( std::string skill_tested = "", int skill_level = 0 );
npc *companion_choose_return( npc &p, const std::string &id, const time_point &deadline );
void companion_return( npc &comp );               //Return NPC to your party
std::vector<item *> loot_building( const tripoint
                                   site ); //Smash stuff, steal valuables, and change map maker
};

void unload_talk_topics();
void load_talk_topic( JsonObject &jo );

#endif
