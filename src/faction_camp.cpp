#include "npc.h"
#include "output.h"
#include "game.h"
#include "map.h"
#include "mapbuffer.h"
#include "dialogue.h"
#include "rng.h"
#include "itype.h"
#include "line.h"
#include "bionics.h"
#include "debug.h"
#include "catacharset.h"
#include "messages.h"
#include "mission.h"
#include "ammo.h"
#include "output.h"
#include "overmap.h"
#include "overmap_ui.h"
#include "overmapbuffer.h"
#include "skill.h"
#include "translations.h"
#include "martialarts.h"
#include "input.h"
#include "item_group.h"
#include "compatibility.h"
#include "mapdata.h"
#include "recipe.h"
#include "requirements.h"
#include "map_iterator.h"
#include "mongroup.h"
#include "mtype.h"
#include "editmap.h"
#include "construction.h"
#include "coordinate_conversions.h"
#include "craft_command.h"
#include "iexamine.h"
#include "vehicle.h"
#include "veh_type.h"
#include "vpart_reference.h"
#include "vpart_range.h"
#include "requirements.h"
#include "string_input_popup.h"
#include "line.h"
#include "recipe_groups.h"
#include "faction_camp.h"
#include "mission_companion.h"
#include "npctalk.h"

#include <vector>
#include <string>
#include <sstream>
#include <algorithm>
#include <cassert>

const skill_id skill_dodge( "dodge" );
const skill_id skill_gun( "gun" );
const skill_id skill_unarmed( "unarmed" );
const skill_id skill_cutting( "cutting" );
const skill_id skill_stabbing( "stabbing" );
const skill_id skill_bashing( "bashing" );
const skill_id skill_melee( "melee" );
const skill_id skill_fabrication( "fabrication" );
const skill_id skill_survival( "survival" );
const skill_id skill_mechanics( "mechanics" );
const skill_id skill_electronics( "electronics" );
const skill_id skill_firstaid( "firstaid" );
const skill_id skill_speech( "speech" );
const skill_id skill_tailor( "tailor" );
const skill_id skill_cooking( "cooking" );
const skill_id skill_traps( "traps" );
const skill_id skill_archery( "archery" );
const skill_id skill_swimming( "swimming" );

static const int COMPANION_SORT_POINTS = 12;

