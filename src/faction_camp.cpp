#include "faction_camp.h" // IWYU pragma: associated

#include <cstddef>
#include <algorithm>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <memory>
#include <set>
#include <unordered_set>

#include "activity_handlers.h"
#include "avatar.h"
#include "bionics.h"
#include "catacharset.h"
#include "clzones.h"
#include "compatibility.h" // needed for the workaround for the std::to_string bug in some compilers
#include "coordinate_conversions.h"
#include "debug.h"
#include "editmap.h"
#include "game.h"
#include "iexamine.h"
#include "input.h"
#include "item_group.h"
#include "itype.h"
#include "line.h"
#include "kill_tracker.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "mapgen_functions.h"
#include "messages.h"
#include "mission.h"
#include "mission_companion.h"
#include "npc.h"
#include "npctalk.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "recipe.h"
#include "recipe_groups.h"
#include "requirements.h"
#include "rng.h"
#include "skill.h"
#include "string_input_popup.h"
#include "translations.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_range.h"
#include "basecamp.h"
#include "calendar.h"
#include "color.h"
#include "cursesdef.h"
#include "enums.h"
#include "faction.h"
#include "game_constants.h"
#include "int_id.h"
#include "inventory.h"
#include "item.h"
#include "optional.h"
#include "pimpl.h"
#include "player_activity.h"
#include "string_formatter.h"
#include "string_id.h"
#include "ui.h"
#include "units.h"
#include "weighted_list.h"
#include "type_id.h"
#include "colony.h"
#include "item_stack.h"
#include "point.h"
#include "vpart_position.h"
#include "weather.h"

