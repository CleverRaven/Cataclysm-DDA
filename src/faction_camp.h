#pragma once
#ifndef FACTION_CAMP_H
#define FACTION_CAMP_H

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

namespace talk_function
{

///Changes an NPC follower to a camp manager
void become_overseer( npc & );
///Changes an NPC follower to a camp manager, displays camp warnings, and sets the current OM tile to a camp survey
void start_camp( npc & );
///Changes an NPC follower to a camp manager of an existing camp.
void recover_camp( npc & );
///Changes an NPC camp manager to a follower
void remove_overseer( npc & );

/**
 * Counts or destroys and drops the bash items of all furniture that matches @ref furn_id f in the map tile
 * @param omt_tgt, the targeted OM tile
 * @param furn_id, furniture you are looking for
 * @param chance of destruction, 0 to 1.00
 * @param force_bash, whether you want to destroy the furniture and drop the items vs counting the furniture
 */
int om_harvest_furn( npc &comp, const point &omt_tgt, const furn_id &f, float chance = 1.0,
                     bool force_bash = true );
/// Exact same as om_harvest_furn but functions on terrain
int om_harvest_ter( npc &comp, const point &omt_tgt, const ter_id &f, float chance = 1.0,
                    bool force_bash = true );
/// Collects all items in @ref omt_tgt with a @ref chance between 0 - 1.0, @ref take, whether you take the item or count it
int om_harvest_itm( npc &comp, const point &omt_tgt, float chance = 1.0, bool take = true );
/// Counts or cuts trees into trunks and trunks into logs, if both are false it returns the total of the two combined
int om_harvest_trees( npc &comp, const tripoint &omt_tgt, float chance = 1.0, bool force_cut = true,
                      bool force_cut_trunk = true );
/// Creates an improvised shelter at @ref omt_tgt and dumps the @ref itms into the building
bool om_set_hide_site( npc &comp, const tripoint &omt_tgt, const std::vector<item *> &itms,
                       const std::vector<item *> &itms_rem = {} );
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
tripoint om_target_tile( const tripoint &omt_pos, int min_range = 1, int range = 1,
                         const std::vector<std::string> &possible_om_types = {},
                         bool must_see = true, bool popup_notice = true, const tripoint &source = tripoint( -999, -999,
                                 -999 ),
                         bool bounce = false );
void om_range_mark( const tripoint &origin, int range, bool add_notes = true,
                    const std::string &message = "Y;X: MAX RANGE" );
void om_line_mark( const tripoint &origin, const tripoint &dest, bool add_notes = true,
                   const std::string &message = "R;X: PATH" );
std::vector<tripoint> om_companion_path( const tripoint &start, int range = 90,
        bool bounce = true );
/**
 * Can be used to calculate total trip time for an NPC mission or just the traveling portion.  Doesn't use the pathing
 * algorithms yet.
 * @param omt_pos, start point
 * @param omt_tgt, target point
 * @param work, how much time the NPC will stay at the target
 * @param trips, how many trips back and forth the NPC will make
 */
time_duration companion_travel_time_calc( const tripoint &omt_pos, const tripoint &omt_tgt,
        time_duration work,
        int trips = 1 );
time_duration companion_travel_time_calc( const std::vector<tripoint> &journey, time_duration work,
        int trips = 1 );
/// Formats the variables into a standard looking description to be displayed in a ynquery window
std::string camp_trip_description( time_duration total_time, time_duration working_time,
                                   time_duration travel_time,
                                   int distance, int trips, int need_food );
/// Determines how many trips a given NPC @ref comp will take to move all of the items @ref itms
int om_carry_weight_to_trips( npc &comp, const std::vector<item *> &itms );

/// Improve the camp tile to the next level and pushes the camp manager onto his correct position in case he moved
bool om_camp_upgrade( npc &p, const point &omt_pos );
/// Returns the description for the recipe of the next building @ref bldg
std::string om_upgrade_description( const std::string &bldg );
/// Currently does the same as om_upgrade_description but should convert fire charges to raw charcoal needed and allow dark craft
std::string om_craft_description( const std::string &bldg );
/// Provides a "guess" for some of the things your gatherers will return with to upgrade the camp
std::string om_gathering_description( npc &p, const std::string &bldg );
/// Returns the name of the building the current @ref bldg upgrades into, "null" if there isn't one
std::string om_next_upgrade( const std::string &bldg );
/// Returns a vector of all upgrades the current build would have if it reached bldg level, "null" if there isn't one
std::vector<std::string> om_all_upgrade_levels( const std::string &bldg );
/// Is the @ref bldg of the same type as the @ref target and does the level of bldg meet or exceed the target level
bool om_min_level( const std::string &target, const std::string &bldg );
/// Is the @ref bldg of the same type as the @ref target and how many levels greater is it, -1 for no match, 0 for same
int om_over_level( const std::string &target, const std::string &bldg );
/// Called to close upgrade missions, @ref miss is the name of the mission id and @ref omt_pos is location to be upgraded
bool upgrade_return( npc &p, const point &omt_pos, const std::string &miss );
/// Called when a companion completes a gathering @ref task mission
bool camp_gathering_return( npc &p, const std::string &task );
/// Called on completion of recruiting, returns the new NPC.
void camp_recruit_return( npc &p, const std::string &task, int score );
void camp_craft_construction( npc &p, const mission_entry &cur_key,
                              const std::map<std::string, std::string> &recipes, const std::string &miss_id,
                              const tripoint &omt_pos, const std::vector<std::pair<std::string, tripoint>> &om_expansions );
std::string camp_recruit_evaluation( npc &p, const std::string &base,
                                     const std::vector<std::pair<std::string, tripoint>> &om_expansions,
                                     bool raw_score = false );
/// Called when a companion completes a chop shop @ref task mission
bool camp_garage_chop_start( npc &p, const std::string &task );
/**
 * Perform any mix of the three farm tasks.
 * @param harvest, should the NPC harvest every harvestable plant
 * @param plant, NPC will keep planting until they are out of dirt mounds or seeds in mission inventory
 * @param plow, references the farm json and plows any dirt or grass tiles that are where dirt mounds should be
 */
bool camp_farm_return( npc &p, const std::string &task, bool harvest, bool plant, bool plow );
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
std::string om_simple_dir( const point &omt_pos, const tripoint &omt_tar );
/// Returns a string for the number of plants that are harvestable, plots ready to plany, and ground that needs tilling
std::string camp_farm_description( const point &omt_pos, bool harvest = true, bool plots = true,
                                   bool plow = true );
/// Returns a string for display of the selected car so you don't chop shop the wrong one
std::string camp_car_description( vehicle *car );
/// Takes a mission line and gets the camp's direction from it "[NW]"
std::string camp_direction( const std::string &line );
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
std::string name_mission_tabs( npc &p, const std::string &id, const std::string &cur_title,
                               camp_tab_mode cur_tab );

/// Creats a map of all the recipes that are available to a building at om_cur, "ALL" for all possible
std::map<std::string, std::string> camp_recipe_deck( const std::string &om_cur );
/// Determines what the absolute max (out of 9999) that can be crafted using inventory and food supplies
int camp_recipe_batch_max( const recipe making, const inventory &total_inv );
};
#endif