void talk_function::camp_missions( mission_data &mission_key, npc &p )
{
    std::vector<std::shared_ptr<npc>> npc_list;
    std::string entry;

    //Used to determine what kind of OM the NPC is sitting in to determine the missions and upgrades
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, p.posz() );
    std::string om_cur = omt_ref.id().c_str();
    std::string bldg;
    std::vector<std::pair<std::string, tripoint>> om_expansions = om_building_region( p, 1, true );

    bldg = om_next_upgrade( om_cur );

    if( bldg != "null" ) {
        mission_key.text["Upgrade Camp"] = om_upgrade_description( bldg );
        bool avail = companion_list( p, "_faction_upgrade_camp" ).empty();
        mission_key.push( "Upgrade Camp", _( "Upgrade Camp" ), "", false, avail );
    }

    if( om_min_level( "faction_base_camp_1", om_cur ) ) {
        mission_key.text["Gather Materials"] = om_gathering_description( p, bldg );
        bool avail = ( companion_list( p, "_faction_camp_gathering" ).size() < 3 );
        mission_key.push( "Gather Materials", _( "Gather Materials" ), "", false,
                          avail );

        mission_key.text["Distribute Food"] = string_format( _( "Notes:\n"
                                              "Distribute food to your follower and fill you larders.  Place the food you wish to distribute opposite the tent door between "
                                              "the manager and wall.\n \n"
                                              "Effects:\n"
                                              "> Increases your faction's food supply value which in turn is used to pay laborers for their time\n \n"
                                              "Must have enjoyability >= -6\n"
                                              "Perishable food liquidated at penalty depending on upgrades and rot time:\n"
                                              "> Rotten: 0%%\n"
                                              "> Rots in < 2 days: 60%%\n"
                                              "> Rots in < 5 days: 80%%\n \n"
                                              "Total faction food stock: %d kcal or %d day's rations" ), 10 * camp_food_supply(),
                                              camp_food_supply( 0, true ) );
        mission_key.push( "Distribute Food", _( "Distribute Food" ) );

        mission_key.text["Reset Sort Points"] = string_format( _( "Notes:\n"
                                                "Reset the points that items are sorted to using the [ Menial Labor ] mission.\n \n"
                                                "Effects:\n"
                                                "> Assignable Points: food, food for distribution, seeds, weapons, clothing, bionics, "
                                                "all kinds of tools, wood, trash, books, medication, and ammo.\n"
                                                "> Items sitting on any type of furniture will not be moved.\n"
                                                "> Items that are not listed in one of the categories are defaulted to the tools group."
                                                                ) );
        mission_key.push( "Reset Sort Points", _( "Reset Sort Points" ), "", false );
    }

    if( om_min_level( "faction_base_camp_2", om_cur ) ) {
        mission_key.text["Collect Firewood"] = string_format( _( "Notes:\n"
                                               "Send a companion to gather light brush and heavy sticks.\n \n"
                                               "Skill used: survival\n"
                                               "Difficulty: N/A \n"
                                               "Gathering Possibilities:\n"
                                               "> heavy sticks\n"
                                               "> withered plants\n"
                                               "> splintered wood\n \n"
                                               "Risk: Very Low\n"
                                               "Time: 3 Hours, Repeated\n"
                                               "Positions: %d/3\n" ), companion_list( p, "_faction_camp_firewood" ).size()
                                                            );
        bool avail = ( companion_list( p, "_faction_camp_firewood" ).size() < 3 );
        mission_key.push( "Collect Firewood", _( "Collect Firewood" ), "", false, avail );
    }

    if( om_min_level( "faction_base_camp_3", om_cur ) ) {
        mission_key.text["Menial Labor"] = string_format( _( "Notes:\n"
                                           "Send a companion to do low level chores and sort supplies.\n \n"
                                           "Skill used: fabrication\n"
                                           "Difficulty: N/A \n"
                                           "Effects:\n"
                                           "> Material left outside on the ground will be sorted into the four crates in front of the tent.\n"
                                           "Default, top to bottom:  Clothing, Food, Books/Bionics, and Tools.  Wood will be piled to the south.  Trash to the north.\n \n"
                                           "Risk: None\n"
                                           "Time: 3 Hours\n"
                                           "Positions: %d/1\n" ), companion_list( p, "_faction_camp_menial" ).size()
                                                        );
        bool avail = companion_list( p, "_faction_camp_menial" ).empty();
        mission_key.push( "Menial Labor", _( "Menial Labor" ), "", false, avail );
    }

    if( ( ( om_min_level( "faction_base_camp_4", om_cur ) &&
            om_expansions.empty() ) ||
          ( om_min_level( "faction_base_camp_6", om_cur ) && om_expansions.size() < 2 ) ||
          ( om_min_level( "faction_base_camp_8", om_cur ) && om_expansions.size() < 3 ) ||
          ( om_min_level( "faction_base_camp_10", om_cur ) && om_expansions.size() < 4 ) ||
          ( om_min_level( "faction_base_camp_12", om_cur ) && om_expansions.size() < 5 ) ||
          ( om_min_level( "faction_base_camp_14", om_cur ) && om_expansions.size() < 6 ) ||
          ( om_min_level( "faction_base_camp_16", om_cur ) && om_expansions.size() < 7 ) ||
          ( om_min_level( "faction_base_camp_18", om_cur ) && om_expansions.size() < 8 ) ) ) {
        mission_key.text["Expand Base"] = string_format( _( "Notes:\n"
                                          "Your base has become large enough to support an expansion.  Expansions open up new opportunities "
                                          "but can be expensive and time consuming.  Pick them carefully, only 8 can be built at each camp.\n \n"
                                          "Skill used: fabrication\n"
                                          "Difficulty: N/A \n"
                                          "Effects:\n"
                                          "> Choose any one of the available expansions.  Starting with a farm or lumberyard are always a solid choice "
                                          "since food is used to support companion missions and wood is your primary construction material.\n \n"
                                          "Risk: None\n"
                                          "Time: 3 Hours \n"
                                          "Positions: %d/1\n" ), companion_list( p, "_faction_camp_expansion" ).size()
                                                       );
        bool avail = companion_list( p, "_faction_camp_expansion" ).empty();
        mission_key.push( "Expand Base", _( "Expand Base" ), "", false, avail );
    }

    if( om_min_level( "faction_base_camp_5", om_cur ) ) {
        mission_key.text["Cut Logs"] = string_format( _( "Notes:\n"
                                       "Send a companion to a nearby forest to cut logs.\n \n"
                                       "Skill used: fabrication\n"
                                       "Difficulty: 1 \n"
                                       "Effects:\n"
                                       "> 50%% of trees/trunks at the forest position will be cut down.\n"
                                       "> 50%% of total material will be brought back.\n"
                                       "> Repeatable with diminishing returns.\n \n"
                                       "Risk: Low-Medium\n"
                                       "Time: 6 Hour Base + Travel Time + Cutting Time\n"
                                       "Positions: %d/1\n" ), companion_list( p, "_faction_camp_cut_log" ).size()
                                                    );
        bool avail = companion_list( p, "_faction_camp_cut_log" ).empty();
        mission_key.push( "Cut Logs", _( "Cut Logs" ), "", false, avail );
    }

    if( om_min_level( "faction_base_camp_7", om_cur ) ) {
        mission_key.text["Setup Hide Site"] = string_format( _( "Notes:\n"
                                              "Send a companion to build an improvised shelter and stock it with equipment at a distant map location.\n \n"
                                              "Skill used: survival\n"
                                              "Difficulty: 3\n"
                                              "Effects:\n"
                                              "> Good for setting up resupply or contingency points.\n"
                                              "> Gear is left unattended and could be stolen.\n"
                                              "> Time dependent on weight of equipment being sent forward.\n \n"
                                              "Risk: Medium\n"
                                              "Time: 6 Hour Construction + Travel\n"
                                              "Positions: %d/1\n" ), companion_list( p, "_faction_camp_hide_site" ).size()
                                                           );
        bool avail = companion_list( p, "_faction_camp_hide_site" ).empty();
        mission_key.push( "Setup Hide Site", _( "Setup Hide Site" ), "", false, avail );
    }

    if( om_min_level( "faction_base_camp_7", om_cur ) ) {
        mission_key.text["Relay Hide Site"] = string_format( _( "Notes:\n"
                                              "Push gear out to a hide site or bring gear back from one.\n \n"
                                              "Skill used: survival\n"
                                              "Difficulty: 1\n"
                                              "Effects:\n"
                                              "> Good for returning equipment you left in the hide site shelter.\n"
                                              "> Gear is left unattended and could be stolen.\n"
                                              "> Time dependent on weight of equipment being sent forward or back.\n \n"
                                              "Risk: Medium\n"
                                              "Time: 1 Hour Base + Travel\n"
                                              "Positions: %d/1\n" ), companion_list( p, "_faction_camp_hide_trans" ).size()
                                                           );
        bool avail = companion_list( p, "_faction_camp_hide_trans" ).empty();
        mission_key.push( "Relay Hide Site", _( "Relay Hide Site" ), "", false, avail );
    }

    if( om_min_level( "faction_base_camp_9", om_cur ) ) {
        mission_key.text["Construct Map Fortifications"] =
            om_upgrade_description( "faction_wall_level_N_0" );
        mission_key.push( "Construct Map Fortifications",
                          _( "Construct Map Fortifications" ), "", false );
        mission_key.text["Construct Spiked Trench"] = om_upgrade_description( "faction_wall_level_N_1" );
        mission_key.push( "Construct Spiked Trench", _( "Construct Spiked Trench" ), "", false );
    }

    //This handles all crafting by the base, regardless of level
    std::map<std::string, std::string> craft_r = camp_recipe_deck( om_cur );
    inventory found_inv = g->u.crafting_inventory();
    std::string dr = "[B]";
    if( companion_list( p, "_faction_camp_crafting_" + dr ).empty() ) {
        for( std::map<std::string, std::string>::const_iterator it = craft_r.begin(); it != craft_r.end();
             ++it ) {
            std::string title_e = dr + it->first;
            mission_key.text[title_e] = om_craft_description( it->second );

            const recipe *recp = &recipe_id( it->second ).obj();
            bool craftable = recp->requirements().can_make_with_inventory( found_inv, 1 );

            mission_key.push( title_e, "", dr, false, craftable );

        }
    }

    if( om_min_level( "faction_base_camp_11", om_cur ) ) {
        mission_key.text["Recruit Companions"] = camp_recruit_evaluation( p, om_cur, om_expansions );
        bool avail = companion_list( p, "_faction_camp_recruit_0" ).empty();
        mission_key.push( "Recruit Companions", _( "Recruit Companions" ), "", false, avail );
    }

    if( om_min_level( "faction_base_camp_13", om_cur ) ) {
        mission_key.text["Scout Mission"] =  string_format( _( "Notes:\n"
                                             "Send a companion out into the great unknown.  High survival skills are needed to avoid combat but "
                                             "you should expect an encounter or two.\n \n"
                                             "Skill used: survival\n"
                                             "Difficulty: 3\n"
                                             "Effects:\n"
                                             "> Select checkpoints to customize path.\n"
                                             "> Reveals terrain around the path.\n"
                                             "> Can bounce off hide sites to extend range.\n \n"
                                             "Risk: High\n"
                                             "Time: Travel\n"
                                             "Positions: %d/3\n" ), companion_list( p, "_faction_camp_scout_0" ).size()
                                                          );
        bool avail = companion_list( p, "_faction_camp_scout_0" ).size() < 3;
        mission_key.push( "Scout Mission", _( "Scout Mission" ), "", false, avail );
    }

    if( om_min_level( "faction_base_camp_15", om_cur ) ) {
        mission_key.text["Combat Patrol"] =  string_format( _( "Notes:\n"
                                             "Send a companion to purge the wasteland.  Their goal is to kill anything hostile they encounter and return when "
                                             "their wounds are too great or the odds are stacked against them.\n \n"
                                             "Skill used: survival\n"
                                             "Difficulty: 4\n"
                                             "Effects:\n"
                                             "> Pulls creatures encountered into combat instead of fleeing.\n"
                                             "> Select checkpoints to customize path.\n"
                                             "> Can bounce off hide sites to extend range.\n \n"
                                             "Risk: Very High\n"
                                             "Time: Travel\n"
                                             "Positions: %d/3\n" ), companion_list( p, "_faction_camp_combat_0" ).size()
                                                          );
        bool avail = companion_list( p, "_faction_camp_combat_0" ).size() < 3;
        mission_key.push( "Combat Patrol", _( "Combat Patrol" ), "", false, avail );
    }

    //This starts all of the expansion missions
    for( const auto &e : om_expansions ) {
        std::string dr = om_simple_dir( omt_pos, e.second );

        oter_id &omt_ref_exp = overmap_buffer.ter( e.second.x, e.second.y, p.posz() );
        std::string om_cur_exp = omt_ref_exp.id().c_str();
        std::string bldg_exp;

        bldg_exp = om_next_upgrade( om_cur_exp );

        if( bldg_exp != "null" ) {
            std::string title_e = dr + " Expansion Upgrade";

            mission_key.text[title_e] = om_upgrade_description( bldg_exp );
            mission_key.push( title_e, dr + _( " Expansion Upgrade" ), dr );
        }

        if( om_min_level( "faction_base_garage_1", om_cur_exp ) ) {
            std::string title_e = dr + " Chop Shop";
            mission_key.text[title_e] = _( "Notes:\n"
                                           "Have a companion attempt to completely dissemble a vehicle into components.\n \n"
                                           "Skill used: mechanics\n"
                                           "Difficulty: 2 \n"
                                           "Effects:\n"
                                           "> Removed parts placed on the furniture in the garage.\n"
                                           "> Skill plays a huge role to determine what is salvaged.\n \n"
                                           "Risk: None\n"
                                           "Time: Skill Based \n" );
            bool avail = companion_list( p, "_faction_exp_chop_shop_" + dr ).empty();
            mission_key.push( title_e, dr + _( " Chop Shop" ), dr, false, avail );
        }

        std::map<std::string, std::string> cooking_recipes = camp_recipe_deck( om_cur_exp );
        if( om_min_level( "faction_base_kitchen_1", om_cur_exp ) ) {
            inventory found_inv = g->u.crafting_inventory();
            if( companion_list( p, "_faction_exp_kitchen_cooking_" + dr ).empty() ) {
                for( std::map<std::string, std::string>::const_iterator it = cooking_recipes.begin();
                     it != cooking_recipes.end(); ++it ) {
                    std::string title_e = dr + it->first;
                    mission_key.text[title_e] = om_craft_description( it->second );
                    const recipe *recp = &recipe_id( it->second ).obj();
                    bool craftable = recp->requirements().can_make_with_inventory( found_inv, 1 );
                    mission_key.push( title_e, "", dr, false, craftable );

                }
            }
        }

        if( om_min_level( "faction_base_blacksmith_1", om_cur_exp ) ) {
            inventory found_inv = g->u.crafting_inventory();
            if( companion_list( p, "_faction_exp_blacksmith_crafting_" + dr ).empty() ) {
                for( std::map<std::string, std::string>::const_iterator it = cooking_recipes.begin();
                     it != cooking_recipes.end(); ++it ) {
                    std::string title_e = dr + it->first;
                    mission_key.text[title_e] = om_craft_description( it->second );
                    const recipe *recp = &recipe_id( it->second ).obj();
                    bool craftable = recp->requirements().can_make_with_inventory( found_inv, 1 );
                    mission_key.push( title_e, "", dr, false, craftable );

                }
            }
        }

        if( om_min_level( "faction_base_farm_1", om_cur_exp ) ) {
            if( companion_list( p, "_faction_exp_plow_" + dr ).empty() ) {
                std::string title_e = dr + " Plow Fields";
                mission_key.text[title_e] = _( "Notes:\n"
                                               "Plow any spaces that have reverted to dirt or grass.\n \n" ) +
                                            camp_farm_description( point( e.second.x, e.second.y ), false, false, true ) + _( "\n \n"
                                                    "Skill used: fabrication\n"
                                                    "Difficulty: N/A \n"
                                                    "Effects:\n"
                                                    "> Restores only the plots created in the last expansion upgrade.\n"
                                                    "> Does not damage existing crops.\n \n"
                                                    "Risk: None\n"
                                                    "Time: 5 Min / Plot \n"
                                                    "Positions: 0/1 \n" );
                mission_key.push( title_e, dr + _( " Plow Fields" ), dr );
            }

            if( companion_list( p, "_faction_exp_plant_" + dr ).empty() &&
                g->get_temperature( g-> u.pos() ) > 50 ) {
                std::string title_e = dr + " Plant Fields";
                mission_key.text[title_e] = _( "Notes:\n"
                                               "Plant designated seeds in the spaces that have already been tilled.\n \n" ) +
                                            camp_farm_description( point( e.second.x, e.second.y ), false, true, false ) + _( "\n \n"
                                                    "Skill used: survival\n"
                                                    "Difficulty: N/A \n"
                                                    "Effects:\n"
                                                    "> Choose which seed type or all of your seeds.\n"
                                                    "> Stops when out of seeds or planting locations.\n"
                                                    "> Will plant in ALL dirt mounds in the expansion.\n \n"
                                                    "Risk: None\n"
                                                    "Time: 1 Min / Plot \n"
                                                    "Positions: 0/1 \n" );
                mission_key.push( title_e, dr + _( " Plant Fields" ), dr );
            }
            if( companion_list( p, "_faction_exp_harvest_" + dr ).empty() ) {
                std::string title_e = dr + " Harvest Fields";
                mission_key.text[title_e] = _( "Notes:\n"
                                               "Harvest any plants that are ripe and bring the produce back.\n \n" ) +
                                            camp_farm_description( point( e.second.x, e.second.y ), true, false, false ) + _( "\n \n"
                                                    "Skill used: survival\n"
                                                    "Difficulty: N/A \n"
                                                    "Effects:\n"
                                                    "> Will dump all harvesting products onto your location.\n \n"
                                                    "Risk: None\n"
                                                    "Time: 3 Min / Plot \n"
                                                    "Positions: 0/1 \n" );
                mission_key.push( title_e, dr + _( " Harvest Fields" ), dr );
            }
        }

        if( om_min_level( "faction_base_farm_4", om_cur_exp ) ) {
            inventory found_inv = g->u.crafting_inventory();
            if( companion_list( p, "_faction_exp_farm_crafting_" + dr ).empty() ) {
                for( std::map<std::string, std::string>::const_iterator it = cooking_recipes.begin();
                     it != cooking_recipes.end(); ++it ) {
                    std::string title_e = dr + it->first;
                    mission_key.text[title_e] = om_craft_description( it->second );

                    const recipe *recp = &recipe_id( it->second ).obj();
                    bool craftable = recp->requirements().can_make_with_inventory( found_inv, 1 );

                    mission_key.push( title_e, "", dr, false, craftable );

                }
            }
        }
    }

    npc_list = companion_list( p, "_faction_upgrade_camp" );
    if( !npc_list.empty() ) {
        entry = _( "Working to expand your camp!\n" );
        for( auto &elem : npc_list ) {
            int hour_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
            int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
            if( hour_left > 1 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( hour_left )
                        + _( " hours left] \n" );
            } else if( min_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + _( " minutes left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
        mission_key.text["Recover Ally from Upgrading"] = entry;
        mission_key.push( "Recover Ally from Upgrading", _( "Recover Ally from Upgrading" ), "", true );
    }

    npc_list = companion_list( p, "_faction_upgrade_exp_", true );
    if( !npc_list.empty() ) {
        for( auto &elem : npc_list ) {
            const recipe *making_exp;
            entry = _( "Working to upgrade your expansions!\n" );
            //To find what building we want to create next, get direction it is from camp
            std::string dir = camp_direction( elem->get_companion_mission().mission_id );
            std::string bldg_exp;
            for( const auto &e : om_expansions ) {
                //Find the expansion that is in that direction
                if( dir == om_simple_dir( omt_pos, e.second ) ) {
                    oter_id &omt_ref_exp = overmap_buffer.ter( e.second.x, e.second.y, e.second.z );
                    std::string om_exp = omt_ref_exp.id().c_str();
                    //Determine the next upgrade for the building
                    bldg_exp = om_next_upgrade( om_exp );
                    break;
                }
            }
            making_exp = &recipe_id( bldg_exp ).obj();
            entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + "/" + to_string( to_hours<int>( time_duration::from_turns( making_exp->time / 100 ) ) ) +
                    _( " hours] \n" );

            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            mission_key.text["Recover Ally, " + dir + " Expansion"] = entry;
            mission_key.push( "Recover Ally, " + dir + " Expansion",
                              _( "Recover Ally, " ) + dir + _( " Expansion" ), dir, true );
        }
    }

    npc_list = companion_list( p, "_faction_exp_chop_shop_", true );
    if( !npc_list.empty() ) {
        for( auto &elem : npc_list ) {
            entry = _( "Working at the chop shop...\n" );
            //~48 hours = 192 plots * 5 min plow time
            entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + _( "/120 hours] \n" );
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            std::string dir = camp_direction( elem->get_companion_mission().mission_id );
            mission_key.text[ dir + " (Finish) Chop Shop" ] = entry;
            mission_key.push( dir + " (Finish) Chop Shop", dir + _( " (Finish) Chop Shop" ), dir, true );
        }
    }

    npc_list = companion_list( p, "_faction_exp_kitchen_cooking_", true );
    if( !npc_list.empty() ) {
        for( auto &elem : npc_list ) {
            entry = _( "Working in your kitchen!\n" );
            int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
            float sec_left = to_turns<float>( elem->companion_mission_time_ret - calendar::turn );
            if( min_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + _( " minutes left] \n" );
            } else if( sec_left > 0 ) {
                entry = entry + "  " + elem->name + _( " [ALMOST DONE]\n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            std::string dir = camp_direction( elem->get_companion_mission().mission_id );
            mission_key.text[ dir + " (Finish) Cooking" ] = entry;
            mission_key.push( dir + " (Finish) Cooking", dir + _( " (Finish) Cooking" ), dir, true );
        }
    }

    npc_list = companion_list( p, "_faction_exp_blacksmith_crafting_", true );
    if( !npc_list.empty() ) {
        for( auto &elem : npc_list ) {
            entry = _( "Working in your blacksmith shop!\n" );
            int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
            float sec_left = to_turns<float>( elem->companion_mission_time_ret - calendar::turn );
            if( min_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + _( " minutes left] \n" );
            } else if( sec_left > 0 ) {
                entry = entry + "  " + elem->name + _( " [ALMOST DONE]\n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            std::string dir = camp_direction( elem->get_companion_mission().mission_id );
            mission_key.text[ dir + " (Finish) Smithing" ] = entry;
            mission_key.push( dir + " (Finish) Smithing", dir + _( " (Finish) Smithing" ), dir, true );
        }
    }

    npc_list = companion_list( p, "_faction_exp_plow_", true );
    if( !npc_list.empty() ) {
        for( auto &elem : npc_list ) {
            entry = _( "Working to plow your fields!\n" );
            //~48 hours = 192 plots * 5 min plow time
            entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + _( "/~48 hours] \n" );
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            std::string dir = camp_direction( elem->get_companion_mission().mission_id );
            mission_key.text[ dir + " (Finish) Plow Fields" ] = entry;
            mission_key.push( dir + " (Finish) Plow Fields", dir + _( " (Finish) Plow Fields" ), dir, true );
        }
    }

    npc_list = companion_list( p, "_faction_exp_plant_", true );
    if( !npc_list.empty() ) {
        for( auto &elem : npc_list ) {
            entry = _( "Working to plant your fields!\n" );
            //~3.5 hours = 192 plots * 1 min plant time
            entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + _( "/4 hours] \n" );
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            std::string dir = camp_direction( elem->get_companion_mission().mission_id );
            mission_key.text[ dir + " (Finish) Plant Fields" ] = entry;
            mission_key.push( dir + " (Finish) Plant Fields", dir + _( " (Finish) Plant Fields" ), dir, true );
        }
    }

    npc_list = companion_list( p, "_faction_exp_harvest_", true );
    if( !npc_list.empty() ) {
        for( auto &elem : npc_list ) {
            entry = _( "Working to harvest your fields!\n" );
            //~48 hours = 192 plots * 3 min harvest and carry time
            entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + _( "/~10 hours] \n" );
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            std::string dir = camp_direction( elem->get_companion_mission().mission_id );
            mission_key.text[ dir + " (Finish) Harvest Fields" ] = entry;
            mission_key.push( dir + " (Finish) Harvest Fields", dir + _( " (Finish) Harvest Fields" ), dir,
                              true );
        }
    }

    npc_list = companion_list( p, "_faction_exp_farm_crafting_", true );
    if( !npc_list.empty() ) {
        for( auto &elem : npc_list ) {
            entry = _( "Working on your farm!\n" );
            int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
            if( min_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + _( " minutes left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            std::string dir = camp_direction( elem->get_companion_mission().mission_id );
            mission_key.text[ dir + " (Finish) Crafting" ] = entry;
            mission_key.push( dir + " (Finish) Crafting", dir + _( " (Finish) Crafting" ), dir, true );
        }
    }

    npc_list = companion_list( p, "_faction_camp_crafting_", true );
    if( !npc_list.empty() ) {
        for( auto &elem : npc_list ) {
            entry = _( "Busy crafting!\n" );
            int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
            if( min_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + _( " minutes left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
            entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
            std::string dir = camp_direction( elem->get_companion_mission().mission_id );
            mission_key.text[ dir + " (Finish) Crafting" ] = entry;
            mission_key.push( dir + " (Finish) Crafting", dir + _( " (Finish) Crafting" ), dir, true );
        }
    }

    npc_list = companion_list( p, "_faction_camp_gathering" );
    if( !npc_list.empty() ) {
        entry = _( "Searching for materials to upgrade the camp.\n" );
        for( auto &elem : npc_list ) {
            entry = entry + "  " + elem->name + " [" + to_string( to_hours<int>( calendar::turn -
                    elem->companion_mission_time ) ) + _( "/3 hours] \n" );
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
        mission_key.text["Recover Ally from Gathering"] = entry;
        mission_key.push( "Recover Ally from Gathering", _( "Recover Ally from Gathering" ), "", true );
    }
    npc_list = companion_list( p, "_faction_camp_firewood" );
    if( !npc_list.empty() ) {
        entry = _( "Searching for firewood.\n" );
        for( auto &elem : npc_list ) {
            entry = entry + "  " + elem->name + " [" + to_string( to_hours<int>( calendar::turn -
                    elem->companion_mission_time ) ) + _( "/3 hours] \n" );
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
        mission_key.text["Recover Firewood Gatherers"] = entry;
        mission_key.push( "Recover Firewood Gatherers", _( "Recover Firewood Gatherers" ), "", true );
    }
    npc_list = companion_list( p, "_faction_camp_menial" );
    if( !npc_list.empty() ) {
        entry = _( "Performing menial labor...\n" );
        for( auto &elem : npc_list ) {
            int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
            if( min_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + _( " minutes left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?\n" );
        mission_key.text["Recover Menial Laborer"] = entry;
        mission_key.push( "Recover Menial Laborer", _( "Recover Menial Laborer" ), "", true );
    }
    npc_list = companion_list( p, "_faction_camp_expansion" );
    if( !npc_list.empty() ) {
        entry = _( "Surveying for expansion...\n" );
        for( auto &elem : npc_list ) {
            int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
            if( hrs_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + _( " hours left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?\n" );
        mission_key.text["Recover Surveyor"] = entry;
        mission_key.push( "Recover Surveyor", _( "Recover Surveyor" ), "", true );
    }

    npc_list = companion_list( p, "_faction_camp_cut_log" );
    if( !npc_list.empty() ) {
        entry = _( "Cutting logs in the woods...\n" );
        for( auto &elem : npc_list ) {
            int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
            if( hrs_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + _( " hours left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?\n" );
        mission_key.text["Recover Log Cutter"] = entry;
        mission_key.push( "Recover Log Cutter", _( "Recover Log Cutter" ), "", true );
    }

    npc_list = companion_list( p, "_faction_camp_hide_site" );
    if( !npc_list.empty() ) {
        entry = _( "Setting up a hide site...\n" );
        for( auto &elem : npc_list ) {
            int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
            if( hrs_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + _( " hours left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?\n" );
        mission_key.text["Recover Hide Setup"] = entry;
        mission_key.push( "Recover Hide Setup", _( "Recover Hide Setup" ), "", true );
    }

    npc_list = companion_list( p, "_faction_camp_om_fortifications" );
    if( !npc_list.empty() ) {
        entry = _( "Constructing fortifications...\n" );
        for( auto &elem : npc_list ) {
            int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
            if( hrs_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + _( " hours left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?\n" );
        mission_key.text["Finish Map Fortifications"] = entry;
        mission_key.push( "Finish Map Fortifications", _( "Finish Map Fortifications" ), "", true );
    }

    npc_list = companion_list( p, "_faction_camp_recruit_0" );
    if( !npc_list.empty() ) {
        entry = _( "Searching for recruits.\n" );
        for( auto &elem : npc_list ) {
            int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
            if( hrs_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + _( " hours left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
        mission_key.text["Recover Recruiter"] = entry;
        mission_key.push( "Recover Recruiter", _( "Recover Recruiter" ), "", true );
    }

    npc_list = companion_list( p, "_faction_camp_scout_0" );
    if( !npc_list.empty() ) {
        entry = _( "Scouting the region.\n" );
        for( auto &elem : npc_list ) {
            int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
            if( hrs_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + _( " hours left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
        mission_key.text["Recover Scout"] = entry;
        mission_key.push( "Recover Scout", _( "Recover Scout" ), "", true );
    }

    npc_list = companion_list( p, "_faction_camp_combat_0" );
    if( !npc_list.empty() ) {
        entry = _( "Patrolling the region.\n" );
        for( auto &elem : npc_list ) {
            int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
            if( hrs_left > 0 ) {
                entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + _( " hours left] \n" );
            } else {
                entry = entry + "  " + elem->name + _( " [DONE]\n" );
            }
        }
        entry = entry + _( "\n \nDo you wish to bring your allies back into your party?" );
        mission_key.text["Recover Combat Patrol"] = entry;
        mission_key.push( "Recover Combat Patrol", _( "Recover Combat Patrol" ), "", true );
    }
}

bool talk_function::handle_camp_mission( mission_entry &cur_key, npc &p )
{
    //Used to determine what kind of OM the NPC is sitting in to determine the missions and upgrades
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, p.posz() );
    std::string om_cur = omt_ref.id().c_str();
    std::string bldg;
    std::vector<std::pair<std::string, tripoint>> om_expansions = om_building_region( p, 1, true );

    if( cur_key.id == "Upgrade Camp" ) {
        const recipe *making = &recipe_id( bldg ).obj();
        //Stop upgrade if you don't have materials
        inventory total_inv = g->u.crafting_inventory();
        if( making->requirements().can_make_with_inventory( total_inv, 1 ) ) {
            std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_upgrade_camp" );
            int need_food = time_to_food( time_duration::from_turns( making->time / 100 ) );
            if( camp_food_supply() < need_food && bldg != "faction_base_camp_1" ) {
                popup( _( "You don't have enough food stored to feed your companion." ) );
            } else if( npc_list.empty() ) {
                npc *comp = individual_mission( p, _( "begins to upgrade the camp..." ), "_faction_upgrade_camp",
                                                false, {},
                                                making->skill_used.obj().ident().c_str(), making->difficulty );
                if( comp != nullptr ) {
                    comp->companion_mission_time_ret = calendar::turn + time_duration::from_turns(
                                                           making->time / 100 ) ;
                    g->u.consume_components_for_craft( making, 1, true );
                    g->u.invalidate_crafting_inventory();
                    camp_food_supply( -need_food );
                }
            } else {
                popup( _( "You already have a companion upgrading the camp." ) );
            }
        } else {
            popup( _( "You don't have the materials for the upgrade." ) );
        }
    }

    if( cur_key.id == "Recover Ally from Upgrading" ) {
        upgrade_return( p, omt_pos, "_faction_upgrade_camp" );
    }

    if( cur_key.id == "Recover Ally, " + cur_key.dir + " Expansion" ) {
        for( const auto &e : om_expansions ) {
            //Find the expansion that is in that direction
            if( cur_key.dir  == om_simple_dir( omt_pos, e.second ) ) {
                upgrade_return( p, point( e.second.x, e.second.y ), "_faction_upgrade_exp_" + cur_key.dir );
                break;
            }
        }
    }

    if( cur_key.id == cur_key.dir + " (Finish) Chop Shop" ) {
        for( const auto &e : om_expansions ) {
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                npc *comp = companion_choose_return( p, "_faction_exp_chop_shop_" + cur_key.dir,
                                                     calendar::turn - 5_days );
                if( comp != nullptr ) {
                    popup( _( "%s returns from your garage..." ), comp->name.c_str() );
                    companion_return( *comp );
                }
                break;
            }
        }
    }

    if( cur_key.id == cur_key.dir + " (Finish) Cooking" ) {
        for( const auto &e : om_expansions ) {
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                npc *comp = companion_choose_return( p, "_faction_exp_kitchen_cooking_" + cur_key.dir,
                                                     calendar::before_time_starts );
                if( comp != nullptr ) {
                    popup( _( "%s returns from your kitchen with something..." ), comp->name.c_str() );
                    companion_return( *comp );
                }
                break;
            }
        }
    }

    if( cur_key.id == cur_key.dir + " (Finish) Smithing" ) {
        for( const auto &e : om_expansions ) {
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                npc *comp = companion_choose_return( p, "_faction_exp_blacksmith_crafting_" + cur_key.dir,
                                                     calendar::before_time_starts );
                if( comp != nullptr ) {
                    popup( _( "%s returns from your blacksmith shop with something..." ), comp->name.c_str() );
                    companion_return( *comp );
                }
                break;
            }
        }
    }

    if( cur_key.id == cur_key.dir + " (Finish) Plow Fields" ) {
        for( const auto &e : om_expansions ) {
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                camp_farm_return( p, "_faction_exp_plow_" + cur_key.dir, false, false, true );
                break;
            }
        }
    }

    if( cur_key.id == cur_key.dir + " (Finish) Plant Fields" ) {
        for( const auto &e : om_expansions ) {
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                camp_farm_return( p, "_faction_exp_plant_" + cur_key.dir, false, true, false );
                break;
            }
        }
    }

    if( cur_key.id == cur_key.dir + " (Finish) Harvest Fields" ) {
        for( const auto &e : om_expansions ) {
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                camp_farm_return( p, "_faction_exp_harvest_" + cur_key.dir, true, false, false );
                break;
            }
        }
    }

    if( cur_key.id == cur_key.dir + " (Finish) Crafting" ) {
        for( const auto &e : om_expansions ) {
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {

                npc *comp = companion_choose_return( p, "_faction_exp_farm_crafting_" + cur_key.dir,
                                                     calendar::before_time_starts );
                if( comp != nullptr ) {
                    popup( _( "%s returns from your farm with something..." ), comp->name.c_str() );
                    companion_return( *comp );
                }
                break;
            }
        }
        if( cur_key.dir == "[B]" ) {
            npc *comp = companion_choose_return( p, "_faction_camp_crafting_" + cur_key.dir,
                                                 calendar::before_time_starts );
            if( comp != nullptr ) {
                popup( _( "%s returns to you with something..." ), comp->name.c_str() );
                companion_return( *comp );
            }
        }
    }

    if( cur_key.id == "Distribute Food" ) {
        camp_distribute_food( p );
    }

    if( cur_key.id == "Gather Materials" ) {
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_gathering" );
        if( npc_list.size() < 3 ) {
            individual_mission( p, _( "departs to search for materials..." ), "_faction_camp_gathering", false, {},
                                "survival" );
        } else {
            popup( _( "There are too many companions working on this mission!" ) );
        }
    }

    if( cur_key.id == "Recover Ally from Gathering" ) {
        camp_gathering_return( p, "_faction_camp_gathering" );
    }

    if( cur_key.id == "Collect Firewood" ) {
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_firewood" );
        if( npc_list.size() < 3 ) {
            individual_mission( p, _( "departs to search for firewood..." ), "_faction_camp_firewood", false, {},
                                "survival" );
        } else {
            popup( _( "There are too many companions working on this mission!" ) );
        }
    }
    if( cur_key.id == "Recover Firewood Gatherers" ) {
        camp_gathering_return( p, "_faction_camp_firewood" );
    }

    if( cur_key.id == "Menial Labor" ) {
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_menial" );
        int need_food = time_to_food( 3_hours );
        if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
        } else if( npc_list.empty() ) {
            npc *comp = individual_mission( p, _( "departs to dig ditches and scrub toilets..." ),
                                            "_faction_camp_menial" );
            if( comp != nullptr ) {
                comp->companion_mission_time_ret = calendar::turn + 3_hours;
                camp_food_supply( -need_food );
            }
        } else {
            popup( _( "There are too many companions working on this mission!" ) );
        }
    }

    if( cur_key.id == "Recover Menial Laborer" ) {
        camp_menial_return( p );
    }

    if( cur_key.id == "Reset Sort Points" ) {
        camp_menial_sort_pts( p, false, true );
    }

    if( cur_key.id == "Expand Base" ) {
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_expansion" );
        int need_food = time_to_food( 3_hours );
        if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
        } else if( npc_list.empty() ) {
            npc *comp = individual_mission( p, _( "departs to survey land..." ), "_faction_camp_expansion" );
            if( comp != nullptr ) {
                camp_food_supply( - need_food );
                comp->companion_mission_time_ret = calendar::turn + 3_hours;
            }
        } else {
            popup( _( "You have already selected a surveyor!" ) );
        }
    }

    if( cur_key.id == cur_key.dir + " Expansion Upgrade" ) {
        for( const auto &e : om_expansions ) {
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                oter_id &omt_ref_exp = overmap_buffer.ter( e.second.x, e.second.y, e.second.z );
                std::string om_exp = omt_ref_exp.id().c_str();
                std::string bldg_exp = om_next_upgrade( om_exp );
                const recipe *making = &recipe_id( bldg_exp ).obj();
                //Stop upgrade if you don't have materials
                inventory total_inv = g->u.crafting_inventory();
                if( making->requirements().can_make_with_inventory( total_inv, 1 ) ) {
                    std::vector<std::shared_ptr<npc>> npc_list = companion_list( p,
                                                   "_faction_upgrade_exp_" + cur_key.dir );
                    int need_food = time_to_food( time_duration::from_turns( making->time / 100 ) );
                    if( camp_food_supply() < need_food ) {
                        popup( _( "You don't have enough food stored to feed your companion." ) );
                        break;
                    }
                    if( npc_list.empty() ) {
                        if( individual_mission( p, _( "begins to upgrade the expansion..." ),
                                                "_faction_upgrade_exp_" + cur_key.dir, false, {},
                                                making->skill_used.obj().ident().c_str(), making->difficulty ) != nullptr ) {
                            g->u.consume_components_for_craft( making, 1, true );
                            g->u.invalidate_crafting_inventory();
                            camp_food_supply( -need_food );
                        }
                    } else {
                        popup( _( "You already have a worker upgrading that expansion!" ) );
                    }
                } else {
                    popup( _( "You don't have the materials for the upgrade." ) );
                }
            }
        }
    }

    if( cur_key.id == "Cut Logs" ) {
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_cut_log" );
        std::vector<std::string> log_sources = { "forest", "forest_thick", "forest_water" };
        popup( _( "Forests and swamps are the only valid cutting locations." ) );
        tripoint forest = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 1, 50,
                                          log_sources );
        if( forest != tripoint( -999, -999, -999 ) ) {
            int tree_est = om_harvest_trees( p, tripoint( forest.x, forest.y, 0 ), .50, false, false );
            int tree_young_est = om_harvest_ter( p, point( forest.x, forest.y ), ter_id( "t_tree_young" ), .50,
                                                 false );
            int dist = rl_dist( forest.x, forest.y, omt_pos.x, omt_pos.y );
            //Very roughly what the player does + 6 hours for prep, clean up, breaks
            time_duration chop_time = 6_hours + ( 1_hours * tree_est ) + ( 7_minutes * tree_young_est );
            //Generous to believe the NPC can move ~ 2 logs or ~8 heavy sticks (3 per young tree?) per trip, each way is 1 trip
            // 20 young trees => ~60 sticks which can be carried 8 at a time, so 8 round trips or 16 trips total
            //This all needs to be in an om_carry_weight_over_distance function eventually...
            int trips = tree_est + ( tree_young_est * ( 3 / 8 ) * 2 );
            //Alwasy have to come back so no odd number of trips
            trips = ( trips % 2 == 0 ) ? trips : ( trips + 1 );
            time_duration travel_time = companion_travel_time_calc( forest, tripoint( omt_pos.x, omt_pos.y,
                                        forest.z ), 0_minutes, trips );
            time_duration work_time = travel_time + chop_time;
            int need_food = time_to_food( work_time );
            if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time, chop_time, travel_time,
                           dist, trips, need_food ) ) ) {
                return false;
            } else if( camp_food_supply() < need_food ) {
                popup( _( "You don't have enough food stored to feed your companion." ) );
            } else if( npc_list.empty() ) {
                g->draw_ter();
                //wrefresh( g->w_terrain );
                npc *comp = individual_mission( p, _( "departs to cut logs..." ), "_faction_camp_cut_log", false, {},
                                                "fabrication", 2 );
                if( comp != nullptr ) {
                    om_harvest_trees( *comp, forest, .50, true, true );
                    om_harvest_ter( *comp, point( forest.x, forest.y ), ter_id( "t_tree_young" ), .50, true );
                    om_harvest_itm( *comp, point( forest.x, forest.y ), .75, true );
                    comp->companion_mission_time_ret = calendar::turn + work_time;
                    camp_food_supply( -need_food );

                    //If we cleared a forest...
                    tree_est = om_harvest_trees( p, tripoint( forest.x, forest.y, 0 ), .50, false, false );
                    if( tree_est < 20 ) {
                        oter_id &omt_trees = overmap_buffer.ter( forest.x, forest.y, g->u.posz() );
                        //Do this for swamps "forest_wet" if we have a swamp without trees...
                        if( omt_trees.id() == "forest" || omt_trees.id() == "forest_thick" ) {
                            omt_trees = oter_id( "field" );
                        }
                    }
                }
            } else {
                popup( _( "There are too many companions working on this mission!" ) );
            }
        }
    }

    if( cur_key.id == "Recover Log Cutter" ) {
        npc *comp = companion_choose_return( p, "_faction_camp_cut_log", calendar::before_time_starts );
        if( comp != nullptr ) {
            popup( _( "%s returns from working in the woods..." ), comp->name.c_str() );
            companion_return( *comp );
        }
    }

    if( cur_key.id == "Setup Hide Site" ) {
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_hide_site" );
        std::vector<std::string> hide_locations = { "forest", "forest_thick", "forest_water", "field" };
        popup( _( "Forests, swamps, and fields are valid hide site locations." ) );
        tripoint forest = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 10, 90,
                                          hide_locations,
                                          true, true, tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), true );
        if( forest != tripoint( -999, -999, -999 ) ) {
            int dist = rl_dist( forest.x, forest.y, omt_pos.x, omt_pos.y );
            inventory tgt_inv = g->u.inv;
            std::vector<item *> pos_inv = tgt_inv.items_with( []( const item & itm ) {
                return !itm.can_revive();
            } );
            if( !pos_inv.empty() ) {
                std::vector<item *> losing_equipment = individual_mission_give_equipment( pos_inv );
                int trips = om_carry_weight_to_trips( p, losing_equipment ) * 2;
                trips = ( trips % 2 == 0 ) ? trips : ( trips + 1 );
                time_duration build_time = 6_hours;
                time_duration travel_time = companion_travel_time_calc( forest, tripoint( omt_pos.x, omt_pos.y,
                                            forest.z ), 0_minutes, trips );
                time_duration work_time = travel_time + build_time;
                int need_food = time_to_food( work_time );
                if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time, build_time, travel_time,
                               dist, trips, need_food ) ) ) {
                    return false;
                } else if( camp_food_supply() < need_food ) {
                    popup( _( "You don't have enough food stored to feed your companion." ) );
                } else if( npc_list.empty() ) {

                    npc *comp = individual_mission( p, _( "departs to build a hide site..." ),
                                                    "_faction_camp_hide_site", false,
                                                    {}, "survival", 3 );
                    if( comp != nullptr ) {
                        comp->companion_mission_time_ret = calendar::turn + work_time;
                        om_set_hide_site( *comp, forest, losing_equipment );
                        camp_food_supply( -need_food );
                    }
                } else {
                    popup( _( "There are too many companions working on this mission!" ) );
                }
            } else {
                popup( _( "You need equipment to setup a hide site..." ) );
            }
        }
    }

    if( cur_key.id == "Recover Hide Setup" ) {
        npc *comp = companion_choose_return( p, "_faction_camp_hide_site", calendar::before_time_starts );
        if( comp != nullptr ) {
            popup( _( "%s returns from working on the hide site..." ), comp->name.c_str() );
            companion_return( *comp );
        }
    }

    if( cur_key.id == "Relay Hide Site" ) {
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_hide_trans" );
        std::vector<std::string> hide_locations = { "faction_hide_site_0" };
        popup( _( "You must select an existing hide site." ) );
        tripoint forest = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 10, 90,
                                          hide_locations,
                                          true, true, tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), true );
        if( forest != tripoint( -999, -999, -999 ) ) {
            int dist = rl_dist( forest.x, forest.y, omt_pos.x, omt_pos.y );
            inventory tgt_inv = g->u.inv;
            std::vector<item *> pos_inv = tgt_inv.items_with( []( const item & itm ) {
                return !itm.can_revive();
            } );
            std::vector<item *> losing_equipment;
            if( !pos_inv.empty() ) {
                losing_equipment = individual_mission_give_equipment( pos_inv );
            }

            //Check items in improvised shelters at hide site
            tinymap target_bay;
            target_bay.load( forest.x * 2, forest.y * 2, forest.z, false );
            std::vector<item *> hide_inv;
            for( item &i : target_bay.i_at( 11, 10 ) ) {
                hide_inv.push_back( &i );
            }
            std::vector<item *> gaining_equipment;
            if( !hide_inv.empty() ) {
                gaining_equipment = individual_mission_give_equipment( hide_inv, _( "Bring gear back?" ) );
            }
            if( !losing_equipment.empty() || !gaining_equipment.empty() ) {
                //Only get charged the greater trips since return is free for both
                int trips = om_carry_weight_to_trips( p, losing_equipment ) * 2;
                int trips_return = om_carry_weight_to_trips( p, gaining_equipment ) * 2;
                if( trips < trips_return ) {
                    trips = trips_return;
                }
                trips = ( trips % 2 == 0 ) ? trips : ( trips + 1 );
                time_duration build_time = 6_hours;
                time_duration travel_time = companion_travel_time_calc( forest, tripoint( omt_pos.x, omt_pos.y,
                                            forest.z ), 0_minutes, trips );
                time_duration work_time = travel_time + build_time;
                int need_food = time_to_food( work_time );
                if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( work_time, build_time, travel_time,
                               dist, trips, need_food ) ) ) {
                    return false;
                } else if( camp_food_supply() < need_food ) {
                    popup( _( "You don't have enough food stored to feed your companion." ) );
                } else if( npc_list.empty() ) {

                    npc *comp = individual_mission( p, _( "departs for the hide site..." ), "_faction_camp_hide_site",
                                                    false,
                                                    {}, "survival", 3 );
                    if( comp != nullptr ) {
                        comp->companion_mission_time_ret = calendar::turn + work_time;
                        om_set_hide_site( *comp, forest, losing_equipment, gaining_equipment );
                        camp_food_supply( -need_food );
                    }
                } else {
                    popup( _( "There are too many companions working on this mission!" ) );
                }
            } else {
                popup( _( "You need equipment to transport between the hide site..." ) );
            }
        }
    }

    if( cur_key.id == "Recover Hide Transport" ) {
        npc *comp = companion_choose_return( p, "_faction_camp_hide_trans", calendar::before_time_starts );
        if( comp != nullptr ) {
            popup( _( "%s returns from shuttling gear between the hide site..." ), comp->name.c_str() );
            companion_return( *comp );
        }
    }

    if( cur_key.id == "Construct Map Fortifications" || cur_key.id == "Construct Spiked Trench" ) {
        std::vector<std::string> allowed_locations = { "forest", "forest_thick", "forest_water", "field",
                                                       "faction_wall_level_N_0", "faction_wall_level_E_0", "faction_wall_level_S_0", "faction_wall_level_W_0",
                                                       "faction_wall_level_N_1", "faction_wall_level_E_1", "faction_wall_level_S_1", "faction_wall_level_W_1"
                                                     };
        if( cur_key.id == "Construct Spiked Trench" ) {
            allowed_locations = { "faction_wall_level_N_0", "faction_wall_level_E_0", "faction_wall_level_S_0", "faction_wall_level_W_0",
                                  "faction_wall_level_N_1", "faction_wall_level_E_1", "faction_wall_level_S_1", "faction_wall_level_W_1"
                                };
        }

        popup( _( "Select a start and end point.  Line must be straight. Fields, forests, and swamps are valid fortification locations."
                  "  In addition to existing fortification constructions." ) );
        tripoint start = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 2, 90,
                                         allowed_locations );
        popup( _( "Select an end point." ) );
        tripoint stop = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 2, 90,
                                        allowed_locations, true, false, start );
        if( start != tripoint( -999, -999, -999 ) && stop != tripoint( -999, -999, -999 ) ) {
            std::string bldg_exp = "faction_wall_level_N_0";
            if( cur_key.id == "Construct Spiked Trench" ) {
                bldg_exp = "faction_wall_level_N_1";
            }
            const recipe *making = &recipe_id( bldg_exp ).obj();
            inventory total_inv = g->u.crafting_inventory();
            bool change_x = ( start.x != stop.x );
            bool change_y = ( start.y != stop.y );
            if( change_x && change_y ) {
                popup( "Construction line must be straight!" );
                return false;
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
                oter_id &omt_ref = overmap_buffer.ter( fort_om.x, fort_om.y, fort_om.z );
                for( auto pos_om : allowed_locations ) {
                    if( omt_ref.id().c_str() == pos_om ) {
                        valid = true;
                    }
                }

                if( !valid ) {
                    popup( _( "Invalid terrain in construction path." ) );
                    return false;
                }
                trips += 2;
                build_time += time_duration::from_turns( making->time / 100 );
                travel_time += companion_travel_time_calc( fort_om, tripoint( omt_pos.x, omt_pos.y, fort_om.z ),
                               0_minutes, trips );
                dist += rl_dist( fort_om.x, fort_om.y, omt_pos.x, omt_pos.y );
            }
            time_duration total_time = travel_time + build_time;
            int need_food = time_to_food( total_time );
            if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( total_time, build_time,
                           travel_time, dist, trips, need_food ) ) ) {
                return false;
            } else if( !making->requirements().can_make_with_inventory( total_inv,
                       ( fortify_om.size() * 2 ) - 2 ) ) {
                popup( _( "You don't have the material to build the fortification." ) );
            } else if( camp_food_supply() < need_food ) {
                popup( _( "You don't have enough food stored to feed your companion." ) );
            } else {
                npc *comp = individual_mission( p, _( "begins constructing fortifications..." ),
                                                "_faction_camp_om_fortifications", false, {},
                                                making->skill_used.obj().ident().c_str(), making->difficulty );
                if( comp != nullptr ) {
                    g->u.consume_components_for_craft( making, ( fortify_om.size() * 2 ) - 2, true );
                    g->u.invalidate_crafting_inventory();
                    camp_food_supply( -need_food );
                    companion_skill_trainer( *comp, "construction", build_time, 2 );
                    comp->companion_mission_time_ret = calendar::turn + total_time;
                    comp->companion_mission_role_id = bldg_exp;
                    for( auto pt : fortify_om ) {
                        comp->companion_mission_points.push_back( pt );
                    }
                }
            }
        }
    }

    if( cur_key.id == "Finish Map Fortifications" ) {
        npc *comp = companion_choose_return( p, "_faction_camp_om_fortifications",
                                             calendar::before_time_starts );
        if( comp != nullptr ) {
            popup( _( "%s returns from constructing fortifications..." ), comp->name.c_str() );
            editmap edit;
            bool build_dir_NS = ( comp->companion_mission_points[0].y != comp->companion_mission_points[1].y );
            //Ensure all tiles are generated before putting fences/trenches down...
            for( auto pt : comp->companion_mission_points ) {
                if( MAPBUFFER.lookup_submap( om_to_sm_copy( pt ) ) == NULL ) {
                    oter_id &omt_test = overmap_buffer.ter( pt.x, pt.y, pt.z );
                    std::string om_i = omt_test.id().c_str();
                    //The thick forests will gen harsh boundries since it won't recognize these tiles when they become fortifications
                    if( om_i == "forest_thick" ) {
                        om_i = "forest";
                    }
                    edit.mapgen_set( om_i, pt, false );
                }
            }
            //Add fences
            auto build_point = comp->companion_mission_points;
            for( size_t pt = 0; pt < build_point.size(); pt++ ) {
                //First point is always at top or west since they are built in a line and sorted
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
                if( pt == 0 ) {
                    if( build_dir_NS ) {
                        edit.mapgen_set( build_s, build_point[pt] );
                    } else {
                        edit.mapgen_set( build_e, build_point[pt] );
                    }
                } else if( pt == build_point.size() - 1 ) {
                    if( build_dir_NS ) {
                        edit.mapgen_set( build_n, build_point[pt] );
                    } else {
                        edit.mapgen_set( build_w, build_point[pt] );
                    }
                } else if( build_dir_NS ) {
                    edit.mapgen_set( build_n, build_point[pt] );
                    edit.mapgen_set( build_s, build_point[pt] );
                } else {
                    edit.mapgen_set( build_e, build_point[pt] );
                    edit.mapgen_set( build_w, build_point[pt] );
                }
            }
            comp->companion_mission_role_id.clear();
            companion_return( *comp );
        }
    }

    if( cur_key.id == "Recruit Companions" ) {
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_recruit_0" );
        int need_food = time_to_food( 4_days );
        if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
        } else if( npc_list.empty() ) {
            npc *comp = individual_mission( p, _( "departs to search for recruits..." ),
                                            "_faction_camp_recruit_0", false, {}, "speech", 2 );
            if( comp != nullptr ) {
                comp->companion_mission_time_ret = calendar::turn + 4_days;
                camp_food_supply( -need_food );
            }
        } else {
            popup( _( "There are too many companions working on this mission!" ) );
        }
    }

    if( cur_key.id == "Recover Recruiter" ) {
        camp_recruit_return( p, "_faction_camp_recruit_0", atoi( camp_recruit_evaluation( p, om_cur,
                             om_expansions, true ).c_str() ) );
    }

    if( cur_key.id == "Scout Mission" || cur_key.id == "Combat Patrol" ) {
        std::string mission_t = "_faction_camp_scout_0";
        if( cur_key.id == "Combat Patrol" ) {
            mission_t = "_faction_camp_combat_0";
        }
        popup( _( "Select checkpoints until you reach maximum range or select the last point again to end." ) );
        tripoint start = tripoint( omt_pos.x, omt_pos.y, g->u.posz() );
        std::vector<tripoint> scout_points = om_companion_path( start, 90, true );
        if( scout_points.empty() ) {
            return false;
        }
        int dist = scout_points.size();
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, mission_t );
        int trips = 2;
        time_duration travel_time = companion_travel_time_calc( scout_points, 0_minutes, trips );
        time_duration build_time = 0_hours;
        time_duration total_time = travel_time + build_time;
        int need_food = time_to_food( total_time );
        if( !query_yn( _( "Trip Estimate:\n%s" ), camp_trip_description( total_time, build_time,
                       travel_time, dist, trips, need_food ) ) ) {
            return false;
        } else if( camp_food_supply() < need_food ) {
            popup( _( "You don't have enough food stored to feed your companion." ) );
        } else if( npc_list.size() < 3 ) {
            npc *comp = individual_mission( p, _( "departs on patrol..." ), mission_t, false, {}, "survival",
                                            3 );
            if( comp != nullptr ) {
                comp->companion_mission_points = scout_points;
                comp->companion_mission_time_ret = calendar::turn + travel_time;
                camp_food_supply( -need_food );
            }
        } else {
            popup( _( "There are too many companions working on this mission!" ) );
        }
    }

    if( cur_key.id == "Recover Scout" || cur_key.id == "Recover Combat Patrol" ) {
        std::string miss = "_faction_camp_scout_0";
        if( cur_key.id == "Recover Combat Patrol" ) {
            miss = "_faction_camp_combat_0";
        }
        npc *comp = companion_choose_return( p, miss, calendar::before_time_starts );
        if( comp != nullptr ) {
            std::vector<std::shared_ptr<npc>> patrol;
            std::shared_ptr<npc> guy = overmap_buffer.find_npc( comp->getID() );
            patrol.push_back( guy );
            for( auto pt : comp->companion_mission_points ) {
                oter_id &omt_ref = overmap_buffer.ter( pt.x, pt.y, pt.z );
                int swim = comp->get_skill_level( skill_swimming );
                if( is_river( omt_ref ) && swim < 2 ) {
                    if( swim == 0 ) {
                        popup( _( "Your companion hit a river and didn't know how to swim..." ) );
                    } else {
                        popup( _( "Your companion hit a river and didn't know how to swim well enough to cross..." ) );
                    }
                    break;
                }
                bool outcome = false;
                comp->death_drops = false;
                if( cur_key.id == "Recover Scout" ) {
                    outcome = companion_om_combat_check( patrol, pt, false );
                } else if( cur_key.id == "Recover Combat Patrol" ) {
                    outcome = companion_om_combat_check( patrol, pt, true );
                }
                comp->death_drops = true;
                if( !outcome ) {
                    if( comp->is_dead() ) {
                        popup( _( "%s didn't return from patrol..." ), comp->name.c_str() );
                        comp->place_corpse( pt );
                        overmap_buffer.add_note( pt.x, pt.y, pt.z, "DEAD NPC" );
                        overmap_buffer.remove_npc( comp->getID() );
                        return false;
                    } else {
                        popup( _( "%s returns from patrol..." ), comp->name.c_str() );
                        companion_return( *comp );
                        return false;
                    }

                }
                overmap_buffer.reveal( pt, 2 );
            }
            popup( _( "%s returns from patrol..." ), comp->name.c_str() );
            companion_return( *comp );
        }
    }

    if( cur_key.id == cur_key.dir + " Chop Shop" ) {
        for( const auto &e : om_expansions ) {
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                std::vector<std::shared_ptr<npc>> npc_list = companion_list( p,
                                               "_faction_exp_chop_shop_" + cur_key.dir );
                int need_food = time_to_food( 5_days );
                if( camp_food_supply() < need_food ) {
                    popup( _( "You don't have enough food stored to feed your companion." ) );
                } else if( npc_list.empty() ) {
                    camp_garage_chop_start( p, "_faction_exp_chop_shop_" + cur_key.dir );
                    camp_food_supply( - need_food );
                } else {
                    popup( _( "You already have someone working in that garage." ) );
                }
            }
        }
    }

    //Will be generalized for all crafing, when needed
    std::map<std::string, std::string> crafting_recipes = camp_recipe_deck( "ALL" );
    std::map<std::string, std::string> cooking_recipes = camp_recipe_deck( "COOK" );
    std::map<std::string, std::string> base_recipes = camp_recipe_deck( "BASE" );
    std::map<std::string, std::string> farming_recipes = camp_recipe_deck( "FARM" );
    std::map<std::string, std::string> blacksmith_recipes = camp_recipe_deck( "SMITH" );
    camp_craft_construction( p, cur_key, cooking_recipes, "_faction_exp_kitchen_cooking_",
                             tripoint( omt_pos.x, omt_pos.y, p.posz() ),
                             om_expansions );
    camp_craft_construction( p, cur_key, farming_recipes, "_faction_exp_farm_crafting_",
                             tripoint( omt_pos.x, omt_pos.y, p.posz() ),
                             om_expansions );
    camp_craft_construction( p, cur_key, blacksmith_recipes, "_faction_exp_blacksmith_crafting_",
                             tripoint( omt_pos.x, omt_pos.y, p.posz() ),
                             om_expansions );
    //All crafting for base hub
    std::vector<std::pair<std::string, tripoint>> om_expansions_plus;
    std::pair<std::string, tripoint> ent = std::make_pair( om_cur, tripoint( omt_pos.x, omt_pos.y,
                                           p.posz() ) );
    om_expansions_plus.push_back( ent );
    camp_craft_construction( p, cur_key, base_recipes, "_faction_camp_crafting_", tripoint( omt_pos.x,
                             omt_pos.y, p.posz() ),
                             om_expansions_plus );

    if( cur_key.id == cur_key.dir + " Plow Fields" ) {
        for( const auto &e : om_expansions ) {
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                std::vector<std::shared_ptr<npc>> npc_list = companion_list( p,
                                               "_faction_exp_plow_" + cur_key.dir );
                if( npc_list.empty() ) {
                    individual_mission( p, _( "begins plowing the field..." ), "_faction_exp_plow_" + cur_key.dir );
                } else {
                    popup( _( "You already have someone plowing that field." ) );
                }
            }
        }
    }

    if( cur_key.id == cur_key.dir + " Plant Fields" ) {
        for( const auto &e : om_expansions ) {
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                std::vector<std::shared_ptr<npc>> npc_list = companion_list( p,
                                               "_faction_exp_plant_" + cur_key.dir );
                if( npc_list.empty() ) {
                    inventory total_inv = g->u.crafting_inventory();
                    std::vector<item *> seed_inv = total_inv.items_with( []( const item & itm ) {
                        return itm.is_seed() && itm.typeId() != "marloss_seed" && itm.typeId() != "fungal_seeds";
                    } );
                    if( seed_inv.empty() ) {
                        popup( _( "You have no additional seeds to give your companions..." ) );
                        individual_mission( p, _( "begins planting the field..." ), "_faction_exp_plant_" + cur_key.dir );
                    } else {
                        std::vector<item *> lost_equipment = individual_mission_give_equipment( seed_inv,
                                                             _( "Which seeds do you wish to have planted?" ) );
                        individual_mission( p, _( "begins planting the field..." ), "_faction_exp_plant_" + cur_key.dir,
                                            false, lost_equipment );
                    }
                } else {
                    popup( _( "You already have someone planting that field." ) );
                }
            }
        }
    }

    if( cur_key.id == cur_key.dir + " Harvest Fields" ) {
        for( const auto &e : om_expansions ) {
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                std::vector<std::shared_ptr<npc>> npc_list = companion_list( p,
                                               "_faction_exp_harvest_" + cur_key.dir );
                if( npc_list.empty() ) {
                    individual_mission( p, _( "begins to harvest the field..." ), "_faction_exp_harvest_" + cur_key.dir,
                                        false, {}, "survival" );
                } else {
                    popup( _( "You already have someone harvesting that field." ) );
                }
            }
        }
    }

    if( cur_key.id == "Recover Surveyor" ) {
        camp_expansion_select( p );
    }

    g->draw_ter();
    wrefresh( g->w_terrain );

    return true;
}