static const skill_id skill_dodge( "dodge" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_unarmed( "unarmed" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_bashing( "bashing" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_mechanics( "mechanics" );
static const skill_id skill_speech( "speech" );
static const skill_id skill_traps( "traps" );
static const skill_id skill_swimming( "swimming" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );

static const zone_type_id z_camp_storage( "CAMP_STORAGE" );
static const zone_type_id z_camp_food( "CAMP_FOOD" );

struct mass_volume {
    units::mass wgt;
    units::volume vol;
    int count;
};

namespace base_camps
{
// eventually this will include the start and return functions
struct miss_data {
    std::string miss_id;
    translation desc;
    translation action;
    std::string ret_miss_id;
    translation ret_desc;
};

recipe_id select_camp_option( const std::map<recipe_id, translation> &pos_options,
                              const std::string &option );

// enventually this will move to JSON
std::map<std::string, miss_data> miss_info = {{
        {
            "_faction_upgrade_camp", {
                "Upgrade Camp", to_translation( "Upgrade camp" ),
                to_translation( "Working to expand your camp!\n" ),
                "Recover Ally from Upgrading", to_translation( "Recover Ally from Upgrading" )
            }
        },
        {
            "_faction_camp_recall", {
                "Emergency Recall", to_translation( "Emergency Recall" ),
                to_translation( "Lost in the ether!\n" ),
                "Emergency Recall", to_translation( "Emergency Recall" )
            }
        },
        {
            "_faction_camp_crafting_", {
                "Craft Item", to_translation( "Craft Item" ),
                to_translation( "Busy crafting!\n" ),
                " (Finish) Crafting", to_translation( " (Finish) Crafting" )
            }
        },
        {
            "travelling", {
                "Travelling", to_translation( "Travelling" ),
                to_translation( "Busy travelling!\n" ),
                "Recall ally from travelling", to_translation( "Recall ally from travelling" )
            }
        },
        {
            "_faction_camp_gathering", {
                "Gather Materials", to_translation( "Gather Materials" ),
                to_translation( "Searching for materials to upgrade the camp.\n" ),
                "Recover Ally from Gathering", to_translation( "Recover Ally from Gathering" )
            }
        },
        {
            "_faction_camp_firewood", {
                "Collect Firewood", to_translation( "Collect Firewood" ),
                to_translation( "Searching for firewood.\n" ),
                "Recover Firewood Gatherers", to_translation( "Recover Firewood Gatherers" )
            }
        },
        {
            "_faction_camp_menial", {
                "Menial Labor", to_translation( "Menial Labor" ),
                to_translation( "Performing menial labor…\n" ),
                "Recover Menial Laborer", to_translation( "Recover Menial Laborer" )
            }
        },
        {
            "_faction_camp_expansion", {
                "Expand Base", to_translation( "Expand Base" ),
                to_translation( "Surveying for expansion…\n" ),
                "Recover Surveyor", to_translation( "Recover Surveyor" )
            }
        },
        {
            "_faction_camp_cut_log", {
                "Cut Logs", to_translation( "Cut Logs" ),
                to_translation( "Cutting logs in the woods…\n" ),
                "Recover Log Cutter", to_translation( "Recover Log Cutter" )
            }
        },
        {
            "_faction_camp_clearcut", {
                "Clearcut", to_translation( "Clear a forest" ),
                to_translation( "Clearing a forest…\n" ),
                "Recover Clearcutter", to_translation( "Recover Clearcutter" )
            }
        },
        {
            "_faction_camp_hide_site", {
                "Setup Hide Site", to_translation( "Setup Hide Site" ),
                to_translation( "Setting up a hide site…\n" ),
                "Recover Hide Setup", to_translation( "Recover Hide Setup" )
            }
        },
        {
            "_faction_camp_hide_trans", {
                "Relay Hide Site", to_translation( "Relay Hide Site" ),
                to_translation( "Transferring gear to a hide site…\n" ),
                "Recover Hide Relay", to_translation( "Recover Hide Relay" )
            }
        },
        {
            "_faction_camp_foraging", {
                "Camp Forage", to_translation( "Forage for plants" ),
                to_translation( "Foraging for edible plants.\n" ),
                "Recover Foragers", to_translation( "Recover Foragers" )
            }
        },
        {
            "_faction_camp_trapping", {
                "Trap Small Game", to_translation( "Trap Small Game" ),
                to_translation( "Trapping Small Game.\n" ),
                "Recover Trappers", to_translation( "Recover Trappers" )
            }
        },
        {
            "_faction_camp_hunting", {
                "Hunt Large Animals", to_translation( "Hunt Large Animals" ),
                to_translation( "Hunting large animals.\n" ),
                "Recover Hunter", to_translation( "Recover Hunter" )
            }
        },
        {
            "_faction_camp_om_fortifications", {
                "Construct Map Fort", to_translation( "Construct Map Fortifications" ),
                to_translation( "Constructing fortifications…\n" ),
                "Finish Map Fort", to_translation( "Finish Map Fortifications" )
            }
        },
        {
            "_faction_camp_recruit_0", {
                "Recruit Companions", to_translation( "Recruit Companions" ),
                to_translation( "Searching for recruits.\n" ),
                "Recover Recruiter", to_translation( "Recover Recruiter" )
            }
        },
        {
            "_faction_camp_scout_0", {
                "Scout Mission", to_translation( "Scout Mission" ),
                to_translation( "Scouting the region.\n" ),
                "Recover Scout", to_translation( "Recover Scout" )
            }
        },
        {
            "_faction_camp_combat_0", {
                "Combat Patrol", to_translation( "Combat Patrol" ),
                to_translation( "Patrolling the region.\n" ),
                "Recover Combat Patrol", to_translation( "Recover Combat Patrol" )
            }
        },
        {
            "_faction_upgrade_exp_", {
                " Expansion Upgrade", to_translation( " Expansion Upgrade" ),
                to_translation( "Working to upgrade your expansions!\n" ),
                "Recover Ally", to_translation( "Recover Ally" )
            }
        },
        {
            "_faction_exp_chop_shop_", {
                " Chop Shop", to_translation( " Chop Shop" ),
                to_translation( "Working at the chop shop…\n" ),
                " (Finish) Chop Shop", to_translation( " (Finish) Chop Shop" )
            }
        },
        {
            "_faction_exp_kitchen_cooking_", {
                " Kitchen Cooking", to_translation( " Kitchen Cooking" ),
                to_translation( "Working in your kitchen!\n" ),
                " (Finish) Cooking", to_translation( " (Finish) Cooking" )
            }
        },
        {
            "_faction_exp_blacksmith_crafting_", {
                " Blacksmithing", to_translation( " Blacksmithing" ),
                to_translation( "Working in your blacksmith shop!\n" ),
                " (Finish) Smithing", to_translation( " (Finish) Smithing" )
            }
        },
        {
            "_faction_exp_plow_", {
                " Plow Fields", to_translation( " Plow Fields" ),
                to_translation( "Working to plow your fields!\n" ),
                " (Finish) Plow Fields", to_translation( " (Finish) Plow fields" )
            }
        },
        {
            "_faction_exp_plant_", {
                " Plant Fields", to_translation( " Plant Fields" ),
                to_translation( "Working to plant your fields!\n" ),
                " (Finish) Plant Fields", to_translation( " (Finish) Plant Fields" )
            }
        },
        {
            "_faction_exp_harvest_", {
                " Harvest Fields", to_translation( " Harvest Fields" ),
                to_translation( "Working to harvest your fields!\n" ),
                " (Finish) Harvest Fields", to_translation( " (Finish) Harvest Fields" )
            }
        },
        {
            "_faction_exp_farm_crafting_", {
                " Farm Crafting", to_translation( " Farm Crafting" ),
                to_translation( "Working on your farm!\n" ),
                " (Finish) Crafting", to_translation( " (Finish) Crafting" )
            }
        }
    }
};
} // namespace base_camps

/**** Forward declaration of utility functions */
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
int om_harvest_furn_break( npc &comp, const tripoint &omt_tgt, const furn_id &f,
                           int chance = 100 );
/// Exact same as om_harvest_furn but functions on terrain
int om_harvest_ter( npc &comp, const tripoint &omt_tgt, const ter_id &t, int chance = 100,
                    bool estimate = false, bool bring_back = true );
// om_harvest_furn helper function that counts the furniture instances
int om_harvest_ter_est( npc &comp, const tripoint &omt_tgt, const ter_id &t, int chance = 100 );
int om_harvest_ter_break( npc &comp, const tripoint &omt_tgt, const ter_id &t, int chance = 100 );
/// Collects all items in @ref omt_tgt with a @ref chance between 0 - 1.0, returns total
/// mass and volume
/// @ref take, whether you take the item or count it
mass_volume om_harvest_itm( npc_ptr comp, const tripoint &omt_tgt, int chance = 100,
                            bool take = true );
void apply_camp_ownership( const tripoint &camp_pos, int radius );
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
                         bool must_see = true, bool popup_notice = true,
                         const tripoint &source = tripoint( -999, -999, -999 ),
                         bool bounce = false );
void om_range_mark( const tripoint &origin, int range, bool add_notes = true,
                    const std::string &message = "Y;X: MAX RANGE" );
void om_line_mark( const tripoint &origin, const tripoint &dest, bool add_notes = true,
                   const std::string &message = "R;X: PATH" );
std::vector<tripoint> om_companion_path( const tripoint &start, int range_start = 90,
        bool bounce = true );
/**
 * Can be used to calculate total trip time for an NPC mission or just the traveling portion.
 * Doesn't use the pathingalgorithms yet.
 * @param omt_pos start point
 * @param omt_tgt target point
 * @param work how much time the NPC will stay at the target
 * @param trips how many trips back and forth the NPC will make
 */
time_duration companion_travel_time_calc( const tripoint &omt_pos, const tripoint &omt_tgt,
        time_duration work, int trips = 1, int haulage = 0 );
time_duration companion_travel_time_calc( const std::vector<tripoint> &journey, time_duration work,
        int trips = 1, int haulage = 0 );
/// Determines how many round trips a given NPC @ref comp will take to move all of the
/// items @ref itms
int om_carry_weight_to_trips( const std::vector<item *> &itms, npc_ptr comp = nullptr );
/// Determines how many trips it takes to move @ref mass and @ref volume of items
/// with @ref carry_mass and @ref carry_volume moved per trip
int om_carry_weight_to_trips( units::mass mass, units::volume volume, units::mass carry_mass,
                              units::volume carry_volume );
/// Formats the variables into a standard looking description to be displayed in a ynquery window
std::string camp_trip_description( const time_duration &total_time,
                                   const time_duration &working_time,
                                   const time_duration &travel_time,
                                   int distance, int trips, int need_food );

/// Returns a string for display of the selected car so you don't chop shop the wrong one
std::string camp_car_description( vehicle *car );
std::string camp_farm_act( const tripoint &omt_pos, size_t &plots_count, farm_ops operation );

/// Changes the faction food supply by @ref change, 0 returns total food supply, a negative
/// total food supply hurts morale
int camp_food_supply( int change = 0, bool return_days = false );
/// Same as above but takes a time_duration and consumes from faction food supply for that
/// duration of work
int camp_food_supply( time_duration work );
/// Returns the total charges of food time_duration @ref work costs
int time_to_food( time_duration work );
/// Changes the faction respect for you by @ref change, returns repect
int camp_discipline( int change = 0 );
/// Changes the faction opinion for you by @ref change, returns opinion
int camp_morale( int change = 0 );
/*
 * check if a companion survives a random encounter
 * @param comp the companion
 * @param situation what the survivor is doing
 * @param favor a number added to the survivor's skills to see if he can avoid the encounter
 * @param threat a number indicating how dangerous the encounter is
 * TODO: Convert to JSON basic on dynamic line type structure
 */
bool survive_random_encounter( npc &comp, std::string &situation, int favor, int threat );

static bool update_time_left( std::string &entry, const comp_list &npc_list )
{
    bool avail = false;
    for( auto &comp : npc_list ) {
        if( comp->companion_mission_time_ret < calendar:: turn ) {
            entry = entry +  _( " [DONE]\n" );
            avail = true;
        } else {
            entry = entry + " [" +
                    to_string( comp->companion_mission_time_ret - calendar::turn ) +
                    _( " left]\n" );
            avail = g->u.has_trait( trait_DEBUG_HS );
        }
    }
    entry = entry + _( "\n\nDo you wish to bring your allies back into your party?" );
    return avail;
}

static bool update_time_fixed( std::string &entry, const comp_list &npc_list,
                               const time_duration &duration )
{
    bool avail = false;
    for( auto &comp : npc_list ) {
        time_duration elapsed = calendar::turn - comp->companion_mission_time;
        entry = entry + " " +  comp->name + " [" + to_string( elapsed ) + "/" +
                to_string( duration ) + "]\n";
        avail |= elapsed >= duration;
    }
    entry = entry + _( "\n\nDo you wish to bring your allies back into your party?" );
    return avail;
}

static cata::optional<basecamp *> get_basecamp( npc &p, const std::string &camp_type = "default" )
{

    tripoint omt_pos = p.global_omt_location();
    cata::optional<basecamp *> bcp = overmap_buffer.find_camp( omt_pos.xy() );
    if( bcp ) {
        return bcp;
    }
    g->m.add_camp( omt_pos, "faction_camp" );
    bcp = overmap_buffer.find_camp( omt_pos.xy() );
    if( !bcp ) {
        return cata::nullopt;
    }
    basecamp *temp_camp = *bcp;
    temp_camp->define_camp( omt_pos, camp_type );
    return temp_camp;
}

recipe_id base_camps::select_camp_option( const std::map<recipe_id, translation> &pos_options,
        const std::string &option )
{
    std::vector<recipe_id> pos_name_ids;
    std::vector<std::string> pos_names;
    for( const auto &it : pos_options ) {
        pos_names.push_back( it.second.translated() );
        pos_name_ids.push_back( it.first );
    }

    if( pos_name_ids.size() == 1 ) {
        return pos_name_ids.front();
    }

    const int choice = uilist( _( option ), pos_names );
    if( choice < 0 || static_cast<size_t>( choice ) >= pos_name_ids.size() ) {
        popup( _( "You choose to wait…" ) );
        return recipe_id::NULL_ID();
    }
    return pos_name_ids[choice];
}

void talk_function::start_camp( npc &p )
{
    const tripoint omt_pos = p.global_omt_location();
    const oter_id &omt_ref = overmap_buffer.ter( omt_pos );

    const auto &pos_camps = recipe_group::get_recipes_by_id( "all_faction_base_types",
                            omt_ref.id().c_str() );
    if( pos_camps.empty() ) {
        popup( _( "You cannot build a camp here." ) );
        return;
    }
    const recipe_id camp_type = base_camps::select_camp_option( pos_camps,
                                _( "Select a camp type:" ) );
    if( !camp_type ) {
        return;
    }

    std::vector<std::pair<std::string, tripoint>> om_region = om_building_region( omt_pos, 1 );
    int near_fields = 0;
    for( const auto &om_near : om_region ) {
        const oter_id &om_type = oter_id( om_near.first );
        if( is_ot_match( "field", om_type, ot_match_type::contains ) ) {
            near_fields += 1;
        }
    }
    std::vector<std::pair<std::string, tripoint>> om_region_ext = om_building_region( omt_pos, 3 );
    int forests = 0;
    int waters = 0;
    for( const auto &om_near : om_region_ext ) {
        const oter_id &om_type = oter_id( om_near.first );
        if( is_ot_match( "faction_base", om_type, ot_match_type::contains ) ) {
            popup( _( "You are too close to another camp!" ) );
            return;
        } else if( is_ot_match( "forest", om_type, ot_match_type::contains ) ) {
            forests++;
        } else if( is_ot_match( "river", om_type, ot_match_type::contains ) ) {
            waters++;
        }
    }
    bool display = false;
    std::string buffer = _( "Warning, you have selected a region with the following issues:\n\n" );
    if( forests < 3 ) {
        display = true;
        buffer += _( "There are few forests.  Wood is your primary construction material.\n" );
    }
    if( waters == 0 ) {
        display = true;
        buffer += _( "There are few large clean-ish water sources.\n" );
    }
    if( near_fields < 4 ) {
        display = true;
        buffer += _( "There are few fields.  Farming may be difficult.\n" );
    }
    if( display && !query_yn( _( "%s\nAre you sure you wish to continue?" ), buffer ) ) {
        return;
    }
    const recipe &making = camp_type.obj();
    if( !run_mapgen_update_func( making.get_blueprint(), omt_pos ) ) {
        popup( _( "%s failed to start the %s basecamp, perhaps there is a vehicle in the way." ),
               p.disp_name(), making.get_blueprint() );
        return;
    }
    get_basecamp( p, camp_type.str() );
}

void talk_function::recover_camp( npc &p )
{
    const tripoint omt_pos = p.global_omt_location();
    const std::string &omt_ref = overmap_buffer.ter( omt_pos ).id().c_str();
    if( omt_ref.find( "faction_base_camp" ) == std::string::npos ) {
        popup( _( "There is no faction camp here to recover!" ) );
        return;
    }
    get_basecamp( p );
}

void talk_function::remove_overseer( npc &p )
{
    size_t suffix = p.name.find( _( ", Camp Manager" ) );
    if( suffix != std::string::npos ) {
        p.name = p.name.substr( 0, suffix );
    }

    add_msg( _( "%s has abandoned the camp." ), p.name );
    p.companion_mission_role_id.clear();
    stop_guard( p );
}

void talk_function::basecamp_mission( npc &p )
{
    const std::string title = _( "Base Missions" );
    const tripoint omt_pos = p.global_omt_location();
    mission_data mission_key;

    cata::optional<basecamp *> temp_camp = get_basecamp( p );
    if( !temp_camp ) {
        return;
    }
    basecamp *bcp = *temp_camp;
    bcp->set_by_radio( g->u.dialogue_by_radio );
    if( bcp->get_dumping_spot() == tripoint_zero ) {
        auto &mgr = zone_manager::get_manager();
        if( g->m.check_vehicle_zones( g->get_levz() ) ) {
            mgr.cache_vzones();
        }
        tripoint src_loc;
        const auto abspos = p.global_square_location();
        if( mgr.has_near( z_camp_storage, abspos, 60 ) ) {
            const auto &src_set = mgr.get_near( z_camp_storage, abspos );
            const auto &src_sorted = get_sorted_tiles_by_distance( abspos, src_set );
            // Find the nearest unsorted zone to dump objects at
            for( auto &src : src_sorted ) {
                src_loc = g->m.getlocal( src );
                break;
            }
        }
        bcp->set_dumping_spot( g->m.getabs( src_loc ) );
    }
    bcp->get_available_missions( mission_key );
    if( display_and_choose_opts( mission_key, omt_pos, base_camps::id, title ) ) {
        bcp->handle_mission( mission_key.cur_key.id, mission_key.cur_key.dir );
    }
}

void basecamp::add_available_recipes( mission_data &mission_key, const point &dir,
                                      const std::map<recipe_id, translation> &craft_recipes )
{
    const std::string dir_id = base_camps::all_directions.at( dir ).id;
    const std::string dir_abbr = base_camps::all_directions.at( dir ).bracket_abbr.translated();
    for( const auto &recipe_data : craft_recipes ) {
        const std::string id = dir_id + recipe_data.first.str();
        const std::string &title_e = dir_abbr + recipe_data.second;
        const std::string &entry = craft_description( recipe_data.first );
        const recipe &recp = recipe_data.first.obj();
        bool craftable = recp.deduped_requirements().can_make_with_inventory(
                             _inv, recp.get_component_filter() );
        mission_key.add_start( id, title_e, dir, entry, craftable );
    }
}

void basecamp::get_available_missions_by_dir( mission_data &mission_key, const point &dir )
{
    std::string entry;
    std::string gather_bldg = "null";

    const std::string dir_id = base_camps::all_directions.at( dir ).id;
    const std::string dir_abbr = base_camps::all_directions.at( dir ).bracket_abbr.translated();
    const tripoint omt_trg = omt_pos + dir;

    if( dir != base_camps::base_dir ) {
        // return legacy workers
        comp_list npc_list = get_mission_workers( "_faction_upgrade_exp_" + dir_id );
        if( !npc_list.empty() ) {
            const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_upgrade_exp_" ];
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( "Recover Ally, " + dir_id + " Expansion",
                                    _( "Recover Ally, " ) + dir_abbr + _( " Expansion" ), dir,
                                    entry, avail );
        }
        // Generate upgrade missions for expansions
        for( const basecamp_upgrade &upgrade : available_upgrades( dir ) ) {
            const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_upgrade_exp_" ];
            comp_list npc_list = get_mission_workers( upgrade.bldg + "_faction_upgrade_exp_" +
                                 dir_id );
            if( npc_list.empty() ) {
                entry = om_upgrade_description( upgrade.bldg );
                mission_key.add_start( dir_id + miss_info.miss_id + upgrade.bldg,
                                       dir_abbr + miss_info.desc + " " + upgrade.name, dir, entry,
                                       upgrade.avail );
            } else {
                entry = miss_info.action.translated();
                bool avail = update_time_left( entry, npc_list );
                mission_key.add_return( "Recover Ally, " + dir_id + " Expansion" + upgrade.bldg,
                                        _( "Recover Ally, " ) + dir_abbr + _( " Expansion" ) +
                                        " " + upgrade.name, dir, entry, avail );
            }
        }
    }

    if( has_provides( "gathering", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_gathering" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_gathering" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to gather materials for the next camp "
                                  "upgrade.\n\n"
                                  "Skill used: survival\n"
                                  "Difficulty: N/A\n"
                                  "Gathering Possibilities:\n"
                                  "%s\n"
                                  "Risk: Very Low\n"
                                  "Time: 3 Hours, Repeated\n"
                                  "Positions: %d/3\n" ), gathering_description( gather_bldg ),
                               npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), dir,
                               entry, npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_fixed( entry, npc_list, 3_hours );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    dir, entry, avail );
        }
    }
    if( has_provides( "firewood", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_firewood" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_firewood" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to gather light brush and heavy sticks.\n\n"
                                  "Skill used: survival\n"
                                  "Difficulty: N/A\n"
                                  "Gathering Possibilities:\n"
                                  "> heavy sticks\n"
                                  "> withered plants\n"
                                  "> splintered wood\n\n"
                                  "Risk: Very Low\n"
                                  "Time: 3 Hours, Repeated\n"
                                  "Positions: %d/3\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), cata::nullopt,
                               entry, npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_fixed( entry, npc_list, 3_hours );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }
    if( has_provides( "sorting", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_menial" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_menial" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to do low level chores and sort "
                                  "supplies.\n\n"
                                  "Skill used: fabrication\n"
                                  "Difficulty: N/A\n"
                                  "Effects:\n"
                                  "> Material left in the unsorted loot zone will be sorted "
                                  "into a defined loot zone."
                                  "\n\nRisk: None\n"
                                  "Time: 3 Hours\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), cata::nullopt,
                               entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }

    if( has_provides( "logging", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_cut_log" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_cut_log" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to a nearby forest to cut logs.\n\n"
                                  "Skill used: fabrication\n"
                                  "Difficulty: 1\n"
                                  "Effects:\n"
                                  "> 50%% of trees/trunks at the forest position will be "
                                  "cut down.\n"
                                  "> 100%% of total material will be brought back.\n"
                                  "> Repeatable with diminishing returns.\n"
                                  "> Will eventually turn forests into fields.\n"
                                  "Risk: None\n"
                                  "Time: 6 Hour Base + Travel Time + Cutting Time\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), cata::nullopt,
                               entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }

    if( has_provides( "logging", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_clearcut" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_clearcut" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to a clear a nearby forest.\n\n"
                                  "Skill used: fabrication\n"
                                  "Difficulty: 1\n"
                                  "Effects:\n"
                                  "> 95%% of trees/trunks at the forest position"
                                  " will be cut down.\n"
                                  "> 0%% of total material will be brought back.\n"
                                  "> Forest should become a field tile.\n"
                                  "> Useful for clearing land for another faction camp.\n\n"
                                  "Risk: None\n"
                                  "Time: 6 Hour Base + Travel Time + Cutting Time\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), cata::nullopt,
                               entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }

    if( has_provides( "relaying", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_hide_site" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_hide_site" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to build an improvised shelter and stock it "
                                  "with equipment at a distant map location.\n\n"
                                  "Skill used: survival\n"
                                  "Difficulty: 3\n"
                                  "Effects:\n"
                                  "> Good for setting up resupply or contingency points.\n"
                                  "> Gear is left unattended and could be stolen.\n"
                                  "> Time dependent on weight of equipment being sent forward.\n\n"
                                  "Risk: Medium\n"
                                  "Time: 6 Hour Construction + Travel\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), cata::nullopt,
                               entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }

    if( has_provides( "relaying", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_hide_trans" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_hide_trans" ];
        entry = string_format( _( "Notes:\n"
                                  "Push gear out to a hide site or bring gear back from one.\n\n"
                                  "Skill used: survival\n"
                                  "Difficulty: 1\n"
                                  "Effects:\n"
                                  "> Good for returning equipment you left in the hide site "
                                  "shelter.\n"
                                  "> Gear is left unattended and could be stolen.\n"
                                  "> Time dependent on weight of equipment being sent forward or "
                                  "back.\n\n"
                                  "Risk: Medium\n"
                                  "Time: 1 Hour Base + Travel\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), cata::nullopt, entry,
                               npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }

    if( has_provides( "foraging", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_foraging" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_foraging" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to forage for edible plants.\n\n"
                                  "Skill used: survival\n"
                                  "Difficulty: N/A\n"
                                  "Foraging Possibilities:\n"
                                  "> wild vegetables\n"
                                  "> fruits and nuts depending on season\n"
                                  "May produce less food than consumed!\n"
                                  "Risk: Very Low\n"
                                  "Time: 4 Hours, Repeated\n"
                                  "Positions: %d/3\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), dir,
                               entry, npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_fixed( entry, npc_list, 4_hours );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    dir, entry, avail );
        }
    }

    if( has_provides( "trapping", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_trapping" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_trapping" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to set traps for small game.\n\n"
                                  "Skill used: trapping\n"
                                  "Difficulty: N/A\n"
                                  "Trapping Possibilities:\n"
                                  "> small and tiny animal corpses\n"
                                  "May produce less food than consumed!\n"
                                  "Risk: Low\n"
                                  "Time: 6 Hours, Repeated\n"
                                  "Positions: %d/2\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), dir,
                               entry, npc_list.size() < 2 );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_fixed( entry, npc_list, 6_hours );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    dir, entry, avail );
        }
    }

    if( has_provides( "hunting", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_hunting" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_hunting" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to hunt large animals.\n\n"
                                  "Skill used: marksmanship\n"
                                  "Difficulty: N/A\n"
                                  "Hunting Possibilities:\n"
                                  "> small, medium, or large animal corpses\n"
                                  "May produce less food than consumed!\n"
                                  "Risk: Medium\n"
                                  "Time: 6 Hours, Repeated\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), dir,
                               entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_fixed( entry, npc_list, 6_hours );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    dir, entry, avail );
        }
    }

    if( has_provides( "walls", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_om_fortifications" );
        const base_camps::miss_data &miss_info =
            base_camps::miss_info[ "_faction_camp_om_fortifications" ];
        entry = om_upgrade_description( "faction_wall_level_N_0" );
        mission_key.add_start( "Construct Map Fort", _( "Construct Map Fortifications" ),
                               dir, entry, npc_list.empty() );
        entry = om_upgrade_description( "faction_wall_level_N_1" );
        mission_key.add_start( "Construct Trench", _( "Construct Spiked Trench" ), dir, entry,
                               npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }

    if( has_provides( "recruiting", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_recruit_0" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_recruit_0" ];
        entry = recruit_description( npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), dir, entry,
                               npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }

    if( has_provides( "scouting", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_scout_0" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_scout_0" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion out into the great unknown.  High survival "
                                  "skills are needed to avoid combat but you should expect an "
                                  "encounter or two.\n\n"
                                  "Skill used: survival\n"
                                  "Difficulty: 3\n"
                                  "Effects:\n"
                                  "> Select checkpoints to customize path.\n"
                                  "> Reveals terrain around the path.\n"
                                  "> Can bounce off hide sites to extend range.\n\n"
                                  "Risk: High\n"
                                  "Time: Travel\n"
                                  "Positions: %d/3\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), dir,
                               entry, npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id,  miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }

    if( has_provides( "patrolling", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_combat_0" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_combat_0" ];
        entry = string_format( _( "Notes:\n"
                                  "Send a companion to purge the wasteland.  Their goal is to "
                                  "kill anything hostile they encounter and return when "
                                  "their wounds are too great or the odds are stacked against "
                                  "them.\n\n"
                                  "Skill used: survival\n"
                                  "Difficulty: 4\n"
                                  "Effects:\n"
                                  "> Pulls creatures encountered into combat instead of "
                                  "fleeing.\n"
                                  "> Select checkpoints to customize path.\n"
                                  "> Can bounce off hide sites to extend range.\n\n"
                                  "Risk: Very High\n"
                                  "Time: Travel\n"
                                  "Positions: %d/3\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(), cata::nullopt,
                               entry, npc_list.size() < 3 );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    cata::nullopt, entry, avail );
        }
    }

    if( has_provides( "dismantling", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_exp_chop_shop_" + dir_id );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_exp_chop_shop_" ];
        entry = _( "Notes:\n"
                   "Have a companion attempt to completely dissemble a vehicle into "
                   "components.\n\n"
                   "Skill used: mechanics\n"
                   "Difficulty: 2\n"
                   "Effects:\n"
                   "> Removed parts placed on the furniture in the garage.\n"
                   "> Skill plays a huge role to determine what is salvaged.\n\n"
                   "Risk: None\n"
                   "Time: 5 days\n" );
        mission_key.add_start( dir_id + miss_info.miss_id, dir_abbr + miss_info.desc, dir, entry,
                               npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( dir_id + miss_info.ret_miss_id,
                                    dir_abbr + miss_info.ret_desc, dir, entry, avail );
        }
    }

    std::map<recipe_id, translation> craft_recipes = recipe_deck( dir );
    if( has_provides( "kitchen", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_exp_kitchen_cooking_" + dir_id );
        const base_camps::miss_data &miss_info =
            base_camps::miss_info[ "_faction_exp_kitchen_cooking_" ];
        if( npc_list.empty() ) {
            add_available_recipes( mission_key, dir, craft_recipes );
        } else {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( dir_id + miss_info.ret_miss_id,
                                    dir_abbr + miss_info.ret_desc, dir, entry, avail );
        }
    }

    if( has_provides( "blacksmith", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_exp_blacksmith_crafting_" + dir_id );
        const base_camps::miss_data &miss_info =
            base_camps::miss_info[ "_faction_exp_blacksmith_crafting_" ];
        if( npc_list.empty() ) {
            add_available_recipes( mission_key, dir, craft_recipes );
        } else {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( dir_id + miss_info.ret_miss_id,
                                    dir_abbr + miss_info.ret_desc, dir, entry, avail );
        }
    }

    if( has_provides( "farming", dir ) ) {
        size_t plots = 0;
        comp_list npc_list = get_mission_workers( "_faction_exp_plow_" + dir_id );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_exp_plow_" ];
        if( npc_list.empty() ) {
            entry = _( "Notes:\n"
                       "Plow any spaces that have reverted to dirt or grass.\n\n" ) +
                    farm_description( omt_trg, plots, farm_ops::plow ) +
                    _( "\n\n"
                       "Skill used: fabrication\n"
                       "Difficulty: N/A\n"
                       "Effects:\n"
                       "> Restores only the plots created in the last expansion upgrade.\n"
                       "> Does not damage existing crops.\n\n"
                       "Risk: None\n"
                       "Time: 5 Min / Plot\n"
                       "Positions: 0/1\n" );
            mission_key.add_start( dir_id + miss_info.miss_id, dir_abbr + miss_info.desc, dir,
                                   entry, plots > 0 );
        } else {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( dir_id + miss_info.ret_miss_id,
                                    dir_abbr + miss_info.ret_desc, dir, entry, avail );
        }
    }
    if( has_provides( "farming", dir ) ) {
        size_t plots = 0;
        comp_list npc_list = get_mission_workers( "_faction_exp_plant_" + dir_id );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_exp_plant_" ];
        if( npc_list.empty() ) {
            entry = _( "Notes:\n"
                       "Plant designated seeds in the spaces that have already been "
                       "tilled.\n\n" ) +
                    farm_description( omt_trg, plots, farm_ops::plant ) +
                    _( "\n\n"
                       "Skill used: survival\n"
                       "Difficulty: N/A\n"
                       "Effects:\n"
                       "> Choose which seed type or all of your seeds.\n"
                       "> Stops when out of seeds or planting locations.\n"
                       "> Will plant in ALL dirt mounds in the expansion.\n\n"
                       "Risk: None\n"
                       "Time: 1 Min / Plot\n"
                       "Positions: 0/1\n" );
            mission_key.add_start( dir_id + miss_info.miss_id,
                                   dir_abbr + miss_info.desc, dir, entry,
                                   plots > 0 && warm_enough_to_plant( omt_trg ) );
        } else {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( dir_id + miss_info.ret_miss_id,
                                    dir_abbr + miss_info.ret_desc, dir, entry, avail );
        }
    }
    if( has_provides( "farming", dir ) ) {
        size_t plots = 0;
        comp_list npc_list = get_mission_workers( "_faction_exp_harvest_" + dir_id );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_exp_harvest_" ];
        if( npc_list.empty() ) {
            entry = _( "Notes:\n"
                       "Harvest any plants that are ripe and bring the produce back.\n\n" ) +
                    farm_description( omt_trg, plots, farm_ops::harvest ) +
                    _( "\n\n"
                       "Skill used: survival\n"
                       "Difficulty: N/A\n"
                       "Effects:\n"
                       "> Will dump all harvesting products onto your location.\n\n"
                       "Risk: None\n"
                       "Time: 3 Min / Plot\n"
                       "Positions: 0/1\n" );
            mission_key.add_start( dir_id + miss_info.miss_id,
                                   dir_abbr + miss_info.desc, dir, entry,
                                   plots > 0 );
        } else {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( dir_id + miss_info.ret_miss_id,
                                    dir_abbr + miss_info.ret_desc, dir, entry, avail );
        }
    }

    if( has_provides( "reseeding", dir ) ) {
        comp_list npc_list = get_mission_workers( "_faction_exp_farm_crafting_" + dir_id );
        const base_camps::miss_data &miss_info =
            base_camps::miss_info[ "_faction_exp_farm_crafting_" ];
        if( npc_list.empty() ) {
            add_available_recipes( mission_key, dir, craft_recipes );
        } else {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( dir_id + miss_info.ret_miss_id,
                                    dir_abbr + miss_info.ret_desc, dir, entry, avail );
        }
    }
}

void basecamp::get_available_missions( mission_data &mission_key )
{
    std::string entry;

    const point &base_dir = base_camps::base_dir;
    const base_camps::direction_data &base_data = base_camps::all_directions.at( base_dir );
    const std::string base_dir_id = base_data.id;
    const std::string base_dir_abbr = base_data.bracket_abbr.translated();
    reset_camp_resources();
    std::string gather_bldg = "null";

    // Handling for the central tile
    // return legacy workers
    comp_list legacy_npc_list = get_mission_workers( "_faction_upgrade_camp" );
    if( !legacy_npc_list.empty() ) {
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_upgrade_camp" ];
        entry = miss_info.action.translated();
        bool avail = update_time_left( entry, legacy_npc_list );
        mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                base_camps::base_dir, entry, avail );
    }
    for( const basecamp_upgrade &upgrade : available_upgrades( base_camps::base_dir ) ) {
        gather_bldg = upgrade.bldg;
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_upgrade_camp" ];
        comp_list npc_list = get_mission_workers( upgrade.bldg + "_faction_upgrade_camp" );
        if( npc_list.empty() && !upgrade.in_progress ) {
            entry = om_upgrade_description( upgrade.bldg );
            mission_key.add_start( miss_info.miss_id + upgrade.bldg,
                                   miss_info.desc + " " + upgrade.name, base_camps::base_dir,
                                   entry, upgrade.avail );
        } else if( !npc_list.empty() && upgrade.in_progress ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id + upgrade.bldg,
                                    miss_info.ret_desc + " " + upgrade.name, base_camps::base_dir,
                                    entry, avail );
        }
    }

    // Missions that belong exclusively to the central tile
    comp_list npc_list = get_mission_workers( "_faction_camp_crafting_" + base_dir_id );
    const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_crafting_" ];
    //This handles all crafting by the base, regardless of level
    if( npc_list.empty() ) {
        std::map<recipe_id, translation> craft_recipes = recipe_deck( base_camps::base_dir );
        add_available_recipes( mission_key, base_camps::base_dir, craft_recipes );
    } else {
        entry = miss_info.action.translated();
        bool avail = update_time_left( entry, npc_list );
        mission_key.add_return( base_dir_id + miss_info.ret_miss_id,
                                base_dir_abbr + miss_info.ret_desc, base_camps::base_dir, entry,
                                avail );
    }
    if( can_expand() ) {
        comp_list npc_list = get_mission_workers( "_faction_camp_expansion" );
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_expansion" ];
        entry = string_format( _( "Notes:\n"
                                  "Your base has become large enough to support an expansion.  "
                                  "Expansions open up new opportunities but can be expensive and "
                                  "time consuming.  Pick them carefully, only 8 can be built at "
                                  "each camp.\n\n"
                                  "Skill used: N/A\n"
                                  "Effects:\n"
                                  "> Choose any one of the available expansions.  Starting with "
                                  "a farm is always a solid choice since food is used to support "
                                  "companion missions and little is needed to get it going.  "
                                  "With minimal investment, a mechanic can be useful as a "
                                  "chop-shop to rapidly dismantle large vehicles, and a forge "
                                  "provides the resources to make charcoal.\n\n"
                                  "NOTE: Actions available through expansions are located in "
                                  "separate tabs of the Camp Manager window.\n\n"
                                  "Risk: None\n"
                                  "Time: 3 Hours\n"
                                  "Positions: %d/1\n" ), npc_list.size() );
        mission_key.add_start( miss_info.miss_id, miss_info.desc.translated(),
                               base_camps::base_dir, entry, npc_list.empty() );
        if( !npc_list.empty() ) {
            entry = miss_info.action.translated();
            bool avail = update_time_left( entry, npc_list );
            mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                    base_camps::base_dir, entry, avail );
        }
    }

    if( !by_radio ) {
        entry = string_format( _( "Notes:\n"
                                  "Distribute food to your follower and fill you larders.  "
                                  "Place the food you wish to distribute in the camp food zone.  "
                                  "You must have a camp food zone, and a camp storage zone, "
                                  "or you will be prompted to create them using the zone manager.\n"
                                  "Effects:\n"
                                  "> Increases your faction's food supply value which in "
                                  "turn is used to pay laborers for their time\n\n"
                                  "Must have enjoyability >= -6\n"
                                  "Perishable food liquidated at penalty depending on "
                                  "upgrades and rot time:\n"
                                  "> Rotten: 0%%\n"
                                  "> Rots in < 2 days: 60%%\n"
                                  "> Rots in < 5 days: 80%%\n\n"
                                  "Total faction food stock: %d kcal\nor %d day's rations" ),
                               camp_food_supply(), camp_food_supply( 0, true ) );
        mission_key.add( "Distribute Food", _( "Distribute Food" ), entry );
        validate_assignees();
        std::vector<npc_ptr> npc_list = get_npcs_assigned();
        entry = string_format( _( "Notes:\n"
                                  "Assign repeating job duties to NPCs stationed here.\n"
                                  "Difficulty: N/A\n"
                                  "Effects:\n"
                                  "\n\nRisk: None\n"
                                  "Time: Ongoing" ) );
        mission_key.add( "Assign Jobs", _( "Assign Jobs" ), entry );
        entry = _( "Notes:\nAbandon this camp" );
        mission_key.add( "Abandon Camp", _( "Abandon Camp" ), entry );
    }
    // Missions assigned to the central tile that could be done by an expansion
    get_available_missions_by_dir( mission_key, base_camps::base_dir );

    // Loop over expansions
    for( const point &dir : directions ) {
        get_available_missions_by_dir( mission_key, dir );
    }

    if( !camp_workers.empty() ) {
        const base_camps::miss_data &miss_info = base_camps::miss_info[ "_faction_camp_recall" ];
        entry = string_format( _( "Notes:\n"
                                  "Cancel a current mission and force the immediate return of a "
                                  "companion.  No work will be done on the mission and all "
                                  "resources used on the mission will be lost.\n\n"
                                  "WARNING: All resources used on the mission will be lost and "
                                  "no work will be done.  Only use this mission to recover a "
                                  "companion who cannot otherwise be recovered.\n\n"
                                  "Companions must be on missions for at least 24 hours before "
                                  "emergency recall becomes available." ) );
        bool avail = update_time_fixed( entry, camp_workers, 24_hours );
        mission_key.add_return( miss_info.ret_miss_id, miss_info.ret_desc.translated(),
                                base_camps::base_dir, entry, avail );
    }
}

bool basecamp::handle_mission( const std::string &miss_id, cata::optional<point> opt_miss_dir )
{
    const point &miss_dir = opt_miss_dir ? *opt_miss_dir : base_camps::base_dir;

    if( miss_id == "Distribute Food" ) {
        distribute_food();
    }

    if( miss_id.size() > 12 && miss_id.substr( 0, 12 ) == "Upgrade Camp" ) {
        const std::string bldg = miss_id.substr( 12 );
        start_upgrade( bldg, base_camps::base_dir, bldg + "_faction_upgrade_camp" );
    } else if( miss_id == "Recover Ally from Upgrading" ) {
        upgrade_return( base_camps::base_dir, "_faction_upgrade_camp" );
    } else if( miss_id.size() > 27 && miss_id.substr( 0, 27 ) == "Recover Ally from Upgrading" ) {
        const std::string bldg = miss_id.substr( 27 );
        upgrade_return( base_camps::base_dir, bldg + "_faction_upgrade_camp", bldg );
    }

    if( miss_id == "Menial Labor" ) {
        start_menial_labor();
    } else if( miss_id == "Recover Menial Laborer" ) {
        menial_return();
    }

    if( miss_id == "Assign Jobs" ) {
        job_assignment_ui();
    }
    if( miss_id == "Abandon Camp" ) {
        abandon_camp();
    }

    if( miss_id == "Expand Base" ) {
        start_mission( "_faction_camp_expansion", 3_hours, true,
                       _( "departs to survey land…" ), false, {}, skill_gun, 0 );
    } else if( miss_id == "Recover Surveyor" ) {
        survey_return();
    }

    if( miss_id == "Gather Materials" ) {
        start_mission( "_faction_camp_gathering", 3_hours, true,
                       _( "departs to search for materials…" ), false, {}, skill_survival, 0 );
    } else if( miss_id == "Recover Ally from Gathering" ) {
        gathering_return( "_faction_camp_gathering", 3_hours );
    }

    if( miss_id == "Collect Firewood" ) {
        start_mission( "_faction_camp_firewood", 3_hours, true,
                       _( "departs to search for firewood…" ), false, {}, skill_survival, 0 );
    } else if( miss_id == "Recover Firewood Gatherers" ) {
        gathering_return( "_faction_camp_firewood", 3_hours );
    }

    if( miss_id == "Cut Logs" ) {
        start_cut_logs();
    } else if( miss_id == "Recover Log Cutter" ) {
        const std::string msg = _( "returns from working in the woods…" );
        mission_return( "_faction_camp_cut_log", 6_hours, true, msg, "construction", 2 );
    }

    if( miss_id == "Clearcut" ) {
        start_clearcut();
    } else if( miss_id == "Recover Clearcutter" ) {
        const std::string msg = _( "returns from working in the woods…" );
        mission_return( "_faction_camp_clearcut", 6_hours, true, msg, "construction", 1 );
    }

    if( miss_id == "Setup Hide Site" ) {
        start_setup_hide_site();
    } else if( miss_id == "Recover Hide Setup" ) {
        const std::string msg = _( "returns from working on the hide site…" );
        mission_return( "_faction_camp_hide_site", 3_hours, true, msg, "survival", 3 );
    }

    if( miss_id == "Relay Hide Site" ) {
        start_relay_hide_site();
    } else if( miss_id == "Recover Hide Transport" ) {
        const std::string msg = _( "returns from shuttling gear between the hide site…" );
        mission_return( "_faction_camp_hide_trans", 3_hours, true, msg, "survival", 3 );
    }

    if( miss_id == "Camp Forage" ) {
        start_mission( "_faction_camp_foraging", 4_hours, true,
                       _( "departs to search for edible plants…" ), false, {}, skill_survival, 0 );
    } else if( miss_id == "Recover Foragers" ) {
        gathering_return( "_faction_camp_foraging", 4_hours );
    }
    if( miss_id == "Trap Small Game" ) {
        start_mission( "_faction_camp_trapping", 6_hours, true,
                       _( "departs to set traps for small animals…" ), false, {}, skill_traps, 0 );
    } else if( miss_id == "Recover Trappers" ) {
        gathering_return( "_faction_camp_trapping", 6_hours );
    }

    if( miss_id == "Hunt Large Animals" ) {
        start_mission( "_faction_camp_hunting", 6_hours, true,
                       _( "departs to hunt for meat…" ), false, {}, skill_gun, 0 );
    } else if( miss_id == "Recover Hunter" ) {
        gathering_return( "_faction_camp_hunting", 6_hours );
    }

    if( miss_id == "Construct Map Fort" || miss_id == "Construct Trench" ) {
        std::string bldg_exp = "faction_wall_level_N_0";
        if( miss_id == "Construct Trench" ) {
            bldg_exp = "faction_wall_level_N_1";
        }
        start_fortifications( bldg_exp );
    } else if( miss_id == "Finish Map Fort" ) {
        fortifications_return();
    }

    if( miss_id == "Recruit Companions" ) {
        start_mission( "_faction_camp_recruit_0", 4_days, true,
                       _( "departs to search for recruits…" ), false, {}, skill_gun, 0 );
    } else if( miss_id == "Recover Recruiter" ) {
        recruit_return( "_faction_camp_recruit_0", recruit_evaluation() );
    }

    if( miss_id == "Scout Mission" ) {
        start_combat_mission( "_faction_camp_scout_0" );
    } else if( miss_id == "Combat Patrol" ) {
        start_combat_mission( "_faction_camp_combat_0" );
    } else if( miss_id == "Recover Scout" ) {
        combat_mission_return( "_faction_camp_scout_0" );
    } else if( miss_id == "Recover Combat Patrol" ) {
        combat_mission_return( "_faction_camp_combat_0" );
    }

    const std::string base_dir_id = base_camps::all_directions.at( base_camps::base_dir ).id;
    const std::string miss_dir_id = base_camps::all_directions.at( miss_dir ).id;
    const tripoint omt_trg = omt_pos + miss_dir;

    if( miss_id.substr( 0, 18 + miss_dir_id.size() ) == miss_dir_id + " Expansion Upgrade" ) {
        const std::string bldg = miss_id.substr( 18 + miss_dir_id.size() );
        start_upgrade( bldg, miss_dir, bldg + "_faction_upgrade_exp_" + miss_dir_id );
    } else if( miss_id == "Recover Ally, " + miss_dir_id + " Expansion" ) {
        upgrade_return( miss_dir, "_faction_upgrade_exp_" + miss_dir_id );
    } else {
        const std::string search_str = "Recover Ally, " + miss_dir_id + " Expansion";
        size_t search_len = search_str.size();
        if( miss_id.size() > search_len && miss_id.substr( 0, search_len ) == search_str ) {
            const std::string bldg = miss_id.substr( search_len );
            upgrade_return( miss_dir, bldg + "_faction_upgrade_exp_" + miss_dir_id, bldg );
        }
    }

    start_crafting( miss_id, miss_dir, "BASE", "_faction_camp_crafting_" );
    start_crafting( miss_id, miss_dir, "FARM", "_faction_exp_farm_crafting_" );
    if( miss_id == base_dir_id + " (Finish) Crafting" ) {
        const std::string msg = _( "returns to you with something…" );
        mission_return( "_faction_camp_crafting_" + miss_dir_id, 1_minutes, true, msg,
                        "construction", 2 );
    } else if( miss_id == miss_dir_id + " (Finish) Crafting" ) {
        const std::string msg = _( "returns from your farm with something…" );
        mission_return( "_faction_exp_farm_crafting_" + miss_dir_id, 1_minutes, true, msg,
                        "construction", 2 );
    }

    start_crafting( miss_id, miss_dir, "COOK", "_faction_exp_kitchen_cooking_" );
    if( miss_id == miss_dir_id + " (Finish) Cooking" ) {
        const std::string msg = _( "returns from your kitchen with something…" );
        mission_return( "_faction_exp_kitchen_cooking_" + miss_dir_id, 1_minutes,
                        true, msg, "cooking", 2 );
    }

    start_crafting( miss_id, miss_dir, "SMITH", "_faction_exp_blacksmith_crafting_" );
    if( miss_id == miss_dir_id + " (Finish) Smithing" ) {
        const std::string msg = _( "returns from your blacksmith shop with something…" );
        mission_return( "_faction_exp_blacksmith_crafting_" + miss_dir_id, 1_minutes,
                        true, msg, "fabrication", 2 );
    }
    if( miss_id == miss_dir_id + " Plow Fields" ) {
        start_farm_op( miss_dir, omt_trg, farm_ops::plow );
    } else if( miss_id == miss_dir_id + " (Finish) Plow Fields" ) {
        farm_return( "_faction_exp_plow_" + miss_dir_id, omt_trg, farm_ops::plow );
    }

    if( miss_id == miss_dir_id + " Plant Fields" ) {
        start_farm_op( miss_dir, omt_trg, farm_ops::plant );
    } else if( miss_id == miss_dir_id + " (Finish) Plant Fields" ) {
        farm_return( "_faction_exp_plant_" + miss_dir_id, omt_trg, farm_ops::plant );
    }

    if( miss_id == miss_dir_id + " Harvest Fields" ) {
        start_farm_op( miss_dir, omt_trg, farm_ops::harvest );
    }  else if( miss_id == miss_dir_id + " (Finish) Harvest Fields" ) {
        farm_return( "_faction_exp_harvest_" + miss_dir_id, omt_trg, farm_ops::harvest );
    }

    if( miss_id == miss_dir_id + " Chop Shop" ) {
        start_garage_chop( miss_dir, omt_trg );
    } else if( miss_id == miss_dir_id + " (Finish) Chop Shop" ) {
        const std::string msg = _( "returns from your garage…" );
        mission_return( "_faction_exp_chop_shop_" + miss_dir_id, 5_days, true, msg,
                        "mechanics", 2 );
    }

    if( miss_id == "Emergency Recall" ) {
        emergency_recall();
    }

    g->draw_ter();
    wrefresh( g->w_terrain );
    g->draw_panels( true );

    return true;

}

// camp faction companion mission start functions
npc_ptr basecamp::start_mission( const std::string &miss_id, time_duration duration,
                                 bool must_feed, const std::string &desc, bool /*group*/,
                                 const std::vector<item *> &equipment,
                                 const skill_id &skill_tested, int skill_level )
{
    std::map<skill_id, int> required_skills;
    required_skills[ skill_tested ] = skill_level;
    return start_mission( miss_id, duration, must_feed, desc, false, equipment, required_skills );
}

npc_ptr basecamp::start_mission( const std::string &miss_id, time_duration duration,
                                 bool must_feed, const std::string &desc, bool /*group*/,
                                 const std::vector<item *> &equipment,
                                 const std::map<skill_id, int> &required_skills )
{
    if( must_feed && camp_food_supply() < time_to_food( duration ) ) {
        popup( _( "You don't have enough food stored to feed your companion." ) );
        return nullptr;
    }
    npc_ptr comp = talk_function::individual_mission( omt_pos, base_camps::id, desc, miss_id,
                   false, equipment, required_skills );
    if( comp != nullptr ) {
        comp->companion_mission_time_ret = calendar::turn + duration;
        if( must_feed ) {
            camp_food_supply( duration );
        }
    }
    return comp;
}

void basecamp::start_upgrade( const std::string &bldg, const point &dir,
                              const std::string &key )
{
    const recipe &making = recipe_id( bldg ).obj();
    //Stop upgrade if you don't have materials
    if( making.deduped_requirements().can_make_with_inventory(
            _inv, making.get_component_filter() ) ) {
        bool must_feed = bldg != "faction_base_camp_1";

        basecamp_action_components components( making, 1, *this );
        if( !components.choose_components() ) {
            return;
        }

        time_duration work_days = base_camps::to_workdays( making.batch_duration() );
        npc_ptr comp = nullptr;
        if( making.required_skills.empty() ) {
            if( making.skill_used.is_valid() ) {
                comp = start_mission( key, work_days, must_feed,
                                      _( "begins to upgrade the camp…" ), false, {},
                                      making.skill_used, making.difficulty );
            } else {
                comp = start_mission( key, work_days, must_feed,
                                      _( "begins to upgrade the camp…" ), false, {} );
            }
        } else {
            comp = start_mission( key, work_days, must_feed, _( "begins to upgrade the camp…" ),
                                  false, {}, making.required_skills );
        }
        if( comp == nullptr ) {
            return;
        }
        components.consume_components();
        update_in_progress( bldg, dir );
    } else {
        popup( _( "You don't have the materials for the upgrade." ) );
    }
}

void basecamp::abandon_camp()
{
    validate_assignees();
    npc_ptr random_guy;
    for( npc_ptr &guy : overmap_buffer.get_companion_mission_npcs( 10 ) ) {
        npc_companion_mission c_mission = guy->get_companion_mission();
        if( c_mission.role_id != base_camps::id ) {
            continue;
        }
        random_guy = guy;
        const std::string return_msg = _( "responds to the emergency recall…" );
        finish_return( *guy, false, return_msg, "menial", 0, true );
    }
    for( npc_ptr &guy : get_npcs_assigned() ) {
        talk_function::stop_guard( *guy );
    }
    overmap_buffer.remove_camp( *this );
    g->m.remove_submap_camp( random_guy->pos() );
    add_msg( m_info, _( "You abandon %s." ), name );
}

void basecamp::job_assignment_ui()
{
    int term_x = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 2 : 0;
    int term_y = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 2 : 0;

    catacurses::window w_jobs = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                                point( term_y, term_x ) );
    const int entries_per_page = FULL_SCREEN_HEIGHT - 4;
    size_t selection = 0;
    input_context ctxt( "FACTION MANAGER" );
    ctxt.register_cardinal();
    ctxt.register_updown();
    ctxt.register_action( "ANY_INPUT" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    while( true ) {
        werase( w_jobs );
        // create a list of npcs stationed at this camp
        std::vector<npc *> stationed_npcs;
        for( const auto &elem : get_npcs_assigned() ) {
            if( elem ) {
                stationed_npcs.push_back( elem.get() );
            }
        }
        npc *cur_npc = nullptr;
        // entries_per_page * page number
        const size_t top_of_page = entries_per_page * ( selection / entries_per_page );
        if( !stationed_npcs.empty() ) {
            cur_npc = stationed_npcs[selection];
        }

        for( int i = 0; i < FULL_SCREEN_HEIGHT - 1; i++ ) {
            mvwputch( w_jobs, point( 45, i ), BORDER_COLOR, LINE_XOXO );
        }
        draw_border( w_jobs );
        const nc_color col = c_white;
        const std::string no_npcs = _( "There are no npcs stationed here" );
        if( !stationed_npcs.empty() ) {
            draw_scrollbar( w_jobs, selection, entries_per_page, stationed_npcs.size(),
                            point( 0, 3 ) );
            for( size_t i = top_of_page; i < stationed_npcs.size(); i++ ) {
                const int y = i - top_of_page + 3;
                trim_and_print( w_jobs, point( 1, y ), 43, selection == i ? hilite( col ) : col,
                                stationed_npcs[i]->disp_name() );
            }
            if( selection < stationed_npcs.size() ) {
                std::string job_description;
                if( cur_npc && cur_npc->has_job() ) {
                    // get the current NPCs job
                    job_description = npc_job_name( cur_npc->get_job() );
                } else {
                    job_description = _( "No particular job" );
                }
                mvwprintz( w_jobs, point( 46, 3 ), c_light_gray, job_description );
            } else {
                mvwprintz( w_jobs, point( 46, 4 ), c_light_red, no_npcs );
            }
        } else {
            mvwprintz( w_jobs, point( 46, 4 ), c_light_red, no_npcs );
        }
        mvwprintz( w_jobs, point( 1, FULL_SCREEN_HEIGHT - 1 ), c_light_gray,
                   _( "Press %s to change this workers job." ), ctxt.get_desc( "CONFIRM" ) );
        wrefresh( w_jobs );
        const std::string action = ctxt.handle_input();
        if( action == "DOWN" ) {
            selection++;
            if( selection >= stationed_npcs.size() ) {
                selection = 0;
            }
        } else if( action == "UP" ) {
            if( selection == 0 ) {
                selection = stationed_npcs.empty() ? 0 : stationed_npcs.size() - 1;
            } else {
                selection--;
            }
        } else if( action == "CONFIRM" ) {
            if( !stationed_npcs.empty() ) {
                uilist smenu;
                smenu.text = _( "Assign which job?" );
                int count = 0;
                for( const auto &entry : all_jobs() ) {
                    smenu.addentry( count++, true, MENU_AUTOASSIGN, entry );
                }

                smenu.query();
                if( smenu.ret >= 0 ) {
                    cur_npc->set_job( static_cast<npc_job>( smenu.ret ) );
                }
            }
        } else if( action == "QUIT" ) {
            break;
        }
    }

    g->refresh_all();
}

void basecamp::start_menial_labor()
{
    if( camp_food_supply() < time_to_food( 3_hours ) ) {
        popup( _( "You don't have enough food stored to feed your companion." ) );
        return;
    }
    shared_ptr_fast<npc> comp = talk_function::companion_choose();
    if( comp == nullptr ) {
        return;
    }
    validate_sort_points();

    comp->assign_activity( activity_id( "ACT_MOVE_LOOT" ) );
    popup( _( "%s goes off to clean toilets and sort loot." ), comp->disp_name() );
}

void basecamp::start_cut_logs()
{
    std::vector<std::string> log_sources = { "forest", "forest_thick", "forest_water" };
    popup( _( "Forests and swamps are the only valid cutting locations." ) );
    tripoint forest = om_target_tile( omt_pos, 1, 50, log_sources );
    if( forest != tripoint( -999, -999, -999 ) ) {
        standard_npc sample_npc( "Temp" );
        sample_npc.set_fake( true );
        int tree_est = om_cutdown_trees_est( forest, 50 );
        int tree_young_est = om_harvest_ter_est( sample_npc, forest,
                             ter_id( "t_tree_young" ), 50 );
        int dist = rl_dist( forest.xy(), omt_pos.xy() );
        //Very roughly what the player does + 6 hours for prep, clean up, breaks
        time_duration chop_time = 6_hours + 1_hours * tree_est + 7_minutes * tree_young_est;
        int haul_items = 2 * tree_est + 3 * tree_young_est;
        time_duration travel_time = companion_travel_time_calc( forest, omt_pos, 0_minutes,
                                    2, haul_items );
        time_duration work_time = travel_time + chop_time;
        if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time,
                       chop_time, travel_time, dist, 2, time_to_food( work_time ) ) ) ) {
            return;
        }
        g->draw_ter();

        npc_ptr comp = start_mission( "_faction_camp_cut_log", work_time, true,
                                      _( "departs to cut logs…" ), false, {},
                                      skill_fabrication, 2 );
        if( comp != nullptr ) {
            om_cutdown_trees_logs( forest, 50 );
            om_harvest_ter( *comp, forest, ter_id( "t_tree_young" ), 50 );
            mass_volume harvest = om_harvest_itm( comp, forest, 95 );
            // recalculate trips based on actual load and capacity
            travel_time = companion_travel_time_calc( forest, omt_pos, 0_minutes, 2,
                          harvest.count );
            work_time = travel_time + chop_time;
            comp->companion_mission_time_ret = calendar::turn + work_time;
            //If we cleared a forest...
            if( om_cutdown_trees_est( forest ) < 5 ) {
                const oter_id &omt_trees = overmap_buffer.ter( forest );
                //Do this for swamps "forest_wet" if we have a swamp without trees...
                if( omt_trees.id() == "forest" || omt_trees.id() == "forest_thick" ) {
                    overmap_buffer.ter_set( forest, oter_id( "field" ) );
                }
            }
        }
    }
}

