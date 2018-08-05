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
#include "requirements.h"
#include "string_input_popup.h"
#include "line.h"
#include "recipe_groups.h"

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

static const trait_id trait_NPC_CONSTRUCTION_LEV_1( "NPC_CONSTRUCTION_LEV_1" );
static const trait_id trait_NPC_CONSTRUCTION_LEV_2( "NPC_CONSTRUCTION_LEV_2" );
static const trait_id trait_NPC_MISSION_LEV_1( "NPC_MISSION_LEV_1" );

static const int COMPANION_SORT_POINTS = 12;

struct comp_rank {
  int industry;
  int combat;
  int survival;
};

struct mission_entry {
    std::string id;
    std::string name_display;
    std::string dir;
    bool priority;
    bool possible;
};

void talk_function::bionic_install(npc &p)
{
    std::vector<item *> bionic_inv = g->u.items_with( []( const item &itm ) {
        return itm.is_bionic();
    } );
    if( bionic_inv.empty() ) {
        popup(_("You have no bionics to install!"));
        return;
    }

    std::vector<itype_id> bionic_types;
    std::vector<std::string> bionic_names;
    for( auto &bio : bionic_inv ) {
        if( std::find( bionic_types.begin(), bionic_types.end(), bio->typeId() ) == bionic_types.end() ) {
            if (!g->u.has_bionic( bionic_id( bio->typeId() ) ) || bio->typeId() ==  "bio_power_storage" ||
                bio->typeId() ==  "bio_power_storage_mkII"){

                bionic_types.push_back( bio->typeId() );
                bionic_names.push_back( bio->tname() +" - $"+to_string(bio->price( true )*2/100));
            }
        }
    }
    // Choose bionic if applicable
    int bionic_index = 0;
    bionic_names.push_back(_("Cancel"));
    bionic_index = menu_vec(false, _("Which bionic do you wish to have installed?"),
                          bionic_names) - 1;
    if (bionic_index == (int)bionic_names.size() - 1) {
        bionic_index = -1;
    }
    // Did we cancel?
    if (bionic_index < 0) {
        popup(_("You decide to hold off..."));
        return;
    }

    const item tmp = item(bionic_types[bionic_index], 0);
    const itype &it = *tmp.type;
    unsigned int price = tmp.price( true )*2;

    if (price > g->u.cash){
        popup(_("You can't afford the procedure..."));
        return;
    }

    //Makes the doctor awesome at installing but not perfect
    if ( g->u.install_bionics( it, 20, false ) ){
        g->u.cash -= price;
        p.cash += price;
        g->u.amount_of( bionic_types[bionic_index] );
    }
}

void talk_function::bionic_remove(npc &p)
{
    bionic_collection all_bio = *g->u.my_bionics;
    if( all_bio.empty() ) {
        popup(_("You don't have any bionics installed..."));
        return;
    }

    item tmp;
    std::vector<itype_id> bionic_types;
    std::vector<std::string> bionic_names;
    for( auto &bio : all_bio ) {
        if( std::find( bionic_types.begin(), bionic_types.end(), bio.id.str() ) == bionic_types.end() ) {
            if (bio.id != bionic_id( "bio_power_storage" ) || bio.id != bionic_id( "bio_power_storage_mkII" ) ) {
                bionic_types.push_back( bio.id.str() );
                if( item::type_is_defined( bio.id.str() ) ) {
                    tmp = item(bio.id.str(), 0);
                    bionic_names.push_back( tmp.tname() +" - $"+to_string(500+(tmp.price( true )/400)));
                } else {
                    bionic_names.push_back( bio.id.str() +" - $"+to_string(500));
                }
            }
        }
    }
    // Choose bionic if applicable
    int bionic_index = 0;
    bionic_names.push_back(_("Cancel"));
    bionic_index = menu_vec(false, _("Which bionic do you wish to uninstall?"),
                          bionic_names) - 1;
    if (bionic_index == (int)bionic_names.size() - 1) {
        bionic_index = -1;
    }
    // Did we cancel?
    if (bionic_index < 0) {
        popup(_("You decide to hold off..."));
        return;
    }

    unsigned int price;
    if( item::type_is_defined( bionic_types[bionic_index] ) ) {
        price = 50000+(item(bionic_types[bionic_index], 0).price( true )/4);
    } else {
        price = 50000;
    }
    if (price > g->u.cash){
        popup(_("You can't afford the procedure..."));
        return;
    }

    //Makes the doctor awesome at installing but not perfect
    if (g->u.uninstall_bionic(bionic_id( bionic_types[bionic_index] ), 20, false)){
        g->u.cash -= price;
        p.cash += price;
        g->u.amount_of( bionic_types[bionic_index] ); // ??? this does nothing, it just queries the count
    }

}

void talk_function::companion_mission(npc &p)
{
    std::string id = p.companion_mission_role_id;
    std::string title = _("Outpost Missions");
    if( id == "FACTION_CAMP" ){
        title = _("Base Missions");
    } else if( id == "SCAVENGER" ){
        title = _("Junk Shop Missions");
    } else if( id == "COMMUNE CROPS" ){
        title = _("Agricultural Missions");
    } else if( id == "FOREMAN" ){
        title = _("Construction Missions");
    } else if( id == "REFUGEE MERCHANT" ){
        title = _("Free Merchant Missions");
    }
    talk_function::outpost_missions( p, id, title );
}