void talk_function::draw_camp_tabs( const catacurses::window &win, const camp_tab_mode cur_tab,
                                    std::vector<std::vector<mission_entry>> &entries )
{
    werase( win );
    const int width = getmaxx( win );
    mvwhline( win, 2, 0, LINE_OXOX, width );

    std::vector<std::string> tabs;
    tabs.push_back( string_format( _( "MAIN" ) ) );
    tabs.push_back( string_format( _( "  [N] " ) ) );
    tabs.push_back( string_format( _( " [NE] " ) ) );
    tabs.push_back( string_format( _( "  [E] " ) ) );
    tabs.push_back( string_format( _( " [SE] " ) ) );
    tabs.push_back( string_format( _( "  [S] " ) ) );
    tabs.push_back( string_format( _( " [SW] " ) ) );
    tabs.push_back( string_format( _( "  [W] " ) ) );
    tabs.push_back( string_format( _( " [NW] " ) ) );
    const int tab_step = 3;
    int tab_space = 1;
    int tab_x = 0;
    for( auto t : tabs ) {
        bool tab_empty = entries[tab_x + 1].empty();
        draw_subtab( win, tab_space, t, tab_x == cur_tab, false, tab_empty );
        tab_space += tab_step + utf8_width( t );
        tab_x++;
    }
    wrefresh( win );
}