void basecamp::start_clearcut()
{
    std::vector<std::string> log_sources = { "forest", "forest_thick" };
    popup( _( "Forests are the only valid cutting locations." ) );
    tripoint forest = om_target_tile( omt_pos, 1, 50, log_sources );
    if( forest != tripoint( -999, -999, -999 ) ) {
        standard_npc sample_npc( "Temp" );
        sample_npc.set_fake( true );
        int tree_est = om_cutdown_trees_est( forest, 95 );
        int tree_young_est = om_harvest_ter_est( sample_npc, forest,
                             ter_id( "t_tree_young" ), 95 );
        int dist = rl_dist( forest.xy(), omt_pos.xy() );
        //Very roughly what the player does + 6 hours for prep, clean up, breaks
        time_duration chop_time = 6_hours + 1_hours * tree_est + 7_minutes * tree_young_est;
        time_duration travel_time = companion_travel_time_calc( forest, omt_pos, 0_minutes, 2 );
        time_duration work_time = travel_time + chop_time;
        if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time,
                       chop_time, travel_time, dist, 2, time_to_food( work_time ) ) ) ) {
            return;
        }
        g->draw_ter();

        npc_ptr comp = start_mission( "_faction_camp_clearcut", work_time,
                                      true, _( "departs to clear a forest…" ), false, {},
                                      skill_fabrication, 1 );
        if( comp != nullptr ) {
            om_cutdown_trees_trunks( forest, 95 );
            om_harvest_ter_break( *comp, forest, ter_id( "t_tree_young" ), 95 );
            //If we cleared a forest...
            if( om_cutdown_trees_est( forest ) < 5 ) {
                overmap_buffer.ter_set( forest, oter_id( "field" ) );
            }
        }
    }
}

