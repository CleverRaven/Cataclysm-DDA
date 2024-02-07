#include "mission_companion.h"

#include <algorithm>
#include <cstdlib>
#include <list>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include "basecamp.h"
#include "bodypart.h"
#include "calendar.h"
#include "cata_assert.h"
#include "catacharset.h"
#include "character.h"
#include "colony.h"
#include "color.h"
#include "coordinates.h"
#include "creature.h"
#include "creature_tracker.h"
#include "cursesdef.h"
#include "debug.h"
#include "debug_menu.h"
#include "enums.h"
#include "faction.h"
#include "faction_camp.h"
#include "game.h"
#include "game_constants.h"
#include "input_context.h"
#include "inventory.h"
#include "item.h"
#include "item_group.h"
#include "item_stack.h"
#include "itype.h"
#include "line.h"
#include "map.h"
#include "map_iterator.h"
#include "mapdata.h"
#include "memory_fast.h"
#include "messages.h"
#include "monster.h"
#include "mtype.h"
#include "npc.h"
#include "npctrade_utils.h"
#include "output.h"
#include "overmap.h"
#include "overmapbuffer.h"
#include "point.h"
#include "rng.h"
#include "skill.h"
#include "string_formatter.h"
#include "string_input_popup.h"
#include "translations.h"
#include "ui.h"
#include "ui_manager.h"
#include "value_ptr.h"
#include "weather.h"
#include "weighted_list.h"

class character_id;

static const efftype_id effect_riding( "riding" );

static const furn_str_id furn_f_plant_harvest( "f_plant_harvest" );

static const item_group_id Item_spawn_data_farming_seeds( "farming_seeds" );
static const item_group_id Item_spawn_data_forage_autumn( "forage_autumn" );
static const item_group_id Item_spawn_data_forage_spring( "forage_spring" );
static const item_group_id Item_spawn_data_forage_summer( "forage_summer" );
static const item_group_id Item_spawn_data_forage_winter( "forage_winter" );
static const item_group_id Item_spawn_data_npc_weapon_random( "npc_weapon_random" );

static const itype_id itype_FMCNote( "FMCNote" );
static const itype_id itype_fungal_seeds( "fungal_seeds" );
static const itype_id itype_marloss_seed( "marloss_seed" );

static const oter_str_id oter_looted_hospital( "looted_hospital" );
static const oter_str_id oter_looted_hospital_roof( "looted_hospital_roof" );
static const oter_str_id oter_looted_house( "looted_house" );
static const oter_str_id oter_looted_house_basement( "looted_house_basement" );
static const oter_str_id oter_looted_house_roof( "looted_house_roof" );
static const oter_str_id oter_open_air( "open_air" );

static const skill_id skill_bashing( "bashing" );
static const skill_id skill_cutting( "cutting" );
static const skill_id skill_dodge( "dodge" );
static const skill_id skill_fabrication( "fabrication" );
static const skill_id skill_gun( "gun" );
static const skill_id skill_melee( "melee" );
static const skill_id skill_stabbing( "stabbing" );
static const skill_id skill_survival( "survival" );
static const skill_id skill_unarmed( "unarmed" );

static const string_id<class npc_template> npc_template_bandit( "bandit" );

static const string_id<class npc_template> npc_template_commune_guard( "commune_guard" );

static const string_id<class npc_template> npc_template_thug( "thug" );

static const trait_id trait_DEBUG_HS( "DEBUG_HS" );
static const trait_id trait_DEBUG_HS_LIGHT( "DEBUG_HS_LIGHT" );
static const trait_id trait_NPC_CONSTRUCTION_LEV_2( "NPC_CONSTRUCTION_LEV_2" );
static const trait_id trait_NPC_MISSION_LEV_1( "NPC_MISSION_LEV_1" );

static const std::string var_DOCTOR_ANESTHETIC_SCAVENGERS_HELPED =
    "npctalk_var_mission_tacoma_ranch_doctor_anesthetic_scavengers_helped";
static const std::string var_PURCHASED_FIELD_1_FENCE =
    "npctalk_var_dialogue_tacoma_ranch_purchased_field_1_fence";
static const std::string var_SCAVENGER_HOSPITAL_RAID =
    "npctalk_var_mission_tacoma_ranch_scavenger_hospital_raid";
static const std::string var_SCAVENGER_HOSPITAL_RAID_STARTED =
    "npctalk_var_mission_tacoma_ranch_scavenger_hospital_raid_started";

static const std::string role_id_faction_camp = "FACTION_CAMP";

static const std::string omt_evac_center_18 = "evac_center_18";
static const std::string omt_ranch_camp_63 = "ranch_camp_63";

static const std::string return_ally_question_string =
    "\n\nDo you wish to bring your allies back into your party?";

//  Legacy faction camp mission strings used to translate tasks in progress when upgrading.
static const std::string camp_upgrade_npc_string = "_faction_upgrade_camp";
static const std::string camp_upgrade_expansion_npc_string = "_faction_upgrade_exp_";

static const std::string caravan_commune_center_job_assign_parameter = "Assign";
static const std::string caravan_commune_center_job_active_parameter = "Active";

struct miss_data {
    std::string serialize_id;  // Serialized string for enum
    translation action;        // Optional extended UI description of task for return.
};
namespace io
{

template<>
std::string enum_to_string<mission_kind>( mission_kind data )
{
    switch( data ) {
        // *INDENT-OFF*
        case mission_kind::No_Mission: return "No_Mission";
        case mission_kind::Scavenging_Patrol_Job: return "Scavenging_Patrol_Job";
        case mission_kind::Scavenging_Raid_Job: return "Scavenging_Raid_Job";
        case mission_kind::Hospital_Raid_Job: return "Hospital_Raid_Job";
        case mission_kind::Menial_Job: return "Menial_Job";
        case mission_kind::Carpentry_Job: return "Carpentry_Job";
        case mission_kind::Forage_Job: return "Forage_Job";
        case mission_kind::Caravan_Commune_Center_Job: return "Caravan_Commune_Center_Job";
        case mission_kind::Camp_Distribute_Food: return "Camp_Distribute_Food";
		case mission_kind::Camp_Determine_Leadership: return "Camp_Determine_Leadership";
        case mission_kind::Camp_Hide_Mission: return "Camp_Hide_Mission";
        case mission_kind::Camp_Reveal_Mission: return "Camp_Reveal_Mission";
        case mission_kind::Camp_Assign_Jobs: return "Camp_Assign_Jobs";
        case mission_kind::Camp_Assign_Workers: return "Camp_Assign_Workers";
        case mission_kind::Camp_Abandon: return "Camp_Abandon";
        case mission_kind::Camp_Upgrade: return "Camp_Upgrade";
        case mission_kind::Camp_Emergency_Recall: return "Camp_Emergency_Recall";
        case mission_kind::Camp_Crafting: return "Camp_Crafting";
        case mission_kind::Camp_Gather_Materials: return "Camp_Gather_Materials";
        case mission_kind::Camp_Collect_Firewood: return "Camp_Collect_Firewood";
        case mission_kind::Camp_Menial: return "Camp_Menial";
        case mission_kind::Camp_Survey_Field: return "Camp_Survey_Field";
        case mission_kind::Camp_Survey_Expansion: return "Camp_Survey_Expansion";
        case mission_kind::Camp_Cut_Logs: return "Camp_Cut_Logs";
        case mission_kind::Camp_Clearcut: return "Camp_Clearcut";
        case mission_kind::Camp_Setup_Hide_Site: return "Camp_Setup_Hide_Site";
        case mission_kind::Camp_Relay_Hide_Site: return "Camp_Relay_Hide_Site";
        case mission_kind::Camp_Foraging: return "Camp_Foraging";
        case mission_kind::Camp_Trapping: return "Camp_Trapping";
        case mission_kind::Camp_Hunting: return "Camp_Hunting";
        case mission_kind::Camp_OM_Fortifications: return "Camp_OM_Fortifications";
        case mission_kind::Camp_Recruiting: return "Camp_Recruiting";
        case mission_kind::Camp_Scouting: return "Camp_Scouting";
        case mission_kind::Camp_Combat_Patrol: return "Camp_Combat_Patrol";
        case mission_kind::Camp_Chop_Shop: return "Camp_Chop_Shop";
        case mission_kind::Camp_Plow: return "Camp_Plow";
        case mission_kind::Camp_Plant: return "Camp_Plant";
        case mission_kind::Camp_Harvest: return "Camp_Harvest";
        // *INDENT-ON*
        case mission_kind::last_mission_kind:
            break;
    }
    cata_fatal( "Invalid event_type" );
}

} // namespace io

static const std::array < miss_data, Camp_Harvest + 1 > miss_info = { {
        {
            //  No_Mission
            "",
            no_translation( "" )
        },
        {
            "Scavenging_Patrol_Job",
            no_translation( "" )
        },
        {
            "Scavenging_Raid_Job",
            no_translation( "" )
        },
        {
            "Hospital_Raid_Job",
            no_translation( "" )
        },
        {
            "Menial_Job",
            no_translation( "" )
        },
        {
            "Carpentry_Job",
            no_translation( "" )
        },
        {
            "Forage_Job",
            no_translation( "" )
        },
        {
            "Caravan_Commune_Center_Job",
            no_translation( "" )
        },
        //  Faction camp missions
        {
            "Camp_Distribute_Food",
            no_translation( "" )
        },
        {
            "Camp_Determine_Leadership",
            no_translation( "" )
        },
        {
            "Hide_Mission",
            no_translation( "" )
        },
        {
            "Reveal_Mission",
            no_translation( "" )
        },
        {
            "Camp_Assign_Jobs",
            no_translation( "" )
        },
        {
            "Camp_Assign_Workers",
            no_translation( "" )
        },
        {
            "Camp_Abandon",
            no_translation( "" )
        },
        {
            "Camp_Upgrade ",  //  Want to add the blueprint after the space
            to_translation( "Working to expand your camp!\n" )
        },
        {
            "Camp_Emergency_Recall",
            to_translation( "Lost in the ether!\n" )
        },
        {
            "Camp_Crafting ",  //  Want to add the recipe after the space
            to_translation( "Busy crafting!\n" )
        },
        {
            "Camp_Gather_Materials",
            to_translation( "Searching for materials to upgrade the camp.\n" )
        },
        {
            "Camp_Collect_Firewood",
            to_translation( "Searching for firewood.\n" )
        },
        {
            "Camp_Menial",
            to_translation( "Performing menial labor…\n" )
        },
        {
            "Camp_Survey_Field",
            to_translation( "Surveying for suitable fields…\n" )
        },
        {
            "Camp_Survey_Expansion",
            to_translation( "Surveying for expansion…\n" )
        },
        {
            "Camp_Cut_Logs",
            to_translation( "Cutting logs in the woods…\n" )
        },
        {
            "Camp_Clearcut",
            to_translation( "Clearing a forest…\n" )
        },
        {
            "Camp_Setup_Hide_Site",
            to_translation( "Setting up a hide site…\n" )
        },
        {
            "Camp_Relay_Hide_Site",
            to_translation( "Transferring gear to a hide site…\n" )
        },
        {
            "Camp Foraging",
            to_translation( "Foraging for edible plants.\n" )
        },
        {
            "Camp_Trapping",
            to_translation( "Trapping Small Game.\n" )
        },
        {
            "Camp_Hunting",
            to_translation( "Hunting large animals.\n" )
        },
        {
            "Camp_OM_Fortifications",
            to_translation( "Constructing fortifications…\n" )
        },
        {
            "Camp_Recruiting",
            to_translation( "Searching for recruits.\n" )
        },
        {
            "Camp_Scouting",
            to_translation( "Scouting the region.\n" )
        },
        {
            "Camp_Combat Patrol",
            to_translation( "Patrolling the region.\n" )
        },
        {
            //  Obsolete entry
            "Camp_Chop_Shop",
            to_translation( "Working at the chop shop…\n" )
        },
        {
            "Camp_Plow",
            to_translation( "Working to plow your fields!\n" )
        },
        {
            "Camp_Plant",
            to_translation( "Working to plant your fields!\n" )
        },
        {
            "Camp_Harvest",
            to_translation( "Working to harvest your fields!\n" )
        }
    }
};

std::string action_of( mission_kind kind )
{
    return miss_info[kind].action.translated();
}

bool is_equal( const mission_id &first, const mission_id &second )
{
    return first.id == second.id &&
           first.parameters == second.parameters &&
           first.mapgen_args == second.mapgen_args &&
           first.dir == second.dir;
}

void reset_miss_id( mission_id &miss_id )
{
    miss_id = {};
}

void mission_id::serialize( JsonOut &jsout ) const
{
    jsout.start_object();
    jsout.member_as_string( "id", id );
    jsout.member( "parameters", parameters );
    jsout.member( "mapgen_args", mapgen_args );
    jsout.member( "dir", dir );
    jsout.end_object();
}