std::string talk_function::name_mission_tabs( npc &p, const std::string &id,
        const std::string &cur_title,
        camp_tab_mode cur_tab )
{
    if( id != "FACTION_CAMP" ) {
        return cur_title;
    }
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    point change;
    switch( cur_tab ) {
        case TAB_MAIN:
            return _( "Base Missions" );
        case TAB_N:
            change.y--;
            break;
        case TAB_NE:
            change.y--;
            change.x++;
            break;
        case TAB_E:
            change.x++;
            break;
        case TAB_SE:
            change.y++;
            change.x++;
            break;
        case TAB_S:
            change.y++;
            break;
        case TAB_SW:
            change.y++;
            change.x--;
            break;
        case TAB_W:
            change.x--;
            break;
        case TAB_NW:
            change.y--;
            change.x--;
            break;
    }
    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x + change.x, omt_pos.y + change.y, p.posz() );
    std::string om_cur = omt_ref.id().c_str();
    if( om_cur.find( "faction_base_farm" ) != std::string::npos ) {
        return _( "Farm Expansion" );
    }
    if( om_cur.find( "faction_base_garage_" ) != std::string::npos ) {
        return _( "Garage Expansion" );
    }
    if( om_cur.find( "faction_base_kitchen_" ) != std::string::npos ) {
        return _( "Kitchen Expansion" );
    }
    if( om_cur.find( "faction_base_blacksmith_" ) != std::string::npos ) {
        return _( "Blacksmith Expansion" );
    }
    if( om_cur == "field" || om_cur == "forest" || om_cur == "swamp" ) {
        return _( "Empty Expansion" );
    }
    return _( "Base Missions" );
}