void basecamp::start_setup_hide_site()
{
    std::vector<std::string> hide_locations = { "forest", "forest_thick", "forest_water",
                                                "field"
                                              };
    popup( _( "Forests, swamps, and fields are valid hide site locations." ) );
    tripoint forest = om_target_tile( omt_pos, 10, 90, hide_locations, true, true,
                                      omt_pos, true );
    if( forest != tripoint( -999, -999, -999 ) ) {
        int dist = rl_dist( forest.xy(), omt_pos.xy() );
        inventory tgt_inv = g->u.inv;
        std::vector<item *> pos_inv = tgt_inv.items_with( []( const item & itm ) {
            return !itm.can_revive();
        } );
        if( !pos_inv.empty() ) {
            std::vector<item *> losing_equipment = give_equipment( pos_inv,
                                                   _( "Do you wish to give your companion "
                                                           "additional items?" ) );
            int trips = om_carry_weight_to_trips( losing_equipment );
            int haulage = trips <= 2 ? 0 : losing_equipment.size();
            time_duration build_time = 6_hours;
            time_duration travel_time = companion_travel_time_calc( forest, omt_pos, 0_minutes,
                                        2, haulage );
            time_duration work_time = travel_time + build_time;
            if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time,
                           build_time, travel_time, dist, trips, time_to_food( work_time ) ) ) ) {
                return;
            }
            npc_ptr comp = start_mission( "_faction_camp_hide_site", work_time, true,
                                          _( "departs to build a hide site…" ), false, {},
                                          skill_survival, 3 );
            if( comp != nullptr ) {
                trips = om_carry_weight_to_trips( losing_equipment, comp );
                haulage = trips <= 2 ? 0 : losing_equipment.size();
                work_time = companion_travel_time_calc( forest, omt_pos, 0_minutes, 2, haulage ) +
                            build_time;
                comp->companion_mission_time_ret = calendar::turn + work_time;
                om_set_hide_site( *comp, forest, losing_equipment );
            }
        } else {
            popup( _( "You need equipment to setup a hide site…" ) );
        }
    }
}

void basecamp::start_relay_hide_site()
{
    std::vector<std::string> hide_locations = { "faction_hide_site_0" };
    popup( _( "You must select an existing hide site." ) );
    tripoint forest = om_target_tile( omt_pos, 10, 90, hide_locations, true, true,
                                      omt_pos, true );
    if( forest != tripoint( -999, -999, -999 ) ) {
        int dist = rl_dist( forest.xy(), omt_pos.xy() );
        inventory tgt_inv = g->u.inv;
        std::vector<item *> pos_inv = tgt_inv.items_with( []( const item & itm ) {
            return !itm.can_revive();
        } );
        std::vector<item *> losing_equipment;
        if( !pos_inv.empty() ) {
            losing_equipment = give_equipment( pos_inv,
                                               _( "Do you wish to give your companion "
                                                  "additional items?" ) );
        }
        //Check items in improvised shelters at hide site
        tinymap target_bay;
        target_bay.load( tripoint( forest.x * 2, forest.y * 2, forest.z ), false );
        std::vector<item *> hide_inv;
        for( item &i : target_bay.i_at( point( 11, 10 ) ) ) {
            hide_inv.push_back( &i );
        }
        std::vector<item *> gaining_equipment;
        if( !hide_inv.empty() ) {
            gaining_equipment = give_equipment( hide_inv, _( "Bring gear back?" ) );
        }
        if( !losing_equipment.empty() || !gaining_equipment.empty() ) {
            //Only get charged the greater trips since return is free for both
            int trips = std::max( om_carry_weight_to_trips( gaining_equipment ),
                                  om_carry_weight_to_trips( losing_equipment ) );
            int haulage = trips <= 2 ? 0 : std::max( gaining_equipment.size(),
                          losing_equipment.size() );
            time_duration build_time = 6_hours;
            time_duration travel_time = companion_travel_time_calc( forest, omt_pos, 0_minutes,
                                        trips, haulage );
            time_duration work_time = travel_time + build_time;
            if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time, build_time,
                           travel_time, dist, trips, time_to_food( work_time ) ) ) ) {
                return;
            }

            npc_ptr comp = start_mission( "_faction_camp_hide_trans", work_time, true,
                                          _( "departs for the hide site…" ), false, {},
                                          skill_survival, 3 );
            if( comp != nullptr ) {
                // recalculate trips based on actual load
                trips = std::max( om_carry_weight_to_trips( gaining_equipment, comp ),
                                  om_carry_weight_to_trips( losing_equipment, comp ) );
                int haulage = trips <= 2 ? 0 : std::max( gaining_equipment.size(),
                              losing_equipment.size() );
                work_time = companion_travel_time_calc( forest, omt_pos, 0_minutes, trips,
                                                        haulage ) + build_time;
                comp->companion_mission_time_ret = calendar::turn + work_time;
                om_set_hide_site( *comp, forest, losing_equipment, gaining_equipment );
            }
        } else {
            popup( _( "You need equipment to transport between the hide site…" ) );
        }
    }
}

void basecamp::start_fortifications( std::string &bldg_exp )
{
    std::vector<std::string> allowed_locations = {
        "forest", "forest_thick", "forest_water", "field"
    };
    popup( _( "Select a start and end point.  Line must be straight.  Fields, forests, and "
              "swamps are valid fortification locations.  In addition to existing fortification "
              "constructions." ) );
    tripoint start = om_target_tile( omt_pos, 2, 90, allowed_locations );
    popup( _( "Select an end point." ) );
    tripoint stop = om_target_tile( omt_pos, 2, 90, allowed_locations, true, false, start );
    if( start != tripoint( -999, -999, -999 ) && stop != tripoint( -999, -999, -999 ) ) {
        const recipe &making = recipe_id( bldg_exp ).obj();
        bool change_x = ( start.x != stop.x );
        bool change_y = ( start.y != stop.y );
        if( change_x && change_y ) {
            popup( "Construction line must be straight!" );
            return;
        }
        if( bldg_exp == "faction_wall_level_N_1" ) {
            std::vector<tripoint> tmp_line = line_to( stop, start, 0 );
            int line_count = tmp_line.size();
            int yes_count = 0;
            for( auto elem : tmp_line ) {
                if( std::find( fortifications.begin(), fortifications.end(), elem ) != fortifications.end() ) {
                    yes_count += 1;
                }
            }
            if( yes_count < line_count ) {
                popup( _( "Spiked pits must be built over existing trenches!" ) );
                return;
            }
        }
        std::vector<tripoint> fortify_om;
        if( ( change_x && stop.x < start.x ) || ( change_y && stop.y < start.y ) ) {
            //line_to doesn't include the origin point
            fortify_om.push_back( stop );
            std::vector<tripoint> tmp_line = line_to( stop, start, 0 );
            fortify_om.insert( fortify_om.end(), tmp_line.begin(), tmp_line.end() );
        } else {
            fortify_om.push_back( start );
            std::vector<tripoint> tmp_line = line_to( start, stop, 0 );
            fortify_om.insert( fortify_om.end(), tmp_line.begin(), tmp_line.end() );
        }
        int trips = 0;
        time_duration build_time = 0_hours;
        time_duration travel_time = 0_hours;
        int dist = 0;
        for( auto fort_om : fortify_om ) {
            bool valid = false;
            const oter_id &omt_ref = overmap_buffer.ter( fort_om );
            for( const std::string &pos_om : allowed_locations ) {
                if( omt_ref.id().c_str() == pos_om ) {
                    valid = true;
                    break;
                }
            }

            if( !valid ) {
                popup( _( "Invalid terrain in construction path." ) );
                return;
            }
            trips += 2;
            build_time += making.batch_duration();
            dist += rl_dist( fort_om.xy(), omt_pos.xy() );
            travel_time += companion_travel_time_calc( fort_om, omt_pos, 0_minutes, 2 );
        }
        time_duration total_time = base_camps::to_workdays( travel_time + build_time );
        int need_food = time_to_food( total_time );
        if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( total_time, build_time,
                       travel_time, dist, trips, need_food ) ) ) {
            return;
        } else if( !making.deduped_requirements().can_make_with_inventory( _inv,
                   making.get_component_filter(), ( fortify_om.size() * 2 ) - 2 ) ) {
            popup( _( "You don't have the material to build the fortification." ) );
            return;
        }

        const int batch_size = fortify_om.size() * 2 - 2;
        basecamp_action_components components( making, batch_size, *this );
        if( !components.choose_components() ) {
            return;
        }

        npc_ptr comp = start_mission( "_faction_camp_om_fortifications", total_time, true,
                                      _( "begins constructing fortifications…" ), false, {},
                                      making.required_skills );
        if( comp != nullptr ) {
            components.consume_components();
            comp->companion_mission_role_id = bldg_exp;
            for( auto pt : fortify_om ) {
                comp->companion_mission_points.push_back( pt );
            }
        }
    }
}