bool talk_function::outpost_missions( npc &p, const std::string &id, const std::string &title )
{
    //see mission_key_push() for each key description
    std::vector<std::vector<mission_entry>> mission_key_vectors;
    for( int tab_num = TAB_MAIN; tab_num != TAB_NW + 3; tab_num++ ){
        std::vector<mission_entry> k;
        mission_key_vectors.push_back( k );
    }

    std::map<std::string, std::string> col_missions;
    std::vector<std::shared_ptr<npc>> npc_list;
    std::string entry;
    std::string entry_aux;
    camp_tab_mode tab_mode = TAB_MAIN;

    if (id == "SCAVENGER"){
        col_missions["Assign Scavenging Patrol"] = _("Profit: $25-$500\nDanger: Low\nTime: 10 hour missions\n \n"
            "Assigning one of your allies to patrol the surrounding wilderness and isolated buildings presents "
            "the opportunity to build survival skills while engaging in relatively safe combat against isolated "
            "creatures.");
        mission_key_push( mission_key_vectors, "Assign Scavenging Patrol", _("Assign Scavenging Patrol") );
        npc_list = companion_list( p, "_scavenging_patrol" );
        if (npc_list.size()>0){
            entry = _("Profit: $25-$500\nDanger: Low\nTime: 10 hour missions\n \nPatrol Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Retrieve Scavenging Patrol"] = entry;
            mission_key_push( mission_key_vectors, "Retrieve Scavenging Patrol", _("Retrieve Scavenging Patrol") );
        }
    }

    if ( id == "SCAVENGER" && p.has_trait( trait_NPC_MISSION_LEV_1 ) ){
        col_missions["Assign Scavenging Raid"] = _("Profit: $200-$1000\nDanger: Medium\nTime: 10 hour missions\n \n"
            "Scavenging raids target formerly populated areas to loot as many valuable items as possible before "
            "being surrounded by the undead.  Combat is to be expected and assistance from the rest of the party "
            "can't be guaranteed.  The rewards are greater and there is a chance of the companion bringing back items.");
        mission_key_push( mission_key_vectors, "Assign Scavenging Raid", _("Assign Scavenging Raid") );
        npc_list = companion_list( p, "_scavenging_raid" );
        if (npc_list.size()>0){
            entry = _("Profit: $200-$1000\nDanger: Medium\nTime: 10 hour missions\n \nRaid Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Retrieve Scavenging Raid"] = entry;
            mission_key_push( mission_key_vectors, "Retrieve Scavenging Raid", _("Retrieve Scavenging Raid") );
        }
    }

    if (id == "FOREMAN"){
        col_missions["Assign Ally to Menial Labor"] = _("Profit: $8/hour\nDanger: Minimal\nTime: 1 hour minimum\n \n"
            "Assigning one of your allies to menial labor is a safe way to teach them basic skills and build "
            "reputation with the outpost.  Don't expect much of a reward though.");
        mission_key_push( mission_key_vectors, "Assign Ally to Menial Labor", _("Assign Ally to Menial Labor") );
        npc_list = companion_list( p, "_labor" );
        if (npc_list.size()>0){
            entry = _("Profit: $8/hour\nDanger: Minimal\nTime: 1 hour minimum\n \nLabor Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Ally from Menial Labor"] = entry;
            mission_key_push( mission_key_vectors, "Recover Ally from Menial Labor", _("Recover Ally from Menial Labor") );
        }
    }

    if ( id == "FOREMAN" && p.has_trait( trait_NPC_MISSION_LEV_1 ) ){
        col_missions["Assign Ally to Carpentry Work"] = _("Profit: $12/hour\nDanger: Minimal\nTime: 1 hour minimum\n \n"
            "Carpentry work requires more skill than menial labor while offering modestly improved pay.  It is "
            "unlikely that your companions will face combat but there are hazards working on makeshift buildings.");
        mission_key_push( mission_key_vectors, "Assign Ally to Carpentry Work", _("Assign Ally to Carpentry Work") );
        npc_list = companion_list( p, "_carpenter" );
        if (npc_list.size()>0){
            entry = _("Profit: $12/hour\nDanger: Minimal\nTime: 1 hour minimum\n \nLabor Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Ally from Carpentry Work"] = entry;
            mission_key_push( mission_key_vectors, "Recover Ally from Carpentry Work", _("Recover Ally from Carpentry Work") );
        }
    }

    //Used to determine what kind of OM the NPC is sitting in to determine the missions and upgrades
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, p.posz() );
    std::string om_cur = omt_ref.id().c_str();
    std::string bldg;
    std::vector<std::pair<std::string, tripoint>> om_expansions = om_building_region( p, 1, true );

    if( id == "FACTION_CAMP" ){
        bldg = om_next_upgrade(om_cur);

        if (bldg != "null"){
            col_missions["Upgrade Camp"] = om_upgrade_description( bldg );
            bool avail = companion_list( p, "_faction_upgrade_camp" ).empty();
            mission_key_push( mission_key_vectors, "Upgrade Camp", _("Upgrade Camp"), "", false, avail );
        }
    }

    if( id == "FACTION_CAMP" && om_min_level("faction_base_camp_1", om_cur ) ){
        col_missions["Gather Materials"] = om_gathering_description( p, bldg);
        bool avail = ( companion_list( p, "_faction_camp_gathering" ).size() < 3 );
        mission_key_push( mission_key_vectors, "Gather Materials", _("Gather Materials"), "", false, avail );


        col_missions["Distribute Food"] = string_format( "Notes:\n"
            "Distribute food to your follower and fill you larders.  Place the food you wish to distribute opposite the tent door between "
            "the manager and wall.\n \n"
            "Effects:\n"
            "> Increases your faction's food supply value which in turn is used to pay laborers for their time\n \n"
            "Must have enjoyability >= -6\n"
            "Perishable food liquidated at penalty depending on upgrades and rot time:\n"
            "> Rotten: 0%%\n"
            "> Rots in < 2 days: 60%%\n"
            "> Rots in < 5 days: 80%%\n \n"
            "Total faction food stock: %d kcal or %d day's rations", 10 * camp_food_supply(), camp_food_supply( 0, true ) );
        mission_key_push( mission_key_vectors, "Distribute Food", _("Distribute Food") );

        col_missions["Reset Sort Points"] = string_format("Notes:\n"
            "Resest the points that items are sorted to using the [ Menial Labor ] mission.\n \n"
            "Effects:\n"
            "> Assignable Points: food, food for distribution, seeds, weapons, clothing, bionics, "
            "all kinds of tools, wood, trash, books, medication, and ammo.\n"
            "> Items sitting on any type of furniture will not be moved.\n"
            "> Items that are not listed in one of the categories are defaulted to the tools group."
            );
        mission_key_push( mission_key_vectors, "Reset Sort Points", _("Reset Sort Points"), "", false );
    }

    if ( id == "FACTION_CAMP" && om_min_level("faction_base_camp_2", om_cur)){
        col_missions["Collect Firewood"] = string_format("Notes:\n"
            "Send a companion to gather light brush and heavy sticks.\n \n"
            "Skill used: survival\n"
            "Difficulty: N/A \n"
            "Gathering Possibilities:\n"
            "> heavy sticks\n"
            "> withered plants\n"
            "> splintered wood\n \n"
            "Risk: Very Low\n"
            "Time: 3 Hours, Repeated\n"
            "Positions: %d/3\n", companion_list( p, "_faction_camp_firewood" ).size()
            );
        bool avail = ( companion_list( p, "_faction_camp_firewood" ).size() < 3 );
        mission_key_push( mission_key_vectors, "Collect Firewood", _("Collect Firewood"), "", false, avail );
    }

    if ( id == "FACTION_CAMP" && om_min_level("faction_base_camp_3", om_cur)){
        col_missions["Menial Labor"] = string_format("Notes:\n"
            "Send a companion to do low level chores and sort supplies.\n \n"
            "Skill used: fabrication\n"
            "Difficulty: N/A \n"
            "Effects:\n"
            "> Material left outside on the ground will be sorted into the four crates in front of the tent.\n"
            "Default, top to bottom:  Clothing, Food, Books/Bionics, and Tools.  Wood will be piled to the south.  Trash to the north.\n \n"
            "Risk: None\n"
            "Time: 3 Hours\n"
            "Positions: %d/1\n", companion_list( p, "_faction_camp_menial" ).size()
            );
        bool avail = companion_list( p, "_faction_camp_menial" ).empty();
        mission_key_push( mission_key_vectors, "Menial Labor", _("Menial Labor"), "", false, avail );
    }

    if ( id == "FACTION_CAMP" && ( (om_min_level("faction_base_camp_4", om_cur) && om_expansions.empty()) ||
        (om_min_level("faction_base_camp_6", om_cur) && om_expansions.size() < 2) ||
        (om_min_level("faction_base_camp_8", om_cur) && om_expansions.size() < 3) ||
        (om_min_level("faction_base_camp_10", om_cur) && om_expansions.size() < 4) ||
        (om_min_level("faction_base_camp_12", om_cur) && om_expansions.size() < 5) ||
        (om_min_level("faction_base_camp_14", om_cur) && om_expansions.size() < 6) ||
        (om_min_level("faction_base_camp_16", om_cur) && om_expansions.size() < 7) ||
        (om_min_level("faction_base_camp_18", om_cur) && om_expansions.size() < 8) ) ){
        col_missions["Expand Base"] = string_format("Notes:\n"
            "Your base has become large enough to support an expansion.  Expansions open up new opportunities "
            "but can be expensive and time consuming.  Pick them carefully, only 8 can be built at each camp.\n \n"
            "Skill used: fabrication\n"
            "Difficulty: N/A \n"
            "Effects:\n"
            "> Choose any one of the available expansions.  Starting with a farm or lumberyard are always a solid choice "
            "since food is used to support companion missions and wood is your primary construction material.\n \n"
            "Risk: None\n"
            "Time: 3 Hours \n"
            "Positions: %d/1\n", companion_list( p, "_faction_camp_expansion" ).size()
            );
        bool avail = companion_list( p, "_faction_camp_expansion" ).empty();
        mission_key_push( mission_key_vectors, "Expand Base", _("Expand Base"), "", false, avail );
    }

    if ( id == "FACTION_CAMP" && om_min_level("faction_base_camp_5", om_cur)){
        col_missions["Cut Logs"] = string_format("Notes:\n"
            "Send a companion to a nearby forest to cut logs.\n \n"
            "Skill used: fabrication\n"
            "Difficulty: 1 \n"
            "Effects:\n"
            "> 50%% of trees/trunks at the forest position will be cut down.\n"
            "> 50%% of total material will be brought back.\n"
            "> Repeatable with diminishing returns.\n \n"
            "Risk: Low-Medium\n"
            "Time: 6 Hour Base + Travel Time + Cutting Time\n"
            "Positions: %d/1\n", companion_list( p, "_faction_camp_cut_log" ).size()
            );
        bool avail = companion_list( p, "_faction_camp_cut_log" ).empty();
        mission_key_push( mission_key_vectors, "Cut Logs", _("Cut Logs"), "", false, avail );
    }

    if ( id == "FACTION_CAMP" && om_min_level("faction_base_camp_7", om_cur) ){
        col_missions["Setup Hide Site"] = string_format("Notes:\n"
            "Send a companion to build an improvised shelter and stock it with equipment at a distant map location.\n \n"
            "Skill used: survival\n"
            "Difficulty: 3\n"
            "Effects:\n"
            "> Good for setting up resupply or contingency points.\n"
            "> Gear is left unattended and could be stolen.\n"
            "> Time dependent on weight of equipment being sent forward.\n \n"
            "Risk: Medium\n"
            "Time: 6 Hour Construction + Travel\n"
            "Positions: %d/1\n", companion_list( p, "_faction_camp_hide_site" ).size()
            );
        bool avail = companion_list( p, "_faction_camp_hide_site" ).empty();
        mission_key_push( mission_key_vectors, "Setup Hide Site", _("Setup Hide Site"), "", false, avail );
    }

    if ( id == "FACTION_CAMP" && om_min_level("faction_base_camp_7", om_cur) ){
        col_missions["Relay Hide Site"] = string_format("Notes:\n"
            "Push gear out to a hide site or bring gear back from one.\n \n"
            "Skill used: survival\n"
            "Difficulty: 1\n"
            "Effects:\n"
            "> Good for returning equipment you left in the hide site shelter.\n"
            "> Gear is left unattended and could be stolen.\n"
            "> Time dependent on weight of equipment being sent forward or back.\n \n"
            "Risk: Medium\n"
            "Time: 1 Hour Base + Travel\n"
            "Positions: %d/1\n", companion_list( p, "_faction_camp_hide_trans" ).size()
            );
        bool avail = companion_list( p, "_faction_camp_hide_trans" ).empty();
        mission_key_push( mission_key_vectors, "Relay Hide Site", _("Relay Hide Site"), "", false, avail );
    }

    if ( id == "FACTION_CAMP" && om_min_level("faction_base_camp_9", om_cur) ){
        col_missions["Construct Map Fortifications"] = om_upgrade_description( "faction_wall_level_N_0" );
        mission_key_push( mission_key_vectors, "Construct Map Fortifications", _("Construct Map Fortifications"), "", false );
        col_missions["Construct Spiked Trench"] = om_upgrade_description( "faction_wall_level_N_1" );
        mission_key_push( mission_key_vectors, "Construct Spiked Trench", _("Construct Spiked Trench"), "", false );
    }

    //This handles all crafting by the base, regardless of level
    std::map<std::string,std::string> craft_r = camp_recipe_deck( om_cur );
    inventory found_inv = g->u.crafting_inventory();
    std::string dr = "[B]";
    if( companion_list( p, "_faction_camp_crafting_"+dr ).empty() ){
        for( std::map<std::string,std::string>::const_iterator it = craft_r.begin(); it != craft_r.end(); ++it ) {
            std::string title_e = dr + it->first;
            col_missions[title_e] = om_craft_description( it->second );

            const recipe *recp = &recipe_id( it->second ).obj();
            bool craftable = recp->requirements().can_make_with_inventory( found_inv, 1 );

            mission_key_push( mission_key_vectors, title_e, "", dr, false, craftable  );

        }
    }

    if ( id == "FACTION_CAMP" && om_min_level("faction_base_camp_11", om_cur) ){
        col_missions["Recruit Companions"] = camp_recruit_evaluation( p, om_cur, om_expansions );
        bool avail = companion_list( p, "_faction_camp_recruit_0" ).empty();
        mission_key_push( mission_key_vectors, "Recruit Companions", _("Recruit Companions"), "", false, avail );
    }

    if ( id == "FACTION_CAMP" && om_min_level("faction_base_camp_13", om_cur) ){
        col_missions["Scout Mission"] =  string_format("Notes:\n"
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
            "Positions: %d/3\n", companion_list( p, "_faction_camp_scout_0" ).size()
            );
        bool avail = companion_list( p, "_faction_camp_scout_0" ).size() < 3;
        mission_key_push( mission_key_vectors, "Scout Mission", _("Scout Mission"), "", false, avail );
    }

    if ( id == "FACTION_CAMP" && om_min_level("faction_base_camp_15", om_cur) ){
        col_missions["Combat Patrol"] =  string_format("Notes:\n"
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
            "Positions: %d/3\n", companion_list( p, "_faction_camp_combat_0" ).size()
            );
        bool avail = companion_list( p, "_faction_camp_combat_0" ).size() < 3;
        mission_key_push( mission_key_vectors, "Combat Patrol", _("Combat Patrol"), "", false, avail );
    }

    //This starts all of the expansion missions
    if ( id == "FACTION_CAMP" ){
        if( !om_expansions.empty() ){
            for( const auto &e : om_expansions ){
                std::string dr = om_simple_dir( omt_pos, e.second );

                oter_id &omt_ref_exp = overmap_buffer.ter( e.second.x, e.second.y, p.posz() );
                std::string om_cur_exp = omt_ref_exp.id().c_str();
                std::string bldg_exp;

                bldg_exp = om_next_upgrade(om_cur_exp);

                if( bldg_exp != "null" ){
                    std::string title_e = dr+" Expansion Upgrade";

                    col_missions[title_e] = om_upgrade_description( bldg_exp );
                    mission_key_push( mission_key_vectors, title_e, "", dr );
                }

                if( om_min_level("faction_base_garage_1", om_cur_exp) ){
                    std::string title_e = dr+" Chop Shop";
                    col_missions[title_e] = _( "Notes:\n"
                        "Have a companion attempt to completely dissemble a vehicle into components.\n \n"
                        "Skill used: mechanics\n"
                        "Difficulty: 2 \n"
                        "Effects:\n"
                        "> Removed parts placed on the furniture in the garage.\n"
                        "> Skill plays a huge role to determine what is salvaged.\n \n"
                        "Risk: None\n"
                        "Time: Skill Based \n");
                    bool avail = companion_list( p, "_faction_exp_chop_shop_"+dr ).empty();
                    mission_key_push( mission_key_vectors, title_e, dr + _(" Chop Shop"), dr, false, avail );
                }

                std::map<std::string,std::string> cooking_recipes = camp_recipe_deck( om_cur_exp );
                if( om_min_level("faction_base_kitchen_1", om_cur_exp) ){
                    inventory found_inv = g->u.crafting_inventory();
                    if( companion_list( p, "_faction_exp_kitchen_cooking_"+dr ).empty() ){
                        for( std::map<std::string,std::string>::const_iterator it = cooking_recipes.begin(); it != cooking_recipes.end(); ++it ) {
                            std::string title_e = dr + it->first;
                            col_missions[title_e] = om_craft_description( it->second );
                            const recipe *recp = &recipe_id( it->second ).obj();
                            bool craftable = recp->requirements().can_make_with_inventory( found_inv, 1 );
                            mission_key_push( mission_key_vectors, title_e, "", dr, false, craftable  );

                        }
                    }
                }

                if( om_min_level("faction_base_blacksmith_1", om_cur_exp) ){
                    inventory found_inv = g->u.crafting_inventory();
                    if( companion_list( p, "_faction_exp_blacksmith_crafting_"+dr ).empty() ){
                        for( std::map<std::string,std::string>::const_iterator it = cooking_recipes.begin(); it != cooking_recipes.end(); ++it ) {
                            std::string title_e = dr + it->first;
                            col_missions[title_e] = om_craft_description( it->second );
                            const recipe *recp = &recipe_id( it->second ).obj();
                            bool craftable = recp->requirements().can_make_with_inventory( found_inv, 1 );
                            mission_key_push( mission_key_vectors, title_e, "", dr, false, craftable  );

                        }
                    }
                }

                if( om_min_level("faction_base_farm_1", om_cur_exp) ){
                    if( companion_list( p, "_faction_exp_plow_"+dr ).empty() ){
                        std::string title_e = dr+" Plow Fields";
                        col_missions[title_e] = _( "Notes:\n"
                            "Plow any spaces that have reverted to dirt or grass.\n \n") +
                            camp_farm_description( point(e.second.x, e.second.y), false, false, true) + _("\n \n"
                            "Skill used: fabrication\n"
                            "Difficulty: N/A \n"
                            "Effects:\n"
                            "> Restores only the plots created in the last expansion upgrade.\n"
                            "> Does not damage existing crops.\n \n"
                            "Risk: None\n"
                            "Time: 5 Min / Plot \n"
                            "Positions: 0/1 \n" );
                        mission_key_push( mission_key_vectors, title_e, dr + _(" Plow Fields"), dr );
                    }

                    if( companion_list( p, "_faction_exp_plant_"+dr ).empty() && g->get_temperature( g-> u.pos() ) > 50 ){
                        std::string title_e = dr+" Plant Fields";
                        col_missions[title_e] = _( "Notes:\n"
                            "Plant designated seeds in the spaces that have already been tilled.\n \n") +
                            camp_farm_description( point(e.second.x, e.second.y), false, true, false) + _("\n \n"
                            "Skill used: survival\n"
                            "Difficulty: N/A \n"
                            "Effects:\n"
                            "> Choose which seed type or all of your seeds.\n"
                            "> Stops when out of seeds or planting locations.\n"
                            "> Will plant in ALL dirt mounds in the expansion.\n \n"
                            "Risk: None\n"
                            "Time: 1 Min / Plot \n"
                            "Positions: 0/1 \n" );
                        mission_key_push( mission_key_vectors, title_e, dr + _(" Plant Fields"), dr );
                    }
                    if( companion_list( p, "_faction_exp_harvest_"+dr ).empty() ){
                        std::string title_e = dr+" Harvest Fields";
                        col_missions[title_e] = _( "Notes:\n"
                            "Harvest any plants that are ripe and bring the produce back.\n \n") +
                            camp_farm_description( point(e.second.x, e.second.y), true, false, false) + _("\n \n"
                            "Skill used: survival\n"
                            "Difficulty: N/A \n"
                            "Effects:\n"
                            "> Will dump all harvesting products onto your location.\n \n"
                            "Risk: None\n"
                            "Time: 3 Min / Plot \n"
                            "Positions: 0/1 \n" );
                        mission_key_push( mission_key_vectors, title_e, dr + _(" Harvest Fields"), dr );
                    }
                }

                if( om_min_level("faction_base_farm_4", om_cur_exp) ){
                    inventory found_inv = g->u.crafting_inventory();
                    if( companion_list( p, "_faction_exp_farm_crafting_"+dr ).empty() ){
                        for( std::map<std::string,std::string>::const_iterator it = cooking_recipes.begin(); it != cooking_recipes.end(); ++it ) {
                            std::string title_e = dr + it->first;
                            col_missions[title_e] = om_craft_description( it->second );

                            const recipe *recp = &recipe_id( it->second ).obj();
                            bool craftable = recp->requirements().can_make_with_inventory( found_inv, 1 );

                            mission_key_push( mission_key_vectors, title_e, "", dr, false, craftable  );

                        }
                    }
                }

            }
        }

        npc_list = companion_list( p, "_faction_upgrade_camp" );
        if( !npc_list.empty() ){
            entry = _("Working to expand your camp!\n");
            for( auto &elem : npc_list ) {
                int hour_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
                int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
                if( hour_left > 1 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( hour_left )
                        + " hours left] \n";
                } else if( min_left > 0 ) {
                    entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + " minutes left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Ally from Upgrading"] = entry;
            mission_key_push( mission_key_vectors, "Recover Ally from Upgrading", _("Recover Ally from Upgrading"), "", true );
        }

        npc_list = companion_list( p, "_faction_upgrade_exp_", true );
        if( !npc_list.empty() ){
            for( auto &elem : npc_list ) {
                const recipe *making_exp;
                entry = _("Working to upgrade your expansions!\n");
                //To find what building we want to create next, get direction it is from camp
                std::string dir = camp_direction( elem->get_companion_mission().mission_id );
                std::string bldg_exp;
                for( const auto &e : om_expansions ){
                    //Find the expansion that is in that direction
                    if( dir == om_simple_dir( omt_pos, e.second ) ) {
                        oter_id &omt_ref_exp = overmap_buffer.ter( e.second.x, e.second.y, e.second.z );
                        std::string om_exp = omt_ref_exp.id().c_str();
                        //Determine the next upgrade for the building
                        bldg_exp = om_next_upgrade(om_exp);
                        break;
                    }
                }
                making_exp = &recipe_id( bldg_exp ).obj();
                entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + "/" + to_string( to_hours<int>( time_duration::from_turns( making_exp->time / 100 ) ) )+ " hours] \n";

                entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
                col_missions["Recover Ally, "+dir+" Expansion"] = entry;
                mission_key_push( mission_key_vectors, "Recover Ally, "+dir+" Expansion", _("Recover Ally, ") + dir + _(" Expansion"), dir, true );
            }
        }

        npc_list = companion_list( p, "_faction_exp_chop_shop_", true );
        if( !npc_list.empty() ){
            for( auto &elem : npc_list ) {
                entry = _("Working at the chop shop...\n");
                //~48 hours = 192 plots * 5 min plow time
                entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + "/120 hours] \n";
                entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
                std::string dir = camp_direction(elem->get_companion_mission().mission_id );
                col_missions[ dir + " (Finish) Chop Shop" ] = entry;
                mission_key_push( mission_key_vectors, dir + " (Finish) Chop Shop", "", dir, true );
            }
        }

        npc_list = companion_list( p, "_faction_exp_kitchen_cooking_", true );
        if( !npc_list.empty() ){
            for( auto &elem : npc_list ) {
                entry = _("Working in your kitchen!\n");
                int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
                float sec_left = to_turns<float>( elem->companion_mission_time_ret - calendar::turn );
                if( min_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + " minutes left] \n";
                } else if( sec_left > 0 ) {
                    entry = entry + "  " + elem->name + " [ALMOST DONE]\n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
                entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
                std::string dir = camp_direction( elem->get_companion_mission().mission_id );
                col_missions[ dir + " (Finish) Cooking" ] = entry;
                mission_key_push( mission_key_vectors, dir + " (Finish) Cooking", "", dir, true );
            }
        }

        npc_list = companion_list( p, "_faction_exp_blacksmith_crafting_", true );
        if( !npc_list.empty() ){
            for( auto &elem : npc_list ) {
                entry = _("Working in your blacksmith shop!\n");
                int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
                float sec_left = to_turns<float>( elem->companion_mission_time_ret - calendar::turn );
                if( min_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + " minutes left] \n";
                } else if( sec_left > 0 ) {
                    entry = entry + "  " + elem->name + " [ALMOST DONE]\n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
                entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
                std::string dir = camp_direction( elem->get_companion_mission().mission_id );
                col_missions[ dir + " (Finish) Smithing" ] = entry;
                mission_key_push( mission_key_vectors, dir + " (Finish) Smithing", "", dir, true );
            }
        }

        npc_list = companion_list( p, "_faction_exp_plow_", true );
        if( !npc_list.empty() ){
            for( auto &elem : npc_list ) {
                entry = _("Working to plow your fields!\n");
                //~48 hours = 192 plots * 5 min plow time
                entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + "/~48 hours] \n";
                entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
                std::string dir = camp_direction( elem->get_companion_mission().mission_id );
                col_missions[ dir + " (Finish) Plow Fields" ] = entry;
                mission_key_push( mission_key_vectors, dir + " (Finish) Plow Fields", "", dir, true );
            }
        }

        npc_list = companion_list( p, "_faction_exp_plant_", true );
        if( !npc_list.empty() ){
            for( auto &elem : npc_list ) {
                entry = _("Working to plant your fields!\n");
                //~3.5 hours = 192 plots * 1 min plant time
                entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + "/4 hours] \n";
                entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
                std::string dir = camp_direction( elem->get_companion_mission().mission_id );
                col_missions[ dir + " (Finish) Plant Fields" ] = entry;
                mission_key_push( mission_key_vectors, dir + " (Finish) Plant Fields", "", dir, true );
            }
        }

        npc_list = companion_list( p, "_faction_exp_harvest_", true );
        if( !npc_list.empty() ){
            for( auto &elem : npc_list ) {
                entry = _("Working to harvest your fields!\n");
                //~48 hours = 192 plots * 3 min harvest and carry time
                entry = entry + "  " + elem->name + " ["
                    + to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) )
                    + "/~10 hours] \n";
                entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
                std::string dir = camp_direction( elem->get_companion_mission().mission_id );
                col_missions[ dir + " (Finish) Harvest Fields" ] = entry;
                mission_key_push( mission_key_vectors, dir + " (Finish) Harvest Fields", "", dir, true );
            }
        }


        npc_list = companion_list( p, "_faction_exp_farm_crafting_", true );
        if( !npc_list.empty() ){
            for( auto &elem : npc_list ) {
                entry = _("Working on your farm!\n");
                int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
                if( min_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + " minutes left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
                entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
                std::string dir = camp_direction( elem->get_companion_mission().mission_id );
                col_missions[ dir + " (Finish) Crafting" ] = entry;
                mission_key_push( mission_key_vectors, dir + " (Finish) Crafting", "", dir, true );
            }
        }

        npc_list = companion_list( p, "_faction_camp_crafting_", true );
        if( !npc_list.empty() ){
            for( auto &elem : npc_list ) {
                entry = _("Busy crafting!\n");
                int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
                if( min_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + " minutes left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
                entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
                std::string dir = camp_direction( elem->get_companion_mission().mission_id );
                col_missions[ dir + " (Finish) Crafting" ] = entry;
                mission_key_push( mission_key_vectors, dir + " (Finish) Crafting", "", dir, true );
            }
        }

        npc_list = companion_list( p, "_faction_camp_gathering" );
        if( !npc_list.empty() ){
            entry = _("Searching for materials to upgrade the camp.\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +"/3 hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Ally from Gathering"] = entry;
            mission_key_push( mission_key_vectors, "Recover Ally from Gathering", _("Recover Ally from Gathering"), "", true );
        }
        npc_list = companion_list( p, "_faction_camp_firewood" );
        if( !npc_list.empty() ){
            entry = _("Searching for firewood.\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +"/3 hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Firewood Gatherers"] = entry;
            mission_key_push( mission_key_vectors, "Recover Firewood Gatherers", _("Recover Firewood Gatherers"), "", true );
        }
        npc_list = companion_list( p, "_faction_camp_menial" );
        if( !npc_list.empty() ){
            entry = _("Performing menial labor...\n");
            for( auto &elem : npc_list ) {
                int min_left = to_minutes<int>( elem->companion_mission_time_ret - calendar::turn );
                if( min_left > 0 ) {
                    entry = entry + "  " + elem->name + " ["
                        + to_string( min_left )
                        + " minutes left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?\n");
            col_missions["Recover Menial Laborer"] = entry;
            mission_key_push( mission_key_vectors, "Recover Menial Laborer", _("Recover Menial Laborer"), "", true );
        }
        npc_list = companion_list( p, "_faction_camp_expansion" );
        if( !npc_list.empty() ){
            entry = _("Surveying for expansion...\n");
            for( auto &elem : npc_list ) {
                int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
                if( hrs_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + " hours left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?\n");
            col_missions["Recover Surveyor"] = entry;
            mission_key_push( mission_key_vectors, "Recover Surveyor", _("Recover Surveyor"), "", true );
        }

        npc_list = companion_list( p, "_faction_camp_cut_log" );
        if( !npc_list.empty() ){
            entry = _("Cutting logs in the woods...\n");
            for( auto &elem : npc_list ) {
                int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
                if( hrs_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + " hours left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?\n");
            col_missions["Recover Log Cutter"] = entry;
            mission_key_push( mission_key_vectors, "Recover Log Cutter", _("Recover Log Cutter"), "", true );
        }

        npc_list = companion_list( p, "_faction_camp_hide_site" );
        if( !npc_list.empty() ){
            entry = _("Setting up a hide site...\n");
            for( auto &elem : npc_list ) {
                int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
                if( hrs_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + " hours left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?\n");
            col_missions["Recover Hide Setup"] = entry;
            mission_key_push( mission_key_vectors, "Recover Hide Setup", _("Recover Hide Setup"), "", true );
        }

        npc_list = companion_list( p, "_faction_camp_om_fortifications" );
        if( !npc_list.empty() ){
            entry = _("Constructing fortifications...\n");
            for( auto &elem : npc_list ) {
                int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
                if( hrs_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + " hours left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?\n");
            col_missions["Finish Map Fortifications"] = entry;
            mission_key_push( mission_key_vectors, "Finish Map Fortifications", _("Finish Map Fortifications"), "", true );
        }

        npc_list = companion_list( p, "_faction_camp_recruit_0" );
        if( !npc_list.empty() ){
            entry = _("Searching for recruits.\n");
            for( auto &elem : npc_list ) {
                int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
                if( hrs_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + " hours left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Recruiter"] = entry;
            mission_key_push( mission_key_vectors, "Recover Recruiter", _("Recover Recruiter"), "", true );
        }

        npc_list = companion_list( p, "_faction_camp_scout_0" );
        if( !npc_list.empty() ){
            entry = _("Scouting the region.\n");
            for( auto &elem : npc_list ) {
                int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
                if( hrs_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + " hours left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Scout"] = entry;
            mission_key_push( mission_key_vectors, "Recover Scout", _("Recover Scout"), "", true );
        }

        npc_list = companion_list( p, "_faction_camp_combat_0" );
        if( !npc_list.empty() ){
            entry = _("Patrolling the region.\n");
            for( auto &elem : npc_list ) {
                int hrs_left = to_hours<int>( elem->companion_mission_time_ret - calendar::turn );
                if( hrs_left > 0 ){
                    entry = entry + "  " + elem->name + " ["
                        + to_string( hrs_left )
                        + " hours left] \n";
                } else {
                    entry = entry + "  " + elem->name + " [DONE]\n";
                }
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Combat Patrol"] = entry;
            mission_key_push( mission_key_vectors, "Recover Combat Patrol", _("Recover Combat Patrol"), "", true );
        }

    }
    if ( id == "COMMUNE CROPS" && !p.has_trait( trait_NPC_CONSTRUCTION_LEV_1 ) ){
        col_missions["Purchase East Field"] = _("Cost: $1000\n \n"
            "\n              .........\n              .........\n              .........\n              "
            ".........\n              .........\n              .........\n              ..#....**\n     "
            "         ..#Ov..**\n              ...O|....\n \n"
            "We're willing to let you purchase a field at a substantial discount to use for your own agricultural "
            "enterprises.  We'll plow it for you so you know exactly what is yours... after you have a field "
            "you can hire workers to plant or harvest crops for you.  If the crop is something we have a "
            "demand for, we'll be willing to liquidate it.");
        mission_key_push( mission_key_vectors, "Purchase East Field", _("Purchase East Field") );
    }

    if ( id == "COMMUNE CROPS" && p.has_trait( trait_NPC_CONSTRUCTION_LEV_1 ) && !p.has_trait( trait_NPC_CONSTRUCTION_LEV_2 ) ){
        col_missions["Upgrade East Field I"] = _("Cost: $5500\n \n"
            "\n              .........\n              .........\n              .........\n              "
            ".........\n              .........\n              .........\n              ..#....**\n     "
            "         ..#Ov..**\n              ...O|....\n \n"
            "Protecting your field with a sturdy picket fence will keep most wildlife from nibbling your crops "
            "apart.  You can expect yields to increase.");
        mission_key_push( mission_key_vectors, "Upgrade East Field I", _("Upgrade East Field I") );
    }

    if ( id == "COMMUNE CROPS" && p.has_trait( trait_NPC_CONSTRUCTION_LEV_1 ) ){
        col_missions["Plant East Field"] = _("Cost: $3.00/plot\n \n"
        "\n              .........\n              .........\n              .........\n              .........\n"
        "              .........\n              .........\n              ..#....**\n              ..#Ov..**\n  "
        "            ...O|....\n \n"
        "We'll plant the field with your choice of crop if you are willing to finance it.  When the crop is ready "
        "to harvest you can have us liquidate it or harvest it for you.");
        mission_key_push( mission_key_vectors, "Plant East Field", _("Plant East Field") );
        col_missions["Harvest East Field"] = _("Cost: $2.00/plot\n \n"
        "\n              .........\n              .........\n              .........\n              .........\n"
        "              .........\n              .........\n              ..#....**\n              ..#Ov..**\n  "
        "            ...O|....\n \n"
        "You can either have us liquidate the crop and give you the cash or pay us to harvest it for you.");
        mission_key_push( mission_key_vectors, "Harvest East Field", _("Harvest East Field") );
    }

    if (id == "COMMUNE CROPS"){
        col_missions["Assign Ally to Forage for Food"] = _("Profit: $10/hour\nDanger: Low\nTime: 4 hour minimum\n \n"
            "Foraging for food involves dispatching a companion to search the surrounding wilderness for wild "
            "edibles.  Combat will be avoided but encounters with wild animals are to be expected.  The low pay is "
            "supplemented with the odd item as a reward for particularly large hauls.");
        mission_key_push( mission_key_vectors, "Assign Ally to Forage for Food", _("Assign Ally to Forage for Food") );
        npc_list = companion_list( p, "_forage" );
        if (npc_list.size()>0){
            entry = _("Profit: $10/hour\nDanger: Low\nTime: 4 hour minimum\n \nLabor Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Ally from Foraging"] = entry;
            mission_key_push( mission_key_vectors, "Recover Ally from Foraging", _("Recover Ally from Foraging") );
        }
    }

    if (id == "COMMUNE CROPS" || id == "REFUGEE MERCHANT"){
        col_missions["Caravan Commune-Refugee Center"] = _("Profit: $18/hour\nDanger: High\nTime: UNKNOWN\n \n"
            "Adding companions to the caravan team increases the likelihood of success.  By nature, caravans are "
            "extremely tempting targets for raiders or hostile groups so only a strong party is recommended.  The "
            "rewards are significant for those participating but are even more important for the factions that profit.\n \n"
            "The commune is sending food to the Free Merchants in the Refugee Center as part of a tax and in exchange "
            "for skilled labor.");
        mission_key_push( mission_key_vectors, "Caravan Commune-Refugee Center", _("Caravan Commune-Refugee Center") );
        npc_list = companion_list( p, "_commune_refugee_caravan" );
        std::vector<std::shared_ptr<npc>> npc_list_aux;
        if (npc_list.size()>0){
            entry = _("Profit: $18/hour\nDanger: High\nTime: UNKNOWN\n \n"
            " \nRoster:\n");
            for( auto &elem : npc_list ) {
                if( elem->companion_mission_time == calendar::before_time_starts ) {
                    entry = entry + "  " + elem->name + _(" [READY] \n");
                    npc_list_aux.push_back(elem);
                } else if( calendar::turn >= elem->companion_mission_time ) {
                    entry = entry + "  " + elem->name + _(" [COMPLETE] \n");
                } else {
                    entry = entry + "  " + elem->name + " ["+ to_string( abs( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) ) +" Hours] \n";
                }
            }
            if (npc_list_aux.size()>0){
                entry_aux = _("Profit: $18/hour\nDanger: High\nTime: UNKNOWN\n \n"
                " \nRoster:\n");
                for( auto &elem : npc_list_aux ) {
                    if( elem->companion_mission_time == calendar::before_time_starts ) {
                        entry_aux = entry_aux + "  " + elem->name + " [READY] \n";
                    }
                }
                entry_aux = entry_aux + _("\n \n"
                    "The caravan will contain two or three additional members from the commune, are you ready to depart?");
                col_missions["Begin Commune-Refugee Center Run"] = entry_aux;
                mission_key_push( mission_key_vectors, "Begin Commune-Refugee Center Run", _("Begin Commune-Refugee Center Run") );
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Commune-Refugee Center"] = entry;
            mission_key_push( mission_key_vectors, "Recover Commune-Refugee Center", _("Recover Commune-Refugee Center") );
        }
    }

    if (col_missions.empty()) {
        popup(_("There are no missions at this colony.  Press Spacebar..."));
        return false;
    }

    int TITLE_TAB_HEIGHT = 0;
    if( id == "FACTION_CAMP" ){
        TITLE_TAB_HEIGHT = 1;
    }

    catacurses::window w_list = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                            ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0) + TITLE_TAB_HEIGHT,
                            (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    catacurses::window w_info = catacurses::newwin( FULL_SCREEN_HEIGHT - 3, FULL_SCREEN_WIDTH - 1 - MAX_FAC_NAME_SIZE,
                            ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0) + TITLE_TAB_HEIGHT + 1,
                            MAX_FAC_NAME_SIZE + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0));
    catacurses::window w_tabs = catacurses::newwin( TITLE_TAB_HEIGHT, FULL_SCREEN_WIDTH,
                                                    ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0),
                                                   (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);

    int maxlength = FULL_SCREEN_WIDTH - 1 - MAX_FAC_NAME_SIZE;
    size_t sel = 0;
    int offset = 0;
    bool redraw = true;

    input_context ctxt("FACTIONS");
    ctxt.register_action("UP", _("Move cursor up"));
    ctxt.register_action("DOWN", _("Move cursor down"));
    ctxt.register_action( "NEXT_TAB" );
    ctxt.register_action( "PREV_TAB" );
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");
    mission_entry cur_key;
    auto cur_key_list = mission_key_vectors[0];
    for( auto k : mission_key_vectors[1] ){
        bool has = false;
        for( auto keys : cur_key_list ){
            if( k.id == keys.id ){
                has = true;
                break;
            }
        }
        if( !has ){
            cur_key_list.push_back(k);
        }
    }

    g->draw_ter();
    wrefresh( g->w_terrain );

    while (true) {
        if( cur_key_list.empty() ){
            mission_entry dud;
            dud.id = "NONE";
            dud.name_display = "NONE";
            cur_key_list.push_back( dud );
        }
        cur_key = cur_key_list[sel];
        if (redraw) {
            werase(w_list);
            draw_border(w_list);
            mvwprintz( w_list, 1, 1, c_white, name_mission_tabs( p, id, title, tab_mode ) );

            calcStartPos( offset, sel, FULL_SCREEN_HEIGHT - 3, cur_key_list.size() );

            for( size_t i = 0; ( int )i < FULL_SCREEN_HEIGHT - 3 && ( i + offset ) < cur_key_list.size(); i++ ) {
                size_t  current = i + offset;
                nc_color col = (current == sel ? h_white : c_white);
                //highlight important missions
                for( auto k: mission_key_vectors[0]  ){
                    if( cur_key_list[current].id == k.id ){
                        col = ( current == sel ? h_white : c_yellow );
                    }
                }
                //dull uncraftable items
                for( auto k: mission_key_vectors[10]  ){
                    if( cur_key_list[current].id == k.id ){
                        col = ( current == sel ? h_white : c_dark_gray );
                    }
                }
                mvwprintz(w_list, i + 2, 1, col, "  %s", cur_key_list[current].name_display.c_str());
            }

            draw_scrollbar( w_list, sel, FULL_SCREEN_HEIGHT - 2, cur_key_list.size(), 1);
            wrefresh(w_list);
            werase(w_info);
            fold_and_print(w_info, 0, 0, maxlength, c_white, col_missions[cur_key.id]);
            wrefresh(w_info);

            if( id == "FACTION_CAMP" ){
                werase(w_tabs);
                draw_camp_tabs( w_tabs, tab_mode, mission_key_vectors );
                wrefresh(w_tabs);
            }
            redraw = false;
        }
        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            mvwprintz(w_list, sel + 2, 1, c_white, "-%s", cur_key.id.c_str());
            if (sel == cur_key_list.size() - 1) {
                sel = 0;    // Wrap around
            } else {
                sel++;
            }
            redraw = true;
        } else if (action == "UP") {
            mvwprintz(w_list, sel + 2, 1, c_white, "-%s", cur_key.id.c_str());
            if (sel == 0) {
                sel = cur_key_list.size() - 1;    // Wrap around
            } else {
                sel--;
            }
            redraw = true;
        } else if( action == "NEXT_TAB" && id == "FACTION_CAMP") {
            redraw = true;
            sel = 0;
            offset = 0;
            for( int tab_num = TAB_MAIN; tab_num != TAB_NW; tab_num++ ){
                camp_tab_mode cur = static_cast<camp_tab_mode>(tab_num);
                if( tab_mode == TAB_NW ){
                    tab_mode = TAB_MAIN;
                    cur_key_list = mission_key_vectors[0];
                    for( auto k : mission_key_vectors[1] ){
                        bool has = false;
                        for( auto keys : cur_key_list ){
                            if( k.id == keys.id ){
                                has = true;
                                break;
                            }
                        }
                        if( !has ){
                            cur_key_list.push_back(k);
                        }
                    }

                    break;
                } else if( cur == tab_mode ){
                    cur_key_list = mission_key_vectors[tab_num + 2];
                    tab_mode = static_cast<camp_tab_mode>(tab_num + 1);
                    break;
                }
            }
        } else if( action == "PREV_TAB" && id == "FACTION_CAMP" ) {
            redraw = true;
            sel = 0;
            offset = 0;
            for( int tab_num = TAB_MAIN; tab_num != TAB_NW + 1; tab_num++ ){
                camp_tab_mode cur = static_cast<camp_tab_mode>(tab_num);
                if( tab_mode == TAB_MAIN ){
                    cur_key_list = mission_key_vectors[ TAB_NW + 1 ];
                    tab_mode = TAB_NW;
                    break;
                } else if( cur == tab_mode ){
                    tab_mode = static_cast<camp_tab_mode>(tab_num - 1);
                    if( tab_mode == TAB_MAIN ) {
                        cur_key_list = mission_key_vectors[0];
                        for( auto k : mission_key_vectors[1] ){
                            bool has = false;
                            for( auto keys : cur_key_list ){
                                if( k.id == keys.id ){
                                    has = true;
                                    break;
                                }
                            }
                            if( !has ){
                                cur_key_list.push_back(k);
                            }
                        }

                    } else {
                        cur_key_list = mission_key_vectors[ tab_num ];
                    }
                    break;
                }
            }
        } else if (action == "QUIT") {
            mission_entry dud;
            dud.id = "NONE";
            dud.name_display = "NONE";
            cur_key = dud;
            break;
        } else if (action == "CONFIRM") {
            break;
        } else if ( action == "HELP_KEYBINDINGS" ) {
            g->draw_ter();
            wrefresh( g->w_terrain );
        }
    }
    g->refresh_all();

    if (cur_key.id == "Caravan Commune-Refugee Center"){
        individual_mission(p, _("joins the caravan team..."), "_commune_refugee_caravan", true);
    }
    if (cur_key.id == "Begin Commune-Refugee Center Run"){
        caravan_depart(p, "evac_center_18", "_commune_refugee_caravan");
    }
    if (cur_key.id == "Recover Commune-Refugee Center"){
        caravan_return(p, "evac_center_18", "_commune_refugee_caravan");
    }
    if (cur_key.id == "Purchase East Field"){
        field_build_1(p);
    }
    if (cur_key.id == "Upgrade East Field I"){
        field_build_2(p);
    }
    if (cur_key.id == "Plant East Field"){
        field_plant(p, "ranch_camp_63");
    }
    if (cur_key.id == "Harvest East Field"){
        field_harvest(p, "ranch_camp_63");
    }
    if (cur_key.id == "Assign Scavenging Patrol"){
        individual_mission(p, _("departs on the scavenging patrol..."), "_scavenging_patrol");
    }
    if (cur_key.id == "Retrieve Scavenging Patrol"){
        scavenging_patrol_return(p);
    }
    if (cur_key.id == "Assign Scavenging Raid"){
        individual_mission(p, _("departs on the scavenging raid..."), "_scavenging_raid");
    }
    if (cur_key.id == "Retrieve Scavenging Raid"){
        scavenging_raid_return(p);
    }
    if (cur_key.id == "Assign Ally to Menial Labor"){
        individual_mission(p, _("departs to work as a laborer..."), "_labor");
    }
    if (cur_key.id == "Recover Ally from Menial Labor"){
        labor_return(p);
    }

    if (cur_key.id == "Upgrade Camp"){
        const recipe *making = &recipe_id( bldg ).obj();
         //Stop upgrade if you don't have materials
        inventory total_inv = g->u.crafting_inventory();
        if( making->requirements().can_make_with_inventory( total_inv, 1 ) ){
            std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_upgrade_camp" );
            int need_food = time_to_food ( time_duration::from_turns( making->time / 100 ) );
            if( camp_food_supply() < need_food && bldg != "faction_base_camp_1" ){
                popup( _("You don't have enough food stored to feed your companion.") );
            } else if (npc_list.empty() ) {
                npc* comp = individual_mission( p, _("begins to upgrade the camp..."), "_faction_upgrade_camp", false, {},
                                       making->skill_used.obj().ident().c_str(), making->difficulty );
                if( comp != nullptr ){
                    comp->companion_mission_time_ret = calendar::turn + time_duration::from_turns( making->time/ 100 ) ;
                    g->u.consume_components_for_craft( making, 1, true );
                    g->u.invalidate_crafting_inventory();
                    camp_food_supply( -need_food );
                }
            } else {
                popup( _("You already have a companion upgrading the camp.") );
            }
        } else {
            popup(_("You don't have the materials for the upgrade.") );
        }
    }

    if (cur_key.id == "Recover Ally from Upgrading" ){
        upgrade_return( p, omt_pos, "_faction_upgrade_camp" );
    }

    if ( cur_key.id == "Recover Ally, " + cur_key.dir + " Expansion" ){
        for( const auto &e : om_expansions ){
            //Find the expansion that is in that direction
            if( cur_key.dir  == om_simple_dir( omt_pos, e.second ) ) {
                upgrade_return( p, point(e.second.x, e.second.y), "_faction_upgrade_exp_" + cur_key.dir  );
                break;
            }
        }
    }

    if ( cur_key.id == cur_key.dir + " (Finish) Chop Shop" ){
        for( const auto &e : om_expansions ){
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                npc *comp = companion_choose_return( p, "_faction_exp_chop_shop_" + cur_key.dir, calendar::turn - 5_days );
                if (comp != nullptr){
                    popup(_("%s returns from your garage..."), comp->name.c_str());
                    companion_return( *comp );
                }
                break;
            }
        }
    }

    if ( cur_key.id == cur_key.dir + " (Finish) Cooking" ){
        for( const auto &e : om_expansions ){
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                npc *comp = companion_choose_return( p, "_faction_exp_kitchen_cooking_" + cur_key.dir, calendar::before_time_starts );
                if (comp != nullptr){
                    popup(_("%s returns from your kitchen with something..."), comp->name.c_str());
                    companion_return( *comp );
                }
                break;
            }
        }
    }

    if( cur_key.id == cur_key.dir + " (Finish) Smithing" ){
        for( const auto &e : om_expansions ){
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                npc *comp = companion_choose_return( p, "_faction_exp_blacksmith_crafting_" + cur_key.dir, calendar::before_time_starts );
                if (comp != nullptr){
                    popup(_("%s returns from your blacksmith shop with something..."), comp->name.c_str());
                    companion_return( *comp );
                }
                break;
            }
        }
    }

    if ( cur_key.id == cur_key.dir + " (Finish) Plow Fields" ){
        for( const auto &e : om_expansions ){
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                camp_farm_return( p, "_faction_exp_plow_" + cur_key.dir, false, false, true );
                break;
            }
        }
    }

    if ( cur_key.id == cur_key.dir + " (Finish) Plant Fields" ){
        for( const auto &e : om_expansions ){
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                camp_farm_return( p, "_faction_exp_plant_" + cur_key.dir, false, true, false );
                break;
            }
        }
    }

    if ( cur_key.id == cur_key.dir + " (Finish) Harvest Fields" ){
        for( const auto &e : om_expansions ){
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {
                camp_farm_return( p, "_faction_exp_harvest_" + cur_key.dir, true, false, false );
                break;
            }
        }
    }

    if ( cur_key.id == cur_key.dir + " (Finish) Crafting" ){
        for( const auto &e : om_expansions ){
            if( cur_key.dir == om_simple_dir( omt_pos, e.second ) ) {

                npc *comp = companion_choose_return( p, "_faction_exp_farm_crafting_" + cur_key.dir, calendar::before_time_starts );
                if (comp != nullptr){
                    popup(_("%s returns from your farm with something..."), comp->name.c_str());
                    companion_return( *comp );
                }
                break;
            }
        }
        if( cur_key.dir == "[B]" ){
            npc *comp = companion_choose_return( p, "_faction_camp_crafting_" + cur_key.dir, calendar::before_time_starts );
            if (comp != nullptr){
                popup(_("%s returns to you with something..."), comp->name.c_str());
                companion_return( *comp );
            }
        }
    }

    if (cur_key.id == "Distribute Food"){
        camp_distribute_food( p );
    }

    if (cur_key.id == "Gather Materials"){
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_gathering" );
        if (npc_list.size() < 3) {
            individual_mission(p, _("departs to search for materials..."), "_faction_camp_gathering", false, {}, "survival" );
        } else {
            popup( _("There are too many companions working on this mission!") );
        }
    }

    if (cur_key.id == "Recover Ally from Gathering"){
        camp_gathering_return( p, "_faction_camp_gathering" );
    }

    if (cur_key.id == "Collect Firewood"){
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_firewood" );
        if (npc_list.size() < 3) {
            individual_mission(p, _("departs to search for firewood..."), "_faction_camp_firewood", false, {}, "survival");
        } else {
            popup( _("There are too many companions working on this mission!") );
        }
    }
    if (cur_key.id == "Recover Firewood Gatherers"){
        camp_gathering_return( p, "_faction_camp_firewood" );
    }

    if (cur_key.id == "Menial Labor"){
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_menial" );
        int need_food = time_to_food( 3_hours );
        if( camp_food_supply() < need_food ){
            popup( _("You don't have enough food stored to feed your companion.") );
        } else if (npc_list.empty() ) {
            npc* comp = individual_mission(p, _("departs to dig ditches and scrub toilets..."), "_faction_camp_menial");
            if( comp != nullptr ){
                comp->companion_mission_time_ret = calendar::turn + 3_hours;
                camp_food_supply( -need_food );
            }
        } else {
            popup( _("There are too many companions working on this mission!") );
        }
    }

    if (cur_key.id == "Recover Menial Laborer"){
        camp_menial_return( p );
    }

    if (cur_key.id == "Reset Sort Points"){
        camp_menial_sort_pts( p, false, true );
    }

    if (cur_key.id == "Expand Base"){
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_expansion" );
        int need_food = time_to_food( 3_hours );
        if( camp_food_supply() < need_food ){
            popup( _("You don't have enough food stored to feed your companion.") );
        } else if (npc_list.empty() ) {
            npc* comp = individual_mission(p, _("departs to survey land..."), "_faction_camp_expansion");
            if( comp != nullptr ){
                camp_food_supply( - need_food );
                comp->companion_mission_time_ret = calendar::turn + 3_hours;
            }
        } else {
            popup( _("You have already selected a surveyor!") );
        }
    }

    if ( cur_key.id == cur_key.dir + " Expansion Upgrade" ){
        for( const auto &e : om_expansions ){
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                oter_id &omt_ref_exp = overmap_buffer.ter( e.second.x, e.second.y, e.second.z );
                std::string om_exp = omt_ref_exp.id().c_str();
                std::string bldg_exp = om_next_upgrade(om_exp);
                const recipe *making = &recipe_id( bldg_exp ).obj();
                 //Stop upgrade if you don't have materials
                inventory total_inv = g->u.crafting_inventory();
                if( making->requirements().can_make_with_inventory( total_inv, 1 ) ){
                    std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_upgrade_exp_"+cur_key.dir );
                    int need_food = time_to_food ( time_duration::from_turns( making->time / 100 ) );
                    if( camp_food_supply() < need_food ){
                        popup( _("You don't have enough food stored to feed your companion.") );
                        break;
                    }
                    if (npc_list.empty() ) {
                        if (individual_mission(p, _("begins to upgrade the expansion..."), "_faction_upgrade_exp_"+cur_key.dir, false, {},
                                       making->skill_used.obj().ident().c_str(), making->difficulty ) != nullptr ){
                            g->u.consume_components_for_craft( making, 1, true );
                            g->u.invalidate_crafting_inventory();
                            camp_food_supply( -need_food);
                        }
                    } else {
                        popup( _("You already have a worker upgrading that expansion!") );
                    }
                } else {
                    popup(_("You don't have the materials for the upgrade.") );
                }
            }
        }
    }

    if (cur_key.id == "Cut Logs"){
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_cut_log" );
        std::vector<std::string> log_sources = { "forest", "forest_thick", "forest_water" };
        popup( _("Forests and swamps are the only valid cutting locations.") );
        tripoint forest = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 1, 50, log_sources);
        if( forest != tripoint( -999, -999, -999 ) ){
            int tree_est = om_harvest_trees( p, tripoint( forest.x, forest.y, 0 ), .50, false, false);
            int tree_young_est = om_harvest_ter( p, point( forest.x, forest.y ), ter_id("t_tree_young"), .50, false );
            int dist = rl_dist( forest.x, forest.y, omt_pos.x,omt_pos.y );
            //Very roughly what the player does + 6 hours for prep, clean up, breaks
            time_duration chop_time = 6_hours + ( 1_hours * tree_est ) + ( 7_minutes * tree_young_est );
            //Generous to believe the NPC can move ~ 2 logs or ~8 heavy sticks (3 per young tree?) per trip, each way is 1 trip
            // 20 young trees => ~60 sticks which can be carried 8 at a time, so 8 round trips or 16 trips total
            //This all needs to be in an om_carry_weight_over_distance function eventually...
            int trips = tree_est + ( tree_young_est * ( 3 / 8 ) * 2 );
            //Alwasy have to come back so no odd number of trips
            trips = (trips % 2 == 0) ? trips : (trips + 1);
            time_duration travel_time = companion_travel_time_calc( forest, tripoint(omt_pos.x, omt_pos.y, forest.z), 0_minutes, trips );
            time_duration work_time = travel_time + chop_time;
            int need_food = time_to_food( work_time );
            if (!query_yn(_("Trip Estimate:\n%s"), camp_trip_description( work_time, chop_time, travel_time, dist, trips, need_food ) ) ) {
                return false;
            } else if( camp_food_supply() < need_food ){
                popup( _("You don't have enough food stored to feed your companion.") );
            } else if (npc_list.empty() ) {
                g->draw_ter();
                //wrefresh( g->w_terrain );
                npc* comp = individual_mission(p, _("departs to cut logs..."), "_faction_camp_cut_log", false, {}, "fabrication", 2 );
                if ( comp != nullptr ){
                    om_harvest_trees( *comp, forest, .50, true, true);
                    om_harvest_ter( *comp, point( forest.x, forest.y ), ter_id("t_tree_young"), .50, true );
                    om_harvest_itm( *comp, point( forest.x, forest.y ), .75, true);
                    comp->companion_mission_time_ret = calendar::turn + work_time;
                    camp_food_supply( -need_food);

                    //If we cleared a forest...
                    tree_est = om_harvest_trees( p, tripoint( forest.x, forest.y, 0 ), .50, false, false);
                    if( tree_est < 20 ){
                        oter_id &omt_trees = overmap_buffer.ter( forest.x, forest.y, g->u.posz() );
                        //Do this for swamps "forest_wet" if we have a swamp without trees...
                        if( omt_trees.id() == "forest" || omt_trees.id() == "forest_thick" ){
                            omt_trees = oter_id( "field" );
                        }
                    }
                }
            } else {
                popup( _("There are too many companions working on this mission!") );
            }
        }
    }

    if (cur_key.id == "Recover Log Cutter"){
        npc *comp = companion_choose_return( p, "_faction_camp_cut_log", calendar::before_time_starts );
        if (comp != nullptr){
            popup(_("%s returns from working in the woods..."), comp->name.c_str());
            companion_return( *comp );
        }
    }

    if (cur_key.id == "Setup Hide Site"){
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_hide_site" );
        std::vector<std::string> hide_locations = { "forest", "forest_thick", "forest_water", "field" };
        popup( _("Forests, swamps, and fields are valid hide site locations.") );
        tripoint forest = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 10, 90, hide_locations,
                                         true, true, tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), true );
        if( forest != tripoint( -999, -999, -999 ) ){
            int dist = rl_dist( forest.x, forest.y, omt_pos.x,omt_pos.y );
            inventory tgt_inv = g->u.inv;
            std::vector<item *> pos_inv = tgt_inv.items_with( []( const item &itm ) {
                return !itm.can_revive();
            } );
            if( !pos_inv.empty() ) {
                std::vector<item *> losing_equipment = individual_mission_give_equipment( pos_inv );
                int trips = om_carry_weight_to_trips( p, losing_equipment ) * 2;
                trips = (trips % 2 == 0) ? trips : (trips + 1);
                time_duration build_time = 6_hours;
                time_duration travel_time = companion_travel_time_calc( forest, tripoint(omt_pos.x, omt_pos.y, forest.z), 0_minutes, trips );
                time_duration work_time = travel_time + build_time;
                int need_food = time_to_food( work_time );
                if (!query_yn(_("Trip Estimate:\n%s"), camp_trip_description( work_time, build_time, travel_time, dist, trips, need_food ) ) ) {
                    return false;
                } else if( camp_food_supply() < need_food ){
                    popup( _("You don't have enough food stored to feed your companion.") );
                } else if (npc_list.empty() ) {

                    npc* comp = individual_mission( p, _("departs to build a hide site..."), "_faction_camp_hide_site", false,
                                       {}, "survival", 3 );
                    if ( comp != nullptr ){
                        comp->companion_mission_time_ret = calendar::turn + work_time;
                        om_set_hide_site( *comp, forest, losing_equipment );
                        camp_food_supply( -need_food );
                    }
                } else {
                    popup( _("There are too many companions working on this mission!") );
                }
            } else {
                popup(_("You need equipment to setup a hide site..."));
            }
        }
    }

    if (cur_key.id == "Recover Hide Setup"){
        npc *comp = companion_choose_return( p, "_faction_camp_hide_site", calendar::before_time_starts );
        if( comp != nullptr ){
            popup(_("%s returns from working on the hide site..."), comp->name.c_str());
            companion_return( *comp );
        }
    }

    if (cur_key.id == "Relay Hide Site"){
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_hide_trans" );
        std::vector<std::string> hide_locations = { "faction_hide_site_0" };
        popup( _("You must select an existing hide site.") );
        tripoint forest = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 10, 90, hide_locations,
                                         true, true, tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), true );
        if( forest != tripoint( -999, -999, -999 ) ){
            int dist = rl_dist( forest.x, forest.y, omt_pos.x,omt_pos.y );
            inventory tgt_inv = g->u.inv;
            std::vector<item *> pos_inv = tgt_inv.items_with( []( const item &itm ) {
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
            for( item &i : target_bay.i_at( 11, 10) ){
                    hide_inv.push_back(&i);
            }
            std::vector<item *> gaining_equipment;
            if( !hide_inv.empty() ) {
                gaining_equipment = individual_mission_give_equipment( hide_inv, _("Bring gear back?") );
            }
            if( !losing_equipment.empty() || !gaining_equipment.empty() ) {
                //Only get charged the greater trips since return is free for both
                int trips = om_carry_weight_to_trips( p, losing_equipment ) * 2;
                int trips_return = om_carry_weight_to_trips( p, gaining_equipment ) * 2;
                if( trips < trips_return ){
                    trips = trips_return;
                }
                trips = (trips % 2 == 0) ? trips : (trips + 1);
                time_duration build_time = 6_hours;
                time_duration travel_time = companion_travel_time_calc( forest, tripoint(omt_pos.x, omt_pos.y, forest.z), 0_minutes, trips );
                time_duration work_time = travel_time + build_time;
                int need_food = time_to_food( work_time );
                if (!query_yn(_("Trip Estimate:\n%s"), camp_trip_description( work_time, build_time, travel_time, dist, trips, need_food ) ) ) {
                    return false;
                } else if( camp_food_supply() < need_food ){
                    popup( _("You don't have enough food stored to feed your companion.") );
                } else if (npc_list.empty() ) {

                    npc* comp = individual_mission( p, _("departs for the hide site..."), "_faction_camp_hide_site", false,
                                       {}, "survival", 3 );
                    if ( comp != nullptr ){
                        comp->companion_mission_time_ret = calendar::turn + work_time;
                        om_set_hide_site( *comp, forest, losing_equipment, gaining_equipment );


                        camp_food_supply( -need_food );
                    }
                } else {
                    popup( _("There are too many companions working on this mission!") );
                }
            } else {
                popup(_("You need equipment to transport between the hide site..."));
            }
        }
    }


     if (cur_key.id == "Recover Hide Transport"){
        npc *comp = companion_choose_return( p, "_faction_camp_hide_trans", calendar::before_time_starts );
        if (comp != nullptr){
            popup(_("%s returns from shuttling gear between the hide site..."), comp->name.c_str());
            companion_return( *comp );
        }
    }

    if (cur_key.id == "Construct Map Fortifications" || cur_key.id == "Construct Spiked Trench"){
        std::vector<std::string> allowed_locations = { "forest", "forest_thick", "forest_water", "field",
                                            "faction_wall_level_N_0", "faction_wall_level_E_0","faction_wall_level_S_0","faction_wall_level_W_0",
                                            "faction_wall_level_N_1", "faction_wall_level_E_1","faction_wall_level_S_1","faction_wall_level_W_1" };
        if( cur_key.id == "Construct Spiked Trench" ){
            allowed_locations = { "faction_wall_level_N_0", "faction_wall_level_E_0","faction_wall_level_S_0","faction_wall_level_W_0",
                                "faction_wall_level_N_1", "faction_wall_level_E_1","faction_wall_level_S_1","faction_wall_level_W_1" };
        }

        popup( _("Select a start and end point.  Line must be straight. Fields, forests, and swamps are valid fortification locations."
                 "  In addition to existing fortification constructions.") );
        tripoint start = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 2, 90, allowed_locations );
        popup( _("Select an end point.") );
        tripoint stop = om_target_tile( tripoint( omt_pos.x, omt_pos.y, g->u.posz() ), 2, 90, allowed_locations, true, false, start );
        if( start != tripoint( -999, -999, -999 ) && stop != tripoint( -999, -999, -999 ) ){
            std::string bldg_exp = "faction_wall_level_N_0";
            if( cur_key.id == "Construct Spiked Trench" ){
                bldg_exp = "faction_wall_level_N_1";
            }
            const recipe *making = &recipe_id( bldg_exp ).obj();
            inventory total_inv = g->u.crafting_inventory();
            bool change_x = (start.x != stop.x );
            bool change_y = (start.y != stop.y );
            if( change_x && change_y ){
                popup( "Construction line must be straight!" );
                return false;
            }
            std::vector<tripoint> fortify_om;
            if( (change_x && stop.x < start.x) || (change_y && stop.y < start.y) ) {
                //line_to doesn't include the origin point
                fortify_om.push_back(stop);
                std::vector<tripoint> tmp_line = line_to( stop, start, 0 );
                fortify_om.insert(fortify_om.end(), tmp_line.begin(), tmp_line.end());
            } else {
                fortify_om.push_back(start);
                std::vector<tripoint> tmp_line = line_to( start, stop, 0 );
                fortify_om.insert(fortify_om.end(), tmp_line.begin(), tmp_line.end());
            }
            int trips = 0;
            time_duration build_time = 0_hours;
            time_duration travel_time = 0_hours;
            int dist = 0;
            for( auto fort_om : fortify_om ){
                bool valid = false;
                oter_id &omt_ref = overmap_buffer.ter( fort_om.x, fort_om.y, fort_om.z );
                for( auto pos_om : allowed_locations ){
                    if( omt_ref.id().c_str() == pos_om ){
                        valid = true;
                    }
                }

                if( !valid ){
                    popup( _("Invalid terrain in construction path.") );
                    return false;
                }
                trips += 2;
                build_time += time_duration::from_turns( making->time / 100 );
                travel_time += companion_travel_time_calc( fort_om, tripoint(omt_pos.x, omt_pos.y, fort_om.z), 0_minutes, trips );
                dist += rl_dist( fort_om.x, fort_om.y, omt_pos.x,omt_pos.y );
            }
            time_duration total_time = travel_time + build_time;
            int need_food = time_to_food ( total_time );
            if (!query_yn(_("Trip Estimate:\n%s"), camp_trip_description( total_time, build_time, travel_time, dist, trips, need_food ) ) ) {
                return false;
            } else if( !making->requirements().can_make_with_inventory( total_inv, (fortify_om.size() * 2) - 2 ) ){
                popup( _("You don't have the material to build the fortification.") );
            } else if( camp_food_supply() < need_food ){
                popup( _("You don't have enough food stored to feed your companion.") );
            } else {
                npc* comp = individual_mission(p, _("begins constructing fortifications..."), "_faction_camp_om_fortifications", false, {},
                                       making->skill_used.obj().ident().c_str(), making->difficulty );
                if( comp != nullptr ){
                    g->u.consume_components_for_craft( making, (fortify_om.size() * 2) - 2, true );
                    g->u.invalidate_crafting_inventory();
                    camp_food_supply( -need_food );
                    companion_skill_trainer( *comp, "construction", build_time, 2 );
                    comp->companion_mission_time_ret = calendar::turn + total_time;
                    comp->companion_mission_role_id = bldg_exp;
                    for( auto pt : fortify_om ){
                        comp->companion_mission_points.push_back( pt );
                    }
                }
            }
        }
    }

    if (cur_key.id == "Finish Map Fortifications"){
        npc *comp = companion_choose_return( p, "_faction_camp_om_fortifications", calendar::before_time_starts );
        if( comp != nullptr ){
            popup(_("%s returns from constructing fortifications..."), comp->name.c_str());
            editmap edit;
            bool build_dir_NS = ( comp->companion_mission_points[0].y != comp->companion_mission_points[1].y );
            //Ensure all tiles are generated before putting fences/trenches down...
            for( auto pt : comp->companion_mission_points ){
                if( MAPBUFFER.lookup_submap( om_to_sm_copy(pt) ) == NULL ){
                    oter_id &omt_test = overmap_buffer.ter( pt.x, pt.y, pt.z );
                    std::string om_i = omt_test.id().c_str();
                    //The thick forests will gen harsh boundries since it won't recognize these tiles when they become fortifications
                    if( om_i == "forest_thick" ){
                        om_i = "forest";
                    }
                    edit.mapgen_set( om_i, pt, false );
                }
            }
            //Add fences
            auto build_point = comp->companion_mission_points;
            for( size_t pt = 0; pt < build_point.size(); pt++ ){
                //First point is always at top or west since they are built in a line and sorted
                std::string build_n = "faction_wall_level_N_0";
                std::string build_e = "faction_wall_level_E_0";
                std::string build_s = "faction_wall_level_S_0";
                std::string build_w = "faction_wall_level_W_0";
                if( comp->companion_mission_role_id == "faction_wall_level_N_1" ){
                    build_n = "faction_wall_level_N_1";
                    build_e = "faction_wall_level_E_1";
                    build_s = "faction_wall_level_S_1";
                    build_w = "faction_wall_level_W_1";
                }
                if( pt == 0 ){
                    if( build_dir_NS ){
                        edit.mapgen_set( build_s, build_point[pt] );
                    } else {
                        edit.mapgen_set( build_e, build_point[pt] );
                    }
                } else if ( pt == build_point.size() - 1 ){
                    if( build_dir_NS ){
                        edit.mapgen_set( build_n, build_point[pt] );
                    } else {
                        edit.mapgen_set( build_w, build_point[pt] );
                    }
                } else if( build_dir_NS ){
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


    if (cur_key.id == "Recruit Companions"){
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_recruit_0" );
        int need_food = time_to_food ( 4_days );
        if( camp_food_supply() < need_food ){
            popup( _("You don't have enough food stored to feed your companion.") );
        } else if (npc_list.empty() ) {
            npc* comp = individual_mission(p, _("departs to search for recruits..."), "_faction_camp_recruit_0", false, {}, "speech", 2);
            if ( comp != nullptr ){
                comp->companion_mission_time_ret = calendar::turn + 4_days;
                camp_food_supply( -need_food);
            }
        } else {
            popup( _("There are too many companions working on this mission!") );
        }
    }

    if( cur_key.id == "Recover Recruiter" ){
        camp_recruit_return( p, "_faction_camp_recruit_0", atoi( camp_recruit_evaluation( p, om_cur, om_expansions, true ).c_str() ) );
    }


    if( cur_key.id == "Scout Mission" || cur_key.id == "Combat Patrol" ){
        std::string mission_t = "_faction_camp_scout_0";
        if( cur_key.id == "Combat Patrol" ){
            mission_t = "_faction_camp_combat_0";
        }
        popup( _("Select checkpoints until you reach maximum range or select the last point again to end.") );
        tripoint start = tripoint( omt_pos.x, omt_pos.y, g->u.posz() );
        std::vector<tripoint> scout_points = om_companion_path( start, 90, true);
        if( scout_points.empty() ){
            return false;
        }
        int dist = scout_points.size();
        std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, mission_t );
        int trips = 2;
        time_duration travel_time = companion_travel_time_calc( scout_points, 0_minutes, trips );
        time_duration build_time = 0_hours;
        time_duration total_time = travel_time + build_time;
        int need_food = time_to_food ( total_time );
        if (!query_yn(_("Trip Estimate:\n%s"), camp_trip_description( total_time, build_time, travel_time, dist, trips, need_food ) ) ) {
                return false;
        } else if( camp_food_supply() < need_food ){
            popup( _("You don't have enough food stored to feed your companion.") );
        } else if ( npc_list.size() < 3 ) {
            npc* comp = individual_mission(p, _("departs on patrol..."), mission_t, false, {}, "survival", 3 );
            if ( comp != nullptr ){
                comp->companion_mission_points = scout_points;
                comp->companion_mission_time_ret = calendar::turn + travel_time;
                camp_food_supply( -need_food);
            }
        } else {
            popup( _("There are too many companions working on this mission!") );
        }
    }

    if( cur_key.id == "Recover Scout" || cur_key.id == "Recover Combat Patrol" ){
        std::string miss = "_faction_camp_scout_0";
        if( cur_key.id == "Recover Combat Patrol" ) {
            miss = "_faction_camp_combat_0";
        }
        npc *comp = companion_choose_return( p, miss, calendar::before_time_starts );
        if (comp != nullptr){
            std::vector<std::shared_ptr<npc>> patrol;
            std::shared_ptr<npc> guy = overmap_buffer.find_npc( comp->getID() );
            patrol.push_back( guy );
            for( auto pt : comp->companion_mission_points ){
                oter_id &omt_ref = overmap_buffer.ter( pt.x, pt.y, pt.z );
                int swim = comp->get_skill_level( skill_swimming );
                if( is_river(omt_ref) && swim < 2 ){
                    if( swim == 0 ){
                        popup( _("Your companion hit a river and didn't know how to swim...") );
                    } else {
                        popup( _("Your companion hit a river and didn't know how to swim well enough to cross...") );
                    }
                    break;
                }
                bool outcome = false;
                comp->death_drops = false;
                if( cur_key.id == "Recover Scout" ){
                    outcome = companion_om_combat_check( patrol, pt, false );
                } else if( cur_key.id == "Recover Combat Patrol" ){
                    outcome = companion_om_combat_check( patrol, pt, true );
                }
                comp->death_drops = true;
                if( !outcome ){
                    if( comp->is_dead() ) {
                        popup(_("%s didn't return from patrol..."), comp->name.c_str());
                        comp->place_corpse( pt );
                        overmap_buffer.add_note( pt.x, pt.y, pt.z,"DEAD NPC" );
                        overmap_buffer.remove_npc( comp->getID() );
                        return false;
                    } else {
                        popup(_("%s returns from patrol..."), comp->name.c_str());
                        companion_return( *comp );
                        return false;
                    }

                }
                overmap_buffer.reveal( pt, 2);
            }
            popup(_("%s returns from patrol..."), comp->name.c_str());
            companion_return( *comp );
        }
    }

    if ( cur_key.id == cur_key.dir + " Chop Shop" ){
        for( const auto &e : om_expansions ){
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_exp_chop_shop_"+cur_key.dir );
                int need_food = time_to_food ( 5_days );
                if( camp_food_supply() < need_food ){
                    popup( _("You don't have enough food stored to feed your companion.") );
                } else if (npc_list.empty() ) {
                    camp_garage_chop_start( p, "_faction_exp_chop_shop_" + cur_key.dir );
                    camp_food_supply( - need_food );
                } else {
                    popup( _("You already have someone working in that garage.") );
                }
            }
        }
    }

    //Will be generalized for all crafing, when needed
    std::map<std::string,std::string> crafting_recipes = camp_recipe_deck( "ALL" );
    std::map<std::string,std::string> cooking_recipes = camp_recipe_deck( "COOK" );
    std::map<std::string,std::string> base_recipes = camp_recipe_deck( "BASE" );
    std::map<std::string,std::string> farming_recipes = camp_recipe_deck( "FARM" );
    std::map<std::string,std::string> blacksmith_recipes = camp_recipe_deck( "SMITH" );
    camp_craft_construction( p, cur_key, cooking_recipes, "_faction_exp_kitchen_cooking_", tripoint( omt_pos.x, omt_pos.y, p.posz() ),
                            om_expansions  );
    camp_craft_construction( p, cur_key, farming_recipes, "_faction_exp_farm_crafting_", tripoint( omt_pos.x, omt_pos.y, p.posz() ),
                            om_expansions  );
    camp_craft_construction( p, cur_key, blacksmith_recipes, "_faction_exp_blacksmith_crafting_", tripoint( omt_pos.x, omt_pos.y, p.posz() ),
                            om_expansions  );
    //All crafting for base hub
    std::vector<std::pair<std::string, tripoint>> om_expansions_plus;
    std::pair<std::string, tripoint> ent = std::make_pair( om_cur, tripoint( omt_pos.x, omt_pos.y, p.posz() ) );
    om_expansions_plus.push_back( ent );
    camp_craft_construction( p, cur_key, base_recipes, "_faction_camp_crafting_", tripoint( omt_pos.x, omt_pos.y, p.posz() ),
                            om_expansions_plus  );

    if ( cur_key.id == cur_key.dir + " Plow Fields" ){
        for( const auto &e : om_expansions ){
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_exp_plow_"+cur_key.dir );
                if (npc_list.empty()) {
                    individual_mission(p, _("begins plowing the field..."), "_faction_exp_plow_"+cur_key.dir );
                } else {
                    popup( _("You already have someone plowing that field.") );
                }
            }
        }
    }

    if ( cur_key.id == cur_key.dir + " Plant Fields" ){
        for( const auto &e : om_expansions ){
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_exp_plant_"+cur_key.dir );
                if (npc_list.empty()) {
                    inventory total_inv = g->u.crafting_inventory();
                    std::vector<item *> seed_inv = total_inv.items_with( []( const item &itm ) {
                        return itm.is_seed();
                    } );
                    if( seed_inv.empty() ) {
                        popup(_("You have no additional seeds to give your companions..."));
                        individual_mission(p, _("begins planting the field..."), "_faction_exp_plant_"+cur_key.dir );
                    } else {
                        std::vector<item *> lost_equipment = individual_mission_give_equipment( seed_inv );
                        individual_mission(p, _("begins planting the field..."), "_faction_exp_plant_"+cur_key.dir, false, lost_equipment );
                    }
                } else {
                    popup( _("You already have someone planting that field.") );
                }
            }
        }
    }

    if ( cur_key.id == cur_key.dir + " Harvest Fields" ){
        for( const auto &e : om_expansions ){
            if( om_simple_dir( omt_pos, e.second ) == cur_key.dir ) {
                std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_exp_harvest_"+cur_key.dir );
                if (npc_list.empty()) {
                    individual_mission(p, _("begins to harvest the field..."), "_faction_exp_harvest_"+cur_key.dir, false, {}, "survival" );
                } else {
                    popup( _("You already have someone harvesting that field.") );
                }
            }
        }
    }

    if (cur_key.id == "Recover Surveyor"){
        camp_expansion_select( p );
    }

    if (cur_key.id == "Assign Ally to Carpentry Work"){
        individual_mission(p, _("departs to work as a carpenter..."), "_carpenter");
    }
    if (cur_key.id == "Recover Ally from Carpentry Work"){
        carpenter_return(p);
    }
    if (cur_key.id == "Assign Ally to Forage for Food"){
        individual_mission(p, _("departs to forage for food..."), "_forage");
    }
    if (cur_key.id == "Recover Ally from Foraging"){
        forage_return(p);
    }

    g->draw_ter();
    wrefresh( g->w_terrain );

    return true;
}