std::map<std::string, std::string> talk_function::camp_recipe_deck( const std::string &om_cur )
{
    if( om_cur == "ALL" || om_cur == "COOK" || om_cur == "BASE" || om_cur == "FARM" ||
        om_cur == "SMITH" ) {
        return recipe_group::get_recipes( om_cur );
    }
    std::map<std::string, std::string> cooking_recipes;
    for( auto building_levels : om_all_upgrade_levels( om_cur ) ) {
        std::map<std::string, std::string> test_s = recipe_group::get_recipes( building_levels );
        cooking_recipes.insert( test_s.begin(), test_s.end() );
    }
    return cooking_recipes;
}

int talk_function::camp_recipe_batch_max( const recipe making, const inventory &total_inv )
{
    int max_batch = 0;
    int max_checks = 9;
    int iter = 0;
    size_t batch_size = 1000;
    while( batch_size > 0 ) {
        while( iter < max_checks ) {
            if( making.requirements().can_make_with_inventory( total_inv, max_batch + batch_size ) &&
                ( size_t )camp_food_supply() > ( max_batch + batch_size ) * time_to_food( time_duration::from_turns(
                            making.time / 100 ) ) ) {
                max_batch += batch_size;
            }
            iter++;
        }
        iter = 0;
        batch_size /= 10;
    }
    return max_batch;
}

int talk_function::om_harvest_furn( npc &comp, const point &omt_tgt, const furn_id &f, float chance,
                                    bool force_bash )
{
    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    const furn_t &furn_tgt = f.obj();
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    int harvested = 0;
    int total = 0;
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            if( target_bay.furn( x, y ) == f && x_in_y( chance, 1.0 ) ) {
                if( force_bash || comp.str_cur > furn_tgt.bash.str_min + rng( -2, 2 ) ) {
                    for( auto itm : item_group::items_from( furn_tgt.bash.drop_group, calendar::turn ) ) {
                        comp.companion_mission_inv.push_back( itm );
                    }
                    harvested++;
                    target_bay.furn_set( x, y, furn_tgt.bash.furn_set );
                }
                total++;
            }
        }
    }
    target_bay.save();
    if( !force_bash ) {
        return total;
    }
    return harvested;
}

int talk_function::om_harvest_ter( npc &comp, const point &omt_tgt, const ter_id &t, float chance,
                                   bool force_bash )
{
    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    const ter_t &ter_tgt = t.obj();
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    int harvested = 0;
    int total = 0;
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            if( target_bay.ter( x, y ) == t && x_in_y( chance, 1.0 ) ) {
                if( force_bash ) {
                    for( auto itm : item_group::items_from( ter_tgt.bash.drop_group, calendar::turn ) ) {
                        comp.companion_mission_inv.push_back( itm );
                    }
                    harvested++;
                    target_bay.ter_set( x, y, ter_tgt.bash.ter_set );
                }
                total++;
            }
        }
    }
    target_bay.save();
    if( !force_bash ) {
        return total;
    }
    return harvested;
}

int talk_function::om_harvest_trees( npc &comp, const tripoint &omt_tgt, float chance,
                                     bool force_cut,
                                     bool force_cut_trunk )
{
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    int harvested = 0;
    int total = 0;
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            if( target_bay.ter( x, y ).obj().has_flag( "TREE" ) && rng( 0, 100 ) < chance * 100 ) {
                if( force_cut ) {
                    tripoint direction;
                    direction.x = ( rng( 0, 1 ) == 1 ) ? -1 : 1;
                    direction.y = rng( -1, 1 );
                    tripoint to = tripoint( x, y, 0 ) + tripoint( 3 * direction.x + rng( -1, 1 ),
                                  3 * direction.y + rng( -1, 1 ), 0 );
                    std::vector<tripoint> tree = line_to( tripoint( x, y, 0 ), to, rng( 1, 8 ) );
                    for( auto &elem : tree ) {
                        target_bay.destroy( elem );
                        target_bay.ter_set( elem, t_trunk );
                    }
                    target_bay.ter_set( x, y, t_dirt );
                    harvested++;
                }
                total++;
            }
        }
    }
    //even if we cut down the tree we roll again for all of the trunks to see which we cut
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            if( target_bay.ter( x, y ) == ter_id( "t_trunk" ) && x_in_y( chance, 1.0 ) ) {
                if( force_cut_trunk ) {
                    target_bay.ter_set( x, y, t_dirt );
                    target_bay.spawn_item( x, y, "log", rng( 2, 3 ), 0, calendar::turn );
                    harvested++;
                }
                total++;
            }
        }
    }

    target_bay.save();
    if( !force_cut && !force_cut_trunk ) {
        return total;
    }
    return harvested;
}

int talk_function::om_harvest_itm( npc &comp, const point &omt_tgt, float chance, bool take )
{
    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    int harvested = 0;
    int total = 0;
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            for( item i : target_bay.i_at( x, y ) ) {
                total++;
                if( x_in_y( chance, 1.0 ) ) {
                    comp.companion_mission_inv.push_back( i );
                    harvested++;
                }
            }
            target_bay.i_clear( x, y );
        }
    }
    target_bay.save();
    if( !take ) {
        return total;
    }
    return harvested;
}

tripoint talk_function::om_target_tile( const tripoint &omt_pos, int min_range, int range,
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
    int dist = rl_dist( where.x, where.y, omt_pos.x, omt_pos.y );
    if( dist > range || dist < min_range ) {
        popup( _( "You must select a target between %d and %d range from the base.  Range: %d" ), min_range,
               range, dist );
        errors = true;
    }

    tripoint omt_tgt = tripoint( where.x, where.y, g->u.posz() );

    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, g->u.posz() );

    if( must_see && overmap_buffer.seen( omt_tgt.x, omt_tgt.y, 0 ) == false ) {
        errors = true;
        popup( _( "You must be able to see the target that you select." ) );
    }

    if( !errors ) {
        for( auto pos_om : bounce_locations ) {
            if( bounce && omt_ref.id().c_str() == pos_om && range > 5 ) {
                if( query_yn( _( "Do you want to bounce off this location to extend range?" ) ) ) {
                    om_line_mark( omt_pos, omt_tgt );
                    tripoint dest = om_target_tile( omt_tgt, 2, range * .75, possible_om_types, true, false, omt_tgt,
                                                    true );
                    om_line_mark( omt_pos, omt_tgt, false );
                    return dest;
                }
            }
        }

        if( possible_om_types.empty() ) {
            return omt_tgt;
        }

        for( auto pos_om : possible_om_types ) {
            if( omt_ref.id().c_str() == pos_om ) {
                return omt_tgt;
            }
        }
    }

    return om_target_tile( omt_pos, min_range, range, possible_om_types );
}

void talk_function::om_range_mark( const tripoint &origin, int range, bool add_notes,
                                   const std::string &message )
{
    std::vector<tripoint> note_pts;
    //North Limit
    for( int x = origin.x - range - 1; x < origin.x + range + 2; x++ ) {
        note_pts.push_back( tripoint( x, origin.y - range - 1, origin.z ) );
    }
    //South
    for( int x = origin.x - range - 1; x < origin.x + range + 2; x++ ) {
        note_pts.push_back( tripoint( x, origin.y + range + 1, origin.z ) );
    }
    //West
    for( int y = origin.y - range - 1; y < origin.y + range + 2; y++ ) {
        note_pts.push_back( tripoint( origin.x - range - 1, y, origin.z ) );
    }
    //East
    for( int y = origin.y - range - 1; y < origin.y + range + 2; y++ ) {
        note_pts.push_back( tripoint( origin.x + range + 1, y, origin.z ) );
    }

    for( auto pt : note_pts ) {
        if( add_notes ) {
            if( !overmap_buffer.has_note( pt ) ) {
                overmap_buffer.add_note( pt.x, pt.y, pt.z, message );
            }
        } else {
            if( overmap_buffer.has_note( pt ) && overmap_buffer.note( pt.x, pt.y, pt.z ) == message ) {
                overmap_buffer.delete_note( pt.x, pt.y, pt.z );
            }
        }
    }

}

void talk_function::om_line_mark( const tripoint &origin, const tripoint &dest, bool add_notes,
                                  const std::string &message )
{
    std::vector<tripoint> note_pts = line_to( origin, dest );

    for( auto pt : note_pts ) {
        if( add_notes ) {
            if( !overmap_buffer.has_note( pt ) ) {
                overmap_buffer.add_note( pt.x, pt.y, pt.z, message );
            }
        } else {
            if( overmap_buffer.has_note( pt ) && overmap_buffer.note( pt.x, pt.y, pt.z ) == message ) {
                overmap_buffer.delete_note( pt.x, pt.y, pt.z );
            }
        }
    }
}

time_duration talk_function::companion_travel_time_calc( const tripoint &omt_pos,
        const tripoint &omt_tgt,
        time_duration work, int trips )
{
    std::vector<tripoint> journey = line_to( omt_pos, omt_tgt );
    return companion_travel_time_calc( journey, work, trips );
}

time_duration talk_function::companion_travel_time_calc( const std::vector<tripoint> &journey,
        time_duration work, int trips )
{
    //path = pf::find_path( point( start.x, start.y ), point( finish.x, finish.y ), 2*OX, 2*OY, estimate );
    int one_way = 0;
    for( auto &om : journey ) {
        oter_id &omt_ref = overmap_buffer.ter( om.x, om.y, g->u.posz() );
        std::string om_id = omt_ref.id().c_str();
        //Player walks 1 om is roughly 2.5 min
        if( om_id == "field" ) {
            one_way += 3;
        } else if( om_id == "forest" ) {
            one_way += 4;
        } else if( om_id == "forest_thick" ) {
            one_way += 5;
        } else if( om_id == "forest_water" ) {
            one_way += 6;
        } else if( is_river( omt_ref ) ) {
            one_way += 20;
        } else {
            one_way += 4;
        }
    }
    return time_duration::from_minutes( ( one_way * trips ) + to_minutes<int>( work ) );
}

std::string talk_function::camp_trip_description( time_duration total_time,
        time_duration working_time, time_duration travel_time,
        int distance, int trips, int need_food )
{
    std::string entry = " \n";
    //A square is roughly 3 m
    int dist_m = distance * 24 * 3;
    if( dist_m > 1000 ) {
        entry += string_format( _( ">Distance:%15.2f (km)\n" ), ( dist_m / 1000.0 ) );
        entry += string_format( _( ">One Way: %15d (trips)\n" ), trips );
        entry += string_format( _( ">Covered: %15.2f (km)\n" ), ( dist_m / 1000.0 * trips ) );
    } else {
        entry += string_format( _( ">Distance:%15d (m)\n" ), dist_m );
        entry += string_format( _( ">One Way: %15d (trips)\n" ), trips );
        entry += string_format( _( ">Covered: %15d (m)\n" ), dist_m * trips );
    }
    entry += string_format( _( ">Travel:  %15d (hours)\n" ), to_hours<int>( travel_time ) );
    entry += string_format( _( ">Working: %15d (hours)\n" ), to_hours<int>( working_time ) );
    entry += "----                   ----\n";
    if( total_time > 3_days ) {
        entry += string_format( _( "Total:    %15d (days)\n" ), to_days<int>( total_time ) );
    } else if( total_time > 3_hours ) {
        entry += string_format( _( "Total:    %15d (hours)\n" ), to_hours<int>( total_time ) );
    } else {
        entry += string_format( _( "Total:    %15d (minutes)\n" ), to_minutes<int>( total_time ) );
    }
    entry += string_format( _( "Food:     %15d (kcal)\n \n" ), 10 * need_food );
    return entry;
}

int talk_function::om_carry_weight_to_trips( npc &comp, const std::vector<item *> &itms )
{
    int trips = 0;
    int total_m = 0;
    int total_v = 0;
    for( auto i : itms ) {
        total_m += to_gram( i->weight( true ) );
        total_v += to_milliliter( i->volume( true ) );
    }
    //Assume a significant portion of weight is dedicated to personal gear
    //This isn't a dedicated logistics transport
    float max_m = to_gram( comp.weight_capacity() ) * ( 2 / 3.0 );
    trips = ceil( total_m / max_m );
    trips = ( trips == 0 ) ?  1 : trips;
    //Assume an additional pack will be carried in addition to normal gear
    item sack = item( itype_id( "makeshift_sling" ) );
    float max_v = to_milliliter( comp.volume_capacity() ) * ( 2 / 3.0 ) + to_milliliter(
                      sack.get_storage() );
    int trips_v = ceil( total_v / max_v );
    trips_v = ( trips_v == 0 ) ?  1 : trips_v;
    return ( trips > trips_v ) ?  trips : trips_v;
}