void basecamp::start_combat_mission( const std::string &miss )
{
    popup( _( "Select checkpoints until you reach maximum range or select the last point again "
              "to end." ) );
    tripoint start = omt_pos;
    std::vector<tripoint> scout_points = om_companion_path( start, 90, true );
    if( scout_points.empty() ) {
        return;
    }
    int dist = scout_points.size();
    int trips = 2;
    time_duration travel_time = companion_travel_time_calc( scout_points, 0_minutes, trips );
    if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( 0_hours, 0_hours,
                   travel_time, dist, trips, time_to_food( travel_time ) ) ) ) {
        return;
    }
    npc_ptr comp = start_mission( miss, travel_time, true, _( "departs on patrol…" ),
                                  false, {}, skill_survival, 3 );
    if( comp != nullptr ) {
        comp->companion_mission_points = scout_points;
    }
}

// the structure of this function has driven (at least) two devs insane
// recipe_deck returns a map of recipe ids to descriptions
// it first checks whether the mission id starts with the correct direction prefix,
// and then search for the mission id without direction prefix in the recipes
// if there's a match, the player has selected a crafting mission
void basecamp::start_crafting( const std::string &cur_id, const point &cur_dir,
                               const std::string &type, const std::string &miss_id )
{
    const std::string cur_dir_id = base_camps::all_directions.at( cur_dir ).id;
    const std::map<recipe_id, translation> &recipes = recipe_deck( type );
    if( cur_id.substr( 0, cur_dir_id.size() ) != cur_dir_id ) {
        // not a crafting mission or has the wrong direction
        return;
    }
    const auto it = recipes.find( recipe_id( cur_id.substr( cur_dir_id.size() ) ) );
    if( it != recipes.end() ) {
        const recipe &making = it->first.obj();

        if( !making.deduped_requirements().can_make_with_inventory(
                _inv, making.get_component_filter() ) ) {
            popup( _( "You don't have the materials to craft that" ) );
            return;
        }

        int batch_size = 1;
        string_input_popup popup_input;
        int batch_max = recipe_batch_max( making );
        const std::string title = string_format( _( "Batch crafting %s [MAX: %d]: " ),
                                  making.result_name(), batch_max );
        popup_input.title( title ).edit( batch_size );

        if( popup_input.canceled() || batch_size <= 0 ) {
            return;
        }
        if( batch_size > recipe_batch_max( making ) ) {
            popup( _( "Your batch is too large!" ) );
            return;
        }

        basecamp_action_components components( making, batch_size, *this );
        if( !components.choose_components() ) {
            return;
        }

        time_duration work_days = base_camps::to_workdays( making.batch_duration( batch_size ) );
        npc_ptr comp = start_mission( miss_id + cur_dir_id, work_days, true,
                                      _( "begins to work…" ), false, {},
                                      making.required_skills );
        if( comp != nullptr ) {
            components.consume_components();
            for( const item &results : making.create_results( batch_size ) ) {
                comp->companion_mission_inv.add_item( results );
            }
        }
        return;
    }
}

static bool farm_valid_seed( const item &itm )
{
    return itm.is_seed() && itm.typeId() != "marloss_seed" && itm.typeId() != "fungal_seeds";
}

static std::pair<size_t, std::string> farm_action( const tripoint &omt_tgt, farm_ops op,
        npc_ptr comp = nullptr )
{
    size_t plots_cnt = 0;
    std::string crops;

    const auto is_dirtmound = []( const tripoint & pos, tinymap & bay1, tinymap & bay2 ) {
        return ( bay1.ter( pos ) == t_dirtmound ) && ( !bay2.has_furn( pos ) );
    };
    const auto is_unplowed = []( const tripoint & pos, tinymap & farm_map ) {
        const ter_id &farm_ter = farm_map.ter( pos );
        return farm_ter->has_flag( "PLOWABLE" );
    };

    std::set<std::string> plant_names;
    std::vector<item *> seed_inv;
    if( comp ) {
        seed_inv = comp->companion_mission_inv.items_with( farm_valid_seed );
    }

    //farm_json is what the area should look like according to jsons
    tinymap farm_json;
    farm_json.generate( tripoint( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z ), calendar::turn );
    //farm_map is what the area actually looks like
    tinymap farm_map;
    farm_map.load( tripoint( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z ), false );
    tripoint mapmin = tripoint( 0, 0, omt_tgt.z );
    tripoint mapmax = tripoint( 2 * SEEX - 1, 2 * SEEY - 1, omt_tgt.z );
    bool done_planting = false;
    for( const tripoint &pos : farm_map.points_in_rectangle( mapmin, mapmax ) ) {
        if( done_planting ) {
            break;
        }
        switch( op ) {
            case farm_ops::plow:
                //Needs to be plowed to match json
                if( is_dirtmound( pos, farm_json, farm_map ) && is_unplowed( pos, farm_map ) ) {
                    plots_cnt += 1;
                    if( comp ) {
                        farm_map.ter_set( pos, t_dirtmound );
                    }
                }
                break;
            case farm_ops::plant:
                if( is_dirtmound( pos, farm_map, farm_map ) ) {
                    plots_cnt += 1;
                    if( comp ) {
                        if( seed_inv.empty() ) {
                            done_planting = true;
                            break;
                        }
                        item *tmp_seed = seed_inv.back();
                        seed_inv.pop_back();
                        std::list<item> used_seed;
                        if( tmp_seed->count_by_charges() ) {
                            used_seed.push_back( *tmp_seed );
                            tmp_seed->charges -= 1;
                            if( tmp_seed->charges > 0 ) {
                                seed_inv.push_back( tmp_seed );
                            }
                        }
                        used_seed.front().set_age( 0_turns );
                        farm_map.add_item_or_charges( pos, used_seed.front() );
                        farm_map.set( pos, t_dirt, f_plant_seed );
                    }
                }
                break;
            case farm_ops::harvest:
                if( farm_map.furn( pos ) == f_plant_harvest ) {
                    // Can't use item_stack::only_item() since there might be fertilizer
                    map_stack items = farm_map.i_at( pos );
                    const map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
                        return it.is_seed();
                    } );
                    if( seed != items.end() && farm_valid_seed( *seed ) ) {
                        plots_cnt += 1;
                        if( comp ) {
                            int skillLevel = comp->get_skill_level( skill_survival );
                            ///\EFFECT_SURVIVAL increases number of plants harvested from a seed
                            int plant_cnt = rng( skillLevel / 2, skillLevel );
                            plant_cnt = std::min( std::max( plant_cnt, 1 ), 9 );
                            int seed_cnt = std::max( 1, rng( plant_cnt / 4, plant_cnt / 2 ) );
                            for( auto &i : iexamine::get_harvest_items( *seed->type, plant_cnt,
                                    seed_cnt, true ) ) {
                                g->m.add_item_or_charges( g->u.pos(), i );
                            }
                            farm_map.i_clear( pos );
                            farm_map.furn_set( pos, f_null );
                            farm_map.ter_set( pos, t_dirt );
                        } else {
                            plant_names.insert( item::nname( itype_id( seed->type->seed->fruit_id ) ) );
                        }
                    }
                }
                break;
            default:
                // let the callers handle no op argument
                break;
        }
    }
    if( comp ) {
        farm_map.save();
    }

    int total_c = 0;
    for( const std::string &i : plant_names ) {
        if( total_c < 5 ) {
            crops += "    " + i + "\n";
            total_c++;
        } else if( total_c == 5 ) {
            crops += _( "+ more\n" );
            break;
        }
    }

    return std::make_pair( plots_cnt, crops );
}

void basecamp::start_farm_op( const point &dir, const tripoint &omt_tgt, farm_ops op )
{
    const std::string &dir_id = base_camps::all_directions.at( dir ).id;
    std::pair<size_t, std::string> farm_data = farm_action( omt_tgt, op );
    size_t plots_cnt = farm_data.first;
    if( !plots_cnt ) {
        return;
    }

    time_duration work = 0_minutes;
    switch( op ) {
        case farm_ops::harvest:
            work += 3_minutes * plots_cnt;
            start_mission( "_faction_exp_harvest_" + dir_id, work, true,
                           _( "begins to harvest the field…" ), false, {}, skill_survival, 1 );
            break;
        case farm_ops::plant: {
            std::vector<item *> seed_inv = _inv.items_with( farm_valid_seed );
            if( seed_inv.empty() ) {
                popup( _( "You have no additional seeds to give your companions…" ) );
                return;
            }
            std::vector<item *> plant_these = give_equipment( seed_inv,
                                              _( "Which seeds do you wish to have planted?" ) );
            size_t seed_cnt = 0;
            for( item *seeds : plant_these ) {
                seed_cnt += seeds->count();
            }
            size_t plots_seeded = std::min( seed_cnt, plots_cnt );
            if( !seed_cnt ) {
                return;
            }
            work += 1_minutes * plots_seeded;
            start_mission( "_faction_exp_plant_" + dir_id, work, true,
                           _( "begins planting the field…" ), false, plant_these,
                           skill_survival, 1 );
            break;
        }
        case farm_ops::plow:
            work += 5_minutes * plots_cnt;
            start_mission( "_faction_exp_plow_" + dir_id, work, true,
                           _( "begins plowing the field…" ), false, {} );
            break;
        default:
            debugmsg( "Farm operations called with no operation" );
    }
}

bool basecamp::start_garage_chop( const point &dir, const tripoint &omt_tgt )
{
    editmap edit;
    vehicle *car = edit.mapgen_veh_query( omt_tgt );
    if( car == nullptr ) {
        return false;
    }

    if( !query_yn( _( "       Chopping this vehicle:\n%s" ), camp_car_description( car ) ) ) {
        return false;
    }

    const std::string dir_id = base_camps::all_directions.at( dir ).id;
    npc_ptr comp = start_mission( "_faction_exp_chop_shop_" + dir_id, 5_days, true,
                                  _( "begins working in the garage…" ), false, {},
                                  skill_mechanics, 2 );
    if( comp == nullptr ) {
        return false;
    }
    // FIXME: use ranges, do this sensibly
    //Chopping up the car!
    std::vector<vehicle_part> p_all = car->parts;
    int prt = 0;
    int skillLevel = comp->get_skill_level( skill_mechanics );
    while( !p_all.empty() ) {
        vehicle_stack contents = car->get_items( prt );
        for( auto iter = contents.begin(); iter != contents.end(); ) {
            comp->companion_mission_inv.add_item( *iter );
            iter = contents.erase( iter );
        }
        bool broken = p_all[ prt ].is_broken();
        bool skill_break = false;
        bool skill_destroy = false;

        int dice = rng( 1, 20 );
        dice += skillLevel - p_all[ prt].info().difficulty;

        if( dice >= 20 ) {
            skill_break = false;
            skill_destroy = false;
            talk_function::companion_skill_trainer( *comp, skill_mechanics, 1_hours,
                                                    p_all[ prt].info().difficulty );
        } else if( dice > 15 ) {
            skill_break = false;
        } else if( dice > 9 ) {
            skill_break = true;
            skill_destroy = false;
        } else {
            skill_break = true;
            skill_destroy = true;
        }

        if( !broken && !skill_break ) {
            //Higher level garages will salvage liquids from tanks
            if( !p_all[prt].is_battery() ) {
                p_all[prt].ammo_consume( p_all[prt].ammo_capacity(),
                                         car->global_part_pos3( p_all[prt] ) );
            }
            comp->companion_mission_inv.add_item( p_all[prt].properties_to_item() );
        } else if( !skill_destroy ) {
            for( const item &itm : p_all[prt].pieces_for_broken_part() ) {
                comp->companion_mission_inv.add_item( itm );
            }
        }
        p_all.erase( p_all.begin() + 0 );
    }
    talk_function::companion_skill_trainer( *comp, skill_mechanics, 5_days, 2 );
    edit.mapgen_veh_destroy( omt_tgt, car );
    return true;
}

// camp faction companion mission recovery functions
npc_ptr basecamp::companion_choose_return( const std::string &miss_id,
        time_duration min_duration )
{
    return talk_function::companion_choose_return( omt_pos, base_camps::id, miss_id,
            calendar::turn - min_duration );
}
void basecamp::finish_return( npc &comp, const bool fixed_time, const std::string &return_msg,
                              const std::string &skill, int difficulty, const bool cancel )
{
    popup( "%s %s", comp.name, return_msg );
    // this is the time the mission was expected to take, or did take for fixed time missions
    time_duration reserve_time = comp.companion_mission_time_ret - comp.companion_mission_time;
    time_duration mission_time = reserve_time;
    if( !fixed_time ) {
        mission_time = calendar::turn - comp.companion_mission_time;
    }
    if( !cancel ) {
        talk_function::companion_skill_trainer( comp, skill, mission_time, difficulty );
    }

    // companions subtracted food when they started the mission, but didn't mod their hunger for
    // that food.  so add it back in.
    int need_food = time_to_food( mission_time - reserve_time );
    if( camp_food_supply() < need_food ) {
        popup( _( "Your companion seems disappointed that your pantry is empty…" ) );
    }
    int avail_food = std::min( need_food, camp_food_supply() ) + time_to_food( reserve_time );
    // movng all the logic from talk_function::companion return here instead of polluting
    // mission_companion
    comp.reset_companion_mission();
    comp.companion_mission_time = calendar::before_time_starts;
    comp.companion_mission_time_ret = calendar::before_time_starts;
    if( !cancel ) {
        for( size_t i = 0; i < comp.companion_mission_inv.size(); i++ ) {
            for( const auto &it : comp.companion_mission_inv.const_stack( i ) ) {
                if( !it.count_by_charges() || it.charges > 0 ) {
                    place_results( it );
                }
            }
        }
    }
    comp.companion_mission_inv.clear();
    comp.companion_mission_points.clear();
    // npc *may* be active, or not if outside the reality bubble
    g->reload_npcs();
    validate_assignees();

    camp_food_supply( -need_food );
    comp.mod_hunger( -avail_food );
    comp.mod_stored_kcal( avail_food );
    if( has_water() ) {
        comp.set_thirst( 0 );
    }
    comp.set_fatigue( 0 );
    comp.set_sleep_deprivation( 0 );
}

npc_ptr basecamp::mission_return( const std::string &miss_id, time_duration min_duration,
                                  bool fixed_time, const std::string &return_msg,
                                  const std::string &skill, int difficulty )
{
    npc_ptr comp = companion_choose_return( miss_id, min_duration );
    if( comp != nullptr ) {
        finish_return( *comp, fixed_time, return_msg, skill, difficulty );
    }
    return comp;
}

npc_ptr basecamp::emergency_recall()
{
    npc_ptr comp = talk_function::companion_choose_return( omt_pos, base_camps::id, "",
                   calendar::turn - 24_hours, false );
    if( comp != nullptr ) {
        const std::string return_msg = _( "responds to the emergency recall…" );
        finish_return( *comp, false, return_msg, "menial", 0, true );
    }
    return comp;

}

bool basecamp::upgrade_return( const point &dir, const std::string &miss )
{
    const std::string bldg = next_upgrade( dir, 1 );
    if( bldg == "null" ) {
        return false;
    }
    return upgrade_return( dir, miss, bldg );
}

bool basecamp::upgrade_return( const point &dir, const std::string &miss,
                               const std::string &bldg )
{
    auto e = expansions.find( dir );
    if( e == expansions.end() ) {
        return false;
    }
    const tripoint upos = e->second.pos;
    const recipe &making = recipe_id( bldg ).obj();

    time_duration work_days = base_camps::to_workdays( making.batch_duration() );
    npc_ptr comp = companion_choose_return( miss, work_days );

    if( comp == nullptr ) {
        return false;
    }
    if( !run_mapgen_update_func( making.get_blueprint(), upos ) ) {
        popup( _( "%s failed to build the %s upgrade, perhaps there is a vehicle in the way." ),
               comp->disp_name(),
               making.get_blueprint() );
        return false;
    }
    update_provides( bldg, e->second );
    update_resources( bldg );

    const std::string msg = _( "returns from upgrading the camp having earned a bit of "
                               "experience…" );
    finish_return( *comp, false, msg, "construction", making.difficulty );

    return true;
}

bool basecamp::menial_return()
{
    const std::string msg = _( "returns from doing the dirty work to keep the camp running…" );
    npc_ptr comp = mission_return( "_faction_camp_menial", 3_hours, true, msg, "menial", 2 );
    if( comp == nullptr ) {
        return false;
    }
    comp->revert_after_activity();
    return true;
}

