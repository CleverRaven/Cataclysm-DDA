#include "mission.h"
#include "game.h"
#include "translations.h"

void game::init_missions()
{
 #define MISSION(name, goal, diff, val, urgent, place, start, end, fail) \
 id++; mission_types.push_back( \
mission_type(id, name, goal, diff, val, urgent, place, start, end, fail) )

 #define ORIGINS(...) mission_types[id].origins = { __VA_ARGS__ }
 #define ITEM(itid)     mission_types[id].item_id = itid
 #define COUNT(num)     mission_types[id].item_count = num
 #define DESTINATION(dest)     mission_types[id].target_id = dest
 #define FOLLOWUP(next_up) mission_types[id].follow_up = next_up
// DEADLINE defines the low and high end time limits, in hours
// Omitting DEADLINE means the mission never times out
 #define DEADLINE(low, high) mission_types[id].deadline_low  = low  * 600;\
                             mission_types[id].deadline_high = high * 600


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
         &mission_end::leave, &mission_fail::standard);
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

 MISSION(_("Find a Book"), MGOAL_FIND_ANY_ITEM, 2, 800, false,
         &mission_place::always, &mission_start::place_book,
         &mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_ANY_NPC);
}