bool talk_function::om_set_hide_site( npc &comp, const tripoint &omt_tgt,
                                      const std::vector<item *> &itms,
                                      const std::vector<item *> &itms_rem )
{
    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, comp.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    target_bay.ter_set( 11, 10, t_improvised_shelter );
    for( auto i : itms_rem ) {
        comp.companion_mission_inv.add_item( *i );
        target_bay.i_rem( 11, 10, i );
    }
    for( auto i : itms ) {
        target_bay.add_item_or_charges( 11, 10, *i );
        g->u.use_amount( i->typeId(), 1 );
    }
    target_bay.save();

    omt_ref = oter_id( "faction_hide_site_0" );

    overmap_buffer.reveal( point( omt_tgt.x, omt_tgt.y ), 3, 0 );
    return true;
}

bool talk_function::camp_menial_sort_pts( npc &p, bool reset_pts, bool choose_pts )
{
    //update COMPANION_SORT_POINTS if you add more, forces old saved points to be updated
    //referenced with
    //p.companion_mission_points[x]
    //0  p_food
    //1  p_food_companions
    //2  p_seeds
    //3  p_weapons
    //4  p_clothing
    //5  p_bionics
    //6  p_tools
    //7  p_wood
    //8  p_trash
    //9  p_book
    //10 p_medication
    //11 p_ammo

    std::vector<std::string> sort_names;
    std::vector<tripoint> sort_pts;
    tripoint p_food = tripoint( p.posx() - 3, p.posy() - 1, p.posz() );
    sort_pts.push_back( p_food );
    sort_names.push_back( _( "food for you" ) );
    tripoint p_food_stock = tripoint( p.posx() + 1, p.posy(), p.posz() );
    sort_pts.push_back( p_food_stock );
    sort_names.push_back( _( "food for companions" ) );
    tripoint p_seed = tripoint( p.posx() - 1, p.posy() - 1, p.posz() );
    sort_pts.push_back( p_seed );
    sort_names.push_back( _( "seeds" ) );
    tripoint p_weapon = tripoint( p.posx() - 1, p.posy() + 1, p.posz() );
    sort_pts.push_back( p_weapon );
    sort_names.push_back( _( "weapons" ) );
    tripoint p_clothing = tripoint( p.posx() - 3, p.posy() - 2, p.posz() );
    sort_pts.push_back( p_clothing );
    sort_names.push_back( _( "clothing" ) );
    tripoint p_bionic = tripoint( p.posx() - 3, p.posy() + 1, p.posz() );
    sort_pts.push_back( p_bionic );
    sort_names.push_back( _( "bionics" ) );
    tripoint p_tool = tripoint( p.posx() - 3, p.posy() + 2, p.posz() );
    sort_pts.push_back( p_tool );
    sort_names.push_back( _( "all kinds of tools" ) );
    tripoint p_wood = tripoint( p.posx() - 5, p.posy() + 2, p.posz() );
    sort_pts.push_back( p_wood );
    sort_names.push_back( _( "wood of various sorts" ) );
    tripoint p_trash = tripoint( p.posx() - 6, p.posy() - 3, p.posz() );
    sort_pts.push_back( p_trash );
    sort_names.push_back( _( "trash and rotting food" ) );
    tripoint p_book = tripoint( p.posx() - 3, p.posy() + 1, p.posz() );
    sort_pts.push_back( p_book );
    sort_names.push_back( _( "books" ) );
    tripoint p_medication = tripoint( p.posx() - 3, p.posy() + 1, p.posz() );
    sort_pts.push_back( p_medication );
    sort_names.push_back( _( "medication" ) );
    tripoint p_ammo = tripoint( p.posx() - 3, p.posy() + 1, p.posz() );
    sort_pts.push_back( p_ammo );
    sort_names.push_back( _( "ammo" ) );

    if( reset_pts ) {
        p.companion_mission_points.clear();
        p.companion_mission_points.insert( p.companion_mission_points.end(), sort_pts.begin(),
                                           sort_pts.end() );
        if( !choose_pts ) {
            return true;
        }
    }
    if( choose_pts ) {
        for( size_t x = 0; x < sort_pts.size(); x++ ) {
            if( query_yn( string_format( _( "Reset point: %s?" ), sort_names[x] ) ) ) {
                const tripoint where( g->look_around() );
                if( rl_dist( g->u.pos(), where ) <= 20 ) {
                    sort_pts[x] = where;
                }
            }
        }
    }

    std::string display_pts = _( "                    Items       New Point       Old Point\n \n" );
    for( size_t x = 0; x < sort_pts.size(); x++ ) {
        std::string trip_string =  string_format( "( %d, %d, %d)", sort_pts[x].x, sort_pts[x].y,
                                   sort_pts[x].z );
        std::string old_string = "";
        if( p.companion_mission_points.size() == sort_pts.size() ) {
            old_string =  string_format( "( %d, %d, %d)", p.companion_mission_points[x].x,
                                         p.companion_mission_points[x].y,
                                         p.companion_mission_points[x].z );
        }
        display_pts += string_format( "%25s %15s %15s    \n", sort_names[x], trip_string, old_string );
    }
    display_pts += _( "\n \n             Save Points?" );

    if( query_yn( display_pts ) ) {
        p.companion_mission_points.clear();
        p.companion_mission_points.insert( p.companion_mission_points.end(), sort_pts.begin(),
                                           sort_pts.end() );
        return true;
    }
    if( query_yn( _( "Revert to default points?" ) ) ) {
        return camp_menial_sort_pts( p, true, false );
    }
    return false;
}

void talk_function::camp_craft_construction( npc &p, const mission_entry &cur_key,
        const std::map<std::string, std::string> &recipes, const std::string &miss_id,
        const tripoint &omt_pos, const std::vector<std::pair<std::string, tripoint>> &om_expansions )
{
    for( std::map<std::string, std::string>::const_iterator it = recipes.begin(); it != recipes.end();
         ++it ) {
        if( cur_key.id == cur_key.dir + it->first ) {
            const recipe *making = &recipe_id( it->second ).obj();
            inventory total_inv = g->u.crafting_inventory();

            if( ! making->requirements().can_make_with_inventory( total_inv, 1 ) ) {
                popup( _( "You don't have the materials to craft that" ) );
                continue;
            }

            int batch_size = 1;
            string_input_popup popup_input;
            popup_input
            .title( string_format( _( "Batch crafting %s [MAX: %d]: " ), making->result_name(),
                                   camp_recipe_batch_max( *making, total_inv ) ) )
            .edit( batch_size );

            if( popup_input.canceled() || batch_size <= 0 ) {
                continue;
            }
            if( batch_size > camp_recipe_batch_max( *making, total_inv ) ) {
                popup( _( "Your batch is too large!" ) );
                continue;
            }
            int need_food = batch_size * time_to_food( time_duration::from_turns( making->time / 100 ) );
            for( const auto &e : om_expansions ) {
                if( om_simple_dir( point( omt_pos.x, omt_pos.y ), e.second ) == cur_key.dir ) {
                    std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, miss_id + cur_key.dir );
                    if( !npc_list.empty() ) {
                        popup( _( "You already have someone working in that expansion." ) );
                        continue;
                    }
                    npc *comp = individual_mission( p, _( "begins to work..." ), miss_id + cur_key.dir, false, {},
                                                    making->skill_used.obj().ident().c_str(), making->difficulty );
                    if( comp != nullptr ) {
                        g->u.consume_components_for_craft( making, batch_size, true );
                        g->u.invalidate_crafting_inventory();
                        for( auto results : making->create_results( batch_size ) ) {
                            comp->companion_mission_inv.add_item( results );
                        }
                        comp->companion_mission_time_ret = calendar::turn + ( time_duration::from_turns(
                                                               making->time / 100 ) * batch_size );
                        camp_food_supply( -need_food );
                    }
                }
            }
        }
    }
}

std::string talk_function::camp_recruit_evaluation( npc &p, const std::string &base,
        const std::vector<std::pair<std::string, tripoint>> &om_expansions,
        bool raw_score )
{
    int sbase = om_over_level( "faction_base_camp_11", base ) * 5;
    int sexpansions = om_expansions.size() * 2;
    for( auto expan : om_expansions ) {
        //The expansion is max upgraded
        if( om_next_upgrade( expan.first ) == "null" ) {
            sexpansions += 2;
        }
    }
    int sfaction = std::min( camp_food_supply() / 10000, 10 );
    sfaction += std::min( camp_discipline() / 10, 5 );
    sfaction += std::min( camp_morale() / 10, 5 );

    //Secret or Hidden Bonus
    //Please avoid openly discussing so that there is some mystery to the system
    int sbonus = 0;
    //How could we ever starve?
    //More than 5 farms at recruiting base
    int farm = 0;
    for( auto expan : om_expansions ) {
        if( om_min_level( "faction_base_farm_1", expan.first ) ) {
            farm++;
        }
    }
    if( farm >= 5 ) {
        sbonus += 10;
    }
    //More machine than man
    //Bionics count > 10, respect > 75
    if( g->u.my_bionics->size() > 10 && camp_discipline() > 75 ) {
        sbonus += 10;
    }
    //Survival of the fittest
    if( g->get_npc_kill().size() > 10 ) {
        sbonus += 10;
    }

    int total = sbase + sexpansions + sfaction + sbonus;
    std::string desc = string_format( _( "Notes:\n"
                                         "Recruiting additional followers is very dangerous and expensive.  The outcome is heavily dependent on the skill of the "
                                         "companion you send and the appeal of your base.\n \n"
                                         "Skill used: speech\n"
                                         "Difficulty: 2 \n"
                                         "Base Score:                   +%3d%%\n"
                                         "> Expansion Bonus:            +%3d%%\n"
                                         "> Faction Bonus:              +%3d%%\n"
                                         "> Special Bonus:              +%3d%%\n \n"
                                         "Total: Skill                  +%3d%%\n \n"
                                         "Risk: High\n"
                                         "Time: 4 Days\n"
                                         "Positions: %d/1\n" ), sbase, sexpansions, sfaction, sbonus, total, companion_list( p,
                                                 "_faction_camp_firewood" ).size()
                                    );
    if( raw_score ) {
        return to_string( total );
    }
    return desc;
}

