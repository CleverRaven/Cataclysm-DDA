#include "npc.h"
#include "output.h"
#include "game.h"
#include "map.h"
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
#include "overmapbuffer.h"
#include "skill.h"
#include "translations.h"
#include "martialarts.h"
#include "input.h"
#include "item_group.h"
#include "compatibility.h"
#include "mapdata.h"

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
const skill_id skill_cooking( "cooking" );
const skill_id skill_traps( "traps" );
const skill_id skill_archery( "archery" );

static const trait_id trait_NPC_CONSTRUCTION_LEV_1( "NPC_CONSTRUCTION_LEV_1" );
static const trait_id trait_NPC_CONSTRUCTION_LEV_2( "NPC_CONSTRUCTION_LEV_2" );
static const trait_id trait_NPC_MISSION_LEV_1( "NPC_MISSION_LEV_1" );

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
    if (g->u.install_bionics(it, 20)){
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
    if (g->u.uninstall_bionic(bionic_id( bionic_types[bionic_index] ), 20)){
        g->u.cash -= price;
        p.cash += price;
        g->u.amount_of( bionic_types[bionic_index] ); // ??? this does nothing, it just queries the count
    }

}

void talk_function::companion_mission(npc &p)
{
 std::string id = "NONE";
 std::string title = _("Outpost Missions");
 unsigned int a = -1;
 // Name checks determining role? Horrible!
 if (p.name.find("Scavenger Boss") != a){
    id = "SCAVENGER";
    title = _("Junk Shop Missions");
 }
 if (p.name.find("Crop Overseer") != a){
    id = "COMMUNE CROPS";
    title = _("Agricultural Missions");
 }
 if (p.name.find("Foreman") != a){
    id = "FOREMAN";
    title = _("Construction Missions");
 }
 if (p.name.find(", Merchant") != a){
    id = "REFUGEE MERCHANT";
    title = _("Free Merchant Missions");
 }
 talk_function::outpost_missions( p, id, title );
}