bool basecamp::gathering_return( const std::string &task, time_duration min_time )
{
    npc_ptr comp = companion_choose_return( task, min_time );
    if( comp == nullptr ) {
        return false;
    }

    std::string task_description = _( "gathering materials" );
    int danger = 20;
    int favor = 2;
    int threat = 10;
    std::string skill_group = "gathering";
    int skill = 2 * comp->get_skill_level( skill_survival ) + comp->per_cur;
    int checks_per_cycle = 6;
    if( task == "_faction_camp_foraging" ) {
        task_description = _( "foraging for edible plants" );
        danger = 15;
        checks_per_cycle = 12;
    } else if( task == "_faction_camp_trapping" ) {
        task_description = _( "trapping small animals" );
        favor = 1;
        danger = 15;
        skill_group = "trapping";
        skill = 2 * comp->get_skill_level( skill_traps ) + comp->per_cur;
        checks_per_cycle = 4;
    } else if( task == "_faction_camp_hunting" ) {
        task_description = _( "hunting for meat" );
        danger = 10;
        favor = 0;
        skill_group = "hunting";
        skill = 1.5 * comp->get_skill_level( skill_gun ) + comp->per_cur / 2.0;
        threat = 12;
        checks_per_cycle = 2;
    }

    time_duration mission_time = calendar::turn - comp->companion_mission_time;
    if( one_in( danger ) && !survive_random_encounter( *comp, task_description, favor, threat ) ) {
        return false;
    }
    const std::string msg = string_format( _( "returns from %s carrying supplies and has a bit "
                                           "more experience…" ), task_description );
    finish_return( *comp, false, msg, skill_group, 1 );

    std::string itemlist = "forest";
    if( task == "_faction_camp_firewood" ) {
        itemlist = "gathering_faction_base_camp_firewood";
    } else if( task == "_faction_camp_gathering" ) {
        itemlist = get_gatherlist();
    } else if( task == "_faction_camp_foraging" ) {
        switch( season_of_year( calendar::turn ) ) {
            case SPRING:
                itemlist = "foraging_faction_camp_spring";
                break;
            case SUMMER:
                itemlist = "foraging_faction_camp_summer";
                break;
            case AUTUMN:
                itemlist = "foraging_faction_camp_autumn";
                break;
            case WINTER:
                itemlist = "foraging_faction_camp_winter";
                break;
            default:
                debugmsg( "Invalid season" );
        }
    }
    if( task == "_faction_camp_trapping" || task == "_faction_camp_hunting" ) {
        hunting_results( skill, task, checks_per_cycle * mission_time / min_time, 30 );
    } else {
        search_results( skill, itemlist, checks_per_cycle * mission_time / min_time, 15 );
    }

    return true;
}

void basecamp::fortifications_return()
{
    npc_ptr comp = companion_choose_return( "_faction_camp_om_fortifications", 3_hours );
    if( comp != nullptr ) {
        std::string build_n = "faction_wall_level_N_0";
        std::string build_e = "faction_wall_level_E_0";
        std::string build_s = "faction_wall_level_S_0";
        std::string build_w = "faction_wall_level_W_0";
        if( comp->companion_mission_role_id == "faction_wall_level_N_1" ) {
            build_n = "faction_wall_level_N_1";
            build_e = "faction_wall_level_E_1";
            build_s = "faction_wall_level_S_1";
            build_w = "faction_wall_level_W_1";
        }
        std::string build_first = build_e;
        std::string build_second = build_w;
        bool build_dir_NS = comp->companion_mission_points[0].y !=
                            comp->companion_mission_points[1].y;
        if( build_dir_NS ) {
            build_first = build_s;
            build_second = build_n;
        }
        //Add fences
        auto build_point = comp->companion_mission_points;
        for( size_t pt = 0; pt < build_point.size(); pt++ ) {
            //First point is always at top or west since they are built in a line and sorted
            if( pt == 0 ) {
                run_mapgen_update_func( build_first, build_point[pt] );
            } else if( pt == build_point.size() - 1 ) {
                run_mapgen_update_func( build_second, build_point[pt] );
            } else {
                run_mapgen_update_func( build_first, build_point[pt] );
                run_mapgen_update_func( build_second, build_point[pt] );
            }
            if( comp->companion_mission_role_id == "faction_wall_level_N_0" ) {
                tripoint fort_point = build_point[pt];
                fortifications.push_back( fort_point );
            }
        }
        const std::string msg = _( "returns from constructing fortifications…" );
        finish_return( *comp, true, msg, "construction", 2 );
    }
}

void basecamp::recruit_return( const std::string &task, int score )
{
    const std::string msg = _( "returns from searching for recruits with "
                               "a bit more experience…" );
    npc_ptr comp = mission_return( task, 4_days, true, msg, "recruiting", 2 );
    if( comp == nullptr ) {
        return;
    }

    npc_ptr recruit;
    //Success of finding an NPC to recruit, based on survival/tracking
    int skill = comp->get_skill_level( skill_survival );
    if( rng( 1, 20 ) + skill > 17 ) {
        recruit = make_shared_fast<npc>();
        recruit->normalize();
        recruit->randomize();
        popup( _( "%s encountered %s…" ), comp->name, recruit->name );
    } else {
        popup( _( "%s didn't find anyone to recruit…" ), comp->name );
        return;
    }
    //Chance of convincing them to come back
    skill = ( 100 * comp->get_skill_level( skill_speech ) + score ) / 100;
    if( rng( 1, 20 ) + skill  > 19 ) {
        popup( _( "%s convinced %s to hear a recruitment offer from you…" ), comp->name,
               recruit->name );
    } else {
        popup( _( "%s wasn't interested in anything %s had to offer…" ), recruit->name,
               comp->name );
        return;
    }
    //Stat window
    int rec_m = 0;
    int appeal = rng( -5, 3 ) + std::min( skill / 3, 3 );
    int food_desire = rng( 0, 5 );
    while( rec_m >= 0 ) {
        std::string description = _( "NPC Overview:\n\n" );
        description += string_format( _( "Name:  %s\n\n" ), right_justify( recruit->name, 20 ) );
        description += string_format( _( "Strength:        %10d\n" ), recruit->str_max );
        description += string_format( _( "Dexterity:       %10d\n" ), recruit->dex_max );
        description += string_format( _( "Intelligence:    %10d\n" ), recruit->int_max );
        description += string_format( _( "Perception:      %10d\n\n" ), recruit->per_max );
        description += _( "Top 3 Skills:\n" );

        const auto skillslist = Skill::get_skills_sorted_by( [&]( const Skill & a,
        const Skill & b ) {
            const int level_a = recruit->get_skill_level( a.ident() );
            const int level_b = recruit->get_skill_level( b.ident() );
            return level_a > level_b || ( level_a == level_b && a.name() < b.name() );
        } );

        description += string_format( "%s:          %4d\n", right_justify( skillslist[0]->name(), 12 ),
                                      recruit->get_skill_level( skillslist[0]->ident() ) );
        description += string_format( "%s:          %4d\n", right_justify( skillslist[1]->name(), 12 ),
                                      recruit->get_skill_level( skillslist[1]->ident() ) );
        description += string_format( "%s:          %4d\n\n", right_justify( skillslist[2]->name(), 12 ),
                                      recruit->get_skill_level( skillslist[2]->ident() ) );

        description += _( "Asking for:\n" );
        description += string_format( _( "> Food:     %10d days\n\n" ), food_desire );
        description += string_format( _( "Faction Food:%9d days\n\n" ),
                                      camp_food_supply( 0, true ) );
        description += string_format( _( "Recruit Chance: %10d%%\n\n" ),
                                      std::min( 100 * ( 10 + appeal ) / 20, 100 ) );
        description += _( "Select an option:" );

        std::vector<std::string> rec_options;
        rec_options.push_back( _( "Increase Food" ) );
        rec_options.push_back( _( "Decrease Food" ) );
        rec_options.push_back( _( "Make Offer" ) );
        rec_options.push_back( _( "Not Interested" ) );

        rec_m = uilist( description, rec_options );
        if( rec_m < 0 || rec_m == 3 || static_cast<size_t>( rec_m ) >= rec_options.size() ) {
            popup( _( "You decide you aren't interested…" ) );
            return;
        }

        if( rec_m == 0 && food_desire + 1 <= camp_food_supply( 0, true ) ) {
            food_desire++;
            appeal++;
        }
        if( rec_m == 1 ) {
            if( food_desire > 0 ) {
                food_desire--;
                appeal--;
            }
        }
        if( rec_m == 2 ) {
            break;
        }
    }
    // Roll for recruitment
    if( rng( 1, 20 ) + appeal >= 10 ) {
        popup( _( "%s has been convinced to join!" ), recruit->name );
    } else {
        popup( _( "%s wasn't interested…" ), recruit->name );
        // nullptr;
        return;
    }
    // Time durations always subtract from camp food supply
    camp_food_supply( 1_days * food_desire );
    recruit->spawn_at_precise( { g->get_levx(), g->get_levy() }, g->u.pos() + point( -4, -4 ) );
    overmap_buffer.insert_npc( recruit );
    recruit->form_opinion( g->u );
    recruit->mission = NPC_MISSION_NULL;
    recruit->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC,
                              recruit->global_omt_location(),
                              recruit->getID() ) );
    talk_function::follow( *recruit );
    g->load_npcs();
}

void basecamp::combat_mission_return( const std::string &miss )
{
    npc_ptr comp = companion_choose_return( miss, 3_hours );
    if( comp != nullptr ) {
        bool patrolling = miss == "_faction_camp_combat_0";
        comp_list patrol;
        npc_ptr guy = overmap_buffer.find_npc( comp->getID() );
        if( guy ) {
            patrol.push_back( guy );
        }
        for( auto pt : comp->companion_mission_points ) {
            const oter_id &omt_ref = overmap_buffer.ter( pt );
            int swim = comp->get_skill_level( skill_swimming );
            if( is_river( omt_ref ) && swim < 2 ) {
                if( swim == 0 ) {
                    popup( _( "Your companion hit a river and didn't know how to swim…" ) );
                } else {
                    popup( _( "Your companion hit a river and didn't know how to swim well "
                              "enough to cross…" ) );
                }
                break;
            }
            comp->death_drops = false;
            bool outcome = talk_function::companion_om_combat_check( patrol, pt, patrolling );
            comp->death_drops = true;
            if( outcome ) {
                overmap_buffer.reveal( pt, 2 );
            } else if( comp->is_dead() ) {
                popup( _( "%s didn't return from patrol…" ), comp->name );
                comp->place_corpse( pt );
                overmap_buffer.add_note( pt, "DEAD NPC" );
                overmap_buffer.remove_npc( comp->getID() );
                return;
            }
        }
        const std::string msg = _( "returns from patrol…" );
        finish_return( *comp, true, msg, "combat", 4 );
    }
}

bool basecamp::survey_return()
{
    npc_ptr comp = companion_choose_return( "_faction_camp_expansion", 3_hours );
    if( comp == nullptr ) {
        return false;
    }

    popup( _( "Select a tile up to %d tiles away." ), 1 );
    const tripoint where( ui::omap::choose_point() );
    if( where == overmap::invalid_tripoint ) {
        return false;
    }

    int dist = rl_dist( where.xy(), omt_pos.xy() );
    if( dist != 1 ) {
        popup( _( "You must select a tile within %d range of the camp" ), 1 );
        return false;
    }
    if( omt_pos.z != where.z ) {
        popup( _( "Expansions must be on the same level as the camp" ) );
        return false;
    }
    const point dir = talk_function::om_simple_dir( omt_pos, where );
    if( expansions.find( dir ) != expansions.end() ) {
        popup( _( "You already have an expansion at that location" ) );
        return false;
    }

    const oter_id &omt_ref = overmap_buffer.ter( where );
    const auto &pos_expansions = recipe_group::get_recipes_by_id( "all_faction_base_expansions",
                                 omt_ref.id().c_str() );
    if( pos_expansions.empty() ) {
        popup( _( "You can't build any expansions in a %s." ), omt_ref.id().c_str() );
        return false;
    }

    const recipe_id expansion_type = base_camps::select_camp_option( pos_expansions,
                                     _( "Select an expansion:" ) );

    if( !run_mapgen_update_func( expansion_type.str(), where ) ) {
        popup( _( "%s failed to add the %s expansion, perhaps there is a vehicle in the way." ),
               comp->disp_name(),
               expansion_type->blueprint_name() );
        return false;
    }
    overmap_buffer.ter_set( where, oter_id( expansion_type.str() ) );
    add_expansion( expansion_type.str(), where, dir );
    const std::string msg = _( "returns from surveying for the expansion." );
    finish_return( *comp, true, msg, "construction", 2 );
    return true;
}

bool basecamp::farm_return( const std::string &task, const tripoint &omt_tgt, farm_ops op )
{
    const std::string msg = _( "returns from working your fields…" );
    npc_ptr comp = companion_choose_return( task, 15_minutes );
    if( comp == nullptr ) {
        return false;
    }

    farm_action( omt_tgt, op, comp );

    //Give any seeds the NPC didn't use back to you.
    for( size_t i = 0; i < comp->companion_mission_inv.size(); i++ ) {
        for( const auto &it : comp->companion_mission_inv.const_stack( i ) ) {
            if( it.charges > 0 ) {
                g->u.i_add( it );
            }
        }
    }
    finish_return( *comp, true, msg, "survival", 2 );
    return true;
}

// window manipulation
void talk_function::draw_camp_tabs( const catacurses::window &win,
                                    const base_camps::tab_mode cur_tab,
                                    const std::vector<std::vector<mission_entry>> &entries )
{
    werase( win );
    const int width = getmaxx( win );
    mvwhline( win, point( 0, 2 ), LINE_OXOX, width );

    std::vector<std::string> tabs( base_camps::all_directions.size() );
    for( const auto &direction : base_camps::all_directions ) {
        tabs.at( direction.second.tab_order ) = direction.second.tab_title.translated();
    }
    const int tab_step = 3;
    int tab_space = 1;
    int tab_x = 0;
    for( auto &t : tabs ) {
        bool tab_empty = entries[tab_x + 1].empty();
        draw_subtab( win, tab_space, t, tab_x == cur_tab, false, tab_empty );
        tab_space += tab_step + utf8_width( t );
        tab_x++;
    }
    wrefresh( win );
}

std::string talk_function::name_mission_tabs( const tripoint &omt_pos, const std::string &role_id,
        const std::string &cur_title, base_camps::tab_mode cur_tab )
{
    if( role_id != base_camps::id ) {
        return cur_title;
    }
    cata::optional<basecamp *> temp_camp = overmap_buffer.find_camp( omt_pos.xy() );
    if( !temp_camp ) {
        return cur_title;
    }
    basecamp *bcp = *temp_camp;
    for( const auto &direction : base_camps::all_directions ) {
        if( cur_tab == direction.second.tab_order ) {
            return bcp->expansion_tab( direction.first );
        }
    }
    return bcp->expansion_tab( base_camps::base_dir );
}

// recipes and craft support functions
int basecamp::recipe_batch_max( const recipe &making ) const
{
    int max_batch = 0;
    const int max_checks = 9;
    for( size_t batch_size = 1000; batch_size > 0; batch_size /= 10 ) {
        for( int iter = 0; iter < max_checks; iter++ ) {
            time_duration work_days = base_camps::to_workdays( making.batch_duration(
                                          max_batch + batch_size ) );
            int food_req = time_to_food( work_days );
            bool can_make = making.deduped_requirements().can_make_with_inventory(
                                _inv, making.get_component_filter(), max_batch + batch_size );
            if( can_make && camp_food_supply() > food_req ) {
                max_batch += batch_size;
            } else {
                break;
            }
        }
    }
    return max_batch;
}

void basecamp::search_results( int skill, const Group_tag &group_id, int attempts, int difficulty )
{
    for( int i = 0; i < attempts; i++ ) {
        if( skill > rng( 0, difficulty ) ) {
            auto result = item_group::item_from( group_id, calendar::turn );
            if( !result.is_null() ) {
                place_results( result );
            }
        }
    }
}

void basecamp::hunting_results( int skill, const std::string &task, int attempts, int difficulty )
{
    // no item groups for corpses, so we'll have to improvise
    weighted_int_list<mtype_id> hunting_targets;
    hunting_targets.add( mtype_id( "mon_beaver" ), 10 );
    hunting_targets.add( mtype_id( "mon_fox_red" ), 10 );
    hunting_targets.add( mtype_id( "mon_fox_gray" ), 10 );
    hunting_targets.add( mtype_id( "mon_mink" ), 5 );
    hunting_targets.add( mtype_id( "mon_muskrat" ), 10 );
    hunting_targets.add( mtype_id( "mon_otter" ), 10 );
    hunting_targets.add( mtype_id( "mon_duck" ), 10 );
    hunting_targets.add( mtype_id( "mon_cockatrice" ), 1 );
    if( task == "_faction_camp_trapping" ) {
        hunting_targets.add( mtype_id( "mon_black_rat" ), 40 );
        hunting_targets.add( mtype_id( "mon_chipmunk" ), 30 );
        hunting_targets.add( mtype_id( "mon_groundhog" ), 30 );
        hunting_targets.add( mtype_id( "mon_hare" ), 20 );
        hunting_targets.add( mtype_id( "mon_lemming" ), 40 );
        hunting_targets.add( mtype_id( "mon_opossum" ), 10 );
        hunting_targets.add( mtype_id( "mon_rabbit" ), 20 );
        hunting_targets.add( mtype_id( "mon_squirrel" ), 20 );
        hunting_targets.add( mtype_id( "mon_weasel" ), 20 );
        hunting_targets.add( mtype_id( "mon_chicken" ), 10 );
        hunting_targets.add( mtype_id( "mon_grouse" ), 10 );
        hunting_targets.add( mtype_id( "mon_pheasant" ), 10 );
        hunting_targets.add( mtype_id( "mon_turkey" ), 20 );
    } else if( task == "_faction_camp_hunting" ) {
        hunting_targets.add( mtype_id( "mon_chicken" ), 20 );
        // good luck hunting upland game birds without dogs
        hunting_targets.add( mtype_id( "mon_grouse" ), 2 );
        hunting_targets.add( mtype_id( "mon_pheasant" ), 2 );
        hunting_targets.add( mtype_id( "mon_turkey" ), 10 );
        hunting_targets.add( mtype_id( "mon_bear" ), 1 );
        hunting_targets.add( mtype_id( "mon_cougar" ), 5 );
        hunting_targets.add( mtype_id( "mon_cow" ), 1 );
        hunting_targets.add( mtype_id( "mon_coyote" ), 15 );
        hunting_targets.add( mtype_id( "mon_deer" ), 2 );
        hunting_targets.add( mtype_id( "mon_moose" ), 1 );
        hunting_targets.add( mtype_id( "mon_pig" ), 1 );
        hunting_targets.add( mtype_id( "mon_wolf" ), 10 );
    }
    for( int i = 0; i < attempts; i++ ) {
        if( skill > rng( 0, difficulty ) ) {
            const mtype_id *target = hunting_targets.pick();
            auto result = item::make_corpse( *target, calendar::turn, "" );
            if( !result.is_null() ) {
                place_results( result );
            }
        }
    }
}