npc* talk_function::individual_mission( npc &p, const std::string &desc, const std::string &miss_id, bool group, std::vector<item *> equipment, std::string skill_tested, int skill_level )
{
    npc *comp = companion_choose( skill_tested, skill_level );
    if( comp == nullptr ){
        return comp;
    }

    //Ensure we have someone to give equipment to before we lose it
    for( auto i: equipment){
        comp->companion_mission_inv.add_item(*i);
        //comp->i_add(*i);
        if( item::count_by_charges( i->typeId() ) ) {
            g->u.use_charges( i->typeId(), i->charges );
        } else {
            g->u.use_amount( i->typeId(), 1 );
        }
    }
    if( comp->in_vehicle ) {
        g->m.unboard_vehicle( comp->pos() );
    }
    popup("%s %s", comp->name.c_str(), desc.c_str());
    comp->set_companion_mission( p, miss_id );
    if (group){
        comp->companion_mission_time = calendar::before_time_starts;
    } else {
        comp->companion_mission_time = calendar::turn;
    }
    g->reload_npcs();
    assert( !comp->is_active() );
    return comp;
}

std::vector<item *> talk_function::individual_mission_give_equipment( std::vector<item *> equipment, std::string message ){
    std::vector<item *> equipment_lost;
    do {
        g->draw_ter();
        wrefresh( g->w_terrain );

        std::vector<std::string> names;
        for( auto &i : equipment ) {
            names.push_back( i->tname() + " [" + to_string(i->charges) + "]" );
        }

        // Choose item if applicable
        int i_index = 0;
        names.push_back(_("Done"));
        i_index = menu_vec(false, message.c_str(), names) - 1;
        if (i_index == (int)names.size() - 1) {
            i_index = -1;
        }

        if (i_index < 0) {
            return equipment_lost;
        }
        equipment_lost.push_back(equipment[i_index]);
        equipment.erase(equipment.begin()+i_index);
    } while ( !equipment.empty() );
    return equipment_lost;
}