bool talk_function::outpost_missions( npc &p, std::string id, std::string title )
{
    std::vector<std::string> keys;
    std::map<std::string, std::string> col_missions;
    std::vector<std::shared_ptr<npc>> npc_list;
    std::string entry, entry_aux;

    if (id == "SCAVENGER"){
        col_missions["Assign Scavenging Patrol"] = _("Profit: $25-$500\nDanger: Low\nTime: 10 hour missions\n \n"
            "Assigning one of your allies to patrol the surrounding wilderness and isolated buildings presents "
            "the opportunity to build survival skills while engaging in relatively safe combat against isolated "
            "creatures.");
        keys.push_back("Assign Scavenging Patrol");
        npc_list = companion_list( p, "_scavenging_patrol" );
        if (npc_list.size()>0){
            entry = _("Profit: $25-$500\nDanger: Low\nTime: 10 hour missions\n \nPatrol Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Retrieve Scavenging Patrol"] = entry;
            keys.push_back("Retrieve Scavenging Patrol");
        }
    }

    if ( id == "SCAVENGER" && p.has_trait( trait_NPC_MISSION_LEV_1 ) ){
        col_missions["Assign Scavenging Raid"] = _("Profit: $200-$1000\nDanger: Medium\nTime: 10 hour missions\n \n"
            "Scavenging raids target formerly populated areas to loot as many valuable items as possible before "
            "being surrounded by the undead.  Combat is to be expected and assistance from the rest of the party "
            "can't be guaranteed.  The rewards are greater and there is a chance of the companion bringing back items.");
        keys.push_back("Assign Scavenging Raid");
        npc_list = companion_list( p, "_scavenging_raid" );
        if (npc_list.size()>0){
            entry = _("Profit: $200-$1000\nDanger: Medium\nTime: 10 hour missions\n \nRaid Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Retrieve Scavenging Raid"] = entry;
            keys.push_back("Retrieve Scavenging Raid");
        }
    }

    if (id == "FOREMAN"){
        col_missions["Assign Ally to Menial Labor"] = _("Profit: $8/hour\nDanger: Minimal\nTime: 1 hour minimum\n \n"
            "Assigning one of your allies to menial labor is a safe way to teach them basic skills and build "
            "reputation with the outpost.  Don't expect much of a reward though.");
        keys.push_back("Assign Ally to Menial Labor");
        npc_list = companion_list( p, "_labor" );
        if (npc_list.size()>0){
            entry = _("Profit: $8/hour\nDanger: Minimal\nTime: 1 hour minimum\n \nLabor Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Ally from Menial Labor"] = entry;
            keys.push_back("Recover Ally from Menial Labor");
        }
    }

    if ( id == "FOREMAN" && p.has_trait( trait_NPC_MISSION_LEV_1 ) ){
        col_missions["Assign Ally to Carpentry Work"] = _("Profit: $12/hour\nDanger: Minimal\nTime: 1 hour minimum\n \n"
            "Carpentry work requires more skill than menial labor while offering modestly improved pay.  It is "
            "unlikely that your companions will face combat but there are hazards working on makeshift buildings.");
        keys.push_back("Assign Ally to Carpentry Work");
        npc_list = companion_list( p, "_carpenter" );
        if (npc_list.size()>0){
            entry = _("Profit: $12/hour\nDanger: Minimal\nTime: 1 hour minimum\n \nLabor Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Ally from Carpentry Work"] = entry;
            keys.push_back("Recover Ally from Carpentry Work");
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
        keys.push_back("Purchase East Field");
    }

    if ( id == "COMMUNE CROPS" && p.has_trait( trait_NPC_CONSTRUCTION_LEV_1 ) && !p.has_trait( trait_NPC_CONSTRUCTION_LEV_2 ) ){
        col_missions["Upgrade East Field I"] = _("Cost: $5500\n \n"
            "\n              .........\n              .........\n              .........\n              "
            ".........\n              .........\n              .........\n              ..#....**\n     "
            "         ..#Ov..**\n              ...O|....\n \n"
            "Protecting your field with a sturdy picket fence will keep most wildlife from nibbling your crops "
            "apart.  You can expect yields to increase.");
        keys.push_back("Upgrade East Field I");
    }

    if ( id == "COMMUNE CROPS" && p.has_trait( trait_NPC_CONSTRUCTION_LEV_1 ) ){
        col_missions["Plant East Field"] = _("Cost: $3.00/plot\n \n"
        "\n              .........\n              .........\n              .........\n              .........\n"
        "              .........\n              .........\n              ..#....**\n              ..#Ov..**\n  "
        "            ...O|....\n \n"
        "We'll plant the field with your choice of crop if you are willing to finance it.  When the crop is ready "
        "to harvest you can have us liquidate it or harvest it for you.");
        keys.push_back("Plant East Field");
        col_missions["Harvest East Field"] = _("Cost: $2.00/plot\n \n"
        "\n              .........\n              .........\n              .........\n              .........\n"
        "              .........\n              .........\n              ..#....**\n              ..#Ov..**\n  "
        "            ...O|....\n \n"
        "You can either have us liquidate the crop and give you the cash or pay us to harvest it for you.");
        keys.push_back("Harvest East Field");
    }

    if (id == "COMMUNE CROPS"){
        col_missions["Assign Ally to Forage for Food"] = _("Profit: $10/hour\nDanger: Low\nTime: 4 hour minimum\n \n"
            "Foraging for food involves dispatching a companion to search the surrounding wilderness for wild "
            "edibles.  Combat will be avoided but encounters with wild animals are to be expected.  The low pay is "
            "supplemented with the odd item as a reward for particularly large hauls.");
        keys.push_back("Assign Ally to Forage for Food");
        npc_list = companion_list( p, "_forage" );
        if (npc_list.size()>0){
            entry = _("Profit: $10/hour\nDanger: Low\nTime: 4 hour minimum\n \nLabor Roster:\n");
            for( auto &elem : npc_list ) {
                entry = entry + "  " + elem->name + " ["+ to_string( to_hours<int>( calendar::turn - elem->companion_mission_time ) ) +" hours] \n";
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Ally from Foraging"] = entry;
            keys.push_back("Recover Ally from Foraging");
        }
    }

    if (id == "COMMUNE CROPS" || id == "REFUGEE MERCHANT"){
        col_missions["Caravan Commune-Refugee Center"] = _("Profit: $18/hour\nDanger: High\nTime: UNKNOWN\n \n"
            "Adding companions to the caravan team increases the likelihood of success.  By nature, caravans are "
            "extremely tempting targets for raiders or hostile groups so only a strong party is recommended.  The "
            "rewards are significant for those participating but are even more important for the factions that profit.\n \n"
            "The commune is sending food to the Free Merchants in the Refugee Center as part of a tax and in exchange "
            "for skilled labor.");
        keys.push_back("Caravan Commune-Refugee Center");
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
                keys.push_back("Begin Commune-Refugee Center Run");
            }
            entry = entry + _("\n \nDo you wish to bring your allies back into your party?");
            col_missions["Recover Commune-Refugee Center"] = entry;
            keys.push_back("Recover Commune-Refugee Center");
        }
    }

    if (col_missions.empty()) {
        popup(_("There are no missions at this colony.  Press Spacebar..."));
        return false;
    }

    catacurses::window w_list = catacurses::newwin( FULL_SCREEN_HEIGHT, FULL_SCREEN_WIDTH,
                            ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0),
                            (TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0);
    catacurses::window w_info = catacurses::newwin( FULL_SCREEN_HEIGHT - 2, FULL_SCREEN_WIDTH - 1 - MAX_FAC_NAME_SIZE,
                            1 + ((TERMY > FULL_SCREEN_HEIGHT) ? (TERMY - FULL_SCREEN_HEIGHT) / 2 : 0),
                            MAX_FAC_NAME_SIZE + ((TERMX > FULL_SCREEN_WIDTH) ? (TERMX - FULL_SCREEN_WIDTH) / 2 : 0));

    int maxlength = FULL_SCREEN_WIDTH - 1 - MAX_FAC_NAME_SIZE;
    size_t sel = 0;
    bool redraw = true;


    input_context ctxt("FACTIONS");
    ctxt.register_action("UP", _("Move cursor up"));
    ctxt.register_action("DOWN", _("Move cursor down"));
    ctxt.register_action("CONFIRM");
    ctxt.register_action("QUIT");
    ctxt.register_action("HELP_KEYBINDINGS");
    std::string cur_key;
    while (true) {
        cur_key = keys[sel];
        if (redraw) {
            werase(w_list);
            draw_border(w_list);
            mvwprintz( w_list, 1, 1, c_white, title );
            for (size_t i = 0; i < keys.size(); i++) {
                nc_color col = (i == sel ? h_white : c_white);
                mvwprintz(w_list, i + 2, 1, col, "  %s", keys[i].c_str());
            }
            wrefresh(w_list);
            werase(w_info);
            fold_and_print(w_info, 0, 0, maxlength, c_white, col_missions[cur_key]);
            wrefresh(w_info);
            redraw = false;
        }
        const std::string action = ctxt.handle_input();
        if (action == "DOWN") {
            mvwprintz(w_list, sel + 2, 1, c_white, "-%s", cur_key.c_str());
            if (sel == keys.size() - 1) {
                sel = 0;    // Wrap around
            } else {
                sel++;
            }
            redraw = true;
        } else if (action == "UP") {
            mvwprintz(w_list, sel + 2, 1, c_white, "-%s", cur_key.c_str());
            if (sel == 0) {
                sel = keys.size() - 1;    // Wrap around
            } else {
                sel--;
            }
            redraw = true;
        } else if (action == "QUIT") {
            cur_key = "NONE";
            break;
        } else if (action == "CONFIRM") {
            break;
        }
    }
    g->refresh_all();

    if (cur_key == "Caravan Commune-Refugee Center"){
        individual_mission(p, _("joins the caravan team..."), "_commune_refugee_caravan", true);
    }
    if (cur_key == "Begin Commune-Refugee Center Run"){
        caravan_depart(p, "evac_center_18", "_commune_refugee_caravan");
    }
    if (cur_key == "Recover Commune-Refugee Center"){
        caravan_return(p, "evac_center_18", "_commune_refugee_caravan");
    }

    if (cur_key == "Purchase East Field"){
        field_build_1(p);
    }
    if (cur_key == "Upgrade East Field I"){
        field_build_2(p);
    }
    if (cur_key == "Plant East Field"){
        field_plant(p, "ranch_camp_63");
    }
    if (cur_key == "Harvest East Field"){
        field_harvest(p, "ranch_camp_63");
    }
    if (cur_key == "Assign Scavenging Patrol"){
        individual_mission(p, _("departs on the scavenging patrol..."), "_scavenging_patrol");
    }
    if (cur_key == "Retrieve Scavenging Patrol"){
        scavenging_patrol_return(p);
    }
    if (cur_key == "Assign Scavenging Raid"){
        individual_mission(p, _("departs on the scavenging raid..."), "_scavenging_raid");
    }
    if (cur_key == "Retrieve Scavenging Raid"){
        scavenging_raid_return(p);
    }
    if (cur_key == "Assign Ally to Menial Labor"){
        individual_mission(p, _("departs to work as a laborer..."), "_labor");
    }
    if (cur_key == "Recover Ally from Menial Labor"){
        labor_return(p);
    }
    if (cur_key == "Assign Ally to Carpentry Work"){
        individual_mission(p, _("departs to work as a carpenter..."), "_carpenter");
    }
    if (cur_key == "Recover Ally from Carpentry Work"){
        carpenter_return(p);
    }
    if (cur_key == "Assign Ally to Forage for Food"){
        individual_mission(p, _("departs to forage for food..."), "_forage");
    }
    if (cur_key == "Recover Ally from Foraging"){
        forage_return(p);
    }

    return true;
}