void mission_id::deserialize( const JsonValue &val )
{
    *this = { No_Mission, "", {}, std::nullopt };
    if( val.test_object() ) {
        JsonObject obj = val.get_object();
        obj.read( "id", id );
        obj.read( "parameters", parameters );
        obj.read( "mapgen_args", mapgen_args );
        obj.read( "dir", dir );
    } else if( val.test_string() ) {
        // Legacy values are stored as a string where the pieces need to be
        // parsed out.  Replaced during 0.G.
        std::string str = val.get_string();
        size_t id_size = str.length();

        if( id_size == 0 ) {
            return;
        }

        for( const auto &direction : base_camps::all_directions ) {
            if( string_ends_with( str, direction.second.id ) ) {
                dir = direction.first;
                id_size = str.length() - direction.second.id.length();
                break;
            }
        }

        std::string st = str.substr( 0, id_size );

        for( int i = No_Mission + 1; i <= Camp_Harvest; i++ ) {
            if( st.length() >= miss_info[i].serialize_id.length() &&
                st.substr( 0, miss_info[i].serialize_id.length() ) == miss_info[i].serialize_id ) {
                if( st.length() == miss_info[i].serialize_id.length() ) {
                    id = static_cast<mission_kind>( i );
                    return;
                } else {
                    id = static_cast<mission_kind>( i );
                    parameters = st.substr( miss_info[i].serialize_id.length() );
                    return;
                }
            }
        }

        // Even older legacy definition matching (replaced during 0.F)
        if( str == "_scavenging_patrol" ) {
            id = Scavenging_Patrol_Job;
        } else if( str == "_scavenging_raid" ) {
            id = Scavenging_Raid_Job;
        } else if( str == "_labor" ) {
            id = Menial_Job;
        } else if( str == "_carpenter" ) {
            id = Carpentry_Job;
        } else if( str == "_forage" ) {
            id = Forage_Job;
        } else if( str == "_commune_refugee_caravan" ) {
            id = Caravan_Commune_Center_Job;
        }
        //  The farm field actions do not result in npc missions

        else if( string_ends_with( str, camp_upgrade_npc_string ) ) {  //  blueprint + id
            id = Camp_Upgrade;
            parameters = str.substr( 0, str.length() - camp_upgrade_npc_string.length() );
            dir = base_camps::base_dir;
        }  //  Camp_Emergency_Recall is an immediate action, and so isn't serialized

        else if( st == "_faction_camp_crafting_" ) { //  id + dir
            id = Camp_Crafting;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_gathering" ) {
            id = Camp_Gather_Materials;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_firewood" ) {
            id = Camp_Collect_Firewood;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_menial" ) {
            id = Camp_Menial;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_field" ) {
            id = Camp_Survey_Field;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_expansion" ) {
            id = Camp_Survey_Expansion;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_cut_log" ) {
            id = Camp_Cut_Logs;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_clearcut" ) {
            id = Camp_Clearcut;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_hide_site" ) {
            id = Camp_Setup_Hide_Site;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_hide_trans" ) {
            id = Camp_Relay_Hide_Site;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_foraging" ) {
            id = Camp_Foraging;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_trapping" ) {
            id = Camp_Trapping;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_hunting" ) {
            id = Camp_Hunting;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_om_fortifications" ) {
            //  This legacy mission version hides the blueprint as a mission role rather than a string component
            id = Camp_OM_Fortifications;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_recruit_0" ) {
            id = Camp_Recruiting;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_scout_0" ) {
            id = Camp_Scouting;
            dir = base_camps::base_dir;
        } else if( str == "_faction_camp_combat_0" ) {
            id = Camp_Combat_Patrol;
            dir = base_camps::base_dir;
        } else if( id_size >= camp_upgrade_expansion_npc_string.length() &&
                   st.substr( id_size - camp_upgrade_expansion_npc_string.length() ) ==
                   camp_upgrade_expansion_npc_string ) { // blueprint + id + dir
            id = Camp_Upgrade;
            parameters = st.substr( 0, id_size - camp_upgrade_expansion_npc_string.length() );
        } else if( st == "_faction_exp_chop_shop_" ) {        // id + dir
            id = Camp_Chop_Shop;
        } else if( st == "_faction_exp_kitchen_cooking_" ||   // id + dir
                   st == "_faction_exp_blacksmith_crafting_" ||
                   st == "_faction_exp_farm_crafting_" ) {
            id = Camp_Crafting;
        } else if( st == "_faction_exp_plow_" ) {             // id + dir
            id = Camp_Plow;
        } else if( st == "_faction_exp_plant_" ) {            // id + dir
            id = Camp_Plant;
        } else if( st == "_faction_exp_harvest_" ) {          // id + dir
            id = Camp_Harvest;
        }

        if( id == No_Mission ) {
            debugmsg(
                "Unrecognized npc mission id string encountered: '%s'", str );
        }
    } else {
        debugmsg( "Mission id in JSON should be either a string or object, but was neither" );
    }
}

bool is_equal( const ui_mission_id &first, const ui_mission_id &second )
{
    return first.ret == second.ret &&
           is_equal( first.id, second.id );
}

struct comp_rank {
    int industry;
    int combat;
    int survival;
};

mission_data::mission_data()
{
    for( int tab_num = base_camps::TAB_MAIN; tab_num != base_camps::TAB_NW + 3; tab_num++ ) {
        std::vector<mission_entry> k;
        entries.push_back( k );
    }
}

namespace talk_function
{
static void scavenger_patrol( mission_data &mission_key, npc &p );
static void scavenger_raid( mission_data &mission_key, npc &p );
static void hospital_raid( mission_data &mission_key, npc &p );
static void commune_menial( mission_data &mission_key, npc &p );
static void commune_carpentry( mission_data &mission_key, npc &p );
static void commune_forage( mission_data &mission_key, npc &p );
static void commune_refuge_caravan( mission_data &mission_key, npc &p );
static bool handle_outpost_mission( const mission_entry &cur_key, npc &p );
} // namespace talk_function

void talk_function::companion_mission( npc &p )
{
    mission_data mission_key;

    std::string role_id = p.companion_mission_role_id;
    const tripoint_abs_omt omt_pos = p.global_omt_location();
    std::string title = _( "Outpost Missions" );
    if( role_id == "SCAVENGER" ) {
        title = _( "Junk Shop Missions" );
        scavenger_patrol( mission_key, p );
        if( p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
            scavenger_raid( mission_key, p );
        }

        Character &player_character = get_player_character();
        if( player_character.get_value( var_SCAVENGER_HOSPITAL_RAID ) == "yes" ) {
            hospital_raid( mission_key, p );
        }
    } else if( role_id == "COMMUNE CROPS" ) {
        title = _( "Agricultural Missions" );
        commune_forage( mission_key, p );
        commune_refuge_caravan( mission_key, p );
    } else if( role_id == "FOREMAN" ) {
        title = _( "Construction Missions" );
        commune_menial( mission_key, p );
        if( p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
            commune_carpentry( mission_key, p );
        }
    } else if( role_id == "REFUGEE MERCHANT" ) {
        title = _( "Free Merchant Missions" );
        commune_refuge_caravan( mission_key, p );
    } else if( role_id == "PLANT FIELD" ) {
        field_plant( p, omt_ranch_camp_63 );
        return;
    } else if( role_id == "HARVEST FIELD" ) {
        field_harvest( p, omt_ranch_camp_63 );
        return;
    } else {
        return;
    }
    if( display_and_choose_opts( mission_key, omt_pos, role_id, title ) ) {
        handle_outpost_mission( mission_key.cur_key, p );
    }
}

void talk_function::scavenger_patrol( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: 20-75 merch\nDanger: Low\nTime: 10 hour missions\n\n"
                           "Assigning one of your allies to patrol the surrounding wilderness "
                           "and isolated buildings presents the opportunity to build survival "
                           "skills while engaging in relatively safe combat against isolated "
                           "creatures." );
    const mission_id miss_id = { Scavenging_Patrol_Job, "", {}, std::nullopt };
    mission_key.add_start( miss_id, _( "Assign Scavenging Patrol" ), entry );
    std::vector<npc_ptr> npc_list = companion_list( p, miss_id );
    if( !npc_list.empty() ) {
        entry = _( "Profit: 20-75 merch\nDanger: Low\nTime: 10 hour missions\n\nPatrol Roster:\n" );
        bool avail = false;

        for( auto &elem : npc_list ) {
            const bool done = calendar::turn >= elem->companion_mission_time + 10_hours;
            avail |= done;
            if( done ) {
                entry += "  " + elem->get_name() + _( " [DONE]\n" );
            } else {
                entry += "  " + elem->get_name() + " [" + std::to_string( to_hours<int>( calendar::turn -
                         elem->companion_mission_time ) ) + _( " hours / 10 hours]\n" );
            }
        }
        if( avail ) {
            entry += _( return_ally_question_string );
        }
        mission_key.add_return( miss_id, _( "Retrieve Scavenging Patrol" ), entry, avail );
    }
}

void talk_function::scavenger_raid( mission_data &mission_key, npc &p )
{
    std::string entry =
        _( "Profit: 50-100 merch, some items\nDanger: Medium\nTime: 10 hour missions\n\n"
           "Scavenging raids target formerly populated areas to loot as many "
           "valuable items as possible before being surrounded by the undead.  "
           "Combat is to be expected and assistance from the rest of the party "
           "can't be guaranteed.  The rewards are greater and there is a chance "
           "of the companion bringing back items." );
    const mission_id miss_id = {Scavenging_Raid_Job, "", {}, std::nullopt};
    mission_key.add_start( miss_id, _( "Assign Scavenging Raid" ), entry );
    std::vector<npc_ptr> npc_list = companion_list( p, miss_id );
    if( !npc_list.empty() ) {
        entry = _( "Profit: 50-100 merch\nDanger: Medium\nTime: 10 hour missions\n\n"
                   "Raid Roster:\n" );
        bool avail = false;

        for( auto &elem : npc_list ) {
            const bool done = calendar::turn >= elem->companion_mission_time + 10_hours;
            avail |= done;
            if( done ) {
                entry += "  " + elem->get_name() + _( " [DONE]\n" );
            } else {
                entry += "  " + elem->get_name() + " [" + std::to_string( to_hours<int>( calendar::turn -
                         elem->companion_mission_time ) ) + _( " hours / 10 hours]\n" );
            }
        }
        if( avail ) {
            entry += _( return_ally_question_string );
        }
        mission_key.add_return( miss_id, _( "Retrieve Scavenging Raid" ), entry, avail );
    }
}

void talk_function::hospital_raid( mission_data &mission_key, npc &p )
{
    const mission_id miss_id = {Hospital_Raid_Job, "", {}, std::nullopt};
    if( get_player_character().get_value( var_SCAVENGER_HOSPITAL_RAID_STARTED ) != "yes" ) {
        const std::string entry_assign =
            _( "Profit: hospital equipment, some items\nDanger: High\nTime: 20 hour mission\n\n"
               "Scavenging raid targeting a hospital to search for hospital equipment and as many "
               "valuable items as possible before being surrounded by the undead.  "
               "Combat is to be expected and assistance from the rest of the party "
               "can't be guaranteed.  This will be an extremely dangerous mission, "
               "so make sure everyone is prepared before they go." );
        mission_key.add_start( miss_id, _( "Assign Hospital Raid" ), entry_assign );
    }
    std::vector<npc_ptr> npc_list = companion_list( p, miss_id );
    if( !npc_list.empty() ) {
        std::string entry_return = _( "Profit: hospital materials\nDanger: High\nTime: 20 hour missions\n\n"
                                      "Raid Roster:\n" );
        bool avail = false;

        for( auto &elem : npc_list ) {
            const bool done = calendar::turn >= elem->companion_mission_time + 20_hours;
            avail |= done;
            if( done ) {
                entry_return += "  " + elem->get_name() + _( " [DONE]\n" );
            } else {
                entry_return += "  " + elem->get_name() + " [" + std::to_string( to_hours<int>( calendar::turn -
                                elem->companion_mission_time ) ) + _( " hours / 20 hours]\n" );
            }
        }
        if( avail ) {
            entry_return += _( return_ally_question_string );
        }
        mission_key.add_return( miss_id, _( "Retrieve Hospital Raid" ), entry_return, avail );
    }
}

void talk_function::commune_menial( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: 3 merch/hour\nDanger: None\nTime: 1 hour minimum\n\n"
                           "Assigning one of your allies to menial labor is a safe way to teach "
                           "them basic skills and build reputation with the outpost.  Don't expect "
                           "much of a reward though." );
    const mission_id miss_id = {Menial_Job, "", {}, std::nullopt};
    mission_key.add_start( miss_id, _( "Assign Ally to Menial Labor" ), entry );
    std::vector<npc_ptr> npc_list = companion_list( p, miss_id );
    if( !npc_list.empty() ) {
        std::string entry =
            _( "Profit: 3 merch/hour\nDanger: Minimal\nTime: 1 hour minimum\n\nLabor Roster:\n" );
        bool avail = false;

        for( auto &elem : npc_list ) {
            avail |= calendar::turn >= elem->companion_mission_time + 1_hours;
            entry += "  " + elem->get_name() + " [" + std::to_string( to_hours<int>( calendar::turn -
                     elem->companion_mission_time ) ) + _( " hours / 1 hour]\n" );
        }
        if( avail ) {
            entry += _( return_ally_question_string );
        }
        mission_key.add_return( miss_id, _( "Recover Ally from Menial Labor" ),
                                entry, avail );
    }
}

void talk_function::commune_carpentry( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: 5 merch/hour\nDanger: Minimal\nTime: 1 hour minimum\n\n"
                           "Carpentry work requires more skill than menial labor while offering "
                           "modestly improved pay.  It is unlikely that your companions will face "
                           "combat, but there are hazards working on makeshift buildings." );
    const mission_id miss_id = {Carpentry_Job, "", {}, std::nullopt};
    mission_key.add_start( miss_id, _( "Assign Ally to Carpentry Work" ), entry );
    std::vector<npc_ptr>  npc_list = companion_list( p, miss_id );
    if( !npc_list.empty() ) {
        entry = _( "Profit: 5 merch/hour\nDanger: Minimal\nTime: 1 hour minimum\n\nLabor Roster:\n" );
        bool avail = false;

        for( auto &elem : npc_list ) {
            avail |= calendar::turn >= elem->companion_mission_time + 1_hours;
            entry += "  " + elem->get_name() + " [" + std::to_string( to_hours<int>( calendar::turn -
                     elem->companion_mission_time ) ) + _( " hours / 1 hour]\n" );
        }
        if( avail ) {
            entry += _( return_ally_question_string );
        }
        mission_key.add_return( miss_id,
                                _( "Recover Ally from Carpentry Work" ), entry, avail );
    }
}