void talk_function::caravan_depart( npc &p, const std::string &dest, const std::string &id )
{
    std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, id );
    int distance = caravan_dist(dest);
    int time = 200 + distance * 100;
    popup(_("The caravan departs with an estimated total travel time of %d hours..."), int(time/600));

    for( auto &elem : npc_list ) {
        if( elem->companion_mission_time == calendar::before_time_starts ) {
            //Adds a 10% error in estimated travel time
            elem->companion_mission_time = calendar::turn + time_duration::from_turns( time + time * rng_float( -.1, .1 ) );
        }
    }

}

//Could be expanded to actually path to the site, just returns the distance
int talk_function::caravan_dist( const std::string &dest )
{
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), dest, 0, false );
    int distance = rl_dist( g->u.pos(), site );
    return distance;
}

void talk_function::caravan_return( npc &p, const std::string &dest, const std::string &id )
{
    npc *comp = companion_choose_return( p, id, calendar::turn );
    if (comp == nullptr){
        return;
    }
    if( comp->companion_mission_time == calendar::before_time_starts ) {
        popup(_("%s returns to your party."), comp->name.c_str());
        companion_return( *comp );
        return;
    }
    //So we have chosen to return an individual or party who went on the mission
    //Everyone who was on the mission will have the same companion_mission_time
    //and will simulate the mission and return together
    std::vector<std::shared_ptr<npc>> caravan_party;
    std::vector<std::shared_ptr<npc>> bandit_party;
    std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, id );
    for (int i = 0; i < rng(1,3); i++){
        caravan_party.push_back(temp_npc(string_id<npc_template>( "commune_guard" )));
    }
    for( auto &elem : npc_list ) {
        if (elem->companion_mission_time == comp->companion_mission_time){
            caravan_party.push_back(elem);
        }
    }

    int distance = caravan_dist(dest);
    int time = 200 + distance * 100;
    int experience = rng( 10, time / 300 );

    for (int i = 0; i < rng( 1, 3 ); i++){
        bandit_party.push_back(temp_npc(string_id<npc_template>( "bandit" )));
        bandit_party.push_back(temp_npc(string_id<npc_template>( "thug" )));
    }


    if (one_in(3)){
        if (one_in(2)){
            popup(_("A bandit party approaches the caravan in the open!"));
            force_on_force(caravan_party, "caravan", bandit_party, "band", 1);
        } else if (one_in(3)){
            popup(_("A bandit party attacks the caravan while it it's camped!"));
            force_on_force(caravan_party, "caravan", bandit_party, "band", 2);
        } else {
            popup(_("The caravan walks into a bandit ambush!"));
            force_on_force(caravan_party, "caravan", bandit_party, "band", -1);
        }
    }

    int y = 0;
    int i = 0;
    int money = 0;
    for( const auto &elem : caravan_party ) {
        //Scrub temporary party members and the dead
        if (elem->hp_cur[hp_torso] == 0 && elem->has_companion_mission() ) {
            overmap_buffer.remove_npc( comp->getID() );
            money += (time/600) * 9;
        } else if( elem->has_companion_mission() ){
            money += (time/600) * 18;
            i = 0;
            while (i < experience){
                y = rng(0,100);
                if (y < 60){
                    const skill_id best = elem->best_skill();
                    if( best ) {
                        elem->practice(best, 10);
                    } else {
                        elem->practice( skill_melee, 10);
                    }
                } else if (y < 70){
                    elem->practice( skill_survival, 10);
                } else if (y < 80){
                    elem->practice( skill_melee, 10);
                } else if (y < 85){
                    elem->practice( skill_firstaid, 10);
                } else if (y < 90){
                    elem->practice( skill_speech, 10);
                } else if (y < 92){
                    elem->practice( skill_bashing, 10);
                } else if (y < 94){
                    elem->practice( skill_stabbing, 10);
                } else if (y < 96){
                    elem->practice( skill_cutting, 10);
                } else if (y < 98){
                    elem->practice( skill_dodge, 10);
                } else {
                    elem->practice( skill_unarmed, 10);
                }
                i++;
            };
            companion_return( *elem );
        }
    }

    if (money != 0){
        g->u.cash += (100*money);
        popup(_("The caravan party has returned.  Your share of the profits are $%d!"), money);
    } else {
        popup(_("The caravan was a disaster and your companions never made it home..."));
    }
}

//A random NPC on one team attacks a random monster on the opposite
void talk_function::attack_random( const std::vector<std::shared_ptr<npc>> &attacker, const std::vector< monster * > &group )
{
    if( attacker.empty() || group.empty() ) {
        return;
    }
    const auto att = random_entry( attacker );
    monster *def = random_entry( group );
    att->melee_attack( *def, false);
    if( def->get_hp() <= 0 ){
        popup(_("%s is wasted by %s!"), def->type->nname(), att->name.c_str());
    }
}

//A random monster on one side attacks a random NPC on the other
void talk_function::attack_random( const std::vector< monster * > &group, const std::vector<std::shared_ptr<npc>> &defender )
{
    if( defender.empty() || group.empty() ) {
        return;
    }
    monster *att = random_entry( group );
    const auto def = random_entry( defender );
    att->melee_attack( *def );
    //monster mon;
    if( def->hp_cur[hp_torso] <= 0 || def->is_dead() ){
    //if( def->hp_cur[hp_torso] <= 0 || def->is_dead() ){
        popup(_("%s is wasted by %s!"), def->name.c_str(), att->type->nname() );
    }
}

//A random NPC on one team attacks a random NPC on the opposite
void talk_function::attack_random( const std::vector<std::shared_ptr<npc>> &attacker, const std::vector<std::shared_ptr<npc>> &defender )
{
    if( attacker.empty() || defender.empty() ) {
        return;
    }
    const auto att = random_entry( attacker );
    const auto def = random_entry( defender );
    const skill_id best = att->best_skill();
    int best_score = 1;
    if( best ) {
        best_score = att->get_skill_level(best);
    }
    ///\EFFECT_DODGE_NPC increases avoidance of random attacks
    if( rng( -1, best_score ) >= rng( 0, def->get_skill_level( skill_dodge ) ) ){
        def->hp_cur[hp_torso] = 0;
        popup(_("%s is wasted by %s!"), def->name.c_str(), att->name.c_str());
    } else {
        popup(_("%s dodges %s's attack!"), def->name.c_str(), att->name.c_str());
    }
}

//Used to determine when to retreat, might want to add in a random factor so that engagements aren't
//drawn out wars of attrition
int talk_function::combat_score( const std::vector<std::shared_ptr<npc>> &group )
{
    int score = 0;
    for( const auto &elem : group ) {
        if (elem->hp_cur[hp_torso] != 0){
            const skill_id best = elem->best_skill();
            if( best ) {
                score += elem->get_skill_level(best);
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
    for( const auto &elem : group ) {
        if( elem->get_hp() > 0 ){
            score += elem->type->difficulty;
        }
    }
    return score;
}

std::shared_ptr<npc> talk_function::temp_npc( const string_id<npc_template> &type )
{
    std::shared_ptr<npc> temp = std::make_shared<npc>();
    temp->normalize();
    temp->load_npc_template(type);
    return temp;
}

//The field is designed as more of a convenience than a profit opportunity.
void talk_function::field_build_1( npc &p )
{
    if (g->u.cash < 100000){
        popup(_("I'm sorry, you don't have enough money."));
        return;
    }
    p.set_mutation( trait_NPC_CONSTRUCTION_LEV_1 );
    g->u.cash += -100000;
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), "ranch_camp_63", 20, false );
    tinymap bay;
    bay.load(site.x * 2, site.y * 2, site.z, false);
    bay.draw_square_ter(t_dirt, 5, 4, 15, 14);
    bay.draw_square_ter(t_dirtmound, 6, 5, 6, 13);
    bay.draw_square_ter(t_dirtmound, 8, 5, 8, 13);
    bay.draw_square_ter(t_dirtmound, 10, 5, 10, 13);
    bay.draw_square_ter(t_dirtmound, 12, 5, 12, 13);
    bay.draw_square_ter(t_dirtmound, 14, 5, 14, 13);
    bay.save();
    popup( _( "%s jots your name down on a ledger and yells out to nearby laborers to begin plowing your new field." ), p.name.c_str() );
}

//Really expensive, but that is so you can't tear down the fence and sell the wood for a profit!
void talk_function::field_build_2( npc &p )
{
    if (g->u.cash < 550000){
        popup(_("I'm sorry, you don't have enough money."));
        return;
    }
    p.set_mutation( trait_NPC_CONSTRUCTION_LEV_2 );
    g->u.cash += -550000;
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), "ranch_camp_63", 20, false );
    tinymap bay;
    bay.load(site.x * 2, site.y * 2, site.z, false);
    bay.draw_square_ter(t_fence, 4, 3, 16, 3);
    bay.draw_square_ter(t_fence, 4, 15, 16, 15);
    bay.draw_square_ter(t_fence, 4, 3, 4, 15);
    bay.draw_square_ter(t_fence, 16, 3, 16, 15);
    bay.draw_square_ter(t_fencegate_c, 10, 3, 10, 3);
    bay.draw_square_ter(t_fencegate_c, 10, 15, 10, 15);
    bay.draw_square_ter(t_fencegate_c, 4, 9, 4, 9);
    bay.save();
    popup( _( "After counting your money %s directs a nearby laborer to begin constructing a fence around your plot..." ), p.name.c_str() );
}

void talk_function::field_plant( npc &p, const std::string &place )
{
    if (g->get_temperature( g->u.pos() ) < 50) {
        popup(_("It is too cold to plant anything now."));
        return;
    }
    std::vector<item *> seed_inv = g->u.items_with( []( const item &itm ) {
        return itm.is_seed();
    } );
    if( seed_inv.empty() ) {
        popup(_("You have no seeds to plant!"));
        return;
    }

    int empty_plots = 0;
    int free_seeds = 0;

    std::vector<itype_id> seed_types;
    std::vector<std::string> seed_names;
    for( auto &seed : seed_inv ) {
        if( std::find( seed_types.begin(), seed_types.end(), seed->typeId() ) == seed_types.end() ) {
            if (seed->typeId() !=  "marloss_seed" && seed->typeId() !=  "fungal_seeds"){
                seed_types.push_back( seed->typeId() );
                seed_names.push_back( seed->tname() );
            }
        }
    }
    // Choose seed if applicable
    int seed_index = 0;
    seed_names.push_back(_("Cancel"));
    seed_index = menu_vec(false, _("Which seeds do you wish to have planted?"),
                          seed_names) - 1;
    if (seed_index == (int)seed_names.size() - 1) {
        seed_index = -1;
    }
    // Did we cancel?
    if (seed_index < 0) {
        popup(_("You saved your seeds for later."));
        return;
    }

    const auto &seed_id = seed_types[seed_index];
    if( item::count_by_charges( seed_id ) ) {
        free_seeds = g->u.charges_of( seed_id );
    } else {
        free_seeds = g->u.amount_of( seed_id );
    }

    //Now we need to find how many free plots we have to plant in...
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), place, 20, false );
    tinymap bay;
    bay.load(site.x * 2, site.y * 2, site.z, false);
    for (int x = 0; x < 23; x++){
        for (int y = 0; y < 23; y++){
            if( bay.ter( x, y ) == t_dirtmound ) {
                empty_plots++;
            }
        }
    }

    if (empty_plots == 0){
        popup(_("You have no room to plant seeds..."));
        return;
    }

    int limiting_number = free_seeds;
    if (free_seeds > empty_plots)
        limiting_number = empty_plots;

    unsigned int a = limiting_number*300;
    if ( a > g->u.cash){
        popup(_("I'm sorry, you don't have enough money to plant those seeds..."));
        return;
    }
    if (!query_yn(_("Do you wish to have %d %s planted here for $%d?"), limiting_number, seed_names[seed_index].c_str(), limiting_number*3)) {
        return;
    }

    //Plant the actual seeds
    for (int x = 0; x < 23; x++){
        for (int y = 0; y < 23; y++){
            if( bay.ter( x, y ) == t_dirtmound && limiting_number > 0){
                std::list<item> used_seed;
                if( item::count_by_charges( seed_id ) ) {
                    used_seed = g->u.use_charges( seed_id, 1 );
                } else {
                    used_seed = g->u.use_amount( seed_id, 1 );
                }
                used_seed.front().set_age( 0 );
                bay.add_item_or_charges( x, y, used_seed.front() );
                bay.set( x, y, t_dirt, f_plant_seed);
                limiting_number--;
            }
        }
    }
    bay.draw_square_ter(t_fence, 4, 3, 16, 3);
    bay.save();
    popup( _( "After counting your money and collecting your seeds, %s calls forth a labor party to plant your field." ), p.name.c_str() );
}

void talk_function::field_harvest( npc &p, const std::string &place )
{
    //First we need a list of plants that can be harvested...
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), place, 20, false );
    tinymap bay;
    item tmp;
    bool check;
    std::vector<itype_id> seed_types;
    std::vector<itype_id> plant_types;
    std::vector<std::string> plant_names;
    bay.load(site.x * 2, site.y * 2, site.z, false);
    for (int x = 0; x < 23; x++){
        for (int y = 0; y < 23; y++){
            if (bay.furn(x,y) == furn_str_id( "f_plant_harvest" ) && !bay.i_at(x,y).empty()){
                const item &seed = bay.i_at( x,y )[0];
                if( seed.is_seed() ) {
                    const islot_seed &seed_data = *seed.type->seed;
                    tmp = item( seed_data.fruit_id, calendar::turn );
                    check = false;
                    for( auto elem : plant_names ) {
                        if (elem == tmp.type_name(3).c_str())
                            check = true;
                    }
                    if (!check){
                        plant_types.push_back( tmp.typeId() );
                        plant_names.push_back( tmp.type_name(3).c_str() );
                        seed_types.push_back( seed.typeId() );
                    }
                }
            }
        }
    }
    if( plant_names.empty() ) {
        popup(_("There aren't any plants that are ready to harvest..."));
        return;
    }
    // Choose the crop to harvest
    int plant_index = 0;
    plant_names.push_back(_("Cancel"));
    plant_index = menu_vec(false, _("Which plants do you want to have harvested?"),
                          plant_names) - 1;
    if (plant_index == (int)plant_names.size() - 1) {
        plant_index = -1;
    }
    // Did we cancel?
    if (plant_index < 0) {
        popup(_("You decided to hold off for now..."));
        return;
    }

    int number_plots = 0;
    int number_plants = 0;
    int number_seeds = 0;
    int skillLevel = 2;
    if ( p.has_trait( trait_NPC_CONSTRUCTION_LEV_2 ) )
        skillLevel += 2;

    for (int x = 0; x < 23; x++){
        for (int y = 0; y < 23; y++){
            if (bay.furn(x,y) == furn_str_id( "f_plant_harvest" ) && !bay.i_at(x,y).empty()){
                const item &seed = bay.i_at( x,y )[0];
                if( seed.is_seed() ) {
                    const islot_seed &seed_data = *seed.type->seed;
                    tmp = item( seed_data.fruit_id, calendar::turn );
                    if (tmp.typeId() == plant_types[plant_index]){
                        number_plots++;
                        bay.i_clear(x,y);
                        bay.furn_set(x,y,f_null);
                        bay.ter_set(x,y,t_dirtmound);
                        int plantCount = rng(skillLevel / 2, skillLevel);
                        if (plantCount >= 9) {
                            plantCount = 9;
                        } else if( plantCount <= 0 ) {
                            plantCount = 1;
                        }
                        number_plants += plantCount;
                        number_seeds += std::max( 1l, rng( plantCount / 4, plantCount / 2 ) );
                    }
                }
            }
        }
    }
    bay.save();
    tmp = item( plant_types[plant_index], calendar::turn );
    int money = (number_plants*tmp.price( true )-number_plots*2)/100;
    bool liquidate = false;

    unsigned int a = number_plots*2;
    if ( a > g->u.cash){
        liquidate = true;
        popup(_("You don't have enough to pay the workers to harvest the crop so you are forced to liquidate..."));
    } else {
        liquidate= query_yn(_("Do you wish to liquidate the crop of %d %s for a profit of $%d?"), number_plants, plant_names[plant_index].c_str(), money);
    }

    //Add fruit
    if (liquidate){
        add_msg(_("The %s are liquidated for $%d..."), plant_names[plant_index].c_str(), money);
        g->u.cash += (number_plants*tmp.price( true )-number_plots*2)/100;
    } else {
        if( tmp.count_by_charges() ) {
            tmp.charges = 1;
        }
        for( int i = 0; i < number_plants; ++i ) {
            //Should be dropped at your feet once greedy companions can be controlled
            g->u.i_add(tmp);
        }
        add_msg(_("You receive %d %s..."), number_plants,plant_names[plant_index].c_str());
    }
    tmp = item( seed_types[plant_index], calendar::turn );
    const islot_seed &seed_data = *tmp.type->seed;
    if( seed_data.spawn_seeds ){
        if( tmp.count_by_charges() ) {
            tmp.charges = 1;
        }
        for( int i = 0; i < number_seeds; ++i ) {
            g->u.i_add(tmp);
        }
        add_msg(_("You receive %d %s..."), number_seeds,tmp.type_name(3).c_str());
    }

}