int om_harvest_furn_est( npc &comp, const tripoint &omt_tgt, const furn_id &f, int chance )
{
    return om_harvest_furn( comp, omt_tgt, f, chance, true, false );
}
int om_harvest_furn_break( npc &comp, const tripoint &omt_tgt, const furn_id &f, int chance )
{
    return om_harvest_furn( comp, omt_tgt, f, chance, false, false );
}
int om_harvest_furn( npc &comp, const tripoint &omt_tgt, const furn_id &f, int chance,
                     bool estimate, bool bring_back )
{
    const furn_t &furn_tgt = f.obj();
    tinymap target_bay;
    target_bay.load( tripoint( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z ), false );
    int harvested = 0;
    int total = 0;
    tripoint mapmin = tripoint( 0, 0, omt_tgt.z );
    tripoint mapmax = tripoint( 2 * SEEX - 1, 2 * SEEY - 1, omt_tgt.z );
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        if( target_bay.furn( p ) == f && x_in_y( chance, 100 ) ) {
            total++;
            if( estimate ) {
                continue;
            }
            if( bring_back ) {
                for( const item &itm : item_group::items_from( furn_tgt.bash.drop_group,
                        calendar::turn ) ) {
                    comp.companion_mission_inv.push_back( itm );
                }
                harvested++;
            }
            if( bring_back || comp.str_cur > furn_tgt.bash.str_min + rng( -2, 2 ) ) {
                target_bay.furn_set( p, furn_tgt.bash.furn_set );
            }
        }
    }
    target_bay.save();
    if( bring_back ) {
        return harvested;
    }
    return total;
}

int om_harvest_ter_est( npc &comp, const tripoint &omt_tgt, const ter_id &t, int chance )
{
    return om_harvest_ter( comp, omt_tgt, t, chance, true, false );
}
int om_harvest_ter_break( npc &comp, const tripoint &omt_tgt, const ter_id &t, int chance )
{
    return om_harvest_ter( comp, omt_tgt, t, chance, false, false );
}
int om_harvest_ter( npc &comp, const tripoint &omt_tgt, const ter_id &t, int chance,
                    bool estimate, bool bring_back )
{
    const ter_t &ter_tgt = t.obj();
    tinymap target_bay;
    target_bay.load( tripoint( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z ), false );
    int harvested = 0;
    int total = 0;
    tripoint mapmin = tripoint( 0, 0, omt_tgt.z );
    tripoint mapmax = tripoint( 2 * SEEX - 1, 2 * SEEY - 1, omt_tgt.z );
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        if( target_bay.ter( p ) == t && x_in_y( chance, 100 ) ) {
            total++;
            if( estimate ) {
                continue;
            }
            if( bring_back ) {
                for( const item &itm : item_group::items_from( ter_tgt.bash.drop_group,
                        calendar::turn ) ) {
                    comp.companion_mission_inv.push_back( itm );
                }
                harvested++;
                target_bay.ter_set( p, ter_tgt.bash.ter_set );
            }
        }
    }
    target_bay.save();
    if( bring_back ) {
        return harvested;
    }
    return total;
}

int om_cutdown_trees_est( const tripoint &omt_tgt, int chance )
{
    return om_cutdown_trees( omt_tgt, chance, true, false );
}
int om_cutdown_trees_logs( const tripoint &omt_tgt, int chance )
{
    return om_cutdown_trees( omt_tgt, chance, false, true );
}
int om_cutdown_trees_trunks( const tripoint &omt_tgt, int chance )
{
    return om_cutdown_trees( omt_tgt, chance, false, false );
}
int om_cutdown_trees( const tripoint &omt_tgt, int chance, bool estimate, bool force_cut_trunk )
{
    tinymap target_bay;
    target_bay.load( tripoint( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z ), false );
    int harvested = 0;
    int total = 0;
    tripoint mapmin = tripoint( 0, 0, omt_tgt.z );
    tripoint mapmax = tripoint( 2 * SEEX - 1, 2 * SEEY - 1, omt_tgt.z );
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        if( target_bay.ter( p ).obj().has_flag( "TREE" ) && rng( 0, 100 ) < chance ) {
            total++;
            if( estimate ) {
                continue;
            }
            // get a random number that is either 1 or -1
            int dir_x = 3 * ( 2 * rng( 0, 1 ) - 1 ) + rng( -1, 1 );
            int dir_y = 3 * rng( -1, 1 ) + rng( -1, 1 );
            tripoint to = p + tripoint( dir_x, dir_y, omt_tgt.z );
            std::vector<tripoint> tree = line_to( p, to, rng( 1, 8 ) );
            for( auto &elem : tree ) {
                target_bay.destroy( elem );
                target_bay.ter_set( elem, t_trunk );
            }
            target_bay.ter_set( p, t_dirt );
            harvested++;
        }
    }
    if( estimate ) {
        return total;
    }
    if( !force_cut_trunk ) {
        target_bay.save();
        return harvested;
    }
    // having cut down the trees, cut the trunks into logs
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        if( target_bay.ter( p ) == ter_id( "t_trunk" ) ) {
            target_bay.ter_set( p, t_dirt );
            target_bay.spawn_item( p, "log", rng( 2, 3 ), 0, calendar::turn );
            harvested++;
        }
    }
    target_bay.save();
    return harvested;
}

mass_volume om_harvest_itm( npc_ptr comp, const tripoint &omt_tgt, int chance, bool take )
{
    tinymap target_bay;
    target_bay.load( tripoint( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z ), false );
    units::mass harvested_m = 0_gram;
    units::volume harvested_v = 0_ml;
    units::mass total_m = 0_gram;
    units::volume total_v = 0_ml;
    int total_num = 0;
    int harvested_num = 0;
    tripoint mapmin = tripoint( 0, 0, omt_tgt.z );
    tripoint mapmax = tripoint( 2 * SEEX - 1, 2 * SEEY - 1, omt_tgt.z );
    for( const tripoint &p : target_bay.points_in_rectangle( mapmin, mapmax ) ) {
        for( const item &i : target_bay.i_at( p ) ) {
            total_m += i.weight( true );
            total_v += i.volume( true );
            total_num += 1;
            if( take && x_in_y( chance, 100 ) ) {
                if( comp ) {
                    comp->companion_mission_inv.push_back( i );
                }
                harvested_m += i.weight( true );
                harvested_v += i.volume( true );
                harvested_num += 1;
            }
        }
        if( take ) {
            target_bay.i_clear( p );
        }
    }
    target_bay.save();
    mass_volume results;
    if( take ) {
        results.wgt = harvested_m;
        results.vol = harvested_v;
        results.count = harvested_num;
    } else {
        results.wgt = total_m;
        results.vol = total_v;
        results.count = total_num;
    }
    return results;
}

tripoint om_target_tile( const tripoint &omt_pos, int min_range, int range,
                         const std::vector<std::string> &possible_om_types, bool must_see,
                         bool popup_notice, const tripoint &source, bool bounce )
{
    bool errors = false;
    if( popup_notice ) {
        popup( _( "Select a location between  %d and  %d tiles away." ), min_range, range );
    }

    std::vector<std::string> bounce_locations = { "faction_hide_site_0" };

    tripoint where;
    om_range_mark( omt_pos, range );
    om_range_mark( omt_pos, min_range, true, "Y;X: MIN RANGE" );
    if( source == tripoint( -999, -999, -999 ) ) {
        where = ui::omap::choose_point();
    } else {
        where = ui::omap::choose_point( source );
    }
    om_range_mark( omt_pos, range, false );
    om_range_mark( omt_pos, min_range, false, "Y;X: MIN RANGE" );

    if( where == overmap::invalid_tripoint ) {
        return tripoint( -999, -999, -999 );
    }
    int dist = rl_dist( where.xy(), omt_pos.xy() );
    if( dist > range || dist < min_range ) {
        popup( _( "You must select a target between %d and %d range from the base.  Range: %d" ),
               min_range, range, dist );
        errors = true;
    }

    tripoint omt_tgt = tripoint( where );

    const oter_id &omt_ref = overmap_buffer.ter( omt_tgt );

    if( must_see && !overmap_buffer.seen( omt_tgt ) ) {
        errors = true;
        popup( _( "You must be able to see the target that you select." ) );
    }

    if( !errors ) {
        for( const std::string &pos_om : bounce_locations ) {
            if( bounce && omt_ref.id().c_str() == pos_om && range > 5 ) {
                if( query_yn( _( "Do you want to bounce off this location to extend range?" ) ) ) {
                    om_line_mark( omt_pos, omt_tgt );
                    tripoint dest = om_target_tile( omt_tgt, 2, range * .75, possible_om_types,
                                                    true, false, omt_tgt, true );
                    om_line_mark( omt_pos, omt_tgt, false );
                    return dest;
                }
            }
        }

        if( possible_om_types.empty() ) {
            return omt_tgt;
        }

        for( const std::string &pos_om : possible_om_types ) {
            if( omt_ref.id().c_str() == pos_om ) {
                return omt_tgt;
            }
        }
    }

    return om_target_tile( omt_pos, min_range, range, possible_om_types );
}

void om_range_mark( const tripoint &origin, int range, bool add_notes,
                    const std::string &message )
{
    std::vector<tripoint> note_pts;
    //North Limit
    for( int x = origin.x - range; x < origin.x + range + 1; x++ ) {
        note_pts.push_back( tripoint( x, origin.y - range, origin.z ) );
    }
    //South
    for( int x = origin.x - range; x < origin.x + range + 1; x++ ) {
        note_pts.push_back( tripoint( x, origin.y + range, origin.z ) );
    }
    //West
    for( int y = origin.y - range; y < origin.y + range + 1; y++ ) {
        note_pts.push_back( tripoint( origin.x - range, y, origin.z ) );
    }
    //East
    for( int y = origin.y - range; y < origin.y + range + 1; y++ ) {
        note_pts.push_back( tripoint( origin.x + range, y, origin.z ) );
    }

    for( auto pt : note_pts ) {
        if( add_notes ) {
            if( !overmap_buffer.has_note( pt ) ) {
                overmap_buffer.add_note( pt, message );
            }
        } else {
            if( overmap_buffer.has_note( pt ) && overmap_buffer.note( pt ) == message ) {
                overmap_buffer.delete_note( pt );
            }
        }
    }
}

void om_line_mark( const tripoint &origin, const tripoint &dest, bool add_notes,
                   const std::string &message )
{
    std::vector<tripoint> note_pts = line_to( origin, dest );

    for( auto pt : note_pts ) {
        if( add_notes ) {
            if( !overmap_buffer.has_note( pt ) ) {
                overmap_buffer.add_note( pt, message );
            }
        } else {
            if( overmap_buffer.has_note( pt ) && overmap_buffer.note( pt ) == message ) {
                overmap_buffer.delete_note( pt );
            }
        }
    }
}

std::string get_mission_action_string( const std::string &input_mission )
{
    const base_camps::miss_data &miss_info = base_camps::miss_info[ input_mission ];
    return miss_info.action.translated();

}

bool om_set_hide_site( npc &comp, const tripoint &omt_tgt,
                       const std::vector<item *> &itms,
                       const std::vector<item *> &itms_rem )
{
    tinymap target_bay;
    target_bay.load( tripoint( omt_tgt.x * 2, omt_tgt.y * 2, omt_tgt.z ), false );
    target_bay.ter_set( point( 11, 10 ), t_improvised_shelter );
    for( auto i : itms_rem ) {
        comp.companion_mission_inv.add_item( *i );
        target_bay.i_rem( point( 11, 10 ), i );
    }
    for( auto i : itms ) {
        target_bay.add_item_or_charges( point( 11, 10 ), *i );
        g->u.use_amount( i->typeId(), 1 );
    }
    target_bay.save();

    overmap_buffer.ter_set( omt_tgt, oter_id( "faction_hide_site_0" ) );

    overmap_buffer.reveal( omt_tgt.xy(), 3, 0 );
    return true;
}

// path and travel time
time_duration companion_travel_time_calc( const tripoint &omt_pos,
        const tripoint &omt_tgt, time_duration work, int trips, int haulage )
{
    std::vector<tripoint> journey = line_to( omt_pos, omt_tgt );
    return companion_travel_time_calc( journey, work, trips, haulage );
}

time_duration companion_travel_time_calc( const std::vector<tripoint> &journey,
        time_duration work, int trips, int haulage )
{
    int one_way = 0;
    for( auto &om : journey ) {
        const oter_id &omt_ref = overmap_buffer.ter( om );
        std::string om_id = omt_ref.id().c_str();
        //Player walks 1 om is roughly 30 seconds
        if( om_id == "field" ) {
            one_way += 30 + 30 * haulage;
        } else if( om_id == "forest" ) {
            one_way += 40 + 30 * haulage;
        } else if( om_id == "forest_thick" ) {
            one_way += 50 + 30 * haulage;
        } else if( om_id == "forest_water" ) {
            one_way += 60 + 30 * haulage;
        } else if( is_river( omt_ref ) ) {
            // hauling stuff over a river is slow, because you have to portage most items
            one_way += 200 + 40 * haulage;
        } else {
            one_way += 40 + 30 * haulage;
        }
    }
    return work + one_way * trips * 1_seconds;
}

int om_carry_weight_to_trips( units::mass mass, units::volume volume,
                              units::mass carry_mass, units::volume carry_volume )
{
    int trips_m = 1 + mass / carry_mass;
    int trips_v = 1 + volume / carry_volume;
    // return the number of round trips
    return 2 * std::max( trips_m, trips_v );
}

int om_carry_weight_to_trips( const std::vector<item *> &itms, npc_ptr comp )
{
    units::mass total_m = 0_gram;
    units::volume total_v = 0_ml;
    for( auto &i : itms ) {
        total_m += i->weight( true );
        total_v += i->volume( true );
    }
    units::mass max_m = comp ? comp->weight_capacity() - comp->weight_carried() : 30_kilogram;
    //Assume an additional pack will be carried in addition to normal gear
    units::volume sack_v = item( itype_id( "makeshift_sling" ) ).get_storage();
    units::volume max_v = comp ? comp->volume_capacity() - comp->volume_carried() : sack_v;
    max_v += sack_v;
    return om_carry_weight_to_trips( total_m, total_v, max_m, max_v );
}

std::vector<tripoint> om_companion_path( const tripoint &start, int range_start,
        bool bounce )
{
    std::vector<tripoint> scout_points;
    tripoint last = start;
    int range = range_start;
    int def_range = range_start;
    while( range > 3 ) {
        tripoint spt = om_target_tile( last, 0, range, {}, false, true, last, false );
        if( spt == tripoint( -999, -999, -999 ) ) {
            scout_points.clear();
            return scout_points;
        }
        if( last == spt ) {
            break;
        }
        std::vector<tripoint> note_pts = line_to( last, spt );
        scout_points.insert( scout_points.end(), note_pts.begin(), note_pts.end() );
        om_line_mark( last, spt );
        range -= rl_dist( spt.xy(), last.xy() );
        last = spt;

        const oter_id &omt_ref = overmap_buffer.ter( last );

        if( bounce && omt_ref.id() == "faction_hide_site_0" ) {
            range = def_range * .75;
            def_range = range;
        }
    }
    for( auto pt : scout_points ) {
        om_line_mark( pt, pt, false );
    }
    return scout_points;
}

// camp utility functions
// mission support functions
std::vector<item *> basecamp::give_equipment( std::vector<item *> equipment,
        const std::string &msg )
{
    std::vector<item *> equipment_lost;
    do {
        g->draw_ter();
        wrefresh( g->w_terrain );

        std::vector<std::string> names;
        names.reserve( equipment.size() );
        for( auto &i : equipment ) {
            names.push_back( i->tname() + " [" + to_string( i->charges ) + "]" );
        }

        // Choose item if applicable
        const int i_index = uilist( msg, names );
        if( i_index < 0 || static_cast<size_t>( i_index ) >= equipment.size() ) {
            return equipment_lost;
        }
        equipment_lost.push_back( equipment[i_index] );
        equipment.erase( equipment.begin() + i_index );
    } while( !equipment.empty() );
    return equipment_lost;
}

bool basecamp::validate_sort_points()
{
    auto &mgr = zone_manager::get_manager();
    if( g->m.check_vehicle_zones( g->get_levz() ) ) {
        mgr.cache_vzones();
    }
    tripoint src_loc = bb_pos + point_north;
    const tripoint abspos = g->m.getabs( g->u.pos() );
    if( !mgr.has_near( z_camp_storage, abspos, 60 ) || !mgr.has_near( z_camp_food, abspos, 60 ) ) {
        if( query_yn( _( "You do not have sufficient sort zones.  Do you want to add them?" ) ) ) {
            return set_sort_points();
        } else {
            return false;
        }
    } else {
        const std::unordered_set<tripoint> &src_set = mgr.get_near( z_camp_storage, abspos );
        const std::vector<tripoint> &src_sorted = get_sorted_tiles_by_distance( abspos, src_set );
        // Find the nearest unsorted zone to dump objects at
        for( auto &src : src_sorted ) {
            src_loc = g->m.getlocal( src );
            break;
        }
    }
    set_dumping_spot( g->m.getabs( src_loc ) );
    return true;
}

