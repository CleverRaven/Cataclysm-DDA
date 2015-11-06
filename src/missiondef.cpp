#include "mission.h"
#include "translations.h"
#include "rng.h"

std::vector<mission_type> mission_type::types;

void mission_type::initialize()
{
 #define MISSION(name, goal, diff, val, urgent, place, start, end, fail) \
 id++; types.push_back( \
mission_type(static_cast<mission_type_id>( id ), name, goal, diff, val, urgent, place, start, end, fail) )

 #define ORIGINS(...) types[id].origins = { __VA_ARGS__ }
 #define ITEM(itid)     types[id].item_id = itid
 #define COUNT(num)     types[id].item_count = num
 #define DESTINATION(dest)     types[id].target_id = dest
 #define FOLLOWUP(next_up) types[id].follow_up = next_up
// DEADLINE defines the low and high end time limits, in hours
// Omitting DEADLINE means the mission never times out
 #define DEADLINE(low, high) types[id].deadline_low  = low  * 600;\
                             types[id].deadline_high = high * 600


// The order of missions should match enum mission_id in mission.h
 int id = -1;

 MISSION("Null mission", MGOAL_NULL, 0, 0, false,
         &mission_place::never, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);

 MISSION(_("Find Antibiotics"), MGOAL_FIND_ITEM, 2, 150000, true,
         &mission_place::always, &mission_start::infect_npc,
         &mission_end::heal_infection, &mission_fail::kill_npc);
  ORIGINS(ORIGIN_OPENER_NPC);
  ITEM("antibiotics");
  DEADLINE(24, 48); // 1 - 2 days

 MISSION(_("Retrieve Software"), MGOAL_FIND_ANY_ITEM, 2, 80000, false,
         &mission_place::near_town, &mission_start::place_npc_software,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);

 MISSION(_("Analyze Zombie Blood"), MGOAL_FIND_ITEM, 8, 250000, false,
         &mission_place::always, &mission_start::reveal_hospital,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("software_blood_data");

 MISSION(_("Find Lost Dog"), MGOAL_FIND_MONSTER, 3, 100000, false,
         &mission_place::near_town, &mission_start::place_dog,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC);

 MISSION(_("Kill Zombie Mom"), MGOAL_KILL_MONSTER, 5, 120000, true,
         &mission_place::near_town, &mission_start::place_zombie_mom,
         &mission_end::thankful, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);

 MISSION(_("Reach Safety"), MGOAL_GO_TO, 1, 0, false,
         &mission_place::always, &mission_start::find_safety,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_NULL);

//patriot mission 1
MISSION(_("Find Flag"), MGOAL_FIND_ITEM, 2, 100000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  FOLLOWUP(MISSION_GET_BLACK_BOX);
  ITEM("american_flag");

//patriot mission 2
 MISSION(_("Retrieve Military Black Box"), MGOAL_FIND_ITEM, 2, 100000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_GET_BLACK_BOX_TRANSCRIPT);
  ITEM("black_box");

//patriot mission 3
 MISSION(_("Retrieve Black Box Transcript"), MGOAL_FIND_ITEM, 2, 150000, false,
         &mission_place::always, &mission_start::reveal_lab_black_box,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_EXPLORE_SARCOPHAGUS);
  ITEM("black_box_transcript");

//patriot mission 4
 MISSION(_("Follow Sarcophagus Team"), MGOAL_GO_TO_TYPE, 2, 50000, false,
         &mission_place::always, &mission_start::open_sarcophagus,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  DESTINATION("haz_sar_b1");

//martyr mission 1
 MISSION(_("Find Relic"), MGOAL_FIND_ITEM, 2, 100000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  FOLLOWUP(MISSION_RECOVER_PRIEST_DIARY);
  ITEM("small_relic");

//martyr mission 2
 MISSION(_("Recover Priest's Diary"), MGOAL_FIND_ITEM, 2, 70000, false,
         &mission_place::always, &mission_start::place_priest_diary,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_INVESTIGATE_CULT);
  ITEM("priest_diary");

//martyr mission 3
 MISSION(_("Investigate Cult"), MGOAL_FIND_ITEM, 2, 150000, false,
         &mission_place::always, &mission_start::point_cabin_strange,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_INVESTIGATE_PRISON_VISIONARY);
  ITEM("etched_skull");

//martyr mission 4
 MISSION(_("Prison Visionary"), MGOAL_FIND_ITEM, 2, 150000, false,
         &mission_place::always, &mission_start::point_prison,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("visions_solitude");

 MISSION(_("Find Weather Log"), MGOAL_FIND_ITEM, 2, 50000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  ITEM("record_weather");

//humanitarian mission 1
 MISSION(_("Find Patient Records"), MGOAL_FIND_ITEM, 2, 60000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  FOLLOWUP(MISSION_REACH_FEMA_CAMP);
  ITEM("record_patient");

//humanitarian mission 2
 MISSION(_("Reach FEMA Camp"), MGOAL_GO_TO_TYPE, 2, 60000, false,
         &mission_place::always, &mission_start::join,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  DESTINATION("fema");
  FOLLOWUP(MISSION_REACH_FARM_HOUSE);

//humanitarian mission 3
 MISSION(_("Reach Farm House"), MGOAL_GO_TO_TYPE, 2, 60000, false,
         &mission_place::always, &mission_start::join,
         &mission_end::leave, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  DESTINATION("farm");

//vigilante mission 1
 MISSION(_("Find Corporate Accounts"), MGOAL_FIND_ITEM, 2, 140000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  FOLLOWUP(MISSION_GET_SAFE_BOX);
  ITEM("record_accounting");

//vigilante mission 2
 MISSION(_("Retrieve Deposit Box"), MGOAL_FIND_ITEM, 2, 30000, false,
         &mission_place::always, &mission_start::place_deposit_box,
         &mission_end::deposit_box, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_GET_DEPUTY_BADGE);
  ITEM("safe_box");

//vigilante mission 3
 MISSION(_("Find Deputy Badge"), MGOAL_FIND_ITEM, 2, 150000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("badge_deputy");

 //demon slayer mission 1
 MISSION(_("Kill Jabberwock"), MGOAL_KILL_MONSTER, 5, 200000, true,
         &mission_place::always, &mission_start::place_jabberwock,
         &mission_end::thankful, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  FOLLOWUP(MISSION_KILL_100_Z);

//demon slayer mission 2
 MISSION(_("Kill 100 Zombies"), MGOAL_KILL_MONSTER_TYPE, 5, 250000, false,
         &mission_place::always, &mission_start::kill_100_z,
         &mission_end::leave, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_KILL_HORDE_MASTER);

//demon slayer mission 3
 MISSION(_("Kill Horde Master"),MGOAL_KILL_MONSTER, 5, 250000, true,
         &mission_place::always, &mission_start::kill_horde_master,
         &mission_end::leave, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_RECRUIT_TRACKER);

//demon slayer mission 4
 MISSION(_("Recruit Tracker"),MGOAL_RECRUIT_NPC_CLASS, 5, 70000, false,
         &mission_place::always, &mission_start::recruit_tracker,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);

//demon slayer mission 4b
 MISSION(_("Tracker"), MGOAL_FIND_ITEM, 5, 0, true,
         &mission_place::always, &mission_start::join,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("badge_deputy");

////Free Merchants
 MISSION(_("Clear Back Bay"), MGOAL_KILL_MONSTER, 2, 50000, false,
         &mission_place::always, &mission_start::place_zombie_bay,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);//So it won't spawn on random npcs
  FOLLOWUP(MISSION_FREE_MERCHANTS_EVAC_2);

 MISSION(_("Missing Caravan"), MGOAL_ASSASSINATE, 5, 5000, false,
         &mission_place::always, &mission_start::place_caravan_ambush,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_FREE_MERCHANTS_EVAC_3);

 MISSION(_("Find 25 Plutonium Cells"), MGOAL_FIND_ITEM, 5, 400000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("plut_cell");
  COUNT(25);

////Old Guard
 MISSION(_("Kill Bandits"), MGOAL_ASSASSINATE, 5, 50000, false,
         &mission_place::always, &mission_start::place_bandit_cabin,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_OLD_GUARD_REP_2);

 MISSION(_("Deal with Informant"), MGOAL_ASSASSINATE, 5, 50000, false,
         &mission_place::always, &mission_start::place_informant,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_OLD_GUARD_REP_3);

 MISSION(_("Kill ???"), MGOAL_KILL_MONSTER, 5, 100000, false,
         &mission_place::always, &mission_start::place_grabber,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_OLD_GUARD_REP_4);

 MISSION(_("Kill Raider Leader"), MGOAL_ASSASSINATE, 10, 300000, false,
         &mission_place::always, &mission_start::place_bandit_camp,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);

 MISSION(_("Locate Commo Team"), MGOAL_FIND_ITEM, 2, 250000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("necropolis_freq");
  FOLLOWUP(MISSION_OLD_GUARD_NEC_2);

 MISSION(_("Cull Nightmares"), MGOAL_KILL_MONSTER_TYPE, 5, 250000, false,
         &mission_place::always, &mission_start::kill_20_nightmares,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);

 MISSION(_("Fabricate Repeater Mod"), MGOAL_FIND_ITEM, 2, 250000, false,
         &mission_place::always, &mission_start::radio_repeater,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("radio_repeater_mod");
  FOLLOWUP(MISSION_OLD_GUARD_NEC_COMMO_2);

 MISSION(_("Disable External Power"), MGOAL_COMPUTER_TOGGLE, 2, 150000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_OLD_GUARD_NEC_COMMO_3);

 MISSION(_("Install Repeater Mod"), MGOAL_COMPUTER_TOGGLE, 2, 300000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_OLD_GUARD_NEC_COMMO_4);

 MISSION(_("Install Repeater Mod"), MGOAL_COMPUTER_TOGGLE, 2, 350000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_OLD_GUARD_NEC_COMMO_4);
 ////Tacoma Commune
 MISSION(_("Cut 200 2x4's"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("2x4");
  COUNT(200);
  FOLLOWUP(MISSION_RANCH_FOREMAN_2);

 MISSION(_("Find 25 Blankets"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_1,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("blanket");
  COUNT(25);
  FOLLOWUP(MISSION_RANCH_FOREMAN_3);

 MISSION(_("Gather 2500 Nails"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_2,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("nail");
  COUNT(2500);
  FOLLOWUP(MISSION_RANCH_FOREMAN_4);

 MISSION(_("Gather 300 Salt"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_3,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("salt");
  COUNT(300);
  FOLLOWUP(MISSION_RANCH_FOREMAN_5);

 MISSION(_("30 Liquid Fertilizer"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_4,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("fertilizer_liquid");
  COUNT(30);
  FOLLOWUP(MISSION_RANCH_FOREMAN_6);


 MISSION(_("Gather 75 Stones"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_5,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("rock");
  COUNT(75);
  FOLLOWUP(MISSION_RANCH_FOREMAN_7);

 MISSION(_("Gather 50 Pipes"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_6,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("pipe");
  COUNT(50);
  FOLLOWUP(MISSION_RANCH_FOREMAN_8);

 MISSION(_("Gather 2 Motors"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_7,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("motor");
  COUNT(2);
  FOLLOWUP(MISSION_RANCH_FOREMAN_9);

 MISSION(_("Gather 150 Bleach"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_8,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("bleach");
  COUNT(150);
  FOLLOWUP(MISSION_RANCH_FOREMAN_10);

 MISSION(_("Gather 6 First Aid Kits"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_9,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("1st_aid");
  COUNT(6);
  FOLLOWUP(MISSION_RANCH_FOREMAN_11);

 MISSION(_("Find 2 Electric Welders"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_10,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("welder");
  COUNT(2);
  FOLLOWUP(MISSION_RANCH_FOREMAN_12);

 MISSION(_("Find 12 Car Batteries"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_11,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("battery_car");
  COUNT(12);
  FOLLOWUP(MISSION_RANCH_FOREMAN_13);

 MISSION(_("Find 2 Two-Way Radios"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_12,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("two_way_radio");
  COUNT(2);
  FOLLOWUP(MISSION_RANCH_FOREMAN_14);

 MISSION(_("Gather 5 Backpacks"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_13,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("backpack");
  COUNT(5);
  FOLLOWUP(MISSION_RANCH_FOREMAN_15);

 MISSION(_("Find Homebrewer's Bible"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_14,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("brewing_cookbook");
  COUNT(1);
  FOLLOWUP(MISSION_RANCH_FOREMAN_16);

 MISSION(_("Gather 80 Sugar"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_15,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("sugar");
  COUNT(80);
  FOLLOWUP(MISSION_RANCH_FOREMAN_17);

 MISSION(_("Collect 30 Glass Sheets"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_construct_16,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("glass_sheet");
  COUNT(30);
  FOLLOWUP(MISSION_RANCH_FOREMAN_17);

//Commune nurse missions
 MISSION(_("Collect 100 Aspirin"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("aspirin");
  COUNT(100);
  FOLLOWUP(MISSION_RANCH_NURSE_2);

 MISSION(_("Collect 3 Hotplates"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_nurse_1,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("hotplate");
  COUNT(3);
  FOLLOWUP(MISSION_RANCH_NURSE_3);

 MISSION(_("Collect 200 Vitamins"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_nurse_2,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("vitamins");
  COUNT(200);
  FOLLOWUP(MISSION_RANCH_NURSE_4);

 MISSION(_("Make 4 Charcoal Purifiers"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_nurse_3,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("char_purifier");
  COUNT(4);
  FOLLOWUP(MISSION_RANCH_NURSE_5);

 MISSION(_("Find a Chemistry Sets"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_nurse_4,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("chemistry_set");
  FOLLOWUP(MISSION_RANCH_NURSE_6);

 MISSION(_("Find 10 Filter Masks"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_nurse_5,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("mask_filter");
  COUNT(10);
  FOLLOWUP(MISSION_RANCH_NURSE_7);

 MISSION(_("Find 4 Pairs of Rubber Gloves"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_nurse_6,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("gloves_rubber");
  COUNT(4);
  FOLLOWUP(MISSION_RANCH_NURSE_8);

 MISSION(_("Find 2 Scalpels"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_nurse_7,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("scalpel");
  COUNT(2);
  FOLLOWUP(MISSION_RANCH_NURSE_9);

 MISSION(_("Find Advanced Emergency Care"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_nurse_8,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("emergency_book");
  FOLLOWUP(MISSION_RANCH_NURSE_10);

 MISSION(_("Find a Flu Shot"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_nurse_9,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("flu_shot");
  FOLLOWUP(MISSION_RANCH_NURSE_11);

 MISSION(_("Find 10 Syringes"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("syringe");
  COUNT(3);
  FOLLOWUP(MISSION_RANCH_NURSE_11);

 MISSION(_("Make 12 Knife Spears"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("spear_knife");
  COUNT(12);
  FOLLOWUP(MISSION_RANCH_SCAVENGER_2);

 MISSION(_("Make 5 Wearable Flashlights"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_scavenger_1,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("wearable_light");
  COUNT(5);
  FOLLOWUP(MISSION_RANCH_SCAVENGER_3);

 MISSION(_("Make 3 Leather Body Armor"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_scavenger_2,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("armor_larmor");
  COUNT(3);
  FOLLOWUP(MISSION_RANCH_SCAVENGER_4);

 MISSION(_("Make 12 Molotov Cocktails"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_scavenger_3,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("molotov");
  COUNT(12);
  FOLLOWUP(MISSION_RANCH_SCAVENGER_4);

 MISSION(_("Make 2 Stills"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("still");
  COUNT(2);
  FOLLOWUP(MISSION_RANCH_BARTENDER_2);

 MISSION(_("Find 20 Yeast"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_bartender_1,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("yeast");
  COUNT(20);
  FOLLOWUP(MISSION_RANCH_BARTENDER_3);

 MISSION(_("Find 10 Sugar Beet Seeds"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_bartender_2,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("seed_sugar_beet");
  COUNT(10);
  FOLLOWUP(MISSION_RANCH_BARTENDER_4);

 MISSION(_("Find 12 Metal Tanks"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_bartender_3,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("metal_tank");
  COUNT(12);
  FOLLOWUP(MISSION_RANCH_BARTENDER_5);

 MISSION(_("Find 2 55-Gallon Drums"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::ranch_bartender_4,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("55gal_drum");
  COUNT(2);
  FOLLOWUP(MISSION_RANCH_BARTENDER_5);


 MISSION(_("Retrieve Prospectus"), MGOAL_FIND_ITEM, 5, 50000, false,
         &mission_place::always, &mission_start::start_commune,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("commune_prospectus");
  FOLLOWUP(MISSION_FREE_MERCHANTS_EVAC_4);

 MISSION(_("Find a Book"), MGOAL_FIND_ANY_ITEM, 2, 800, false,
         &mission_place::always, &mission_start::place_book,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_ANY_NPC);
}

const mission_type *mission_type::get( const mission_type_id id )
{
    for( auto & t : types ) {
        if( t.id == id ) {
            return &t;
        }
    }
    return nullptr;
}

const std::vector<mission_type> &mission_type::get_all()
{
    return types;
}

mission_type_id mission_type::get_random_id( const mission_origin origin, const tripoint &p )
{
    std::vector<mission_type_id> valid;
    mission_place place;
    for( auto & t : types ) {
        if( std::find( t.origins.begin(), t.origins.end(), origin ) == t.origins.end() ) {
            continue;
        }
        if( ( place.*t.place )( p ) ) {
            valid.push_back( t.id );
        }
    }
    return random_entry( valid, MISSION_NULL );
}

void mission_type::reset() {
    types.clear();
}