void talk_function::commune_forage( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: 4 merch/hour\nDanger: Low\nTime: 4 hour minimum\n\n"
                           "Foraging for food involves dispatching a companion to search the "
                           "surrounding wilderness for wild edibles.  Combat will be avoided, but "
                           "encounters with wild animals are to be expected.  The low pay is "
                           "supplemented with the odd item as a reward for particularly large "
                           "hauls." );
    const mission_id miss_id = {Forage_Job, "", {}, std::nullopt};
    mission_key.add_start( miss_id, _( "Assign Ally to Forage for Food" ),
                           entry );
    std::vector<npc_ptr> npc_list = companion_list( p, miss_id );
    if( !npc_list.empty() ) {
        bool avail = false;
        entry = _( "Profit: 4 merch/hour\nDanger: Low\nTime: 4 hour minimum\n\nLabor Roster:\n" );
        for( auto &elem : npc_list ) {
            avail |= calendar::turn >= elem->companion_mission_time + 4_hours;
            entry += "  " + elem->get_name() + " [" + std::to_string( to_hours<int>( calendar::turn -
                     elem->companion_mission_time ) ) + _( " hours / 4 hours]\n" );
        }
        if( avail ) {
            entry += _( return_ally_question_string );
        }
        mission_key.add_return( miss_id, _( "Recover Ally from Foraging" ), entry, avail );
    }
}

void talk_function::commune_refuge_caravan( mission_data &mission_key, npc &p )
{
    std::string entry = _( "Profit: 7 merch/hour\nDanger: High\nTime: UNKNOWN\n\n"
                           "Adding companions to the caravan team increases the likelihood of "
                           "success.  By nature, caravans are extremely tempting targets for "
                           "raiders or hostile groups, so only a strong party is recommended.  The "
                           "rewards are significant for those participating, but are even more "
                           "important for the factions that profit.\n\n"
                           "The commune is sending food to the Free Merchants in the Refugee "
                           "Center as part of a tax and in exchange for skilled labor." );
    mission_id miss_id = {
        Caravan_Commune_Center_Job, caravan_commune_center_job_assign_parameter, {}, std::nullopt
    };
    mission_key.add_start( miss_id, _( "Caravan Commune-Refugee Center" ),
                           entry );

    std::vector<npc_ptr> npc_list = companion_list( p, miss_id );
    std::string return_entry = _( "Profit: 7 merch/hour\nDanger: High\nTime: UNKNOWN\n\n"
                                  "\nRoster:\n" );
    bool display_return = false;
    bool ready_return = false;

    if( !npc_list.empty() ) {
        entry = return_entry;
        display_return = true;
        ready_return = true;

        for( auto &elem : npc_list ) {
            const std::string line = "  " + elem->get_name() + _( " [READY TO DEPART]\n" );
            entry += line;
            return_entry += line;
        }
        entry += _( "\n\n"
                    "The caravan will contain two or three additional members "
                    "from the commune, are you ready to depart?" );
        miss_id.parameters = caravan_commune_center_job_active_parameter;
        mission_key.add_start( miss_id,
                               _( "Begin Commune-Refugee Center Run" ), entry );
    }

    miss_id.parameters = caravan_commune_center_job_active_parameter;
    npc_list = companion_list( p, miss_id );

    if( !npc_list.empty() ) {
        display_return = true;

        for( auto &elem : npc_list ) {
            if( calendar::turn >= elem->companion_mission_time ) {
                ready_return = true;
                return_entry += "  " + elem->get_name() + _( " [COMPLETE, READY TO BE RETURNED]\n" );
            } else {
                return_entry += "  " + elem->get_name() + " [" + std::to_string( std::abs( to_hours<int>
                                ( calendar::turn - elem->companion_mission_time ) ) ) + _( " Hours]\n" );
            }
        }
    }

    // Legacy compatibility. Changed during 0.F.
    miss_id.parameters.clear();
    npc_list = companion_list( p, miss_id );

    if( !npc_list.empty() ) {
        display_return = true;

        for( auto &elem : npc_list ) {
            if( calendar::turn >= elem->companion_mission_time ) {
                ready_return = true;
                return_entry += "  " + elem->get_name() + _( " [COMPLETE, READY TO BE RETURNED]\n" );
            } else {
                return_entry += "  " + elem->get_name() + " [" + std::to_string( std::abs( to_hours<int>
                                ( calendar::turn - elem->companion_mission_time ) ) ) + _( " Hours]\n" );
            }
        }
    }

    if( display_return ) {
        miss_id.parameters = caravan_commune_center_job_assign_parameter;
        mission_key.add_return( miss_id, _( "Recover Commune-Refugee Center" ),
                                return_entry, ready_return );
    }
}

bool talk_function::display_and_choose_opts(
    mission_data &mission_key, const tripoint_abs_omt &omt_pos, const std::string &role_id,
    const std::string &title )
{
    if( mission_key.entries.empty() ) {
        popup( _( "There are no missions at this colony.  Press Spacebar…" ) );
        return false;
    }

    int TITLE_TAB_HEIGHT = 0;
    if( role_id == role_id_faction_camp ) {
        TITLE_TAB_HEIGHT = 1;
    }

    base_camps::tab_mode tab_mode = base_camps::TAB_MAIN;

    size_t sel = 0;

    // The following are for managing the right pane scrollbar.
    size_t info_offset = 0;
    nc_color col = c_white;
    std::vector<std::string> name_text;
    std::vector<std::string> mission_text;

    input_context ctxt( "FACTIONS" );
    ctxt.register_action( "UP", to_translation( "Move cursor up" ) );
    ctxt.register_action( "DOWN", to_translation( "Move cursor down" ) );
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action( "PAGE_UP", to_translation( "Fast scroll up" ) );
    ctxt.register_action( "PAGE_DOWN", to_translation( "Fast scroll down" ) );
    ctxt.register_action( "SCROLL_MISSION_INFO_UP" );
    ctxt.register_action( "SCROLL_MISSION_INFO_DOWN" );
    ctxt.register_action( "CONFIRM" );
    ctxt.register_action( "QUIT" );
    ctxt.register_action( "HELP_KEYBINDINGS" );
    if( get_player_character().has_trait( trait_DEBUG_HS_LIGHT ) ) {
        ctxt.register_action( "DEBUG", to_translation( "Debug creation of items", "Use hammerspace" ) );
    }
    std::vector<mission_entry> cur_key_list;

    auto reset_cur_key_list = [&]() {
        cur_key_list = mission_key.entries[0];
        for( const mission_entry &k : mission_key.entries[1] ) {
            bool has = false;
            for( const mission_entry &keys : cur_key_list ) {
                if( is_equal( k.id, keys.id ) ) {
                    has = true;
                    break;
                }
            }
            if( !has ) {
                cur_key_list.push_back( k );
            }
        }
    };

    reset_cur_key_list();

    if( cur_key_list.empty() ) {
        popup( _( "There are no missions at this colony.  Press Spacebar…" ) );
        return false;
    }

    size_t part_y = 0;
    size_t part_x = 0;
    size_t maxy = 0;
    size_t maxx = 0;
    size_t info_height = 0;
    size_t info_width = 0;

    catacurses::window w_list;
    catacurses::window w_tabs;
    catacurses::window w_info;

    ui_adaptor ui;
    ui.on_screen_resize( [&]( ui_adaptor & ui ) {
        part_y = TERMY > FULL_SCREEN_HEIGHT ? ( TERMY - FULL_SCREEN_HEIGHT ) / 4 : 0;
        part_x = TERMX > FULL_SCREEN_WIDTH ? ( TERMX - FULL_SCREEN_WIDTH ) / 4 : 0;
        maxy = part_y ? TERMY - 2 * part_y : FULL_SCREEN_HEIGHT;
        maxx = part_x ? TERMX - 2 * part_x : FULL_SCREEN_WIDTH;
        info_height = maxy - 3;
        info_width = maxx - 1 - MAX_FAC_NAME_SIZE;

        w_list = catacurses::newwin( maxy, maxx,
                                     point( part_x, part_y + TITLE_TAB_HEIGHT ) );
        w_info = catacurses::newwin( info_height, info_width,
                                     point( part_x + MAX_FAC_NAME_SIZE, part_y + TITLE_TAB_HEIGHT + 1 ) );

        if( role_id == role_id_faction_camp ) {
            w_tabs = catacurses::newwin( TITLE_TAB_HEIGHT, maxx, point( part_x, part_y ) );
            ui.position( point( part_x, part_y ), point( maxx, maxy + TITLE_TAB_HEIGHT ) );
        } else {
            ui.position( point( part_x, part_y + TITLE_TAB_HEIGHT ), point( maxx, maxy ) );
        }
    } );
    ui.mark_resize();

    ui.on_redraw( [&]( const ui_adaptor & ) {
        werase( w_list );
        draw_border( w_list );
        // NOLINTNEXTLINE(cata-use-named-point-constants)
        mvwprintz( w_list, point( 1, 1 ), c_white, name_mission_tabs( omt_pos, role_id, title,
                   tab_mode ) );

        std::vector<std::vector<std::string>> folded_names;
        size_t folded_names_lines = 0;
        for( const mission_entry &cur_key_entry : cur_key_list ) {
            std::vector<std::string> f_name = foldstring( cur_key_entry.name_display, MAX_FAC_NAME_SIZE - 5,
                                              ' ' );
            folded_names_lines += f_name.size();
            folded_names.emplace_back( f_name );
        }

        int name_offset = 0;
        size_t sel_pos = 0;
        // Translate from entry index to line index
        for( size_t i = 0; i < sel; i++ ) {
            sel_pos += folded_names[i].size();
        }

        calcStartPos( name_offset, sel_pos, info_height, folded_names_lines );

        int name_index = 0;
        // Are we so far down the list that we bump into the end?
        bool last_section = folded_names_lines < info_height ||
                            folded_names_lines - info_height <= static_cast<size_t>( name_offset );

        // Translate back from desired line index to the corresponding entry, making sure to round up
        // near the end to ensure the last line gets included.
        if( name_offset > 0 ) {
            for( size_t i = 0; i < cur_key_list.size(); i++ ) {
                name_offset -= folded_names[i].size();
                if( name_offset <= 0 ) {
                    if( name_offset == 0 || last_section ) {
                        name_index++;
                    }
                    break;
                }
                name_index++;
            }
        }

        size_t list_line = 2;
        for( size_t current = name_index; list_line < info_height + 2 &&
             current < cur_key_list.size(); current++ ) {
            nc_color col = current == sel ? h_white : c_white;
            //highlight important missions
            for( const mission_entry &k : mission_key.entries[0] ) {
                if( is_equal( cur_key_list[current].id, k.id ) ) {
                    col = ( current == sel ? h_white : c_yellow );
                    break;
                }
            }
            //dull uncraftable items
            for( const mission_entry &k : mission_key.entries[10] ) {
                if( is_equal( cur_key_list[current].id, k.id ) ) {
                    col = ( current == sel ? h_white : c_dark_gray );
                    break;
                }
            }
            std::vector<std::string> &name_text = folded_names[current];
            if( list_line + name_text.size() > info_height + 2 ) {
                break;  //  Skip last entry that would spill over into the bar.
            }
            for( size_t name_line = 0; name_line < name_text.size(); name_line++ ) {
                print_colored_text( w_list, point( name_line ? 5 : 1, list_line ),
                                    col, col, name_text[name_line] );
                list_line += 1;
            }
        }

        if( folded_names_lines > info_height ) {
            scrollbar()
            .offset_x( 0 )
            .offset_y( 1 )
            .content_size( folded_names_lines )
            .viewport_pos( sel_pos )
            .viewport_size( info_height )
            .apply( w_list );
        }
        wnoutrefresh( w_list );
        werase( w_info );

        // Fold mission text, store it for scrolling
        mission_text = foldstring( mission_key.cur_key.text, info_width - 2, ' ' );
        if( info_height >= mission_text.size() ) {
            info_offset = 0;
        } else if( info_offset + info_height > mission_text.size() ) {
            info_offset = mission_text.size() - info_height;
        }
        if( mission_text.size() > info_height ) {
            scrollbar()
            .offset_x( info_width - 1 )
            .offset_y( 0 )
            .content_size( mission_text.size() )
            .viewport_pos( info_offset )
            .viewport_size( info_height )
            .apply( w_info );
        }
        const size_t end_line = std::min( info_height, mission_text.size() - info_offset );

        // Display the current subset of the mission text.
        for( size_t start_line = 0; start_line < end_line; start_line++ ) {
            print_colored_text( w_info, point( 0, start_line ), col, col,
                                mission_text[start_line + info_offset] );
        }

        wnoutrefresh( w_info );

        if( role_id == role_id_faction_camp ) {
            werase( w_tabs );
            draw_camp_tabs( w_tabs, tab_mode, mission_key.entries );
            wnoutrefresh( w_tabs );
        }
    } );

    while( true ) {
        mission_key.cur_key = cur_key_list[sel];
        ui_manager::redraw();
        const std::string action = ctxt.handle_input();
        const int recmax = static_cast<int>( cur_key_list.size() );
        const int scroll_rate = recmax > 20 ? 10 : 3;
        if( action == "UP" || action == "SCROLL_UP" || action == "DOWN" || action == "SCROLL_DOWN" ) {
            sel = inc_clamp_wrap( sel, action == "DOWN" || action == "SCROLL_DOWN", cur_key_list.size() );
            info_offset = 0;
        } else if( action == "SCROLL_MISSION_INFO_UP" ) {
            if( info_offset > 0 ) {
                info_offset--;
            }
        } else if( action == "SCROLL_MISSION_INFO_DOWN" ) {
            info_offset++;
        } else if( action == "PAGE_UP" || action == "PAGE_DOWN" ) {
            sel = inc_clamp_wrap( sel, action == "PAGE_UP" ? -scroll_rate : scroll_rate, cur_key_list.size() );
            info_offset = 0;

        } else if( action == "NEXT_TAB" && role_id == role_id_faction_camp ) {
            sel = 0;
            info_offset = 0;

            do {
                if( tab_mode == base_camps::TAB_NW ) {
                    tab_mode = base_camps::TAB_MAIN;
                    reset_cur_key_list();
                } else {
                    tab_mode = static_cast<base_camps::tab_mode>( tab_mode + 1 );
                    cur_key_list = mission_key.entries[tab_mode + 1];
                }
            } while( cur_key_list.empty() );
        } else if( action == "PREV_TAB" && role_id == role_id_faction_camp ) {
            sel = 0;
            info_offset = 0;

            do {
                if( tab_mode == base_camps::TAB_MAIN ) {
                    tab_mode = base_camps::TAB_NW;
                } else {
                    tab_mode = static_cast<base_camps::tab_mode>( tab_mode - 1 );
                }

                if( tab_mode == base_camps::TAB_MAIN ) {
                    reset_cur_key_list();
                } else {
                    cur_key_list = mission_key.entries[size_t( tab_mode ) + 1];
                }
            } while( cur_key_list.empty() );
        } else if( action == "QUIT" ) {
            mission_entry dud;
            dud.id = { {No_Mission, "", {}, std::nullopt}, false};
            dud.name_display = "NONE";
            mission_key.cur_key = dud;
            break;
        } else if( action == "DEBUG" ) {
            if( mission_key.cur_key.id.id.id != Camp_Upgrade &&
                mission_key.cur_key.id.id.id != Camp_Crafting ) {
                continue;
            }
            debug_menu::set_random_seed( _( "Enter random seed for mission materials" ) );
            add_msg( m_good, _( "Spawning materials for mission: %s" ),
                     mission_key.cur_key.name_display );
            recipe making;
            int batch_size = 1;
            if( mission_key.cur_key.id.id.id == Camp_Upgrade ) {
                making = recipe_id( mission_key.cur_key.id.id.parameters ).obj();
            } else if( mission_key.cur_key.id.id.id == Camp_Crafting ) {
                making = recipe_id( mission_key.cur_key.id.id.parameters ).obj();
                string_input_popup popup;
                popup.title( string_format( _( "Enter desired batch size for crafting recipe: %s" ),
                                            mission_key.cur_key.name_display ) )
                .only_digits( true );
                batch_size = popup.query_int();
                if( batch_size < 1 ) {
                    continue;
                }
            } else {
                continue;
            }
            std::vector<std::pair<itype_id, int>> items_to_spawn = debug_menu::get_items_for_requirements(
                                                   making.simple_requirements(), batch_size,
                                                   mission_key.cur_key.name_display );
            debug_menu::spawn_item_collection( items_to_spawn, false );
            // If possible, go back to the basecamp and refresh mission entries based on the new inventory
            mission_data updated_missions;
            point_abs_omt omt( omt_pos.xy() );
            std::optional<basecamp *> bcp = overmap_buffer.find_camp( omt );
            if( bcp.has_value() ) {
                bcp.value()->form_crafting_inventory();;
                map &here = bcp.value()->get_camp_map();
                bcp.value()->get_available_missions( updated_missions, here );
                mission_key = updated_missions;
                if( tab_mode == base_camps::TAB_MAIN ) {
                    reset_cur_key_list();
                } else {
                    cur_key_list = mission_key.entries[size_t( tab_mode ) + 1];
                }
            }
        } else if( action == "CONFIRM" ) {
            if( mission_key.cur_key.possible ) {
                break;
            } else {
                continue;
            }
        }
    }
    return true;
}