bool talk_function::scavenging_patrol_return( npc &p )
{
    npc *comp = companion_choose_return( p, "_scavenging_patrol", calendar::turn - 10_hours );
    if (comp == nullptr){
        return false;
    }
    int experience = rng( 5, 20 );
    if (one_in(4)){
        popup(_("While scavenging, %s's party suddenly found itself set upon by a large mob of undead..."), comp->name.c_str());
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_MELEE_NPC affects scavenging_patrol results

        ///\EFFECT_SURVIVAL_NPC affects scavenging_patrol results

        ///\EFFECT_BASHING_NPC affects scavenging_patrol results

        ///\EFFECT_CUTTING_NPC affects scavenging_patrol results

        ///\EFFECT_GUN_NPC affects scavenging_patrol results

        ///\EFFECT_STABBING_NPC affects scavenging_patrol results

        ///\EFFECT_UNARMED_NPC affects scavenging_patrol results

        ///\EFFECT_DODGE_NPC affects scavenging_patrol results
        int skill = comp->get_skill_level( skill_melee ) + (.5*comp->get_skill_level( skill_survival )) + comp->get_skill_level( skill_bashing ) +
            comp->get_skill_level( skill_cutting ) + comp->get_skill_level( skill_gun ) + comp->get_skill_level( skill_stabbing )
            + comp->get_skill_level( skill_unarmed ) + comp->get_skill_level( skill_dodge ) + 4;
        if (one_in(6)){
            popup(_("Through quick thinking the group was able to evade combat!"));
        } else {
            popup(_("Combat took place in close quarters, focusing on melee skills..."));
            int monsters = rng( 8, 30 );
            if( skill * rng_float( .60, 1.4 ) > (.35 * monsters * rng_float( .6, 1.4 )) ) {
                popup(_("Through brute force the party smashed through the group of %d undead!"),
                      monsters);
                experience += rng ( 2, 10 );
            } else {
                popup(_("Unfortunately they were overpowered by the undead... I'm sorry."));
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }

    int money = rng( 25, 450 );
    g->u.cash += money*100;

    int y = 0;
    int i = 0;
    while (i < experience){
        y = rng( 0, 100 );
        if( y < 40 ){
            comp->practice( skill_survival, 10);
        } else if (y < 60){
            comp->practice( skill_mechanics, 10);
        } else if (y < 75){
            comp->practice( skill_electronics, 10);
        } else if (y < 81){
            comp->practice( skill_melee, 10);
        } else if (y < 86){
            comp->practice( skill_firstaid, 10);
        } else if (y < 90){
            comp->practice( skill_speech, 10);
        } else if (y < 92){
            comp->practice( skill_bashing, 10);
        } else if (y < 94){
            comp->practice( skill_stabbing, 10);
        } else if (y < 96){
            comp->practice( skill_cutting, 10);
        } else if (y < 98){
            comp->practice( skill_dodge, 10);
        } else {
            comp->practice( skill_unarmed, 10);
        }
        i++;
    }

    popup(_("%s returns from patrol having earned $%d and a fair bit of experience..."), comp->name.c_str(),money);
    if (one_in(10)){
        popup( _( "%s was impressed with %s's performance and gave you a small bonus ( $100 )" ), p.name.c_str(), comp->name.c_str() );
        g->u.cash += 10000;
    }
    if ( one_in( 10 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ){
        p.set_mutation( trait_NPC_MISSION_LEV_1 );
        popup( _( "%s feels more confident in your abilities and is willing to let you participate in daring raids." ), p.name.c_str() );
    }
    companion_return( *comp );
    return true;
}

bool talk_function::scavenging_raid_return( npc &p )
{
    npc *comp = companion_choose_return( p, "_scavenging_raid", calendar::turn - 10_hours );
    if (comp == nullptr){
        return false;
    }
    int experience = rng(10,20);
    if (one_in(2)){
        popup(_("While scavenging, %s's party suddenly found itself set upon by a large mob of undead..."), comp->name.c_str());
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_MELEE_NPC affects scavenging_raid results

        ///\EFFECT_SURVIVAL_NPC affects scavenging_raid results

        ///\EFFECT_BASHING_NPC affects scavenging_raid results

        ///\EFFECT_CUTTING_NPC affects scavenging_raid results

        ///\EFFECT_GUN_NPC affects scavenging_raid results

        ///\EFFECT_STABBING_NPC affects scavenging_raid results

        ///\EFFECT_UNARMED_NPC affects scavenging_raid results

        ///\EFFECT_DODGE_NPC affects scavenging_raid results
        int skill = comp->get_skill_level( skill_melee ) + (.5*comp->get_skill_level( skill_survival )) + comp->get_skill_level( skill_bashing ) +
            comp->get_skill_level( skill_cutting ) + comp->get_skill_level( skill_gun ) + comp->get_skill_level( skill_stabbing )
            + comp->get_skill_level( skill_unarmed ) + comp->get_skill_level( skill_dodge ) + 4;
        if (one_in(6)){
            popup(_("Through quick thinking the group was able to evade combat!"));
        } else {
            popup(_("Combat took place in close quarters, focusing on melee skills..."));
            int monsters = rng (8, 30);
            if( skill * rng_float( .60, 1.4 ) > (.35 * monsters * rng_float( .6, 1.4 )) ) {
                popup(_("Through brute force the party smashed through the group of %d undead!"), monsters);
                experience += rng( 2, 10 );
            } else {
                popup(_("Unfortunately they were overpowered by the undead... I'm sorry."));
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }
    //The loot value needs to be added to the faction - what the player is payed
    for (int i = 0; i < rng( 2, 3 ); i++){
        const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), "house", 0, false );
        overmap_buffer.reveal(site,2);
        loot_building(site);
    }

    int money = rng( 200, 900 );
    g->u.cash += money * 100;

    int y = 0;
    int i = 0;
    while (i < experience){
        y = rng( 0, 100 );
        if (y < 40){
            comp->practice( skill_survival, 10);
        } else if (y < 60){
            comp->practice( skill_mechanics, 10);
        } else if (y < 75){
            comp->practice( skill_electronics, 10);
        } else if (y < 81){
            comp->practice( skill_melee, 10);
        } else if (y < 86){
            comp->practice( skill_firstaid, 10);
        } else if (y < 90){
            comp->practice( skill_speech, 10);
        } else if (y < 92){
            comp->practice( skill_bashing, 10);
        } else if (y < 94){
            comp->practice( skill_stabbing, 10);
        } else if (y < 96){
            comp->practice( skill_cutting, 10);
        } else if (y < 98){
            comp->practice( skill_dodge, 10);
        } else {
            comp->practice( skill_unarmed, 10);
        }
        i++;
    }

    popup(_("%s returns from the raid having earned $%d and a fair bit of experience..."), comp->name.c_str(),money);
    if (one_in(20)){
        popup( _( "%s was impressed with %s's performance and gave you a small bonus ( $100 )" ), p.name.c_str(), comp->name.c_str() );
        g->u.cash += 10000;
    }
    if (one_in(2)){
        std::string itemlist = "npc_misc";
        if (one_in(8))
            itemlist = "npc_weapon_random";
        auto result = item_group::item_from( itemlist );
        if( !result.is_null() ) {
                popup(_("%s returned with a %s for you!"),comp->name.c_str(),result.tname().c_str());
                g->u.i_add( result );
        }
    }
    companion_return( *comp );
    return true;
}

bool talk_function::labor_return( npc &p )
{
    npc *comp = companion_choose_return( p, "_labor", calendar::turn - 1_hours );
    if (comp == nullptr){
        return false;
    }

    //@todo actually it's hours, not turns
    float turns = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int money = 8*turns;
    g->u.cash += money*100;

    companion_skill_trainer( *comp, "menial", calendar::turn - comp->companion_mission_time, 1 );

    popup(_("%s returns from working as a laborer having earned $%d and a bit of experience..."), comp->name.c_str(),money);
    companion_return( *comp );
    if ( turns >= 8 && one_in( 8 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ){
        p.set_mutation( trait_NPC_MISSION_LEV_1 );
        popup( _( "%s feels more confident in your companions and is willing to let them participate in advanced tasks." ), p.name.c_str() );
    }

    return true;
}

bool talk_function::om_camp_upgrade( npc &comp, const point omt_pos ){
    editmap edit;

    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, comp.posz() );
    std::string om_old = omt_ref.id().c_str();

    if (!edit.mapgen_set( om_next_upgrade(om_old), tripoint(omt_pos.x, omt_pos.y, comp.posz() ) ) ){
        return false;
    }

    tripoint np = tripoint( 5 * SEEX + 5 % SEEX, 4 * SEEY + 5 % SEEY, comp.posz() );
    np.x = g->u.posx()+1;
    np.y = g->u.posy();

    tripoint p( 0, 0, comp.posz() );
    int &x = p.x;
    int &y = p.y;
    for( x = 0; x < SEEX * g->m.getmapsize(); x++ ) {
        for( y = 0; y < SEEY * g->m.getmapsize(); y++ ) {
            if(g->m.ter(x, y) == ter_id( "t_floor_green" )) {
                np.x = x;
                np.y = y;
            }
        }
    }
    comp.setpos(np);
    comp.set_destination();
    g->load_npcs();
    return true;
}

std::string talk_function::om_upgrade_description( std::string bldg ){
    const recipe *making = &recipe_id( bldg ).obj();
    const inventory &total_inv = g->u.crafting_inventory();

    std::vector<std::string> component_print_buffer;
    int pane = FULL_SCREEN_WIDTH;
    auto tools = making->requirements().get_folded_tools_list( pane, c_white, total_inv, 1 );
    auto comps = making->requirements().get_folded_components_list( pane, c_white, total_inv, 1);
    component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
    component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

    std::string comp = "";
    for( auto &elem : component_print_buffer ) {
        comp = comp + elem + "\n";
    }
    comp = string_format( "Notes:\n%s\n \nSkill used: %s\nDifficulty: %d\n%s \nRisk: None\nTime: %s\n", making->description,
                         making->skill_used.obj().ident().c_str(), making->difficulty, comp, to_string( time_duration::from_turns( making->time / 100 ) ) );
    return comp;
}

std::string talk_function::om_craft_description( std::string itm ){
    recipe making = recipe_id( itm ).obj();
    const inventory &total_inv = g->u.crafting_inventory();

    std::vector<std::string> component_print_buffer;
    int pane = FULL_SCREEN_WIDTH;
    auto tools = making.requirements().get_folded_tools_list( pane, c_white, total_inv, 1 );
    auto comps = making.requirements().get_folded_components_list( pane, c_white, total_inv, 1);

    component_print_buffer.insert( component_print_buffer.end(), tools.begin(), tools.end() );
    component_print_buffer.insert( component_print_buffer.end(), comps.begin(), comps.end() );

    std::string comp = "";
    for( auto &elem : component_print_buffer ) {
        comp = comp + elem + "\n";
    }
    comp = string_format("Skill used: %s\nDifficulty: %d\n%s\nTime: %s\n", making.skill_used.obj().name(), making.difficulty,
                         comp, to_string( time_duration::from_turns( making.time / 100 ) ) );
    return comp;
}

std::string talk_function::om_gathering_description( npc &p, std::string bldg ){
    std::string itemlist;
    if (item_group::group_is_defined( "gathering_" + bldg )){
        itemlist = "gathering_" + bldg;
    } else {
        itemlist = "forest" ;
    }
    std::string output = _( "Notes: \nSend a companion to gather materials for the next camp upgrade.\n \n"
        "Skill used: survival\n"
        "Difficulty: N/A \n"
        "Gathering Possibilities:\n" );

    // Functions like the debug item group tester but only rolls 6 times so the player doesn't have perfect knowledge
    std::map<std::string, int> itemnames;
    for (size_t a = 0; a < 6; a++) {
        const auto items = item_group::items_from( itemlist, calendar::turn );
        for( auto &it : items ) {
            itemnames[it.display_name()]++;
        }
    }
    // Invert the map to get sorting!
    std::multimap<int, std::string> itemnames2;
    for (const auto &e : itemnames) {
        itemnames2.insert(std::pair<int, std::string>(e.second, e.first));
    }
    for (const auto &e : itemnames2) {
        output = output + "> " + e.second + "\n";
    }
    std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, "_faction_camp_gathering" );
    output = output + _( " \nRisk: Very Low\n"
            "Time: 3 Hours, Repeated\n"
            "Positions: " ) + to_string(npc_list.size()) +"/3\n";
    return output;
}

std::string talk_function::om_next_upgrade( std::string bldg ){
    std::string comp = "";
    int phase = bldg.find_last_of("_");
    comp = bldg.substr(phase+1);
    int value = atoi(comp.c_str()) + 1;
    comp = bldg.substr(0,phase+1) + to_string(value);
    if( !oter_str_id( comp ).is_valid() ){
        return "null";
    }

    return comp;
}

std::vector<std::string> talk_function::om_all_upgrade_levels( std::string bldg ){
    std::vector<std::string> upgrades;
    std::string comp = "";
    int phase = bldg.find_last_of("_");
    comp = bldg.substr(phase+1);
    int value = 0;
    int current = atoi(comp.c_str());
    while( value <= current ){
        comp = bldg.substr(0,phase+1) + to_string(value);
        if( oter_str_id( comp ).is_valid() ){
            upgrades.push_back( comp );
        }
        value++;
    }
    return upgrades;
}

bool talk_function::om_min_level( std::string target, std::string bldg ){
    return (om_over_level( target, bldg ) >= 0);
}

int talk_function::om_over_level( std::string target, std::string bldg ){
    std::string comp = "";
    int diff = 0;
    int phase_target = target.find_last_of("_");
    int phase_bldg = bldg.find_last_of("_");
    //comparing two different buildings
    if( target.substr(0,phase_target+1) != bldg.substr(0,phase_bldg+1)){
        return -1;
    }
    diff = atoi(bldg.substr(phase_bldg+1).c_str()) - atoi(target.substr(phase_target+1).c_str());
    //not high enough level
    if( diff < 0 ) {
        return -1;
    }
    return diff;
}

bool talk_function::upgrade_return( npc &p, point omt_pos, std::string miss  )
{
    //Ensure there are no vehicles before we update
    editmap edit;
    if( edit.mapgen_veh_has( tripoint( omt_pos.x, omt_pos.y, p.posz() ) ) ){
        popup( _("Engine cannot support merging vehicles from two overmaps, please remove them from the OM tile.") );
        return false;
    }

    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, p.posz() );
    std::string bldg = omt_ref.id().c_str();
    if( bldg == "field" ){
        bldg = "faction_base_camp_1";
    } else {
        bldg = om_next_upgrade(bldg);
    }
    const recipe *making = &recipe_id( bldg ).obj();

    npc *comp = companion_choose_return( p, miss, calendar::turn - time_duration::from_turns( making->time / 100 ) );

    if (comp == nullptr || !om_camp_upgrade( p, omt_pos ) ){
        return false;
    }
    companion_skill_trainer( *comp, "construction", time_duration::from_turns( making->time / 100 ), making->difficulty );
    popup(_("%s returns from upgrading the camp having earned a bit of experience..."), comp->name.c_str());
    companion_return( *comp );
    return true;
}

bool talk_function::camp_gathering_return( npc &p, std::string task )
{
    npc *comp = companion_choose_return( p, task, calendar::turn - 3_hours );
    if( comp == nullptr ){
        return false;
    }

    if (one_in(20)){
        popup(_("While gathering supplies, a silent specter approaches %s..."), comp->name.c_str());
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_SURVIVAL_NPC affects gathering mission results

        ///\EFFECT_DODGE_NPC affects gathering mission results
        int skill_1 = comp->get_skill_level( skill_survival );
        int skill_2 = comp->get_skill_level( skill_speech );
        if( skill_1 > rng( -2, 8 ) ){
            popup(_("%s notices the antlered horror and slips away before if gets too close."), comp->name.c_str());
        } else if( skill_2 > rng( -2, 8 ) ) {
            popup(_("The survivor approaches %s asking for directions."), comp->name.c_str());
            popup(_("Fearful that he may be an agent of some hostile faction, %s doesn't mention the camp."), comp->name.c_str());
            popup(_("The two part on friendly terms and the survivor isn't seen again."));
        } else {
            popup(_("%s didn't detect the ambush until it was too late!"), comp->name.c_str());
            // the following doxygen aliases do not yet exist. this is marked for future reference

            ///\EFFECT_MELEE_NPC affects forage mission results

            ///\EFFECT_SURVIVAL_NPC affects forage mission results

            ///\EFFECT_BASHING_NPC affects forage mission results

            ///\EFFECT_CUTTING_NPC affects forage mission results

            ///\EFFECT_STABBING_NPC affects forage mission results

            ///\EFFECT_UNARMED_NPC affects forage mission results

            ///\EFFECT_DODGE_NPC affects forage mission results
            int skill = comp->get_skill_level( skill_melee ) + (.5*comp->get_skill_level( skill_survival )) + comp->get_skill_level( skill_bashing ) +
            comp->get_skill_level( skill_cutting ) + comp->get_skill_level( skill_stabbing ) + comp->get_skill_level( skill_unarmed )
            + comp->get_skill_level( skill_dodge );
            int monsters = rng( 0, 10 );
            if( skill * rng_float( .80, 1.2 ) > (monsters * rng_float( .8, 1.2 )) ){
                if( one_in(2) ){
                    popup(_("The bull moose charged %s from the tree line..."), comp->name.c_str());
                    popup(_("Despite being caught off guard %s was able to run away until the moose gave up pursuit."), comp->name.c_str());
                } else {
                    popup(_("The jabberwock grabbed %s by the arm from behind and began to scream."), comp->name.c_str());
                    popup(_("Terrified, %s spun around and delivered a massive kick to the creature's torso..."), comp->name.c_str());
                    popup(_("Collapsing into a pile of gore, %s walked away unscathed..."), comp->name.c_str());
                    popup(_("(Sounds like bullshit, you wander what really happened.)"));
                }
            } else {
                if (one_in(2)){
                    popup(_("%s turned to find the hideous black eyes of a giant wasp staring back from only a few feet away..."), comp->name.c_str());
                    popup(_("The screams were terrifying, there was nothing anyone could do."));
                } else {
                    popup(_("Pieces of %s were found strewn across a few bushes."), comp->name.c_str());
                    popup(_("(You wander if your companions are fit to work on their own...)"));
                }
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }

    int exp = to_hours<float>( calendar::turn - comp->companion_mission_time );
    std::string skill_group = "gathering";
    companion_skill_trainer( *comp, skill_group, calendar::turn - comp->companion_mission_time , 1 );

    popup(_("%s returns from gathering materials carrying supplies and has a bit more experience..."), comp->name.c_str());
    // the following doxygen aliases do not yet exist. this is marked for future reference

    ///\EFFECT_SURVIVAL_NPC affects forage mission results
    int skill = comp->get_skill_level( skill_survival );

    int need_food = time_to_food( calendar::turn - comp->companion_mission_time );
    if( camp_food_supply() < need_food ){
        popup( _("Your companion seems disappointed that your pantry is empty...") );
    }
    camp_food_supply( -need_food );

    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    oter_id &omt_ref = overmap_buffer.ter( omt_pos.x, omt_pos.y, p.posz() );
    std::string om_tile = omt_ref.id().c_str();
    std::string itemlist = "forest";

    if (task == "_faction_camp_gathering"){
        if (item_group::group_is_defined( "gathering_" + om_tile )){
            itemlist = "gathering_" + om_tile ;
        }
    }
    if (task == "_faction_camp_firewood"){
        itemlist = "gathering_faction_base_camp_firewood";
    }

    int i = 0;
    while( i < exp ){
        if( skill > rng_float( -.5, 15 ) ) {
            auto result = item_group::item_from( itemlist );
            g->m.add_item_or_charges( g->u.pos(), result, true );
            i += 2;
        }
    }
    companion_return( *comp );
    return true;
}


bool talk_function::camp_garage_chop_start( npc &p, std::string task )
{
    std::string dir = camp_direction( task );
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    tripoint omt_trg;
    std::vector<std::pair<std::string, tripoint>> om_expansions = om_building_region( p, 1, true );
    for( const auto &e : om_expansions ){
        if( dir == om_simple_dir( omt_pos, e.second ) ){
            omt_trg = e.second;
        }
    }

    oter_id &omt_ref = overmap_buffer.ter( omt_trg.x, omt_trg.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    editmap edit;
    vehicle *car = edit.mapgen_veh_query( omt_trg );
    if (car == nullptr){
        return false;
    }

    if (!query_yn(_("       Chopping this vehicle:\n%s"), camp_car_description(car) ) ) {
        return false;
    }

    npc* comp = individual_mission(p, _("begins working in the garage..."),
                                   "_faction_exp_chop_shop_"+dir, false, {}, "mechanics", 2 );
    if( comp == nullptr ){
        return false;
    }

    //Chopping up the car!
    std::vector<vehicle_part> p_all = car->parts;
    int prt = 0;
    int skillLevel = comp->get_skill_level( skill_mechanics );
    while( p_all.size() > 0 ){
        vehicle_stack contents = car->get_items( prt );
        for( auto iter = contents.begin(); iter != contents.end(); ) {
            comp->companion_mission_inv.add_item(*iter);
            iter = contents.erase( iter );
        }
        bool broken = p_all[ prt ].is_broken();
        bool skill_break = false;
        bool skill_destroy = false;

        int dice = rng( 1, 20);
        dice += skillLevel - p_all[ prt].info().difficulty;

        if ( dice >= 20 ){
            skill_break = false;
            skill_destroy = false;
            companion_skill_trainer( *comp, skill_mechanics, 1_hours, p_all[ prt].info().difficulty );
        } else if ( dice > 15 ){
            skill_break = false;
        } else if ( dice > 9 ){
            skill_break = true;
            skill_destroy = false;
        } else {
            skill_break = true;
            skill_destroy = true;
        }

        if( !broken && !skill_break ) {
            //Higher level garages will salvage liquids from tanks
            if( !p_all[prt].is_battery() ){
                p_all[prt].ammo_consume( p_all[prt].ammo_capacity(), car->global_part_pos3( p_all[prt] ) );
            }
            comp->companion_mission_inv.add_item(p_all[prt].properties_to_item());
        } else if( !skill_destroy ) {
            car->break_part_into_pieces(prt, comp->posx(), comp->posy());
        }
        p_all.erase( p_all.begin() + 0 );
    }
    companion_skill_trainer( *comp, skill_mechanics, 5_days, 2 );
    edit.mapgen_veh_destroy( omt_trg, car );
    return true;
}

bool talk_function::camp_farm_return( npc &p, std::string task, bool harvest, bool plant, bool plow)
{
    std::string dir = camp_direction( task );
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    tripoint omt_trg;
    std::vector<std::pair<std::string, tripoint>> om_expansions = om_building_region( p, 1, true );
    for( const auto &e : om_expansions ){
        if( dir == om_simple_dir( omt_pos, e.second ) ){
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
    for (int x = 0; x < 23; x++){
        for (int y = 0; y < 23; y++){
            //Needs to be plowed to match json
            if (bay_json.ter(x,y) == ter_str_id( "t_dirtmound" )
                && (bay.ter(x,y) == ter_str_id( "t_dirt" ) || bay.ter(x,y) == ter_str_id( "t_grass" ))
                && bay.furn(x,y) == furn_str_id( "f_null" ) ){
                plots_plow++;
            }
            if (bay.ter(x,y) == ter_str_id( "t_dirtmound" ) && bay.furn(x,y) == furn_str_id( "f_null" )){
                plots_empty++;
            }
            if (bay.furn(x,y) == furn_str_id( "f_plant_harvest" ) && !bay.i_at(x,y).empty()){
                const item &seed = bay.i_at( x,y )[0];
                if( seed.is_seed() ) {
                    harvestable++;
                }
            }
        }
    }
    time_duration work = 0_minutes;
    if (harvest){
        work += 3_minutes * harvestable;
    }
    if (plant){
        work += 1_minutes * plots_empty;
    }
    if (plow){
        work += 5_minutes * plots_plow;
    }

    npc *comp = companion_choose_return( p, task, calendar::turn - work );
    if( comp == nullptr ){
        return false;
    }

    std::vector<item *> seed_inv_tmp = comp->companion_mission_inv.items_with( []( const item &itm ) {
        return itm.is_seed();
    } );

    std::vector<item *> seed_inv;
    for( auto &seed : seed_inv_tmp ) {
        if (seed->typeId() !=  "marloss_seed" && seed->typeId() !=  "fungal_seeds"){
            seed_inv.push_back(seed);
        }
    }

    if( plant && seed_inv.empty() ) {
        popup(_("No seeds to plant!"));
    }

    //Now that we know we have spent enough time working, we can update the map itself.
    for (int x = 0; x < 23; x++){
        for (int y = 0; y < 23; y++){
            //Needs to be plowed to match json
            if ( plow && bay_json.ter(x,y) == ter_str_id( "t_dirtmound" )
                && (bay.ter(x,y) == ter_str_id( "t_dirt" ) || bay.ter(x,y) == ter_str_id( "t_grass" ))
                && bay.furn(x,y) == furn_str_id( "f_null" ) ){
                bay.ter_set(x,y, t_dirtmound);
            }
            if ( plant && bay.ter(x,y) == ter_str_id( "t_dirtmound" ) && bay.furn(x,y) == furn_str_id( "f_null" )){
                if( !seed_inv.empty() ){
                    item *tmp_seed = seed_inv.back();
                    seed_inv.pop_back();
                    std::list<item> used_seed;
                    if( item::count_by_charges( tmp_seed->typeId() ) ) {
                        used_seed.push_back(*tmp_seed);
                        tmp_seed->charges -= 1;
                        if( tmp_seed->charges > 0){
                            seed_inv.push_back(tmp_seed);
                        }
                    }
                    used_seed.front().set_age( 0 );
                    bay.add_item_or_charges( x, y, used_seed.front() );
                    bay.set( x, y, t_dirt, f_plant_seed);
                }
            }
            if ( harvest && bay.furn(x,y) == furn_str_id( "f_plant_harvest" ) && !bay.i_at(x,y).empty()){
                const item &seed = bay.i_at( x,y )[0];
                if( seed.is_seed() && seed.typeId() != "fungal_seeds" && seed.typeId() != "marloss_seed") {
                    const itype &type = *seed.type;
                    int skillLevel = comp->get_skill_level( skill_survival );
                    ///\EFFECT_SURVIVAL increases number of plants harvested from a seed
                    int plantCount = rng(skillLevel / 2, skillLevel);
                    //this differs from
                    if (plantCount >= 9) {
                        plantCount = 9;
                    } else if( plantCount <= 0 ) {
                        plantCount = 1;
                    }
                    const int seedCount = std::max( 1l, rng( plantCount / 4, plantCount / 2 ) );
                    for( auto &i : iexamine::get_harvest_items( type, plantCount, seedCount, true ) ) {
                        g->m.add_item_or_charges( g->u.posx(), g->u.posy(), i );
                    }
                    bay.i_clear( x,y );
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
            if( it.charges > 0 ){
                g->u.i_add( it );
            }
        }
    }
    comp->companion_mission_inv.clear();

    int need_food = time_to_food( work );
    if( camp_food_supply() < need_food ){
        popup( _("Your companion seems disappointed that your pantry is empty...") );
    }
    camp_food_supply( -need_food );
    companion_skill_trainer( *comp, skill_survival, work, 2 );

    popup(_("%s returns from working your fields..."), comp->name.c_str());
    companion_return( *comp );
    return true;
}

bool talk_function::camp_menial_return( npc &p )
{
    npc *comp = companion_choose_return( p, "_faction_camp_menial", calendar::before_time_starts );
    if( comp == nullptr ){
        return false;
    }

    companion_skill_trainer( *comp, "menial", 3_hours, 2 );

    popup(_("%s returns from doing the dirty work to keep the camp running..."), comp->name.c_str());
    if( p.companion_mission_points.size() < COMPANION_SORT_POINTS ){
        popup( _("Sorting points have changed, forcing reset.") );
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
    for( size_t x = 0; x < p.companion_mission_points.size() ; x++ ){
        if( g->m.furn(p.companion_mission_points[x]) == f_null ){
            g->m.furn_set(p.companion_mission_points[x], f_ash);
        }
    }
    for( const tripoint &tmp : g->m.points_in_radius( g->u.pos(), 72 ) ) {
        if( !g->m.has_furn( tmp ) ) {
            for( auto &i : g->m.i_at( tmp ) ) {
                if( !i.made_of( LIQUID ) ) {
                    if( i.is_comestible() && i.rotten() ){
                        g->m.add_item_or_charges( p_trash, i, true );
                    } else if( i.is_seed() ){
                        g->m.add_item_or_charges( p_seed, i, true );
                    } else if( i.is_food() ){
                        g->m.add_item_or_charges( p_food, i, true );
                    } else if( i.is_corpse() ){
                        g->m.add_item_or_charges( p_trash, i, true );
                    } else if( i.is_book() ){
                        g->m.add_item_or_charges( p_book, i, true );
                    } else if( i.is_bionic() ){
                        g->m.add_item_or_charges( p_bionic, i, true );
                    } else if( i.is_medication() ){
                        g->m.add_item_or_charges( p_medication, i, true );
                    } else if( i.is_tool() ){
                        g->m.add_item_or_charges( p_tool, i, true );
                    } else if( i.is_gun() ){
                        g->m.add_item_or_charges( p_weapon, i, true );
                    } else if( i.is_ammo() ){
                        g->m.add_item_or_charges( p_ammo, i, true );
                    } else if( i.is_armor() ){
                        g->m.add_item_or_charges( p_clothing, i, true );
                    } else if( i.typeId() == "log" || i.typeId() == "splinter" || i.typeId() == "stick" || i.typeId() == "2x4" ){
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
    for( size_t x = 0; x < p.companion_mission_points.size() ; x++ ){
        if( g->m.furn(p.companion_mission_points[x]) == f_ash ){
            g->m.furn_set(p.companion_mission_points[x], f_null);
        }
    }
    companion_return( *comp );
    return true;
}

bool talk_function::camp_expansion_select( npc &p )
{
    npc *comp = companion_choose_return( p, "_faction_camp_expansion", calendar::before_time_starts );
    if( comp == nullptr ){
        return false;
    }
    std::vector<std::string> pos_expansion_name_id;
    std::vector<std::string> pos_expansion_name;
    std::map<std::string,std::string> pos_expansions = recipe_group::get_recipes( "all_faction_base_expansions" );
    for( std::map<std::string,std::string>::const_iterator it = pos_expansions.begin(); it != pos_expansions.end(); ++it ) {
        pos_expansion_name.push_back( it->first );
        pos_expansion_name_id.push_back( it->second );
    }
    pos_expansion_name.push_back( _("Cancel") );

    int expan = menu_vec(true, _("Select an expansion:"), pos_expansion_name) - 1;
    int sz = pos_expansion_name.size();
    if (expan < 0 || expan >= sz) {
        popup("You choose to wait...");
        return false;
    }
    editmap edit;
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    if ( !edit.mapgen_set( pos_expansion_name_id[expan], tripoint(omt_pos.x, omt_pos.y, p.posz()), 1 ) ){
        return false;
    }
    companion_skill_trainer( *comp, "construction", 3_hours, 2 );
    popup(_("%s returns from surveying for the expansion."), comp->name.c_str());
    companion_return( *comp );
    return true;
}

bool talk_function::camp_distribute_food( npc &p )
{
    if( p.companion_mission_points.size() < COMPANION_SORT_POINTS ){
        popup( _("Sorting points have changed, forcing reset.") );
        camp_menial_sort_pts( p, true, true );
    }
    tripoint p_food_stock = p.companion_mission_points[1];
    tripoint p_trash = p.companion_mission_points[8];
    tripoint p_litter = tripoint(p.posx()-7, p.posy(), p.posz());
    tripoint p_tool = p.companion_mission_points[6];

    if ( g->m.i_at( p_food_stock ).empty() ){
        popup( _("No items are located at the drop point...") );
        return false;
    }

    int total = 0;
    for( auto &i : g->m.i_at( p_food_stock ) ) {
        if ( i.is_container() && i.get_contained().is_food() ) {
            auto comest = i.get_contained();
            i.contents.clear();
            //NPCs are lazy bastards who leave empties all around the camp fire
            tripoint litter_spread = p_litter;
            litter_spread.x += rng(-3,3);
            litter_spread.y += rng(-3,3);
            i.on_contents_changed();
            g->m.add_item_or_charges( litter_spread, i, false );
            i = comest;
        }
        if( i.is_comestible() && ( i.rotten() || i.type->comestible->fun < -6) ){
            g->m.add_item_or_charges( p_trash, i, false );
        } else if( i.is_food() ){
            float rot_multip;
            int rots_in = to_days<int>( time_duration::from_turns( i.spoilage_sort_order() ) );
            if ( rots_in >= 5 ){
                rot_multip = 1.00;
            } else if ( rots_in >= 2 ) {
                rot_multip = .80;
            } else {
                rot_multip = .60;
            }
            if ( i.count_by_charges() ){
                total += i.type->comestible->nutr * i.charges * rot_multip;
            } else {
                total += i.type->comestible->nutr * rot_multip;
            }
        } else if( i.is_corpse() ){
            g->m.add_item_or_charges( p_trash, i, false );
        } else {
            g->m.add_item_or_charges( p_tool, i, false );
        }
    }
    g->m.i_clear( p_food_stock );
    popup(_("You distribute %d kcal worth of food to your companions."), total * 10);
    camp_food_supply( total );
    return true;
}

std::vector<std::pair<std::string, tripoint>> talk_function::om_building_region( npc &p, int range, bool purge)
{
    std::vector<std::pair<std::string, tripoint>> om_camp_region;
    for( int x = -range; x <= range; x++){
        for( int y = -range; y <= range; y++){
            const point omt_near = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy()) );
            oter_id &omt_rnear = overmap_buffer.ter( omt_near.x+ x, omt_near.y + y, p.posz() );
            std::string om_near = omt_rnear.id().c_str();
            om_camp_region.push_back(std::make_pair( om_near, tripoint( omt_near.x + x, omt_near.y + y, p.posz() ) ));
        }
    }
    if( purge ){
        std::vector<std::pair<std::string, tripoint>> om_expansions;
        for( const auto &e : om_camp_region ){
            if ( e.first.find("faction_base_") != std::string::npos && e.first.find("faction_base_camp") == std::string::npos ){
                om_expansions.push_back(e);
            }
        }
        return om_expansions;
    }
    return om_camp_region;
}

std::string talk_function::om_simple_dir( point omt_pos, tripoint omt_tar )
{
    std::string dr = "[";
    if( omt_tar.y < omt_pos.y){
        dr += "N";
    }
    if( omt_tar.y > omt_pos.y){
        dr += "S";
    }
    if( omt_tar.x < omt_pos.x){
        dr += "W";
    }
    if( omt_tar.x > omt_pos.x){
        dr += "E";
    }
    dr += "]";
    if( omt_tar.x == omt_pos.x && omt_tar.y == omt_pos.y ){
        return "[B]";
    }
    return dr;
}

std::string talk_function::camp_farm_description( point omt_pos, bool harvest, bool plots, bool plow )
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
    for (int x = 0; x < 23; x++){
        for (int y = 0; y < 23; y++){
            //Needs to be plowed to match json
            if (bay_json.ter(x,y) == ter_str_id( "t_dirtmound" )
                && (bay.ter(x,y) == ter_str_id( "t_dirt" ) || bay.ter(x,y) == ter_str_id( "t_grass" ))
                && bay.furn(x,y) == furn_str_id( "f_null" ) ){
                plots_plow++;
            }
            if (bay.ter(x,y) == ter_str_id( "t_dirtmound" ) && bay.furn(x,y) == furn_str_id( "f_null" )){
                plots_empty++;
            }
            if (bay.furn(x,y) == furn_str_id( "f_plant_harvest" ) && !bay.i_at(x,y).empty()){
                const item &seed = bay.i_at( x,y )[0];
                if( seed.is_seed() ) {
                    harvestable++;
                    const islot_seed &seed_data = *seed.type->seed;
                    item tmp = item( seed_data.fruit_id, calendar::turn );
                    bool check = false;
                    for( auto elem : plant_names ) {
                        if (elem == tmp.type_name(3).c_str())
                            check = true;
                    }
                    if (!check){
                        plant_names.push_back( tmp.type_name(3).c_str() );
                    }
                }
            }
        }
    }

    std::string crops;
    int total_c = 0;
    for( auto i: plant_names ){
        if( total_c < 5 ){
            crops += "\t"+i+" \n";
            total_c++;
        } else if( total_c == 5 ) {
            crops += "+ more \n";
            total_c++;
        }
    }
    std::string entry;
    if( harvest ){
        entry += "Harvestable: " + to_string(harvestable) + " \n" + crops;
    }
    if( plots ){
        entry += "Ready for Planting: " + to_string(plots_empty) + " \n";
    }
    if( plow ){
        entry += "Needs Plowing: " + to_string(plots_plow) + " \n";
    }
    return entry;
}

std::string talk_function::camp_car_description( vehicle *car )
{
    std::string entry = string_format("Name:     %25s\n", car->name );
    entry += "----          Engines          ----\n";
    std::vector<vehicle_part *> part_engines = car->get_parts( "ENGINE" );
    for( auto pt: part_engines ){
        const vpart_info &vp = pt->info();
        entry += string_format("Enginge:  %25s\n", vp.name() );
        entry += string_format(">Status:  %24d%%\n", int( 100.0 * pt->hp() / vp.durability ) );
        entry += string_format(">Fuel:    %25s\n", vp.fuel_type );
    }
    std::map<itype_id, long> fuels = car->fuels_left();
    entry += "----  Fuel Storage & Battery   ----\n";
    for (std::map<itype_id, long>::iterator it=fuels.begin(); it!=fuels.end(); ++it){
        std::string fuel_entry = string_format("%d/%d", car->fuel_left(it->first), car->fuel_capacity(it->first) );
        entry += string_format(">%s:%*s\n", item(it->first).tname(), 33-item(it->first).tname().length(), fuel_entry );
    }
    std::vector<vehicle_part> part_all = car->parts;
    for( auto pt: part_all ){
        if( pt.is_battery() ){
            const vpart_info &vp = pt.info();
            entry += string_format(">%s:%*d%%\n", vp.name(), 32-vp.name().length(),
                                   int( 100.0 * pt.ammo_remaining() / pt.ammo_capacity() ) );
        }
    }
    entry += " \n";
    entry += "Estimated Chop Time:         5 Days\n";
    return entry;
}

std::string talk_function::camp_direction ( std::string line )
{
    return line.substr( line.find_last_of("["), line.find_last_of("]") - line.find_last_of("[") +1 );
}

int talk_function::camp_food_supply( int change, bool return_days )
{
    faction *yours = g->faction_manager_ptr->get( faction_id( "your_followers" ) );
    yours->food_supply += change;
    if ( yours->food_supply < 0 ){
        yours->likes_u += yours->food_supply / 500;
        yours->respects_u += yours->food_supply / 100;
        yours->food_supply = 0;
    }
    if( return_days ){
        return 10 * yours->food_supply /2600;
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
    return (260/24.0) * to_hours<float>( work );
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

bool talk_function::carpenter_return( npc &p )
{
    npc *comp = companion_choose_return( p, "_carpenter", calendar::turn - 1_hours );
    if( comp == nullptr ){
        return false;
    }

    if (one_in(20)){
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_FABRICATION_NPC affects carpenter mission results

        ///\EFFECT_DODGE_NPC affects carpenter mission results

        ///\EFFECT_SURVIVAL_NPC affects carpenter mission results
        int skill_1 = comp->get_skill_level( skill_fabrication );
        int skill_2 = comp->get_skill_level( skill_dodge );
        int skill_3 = comp->get_skill_level( skill_survival );
        popup(_("While %s was framing a building one of the walls began to collapse..."), comp->name.c_str());
        if( skill_1 > rng( 1, 8 ) ){
            popup(_("In the blink of an eye, %s threw a brace up and averted a disaster."), comp->name.c_str());
        } else if( skill_2 > rng( 1, 8 ) ) {
            popup(_("Darting out a window, %s escaped the collapse."), comp->name.c_str());
        } else if( skill_3 > rng( 1, 8 ) ) {
            popup(_("%s didn't make it out in time..."), comp->name.c_str());
            popup(_("but %s was rescued from the debris with only minor injuries!"), comp->name.c_str());
        } else {
            popup(_("%s didn't make it out in time..."), comp->name.c_str());
            popup(_("Everyone who was trapped under the collapsing roof died..."));
            popup(_("I'm sorry, there is nothing we could do."));
            overmap_buffer.remove_npc( comp->getID() );
            return false;
        }
    }

    //@todo actually it's hours, not turns
    float turns = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int money = 12*turns;
    g->u.cash += money*100;

    companion_skill_trainer( *comp, "construction", calendar::turn - comp->companion_mission_time, 2 );

    popup(_("%s returns from working as a carpenter having earned $%d and a bit of experience..."), comp->name.c_str(),money);
    companion_return( *comp );
    return true;
}

bool talk_function::forage_return( npc &p )
{
    npc *comp = companion_choose_return( p, "_forage", calendar::turn - 4_hours );
    if( comp == nullptr ){
        return false;
    }

    if (one_in(10)){
        popup(_("While foraging, a beast began to stalk %s..."), comp->name.c_str());
        // the following doxygen aliases do not yet exist. this is marked for future reference

        ///\EFFECT_SURVIVAL_NPC affects forage mission results

        ///\EFFECT_DODGE_NPC affects forage mission results
        int skill_1 = comp->get_skill_level( skill_survival );
        int skill_2 = comp->get_skill_level( skill_dodge );
        if( skill_1 > rng( -2, 8 ) ){
            popup(_("Alerted by a rustle, %s fled to the safety of the outpost!"), comp->name.c_str());
        } else if( skill_2 > rng( -2, 8 ) ) {
            popup(_("As soon as the cougar sprang %s darted to the safety of the outpost!"), comp->name.c_str());
        } else {
            popup(_("%s was caught unaware and was forced to fight the creature at close range!"), comp->name.c_str());
            // the following doxygen aliases do not yet exist. this is marked for future reference

            ///\EFFECT_MELEE_NPC affects forage mission results

            ///\EFFECT_SURVIVAL_NPC affects forage mission results

            ///\EFFECT_BASHING_NPC affects forage mission results

            ///\EFFECT_CUTTING_NPC affects forage mission results

            ///\EFFECT_STABBING_NPC affects forage mission results

            ///\EFFECT_UNARMED_NPC affects forage mission results

            ///\EFFECT_DODGE_NPC affects forage mission results
            int skill = comp->get_skill_level( skill_melee ) + (.5*comp->get_skill_level( skill_survival )) + comp->get_skill_level( skill_bashing ) +
            comp->get_skill_level( skill_cutting ) + comp->get_skill_level( skill_stabbing ) + comp->get_skill_level( skill_unarmed )
            + comp->get_skill_level( skill_dodge );
            int monsters = rng( 0, 10 );
            if( skill * rng_float( .80, 1.2 ) > (monsters * rng_float( .8, 1.2 )) ){
                if( one_in(2) ){
                    popup(_("%s was able to scare off the bear after delivering a nasty blow!"), comp->name.c_str());
                } else {
                    popup(_("%s beat the cougar into a bloody pulp!"), comp->name.c_str());
                }
            } else {
                if (one_in(2)){
                    popup(_("%s was able to hold off the first wolf but the others that were sulking in the tree line caught up..."), comp->name.c_str());
                    popup(_("I'm sorry, there wasn't anything we could do..."));
                } else {
                    popup(_("We... we don't know what exactly happened but we found %s's gear ripped and bloody..."), comp->name.c_str());
                    popup(_("I fear your companion won't be returning."));
                }
                overmap_buffer.remove_npc( comp->getID() );
                return false;
            }
        }
    }

    //@todo actually it's hours, not turns
    float turns = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int money = 10*turns;
    g->u.cash += money*100;

    companion_skill_trainer( *comp, "gathering", calendar::turn - comp->companion_mission_time, 2 );

    popup(_("%s returns from working as a forager having earned $%d and a bit of experience..."), comp->name.c_str(),money);
    // the following doxygen aliases do not yet exist. this is marked for future reference

    ///\EFFECT_SURVIVAL_NPC affects forage mission results
    int skill = comp->get_skill_level( skill_survival );
    if( skill > rng_float( -.5, 8 ) ) {
        std::string itemlist = "farming_seeds";
        if (one_in(2)){
            switch( season_of_year( calendar::turn ) ) {
                case SPRING:
                    itemlist = "forage_spring";
                    break;
                case SUMMER:
                    itemlist = "forage_summer";
                    break;
                case AUTUMN:
                    itemlist = "forage_autumn";
                    break;
                case WINTER:
                    itemlist = "forage_winter";
                    break;
            }
        }
        auto result = item_group::item_from( itemlist );
        if( !result.is_null() ) {
                popup(_("%s returned with a %s for you!"),comp->name.c_str(),result.tname().c_str());
                g->u.i_add( result );
        }
        if ( one_in( 6 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ){
            p.set_mutation( trait_NPC_MISSION_LEV_1 );
            popup( _( "%s feels more confident in your companions and is willing to let them participate in advanced tasks." ), p.name.c_str() );
        }

    }
    companion_return( *comp );
    return true;
}

bool talk_function::companion_om_combat_check( std::vector<std::shared_ptr<npc>> group, tripoint om_tgt, bool try_engage){
    if( overmap_buffer.is_safe( om_tgt ) ){
        //Should work but is_safe is always returning true regardless of what is there.
        //return true;
    }

    //If the map isn't generated we need to do that...
    if( MAPBUFFER.lookup_submap( om_to_sm_copy(om_tgt) ) == NULL ){
        //This doesn't gen monsters...
        //tinymap tmpmap;
        //tmpmap.generate( om_tgt.x * 2, om_tgt.y * 2, om_tgt.z, calendar::turn );
        //tmpmap.save();
    }

    tinymap target_bay;
    target_bay.load( om_tgt.x * 2, om_tgt.y * 2, om_tgt.z, false );
    std::vector< monster * > monsters_around;
    for( int x = 0; x < 2; x++ ) {
        for( int y = 0; y < 2; y++ ) {

            point sm( (om_tgt.x * 2) + x, (om_tgt.x * 2) + y );
            const point omp = sm_to_om_remain( sm );
            overmap &omi = overmap_buffer.get( omp.x, omp.y );

            const tripoint current_submap_loc( om_tgt.x * 2 + x, om_tgt.y * 2 + y, om_tgt.z );
            auto monster_bucket = omi.monster_map.equal_range( current_submap_loc );
            std::for_each( monster_bucket.first, monster_bucket.second, [&](std::pair<const tripoint, monster> &monster_entry ) {
                monster &this_monster = monster_entry.second;
                monsters_around.push_back( &this_monster );
            } );
        }
    }
    float avg_survival = 0;
    for( auto guy : group ){
        avg_survival += guy->get_skill_level( skill_survival );
    }
    avg_survival = avg_survival / group.size();

    monster mon;

    std::vector< monster * > monsters_fighting;
    for( auto mons : monsters_around ){
        if( mons->get_hp() <= 0 ){
            continue;
        }
        int d_modifier = avg_survival - mons->type->difficulty;
        int roll = rng( 1, 20 ) + d_modifier;
        if( roll > 10 ){
            if( try_engage ){
                mons->death_drops = false;
                monsters_fighting.push_back( mons );
            }
        } else {
            mons->death_drops = false;
            monsters_fighting.push_back( mons );
        }
    }

    if( !monsters_fighting.empty() ){
        bool outcome = force_on_force( group, "Patrol", monsters_fighting, "attacking monsters", rng( -1, 2 ) );
        for( auto mons : monsters_fighting ){
            mons->death_drops = true;
        }
        return outcome;
    }
    return true;
}

bool talk_function::force_on_force( std::vector<std::shared_ptr<npc>> defender, const std::string &def_desc,
    std::vector< monster * > monsters_fighting, const std::string &att_desc, int advantage )
{
    std::string adv = "";
    if (advantage < 0){
        adv = ", attacker advantage";
    } else if (advantage > 0){
        adv = ", defender advantage";
    }
    //Find out why your followers don't have your faction...
    popup(_("Engagement between %d members of %s %s and %d %s%s!"),
        defender.size(), g->faction_manager_ptr->get( faction_id( "your_followers" ) )->name.c_str(), def_desc.c_str(),
        monsters_fighting.size(), att_desc.c_str(), adv.c_str());
    int defense = 0;
    int attack = 0;
    int att_init = 0;
    int def_init = 0;
    while (true){
        std::vector< monster * > remaining_mon;
        for( const auto &elem : monsters_fighting ) {
            if (elem->get_hp() > 0){
                remaining_mon.push_back(elem);
            }
        }
        std::vector<std::shared_ptr<npc>> remaining_def;
        for( const auto &elem : defender ) {
            if( !elem->is_dead() && elem->hp_cur[hp_torso] >= 0 && elem->hp_cur[hp_head] >= 0 ){
                remaining_def.push_back(elem);
            }
        }

        defense = combat_score(remaining_def);
        attack = combat_score(remaining_mon);
        if (attack > defense*3){
            attack_random( remaining_mon, remaining_def);
            if (defense == 0 || (remaining_def.size() == 1 && remaining_def[0]->is_dead() )){
                //Here too...
                popup(_("%s forces are destroyed!"), g->faction_manager_ptr->get( faction_id( "your_followers" ) )->name.c_str() );
            } else {
                //Again, no faction for your followers
                popup(_("%s forces retreat from combat!"), g->faction_manager_ptr->get( faction_id( "your_followers" ) )->name.c_str() );
            }
            return false;
        } else if (attack*3 < defense) {
            attack_random( remaining_def, remaining_mon);
            if (attack == 0 || (remaining_mon.size() == 1 && remaining_mon[0]->get_hp() == 0 )){
                popup(_("The monsters are destroyed!") );
            } else {
                popup(_("The monsters disengage!") );
            }
            return true;
        } else {
            def_init = rng( 1, 6 ) + advantage;
            att_init = rng( 1, 6 );
            if (def_init >= att_init) {
                attack_random( remaining_mon, remaining_def);
            }
            if (def_init <= att_init) {
                attack_random( remaining_def, remaining_mon);
            }
        }
    }
}

void talk_function::force_on_force( std::vector<std::shared_ptr<npc>> defender, const std::string &def_desc,
    std::vector<std::shared_ptr<npc>> attacker, const std::string &att_desc, int advantage )
{
    std::string adv = "";
    if (advantage < 0){
        adv = ", attacker advantage";
    } else if (advantage > 0){
        adv = ", defender advantage";
    }
    popup(_("Engagement between %d members of %s %s and %d members of %s %s%s!"),
        defender.size(), defender[0]->my_fac->name.c_str(), def_desc.c_str(),
        attacker.size(), attacker[0]->my_fac->name.c_str(), att_desc.c_str(),
        adv.c_str());
    int defense = 0;
    int attack = 0;
    int att_init = 0;
    int def_init = 0;
    while (true){
        std::vector<std::shared_ptr<npc>> remaining_att;
        for( const auto &elem : attacker ) {
            if (elem->hp_cur[hp_torso] != 0){
                remaining_att.push_back(elem);
            }
        }
        std::vector<std::shared_ptr<npc>> remaining_def;
        for( const auto &elem : defender ) {
            if (elem->hp_cur[hp_torso] != 0){
                remaining_def.push_back(elem);
            }
        }

        defense = combat_score(remaining_def);
        attack = combat_score(remaining_att);
        if (attack > defense*3){
            attack_random( remaining_att, remaining_def);
            if (defense == 0 || (remaining_def.size() == 1 && remaining_def[0]->hp_cur[hp_torso] == 0 )){
                popup(_("%s forces are destroyed!"), defender[0]->my_fac->name.c_str());
            } else {
                popup(_("%s forces retreat from combat!"), defender[0]->my_fac->name.c_str());
            }
            return;
        } else if (attack*3 < defense) {
            attack_random( remaining_def, remaining_att);
            if (attack == 0 || (remaining_att.size() == 1 && remaining_att[0]->hp_cur[hp_torso] == 0 )){
                popup(_("%s forces are destroyed!"), attacker[0]->my_fac->name.c_str());
            } else {
                popup(_("%s forces retreat from combat!"), attacker[0]->my_fac->name.c_str());
            }
            return;
        } else {
            def_init = rng( 1, 6 ) + advantage;
            att_init = rng( 1, 6 );
            if (def_init >= att_init) {
                attack_random( remaining_att, remaining_def);
            }
            if (def_init <= att_init) {
                attack_random( remaining_def, remaining_att);
            }
        }
    }
}

void talk_function::companion_return( npc &comp ){
    assert( !comp.is_active() );
    comp.reset_companion_mission();
    comp.companion_mission_time = calendar::before_time_starts;
    comp.companion_mission_time_ret = calendar::before_time_starts;
    for( size_t i = 0; i < comp.companion_mission_inv.size(); i++ ) {
        for( const auto &it : comp.companion_mission_inv.const_stack( i ) ) {
            g->m.add_item_or_charges( g->u.posx(), g->u.posy(), it );
        }
    }
    comp.companion_mission_inv.clear();
    comp.companion_mission_points.clear();
    // npc *may* be active, or not if outside the reality bubble
    g->reload_npcs();
}

std::vector<std::shared_ptr<npc>> talk_function::companion_list( const npc &p, const std::string &id, bool contains )
{
    std::vector<std::shared_ptr<npc>> available;
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    for( const auto &elem : overmap_buffer.get_companion_mission_npcs() ) {
        npc_companion_mission c_mission = elem->get_companion_mission();
        if( c_mission.position == tripoint( omt_pos.x, omt_pos.y, p.posz() ) &&
           c_mission.mission_id == id && c_mission.role_id == p.companion_mission_role_id ){

            available.push_back( elem );
        } else if( contains && c_mission.mission_id.find( id ) != std::string::npos ){
            available.push_back( elem );
        }
    }
    return available;
}

bool companion_sort_compare( npc* first, npc* second ){
    std::vector<npc *> comp_list;
    comp_list.push_back(first);
    comp_list.push_back(second);
    std::vector<comp_rank> scores = talk_function::companion_rank( comp_list );
    return scores[0].combat > scores[1].combat;
}

std::vector<npc *> talk_function::companion_sort( std::vector<npc *> available, std::string skill_tested )
{
    if( skill_tested.empty() ){
        std::sort (available.begin(), available.end(), companion_sort_compare);
        return available;
    }

    struct companion_sort_skill {
        companion_sort_skill(std::string skill_tested) { this->skill_tested = skill_tested; }

        bool operator()( npc* first, npc* second ){
            if( first->get_skill_level( skill_id(skill_tested) ) > second->get_skill_level( skill_id(skill_tested) ) ){
                    return true;
            }
            return false;
        }

        std::string skill_tested;
    };
    std::sort (available.begin(), available.end(), companion_sort_skill(skill_tested) );

    return available;
}

std::vector<comp_rank> talk_function::companion_rank( std::vector<npc *> available, bool adj )
{
    std::vector<comp_rank> raw;
    int max_combat = 0;
    int max_survival = 0;
    int max_industry = 0;
    for( auto e : available ){
        float combat =   ( e->get_dex() / 6.0  ) + ( e->get_str() / 4.0  ) + ( e->get_per() / 6.0  ) + ( e->get_int() / 12.0 );
        float survival = ( e->get_dex() / 6.0  ) + ( e->get_str() / 12.0 ) + ( e->get_per() / 6.0  ) + ( e->get_int() / 8.0  );
        float industry = ( e->get_dex() / 12.0 ) + ( e->get_str() / 12.0 ) + ( e->get_per() / 12.0 ) + ( e->get_int() / 4.0  );

        combat += e->get_skill_level( skill_id( "gun" ) ) +
                e->get_skill_level( skill_id( "unarmed" ) ) +
                e->get_skill_level( skill_id( "cutting" ) ) +
                e->get_skill_level( skill_id( "stabbing" ) ) +
                e->get_skill_level( skill_id( "bashing" ) ) +
                e->get_skill_level( skill_id( "cutting" ) ) +
                e->get_skill_level( skill_id( "melee" ) ) +
                e->get_skill_level( skill_id( "archery" ) ) +
                e->get_skill_level( skill_id( "survival" ) );
        combat *= ( e->get_dex() / 8.0 ) * ( e->get_str() / 8.0 );
        survival += e->get_skill_level( skill_id( "survival" ) ) +
                e->get_skill_level( skill_id( "unarmed" ) ) +
                e->get_skill_level( skill_id( "firstaid" ) ) +
                e->get_skill_level( skill_id( "speech" ) ) +
                e->get_skill_level( skill_id( "traps" ) ) +
                e->get_skill_level( skill_id( "archery" ) );
        survival *= ( e->get_dex() / 8.0 ) * ( e->get_per() / 8.0 );
        industry += e->get_skill_level( skill_id( "fabrication" ) ) +
                e->get_skill_level( skill_id( "mechanics" ) ) +
                e->get_skill_level( skill_id( "electronics" ) ) +
                e->get_skill_level( skill_id( "tailor" ) ) +
                e->get_skill_level( skill_id( "cooking" ) );
        industry *= e->get_int() / 8.0 ;
        comp_rank r;
        r.combat = combat;
        r.survival = survival;
        r.industry = industry;
        raw.push_back( r );
        if( combat > max_combat ){
            max_combat = combat;
        }
        if( survival > max_survival ){
            max_survival = survival;
        }
        if( industry > max_industry ){
            max_industry = industry;
        }
    }

    if( !adj ){
        return raw;
    }

    std::vector<comp_rank> adjusted;
    for( auto entry : raw ){
            comp_rank r;
            r.combat = 100.0 * entry.combat / max_combat;
            r.survival = 100.0 * entry.survival / max_survival;
            r.industry = 100.0 * entry.industry / max_industry;
            adjusted.push_back( r );
    }
    return adjusted;
}

npc *talk_function::companion_choose( std::string skill_tested, int skill_level ){
    std::vector<npc *> available = g->get_npcs_if( [&]( const npc &guy ) {
        return g->u.sees( guy.pos() ) && guy.is_friend() &&
            rl_dist( g->u.pos(), guy.pos() ) <= 24;
    } );

    if (available.empty()) {
        popup(_("You don't have any companions to send out..."));
        return nullptr;
    }
    std::vector<std::string> npcs;
    available = companion_sort( available, skill_tested );
    std::vector<comp_rank> rankings = companion_rank( available );

    int x = 0;
    for( auto &elem : available ) {
        std::string npc_entry = elem->name + "  ";
        if( !skill_tested.empty() ){
            std::string req_skill = (skill_level == 0) ? ("") : ("/"+to_string(skill_level));
            std::string skill_test = "["+ skill_id(skill_tested).str() + " " + to_string( elem->get_skill_level( skill_id(skill_tested) ) ) + req_skill + "] ";
            while( skill_test.length() + npc_entry.length() < 51 ){
                npc_entry += " ";
            }
            npc_entry += skill_test;
        }
        while( npc_entry.length() < 51 ){
            npc_entry += " ";
        }
        npc_entry = string_format("%s [ %4d : %4d : %4d ]", npc_entry, rankings[x].combat, rankings[x].survival, rankings[x].industry );
        x++;
        npcs.push_back( npc_entry );
    }
    npcs.push_back(_("Cancel"));
    int npc_choice = menu_vec(true, _("Who do you want to send?                    [ COMBAT : SURVIVAL : INDUSTRY ]"), npcs) - 1;
    if (npc_choice < 0 || npc_choice >= (int)available.size() ) {
        popup("You choose to send no one...");
        return nullptr;
    }

    if ( !skill_tested.empty() && available[npc_choice]->get_skill_level( skill_id(skill_tested) ) < skill_level ) {
        popup("The companion you selected doesn't have the skills!");
        return nullptr;
    }
    return available[npc_choice];
}

npc *talk_function::companion_choose_return( npc &p, const std::string &id, const time_point &deadline )
{
    std::vector<npc *> available;
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    for( const auto &guy : overmap_buffer.get_companion_mission_npcs() ) {
        npc_companion_mission c_mission = guy->get_companion_mission();
        if( c_mission.position != tripoint( omt_pos.x, omt_pos.y, p.posz() ) ||
           c_mission.mission_id != id || c_mission.role_id != p.companion_mission_role_id ){

            continue;
        }

        if( g->u.has_trait( trait_id( "DEBUG_HS" ) ) ){
            available.push_back( guy.get() );
        } else if( deadline == calendar::before_time_starts ){
            if( guy->companion_mission_time_ret <= calendar::turn ){
                available.push_back( guy.get() );
            }
        } else if( guy->companion_mission_time <= deadline ){
            available.push_back( guy.get() );
        }
    }

    if( available.empty() ) {
        popup(_("You don't have any companions ready to return..."));
        return nullptr;
    }

    if( available.size() == (size_t)1 ) {
        return available[0];
    }

    std::vector<std::string> npcs;
    for( auto &elem : available ) {
        npcs.push_back( ( elem )->name );
    }
    npcs.push_back(_("Cancel"));
    int npc_choice = menu_vec(true, _("Who should return?"), npcs) - 1;
    if (npc_choice >= 0 && size_t(npc_choice) < available.size()) {
        return available[npc_choice];
    }
    popup(_("No one returns to your party..."));
    return nullptr;
}

//Smash stuff, steal valuables, and change map maker
std::vector<item*> talk_function::loot_building(const tripoint site)
{
    tinymap bay;
    std::vector<item *> items_found;
    tripoint p;
    bay.load(site.x * 2, site.y * 2, site.z, false);
    for (int x = 0; x < 23; x++){
        for (int y = 0; y < 23; y++){
            p.x = x;
            p.y = y;
            p.z = site.z;
            ter_id t = bay.ter( x, y );
            //Open all the doors, doesn't need to be exhaustive
            if (t == t_door_c || t == t_door_c_peep || t == t_door_b
                || t == t_door_boarded || t == t_door_boarded_damaged
                || t == t_rdoor_boarded || t == t_rdoor_boarded_damaged
                || t == t_door_boarded_peep || t == t_door_boarded_damaged_peep){
                    bay.ter_set( x, y, t_door_o );
            } else if (t == t_door_locked || t == t_door_locked_peep
                || t == t_door_locked_alarm){
                    const map_bash_info &bash = bay.ter(x,y).obj().bash;
                    bay.ter_set( x, y, bash.ter_set);
                    bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
            } else if (t == t_door_metal_c || t == t_door_metal_locked
                || t == t_door_metal_pickable){
                    bay.ter_set( x, y, t_door_metal_o );
            } else if (t == t_door_glass_c){
                    bay.ter_set( x, y, t_door_glass_o );
            } else if (t == t_wall && one_in(25)){
                    const map_bash_info &bash = bay.ter(x,y).obj().bash;
                    bay.ter_set( x, y, bash.ter_set);
                    bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
                    bay.collapse_at( p, false );
            }
            //Smash easily breakable stuff
            else if ((t == t_window || t == t_window_taped || t == t_window_domestic ||
                    t == t_window_boarded_noglass || t == t_window_domestic_taped ||
                    t == t_window_alarm_taped || t == t_window_boarded ||
                    t == t_curtains || t == t_window_alarm ||
                    t == t_window_no_curtains || t == t_window_no_curtains_taped )
                    && one_in(4) ){
                const map_bash_info &bash = bay.ter(x,y).obj().bash;
                bay.ter_set( x, y, bash.ter_set);
                bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
            } else if ((t == t_wall_glass || t == t_wall_glass_alarm) && one_in(3) ){
                const map_bash_info &bash = bay.ter(x,y).obj().bash;
                bay.ter_set( x, y, bash.ter_set);
                bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
            } else if ( bay.has_furn(x,y) && bay.furn(x,y).obj().bash.str_max != -1 && one_in(10)) {
                const map_bash_info &bash = bay.furn(x,y).obj().bash;
                bay.furn_set(x,y, bash.furn_set);
                bay.delete_signage( p );
                bay.spawn_items( p, item_group::items_from( bash.drop_group, calendar::turn ) );
            }
            //Kill zombies!  Only works against pre-spawned enemies at the moment...
            Creature *critter = g->critter_at( p);
            if ( critter != nullptr ) {
                critter->die(nullptr);
            }
            //Hoover up tasty items!
            for (unsigned int i = 0; i < bay.i_at(p).size(); i++){
                if (((bay.i_at(p)[i].is_food() || bay.i_at(p)[i].is_food_container()) && !one_in(8)) ||
                    (bay.i_at(p)[i].made_of ( LIQUID ) && !one_in(8)) ||
                    (bay.i_at(p)[i].price( true ) > 1000 && !one_in(4)) ||
                    one_in(5)){
                    item *it = &bay.i_at(p)[i];
                    items_found.push_back(it);
                    bay.i_rem(p,i);
                }
            }
        }
    }
    bay.save();
    overmap_buffer.ter(site.x, site.y, site.z) = oter_id( "looted_building" );
    return items_found;
}

void talk_function::mission_key_push( std::vector<std::vector<mission_entry>> &mission_key_vectors, std::string id,
                                     std::string name_display, std::string dir, bool priority, bool possible )
{
    if( name_display == "" ){
        name_display = id;
    }
    mission_entry miss;
    miss.id = id;
    miss.name_display = name_display;
    miss.dir = dir;
    miss.priority = priority;
    miss.possible = possible;

    if( priority ){
        mission_key_vectors[0].push_back(miss);
    }
    if( !possible ){
        mission_key_vectors[10].push_back(miss);
    }
    if( dir == "" || dir == "[B]" ){
        mission_key_vectors[1].push_back(miss);
    }
    if( dir == "[N]" ){
        mission_key_vectors[2].push_back(miss);
    } else if( dir == "[NE]" ){
        mission_key_vectors[3].push_back(miss);
    } else if( dir == "[E]" ){
        mission_key_vectors[4].push_back(miss);
    } else if( dir == "[SE]" ){
        mission_key_vectors[5].push_back(miss);
    } else if( dir == "[S]" ){
        mission_key_vectors[6].push_back(miss);
    } else if( dir == "[SW]" ){
        mission_key_vectors[7].push_back(miss);
    } else if( dir == "[W]" ){
        mission_key_vectors[8].push_back(miss);
    } else if( dir == "[NW]" ){
        mission_key_vectors[9].push_back(miss);
    }
}

void talk_function::draw_camp_tabs( const catacurses::window &win, const camp_tab_mode cur_tab,
                                   std::vector<std::vector<mission_entry>> &mission_key_vectors )
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
    for( auto t : tabs ){
        bool tab_empty = mission_key_vectors[tab_x +1].empty();
        draw_subtab( win, tab_space, t, tab_x == cur_tab, false, tab_empty );
        tab_space += tab_step + utf8_width( t );
        tab_x++;
    }
    wrefresh( win );
}

std::string talk_function::name_mission_tabs( npc &p, std::string id, std::string cur_title, camp_tab_mode cur_tab ){
    if( id != "FACTION_CAMP" ){
        return cur_title;
    }
    const point omt_pos = ms_to_omt_copy( g->m.getabs( p.posx(), p.posy() ) );
    point change;
    switch (cur_tab){
        case TAB_MAIN:
            return _("Base Missions");
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
    if( om_cur.find("faction_base_farm") != std::string::npos ){
        return _("Farm Expansion");
    }
    if( om_cur.find("faction_base_garage_") != std::string::npos ){
        return _("Garage Expansion");
    }
    if( om_cur.find("faction_base_kitchen_") != std::string::npos ){
        return _("Kitchen Expansion");
    }
    if( om_cur.find("faction_base_blacksmith_") != std::string::npos ){
        return _("Blacksmith Expansion");
    }
    if( om_cur == "field" || om_cur == "forest" || om_cur == "swamp" ){
        return _("Empty Expansion");
    }
    return _("Base Missions");
}

std::map<std::string,std::string> talk_function::camp_recipe_deck( std::string om_cur ){
    if( om_cur == "ALL" || om_cur == "COOK" || om_cur == "BASE" || om_cur == "FARM" || om_cur == "SMITH" ){
        return recipe_group::get_recipes(om_cur);
    }
    std::map<std::string,std::string> cooking_recipes;
    for( auto building_levels : om_all_upgrade_levels( om_cur ) ){
        std::map<std::string,std::string> test_s = recipe_group::get_recipes(building_levels);
        cooking_recipes.insert( test_s.begin(), test_s.end() );
    }
    return cooking_recipes;
}

int talk_function::camp_recipe_batch_max( const recipe making, inventory total_inv ){
    int max_batch = 0;
    int max_checks = 9;
    int iter = 0;
    size_t batch_size = 1000;
    while( batch_size > 0 ) {
        while( iter < max_checks ) {
            if( making.requirements().can_make_with_inventory( total_inv, max_batch + batch_size ) &&
               (size_t)camp_food_supply() > (max_batch + batch_size) * time_to_food ( time_duration::from_turns( making.time / 100 ) ) ){
                max_batch += batch_size;
            }
            iter++;
        }
        iter = 0;
        batch_size /= 10;
    }
    return max_batch;
}

int talk_function::companion_skill_trainer( npc &comp, std::string skill_tested, time_duration time_worked, int difficulty ){
    difficulty = std::max( 1, difficulty );
    float checks = to_minutes<int>( time_worked )/10.0;
    int total = difficulty * checks;
    int i = 0;

    if( skill_tested == "" || skill_tested == "gathering" ){
        weighted_int_list<skill_id> gathering_practice;
        gathering_practice.add( skill_survival, 80 );
        gathering_practice.add( skill_traps, 5 );
        gathering_practice.add( skill_fabrication, 5 );
        gathering_practice.add( skill_archery, 5 );
        gathering_practice.add( skill_melee, 5 );
        while( i < checks ){
            comp.practice( *gathering_practice.pick(), difficulty );
            i++;
        }
        return total;
    }

    if( skill_tested == "menial" ){
        weighted_int_list<skill_id> menial_practice;
        menial_practice.add( skill_fabrication, 60 );
        menial_practice.add( skill_survival, 10 );
        menial_practice.add( skill_speech, 15 );
        menial_practice.add( skill_tailor, 5 );
        menial_practice.add( skill_cooking, 10 );
        while( i < checks ){
            comp.practice( *menial_practice.pick(), difficulty );
            i++;
        }
        return total;
    }

    if( skill_tested == "construction" ){
        weighted_int_list<skill_id> construction_practice;
        construction_practice.add( skill_fabrication, 80 );
        construction_practice.add( skill_survival, 10 );
        construction_practice.add( skill_mechanics, 10 );
        while( i < checks ){
            comp.practice( *construction_practice.pick(), difficulty );
            i++;
        }
        return total;
    }

    if( skill_tested == "recruiting" ){
        weighted_int_list<skill_id> construction_practice;
        construction_practice.add( skill_speech, 70 );
        construction_practice.add( skill_survival, 25 );
        construction_practice.add( skill_melee, 5 );
        while( i < checks ){
            comp.practice( *construction_practice.pick(), difficulty );
            i++;
        }
        return total;
    }
    comp.practice( skill_id(skill_tested), total );
    return total;
}

int talk_function::companion_skill_trainer( npc &comp, skill_id skill_tested, time_duration time_worked, int difficulty ){
    difficulty = ((difficulty <= 0) ? difficulty : 1);
    float checks = to_minutes<int>( time_worked )/10.0;
    int total = difficulty * checks;
    comp.practice( skill_tested, total );
    return total;
}

int talk_function::om_harvest_furn( npc &comp, point omt_tgt, furn_id f, float chance, bool force_bash )
{
    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    const furn_t &furn_tgt = f.obj();
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    int harvested = 0;
    int total = 0;
    for( int x = 0; x < 23; x++ ){
        for( int y = 0; y < 23; y++ ){
            if( target_bay.furn(x,y) == f && x_in_y( chance, 1.0 ) ){
                if( force_bash || comp.str_cur > furn_tgt.bash.str_min + rng( -2, 2) ){
                    for( auto itm : item_group::items_from( furn_tgt.bash.drop_group, calendar::turn ) ) {
                        comp.companion_mission_inv.push_back(itm);
                    }
                    harvested++;
                    target_bay.furn_set( x, y, furn_tgt.bash.furn_set );
                }
                total++;
            }
        }
    }
    target_bay.save();
    if( !force_bash ){
        return total;
    }
    return harvested;
}

int talk_function::om_harvest_ter( npc &comp, point omt_tgt, ter_id t, float chance, bool force_bash )
{
    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    const ter_t &ter_tgt = t.obj();
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    int harvested = 0;
    int total = 0;
    for( int x = 0; x < 23; x++ ){
        for( int y = 0; y < 23; y++ ){
            if( target_bay.ter(x,y) == t && x_in_y( chance, 1.0 ) ){
                if( force_bash ){
                    for( auto itm : item_group::items_from( ter_tgt.bash.drop_group, calendar::turn ) ) {
                        comp.companion_mission_inv.push_back(itm);
                    }
                    harvested++;
                    target_bay.ter_set( x, y, ter_tgt.bash.ter_set );
                }
                total++;
            }
        }
    }
    target_bay.save();
    if( !force_bash ){
        return total;
    }
    return harvested;
}

int talk_function::om_harvest_trees( npc &comp, tripoint omt_tgt, float chance, bool force_cut, bool force_cut_trunk )
{
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    int harvested = 0;
    int total = 0;
    for( int x = 0; x < 23; x++ ){
        for( int y = 0; y < 23; y++ ){
            if( target_bay.ter(x,y).obj().has_flag( "TREE" ) && rng( 0, 100) < chance * 100 ){
                if( force_cut ){
                    tripoint direction;
                    direction.x = ( rng(0,1) == 1 ) ? -1 : 1;
                    direction.y = rng( -1, 1 );
                    tripoint to = tripoint(x, y, 0) + tripoint( 3 * direction.x + rng( -1, 1 ), 3 * direction.y + rng( -1, 1 ), 0 );
                    std::vector<tripoint> tree = line_to( tripoint(x, y, 0), to, rng( 1, 8 ) );
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
    for( int x = 0; x < 23; x++ ){
        for( int y = 0; y < 23; y++ ){
            if( target_bay.ter(x,y) == ter_id( "t_trunk" ) && x_in_y( chance, 1.0 ) ){
                if( force_cut_trunk ){
                    target_bay.ter_set( x, y, t_dirt );
                    target_bay.spawn_item( x, y, "log", rng( 2, 3 ), 0, calendar::turn );
                    harvested++;
                }
                total++;
            }
        }
    }

    target_bay.save();
    if( !force_cut && !force_cut_trunk ){
        return total;
    }
    return harvested;
}

int talk_function::om_harvest_itm( npc &comp, point omt_tgt, float chance, bool take )
{
    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, g->u.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    int harvested = 0;
    int total = 0;
    for( int x = 0; x < 23; x++ ){
        for( int y = 0; y < 23; y++ ){
            for( item i : target_bay.i_at( x, y) ){
                total++;
                if( x_in_y( chance, 1.0 ) ){
                    comp.companion_mission_inv.push_back(i);
                    harvested++;
                }
            }
            target_bay.i_clear( x, y);
        }
    }
    target_bay.save();
    if( !take ){
        return total;
    }
    return harvested;
}

tripoint talk_function::om_target_tile( tripoint omt_pos, int min_range, int range, std::vector<std::string> possible_om_types, bool must_see,
                                       bool popup_notice, tripoint source, bool bounce ){
    bool errors = false;
    if( popup_notice ){
            popup( _("Select a location between  %d and  %d tiles away."), min_range, range );
    }

    std::vector<std::string> bounce_locations = { "faction_hide_site_0" };

    tripoint where;
    om_range_mark( omt_pos, range );
    om_range_mark( omt_pos, min_range, true, "Y;X: MIN RANGE" );
    if( source == tripoint(-999,-999,-999) ){
        where = ui::omap::choose_point();
    } else {
        where = ui::omap::choose_point( source );
    }
    om_range_mark( omt_pos, range, false );
    om_range_mark( omt_pos, min_range, false, "Y;X: MIN RANGE" );

    if( where == overmap::invalid_tripoint ) {
        return tripoint(-999, -999, -999);
    }
    int dist = rl_dist(where.x, where.y, omt_pos.x,omt_pos.y);
    if ( dist > range || dist < min_range ) {
        popup( _("You must select a target between %d and %d range from the base.  Range: %d"), min_range, range, dist );
        errors = true;
    }

    tripoint omt_tgt = tripoint(where.x, where.y, g->u.posz() );

    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, g->u.posz() );

    if( must_see && overmap_buffer.seen( omt_tgt.x, omt_tgt.y, 0 ) == false ){
        errors = true;
        popup( _("You must be able to see the target that you select.") );
    }

    if( !errors ){
        for( auto pos_om : bounce_locations ){
            if( bounce && omt_ref.id().c_str() == pos_om && range > 5 ){
                if( query_yn( _("Do you want to bounce off this location to extend range?") ) ){
                    om_line_mark( omt_pos, omt_tgt);
                    tripoint dest = om_target_tile( omt_tgt, 2, range * .75, possible_om_types, true, false, omt_tgt, true );
                    om_line_mark( omt_pos, omt_tgt, false);
                    return dest;
                }
            }
        }

        if( possible_om_types.empty() ) {
            return omt_tgt;
        }

        for( auto pos_om : possible_om_types ){
            if( omt_ref.id().c_str() == pos_om ){
                return omt_tgt;
            }
        }
    }

    return om_target_tile( omt_pos, min_range, range, possible_om_types );
}

void talk_function::om_range_mark( tripoint origin, int range, bool add_notes,  std::string message ){
    std::vector<tripoint> note_pts;
    //North Limit
    for( int x = origin.x - range - 1; x < origin.x + range + 2; x++ ){
        note_pts.push_back( tripoint( x, origin.y - range - 1, origin.z) );
    }
    //South
    for( int x = origin.x - range - 1; x < origin.x + range + 2; x++ ){
        note_pts.push_back( tripoint( x, origin.y + range + 1, origin.z) );
    }
    //West
    for( int y = origin.y - range - 1; y < origin.y + range + 2; y++ ){
        note_pts.push_back( tripoint( origin.x - range - 1, y, origin.z) );
    }
    //East
    for( int y = origin.y - range - 1; y < origin.y + range + 2; y++ ){
        note_pts.push_back( tripoint( origin.x + range + 1, y, origin.z) );
    }

    for( auto pt : note_pts ){
        if( add_notes ){
            if( !overmap_buffer.has_note(pt) ){
                overmap_buffer.add_note( pt.x, pt.y, pt.z, message);
            }
        } else {
            if( overmap_buffer.has_note(pt) && overmap_buffer.note( pt.x, pt.y, pt.z ) == message ){
                overmap_buffer.delete_note( pt.x, pt.y, pt.z );
            }
        }
    }

}

void talk_function::om_line_mark( tripoint origin, tripoint dest, bool add_notes, std::string message ){
    std::vector<tripoint> note_pts = line_to( origin, dest );

    for( auto pt : note_pts ){
        if( add_notes ){
            if( !overmap_buffer.has_note(pt) ){
                overmap_buffer.add_note( pt.x, pt.y, pt.z, message);
            }
        } else {
            if( overmap_buffer.has_note(pt) && overmap_buffer.note( pt.x, pt.y, pt.z ) == message ){
                overmap_buffer.delete_note( pt.x, pt.y, pt.z );
            }
        }
    }
}

time_duration talk_function::companion_travel_time_calc( tripoint omt_pos, tripoint omt_tgt, time_duration work, int trips ){
    std::vector<tripoint> journey = line_to( omt_pos, omt_tgt );
    return companion_travel_time_calc( journey, work, trips );
}

time_duration talk_function::companion_travel_time_calc( std::vector<tripoint> journey, time_duration work, int trips ){
    //path = pf::find_path( point( start.x, start.y ), point( finish.x, finish.y ), 2*OX, 2*OY, estimate );
    int one_way = 0;
    for( auto &om : journey ) {
        oter_id &omt_ref = overmap_buffer.ter( om.x, om.y, g->u.posz() );
        std::string om_id = omt_ref.id().c_str();
        //Player walks 1 om is roughly 2.5 min
        if( om_id == "field" ){
            one_way += 3;
        }else if( om_id == "forest" ){
            one_way += 4;
        }else if( om_id == "forest_thick" ){
            one_way += 5;
        }else if( om_id == "forest_water" ){
            one_way += 6;
        }else if( is_river(omt_ref) ){
            one_way += 20;
        } else {
            one_way += 4;
        }
    }
    return time_duration::from_minutes( ( one_way * trips ) + to_minutes<int>(work) );
}

std::string talk_function::camp_trip_description( time_duration total_time, time_duration working_time, time_duration travel_time,
                                  int distance, int trips, int need_food ){
    std::string entry = " \n";
    //A square is roughly 3 m
    int dist_m = distance * 24 * 3;
    if( dist_m > 1000 ){
        entry += string_format(">Distance:%15.2f (km)\n", ( dist_m / 1000.0 ) );
        entry += string_format(">One Way: %15d (trips)\n", trips );
        entry += string_format(">Covered: %15.2f (km)\n", ( dist_m /1000.0 * trips ) );
    } else {
        entry += string_format(">Distance:%15d (m)\n", dist_m );
        entry += string_format(">One Way: %15d (trips)\n", trips );
        entry += string_format(">Covered: %15d (m)\n", dist_m * trips );
    }
    entry += string_format(">Travel:  %15d (hours)\n", to_hours<int>( travel_time ) );
    entry += string_format(">Working: %15d (hours)\n", to_hours<int>( working_time ) );
    entry += "----                   ----\n";
    if( total_time > 3_days ){
        entry += string_format("Total:    %15d (days)\n", to_days<int>( total_time ) );
    } else if( total_time > 3_hours ){
        entry += string_format("Total:    %15d (hours)\n", to_hours<int>( total_time ) );
    } else{
        entry += string_format("Total:    %15d (minutes)\n", to_minutes<int>( total_time ) );
    }
    entry += string_format("Food:     %15d (kcal)\n \n", 10 * need_food);
    return entry;
}

int talk_function::om_carry_weight_to_trips( npc &comp, std::vector<item *> itms )
{
    int trips = 0;
    int total_m = 0;
    int total_v = 0;
    for( auto i : itms ){
        total_m += to_gram( i->weight( true ) );
        total_v += to_milliliter( i->volume( true ) );
    }
    //Assume a significant portion of weight is dedicated to personal gear
    //This isn't a dedicated logistics transport
    float max_m = to_gram( comp.weight_capacity() ) * ( 2 / 3.0 );
    trips = ceil( total_m / max_m );
    trips = ( trips == 0 ) ?  1 : trips;
    //Assume an additional pack will be carried in addition to normal gear
    item sack = item(itype_id( "makeshift_sling" ) );
    float max_v = to_milliliter( comp.volume_capacity() ) * ( 2 / 3.0 ) + to_milliliter( sack.get_storage() );
    int trips_v = ceil( total_v / max_v );
    trips_v = ( trips_v == 0 ) ?  1 : trips_v;
    return ( trips > trips_v ) ?  trips : trips_v;
}

bool talk_function::om_set_hide_site( npc &comp, tripoint omt_tgt, std::vector<item *> itms, std::vector<item *> itms_rem )
{
    oter_id &omt_ref = overmap_buffer.ter( omt_tgt.x, omt_tgt.y, comp.posz() );
    omt_ref = oter_id( omt_ref.id().c_str() );
    tinymap target_bay;
    target_bay.load( omt_tgt.x * 2, omt_tgt.y * 2, comp.posz(), false );
    target_bay.ter_set( 11, 10, t_improvised_shelter );
    for( auto i : itms_rem ){
        comp.companion_mission_inv.add_item(*i);
        target_bay.i_rem( 11, 10, i );
    }
    for( auto i : itms ){
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
    tripoint p_food = tripoint(p.posx()-3, p.posy()-1, p.posz());
    sort_pts.push_back(p_food);
    sort_names.push_back("food for you");
    tripoint p_food_stock = tripoint(p.posx()+1, p.posy(), p.posz());
    sort_pts.push_back(p_food_stock);
    sort_names.push_back("food for companions");
    tripoint p_seed = tripoint(p.posx()-1, p.posy()-1, p.posz());
    sort_pts.push_back(p_seed);
    sort_names.push_back("seeds");
    tripoint p_weapon = tripoint(p.posx()-1, p.posy()+1, p.posz());
    sort_pts.push_back(p_weapon);
    sort_names.push_back("weapons");
    tripoint p_clothing = tripoint(p.posx()-3, p.posy()-2, p.posz());
    sort_pts.push_back(p_clothing);
    sort_names.push_back("clothing");
    tripoint p_bionic = tripoint(p.posx()-3, p.posy()+1, p.posz());
    sort_pts.push_back(p_bionic);
    sort_names.push_back("bionics");
    tripoint p_tool = tripoint(p.posx()-3, p.posy()+2, p.posz());
    sort_pts.push_back(p_tool);
    sort_names.push_back("all kinds of tools");
    tripoint p_wood = tripoint(p.posx()-5, p.posy()+2, p.posz());
    sort_pts.push_back(p_wood);
    sort_names.push_back("wood of various sorts");
    tripoint p_trash = tripoint(p.posx()-6, p.posy()-3, p.posz());
    sort_pts.push_back(p_trash);
    sort_names.push_back("trash and rotting food");
    tripoint p_book = tripoint(p.posx()-3, p.posy()+1, p.posz());
    sort_pts.push_back(p_book);
    sort_names.push_back("books");
    tripoint p_medication = tripoint(p.posx()-3, p.posy()+1, p.posz());
    sort_pts.push_back(p_medication);
    sort_names.push_back("medication");
    tripoint p_ammo = tripoint(p.posx()-3, p.posy()+1, p.posz());
    sort_pts.push_back(p_ammo);
    sort_names.push_back("ammo");

    if( reset_pts ){
        p.companion_mission_points.clear();
        p.companion_mission_points.insert( p.companion_mission_points.end(), sort_pts.begin(), sort_pts.end() );
        if( !choose_pts ){
            return true;
        }
    }
    if( choose_pts ){
        for( size_t x = 0; x < sort_pts.size(); x++ ){
            if( query_yn( string_format( "Reset point: %s?", sort_names[x] ) ) ){
                const tripoint where( g->look_around() );
                if( rl_dist( g->u.pos(), where ) <= 20 ) {
                    sort_pts[x] = where;
                }
            }
        }
    }

    std::string display_pts = _("                    Items       New Point       Old Point\n \n");
    for( size_t x = 0; x < sort_pts.size(); x++ ){
        std::string trip_string =  string_format( "( %d, %d, %d)", sort_pts[x].x, sort_pts[x].y, sort_pts[x].z );
        std::string old_string = "";
        if( p.companion_mission_points.size() == sort_pts.size() ){
             old_string =  string_format( "( %d, %d, %d)", p.companion_mission_points[x].x, p.companion_mission_points[x].y,
                                         p.companion_mission_points[x].z );
        }
        display_pts += string_format( "%25s %15s %15s    \n", sort_names[x], trip_string, old_string );
    }
    display_pts += _("\n \n             Save Points?");

    if( query_yn( display_pts ) ){
        p.companion_mission_points.clear();
        p.companion_mission_points.insert( p.companion_mission_points.end(), sort_pts.begin(), sort_pts.end() );
        return true;
    }
    if( query_yn( _("Revert to default points?") ) ){
        return camp_menial_sort_pts( p, true, false );
    }
    return false;
}

void talk_function::faction_camp_tutorial(){
    std::string slide_overview = _("Faction Camp Tutorial:\n \n"
                                   "The faction camp system is designed to give you greater control over your companions by allowing you to "
                                   "assign them to their own missions.  These missions can range from gathering and crafting to eventual combat "
                                   "patrols.\n \n"
                                   );
    popup( "%s", slide_overview );
    slide_overview = _("Faction Camp Tutorial:\n \n"
                       "<color_light_green>FOOD:</color>  Food is required for or produced during every mission.  Missions that are for a fixed amount of time will "
                       "require you to pay in advance while repeating missions, like gathering firewood, are paid upon completion.  "
                       "Not having the food needed to pay a companion will result in a loss of reputation across the faction.  Which "
                       "can lead to VERY bad things if it gets too low.\n \n"
                        );
    popup( "%s", slide_overview );
    slide_overview = _("Faction Camp Tutorial:\n \n"
                        "<color_light_green>SELECTING A SITE:</color>  For your first camp, pick a site that has fields in the 8 adjacent tiles and lots of forests around it. "
                        "Forests are your primary source of construction materials in the early game while fields can be used for farming.  You "
                        "don't have to be too picky, you can build as many camps as you want but each will require an NPC camp manager and an additional NPC to task out.\n \n"
                        );
    popup( "%s", slide_overview );
    slide_overview = _("Faction Camp Tutorial:\n \n"
                        "<color_light_green>UPGRADING:</color>  After you pick a site you will need to find or make materials to upgrade the camp further to access new "
                        "missions.  The first new missions are focused on gathering materials to upgrade the camp so you don't have to. "
                        "After two or three upgrades you will have access to the <color_yellow>[Menial Labor]</color> mission which will allow you to task companions "
                        "with sorting all of the items around your camp into categories.  Later upgrades allow you to send companions to recruit new members, build overmap "
                        "fortifications, or even conduct combat patrols.\n"
                        );
    popup( "%s", slide_overview );
    slide_overview = _("Faction Camp Tutorial:\n \n"
                        "<color_light_green>EXPANSIONS:</color>  When you upgrade your first tent all the way you will unlock the ability to construct expansions. Expansions "
                        "allow you to specialize each camp you build by focusing on the industries that you need.  A <color_light_green>[Farm]</color> is recommended for "
                        "players that want to pursue a large faction while a <color_light_green>[Kitchen]</color> is better for players that just want the quality of life "
                        "improvement of having an NPC do all of their cooking.  A <color_light_green>[Garage]</color> is useful for chop shop type missions that let you "
                        "trade vehicles for large amounts of parts and resources.  All those resources can be turning into valuable eqiupment in the "
                        "<color_light_green>[Blacksmith Shop]</color>. You can build an additional expansion every other level after the first is unlocked and when one "
                        "camp is full you can just as easily build another.\n \n"
                        );
    popup( "%s", slide_overview );
    if( query_yn( _("Repeat?") ) ){
        faction_camp_tutorial();
    }
}

void talk_function::camp_craft_construction( npc &p, mission_entry cur_key, std::map<std::string,std::string> recipes, std::string miss_id,
                                            tripoint omt_pos, std::vector<std::pair<std::string, tripoint>> om_expansions  )
{
    for( std::map<std::string,std::string>::const_iterator it = recipes.begin(); it != recipes.end(); ++it ) {
        if ( cur_key.id == cur_key.dir + it->first ){
            const recipe *making = &recipe_id( it->second ).obj();
            inventory total_inv = g->u.crafting_inventory();

            if( ! making->requirements().can_make_with_inventory( total_inv, 1 ) ){
                popup(_("You don't have the materials to craft that") );
                continue;
            }

            int batch_size = 1;
            string_input_popup popup_input;
            popup_input
                .title( string_format( "Batch crafting %s [MAX: %d]: ", making->result_name(), camp_recipe_batch_max( *making, total_inv ) ) )
                .edit( batch_size );

            if( popup_input.canceled() || batch_size <= 0 ){
                continue;
            }
            if( batch_size > camp_recipe_batch_max( *making, total_inv) ){
                popup( _("Your batch is too large!") );
                continue;
            }
            int need_food = batch_size * time_to_food ( time_duration::from_turns( making->time / 100 ) );
            for( const auto &e : om_expansions ){
                if( om_simple_dir( point( omt_pos.x, omt_pos.y), e.second ) == cur_key.dir ) {
                    std::vector<std::shared_ptr<npc>> npc_list = companion_list( p, miss_id + cur_key.dir );
                    if( !npc_list.empty() ) {
                        popup( _("You already have someone working in that expansion.") );
                        continue;
                    }
                    npc* comp = individual_mission(p, _("begins to work..."), miss_id + cur_key.dir, false, {},
                                   making->skill_used.obj().ident().c_str(), making->difficulty );
                    if ( comp != nullptr ){
                        g->u.consume_components_for_craft( making, batch_size, true );
                        g->u.invalidate_crafting_inventory();
                        for ( auto results : making->create_results( batch_size ) ) {
                            comp->companion_mission_inv.add_item( results );
                        }
                        comp->companion_mission_time_ret = calendar::turn + (time_duration::from_turns( making->time / 100 ) * batch_size );
                        camp_food_supply( -need_food);
                    }
                }
            }
        }
    }
}


std::string talk_function::camp_recruit_evaluation( npc &p, std::string base, std::vector<std::pair<std::string, tripoint>> om_expansions,
                                                   bool raw_score ){
    int sbase = om_over_level( "faction_base_camp_11", base ) * 5;
    int sexpansions = om_expansions.size() * 2;
    for( auto expan : om_expansions ){
        //The expansion is max upgraded
        if( om_next_upgrade(expan.first) == "null" ){
            sexpansions += 2;
        }
    }
    int sfaction = std::min( camp_food_supply() / 10000, 10 );
    sfaction += std::min( camp_discipline() / 10, 5 );
    sfaction += std::min( camp_morale() / 10, 5);

    //Secret or Hidden Bonus
    //Please avoid openly discussing so that there is some mystery to the system
    int sbonus = 0;
    //How could we ever starve?
    //More than 5 farms at recruiting base
    int farm = 0;
    for( auto expan : om_expansions ){
        if( om_min_level( "faction_base_farm_1", expan.first ) ){
            farm++;
        }
    }
    if( farm >= 5 ){
        sbonus += 10;
    }
    //More machine than man
    //Bionics count > 10, respect > 75
    if( g->u.my_bionics->size() > 10 && camp_discipline() > 75 ){
        sbonus += 10;
    }
    //Survival of the fittest
    if( g->get_npc_kill().size() > 10 ){
        sbonus += 10;
    }

    int total = sbase + sexpansions + sfaction + sbonus;
    std::string desc = string_format( "Notes:\n"
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
            "Positions: %d/1\n", sbase, sexpansions, sfaction, sbonus, total, companion_list( p, "_faction_camp_firewood" ).size()
            );
    if( raw_score ){
        return to_string( total );
    }
    return desc;
}

void talk_function::camp_recruit_return( npc &p, std::string task, int score )
{
    npc *comp = companion_choose_return( p, task, calendar::turn - 4_days );
    if (comp == nullptr){
        return;
    }
    std::string skill_group = "recruiting";
    companion_skill_trainer( *comp, skill_group, 4_days, 2 );
    popup(_("%s returns from searching for recruits with a bit more experience..."), comp->name.c_str());
    companion_return( *comp );

    std::shared_ptr<npc> recruit;
    //Success of finding an NPC to recruit, based on survival/tracking
    int skill = comp->get_skill_level( skill_survival );
    int dice = rng( 1, 20);
    dice += skill - 2;
    if ( dice > 15 ){
        recruit = std::make_shared<npc>();
        recruit->normalize();
        recruit->randomize();
        popup(_("%s encountered %s..."), comp->name.c_str(), recruit->name.c_str() );
    } else {
        popup(_("%s didn't find anyone to recruit..."), comp->name.c_str());
        return;
    }
    //Chance of convencing them to come back
    skill = comp->get_skill_level( skill_speech );
    skill = skill * ( 1 + (score / 100.0) );
    dice = rng( 1, 20);
    dice += skill - 4;
    if ( dice > 15 ){
        popup(_("%s convinced %s to hear a recruitment offer from you..."), comp->name.c_str(), recruit->name.c_str() );
    } else {
        popup(_("%s wasn't interested in anything %s had to offer..."), recruit->name.c_str(), comp->name.c_str());
        return;
    }
    //Stat window
    int rec_m = 0;
    int appeal = rng( -5, 3 );
    appeal += std::min( skill / 3 , 3);
    int food_desire = rng( 0, 5 );
    while( rec_m >= 0 ){
        std::string description = string_format( "NPC Overview:\n \n" );
        description += string_format( "Name:  %20s\n \n", recruit->name.c_str() );
        description += string_format( "Strength:        %10d\n", recruit->str_max );
        description += string_format( "Dexterity:       %10d\n", recruit->dex_max );
        description += string_format( "Intelligence:    %10d\n", recruit->int_max );
        description += string_format( "Perception:      %10d\n \n", recruit->per_max );
        description += string_format( "Top 3 Skills:\n" );

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

        description += string_format( "Asking for:\n" );
        description += string_format( "> Food:     %10d days\n \n", food_desire );
        description += string_format( "Faction Food:%9d days\n \n", camp_food_supply( 0, true ) );
        description += string_format( "Recruit Chance: %10d%%\n \n", std::min( (int)( (10.0 + appeal) / 20.0 * 100), 100 ) );
        description += _("Select an option:");

        std::vector<std::string> rec_options;
        rec_options.push_back( _("Increase Food") );
        rec_options.push_back( _("Decrease Food") );
        rec_options.push_back( _("Make Offer") );
        rec_options.push_back( _("Not Interested") );

        rec_m = menu_vec(true, description.c_str(), rec_options) - 1;
        int sz = rec_options.size();
        if (rec_m < 0 || rec_m >= sz) {
            popup("You decide you aren't interested...");
            return;
        }

        if ( rec_m == 0 && food_desire + 1 <= camp_food_supply( 0, true ) ) {
            food_desire++;
            appeal++;
        }
        if ( rec_m == 1 ) {
            if( food_desire > 0 ){
                food_desire--;
                appeal--;
            }
        }
        if ( rec_m == 2 ) {
            break;
        }
    }
    //Roll for recruitment
    dice = rng( 1, 20);
    if ( dice + appeal >= 10 ){
        popup( _("%s has been convinced to join!"), recruit->name.c_str() );
    } else {
        popup(_("%s wasn't interested..."), recruit->name.c_str() );
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

std::vector<tripoint> talk_function::om_companion_path( tripoint start, int range_start, bool bounce ){
    std::vector<tripoint> scout_points;
    tripoint spt;
    tripoint last = start;
    int range = range_start;
    int def_range = range_start;
    while( range > 3 ){
        spt = om_target_tile( last, 0, range, {}, false, true, last, false );
        if( spt == tripoint( -999, -999, -999 ) ){
            scout_points.clear();
            return scout_points;
        }
        if( last == spt ){
            break;
        }
        std::vector<tripoint> note_pts = line_to( last, spt );
        scout_points.insert( scout_points.end(), note_pts.begin(), note_pts.end() );
        om_line_mark( last, spt );
        range -= rl_dist( spt.x, spt.y, last.x, last.y );
        last = spt;

        oter_id &omt_ref = overmap_buffer.ter( last.x, last.y, g->u.posz() );

        if( bounce && omt_ref.id() == "faction_hide_site_0" ){
            range = def_range * .75;
            def_range = range;
        }
    }
    for( auto pt : scout_points ){
        om_line_mark( pt, pt, false );
    }
    return scout_points;
}
