#pragma once
#ifndef FACTION_CAMP_H
#define FACTION_CAMP_H

#include <string>
#include <vector>

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

enum class farm_ops {
    plow = 1,
    plant = 2,
    harvest = 4
};
inline bool operator&( const farm_ops &rhs, const farm_ops &lhs )
{
    return static_cast<int>( rhs ) & static_cast<int>( lhs );
}

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
 * Counts or destroys and drops the bash items of all furniture that matches @ref f in the map tile
 * @param comp NPC companion
 * @param omt_tgt the targeted OM tile
 * @param f furniture you are looking for
 * @param chance chance of destruction, 0 to 100
 * @param estimate if true, non-destructive count of the furniture
 * @param bring_back force the destruction of the furniture and bring back the drop items
 */
int om_harvest_furn( npc &comp, const tripoint &omt_tgt, const furn_id &f, int chance = 100,
                     bool estimate = false, bool bring_back = true );
// om_harvest_furn helper function that counts the furniture instances
int om_harvest_furn_est( npc &comp, const tripoint &omt_tgt, const furn_id &f, int chance = 100 );
int om_harvest_furn_break( npc &comp, const tripoint &omt_tgt, const furn_id &f, int chance = 100 );
/// Exact same as om_harvest_furn but functions on terrain
int om_harvest_ter( npc &comp, const tripoint &omt_tgt, const ter_id &t, int chance = 100,
                    bool estimate = false, bool bring_back = true );
// om_harvest_furn helper function that counts the furniture instances
int om_harvest_ter_est( npc &comp, const tripoint &omt_tgt, const ter_id &t, int chance = 100 );
int om_harvest_ter_break( npc &comp, const tripoint &omt_tgt, const ter_id &t, int chance = 100 );
/// Collects all items in @ref omt_tgt with a @ref chance between 0 - 1.0, returns total mass and volume
/// @ref take, whether you take the item or count it
std::pair<units::mass, units::volume> om_harvest_itm( std::shared_ptr<npc> comp,
        const tripoint &omt_tgt, int chance = 100, bool take = true );
/*
 * Counts or cuts trees into trunks and trunks into logs
 * @param omt_tgt the targeted OM tile
 * @param chance chance of destruction, 0 to 100
 * @param estimate if true, non-destructive count of trees
 * @force_cut_trunk if true and estimate is false, chop tree trunks into logs
 */
int om_cutdown_trees( const tripoint &omt_tgt, int chance = 100, bool estimate = false,
                      bool force_cut_trunk = true );
int om_cutdown_trees_est( const tripoint &omt_tgt, int chance = 100 );
int om_cutdown_trees_logs( const tripoint &omt_tgt, int chance = 100 );
int om_cutdown_trees_trunks( const tripoint &omt_tgt, int chance = 100 );

/// Creates an improvised shelter at @ref omt_tgt and dumps the @ref itms into the building
bool om_set_hide_site( npc &comp, const tripoint &omt_tgt, const std::vector<item *> &itms,
                       const std::vector<item *> &itms_rem = {} );