bool talk_function::handle_outpost_mission( const mission_entry &cur_key, npc &p )
{
    switch( cur_key.id.id.id ) {
        case Scavenging_Patrol_Job:
            if( cur_key.id.ret ) {
                scavenging_patrol_return( p );
            } else {
                individual_mission( p, _( "departs on the scavenging patrol…" ), cur_key.id.id );
            }
            break;

        case Scavenging_Raid_Job:
            if( cur_key.id.ret ) {
                scavenging_raid_return( p );
            } else {
                individual_mission( p, _( "departs on the scavenging raid…" ), cur_key.id.id );
            }
            break;

        case Hospital_Raid_Job:
            if( cur_key.id.ret ) {
                hospital_raid_return( p );
            } else {
                npc_ptr npc = individual_mission( p, _( "departs on the hospital raid…" ), cur_key.id.id );
                if( npc != nullptr ) {
                    get_player_character().set_value( var_SCAVENGER_HOSPITAL_RAID_STARTED, "yes" );
                }
            }
            break;

        case Menial_Job:
            if( cur_key.id.ret ) {
                labor_return( p );
            } else {
                individual_mission( p, _( "departs to work as a laborer…" ), cur_key.id.id );
            }
            break;

        case Carpentry_Job:
            if( cur_key.id.ret ) {
                carpenter_return( p );
            } else {
                individual_mission( p, _( "departs to work as a carpenter…" ), cur_key.id.id );
            }
            break;

        case Forage_Job:
            if( cur_key.id.ret ) {
                forage_return( p );
            } else {
                individual_mission( p, _( "departs to forage for food…" ), cur_key.id.id );
            }
            break;

        case Caravan_Commune_Center_Job:
            if( cur_key.id.ret ) {
                caravan_return( p, omt_evac_center_18, cur_key.id.id );
            } else if( cur_key.id.id.parameters == caravan_commune_center_job_assign_parameter ) {
                individual_mission( p, _( "joins the caravan team…" ), cur_key.id.id, true );
            } else if( cur_key.id.id.parameters == caravan_commune_center_job_active_parameter ) {
                caravan_depart( p, omt_evac_center_18, cur_key.id.id );
            } else {
                debugmsg( "Unrecognized caravan mission id parameter encountered: '%s'", cur_key.id.id.parameters );
            }
            break;

        case No_Mission:
            return false;

        case Camp_Distribute_Food:
        case Camp_Determine_Leadership:
        case Camp_Hide_Mission:
        case Camp_Reveal_Mission:
        case Camp_Assign_Jobs:
        case Camp_Assign_Workers:
        case Camp_Abandon:
        case Camp_Upgrade:
        case Camp_Emergency_Recall:
        case Camp_Crafting:
        case Camp_Gather_Materials:
        case Camp_Collect_Firewood:
        case Camp_Menial:
        case Camp_Survey_Field:
        case Camp_Survey_Expansion:
        case Camp_Cut_Logs:
        case Camp_Clearcut:
        case Camp_Setup_Hide_Site:
        case Camp_Relay_Hide_Site:
        case Camp_Foraging:
        case Camp_Trapping:
        case Camp_Hunting:
        case Camp_OM_Fortifications:
        case Camp_Recruiting:
        case Camp_Scouting:
        case Camp_Combat_Patrol:
        case Camp_Chop_Shop:
        case Camp_Plow:
        case Camp_Plant:
        case Camp_Harvest:
            debugmsg( "Non outpost mission encountered in outpost mission handling: '%s'",
                      miss_info [cur_key.id.id.id].serialize_id );
            return false;

        default:
            debugmsg( "Unrecognized mission id encountered in outpost mission handling: '%d'",
                      cur_key.id.id.id );
            return false;

    }

    return true;
}

npc_ptr talk_function::individual_mission( npc &p, const std::string &desc,
        const mission_id &miss_id, bool group, const std::vector<item *> &equipment,
        const std::map<skill_id, int> &required_skills, bool silent_failure )
{
    const tripoint_abs_omt omt_pos = p.global_omt_location();
    return individual_mission( omt_pos, p.companion_mission_role_id, desc, miss_id, group,
                               equipment, required_skills, silent_failure );
}
npc_ptr talk_function::individual_mission( const tripoint_abs_omt &omt_pos,
        const std::string &role_id, const std::string &desc,
        const mission_id &miss_id, bool group, const std::vector<item *> &equipment,
        const std::map<skill_id, int> &required_skills, bool silent_failure )
{
    npc_ptr comp = companion_choose( required_skills, silent_failure );
    if( comp == nullptr ) {
        return comp;
    }
    // make sure, for now, that NPCs dismount their horse before going on a mission.
    if( comp->has_effect( effect_riding ) ) {
        comp->npc_dismount();
    }

    //Ensure we have someone to give equipment to before we lose it
    for( item *i : equipment ) {
        comp->companion_mission_inv.add_item( *i );
        if( item::count_by_charges( i->typeId() ) ) {
            comp->as_character()->use_charges( i->typeId(), i->charges );
        } else {
            comp->as_character()->use_amount( i->typeId(), 1 );
        }
    }
    if( comp->in_vehicle ) {
        get_map().unboard_vehicle( comp->pos() );
    }
    popup( "%s %s", comp->get_name(), desc );
    comp->set_companion_mission( omt_pos, role_id, miss_id );
    if( group ) {
        comp->companion_mission_time = calendar::before_time_starts;
    } else {
        comp->companion_mission_time = calendar::turn;
    }
    g->reload_npcs();
    g->validate_npc_followers();
    cata_assert( !comp->is_active() );
    return comp;
}

void talk_function::caravan_depart( npc &p, const std::string &dest, const mission_id &id )
{
    mission_id miss_id = id;
    miss_id.parameters = caravan_commune_center_job_assign_parameter;
    std::vector<npc_ptr> npc_list = companion_list( p, miss_id );
    int distance = caravan_dist( dest );
    time_duration time = 20_minutes + distance * 10_minutes;
    const int hours = to_hours<int>( time );
    popup( n_gettext( "The caravan departs with an estimated total travel time of %d hour…",
                      "The caravan departs with an estimated total travel time of %d hours…", hours ), hours );

    const double uncertainty_multiplier = rng_float( 0.9, 1.1 );
    //  The caravan's duration has an uncertainty, but all members are part of the same caravan...

    for( auto &elem : npc_list ) {
        if( elem->companion_mission_time == calendar::before_time_starts ) {
            //Adds a 10% error in estimated travel time
            elem->set_companion_mission( p, id );
            elem->companion_mission_time = calendar::turn + time * uncertainty_multiplier;
        }
    }
}

//Could be expanded to actually path to the site, just returns the distance
int talk_function::caravan_dist( const std::string &dest )
{
    Character &player_character = get_player_character();
    const tripoint_abs_omt site =
        overmap_buffer.find_closest( player_character.global_omt_location(), dest, 0, false );
    int distance = rl_dist( player_character.global_omt_location(), site );
    return distance;
}

void talk_function::caravan_return( npc &p, const std::string &dest, const mission_id &id )
{
    npc_ptr comp = companion_choose_return( p, id, calendar::turn,
                                            true );  //  ignore_parameters = true due to backwards compatibility, changed during 0.F
    if( comp == nullptr ) {
        return;
    }
    if( comp->companion_mission_time == calendar::before_time_starts ) {
        popup( _( "%s returns to your party." ), comp->get_name() );
        companion_return( *comp );
        return;
    }
    //So we have chosen to return an individual or party who went on the mission
    //Everyone who was on the mission will have the same companion_mission_time
    //and will simulate the mission and return together
    std::vector<npc_ptr> caravan_party;
    std::vector<npc_ptr> bandit_party;
    std::vector<npc_ptr> npc_list = companion_list( p, id,
                                    true );  //  contains = true due to backwards compatibility, changed during 0.F
    const int rand_caravan_size = rng( 1, 3 );
    caravan_party.reserve( npc_list.size() + rand_caravan_size );
    for( int i = 0; i < rand_caravan_size; i++ ) {
        caravan_party.push_back( temp_npc( npc_template_commune_guard ) );
    }
    for( auto &elem : npc_list ) {
        if( elem->companion_mission_time == comp->companion_mission_time ) {
            caravan_party.push_back( elem );
        }
    }

    int distance = caravan_dist( dest );
    int time = 200 + distance * 100;
    int experience = rng( 10, time / 300 );

    const size_t rand_bandit_size = rng( 1, 3 );
    bandit_party.reserve( rand_bandit_size * 2 );
    for( size_t i = 0; i < rand_bandit_size * 2; i++ ) {
        bandit_party.push_back( temp_npc( npc_template_bandit ) );
        bandit_party.push_back( temp_npc( npc_template_thug ) );
    }

    if( one_in( 3 ) ) {
        if( one_in( 2 ) ) {
            popup( _( "A bandit party approaches the caravan in the open!" ) );
            force_on_force( caravan_party, "caravan", bandit_party, "band", 1 );
        } else if( one_in( 3 ) ) {
            popup( _( "A bandit party attacks the caravan while it it's camped!" ) );
            force_on_force( caravan_party, "caravan", bandit_party, "band", 2 );
        } else {
            popup( _( "The caravan walks into a bandit ambush!" ) );
            force_on_force( caravan_party, "caravan", bandit_party, "band", -1 );
        }
    }

    int merch_amount = 0;
    for( const auto &elem : caravan_party ) {
        //Scrub temporary party members and the dead
        if( elem->get_part_hp_cur( bodypart_id( "torso" ) ) == 0 && elem->has_companion_mission() ) {
            overmap_buffer.remove_npc( comp->getID() );
            merch_amount += ( time / 600 ) * 4;
        } else if( elem->has_companion_mission() ) {
            merch_amount += ( time / 600 ) * 7;
            companion_skill_trainer( *elem, "combat", experience * 10_minutes, 10 );
            companion_return( *elem );
        }
    }

    if( merch_amount != 0 ) {
        item merch = item( itype_FMCNote );
        get_player_character().i_add_or_drop( merch, merch_amount );
        popup( _( "The caravan party has returned.  Your share of the profits are %d merch!" ),
               merch_amount );
    } else {
        popup( _( "The caravan was a disaster and your companions never made it home…" ) );
    }
}