bool basecamp::set_sort_points()
{
    popup( _( "Sorting zones have changed.  Please create some sorting zones.  "
              "You must create a camp food zone, and a camp storage zone." ) );
    g->zones_manager();
    return validate_sort_points();
}

// camp analysis functions
std::vector<std::pair<std::string, tripoint>> talk_function::om_building_region(
            const tripoint &omt_pos, int range, bool purge )
{
    std::vector<std::pair<std::string, tripoint>> om_camp_region;
    for( const tripoint &omt_near_pos : points_in_radius( omt_pos, range ) ) {
        const oter_id &omt_rnear = overmap_buffer.ter( omt_near_pos );
        std::string om_rnear_id = omt_rnear.id().c_str();
        if( !purge || ( om_rnear_id.find( "faction_base_" ) != std::string::npos &&
                        om_rnear_id.find( "faction_base_camp" ) == std::string::npos ) ) {
            om_camp_region.push_back( std::make_pair( om_rnear_id, omt_near_pos ) );
        }
    }
    return om_camp_region;
}

point talk_function::om_simple_dir( const tripoint &omt_pos, const tripoint &omt_tar )
{
    return { clamp( omt_tar.x - omt_pos.x, -1, 1 ), clamp( omt_tar.y - omt_pos.y, -1, 1 ) };
}

// mission descriptions
std::string camp_trip_description( const time_duration &total_time,
                                   const time_duration &working_time,
                                   const time_duration &travel_time, int distance, int trips,
                                   int need_food )
{
    std::string entry = "\n";
    //A square is roughly 3 m
    int dist_m = distance * SEEX * 2 * 3;
    if( dist_m > 1000 ) {
        entry += string_format( _( ">Distance:%15.2f (km)\n" ), dist_m / 1000.0 );
        entry += string_format( _( ">One Way: %15d (trips)\n" ), trips );
        entry += string_format( _( ">Covered: %15.2f (km)\n" ), dist_m / 1000.0 * trips );
    } else {
        entry += string_format( _( ">Distance:%15d (m)\n" ), dist_m );
        entry += string_format( _( ">One Way: %15d (trips)\n" ), trips );
        entry += string_format( _( ">Covered: %15d (m)\n" ), dist_m * trips );
    }
    entry += string_format( _( ">Travel:  %s\n" ), right_justify( to_string( travel_time ), 23 ) );
    entry += string_format( _( ">Working: %s\n" ), right_justify( to_string( working_time ), 23 ) );
    entry += "----                   ----\n";
    entry += string_format( _( "Total:    %s\n" ), right_justify( to_string( total_time ), 23 ) );
    entry += string_format( _( "Food:     %15d (kcal)\n\n" ), need_food );
    return entry;
}

std::string basecamp::craft_description( const recipe_id &itm )
{
    const recipe &making = itm.obj();

    std::vector<std::string> component_print_buffer;
    int pane = FULL_SCREEN_WIDTH;
    const requirement_data &req = making.simple_requirements();
    auto tools = req.get_folded_tools_list( pane, c_white, _inv, 1 );
    auto comps = req.get_folded_components_list( pane, c_white, _inv,
                 making.get_component_filter(), 1 );

    component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
    component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

    std::string comp;
    for( auto &elem : component_print_buffer ) {
        comp = comp + elem + "\n";
    }
    comp = string_format( _( "Skill used: %s\nDifficulty: %d\n%s\nTime: %s\n" ),
                          making.skill_used.obj().name(), making.difficulty, comp,
                          to_string( base_camps::to_workdays( making.batch_duration() ) ) );
    return comp;
}

int basecamp::recruit_evaluation( int &sbase, int &sexpansions, int &sfaction, int &sbonus ) const
{
    auto e = expansions.find( base_camps::base_dir );
    if( e == expansions.end() ) {
        sbase = 0;
        sexpansions = 0;
        sfaction = 0;
        sbonus = 0;
        return 0;
    }
    sbase = e->second.cur_level * 5;
    sexpansions = expansions.size() * 2;

    //How could we ever starve?
    //More than 5 farms at recruiting base
    int farm = 0;
    for( const point &dir : directions ) {
        if( has_provides( "farming", dir ) ) {
            farm++;
        }
    }
    sfaction = std::min( camp_food_supply() / 10000, 10 );
    sfaction += std::min( camp_discipline() / 10, 5 );
    sfaction += std::min( camp_morale() / 10, 5 );

    //Secret or Hidden Bonus
    //Please avoid openly discussing so that there is some mystery to the system
    sbonus = 0;
    if( farm >= 5 ) {
        sbonus += 10;
    }
    //More machine than man
    //Bionics count > 10, respect > 75
    if( g->u.get_bionics().size() > 10 && camp_discipline() > 75 ) {
        sbonus += 10;
    }
    //Survival of the fittest
    if( g->get_kill_tracker().npc_kill_count() > 10 ) {
        sbonus += 10;
    }
    return sbase + sexpansions + sfaction + sbonus;
}
int basecamp::recruit_evaluation() const
{
    int sbase;
    int sexpansions;
    int sfaction;
    int sbonus;
    return recruit_evaluation( sbase, sexpansions, sfaction, sbonus );
}

std::string basecamp::recruit_description( int npc_count )
{
    int sbase;
    int sexpansions;
    int sfaction;
    int sbonus;
    int total = recruit_evaluation( sbase, sexpansions, sfaction, sbonus );
    std::string desc = string_format( _( "Notes:\n"
                                         "Recruiting additional followers is very dangerous and "
                                         "expensive.  The outcome is heavily dependent on the "
                                         "skill of the  companion you send and the appeal of "
                                         "your base.\n\n"
                                         "Skill used: speech\n"
                                         "Difficulty: 2\n"
                                         "Base Score:                   +%3d%%\n"
                                         "> Expansion Bonus:            +%3d%%\n"
                                         "> Faction Bonus:              +%3d%%\n"
                                         "> Special Bonus:              +%3d%%\n\n"
                                         "Total: Skill                  +%3d%%\n\n"
                                         "Risk: High\n"
                                         "Time: 4 Days\n"
                                         "Positions: %d/1\n" ), sbase, sexpansions, sfaction,
                                      sbonus, total, npc_count );
    return desc;
}

std::string basecamp::gathering_description( const std::string &bldg )
{
    std::string itemlist;
    if( item_group::group_is_defined( "gathering_" + bldg ) ) {
        itemlist = "gathering_" + bldg;
    } else {
        itemlist = "forest";
    }
    std::string output;

    // Functions like the debug item group tester but only rolls 6 times so the player
    // doesn't have perfect knowledge
    std::map<std::string, int> itemnames;
    for( size_t a = 0; a < 6; a++ ) {
        const auto items = item_group::items_from( itemlist, calendar::turn );
        for( auto &it : items ) {
            itemnames[it.display_name()]++;
        }
    }
    // Invert the map to get sorting!
    std::multimap<int, std::string> itemnames2;
    for( const auto &e : itemnames ) {
        itemnames2.insert( std::pair<int, std::string>( e.second, e.first ) );
    }
    for( const auto &e : itemnames2 ) {
        output = output + "> " + e.second + "\n";
    }
    return output;
}

std::string basecamp::farm_description( const tripoint &farm_pos, size_t &plots_count,
                                        farm_ops operation )
{
    std::pair<size_t, std::string> farm_data = farm_action( farm_pos, operation );
    std::string entry;
    plots_count = farm_data.first;
    switch( operation ) {
        case farm_ops::harvest:
            entry += _( "Harvestable: " ) + to_string( plots_count ) + "\n" + farm_data.second;
            break;
        case farm_ops::plant:
            entry += _( "Ready for Planting: " ) + to_string( plots_count ) + "\n";
            break;
        case farm_ops::plow:
            entry += _( "Needs Plowing: " ) + to_string( plots_count ) + "\n";
            break;
        default:
            debugmsg( "Farm operations called with no operation" );
            break;
    }
    return entry;
}

std::string camp_car_description( vehicle *car )
{
    std::string entry = string_format( _( "Name:     %s\n" ), right_justify( car->name, 25 ) );
    entry += _( "----          Engines          ----\n" );
    for( const vpart_reference &vpr : car->get_any_parts( "ENGINE" ) ) {
        const vehicle_part &pt = vpr.part();
        const vpart_info &vp = pt.info();
        entry += string_format( _( "Engine:   %s\n" ), right_justify( vp.name(), 25 ) );
        entry += string_format( _( ">Status:  %24d%%\n" ),
                                static_cast<int>( 100 * pt.health_percent() ) );
        entry += string_format( _( ">Fuel:    %s\n" ), right_justify( item::nname( vp.fuel_type ), 25 ) );
    }
    std::map<itype_id, int> fuels = car->fuels_left();
    entry += _( "----  Fuel Storage & Battery   ----\n" );
    for( auto &fuel : fuels ) {
        std::string fuel_entry = string_format( "%d/%d", car->fuel_left( fuel.first ),
                                                car->fuel_capacity( fuel.first ) );
        entry += string_format( ">%s:%s\n", item( fuel.first ).tname(),
                                right_justify( fuel_entry, 33 - utf8_width( item( fuel.first ).tname() ) ) );
    }
    for( auto &pt : car->parts ) {
        if( pt.is_battery() ) {
            const vpart_info &vp = pt.info();
            entry += string_format( ">%s:%*d%%\n", vp.name(), 32 - utf8_width( vp.name() ),
                                    static_cast<int>( 100.0 * pt.ammo_remaining() /
                                            pt.ammo_capacity() ) );
        }
    }
    entry += "\n";
    entry += _( "Estimated Chop Time:         5 Days\n" );
    return entry;
}

// food supply
int camp_food_supply( int change, bool return_days )
{
    faction *yours = g->u.get_faction();
    yours->food_supply += change;
    if( yours->food_supply < 0 ) {
        yours->likes_u += yours->food_supply / 1250;
        yours->respects_u += yours->food_supply / 625;
        yours->food_supply = 0;
    }
    if( return_days ) {
        return yours->food_supply / 2500;
    }

    return yours->food_supply;
}

int camp_food_supply( time_duration work )
{
    return camp_food_supply( -time_to_food( work ) );
}

int time_to_food( time_duration work )
{
    return 2500 * to_hours<int>( work ) / 24;
}

// mission support
bool basecamp::distribute_food()
{
    if( !validate_sort_points() ) {
        popup( _( "You do not have a camp food zone.  Aborting…" ) );
        return false;
    }

    auto &mgr = zone_manager::get_manager();
    if( g->m.check_vehicle_zones( g->get_levz() ) ) {
        mgr.cache_vzones();
    }
    const tripoint &abspos = get_dumping_spot();
    const std::unordered_set<tripoint> &z_food = mgr.get_near( z_camp_food, abspos, 60 );

    tripoint p_litter = omt_to_sm_copy( omt_pos ) + point( -7, 0 );

    bool has_food = false;
    for( const tripoint &p_food_stock_abs : z_food ) {
        const tripoint p_food_stock = g->m.getlocal( p_food_stock_abs );
        if( !g->m.i_at( p_food_stock ).empty() ) {
            has_food = true;
            break;
        }
    }
    if( !has_food ) {
        popup( _( "No items are located at the drop point…" ) );
        return false;
    }
    double quick_rot = 0.6 + ( has_provides( "pantry" ) ? 0.1 : 0 );
    double slow_rot = 0.8 + ( has_provides( "pantry" ) ? 0.05 : 0 );
    int total = 0;
    std::vector<item> keep_me;
    for( const tripoint &p_food_stock_abs : z_food ) {
        const tripoint p_food_stock = g->m.getlocal( p_food_stock_abs );
        map_stack initial_items = g->m.i_at( p_food_stock );
        for( item &i : initial_items ) {
            if( i.is_container() && i.get_contained().is_food() ) {
                auto comest = i.get_contained();
                i.contents.clear();
                //NPCs are lazy bastards who leave empties all around the camp fire
                tripoint litter_spread = p_litter;
                litter_spread.x += rng( -3, 3 );
                litter_spread.y += rng( -3, 3 );
                i.on_contents_changed();
                g->m.add_item_or_charges( litter_spread, i, false );
                i = comest;
            }
            if( i.is_comestible() && ( i.rotten() || i.get_comestible_fun() < -6 ) ) {
                keep_me.push_back( i );
            } else if( i.is_food() ) {
                double rot_multip;
                int rots_in = to_days<int>( time_duration::from_turns( i.spoilage_sort_order() ) );
                if( rots_in >= 5 ) {
                    rot_multip = 1.00;
                } else if( rots_in >= 2 ) {
                    rot_multip = slow_rot;
                } else {
                    rot_multip = quick_rot;
                }
                total += i.get_comestible()->default_nutrition.kcal * rot_multip * i.count();
            } else {
                keep_me.push_back( i );
            }
        }
        g->m.i_clear( p_food_stock );
        for( item &i : keep_me ) {
            g->m.add_item_or_charges( p_food_stock, i, false );
        }
        keep_me.clear();
    }

    popup( _( "You distribute %d kcal worth of food to your companions." ), total );
    camp_food_supply( total );
    return true;
}

// morale
int camp_discipline( int change )
{
    faction *yours = g->u.get_faction();
    yours->respects_u += change;
    return yours->respects_u;
}

int camp_morale( int change )
{
    faction *yours = g->u.get_faction();
    yours->likes_u += change;
    return yours->likes_u;
}

void basecamp::place_results( item result )
{
    if( by_radio ) {
        tinymap target_bay;
        target_bay.load( tripoint( omt_pos.x * 2, omt_pos.y * 2, omt_pos.z ), false );
        const tripoint &new_spot = target_bay.getlocal( get_dumping_spot() );
        target_bay.add_item_or_charges( new_spot, result, true );
        apply_camp_ownership( new_spot, 10 );
        target_bay.save();
    } else {
        auto &mgr = zone_manager::get_manager();
        if( g->m.check_vehicle_zones( g->get_levz() ) ) {
            mgr.cache_vzones();
        }
        const auto abspos = g->m.getabs( g->u.pos() );
        if( mgr.has_near( z_camp_storage, abspos ) ) {
            const auto &src_set = mgr.get_near( z_camp_storage, abspos );
            const auto &src_sorted = get_sorted_tiles_by_distance( abspos, src_set );
            // Find the nearest unsorted zone to dump objects at
            for( auto &src : src_sorted ) {
                const auto &src_loc = g->m.getlocal( src );
                g->m.add_item_or_charges( src_loc, result, true );
                apply_camp_ownership( src_loc, 10 );
                break;
            }
            //or dump them at players feet
        } else {
            g->m.add_item_or_charges( g->u.pos(), result, true );
            apply_camp_ownership( g->u.pos(), 0 );
        }
    }
}

void apply_camp_ownership( const tripoint &camp_pos, int radius )
{
    for( const tripoint &p : g->m.points_in_rectangle( camp_pos + point( -radius, -radius ),
            camp_pos + point( radius, radius ) ) ) {
        auto items = g->m.i_at( p.xy() );
        for( item &elem : items ) {
            elem.set_owner( g->u );
        }
    }
}

// combat and danger
// this entire system is stupid
bool survive_random_encounter( npc &comp, std::string &situation, int favor, int threat )
{
    popup( _( "While %s, a silent specter approaches %s…" ), situation, comp.name );
    int skill_1 = comp.get_skill_level( skill_survival );
    int skill_2 = comp.get_skill_level( skill_speech );
    if( skill_1 + favor > rng( 0, 10 ) ) {
        popup( _( "%s notices the antlered horror and slips away before it gets too close." ),
               comp.name );
        talk_function::companion_skill_trainer( comp, "gathering", 10_minutes, 10 - favor );
    } else if( skill_2 + favor > rng( 0, 10 ) ) {
        popup( _( "Another survivor approaches %s asking for directions." ), comp.name );
        popup( _( "Fearful that he may be an agent of some hostile faction, "
                  "%s doesn't mention the camp." ), comp.name );
        popup( _( "The two part on friendly terms and the survivor isn't seen again." ) );
        talk_function::companion_skill_trainer( comp, "recruiting", 10_minutes, 10 - favor );
    } else {
        popup( _( "%s didn't detect the ambush until it was too late!" ), comp.name );
        int skill = comp.get_skill_level( skill_melee ) +
                    0.5 * comp.get_skill_level( skill_survival ) +
                    comp.get_skill_level( skill_bashing ) +
                    comp.get_skill_level( skill_cutting ) +
                    comp.get_skill_level( skill_stabbing ) +
                    comp.get_skill_level( skill_unarmed ) + comp.get_skill_level( skill_dodge );
        int monsters = rng( 0, threat );
        if( skill * rng( 8, 12 ) > ( monsters * rng( 8, 12 ) ) ) {
            if( one_in( 2 ) ) {
                popup( _( "The bull moose charged %s from the tree line…" ), comp.name );
                popup( _( "Despite being caught off guard %s was able to run away until the "
                          "moose gave up pursuit." ), comp.name );
            } else {
                popup( _( "The jabberwock grabbed %s by the arm from behind and "
                          "began to scream." ), comp.name );
                popup( _( "Terrified, %s spun around and delivered a massive kick "
                          "to the creature's torso…" ), comp.name );
                popup( _( "Collapsing into a pile of gore, %s walked away unscathed…" ),
                       comp.name );
                popup( _( "(Sounds like bullshit, you wonder what really happened.)" ) );
            }
            talk_function::companion_skill_trainer( comp, "combat", 10_minutes, 10 - favor );
        } else {
            if( one_in( 2 ) ) {
                popup( _( "%s turned to find the hideous black eyes of a giant wasp "
                          "staring back from only a few feet away…" ), comp.name );
                popup( _( "The screams were terrifying, there was nothing anyone could do." ) );
            } else {
                popup( _( "Pieces of %s were found strewn across a few bushes." ), comp.name );
                popup( _( "(You wonder if your companions are fit to work on their own…)" ) );
            }
            overmap_buffer.remove_npc( comp.getID() );
            return false;
        }
    }
    return true;
}