void talk_function::individual_mission( npc &p, std::string desc, std::string id, bool group )
{
    npc *comp = companion_choose();
    if (comp == NULL){
        return;
    }
    popup("%s %s", comp->name.c_str(), desc.c_str());
    comp->set_companion_mission( p, id );
    if (group){
        comp->companion_mission_time = calendar::before_time_starts;
    } else {
        comp->companion_mission_time = calendar::turn;
    }
    g->reload_npcs();
    assert( !comp->is_active() );
}

void talk_function::caravan_depart( npc &p, std::string dest, std::string id )
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
int talk_function::caravan_dist(std::string dest)
{
    const tripoint site = overmap_buffer.find_closest( g->u.global_omt_location(), dest, 0, false );
    int distance = rl_dist( g->u.pos(), site );
    return distance;
}

void talk_function::caravan_return( npc &p, std::string dest, std::string id )
{
    npc *comp = companion_choose_return( p.name + id, calendar::turn );
    if (comp == NULL){
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
    std::vector<std::shared_ptr<npc>> caravan_party, bandit_party;
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

    int y,i;
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
    bay.draw_square_ter(t_fence_h, 4, 3, 16, 3);
    bay.draw_square_ter(t_fence_h, 4, 15, 16, 15);
    bay.draw_square_ter(t_fence_v, 4, 3, 4, 15);
    bay.draw_square_ter(t_fence_v, 16, 3, 16, 15);
    bay.draw_square_ter(t_fencegate_c, 10, 3, 10, 3);
    bay.draw_square_ter(t_fencegate_c, 10, 15, 10, 15);
    bay.draw_square_ter(t_fencegate_c, 4, 9, 4, 9);
    bay.save();
    popup( _( "After counting your money %s directs a nearby laborer to begin constructing a fence around your plot..." ), p.name.c_str() );
}

void talk_function::field_plant( npc &p, std::string place )
{
    if (g->get_temperature() < 50) {
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
    bay.draw_square_ter(t_fence_h, 4, 3, 16, 3);
    bay.save();
    popup( _( "After counting your money and collecting your seeds, %s calls forth a labor party to plant your field." ), p.name.c_str() );
}

void talk_function::field_harvest( npc &p, std::string place )
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
    npc *comp = companion_choose_return( p.name + "_scavenging_patrol", calendar::turn - 10_hours );
    if (comp == NULL){
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

    int y, i = 0;
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
    npc *comp = companion_choose_return( p.name + "_scavenging_raid", calendar::turn - 10_hours );
    if (comp == NULL){
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

    int y,i=0;
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
    //minimum working time is 1 hour
    npc *comp = companion_choose_return( p.name + "_labor", calendar::turn - 1_hours );
    if (comp == NULL){
        return false;
    }

    //@todo actually it's hours, not turns
    float turns = to_hours<float>( calendar::turn - comp->companion_mission_time );
    int money = 8*turns;
    g->u.cash += money*100;

    int exp = turns;
    int y,i = 0;
    while (i < exp){
        y = rng( 0, 100 );
        if (y < 50){
            comp->practice( skill_fabrication, 5);
        } else if (y < 70){
            comp->practice( skill_survival, 5);
        } else if (y < 85){
            comp->practice( skill_mechanics, 5);
        } else if (y < 92){
            comp->practice( skill_speech, 5);
        } else{
            comp->practice( skill_cooking, 5);
        }
        i++;
    }

    popup(_("%s returns from working as a laborer having earned $%d and a bit of experience..."), comp->name.c_str(),money);
    companion_return( *comp );
    if ( turns >= 8 && one_in( 8 ) && !p.has_trait( trait_NPC_MISSION_LEV_1 ) ){
        p.set_mutation( trait_NPC_MISSION_LEV_1 );
        popup( _( "%s feels more confident in your companions and is willing to let them participate in advanced tasks." ), p.name.c_str() );
    }

    return true;
}

bool talk_function::carpenter_return( npc &p )
{
    npc *comp = companion_choose_return( p.name + "_carpenter", calendar::turn - 1_hours );
    if (comp == NULL){
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

    int exp = turns;
    int y,i = 0;
    while (i < exp){
        y = rng( 0, 100 );
        if (y < 70){
            comp->practice( skill_fabrication, 10 );
        } else if (y < 80){
            comp->practice( skill_survival, 10);
        } else if (y < 90){
            comp->practice( skill_mechanics, 10);
        } else {
            comp->practice( skill_speech, 10);
        }
        i++;
    }

    popup(_("%s returns from working as a carpenter having earned $%d and a bit of experience..."), comp->name.c_str(),money);
    companion_return( *comp );
    return true;
}

bool talk_function::forage_return( npc &p )
{
    npc *comp = companion_choose_return( p.name + "_forage", calendar::turn - 4_hours );
    if (comp == NULL){
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

    int exp = turns;
    int y,i = 0;
    while (i < exp){
        y = rng( 0, 100 );
        if (y < 60){
            comp->practice( skill_survival, 7);
        } else if (y < 75){
            comp->practice( skill_cooking, 7);
        } else if (y < 85){
            comp->practice( skill_traps, 7);
        } else if (y < 92){
            comp->practice( skill_archery, 7);
        } else{
            comp->practice( skill_gun, 7);
        }
        i++;
    }

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

void talk_function::force_on_force( std::vector<std::shared_ptr<npc>> defender, std::string def_desc,
    std::vector<std::shared_ptr<npc>> attacker, std::string att_desc, int advantage )
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
    int defense, attack;
    int att_init, def_init;
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
    // npc *may* be active, or not if outside the reality bubble
    g->reload_npcs();
}

std::vector<std::shared_ptr<npc>> talk_function::companion_list( const npc &p, const std::string &id )
{
    std::vector<std::shared_ptr<npc>> available;
    for( const auto &elem : overmap_buffer.get_companion_mission_npcs() ) {
        if( elem->get_companion_mission() == p.name + id ) {
            available.push_back( elem );
        }
    }
    return available;
}

npc *talk_function::companion_choose(){
    const std::vector<npc *> available = g->get_npcs_if( [&]( const npc &guy ) {
        return g->u.sees( guy.pos() ) && guy.is_friend() &&
            rl_dist( g->u.pos(), guy.pos() ) <= 24;
    } );

    if (available.empty()) {
        popup(_("You don't have any companions to send out..."));
        return NULL;
    }

    std::vector<std::string> npcs;
    for( auto &elem : available ) {
        npcs.push_back( ( elem )->name );
    }
    npcs.push_back(_("Cancel"));
    int npc_choice = menu_vec(true, _("Who do you want to send?"), npcs) - 1;
    if (npc_choice >= 0 && size_t(npc_choice) < available.size()) {
        return available[npc_choice];
    }
    popup("You choose to send no one...");
    return NULL;
}

npc *talk_function::companion_choose_return( std::string id, const time_point &deadline )
{
    std::vector<npc *> available;
    for( const auto &guy : overmap_buffer.get_companion_mission_npcs() ) {
        if( guy->get_companion_mission() == id && guy->companion_mission_time <= deadline ) {
            available.push_back( guy.get() );
        }
    }

    if (available.empty()) {
        popup(_("You don't have any companions ready to return..."));
        return NULL;
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
    return NULL;
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