//A random NPC on one team attacks a random monster on the opposite
void talk_function::attack_random( const std::vector<npc_ptr> &attacker,
                                   const std::vector< monster * > &group )
{
    if( attacker.empty() || group.empty() ) {
        return;
    }
    const auto att = random_entry( attacker );
    monster *def = random_entry( group );
    att->melee_attack_abstract( *def, false, matec_id( "" ) );
    if( def->get_hp() <= 0 ) {
        popup( _( "%s is wasted by %s!" ), def->type->nname(), att->get_name() );
    }
}

//A random monster on one side attacks a random NPC on the other
void talk_function::attack_random( const std::vector< monster * > &group,
                                   const std::vector<npc_ptr> &defender )
{
    if( defender.empty() || group.empty() ) {
        return;
    }
    monster *att = random_entry( group );
    const auto def = random_entry( defender );
    att->melee_attack( *def );
    //monster mon;
    if( def->get_part_hp_cur( bodypart_id( "torso" ) ) <= 0 || def->is_dead() ) {
        popup( _( "%s is wasted by %s!" ), def->get_name(), att->type->nname() );
    }
}

//A random NPC on one team attacks a random NPC on the opposite
void talk_function::attack_random( const std::vector<npc_ptr> &attacker,
                                   const std::vector<npc_ptr> &defender )
{
    if( attacker.empty() || defender.empty() ) {
        return;
    }
    const auto att = random_entry( attacker );
    const auto def = random_entry( defender );
    const skill_id best = att->best_combat_skill( combat_skills::NO_GENERAL ).first;
    int best_score = 1;
    if( best ) {
        best_score = att->get_skill_level( best );
    }
    ///\EFFECT_DODGE_NPC increases avoidance of random attacks
    if( rng( -1, best_score ) >= rng( 0, def->get_skill_level( skill_dodge ) ) ) {
        def->set_part_hp_cur( bodypart_id( "torso" ), 0 );
        popup( _( "%s is wasted by %s!" ), def->get_name(), att->get_name() );
    } else {
        popup( _( "%s dodges %s's attack!" ), def->get_name(), att->get_name() );
    }
}

//Used to determine when to retreat, might want to add in a random factor so that engagements aren't
//drawn out wars of attrition
int talk_function::combat_score( const std::vector<npc_ptr> &group )
{
    int score = 0;
    for( const auto &elem : group ) {
        if( elem->get_part_hp_cur( bodypart_id( "torso" ) ) != 0 ) {
            const skill_id best = elem->best_combat_skill( combat_skills::ALL ).first;
            if( best ) {
                score += round( elem->get_skill_level( best ) );
            } else {
                score += 1;
            }
        }
    }
    return score;
}

int talk_function::combat_score( const std::vector< monster * > &group )
{
    int score = 0;
    for( monster * const &elem : group ) {
        if( elem->get_hp() > 0 ) {
            score += elem->type->difficulty;
        }
    }
    return score;
}

npc_ptr talk_function::temp_npc( const string_id<npc_template> &type )
{
    npc_ptr temp = make_shared_fast<npc>();
    temp->normalize();
    temp->load_npc_template( type );
    return temp;
}

void talk_function::field_plant( npc &p, const std::string &place )
{
    Character &player_character = get_player_character();
    if( !warm_enough_to_plant( player_character.pos() ) ) {
        popup( _( "It is too cold to plant anything now." ) );
        return;
    }
    std::vector<item *> seed_inv = player_character.cache_get_items_with( "is_seed", &item::is_seed,
    []( const item & itm ) {
        return itm.typeId() != itype_marloss_seed && itm.typeId() != itype_fungal_seeds;
    } );
    if( seed_inv.empty() ) {
        popup( _( "You have no seeds to plant!" ) );
        return;
    }

    int empty_plots = 0;
    int free_seeds = 0;

    std::vector<itype_id> seed_types;
    std::vector<std::string> seed_names;
    for( item *&seed : seed_inv ) {
        if( std::find( seed_types.begin(), seed_types.end(), seed->typeId() ) == seed_types.end() ) {
            seed_types.push_back( seed->typeId() );
            seed_names.push_back( seed->tname() );
        }
    }
    // Choose seed if applicable
    const int seed_index = uilist( _( "Which seeds do you wish to have planted?" ), seed_names );
    // Did we cancel?
    if( seed_index < 0 || static_cast<size_t>( seed_index ) >= seed_types.size() ) {
        popup( _( "You saved your seeds for later." ) );
        return;
    }

    const auto &seed_id = seed_types[seed_index];
    if( item::count_by_charges( seed_id ) ) {
        free_seeds = player_character.charges_of( seed_id );
    } else {
        free_seeds = player_character.amount_of( seed_id );
    }

    //Now we need to find how many free plots we have to plant in...
    const tripoint_abs_omt site = overmap_buffer.find_closest(
                                      player_character.global_omt_location(), place, 20, false );
    tinymap bay;
    bay.load( project_to<coords::sm>( site ), false );
    for( const tripoint &plot : bay.points_on_zlevel() ) {
        if( bay.ter( plot ) == t_dirtmound ) {
            empty_plots++;
        }
    }

    if( empty_plots == 0 ) {
        popup( _( "You have no room to plant seeds…" ) );
        return;
    }

    int limiting_number = free_seeds;
    if( free_seeds > empty_plots ) {
        limiting_number = empty_plots;
    }

    int player_merch = player_character.amount_of( itype_FMCNote );
    if( player_merch == 0 ) {
        popup( _( "I'm sorry, you don't have any money to plant those seeds…" ) );
        return;
    }

    if( player_merch < limiting_number ) {
        limiting_number = player_merch;
        if( !query_yn(
                _( "You only have enough money to plant %d plants.  Do you wish to have %d %s planted here for %d merch?" ),
                limiting_number, limiting_number,
                seed_names[seed_index], limiting_number ) ) {
            return;
        }
    } else {
        if( !query_yn( _( "Do you wish to have %d %s planted here for %d merch?" ), limiting_number,
                       seed_names[seed_index], limiting_number ) ) {
            return;
        }
    }

    player_character.use_amount( itype_FMCNote, limiting_number );

    //Plant the actual seeds
    for( const tripoint &plot : bay.points_on_zlevel() ) {
        if( bay.ter( plot ) == t_dirtmound && limiting_number > 0 ) {
            std::list<item> used_seed;
            if( item::count_by_charges( seed_id ) ) {
                used_seed = player_character.use_charges( seed_id, 1 );
            } else {
                used_seed = player_character.use_amount( seed_id, 1 );
            }
            used_seed.front().set_age( 0_turns );
            bay.add_item_or_charges( plot, used_seed.front() );
            bay.set( plot, t_dirt, f_plant_seed );
            limiting_number--;
        }
    }

    bay.save();
    popup( _( "After counting your money and collecting your seeds, %s calls forth a labor party "
              "to plant your field." ), p.get_name() );
}

void talk_function::field_harvest( npc &p, const std::string &place )
{
    Character &player_character = get_player_character();
    //First we need a list of plants that can be harvested...
    const tripoint_abs_omt site = overmap_buffer.find_closest(
                                      player_character.global_omt_location(), place, 20, false );
    tinymap bay;
    item tmp;
    std::vector<itype_id> seed_types;
    std::vector<itype_id> plant_types;
    std::vector<std::string> plant_names;
    bay.load( project_to<coords::sm>( site ), false );
    for( const tripoint &plot : bay.points_on_zlevel() ) {
        map_stack items = bay.i_at( plot );
        if( bay.furn( plot ) == furn_f_plant_harvest && !items.empty() ) {
            // Can't use item_stack::only_item() since there might be fertilizer
            map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
                return it.is_seed();
            } );

            if( seed != items.end() ) {
                const islot_seed &seed_data = *seed->type->seed;
                tmp = item( seed_data.fruit_id, calendar::turn );
                bool check = false;
                for( const std::string &elem : plant_names ) {
                    if( elem == tmp.type_name( 3 ) ) {
                        check = true;
                    }
                }
                if( !check ) {
                    plant_types.push_back( tmp.typeId() );
                    plant_names.push_back( tmp.type_name( 3 ) );
                    seed_types.push_back( seed->typeId() );
                }
            }
        }
    }
    if( plant_names.empty() ) {
        popup( _( "There aren't any plants that are ready to harvest…" ) );
        return;
    }
    // Choose the crop to harvest
    const int plant_index = uilist( _( "Which plants do you want to have harvested?" ),
                                    plant_names );
    // Did we cancel?
    if( plant_index < 0 || static_cast<size_t>( plant_index ) >= plant_types.size() ) {
        popup( _( "You decided to hold off for now…" ) );
        return;
    }

    int number_plots = 0;
    int number_plants = 0;
    int number_seeds = 0;
    int skillLevel = 2;
    if( p.has_trait( trait_NPC_CONSTRUCTION_LEV_2 ) ||
        p.get_value( var_PURCHASED_FIELD_1_FENCE ) == "yes" ) {
        skillLevel += 2;
    }

    for( const tripoint &plot : bay.points_on_zlevel() ) {
        if( bay.furn( plot ) == furn_f_plant_harvest ) {
            // Can't use item_stack::only_item() since there might be fertilizer
            map_stack items = bay.i_at( plot );
            map_stack::iterator seed = std::find_if( items.begin(), items.end(), []( const item & it ) {
                return it.is_seed();
            } );
            if( seed != items.end() ) {
                const islot_seed &seed_data = *seed->type->seed;
                const item item_seed = item( seed->type, calendar::turn );
                tmp = item( seed_data.fruit_id, calendar::turn );
                if( tmp.typeId() == plant_types[plant_index] ) {
                    number_plots++;

                    int plant_count = rng( skillLevel / 2, skillLevel );
                    plant_count *= bay.furn( plot )->plant->harvest_multiplier;
                    plant_count = std::min( std::max( plant_count, 1 ), 12 );

                    // Multiply by the plant's and seed's base charges to mimic creating
                    // items similar to iexamine::harvest_plant
                    number_plants += plant_count * tmp.charges;
                    number_seeds += std::max( 1, rng( plant_count / 4, plant_count / 2 ) ) * item_seed.charges;

                    bay.i_clear( plot );
                    bay.furn_set( plot, f_null );
                    bay.ter_set( plot, t_dirtmound );
                }
            }
        }
    }
    bay.save();
    tmp = item( plant_types[plant_index], calendar::turn );
    int merch_amount = std::max( 0, ( number_plants * tmp.price( true ) / 250 ) - number_plots );
    bool liquidate = false;

    if( !player_character.has_amount( itype_FMCNote, number_plots ) ) {
        liquidate = true;
        popup( _( "You don't have enough to pay the workers to harvest the crop, so you are forced "
                  "to sell…" ) );
    } else {
        liquidate = query_yn( _( "Do you wish to sell the crop of %1$d %2$s for a profit of %3$d merch?" ),
                              number_plants, plant_names[plant_index], merch_amount );
    }

    //Add fruit
    if( liquidate ) {
        add_msg( _( "The %s are sold for %d merch…" ), plant_names[plant_index], merch_amount );
        item merch = item( itype_FMCNote );
        player_character.i_add_or_drop( merch, merch_amount );
    } else {
        if( tmp.count_by_charges() ) {
            tmp.charges = 1;
        }
        player_character.i_add_or_drop( tmp, number_plants );
        add_msg( _( "You receive %d %s…" ), number_plants, plant_names[plant_index] );
    }
    tmp = item( seed_types[plant_index], calendar::turn );
    const islot_seed &seed_data = *tmp.type->seed;
    if( seed_data.spawn_seeds ) {
        if( tmp.count_by_charges() ) {
            tmp.charges = 1;
        }
        player_character.i_add_or_drop( tmp, number_seeds );
        add_msg( _( "You receive %d %s…" ), number_seeds, tmp.type_name( 3 ) );
    }
}

static int scavenging_combat_skill( npc &p, int bonus, bool guns )
{
    // the following doxygen aliases do not yet exist. this is marked for future reference
    ///\EFFECT_MELEE_NPC affects scavenging_patrol results
    ///\EFFECT_SURVIVAL_NPC affects scavenging_patrol results
    ///\EFFECT_BASHING_NPC affects scavenging_patrol results
    ///\EFFECT_CUTTING_NPC affects scavenging_patrol results
    ///\EFFECT_GUN_NPC affects scavenging_patrol results
    ///\EFFECT_STABBING_NPC affects scavenging_patrol results
    ///\EFFECT_UNARMED_NPC affects scavenging_patrol results
    ///\EFFECT_DODGE_NPC affects scavenging_patrol results
    return bonus + p.get_skill_level( skill_melee ) + .5 * p.get_skill_level( skill_survival ) +
           p.get_skill_level( skill_bashing ) + p.get_skill_level( skill_cutting ) +
           ( guns ? p.get_skill_level( skill_gun ) : 0 ) + p.get_skill_level( skill_stabbing ) +
           p.get_skill_level( skill_unarmed ) + p.get_skill_level( skill_dodge );
}