/**
 * Opens the overmap so that you can select points for missions or constructions.
 * @param omt_pos where your camp is, used for calculating travel distances
 * @param min_range
 * @param range max number of OM tiles the user can select
 * @param possible_om_types requires the user to reselect if the OM picked isn't in the list
 * @param must_see whether the user can select points in the unknown/fog of war
 * @param popup_notice toggles if the user should be shown ranges before being allowed to pick
 * @param source if you are selecting multiple points this is where the OM is centered to start
 * @param bounce
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
 * @param omt_pos start point
 * @param omt_tgt target point
 * @param work how much time the NPC will stay at the target
 * @param trips how many trips back and forth the NPC will make
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
/// Determines how many round trips a given NPC @ref comp will take to move all of the items @ref itms
int om_carry_weight_to_trips( const std::vector<item *> &itms,
                              std::shared_ptr<npc> comp = nullptr );
/// Determines how many trips it takes to move @ref mass and @ref volume of items with @ref carry_mass and @ref carry_volume moved per trip
int om_carry_weight_to_trips( units::mass mass, units::volume volume, units::mass carry_mass,
                              units::volume carry_volume );

/// Returns the description for the recipe of the next building @ref bldg
std::string om_upgrade_description( const std::string &bldg );
/// Returns the description of a camp crafting options. converts fire charges to charcoal, allows dark crafting
std::string om_craft_description( const std::string &bldg );
/// Provides a "guess" for some of the things your gatherers will return with to upgrade the camp
std::string om_gathering_description( npc &p, const std::string &bldg );
/// Called when a companion completes a gathering @ref task mission
bool camp_gathering_return( npc &p, const std::string &task, time_duration min_time );
void camp_recruit_return( npc &p, const std::string &task, int score );
/// Called when a companion is sent to cut logs
void start_camp_upgrade( npc &p, const std::string &bldg, const std::string &key );
void start_cut_logs( npc &p );
void start_clearcut( npc &p );
void start_setup_hide_site( npc &p );
void start_relay_hide_site( npc &p );
/// Called when a compansion is sent to start fortifications
void start_fortifications( std::string &bldg_exp, npc &p );
void start_combat_mission( const std::string &miss, npc &p );

/// Called when a companion completes a chop shop @ref task mission
bool camp_garage_chop_start( npc &p, const std::string dir, const tripoint &omt_tgt );
void camp_farm_start( npc &p, const std::string &dir, const tripoint &omt_tgt, farm_ops op );

/**
 * spawn items or corpses based on search attempts
 * @param skill skill level of the search
 * @param group_id name of the item_group that provides the items
 * @param attempts number of skill checks to make
 * @param difficulty a random number from 0 to difficulty is created for each attempt, and if skill is higher, an item or corpse is spawned
 */
void camp_search_results( int skill, const Group_tag &group_id, int attempts, int difficulty );
/**
 * spawn items or corpses based on search attempts
 * @param skill skill level of the search
 * @param task string to identify what types of corpses to provide ( _faction_camp_hunting or _faction_camp_trapping )
 * @param attempts number of skill checks to make
 * @param difficulty a random number from 0 to difficulty is created for each attempt, and if skill is higher, an item or corpse is spawned
 */
void camp_hunting_results( int skill, const std::string &task, int attempts, int difficulty );

/// Called when a companion completes any mission and calls companion_return
void camp_companion_return( npc &comp );
/**
 * Perform any mix of the three farm tasks.
 * @param p NPC companion
 * @param task string to identify what types of corpses to provide ( _faction_camp_hunting or _faction_camp_trapping )
 * @param harvest should the NPC harvest every harvestable plant
 * @param plant NPC will keep planting until they are out of dirt mounds or seeds in mission inventory
 * @param plow references the farm json and plows any dirt or grass tiles that are where dirt mounds should be
 */
bool camp_farm_return( npc &p, const std::string &task, const tripoint &omt_trg, farm_ops op );
void camp_fortifications_return( npc &p );
void combat_mission_return( const std::string &miss, npc &p );
/// Returns the OM tiles surrounding the camp, @ref purge removes all tiles that aren't expansions
std::vector<std::pair<std::string, tripoint>> om_building_region( npc &p, int range,
        bool purge = false );
/// Converts the camp and expansion points into direction strings, "[NW]"
std::string om_simple_dir( const tripoint &omt_pos, const tripoint &omt_tar );
/// Converts a direction into a point offset
point om_dir_to_offset( const std::string &dir );
/// Returns a string for the number of plants that are harvestable, plots ready to plany, and ground that needs tilling
std::string camp_farm_description( const tripoint &omt_pos, size_t &plots, farm_ops operation );
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
int camp_recipe_batch_max( const recipe &making, const inventory &total_inv );

/*
 * check if a companion survives a random encounter
 * @param comp the companion
 * @param situation what the survivor is doing
 * @param favor a number added to the survivor's skills to see if he can avoid the encounter
 * @param threat a number indicating how dangerous the encounter is
 * TODO: Convert to JSON basic on dynamic line type structure
 */
bool survive_random_encounter( npc &comp, std::string &situation, int favor, int threat );

}
#endif