void talk_function::camp_recruit_return( npc &p, const std::string &task, int score )
{
    npc *comp = companion_choose_return( p, task, calendar::turn - 4_days );
    if( comp == nullptr ) {
        return;
    }
    std::string skill_group = "recruiting";
    companion_skill_trainer( *comp, skill_group, 4_days, 2 );
    popup( _( "%s returns from searching for recruits with a bit more experience..." ),
           comp->name.c_str() );
    companion_return( *comp );

    std::shared_ptr<npc> recruit;
    //Success of finding an NPC to recruit, based on survival/tracking
    int skill = comp->get_skill_level( skill_survival );
    int dice = rng( 1, 20 );
    dice += skill - 2;
    if( dice > 15 ) {
        recruit = std::make_shared<npc>();
        recruit->normalize();
        recruit->randomize();
        popup( _( "%s encountered %s..." ), comp->name.c_str(), recruit->name.c_str() );
    } else {
        popup( _( "%s didn't find anyone to recruit..." ), comp->name.c_str() );
        return;
    }
    //Chance of convencing them to come back
    skill = comp->get_skill_level( skill_speech );
    skill = skill * ( 1 + ( score / 100.0 ) );
    dice = rng( 1, 20 );
    dice += skill - 4;
    if( dice > 15 ) {
        popup( _( "%s convinced %s to hear a recruitment offer from you..." ), comp->name.c_str(),
               recruit->name.c_str() );
    } else {
        popup( _( "%s wasn't interested in anything %s had to offer..." ), recruit->name.c_str(),
               comp->name.c_str() );
        return;
    }
    //Stat window
    int rec_m = 0;
    int appeal = rng( -5, 3 );
    appeal += std::min( skill / 3, 3 );
    int food_desire = rng( 0, 5 );
    while( rec_m >= 0 ) {
        std::string description = string_format( _( "NPC Overview:\n \n" ) );
        description += string_format( _( "Name:  %20s\n \n" ), recruit->name.c_str() );
        description += string_format( _( "Strength:        %10d\n" ), recruit->str_max );
        description += string_format( _( "Dexterity:       %10d\n" ), recruit->dex_max );
        description += string_format( _( "Intelligence:    %10d\n" ), recruit->int_max );
        description += string_format( _( "Perception:      %10d\n \n" ), recruit->per_max );
        description += string_format( _( "Top 3 Skills:\n" ) );

        const auto skillslist = Skill::get_skills_sorted_by( [&]( Skill const & a, Skill const & b ) {
            int const level_a = recruit->get_skill_level_object( a.ident() ).exercised_level();
            int const level_b = recruit->get_skill_level_object( b.ident() ).exercised_level();
            return level_a > level_b || ( level_a == level_b && a.name() < b.name() );
        } );

        description += string_format( "%12s:          %4d\n", skillslist[0]->ident().c_str(),
                                      recruit->get_skill_level_object( skillslist[0]->ident() ).level() );
        description += string_format( "%12s:          %4d\n", skillslist[1]->ident().c_str(),
                                      recruit->get_skill_level_object( skillslist[1]->ident() ).level() );
        description += string_format( "%12s:          %4d\n \n", skillslist[2]->ident().c_str(),
                                      recruit->get_skill_level_object( skillslist[2]->ident() ).level() );

        description += string_format( _( "Asking for:\n" ) );
        description += string_format( _( "> Food:     %10d days\n \n" ), food_desire );
        description += string_format( _( "Faction Food:%9d days\n \n" ), camp_food_supply( 0, true ) );
        description += string_format( _( "Recruit Chance: %10d%%\n \n" ),
                                      std::min( ( int )( ( 10.0 + appeal ) / 20.0 * 100 ), 100 ) );
        description += _( "Select an option:" );

        std::vector<std::string> rec_options;
        rec_options.push_back( _( "Increase Food" ) );
        rec_options.push_back( _( "Decrease Food" ) );
        rec_options.push_back( _( "Make Offer" ) );
        rec_options.push_back( _( "Not Interested" ) );

        rec_m = uilist( description, rec_options );
        if( rec_m < 0 || rec_m == 3 || static_cast<size_t>( rec_m ) >= rec_options.size() ) {
            popup( _( "You decide you aren't interested..." ) );
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
    //Roll for recruitment
    dice = rng( 1, 20 );
    if( dice + appeal >= 10 ) {
        popup( _( "%s has been convinced to join!" ), recruit->name.c_str() );
    } else {
        popup( _( "%s wasn't interested..." ), recruit->name.c_str() );
        return;// nullptr;
    }
    camp_food_supply( 1_days * food_desire );
    recruit->spawn_at_precise( { g->get_levx(), g->get_levy() }, g->u.pos() + point( -4, -4 ) );
    overmap_buffer.insert_npc( recruit );
    recruit->form_opinion( g->u );
    recruit->mission = NPC_MISSION_NULL;
    recruit->add_new_mission( mission::reserve_random( ORIGIN_ANY_NPC, recruit->global_omt_location(),
                              recruit->getID() ) );
    recruit->set_attitude( NPCATT_FOLLOW );
    g->load_npcs();
}

std::vector<tripoint> talk_function::om_companion_path( const tripoint &start, int range_start,
        bool bounce )
{
    std::vector<tripoint> scout_points;
    tripoint spt;
    tripoint last = start;
    int range = range_start;
    int def_range = range_start;
    while( range > 3 ) {
        spt = om_target_tile( last, 0, range, {}, false, true, last, false );
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
        range -= rl_dist( spt.x, spt.y, last.x, last.y );
        last = spt;

        oter_id &omt_ref = overmap_buffer.ter( last.x, last.y, g->u.posz() );

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

bool talk_function::om_camp_upgrade( npc &comp, const point &omt_pos )
{
    editmap edit;

    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, comp.posz() );
    std::string om_old = omt_ref.id().c_str();

    if( !edit.mapgen_set( om_next_upgrade( om_old ), tripoint( omt_pos.x, omt_pos.y, comp.posz() ) ) ) {
        return false;
    }

    tripoint np = tripoint( 5 * SEEX + 5 % SEEX, 4 * SEEY + 5 % SEEY, comp.posz() );
    np.x = g->u.posx() + 1;
    np.y = g->u.posy();

    tripoint p( 0, 0, comp.posz() );
    int &x = p.x;
    int &y = p.y;
    for( x = 0; x < SEEX * g->m.getmapsize(); x++ ) {
        for( y = 0; y < SEEY * g->m.getmapsize(); y++ ) {
            if( g->m.ter( x, y ) == ter_id( "t_floor_green" ) ) {
                np.x = x;
                np.y = y;
            }
        }
    }
    comp.setpos( np );
    comp.set_destination();
    g->load_npcs();
    return true;
}

std::string talk_function::om_upgrade_description( const std::string &bldg )
{
    const recipe *making = &recipe_id( bldg ).obj();
    const inventory &total_inv = g->u.crafting_inventory();

    std::vector<std::string> component_print_buffer;
    int pane = FULL_SCREEN_WIDTH;
    auto tools = making->requirements().get_folded_tools_list( pane, c_white, total_inv, 1 );
    auto comps = making->requirements().get_folded_components_list( pane, c_white, total_inv, 1 );
    component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
    component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

    std::string comp = "";
    for( auto &elem : component_print_buffer ) {
        comp = comp + elem + "\n";
    }
    comp = string_format(
               _( "Notes:\n%s\n \nSkill used: %s\nDifficulty: %d\n%s \nRisk: None\nTime: %s\n" ),
               making->description,
               making->skill_used.obj().ident().c_str(), making->difficulty, comp,
               to_string( time_duration::from_turns( making->time / 100 ) ) );
    return comp;
}

std::string talk_function::om_craft_description( const std::string &itm )
{
    recipe making = recipe_id( itm ).obj();
    const inventory &total_inv = g->u.crafting_inventory();

    std::vector<std::string> component_print_buffer;
    int pane = FULL_SCREEN_WIDTH;
    auto tools = making.requirements().get_folded_tools_list( pane, c_white, total_inv, 1 );
    auto comps = making.requirements().get_folded_components_list( pane, c_white, total_inv, 1 );

    component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
    component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

    std::string comp = "";
    for( auto &elem : component_print_buffer ) {
        comp = comp + elem + "\n";
    }
    comp = string_format( _( "Skill used: %s\nDifficulty: %d\n%s\nTime: %s\n" ),
                          making.skill_used.obj().name(), making.difficulty,
                          comp, to_string( time_duration::from_turns( making.time / 100 ) ) );
    return comp;
}

std::string talk_function::om_gathering_description( npc &p, const std::string &bldg )
{
    std::string itemlist;
    if( item_group::group_is_defined( "gathering_" + bldg ) ) {
        itemlist = "gathering_" + bldg;
    } else {
        itemlist = "forest" ;
    }
    std::string output =
        _( "Notes: \nSend a companion to gather materials for the next camp upgrade.\n \n"
           "Skill used: survival\n"
           "Difficulty: N/A \n"
           "Gathering Possibilities:\n" );

    // Functions like the debug item group tester but only rolls 6 times so the player doesn't have perfect knowledge
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
    std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_gathering" );
    output = output + _( " \nRisk: Very Low\n"
                         "Time: 3 Hours, Repeated\n"
                         "Positions: " ) + to_string( npc_list.size() ) + "/3\n";
    return output;
}

std::string talk_function::om_next_upgrade( const std::string &bldg )
{
    int phase = bldg.find_last_of( '_' );
    std::string comp = bldg.substr( phase + 1 );
    int value = atoi( comp.c_str() ) + 1;
    comp = bldg.substr( 0, phase + 1 ) + to_string( value );
    if( !oter_str_id( comp ).is_valid() ) {
        return "null";
    }

    return comp;
}

std::vector<std::string> talk_function::om_all_upgrade_levels( const std::string &bldg )
{
    std::vector<std::string> upgrades;
    int phase = bldg.find_last_of( '_' );
    std::string comp = bldg.substr( phase + 1 );
    int value = 0;
    int current = atoi( comp.c_str() );
    while( value <= current ) {
        comp = bldg.substr( 0, phase + 1 ) + to_string( value );
        if( oter_str_id( comp ).is_valid() ) {
            upgrades.push_back( comp );
        }
        value++;
    }
    return upgrades;
}

bool talk_function::om_min_level( const std::string &target, const std::string &bldg )
{
    return ( om_over_level( target, bldg ) >= 0 );
}

int talk_function::om_over_level( const std::string &target, const std::string &bldg )
{
    int diff = 0;
    int phase_target = target.find_last_of( '_' );
    int phase_bldg = bldg.find_last_of( '_' );
    //comparing two different buildings
    if( target.substr( 0, phase_target + 1 ) != bldg.substr( 0, phase_bldg + 1 ) ) {
        return -1;
    }
    diff = atoi( bldg.substr( phase_bldg + 1 ).c_str() ) - atoi( target.substr(
                phase_target + 1 ).c_str() );
    //not high enough level
    if( diff < 0 ) {
        return -1;
    }
    return diff;
}

bool talk_function::upgrade_return( npc &p, const point &omt_pos, const std::string &miss )
{
    //Ensure there are no vehicles before we update
    editmap edit;
    if( edit.mapgen_veh_has( tripoint( omt_pos.x, omt_pos.y, p.posz() ) ) ) {
        popup( _( "Engine cannot support merging vehicles from two overmaps, please remove them from the OM tile." ) );
        return false;
    }

    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, p.posz() );
    std::string bldg = omt_ref.id().c_str();
    if( bldg == "field" ) {
        bldg = "faction_base_camp_1";
    } else {
        bldg = om_next_upgrade( bldg );
    }
    const recipe *making = &recipe_id( bldg ).obj();

    npc *comp = companion_choose_return( p, miss,
                                         calendar::turn - time_duration::from_turns( making->time / 100 ) );

    if( comp == nullptr || !om_camp_upgrade( p, omt_pos ) ) {
        return false;
    }
    companion_skill_trainer( *comp, "construction", time_duration::from_turns( making->time / 100 ),
                             making->difficulty );
    popup( _( "%s returns from upgrading the camp having earned a bit of experience..." ),
           comp->name.c_str() );
    companion_return( *comp );
    return true;
}

bool talk_function::camp_gathering_return( npc &p, const std::string &task )
{
    npc *comp = companion_choose_return( p, task, calendar::turn - 3_hours );
    if( comp == nullptr ) {
        return false;
    }

    if( one_in( 20 ) ) {
        popup( _( "While gathering supplies, a silent specter approaches %s..." ), comp->name.c_str() );
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_SURVIVAL_NPC affects gathering mission results

        ///\EFFECT_DODGE_NPC affects gathering mission results
        int skill_1 = comp->get_skill_level( skill_survival );
        int skill_2 = comp->get_skill_level( skill_speech );
        if( skill_1 > rng( -2, 8 ) ) {
            popup( _( "%s notices the antlered horror and slips away before it gets too close." ),
                   comp->name.c_str() );
        } else if( skill_2 > rng( -2, 8 ) ) {
            popup( _( "The survivor approaches %s asking for directions." ), comp->name.c_str() );
            popup( _( "Fearful that he may be an agent of some hostile faction, %s doesn't mention the camp." ),
                   comp->name.c_str() );
            popup( _( "The two part on friendly terms and the survivor isn't seen again." ) );
        } else {
            popup( _( "%s didn't detect the ambush until it was too late!" ), comp->name.c_str() );
            // the following doxygen aliases do not yet exist. this is marked for future reference

            ///\EFFECT_MELEE_NPC affects forage mission results

            ///\EFFECT_SURVIVAL_NPC affects forage mission results

            ///\EFFECT_BASHING_NPC affects forage mission results

            ///\EFFECT_CUTTING_NPC affects forage mission results

            ///\EFFECT_STABBING_NPC affects forage mission results

            ///\EFFECT_UNARMED_NPC affects forage mission results

            ///\EFFECT_DODGE_NPC affects forage mission results
            int skill = comp->get_skill_level( skill_melee ) + ( .5 * comp->get_skill_level(
                            skill_survival ) ) + comp->get_skill_level( skill_bashing ) +
                        comp->get_skill_level( skill_cutting ) + comp->get_skill_level( skill_stabbing ) +
                        comp->get_skill_level( skill_unarmed )
                        + comp->get_skill_level( skill_dodge );
            int monsters = rng( 0, 10 );
            if( skill * rng_float( .80, 1.2 ) > ( monsters * rng_float( .8, 1.2 ) ) ) {
                if( one_in( 2 ) ) {
                    popup( _( "The bull moose charged %s from the tree line..." ), comp->name.c_str() );
                    popup( _( "Despite being caught off guard %s was able to run away until the moose gave up pursuit." ),
                           comp->name.c_str() );
                } else {
                    popup( _( "The jabberwock grabbed %s by the arm from behind and began to scream." ),
                           comp->name.c_str() );
                    popup( _( "Terrified, %s spun around and delivered a massive kick to the creature's torso..." ),
                           comp->name.c_str() );
                    popup( _( "Collapsing into a pile of gore, %s walked away unscathed..." ), comp->name.c_str() );
                    popup( _( "(Sounds like bullshit, you wonder what really happened.)" ) );
                }
            } else {
                if( one_in( 2 ) ) {
                    popup( _( "%s turned to find the hideous black eyes of a giant wasp staring back from only a few feet away..." ),
                           comp->name.c_str() );
                    popup( _( "The screams were terrifying, there was nothing anyone could do." ) );
                } else {
                    popup( _( "Pieces of %s were found strewn across a few bushes." ), comp->name.c_str() );
                    popup( _( "(You wonder if your companions are fit to work on their own...)" ) );
                }
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }

    int exp = to_hours<float>( calendar::turn - comp->companion_mission_time );
    std::string skill_group = "gathering";
    companion_skill_trainer( *comp, skill_group, calendar::turn - comp->companion_mission_time, 1 );

    popup( _( "%s returns from gathering materials carrying supplies and has a bit more experience..." ),
           comp->name.c_str() );
    // the following doxygen aliases do not yet exist. this is marked for future reference

    ///\EFFECT_SURVIVAL_NPC affects forage mission results
    int skill = comp->get_skill_level( skill_survival );

    int need_food = time_to_food( calendar::turn - comp->companion_mission_time );
    if( camp_food_supply() < need_food ) {
        popup( _( "Your companion seems disappointed that your pantry is empty..." ) );
    }
    camp_food_supply( -need_food );

    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, p.posz() );
    std::string om_tile = omt_ref.id().c_str();
    std::string itemlist = "forest";

    if( task == "_faction_camp_gathering" ) {
        if( item_group::group_is_defined( "gathering_" + om_tile ) ) {
            itemlist = "gathering_" + om_tile ;
        }
    }
    if( task == "_faction_camp_firewood" ) {
        itemlist = "gathering_faction_base_camp_firewood";
    }

    int i = 0;
    while( i < exp ) {
        if( skill > rng_float( -.5, 15 ) ) {
            auto result = item_group::item_from( itemlist );
            g->m.add_item_or_charges( g->u.pos(), result, true );
            i += 2;
        }
    }
    companion_return( *comp );
    return true;
}

bool talk_function::camp_garage_chop_start( npc &p, const std::string &task )
{
    std::string dir = camp_direction( task );
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    tripoint omt_trg;
    std::vector<std::pair<std::string, tripoint>> om_expansions = om_building_region( p, 1, true );
    for( const auto &e : om_expansions ) {
        if( dir == om_simple_dir( omt_pos, e.second ) ) {
            omt_trg = e.second;
        }
    }

    oter_id &omt_ref = overmap_buffer.ter( omt_trg.x, omt_trg.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    editmap edit;
    vehicle *car = edit.mapgen_veh_query( omt_trg );
    if( car == nullptr ) {
        return false;
    }

    if( !query_yn( _( "       Chopping this vehicle:\n%s" ), camp_car_description( car ) ) ) {
        return false;
    }

    npc *comp = individual_mission( p, _( "begins working in the garage..." ),
                                    "_faction_exp_chop_shop_" + dir, false, {}, "mechanics", 2 );
    if( comp == nullptr ) {
        return false;
    }

    //Chopping up the car!
    std::vector<vehicle_part> p_all = car->parts;
    int prt = 0;
    int skillLevel = comp->get_skill_level( skill_mechanics );
    while( p_all.size() > 0 ) {
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
            companion_skill_trainer( *comp, skill_mechanics, 1_hours, p_all[ prt].info().difficulty );
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
                p_all[prt].ammo_consume( p_all[prt].ammo_capacity(), car->global_part_pos3( p_all[prt] ) );
            }
            comp->companion_mission_inv.add_item( p_all[prt].properties_to_item() );
        } else if( !skill_destroy ) {
            car->break_part_into_pieces( prt, comp->posx(), comp->posy() );
        }
        p_all.erase( p_all.begin() + 0 );
    }
    companion_skill_trainer( *comp, skill_mechanics, 5_days, 2 );
    edit.mapgen_veh_destroy( omt_trg, car );
    return true;
}

bool talk_function::camp_farm_return( npc &p, const std::string &task, bool harvest, bool plant,
                                      bool plow )
{
    std::string dir = camp_direction( task );
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    tripoint omt_trg;
    std::vector<std::pair<std::string, tripoint>> om_expansions = om_building_region( p, 1, true );
    for( const auto &e : om_expansions ) {
        if( dir == om_simple_dir( omt_pos, e.second ) ) {
            omt_trg = e.second;
        }
    }
    int harvestable = 0;
    int plots_empty = 0;
    int plots_plow = 0;

    oter_id &omt_ref = overmap_buffer.ter( omt_trg.x, omt_trg.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    //bay_json is what the are should look like according to jsons
    tinymap bay_json;
    bay_json.generate( omt_trg.x * 2, omt_trg.y * 2, g->u.posz(), calendar::turn );
    //bay is what the area actually looks like
    tinymap bay;
    bay.load( omt_trg.x * 2, omt_trg.y * 2, g->u.posz(), false );
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            //Needs to be plowed to match json
            if( bay_json.ter( x, y ) == ter_str_id( "t_dirtmound" )
                && ( bay.ter( x, y ) == ter_str_id( "t_dirt" ) || bay.ter( x, y ) == ter_str_id( "t_grass" ) )
                && bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                plots_plow++;
            }
            if( bay.ter( x, y ) == ter_str_id( "t_dirtmound" ) &&
                bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                plots_empty++;
            }
            if( bay.furn( x, y ) == furn_str_id( "f_plant_harvest" ) && !bay.i_at( x, y ).empty() ) {
                const item &seed = bay.i_at( x, y )[0];
                if( seed.is_seed() ) {
                    harvestable++;
                }
            }
        }
    }
    time_duration work = 0_minutes;
    if( harvest ) {
        work += 3_minutes * harvestable;
    }
    if( plant ) {
        work += 1_minutes * plots_empty;
    }
    if( plow ) {
        work += 5_minutes * plots_plow;
    }

    npc *comp = companion_choose_return( p, task, calendar::turn - work );
    if( comp == nullptr ) {
        return false;
    }

    std::vector<item *> seed_inv = comp->companion_mission_inv.items_with( []( const item & itm ) {
        return itm.is_seed() && itm.typeId() != "marloss_seed" && itm.typeId() != "fungal_seeds";
    } );

    if( plant && seed_inv.empty() ) {
        popup( _( "No seeds to plant!" ) );
    }

    //Now that we know we have spent enough time working, we can update the map itself.
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            //Needs to be plowed to match json
            if( plow && bay_json.ter( x, y ) == ter_str_id( "t_dirtmound" )
                && ( bay.ter( x, y ) == ter_str_id( "t_dirt" ) || bay.ter( x, y ) == ter_str_id( "t_grass" ) )
                && bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                bay.ter_set( x, y, t_dirtmound );
            }
            if( plant && bay.ter( x, y ) == ter_str_id( "t_dirtmound" ) &&
                bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                if( !seed_inv.empty() ) {
                    item *tmp_seed = seed_inv.back();
                    seed_inv.pop_back();
                    std::list<item> used_seed;
                    if( item::count_by_charges( tmp_seed->typeId() ) ) {
                        used_seed.push_back( *tmp_seed );
                        tmp_seed->charges -= 1;
                        if( tmp_seed->charges > 0 ) {
                            seed_inv.push_back( tmp_seed );
                        }
                    }
                    used_seed.front().set_age( 0 );
                    bay.add_item_or_charges( x, y, used_seed.front() );
                    bay.set( x, y, t_dirt, f_plant_seed );
                }
            }
            if( harvest && bay.furn( x, y ) == furn_str_id( "f_plant_harvest" ) && !bay.i_at( x, y ).empty() ) {
                const item &seed = bay.i_at( x, y )[0];
                if( seed.is_seed() && seed.typeId() != "fungal_seeds" && seed.typeId() != "marloss_seed" ) {
                    const itype &type = *seed.type;
                    int skillLevel = comp->get_skill_level( skill_survival );
                    ///\EFFECT_SURVIVAL increases number of plants harvested from a seed
                    int plantCount = rng( skillLevel / 2, skillLevel );
                    //this differs from
                    if( plantCount >= 9 ) {
                        plantCount = 9;
                    } else if( plantCount <= 0 ) {
                        plantCount = 1;
                    }
                    const int seedCount = std::max( 1l, rng( plantCount / 4, plantCount / 2 ) );
                    for( auto &i : iexamine::get_harvest_items( type, plantCount, seedCount, true ) ) {
                        g->m.add_item_or_charges( g->u.posx(), g->u.posy(), i );
                    }
                    bay.i_clear( x, y );
                    bay.furn_set( x, y, f_null );
                    bay.ter_set( x, y, t_dirt );
                }
            }
        }
    }
    bay.save();

    //Give any seeds the NPC didn't use back to you.
    for( size_t i = 0; i < comp->companion_mission_inv.size(); i++ ) {
        for( const auto &it : comp->companion_mission_inv.const_stack( i ) ) {
            if( it.charges > 0 ) {
                g->u.i_add( it );
            }
        }
    }
    comp->companion_mission_inv.clear();

    int need_food = time_to_food( work );
    if( camp_food_supply() < need_food ) {
        popup( _( "Your companion seems disappointed that your pantry is empty..." ) );
    }
    camp_food_supply( -need_food );
    companion_skill_trainer( *comp, skill_survival, work, 2 );

    popup( _( "%s returns from working your fields..." ), comp->name.c_str() );
    companion_return( *comp );
    return true;
}