bool talk_function::scavenging_patrol_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, { Scavenging_Patrol_Job, "", {}, std::nullopt},
                                            calendar::turn - 10_hours );
    if( comp == nullptr ) {
        return false;
    }
    int experience = rng( 5, 20 );
    if( one_in( 4 ) ) {
        popup( _( "While scavenging, %s's party suddenly found itself set upon by a large mob of "
                  "undead…" ), comp->get_name() );
        int skill = scavenging_combat_skill( *comp, 4, true );
        if( one_in( 6 ) ) {
            popup( _( "Through quick thinking the group was able to evade combat!" ) );
        } else {
            popup( _( "Combat took place in close quarters, focusing on melee skills…" ) );
            int monsters = rng( 8, 30 );
            if( skill * rng_float( .60, 1.4 ) > .35 * monsters * rng_float( .6, 1.4 ) ) {
                popup( _( "Through brute force the party smashed through the group of %d"
                          " undead!" ), monsters );
                experience += rng( 2, 10 );
            } else {
                popup( _( "Unfortunately they were overpowered by the undead…  I'm sorry." ) );
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }

    int merch_amount = rng( 20, 75 );
    companion_skill_trainer( *comp, "combat", experience * 10_minutes, 10 );
    popup( _( "%s returns from patrol having earned %d merch and a fair bit of experience…" ),
           comp->get_name(), merch_amount );
    if( one_in( 10 ) ) {
        popup( _( "%s was impressed with %s's performance and gave you a small bonus (25 merch)" ),
               p.get_name(), comp->get_name() );
        merch_amount += 25;
    }
    if( one_in( 10 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
        p.set_mutation( trait_NPC_MISSION_LEV_1 );
        popup( _( "%s feels more confident in your abilities and is willing to let you "
                  "participate in daring raids." ), p.get_name() );
    }

    item merch = item( itype_FMCNote );
    get_player_character().i_add_or_drop( merch, merch_amount );

    companion_return( *comp );
    return true;
}

bool talk_function::scavenging_raid_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, { Scavenging_Raid_Job, "", {}, std::nullopt},
                                            calendar::turn - 10_hours );
    if( comp == nullptr ) {
        return false;
    }
    int experience = rng( 10, 20 );
    if( one_in( 2 ) ) {
        popup( _( "While scavenging, %s's party suddenly found itself set upon by a large mob of "
                  "undead…" ), comp->get_name() );
        int skill = scavenging_combat_skill( *comp, 4, true );
        if( one_in( 6 ) ) {
            popup( _( "Through quick thinking the group was able to evade combat!" ) );
        } else {
            popup( _( "Combat took place in close quarters, focusing on melee skills…" ) );
            int monsters = rng( 8, 30 );
            if( skill * rng_float( .60, 1.4 ) > ( .35 * monsters * rng_float( .6, 1.4 ) ) ) {
                popup( _( "Through brute force the party smashed through the group of %d "
                          "undead!" ), monsters );
                experience += rng( 2, 10 );
            } else {
                popup( _( "Unfortunately they were overpowered by the undead…  I'm sorry." ) );
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }
    Character &player_character = get_player_character();
    tripoint_abs_omt loot_location = player_character.global_omt_location();
    std::set<item> all_returned_items;

    for( int i = 0; i < rng( 2, 3 ); i++ ) {
        tripoint_abs_omt site = overmap_buffer.find_closest(
                                    loot_location, "house", 0, false, ot_match_type::prefix );
        // Search the entire height of the house, including the basement and roof
        for( int z = -1; z <= OVERMAP_HEIGHT; z++ ) {
            site.z() = z;

            const oter_id &omt_ref = overmap_buffer.ter( site );
            // We're past the roof, so we can stop
            if( omt_ref == oter_open_air ) {
                break;
            }
            const std::string om_cur = omt_ref.id().c_str();

            oter_str_id looted_replacement = oter_looted_house;
            if( om_cur.find( "_roof" ) != std::string::npos ) {
                looted_replacement = oter_looted_house_roof;
            }

            if( z == -1 ) {
                if( om_cur.find( "basement" ) != std::string::npos ) {
                    looted_replacement = oter_looted_house_basement;
                } else {
                    continue;
                }
            }

            overmap_buffer.reveal( site, 2 );
            std::set<item> returned_items = loot_building( site, looted_replacement );
            all_returned_items.insert( returned_items.begin(), returned_items.end() );
        }
    }

    int merch_amount = rng( 50, 100 );
    companion_skill_trainer( *comp, "combat", experience * 10_minutes, 10 );
    popup( _( "%s returns from the raid having earned %d merch and a fair bit of experience…" ),
           comp->get_name(), merch_amount );
    if( one_in( 20 ) ) {
        popup( _( "%s was impressed with %s's performance and gave you a small bonus (25 merch)" ),
               p.get_name(), comp->get_name() );
        merch_amount += 25;
    }
    if( one_in( 2 ) ) {
        item_group_id itemlist( "npc_misc" );
        if( one_in( 8 ) ) {
            itemlist = Item_spawn_data_npc_weapon_random;
        }
        item result = item_group::item_from( itemlist );
        if( !result.is_null() ) {
            popup( _( "%s returned with a %s for you!" ), comp->get_name(), result.tname() );
            player_character.i_add_or_drop( result );
        }
    }

    std::list<item> to_distribute;
    for( item i : all_returned_items ) {
        // Scavengers get most items and put them up for sale, player gets the scraps
        if( one_in( 8 ) ) {
            player_character.i_drop_at( i );
        } else {
            i.set_owner( p );
            to_distribute.push_back( i );
        }
    }
    distribute_items_to_npc_zones( to_distribute, p );

    item merch = item( itype_FMCNote );
    player_character.i_add_or_drop( merch, merch_amount );

    companion_return( *comp );
    return true;
}

bool talk_function::hospital_raid_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, { Hospital_Raid_Job, "", {}, std::nullopt},
                                            calendar::turn - 20_hours );
    if( comp == nullptr ) {
        return false;
    }
    int experience = rng( 20, 40 );
    if( one_in( 2 ) ) {
        popup( _( "While scavenging, %s's party suddenly found itself set upon by a large mob of "
                  "undead…" ), comp->get_name() );
        int skill = scavenging_combat_skill( *comp, 4, true );
        if( one_in( 10 ) ) {
            popup( _( "Through quick thinking the group was able to evade combat!" ) );
        } else {
            popup( _( "Combat took place in close quarters, focusing on melee skills…" ) );
            int monsters = rng( 12, 30 );
            if( skill * rng_float( .60, 1.4 ) > ( .35 * monsters * rng_float( .6, 1.4 ) ) ) {
                popup( _( "Through brute force the party smashed through the group of %d "
                          "undead!" ), monsters );
                experience += rng( 2, 10 );
            } else {
                popup( _( "Unfortunately they were overpowered by the undead…  I'm sorry." ) );
                overmap_buffer.remove_npc( comp->getID() );

                // Let the player retry if everybody dies
                get_player_character().set_value( var_SCAVENGER_HOSPITAL_RAID_STARTED, "no" );
                return false;
            }
        }
    }
    Character &player_character = get_player_character();
    tripoint_abs_omt loot_location = player_character.global_omt_location();
    std::set<item> all_returned_items;
    for( int i = 0; i < rng( 2, 3 ); i++ ) {
        tripoint_abs_omt site = overmap_buffer.find_closest(
                                    loot_location, "hospital", 0, false, ot_match_type::prefix );
        if( site == overmap::invalid_tripoint ) {
            debugmsg( "No hospitals found." );
        } else {
            // Search the entire height of the hospital, including the roof
            for( int z = 0; z <= OVERMAP_HEIGHT; z++ ) {
                site.z() = z;

                const oter_id &omt_ref = overmap_buffer.ter( site );
                // We're past the roof, so we can stop
                if( omt_ref == oter_open_air ) {
                    break;
                }
                const std::string om_cur = omt_ref.id().c_str();

                oter_str_id looted_replacement = oter_looted_hospital;
                if( om_cur.find( "_roof" ) != std::string::npos ) {
                    looted_replacement = oter_looted_hospital_roof;
                }

                overmap_buffer.reveal( site, 2 );
                std::set<item> returned_items = loot_building( site, looted_replacement );
                all_returned_items.insert( returned_items.begin(), returned_items.end() );
            }
        }
    }

    companion_skill_trainer( *comp, "combat", experience * 10_minutes, 10 );
    popup( _( "%s returns from the raid having a fair bit of experience…" ),
           comp->get_name() );

    if( one_in( 2 ) ) {
        item_group_id itemlist( "npc_misc" );
        if( one_in( 8 ) ) {
            itemlist = Item_spawn_data_npc_weapon_random;
        }
        item result = item_group::item_from( itemlist );
        if( !result.is_null() ) {
            popup( _( "%s returned with a %s for you!" ), comp->get_name(), result.tname() );
            player_character.i_add_or_drop( result );
        }
    }

    std::list<item> to_distribute;
    for( item i : all_returned_items ) {
        // One time mission, so the loot gets split evenly
        if( one_in( 2 ) ) {
            player_character.i_drop_at( i );
        } else {
            i.set_owner( p );
            to_distribute.push_back( i );
        }
    }
    distribute_items_to_npc_zones( to_distribute, p );

    player_character.set_value( var_DOCTOR_ANESTHETIC_SCAVENGERS_HELPED, "yes" );
    player_character.set_value( var_SCAVENGER_HOSPITAL_RAID, "no" );

    popup( _( "%s returned with some medical equipment that should help the doctor!" ),
           comp->get_name() );

    companion_return( *comp );
    return true;
}

bool talk_function::labor_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, { Menial_Job, "", {}, std::nullopt}, calendar::turn -
                                            1_hours );
    if( comp == nullptr ) {
        return false;
    }

    float hours = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int merch_amount = 3 * hours;

    companion_skill_trainer( *comp, "menial", calendar::turn - comp->companion_mission_time, 1 );

    popup( _( "%s returns from working as a laborer having earned %d merch and a bit of experience…" ),
           comp->get_name(), merch_amount );
    companion_return( *comp );
    if( hours >= 8 && one_in( 8 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
        p.set_mutation( trait_NPC_MISSION_LEV_1 );
        popup( _( "%s feels more confident in your companions and is willing to let them "
                  "participate in advanced tasks." ), p.get_name() );
    }

    item merch = item( itype_FMCNote );
    get_player_character().i_add_or_drop( merch, merch_amount );

    return true;
}

bool talk_function::carpenter_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, { Carpentry_Job, "", {}, std::nullopt},
                                            calendar::turn - 1_hours );
    if( comp == nullptr ) {
        return false;
    }

    if( one_in( 20 ) ) {
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_FABRICATION_NPC affects carpenter mission results

        ///\EFFECT_DODGE_NPC affects carpenter mission results

        ///\EFFECT_SURVIVAL_NPC affects carpenter mission results
        int skill_1 = comp->get_greater_skill_or_knowledge_level( skill_fabrication );
        int skill_2 = comp->get_skill_level( skill_dodge );
        int skill_3 = comp->get_skill_level( skill_survival );
        popup( _( "While %s was framing a building, one of the walls began to collapse…" ),
               comp->get_name() );
        if( skill_1 > rng( 1, 8 ) ) {
            popup( _( "In the blink of an eye, %s threw a brace up and averted a disaster." ),
                   comp->get_name() );
        } else if( skill_2 > rng( 1, 8 ) ) {
            popup( _( "Darting out a window, %s escaped the collapse." ), comp->get_name() );
        } else if( skill_3 > rng( 1, 8 ) ) {
            popup( _( "%s didn't make it out in time…" ), comp->get_name() );
            popup( _( "but %s was rescued from the debris with only minor injuries!" ),
                   comp->get_name() );
        } else {
            popup( _( "%s didn't make it out in time…" ), comp->get_name() );
            popup( _( "Everyone who was trapped under the collapsing roof died…" ) );
            popup( _( "I'm sorry, there is nothing we could do." ) );
            overmap_buffer.remove_npc( comp->getID() );
            return false;
        }
    }

    float hours = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int merch_amount = 5 * hours;

    companion_skill_trainer( *comp, "construction", calendar::turn -
                             comp->companion_mission_time, 2 );

    popup( _( "%s returns from working as a carpenter having earned %d merch and a bit of "
              "experience…" ), comp->get_name(), merch_amount );

    item merch = item( itype_FMCNote );
    get_player_character().i_add_or_drop( merch, merch_amount );

    companion_return( *comp );
    return true;
}

bool talk_function::forage_return( npc &p )
{
    npc_ptr comp = companion_choose_return( p, { Forage_Job, "", {}, std::nullopt}, calendar::turn -
                                            4_hours );
    if( comp == nullptr ) {
        return false;
    }

    if( one_in( 10 ) ) {
        popup( _( "While foraging, a beast began to stalk %s…" ), comp->get_name() );
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_SURVIVAL_NPC affects forage mission results

        ///\EFFECT_DODGE_NPC affects forage mission results
        int skill_1 = comp->get_skill_level( skill_survival );
        int skill_2 = comp->get_skill_level( skill_dodge );
        if( skill_1 > rng( -2, 8 ) ) {
            popup( _( "Alerted by a rustle, %s fled to the safety of the outpost!" ), comp->get_name() );
        } else if( skill_2 > rng( -2, 8 ) ) {
            popup( _( "As soon as the cougar sprang, %s darted to the safety of the outpost!" ),
                   comp->get_name() );
        } else {
            popup( _( "%s was caught unaware and was forced to fight the creature at close "
                      "range!" ), comp->get_name() );
            int skill = scavenging_combat_skill( *comp, 0, false );
            int monsters = rng( 0, 10 );
            if( skill * rng_float( .80, 1.2 ) > monsters * rng_float( .8, 1.2 ) ) {
                if( one_in( 2 ) ) {
                    popup( _( "%s was able to scare off the bear after delivering a nasty "
                              "blow!" ), comp->get_name() );
                } else {
                    popup( _( "%s beat the cougar into a bloody pulp!" ), comp->get_name() );
                }
            } else {
                if( one_in( 2 ) ) {
                    popup( _( "%s was able to hold off the first wolf, but the others that were "
                              "skulking in the tree line caught up…" ), comp->get_name() );
                    popup( _( "I'm sorry, there wasn't anything we could do…" ) );
                } else {
                    popup( _( "We… we don't know what exactly happened, but we found %s's gear "
                              "ripped and bloody…" ), comp->get_name() );
                    popup( _( "I fear your companion won't be returning." ) );
                }
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }

    Character &player_character = get_player_character();
    float hours = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int merch_amount = 4 * hours;

    companion_skill_trainer( *comp, "gathering", calendar::turn -
                             comp->companion_mission_time, 2 );

    popup( _( "%s returns from working as a forager having earned %d merch and a bit of "
              "experience…" ), comp->get_name(), merch_amount );

    item merch = item( itype_FMCNote );
    player_character.i_add_or_drop( merch, merch_amount );
    // the following doxygen aliases do not yet exist. this is marked for future reference

    ///\EFFECT_SURVIVAL_NPC affects forage mission results
    float skill = comp->get_skill_level( skill_survival );
    if( skill > rng_float( -.5, 8 ) ) {
        item_group_id itemlist = Item_spawn_data_farming_seeds;
        if( one_in( 2 ) ) {
            switch( season_of_year( calendar::turn ) ) {
                case SPRING:
                    itemlist = Item_spawn_data_forage_spring;
                    break;
                case SUMMER:
                    itemlist = Item_spawn_data_forage_summer;
                    break;
                case AUTUMN:
                    itemlist = Item_spawn_data_forage_autumn;
                    break;
                case WINTER:
                    itemlist = Item_spawn_data_forage_winter;
                    break;
                default:
                    debugmsg( "Invalid season" );
            }
        }
        item result = item_group::item_from( itemlist );
        if( !result.is_null() ) {
            popup( _( "%s returned with a %s for you!" ), comp->get_name(), result.tname() );
            player_character.i_add_or_drop( result );
        }
        if( one_in( 6 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ) {
            p.set_mutation( trait_NPC_MISSION_LEV_1 );
            popup( _( "%s feels more confident in your companions and is willing to let them "
                      "participate in advanced tasks." ), p.get_name() );
        }
    }
    companion_return( *comp );
    return true;
}

bool talk_function::companion_om_combat_check( const std::vector<npc_ptr> &group,
        const tripoint_abs_omt &om_tgt, bool try_engage )
{
    if( overmap_buffer.is_safe( om_tgt ) ) {
        //Should work but is_safe is always returning true regardless of what is there.
        //return true;
    }

    tripoint_abs_sm sm_tgt = project_to<coords::sm>( om_tgt );

    tinymap target_bay;
    target_bay.load( sm_tgt, false );
    std::vector< monster * > monsters_around;
    for( int x = 0; x < 2; x++ ) {
        for( int y = 0; y < 2; y++ ) {
            tripoint_abs_sm sm = sm_tgt + point( x, y );
            point_abs_om omp;
            tripoint_om_sm local_sm;
            std::tie( omp, local_sm ) = project_remain<coords::om>( sm );
            overmap &omi = overmap_buffer.get( omp );

            auto monster_bucket = omi.monster_map.equal_range( local_sm );
            std::for_each( monster_bucket.first,
            monster_bucket.second, [&]( std::pair<const tripoint_om_sm, monster> &monster_entry ) {
                monster &this_monster = monster_entry.second;
                monsters_around.push_back( &this_monster );
            } );
        }
    }
    float avg_survival = 0.0f;
    for( const auto &guy : group ) {
        avg_survival += guy->get_skill_level( skill_survival );
    }
    avg_survival = avg_survival / group.size();

    monster mon;

    std::vector< monster * > monsters_fighting;
    for( monster *mons : monsters_around ) {
        if( mons->get_hp() <= 0 ) {
            continue;
        }
        int d_modifier = avg_survival - mons->type->difficulty;
        int roll = rng( 1, 20 ) + d_modifier;
        if( roll > 10 ) {
            if( try_engage ) {
                mons->death_drops = false;
                monsters_fighting.push_back( mons );
            }
        } else {
            mons->death_drops = false;
            monsters_fighting.push_back( mons );
        }
    }

    if( !monsters_fighting.empty() ) {
        bool outcome = force_on_force( group, "Patrol", monsters_fighting, "attacking monsters",
                                       rng( -1, 2 ) );
        for( monster *mons : monsters_fighting ) {
            mons->death_drops = true;
        }
        return outcome;
    }
    return true;
}

bool talk_function::force_on_force( const std::vector<npc_ptr> &defender,
                                    const std::string &def_desc,
                                    const std::vector< monster * > &monsters_fighting,
                                    const std::string &att_desc, int advantage )
{
    std::string adv;
    if( advantage < 0 ) {
        adv = ", attacker advantage";
    } else if( advantage > 0 ) {
        adv = ", defender advantage";
    }
    Character &player_character = get_player_character();
    faction *yours = player_character.get_faction();
    //Find out why your followers don't have your faction...
    popup( _( "Engagement between %d members of %s %s and %d %s%s!" ), defender.size(),
           yours->name, def_desc, monsters_fighting.size(), att_desc, adv );
    int defense = 0;
    int attack = 0;
    int att_init = 0;
    int def_init = 0;
    while( true ) {
        std::vector< monster * > remaining_mon;
        for( monster * const &elem : monsters_fighting ) {
            if( elem->get_hp() > 0 ) {
                remaining_mon.push_back( elem );
            }
        }
        std::vector<npc_ptr> remaining_def;
        for( const auto &elem : defender ) {
            if( !elem->is_dead() && elem->get_part_hp_cur( bodypart_id( "torso" ) ) >= 0 &&
                elem->get_part_hp_cur( bodypart_id( "head" ) ) >= 0 ) {
                remaining_def.push_back( elem );
            }
        }

        defense = combat_score( remaining_def );
        attack = combat_score( remaining_mon );
        if( attack > defense * 3 ) {
            attack_random( remaining_mon, remaining_def );
            if( defense == 0 || ( remaining_def.size() == 1 && remaining_def[0]->is_dead() ) ) {
                //Here too...
                popup( _( "%s forces are destroyed!" ), yours->name );
            } else {
                //Again, no faction for your followers
                popup( _( "%s forces retreat from combat!" ), yours->name );
            }
            return false;
        } else if( attack * 3 < defense ) {
            attack_random( remaining_def, remaining_mon );
            if( attack == 0 || ( remaining_mon.size() == 1 && remaining_mon[0]->get_hp() == 0 ) ) {
                popup( _( "The monsters are destroyed!" ) );
            } else {
                popup( _( "The monsters disengage!" ) );
            }
            return true;
        } else {
            def_init = rng( 1, 6 ) + advantage;
            att_init = rng( 1, 6 );
            if( def_init >= att_init ) {
                attack_random( remaining_mon, remaining_def );
            }
            if( def_init <= att_init ) {
                attack_random( remaining_def, remaining_mon );
            }
        }
    }
}

void talk_function::force_on_force( const std::vector<npc_ptr> &defender,
                                    const std::string &def_desc,
                                    const std::vector<npc_ptr> &attacker,
                                    const std::string &att_desc, int advantage )
{
    std::string adv;
    if( advantage < 0 ) {
        adv = ", attacker advantage";
    } else if( advantage > 0 ) {
        adv = ", defender advantage";
    }
    popup( _( "Engagement between %d members of %s %s and %d members of %s %s%s!" ),
           defender.size(), defender[0]->get_faction()->name, def_desc, attacker.size(),
           attacker[0]->get_faction()->name, att_desc, adv );
    int defense = 0;
    int attack = 0;
    int att_init = 0;
    int def_init = 0;
    while( true ) {
        std::vector<npc_ptr> remaining_att;
        for( const auto &elem : attacker ) {
            if( elem->get_part_hp_cur( bodypart_id( "torso" ) ) != 0 ) {
                remaining_att.push_back( elem );
            }
        }
        std::vector<npc_ptr> remaining_def;
        for( const auto &elem : defender ) {
            if( elem->get_part_hp_cur( bodypart_id( "torso" ) ) != 0 ) {
                remaining_def.push_back( elem );
            }
        }

        defense = combat_score( remaining_def );
        attack = combat_score( remaining_att );
        if( attack > defense * 3 ) {
            attack_random( remaining_att, remaining_def );
            if( defense == 0 || ( remaining_def.size() == 1 &&
                                  remaining_def[0]->get_part_hp_cur( bodypart_id( "torso" ) ) == 0 ) ) {
                popup( _( "%s forces are destroyed!" ), defender[0]->get_faction()->name );
            } else {
                popup( _( "%s forces retreat from combat!" ), defender[0]->get_faction()->name );
            }
            return;
        } else if( attack * 3 < defense ) {
            attack_random( remaining_def, remaining_att );
            if( attack == 0 || ( remaining_att.size() == 1 &&
                                 remaining_att[0]->get_part_hp_cur( bodypart_id( "torso" ) ) == 0 ) ) {
                popup( _( "%s forces are destroyed!" ), attacker[0]->get_faction()->name );
            } else {
                popup( _( "%s forces retreat from combat!" ), attacker[0]->get_faction()->name );
            }
            return;
        } else {
            def_init = rng( 1, 6 ) + advantage;
            att_init = rng( 1, 6 );
            if( def_init >= att_init ) {
                attack_random( remaining_att, remaining_def );
            }
            if( def_init <= att_init ) {
                attack_random( remaining_def, remaining_att );
            }
        }
    }
}

// skill training support functions
void talk_function::companion_skill_trainer( npc &comp, const std::string &skill_tested,
        time_duration time_worked, int difficulty )
{
    difficulty = std::max( 1, difficulty );
    int checks = 1 + to_minutes<int>( time_worked ) / 10;

    weighted_int_list<skill_id> skill_practice;
    if( skill_tested == "combat" ) {
        const skill_id best_skill = comp.best_combat_skill( combat_skills::ALL ).first;
        if( best_skill ) {
            skill_practice.add( best_skill, 30 );
        }
    }
    for( Skill &sk : Skill::skills ) {
        skill_practice.add( sk.ident(), sk.get_companion_skill_practice( skill_tested ) );
    }
    if( skill_practice.empty() ) {
        comp.practice( skill_id( skill_tested ), difficulty * to_minutes<int>( time_worked ) / 10 );
    } else {
        for( int i = 0; i < checks; i++ ) {
            skill_id *ident = skill_practice.pick();
            if( ident ) {
                comp.practice( *ident, difficulty );
            }
        }
    }
}

void talk_function::companion_skill_trainer( npc &comp, const skill_id &skill_tested,
        time_duration time_worked, int difficulty )
{
    difficulty = std::max( 1, difficulty );
    comp.practice( skill_tested, difficulty * to_minutes<int>( time_worked ) / 10 );
}

void talk_function::companion_return( npc &comp )
{
    cata_assert( !comp.is_active() );
    comp.reset_companion_mission();
    comp.companion_mission_time = calendar::before_time_starts;
    comp.companion_mission_time_ret = calendar::before_time_starts;
    Character &player_character = get_player_character();
    map &here = get_map();
    for( size_t i = 0; i < comp.companion_mission_inv.size(); i++ ) {
        for( const item &it : comp.companion_mission_inv.const_stack( i ) ) {
            if( !it.count_by_charges() || it.charges > 0 ) {
                here.add_item_or_charges( player_character.pos(), it );
            }
        }
    }
    comp.companion_mission_inv.clear();
    comp.companion_mission_points.clear();
    // npc *may* be active, or not if outside the reality bubble
    g->reload_npcs();
}

std::vector<npc_ptr> talk_function::companion_list( const npc &p, const mission_id &miss_id,
        bool contains )
{
    std::vector<npc_ptr> available;
    const tripoint_abs_omt omt_pos = p.global_omt_location();
    for( const auto &elem : overmap_buffer.get_companion_mission_npcs() ) {
        npc_companion_mission c_mission = elem->get_companion_mission();
        if( c_mission.position == omt_pos && is_equal( c_mission.miss_id, miss_id ) &&
            c_mission.role_id == p.companion_mission_role_id ) { // NOLINT(bugprone-branch-clone)
            available.push_back( elem );
        } else if( contains && c_mission.miss_id.id == miss_id.id ) {
            available.push_back( elem );
        }
    }
    return available;
}

static int companion_combat_rank( const npc &p )
{
    int combat = 2 * p.get_dex() + 3 * p.get_str() + 2 * p.get_per() + p.get_int();
    for( const Skill &sk : Skill::skills ) {
        combat += p.get_skill_level( sk.ident() ) * sk.companion_combat_rank_factor();
    }
    return combat * std::min( p.get_dex(), 32 ) * std::min( p.get_str(), 32 ) / 64;
}

static int companion_survival_rank( const npc &p )
{
    float survival = 2 * p.get_dex() + p.get_str() + 2 * p.get_per() + 1.5 * p.get_int();
    for( const Skill &sk : Skill::skills ) {
        survival += p.get_skill_level( sk.ident() ) * sk.companion_survival_rank_factor();
    }
    return survival * std::min( p.get_dex(), 32 ) * std::min( p.get_per(), 32 ) / 64;
}

static int companion_industry_rank( const npc &p )
{
    int industry = p.get_dex() + p.get_str() + p.get_per() + 3 * p.get_int();
    for( const Skill &sk : Skill::skills ) {
        industry += p.get_skill_level( sk.ident() ) * sk.companion_industry_rank_factor();
    }
    return industry * std::min( p.get_int(), 32 ) / 8;
}

static bool companion_sort_compare( const npc_ptr &first, const npc_ptr &second )
{
    return companion_combat_rank( *first ) > companion_combat_rank( *second );
}

comp_list talk_function::companion_sort( comp_list available,
        const std::map<skill_id, int> &required_skills )
{
    if( required_skills.empty() ) {
        std::sort( available.begin(), available.end(), companion_sort_compare );
        return available;
    }
    skill_id hardest_skill;
    int hardest_diff = -1;
    for( const std::pair<const skill_id, int> &req_skill : required_skills ) {
        if( req_skill.second > hardest_diff ) {
            hardest_diff = req_skill.second;
            hardest_skill = req_skill.first;
        }
    }

    struct companion_sort_skill {
        explicit companion_sort_skill( const skill_id  &skill_tested ) {
            req_skill = skill_tested;
        }

        bool operator()( const npc_ptr &first, const npc_ptr &second ) const {
            return static_cast<int>( first->get_skill_level( req_skill ) ) > static_cast<int>
                   ( second->get_skill_level( req_skill ) );
        }

        skill_id req_skill;
    };
    std::sort( available.begin(), available.end(), companion_sort_skill( hardest_skill ) );

    return available;
}

std::vector<comp_rank> talk_function::companion_rank( const std::vector<npc_ptr> &available,
        bool adj )
{
    std::vector<comp_rank> raw;
    int max_combat = 0;
    int max_survival = 0;
    int max_industry = 0;
    for( const auto &e : available ) {
        comp_rank r;
        r.combat = companion_combat_rank( *e );
        r.survival = companion_survival_rank( *e );
        r.industry = companion_industry_rank( *e );
        raw.push_back( r );
        if( r.combat > max_combat ) {
            max_combat = r.combat;
        }
        if( r.survival > max_survival ) {
            max_survival = r.survival;
        }
        if( r.industry > max_industry ) {
            max_industry = r.industry;
        }
    }

    if( !adj ) {
        return raw;
    }

    std::vector<comp_rank> adjusted;
    for( const comp_rank &entry : raw ) {
        comp_rank r;
        r.combat = max_combat ? 100 * entry.combat / max_combat : 0;
        r.survival = max_survival ? 100 * entry.survival / max_survival : 0;
        r.industry = max_industry ? 100 * entry.industry / max_industry : 0;
        adjusted.push_back( r );
    }
    return adjusted;
}

npc_ptr talk_function::companion_choose( const std::map<skill_id, int> &required_skills,
        bool silent_failure )
{
    Character &player_character = get_player_character();
    std::vector<npc_ptr> available;
    std::optional<basecamp *> bcp = overmap_buffer.find_camp(
                                        player_character.global_omt_location().xy() );

    for( const character_id &elem : g->get_follower_list() ) {
        npc_ptr guy = overmap_buffer.find_npc( elem );
        if( !guy || guy->is_hallucination() ) {
            continue;
        }
        npc_companion_mission c_mission = guy->get_companion_mission();
        // get non-assigned visible followers
        if( player_character.posz() == guy->posz() && !guy->has_companion_mission() &&
            !guy->is_travelling() &&
            ( rl_dist( player_character.pos(), guy->pos() ) <= SEEX * 2 ) &&
            player_character.sees( guy->pos() ) ) {
            available.push_back( guy );
        } else if( bcp ) {
            basecamp *player_camp = *bcp;
            std::vector<npc_ptr> camp_npcs = player_camp->get_npcs_assigned();
            if( std::any_of( camp_npcs.begin(), camp_npcs.end(),
            [guy]( const npc_ptr & i ) {
            return ( i == guy ) && ( !guy->has_companion_mission() );
            } ) ) {
                available.push_back( guy );
            }
        } else {
            const tripoint_abs_omt guy_omt_pos = guy->global_omt_location();
            std::optional<basecamp *> guy_camp = overmap_buffer.find_camp( guy_omt_pos.xy() );
            if( guy_camp ) {
                // get NPCs assigned to guard a remote base
                basecamp *temp_camp = *guy_camp;
                std::vector<npc_ptr> assigned_npcs = temp_camp->get_npcs_assigned();
                if( std::any_of( assigned_npcs.begin(), assigned_npcs.end(),
                [guy]( const npc_ptr & i ) {
                return ( i == guy ) && ( !guy->has_companion_mission() );
                } ) ) {
                    available.push_back( guy );
                }
            }
        }
    }
    if( available.empty() ) {
        if( !silent_failure ) {
            popup( _( "You don't have any companions to send out…" ) );
        }
        return nullptr;
    }
    std::vector<uilist_entry> npc_menu;
    available = companion_sort( available, required_skills );
    std::vector<comp_rank> rankings = companion_rank( available );

    int x = 0;
    std::string menu_header;

    if( silent_failure ) {
        menu_header = left_justify( _( "Do you want to send someone else?" ), 51 );
    } else {
        menu_header = left_justify( _( "Who do you want to send?" ), 51 );
    }

    if( required_skills.empty() ) {
        menu_header += _( "[ COMBAT : SURVIVAL : INDUSTRY ]" );
    }
    for( const npc_ptr &e : available ) {
        std::string npc_desc;
        bool can_do = true;
        if( e->mission == NPC_MISSION_GUARD_ALLY ) {
            //~ %1$s: npc name
            npc_desc = string_format( pgettext( "companion", "%1$s (Guarding)" ), e->get_name() );
        } else {
            npc_desc = e->get_name();
        }
        if( required_skills.empty() ) {
            npc_desc = string_format( pgettext( "companion ranking", "%s [ %4d : %4d : %4d ]" ),
                                      left_justify( npc_desc, 51 ), rankings[x].combat,
                                      rankings[x].survival, rankings[x].industry );
        } else {
            npc_desc = left_justify( npc_desc, 51 );
            bool first = true;
            for( const std::pair<const skill_id, int> &skill_tested : required_skills ) {
                if( first ) {
                    first = false;
                } else {
                    npc_desc += ", ";
                }
                skill_id skill_tested_id = skill_tested.first;
                int skill_level = skill_tested.second;
                if( skill_level == 0 ) {
                    //~ %1$s: skill name, %2$d: companion skill level
                    npc_desc += string_format( pgettext( "companion skill", "%1$s %2$d" ),
                                               skill_tested_id.obj().name(),
                                               e->get_knowledge_level( skill_tested_id ) );
                } else {
                    //~ %1$s: skill name, %2$d: companion skill level, %3$d: skill requirement
                    npc_desc += string_format( pgettext( "companion skill", "%1$s %2$d/%3$d" ),
                                               skill_tested_id.obj().name(),
                                               e->get_knowledge_level( skill_tested_id ),
                                               skill_level );
                    can_do &= e->get_knowledge_level( skill_tested_id ) >= skill_level;
                }
            }
        }
        uilist_entry npc_entry = uilist_entry( x, can_do, -1, npc_desc );
        npc_menu.push_back( npc_entry );
        x++;
    }
    const size_t npc_choice = uilist( menu_header, npc_menu );
    if( npc_choice >= available.size() ) {
        if( !silent_failure ) {
            popup( _( "You choose to send no one…" ), npc_choice );
        }
        return nullptr;
    }

    return available[npc_choice];
}

npc_ptr talk_function::companion_choose_return( const npc &p, const mission_id &miss_id,
        const time_point &deadline, const bool ignore_parameters )
{
    const tripoint_abs_omt omt_pos = p.global_omt_location();
    const std::string &role_id = p.companion_mission_role_id;
    return companion_choose_return( omt_pos, role_id, miss_id, deadline, true, ignore_parameters );
}
npc_ptr talk_function::companion_choose_return( const tripoint_abs_omt &omt_pos,
        const std::string &role_id,
        const mission_id &miss_id,
        const time_point &deadline,
        const bool by_mission,
        const bool ignore_parameters )
{
    std::vector<npc_ptr> available;
    Character &player_character = get_player_character();
    for( npc_ptr &guy : overmap_buffer.get_companion_mission_npcs() ) {
        npc_companion_mission c_mission = guy->get_companion_mission();
        if( c_mission.position != omt_pos ||
            c_mission.role_id != role_id ) {
            continue;
        }

        if( by_mission ) {
            if( c_mission.miss_id.id != miss_id.id || c_mission.miss_id.dir != miss_id.dir ) {
                continue;
            }
            if( !ignore_parameters && c_mission.miss_id.parameters != miss_id.parameters ) {
                continue;
            }
        }

        if( player_character.has_trait( trait_DEBUG_HS ) ||
            guy->companion_mission_time <= deadline ||
            ( deadline == calendar::before_time_starts &&
              guy->companion_mission_time_ret <= calendar::turn ) ) {
            available.push_back( guy );
        }
    }

    return companion_choose_return( available );
}

npc_ptr talk_function::companion_choose_return( comp_list &npc_list )
{
    if( npc_list.empty() ) {
        popup( _( "You don't have any companions ready to return…" ) );
        return nullptr;
    }

    if( npc_list.size() == 1 ) {
        return npc_list[0];
    }

    std::vector<std::string> npcs;
    npcs.reserve( npc_list.size() );
    for( auto &elem : npc_list ) {
        npcs.push_back( elem->get_name() );
    }
    const size_t npc_choice = uilist( _( "Who should return?" ), npcs );
    if( npc_choice < npc_list.size() ) {
        return npc_list[npc_choice];
    }
    popup( _( "No one returns to your party…" ) );
    return nullptr;
}

//Smash stuff, steal valuables, and change map marker
std::set<item> talk_function::loot_building( const tripoint_abs_omt &site,
        const oter_str_id &looted_replacement )
{
    tinymap bay;
    bay.load( project_to<coords::sm>( site ), false );
    creature_tracker &creatures = get_creature_tracker();
    std::set<item> return_items;
    for( const tripoint &p : bay.points_on_zlevel() ) {
        const ter_id t = bay.ter( p );
        //Open all the doors, doesn't need to be exhaustive
        if( t == t_door_c || t == t_door_c_peep || t == t_door_b
            || t == t_door_boarded || t == t_door_boarded_damaged
            || t == t_rdoor_boarded || t == t_rdoor_boarded_damaged
            || t == t_door_boarded_peep || t == t_door_boarded_damaged_peep ) {
            bay.ter_set( p, t_door_o );
        } else if( t == t_door_locked || t == t_door_locked_peep || t == t_door_locked_alarm ) {
            const map_bash_info &bash = bay.ter( p ).obj().bash;
            bay.ter_set( p, bash.ter_set );
            // Bash doors twice
            const map_bash_info &bash_again = bay.ter( p ).obj().bash;
            bay.ter_set( p, bash_again.ter_set );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
            bay.spawn_items( p, item_group::items_from( bash_again.drop_group, calendar::turn ) );
        } else if( t == t_door_metal_c || t == t_door_metal_locked || t == t_door_metal_pickable ) {
            bay.ter_set( p, t_door_metal_o );
        } else if( t == t_door_glass_c ) {
            bay.ter_set( p, t_door_glass_o );
        } else if( t == t_wall && one_in( 25 ) ) {
            const map_bash_info &bash = bay.ter( p ).obj().bash;
            bay.ter_set( p, bash.ter_set );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
            bay.collapse_at( p, false );
        }
        //Smash easily breakable stuff
        else if( ( t == t_window || t == t_window_taped || t == t_window_domestic ||
                   t == t_window_boarded_noglass || t == t_window_domestic_taped ||
                   t == t_window_alarm_taped || t == t_window_boarded ||
                   t == t_curtains || t == t_window_alarm ||
                   t == t_window_no_curtains || t == t_window_no_curtains_taped )
                 && one_in( 4 ) ) {
            const map_bash_info &bash = bay.ter( p ).obj().bash;
            bay.ter_set( p, bash.ter_set );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
        } else if( ( t == t_wall_glass || t == t_wall_glass_alarm ) && one_in( 3 ) ) {
            const map_bash_info &bash = bay.ter( p ).obj().bash;
            bay.ter_set( p, bash.ter_set );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
        } else if( bay.has_furn( p ) && bay.furn( p ).obj().bash.str_max != -1 && one_in( 10 ) ) {
            const map_bash_info &bash = bay.furn( p ).obj().bash;
            bay.furn_set( p, bash.furn_set );
            bay.delete_signage( p );
            bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
        }
        //Kill zombies!  Only works against pre-spawned enemies at the moment...
        Creature *critter = creatures.creature_at( p );
        if( critter != nullptr ) {
            critter->die( nullptr );
        }
        //Hoover up tasty items!
        map_stack items = bay.i_at( p );
        for( map_stack::iterator it = items.begin(); it != items.end(); ) {
            if( !it->made_of( phase_id::LIQUID ) &&
                ( ( ( it->is_food() || it->is_food_container() ) && !one_in( 8 ) ) ||
                  ( it->price( true ) > 1000 && !one_in( 4 ) ) || one_in( 5 ) ) ) {
                return_items.insert( *it );
                it = items.erase( it );
            } else {
                ++it;
            }
        }
    }
    bay.save();
    overmap_buffer.ter_set( site, looted_replacement );

    return return_items;
}

void mission_data::add_return( const mission_id &id, const std::string &name_display,
                               const std::string &text, bool possible )
{
    add( { id, true }, name_display, text, true, possible );
}
void mission_data::add_start( const mission_id &id, const std::string &name_display,
                              const std::string &text, bool possible )
{
    add( { id, false }, name_display, text, false, possible );
}
void mission_data::add( const ui_mission_id &id, const std::string &name_display,
                        const std::string &text,
                        bool priority, bool possible )
{
    Character &player_character = get_player_character();
    std::optional<basecamp *> bcp = overmap_buffer.find_camp(
                                        player_character.global_omt_location().xy() );
    if( bcp.has_value() && bcp.value()->is_hidden( id ) ) {
        return;
    }

    mission_entry miss;
    miss.id = id;
    if( name_display.empty() ) {  //  Poorly designed if this is the case. Do it properly...
        miss.name_display = miss_info[id.id.id].serialize_id;
    } else {
        miss.name_display = name_display;
    }
    miss.text = text;
    miss.priority = priority;
    miss.possible = possible;

    if( priority ) {
        entries[0].push_back( miss );
    }
    if( !possible ) {
        entries[10].push_back( miss );
    }
    const point direction = id.id.dir ? *id.id.dir : base_camps::base_dir;
    const int tab_order = base_camps::all_directions.at( direction ).tab_order;
    entries[tab_order + 1].emplace_back( miss );
}