bool talk_function::camp_menial_return( npc &p )
{
    npc *comp = companion_choose_return( p, "_faction_camp_menial", calendar::before_time_starts );
    if( comp == nullptr ) {
        return false;
    }

    companion_skill_trainer( *comp, "menial", 3_hours, 2 );

    popup( _( "%s returns from doing the dirty work to keep the camp running..." ),
           comp->name.c_str() );
    if( p.companion_mission_points.size() < COMPANION_SORT_POINTS ) {
        popup( _( "Sorting points have changed, forcing reset." ) );
        camp_menial_sort_pts( p, true, true );
    }
    tripoint p_food = p.companion_mission_points[0];
    tripoint p_seed = p.companion_mission_points[2];
    tripoint p_weapon = p.companion_mission_points[3];
    tripoint p_clothing = p.companion_mission_points[4];
    tripoint p_bionic = p.companion_mission_points[5];
    tripoint p_tool = p.companion_mission_points[6];
    tripoint p_wood = p.companion_mission_points[7];
    tripoint p_trash = p.companion_mission_points[8];
    tripoint p_book = p.companion_mission_points[9];
    tripoint p_medication = p.companion_mission_points[10];
    tripoint p_ammo = p.companion_mission_points[11];

    //This prevents the loop from getting stuck on the piles in the open
    for( size_t x = 0; x < p.companion_mission_points.size() ; x++ ) {
        if( g->m.furn( p.companion_mission_points[x] ) == f_null ) {
            g->m.furn_set( p.companion_mission_points[x], f_ash );
        }
    }
    for( const tripoint &tmp : g->m.points_in_radius( g->u.pos(), 72 ) ) {
        if( !g->m.has_furn( tmp ) ) {
            for( auto &i : g->m.i_at( tmp ) ) {
                if( !i.made_of( LIQUID ) ) {
                    if( i.is_comestible() && i.rotten() ) {
                        g->m.add_item_or_charges( p_trash, i, true );
                    } else if( i.is_seed() ) {
                        g->m.add_item_or_charges( p_seed, i, true );
                    } else if( i.is_food() ) {
                        g->m.add_item_or_charges( p_food, i, true );
                    } else if( i.is_corpse() ) {
                        g->m.add_item_or_charges( p_trash, i, true );
                    } else if( i.is_book() ) {
                        g->m.add_item_or_charges( p_book, i, true );
                    } else if( i.is_bionic() ) {
                        g->m.add_item_or_charges( p_bionic, i, true );
                    } else if( i.is_medication() ) {
                        g->m.add_item_or_charges( p_medication, i, true );
                    } else if( i.is_tool() ) {
                        g->m.add_item_or_charges( p_tool, i, true );
                    } else if( i.is_gun() ) {
                        g->m.add_item_or_charges( p_weapon, i, true );
                    } else if( i.is_ammo() ) {
                        g->m.add_item_or_charges( p_ammo, i, true );
                    } else if( i.is_armor() ) {
                        g->m.add_item_or_charges( p_clothing, i, true );
                    } else if( i.typeId() == "log" || i.typeId() == "splinter" || i.typeId() == "stick" ||
                               i.typeId() == "2x4" ) {
                        g->m.add_item_or_charges( p_wood, i, true );
                    } else {
                        g->m.add_item_or_charges( p_tool, i, true );
                    }
                }
            }
            g->m.i_clear( tmp );
        }
    }
    //Undo our hack!
    for( size_t x = 0; x < p.companion_mission_points.size() ; x++ ) {
        if( g->m.furn( p.companion_mission_points[x] ) == f_ash ) {
            g->m.furn_set( p.companion_mission_points[x], f_null );
        }
    }
    companion_return( *comp );
    return true;
}

bool talk_function::camp_expansion_select( npc &p )
{
    npc *comp = companion_choose_return( p, "_faction_camp_expansion", calendar::before_time_starts );
    if( comp == nullptr ) {
        return false;
    }
    std::vector<std::string> pos_expansion_name_id;
    std::vector<std::string> pos_expansion_name;
    std::map<std::string, std::string> pos_expansions =
        recipe_group::get_recipes( "all_faction_base_expansions" );
    for( std::map<std::string, std::string>::const_iterator it = pos_expansions.begin();
         it != pos_expansions.end(); ++it ) {
        pos_expansion_name.push_back( it->first );
        pos_expansion_name_id.push_back( it->second );
    }

    const int expan = uilist( _( "Select an expansion:" ), pos_expansion_name );
    if( expan < 0 || static_cast<size_t>( expan ) >= pos_expansion_name_id.size() ) {
        popup( _( "You choose to wait..." ) );
        return false;
    }
    editmap edit;
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    if( !edit.mapgen_set( pos_expansion_name_id[expan], tripoint( omt_pos.x, omt_pos.y, p.posz() ),
                          1 ) ) {
        return false;
    }
    companion_skill_trainer( *comp, "construction", 3_hours, 2 );
    popup( _( "%s returns from surveying for the expansion." ), comp->name.c_str() );
    companion_return( *comp );
    return true;
}

bool talk_function::camp_distribute_food( npc &p )
{
    if( p.companion_mission_points.size() < COMPANION_SORT_POINTS ) {
        popup( _( "Sorting points have changed, forcing reset." ) );
        camp_menial_sort_pts( p, true, true );
    }
    tripoint p_food_stock = p.companion_mission_points[1];
    tripoint p_trash = p.companion_mission_points[8];
    tripoint p_litter = tripoint( p.posx() - 7, p.posy(), p.posz() );
    tripoint p_tool = p.companion_mission_points[6];

    if( g->m.i_at( p_food_stock ).empty() ) {
        popup( _( "No items are located at the drop point..." ) );
        return false;
    }

    int total = 0;
    for( auto &i : g->m.i_at( p_food_stock ) ) {
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
        if( i.is_comestible() && ( i.rotten() || i.type->comestible->fun < -6 ) ) {
            g->m.add_item_or_charges( p_trash, i, false );
        } else if( i.is_food() ) {
            float rot_multip;
            int rots_in = to_days<int>( time_duration::from_turns( i.spoilage_sort_order() ) );
            if( rots_in >= 5 ) {
                rot_multip = 1.00;
            } else if( rots_in >= 2 ) {
                rot_multip = .80;
            } else {
                rot_multip = .60;
            }
            if( i.count_by_charges() ) {
                total += i.type->comestible->nutr * i.charges * rot_multip;
            } else {
                total += i.type->comestible->nutr * rot_multip;
            }
        } else if( i.is_corpse() ) {
            g->m.add_item_or_charges( p_trash, i, false );
        } else {
            g->m.add_item_or_charges( p_tool, i, false );
        }
    }
    g->m.i_clear( p_food_stock );
    popup( _( "You distribute %d kcal worth of food to your companions." ), total * 10 );
    camp_food_supply( total );
    return true;
}

std::vector<std::pair<std::string, tripoint>> talk_function::om_building_region( npc &p, int range,
        bool purge )
{
    std::vector<std::pair<std::string, tripoint>> om_camp_region;
    for( int x = -range; x <= range; x++ ) {
        for( int y = -range; y <= range; y++ ) {
            const point omt_near = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
            oter_id &omt_rnear = overmap_buffer.ter( omt_near.x + x, omt_near.y + y, p.posz() );
            std::string om_near = omt_rnear.id().c_str();
            om_camp_region.push_back( std::make_pair( om_near, tripoint( omt_near.x + x, omt_near.y + y,
                                      p.posz() ) ) );
        }
    }
    if( purge ) {
        std::vector<std::pair<std::string, tripoint>> om_expansions;
        for( const auto &e : om_camp_region ) {
            if( e.first.find( "faction_base_" ) != std::string::npos &&
                e.first.find( "faction_base_camp" ) == std::string::npos ) {
                om_expansions.push_back( e );
            }
        }
        return om_expansions;
    }
    return om_camp_region;
}

std::string talk_function::om_simple_dir( const point &omt_pos, const tripoint &omt_tar )
{
    std::string dr = "[";
    if( omt_tar.y < omt_pos.y ) {
        dr += "N";
    }
    if( omt_tar.y > omt_pos.y ) {
        dr += "S";
    }
    if( omt_tar.x < omt_pos.x ) {
        dr += "W";
    }
    if( omt_tar.x > omt_pos.x ) {
        dr += "E";
    }
    dr += "]";
    if( omt_tar.x == omt_pos.x && omt_tar.y == omt_pos.y ) {
        return "[B]";
    }
    return dr;
}

std::string talk_function::camp_farm_description( const point &omt_pos, bool harvest, bool plots,
        bool plow )
{
    std::vector<std::string> plant_names;
    int harvestable = 0;
    int plots_empty = 0;
    int plots_plow = 0;

    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    //bay_json is what the are should look like according to jsons
    tinymap bay_json;
    bay_json.generate( omt_pos.x * 2, omt_pos.y * 2, g->u.posz(), calendar::turn );
    //bay is what the area actually looks like
    tinymap bay;
    bay.load( omt_pos.x * 2, omt_pos.y * 2, g->u.posz(), false );
    for( int x = 0; x < 23; x++ ) {
        for( int y = 0; y < 23; y++ ) {
            //Needs to be plowed to match json
            if( bay_json.ter( x, y ) == ter_str_id( "t_dirtmound" )
                && ( bay.ter( x, y ) == ter_str_id( "t_dirt" ) || bay.ter( x, y ) == ter_str_id( "t_grass" ) )
                && bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                plots_plow++;
            }
            if( bay.ter( x, y ) == ter_str_id( "t_dirtmound" ) &&
                bay.furn( x, y ) == furn_str_id( "f_null" ) ) {
                plots_empty++;
            }
            if( bay.furn( x, y ) == furn_str_id( "f_plant_harvest" ) && !bay.i_at( x, y ).empty() ) {
                const item &seed = bay.i_at( x, y )[0];
                if( seed.is_seed() ) {
                    harvestable++;
                    const islot_seed &seed_data = *seed.type->seed;
                    item tmp = item( seed_data.fruit_id, calendar::turn );
                    bool check = false;
                    for( auto elem : plant_names ) {
                        if( elem == tmp.type_name( 3 ).c_str() ) {
                            check = true;
                        }
                    }
                    if( !check ) {
                        plant_names.push_back( tmp.type_name( 3 ).c_str() );
                    }
                }
            }
        }
    }

    std::string crops;
    int total_c = 0;
    for( auto i : plant_names ) {
        if( total_c < 5 ) {
            crops += "\t" + i + " \n";
            total_c++;
        } else if( total_c == 5 ) {
            crops += "+ more \n";
            total_c++;
        }
    }
    std::string entry;
    if( harvest ) {
        entry += _( "Harvestable: " ) + to_string( harvestable ) + " \n" + crops;
    }
    if( plots ) {
        entry += _( "Ready for Planting: " ) + to_string( plots_empty ) + " \n";
    }
    if( plow ) {
        entry += _( "Needs Plowing: " ) + to_string( plots_plow ) + " \n";
    }
    return entry;
}

std::string talk_function::camp_car_description( vehicle *car )
{
    std::string entry = string_format( _( "Name:     %25s\n" ), car->name );
    entry += _( "----          Engines          ----\n" );
    for( const vpart_reference &vpr : car->get_parts( "ENGINE" ) ) {
        const vehicle_part &pt = vpr.vehicle().parts[vpr.part_index()];
        const vpart_info &vp = pt.info();
        entry += string_format( _( "Engine:  %25s\n" ), vp.name() );
        entry += string_format( _( ">Status:  %24d%%\n" ), static_cast<int>( 100 * pt.health_percent() ) );
        entry += string_format( _( ">Fuel:    %25s\n" ), vp.fuel_type );
    }
    std::map<itype_id, long> fuels = car->fuels_left();
    entry += _( "----  Fuel Storage & Battery   ----\n" );
    for( std::map<itype_id, long>::iterator it = fuels.begin(); it != fuels.end(); ++it ) {
        std::string fuel_entry = string_format( "%d/%d", car->fuel_left( it->first ),
                                                car->fuel_capacity( it->first ) );
        entry += string_format( ">%s:%*s\n", item( it->first ).tname(),
                                33 - item( it->first ).tname().length(), fuel_entry );
    }
    for( auto &pt : car->parts ) {
        if( pt.is_battery() ) {
            const vpart_info &vp = pt.info();
            entry += string_format( ">%s:%*d%%\n", vp.name(), 32 - vp.name().length(),
                                    int( 100.0 * pt.ammo_remaining() / pt.ammo_capacity() ) );
        }
    }
    entry += " \n";
    entry += _( "Estimated Chop Time:         5 Days\n" );
    return entry;
}

std::string talk_function::camp_direction( const std::string &line )
{
    return line.substr( line.find_last_of( '[' ),
                        line.find_last_of( ']' ) - line.find_last_of( '[' ) + 1 );
}

int talk_function::camp_food_supply( int change, bool return_days )
{
    faction *yours = g->faction_manager_ptr->get( faction_id( "your_followers" ) );
    yours->food_supply += change;
    if( yours->food_supply < 0 ) {
        yours->likes_u += yours->food_supply / 500;
        yours->respects_u += yours->food_supply / 100;
        yours->food_supply = 0;
    }
    if( return_days ) {
        return 10 * yours->food_supply / 2600;
    }

    return yours->food_supply;
}

int talk_function::camp_food_supply( time_duration work )
{
    int nut_consumption = time_to_food( work );
    g->faction_manager_ptr->get( faction_id( "your_followers" ) )-> food_supply -= nut_consumption;
    return g->faction_manager_ptr->get( faction_id( "your_followers" ) )-> food_supply;
}

int talk_function::time_to_food( time_duration work )
{
    return ( 260 / 24.0 ) * to_hours<float>( work );
}

int talk_function::camp_discipline( int change )
{
    faction *yours = g->faction_manager_ptr->get( faction_id( "your_followers" ) );
    yours->respects_u += change;
    return yours->respects_u;
}

int talk_function::camp_morale( int change )
{
    faction *yours = g->faction_manager_ptr->get( faction_id( "your_followers" ) );
    yours->likes_u += change;
    return yours->likes_u;
}
