#include "mission.h"
#include "setvector.h"
#include "game.h"

void game::init_missions()
{
 #define MISSION(name, goal, diff, val, urgent, place, start, end, fail) \
 id++; mission_types.push_back( \
mission_type(id, name, goal, diff, val, urgent, place, start, end, fail) )

 #define ORIGINS(...) setvector(&mission_types[id].origins, __VA_ARGS__, NULL)
 #define ITEM(itid)     mission_types[id].item_id = itid
 #define DESTINATION(dest)     mission_types[id].target_id = dest
 #define FOLLOWUP(next_up) mission_types[id].follow_up = next_up
// DEADLINE defines the low and high end time limits, in hours
// Omitting DEADLINE means the mission never times out
 #define DEADLINE(low, high) mission_types[id].deadline_low  = low  * 600;\
                             mission_types[id].deadline_high = high * 600
 //#define NPCS   (...) setvector(missions[id].npc


// The order of missions should match enum mission_id in mission.h
 int id = -1;

 MISSION("Null mission", MGOAL_NULL, 0, 0, false,
         &mission_place::never, &mission_start::standard,
         &mission_end::standard, &mission_fail::standard);

 MISSION("Find Antibiotics", MGOAL_FIND_ITEM, 2, 1500, true,
	&mission_place::always, &mission_start::infect_npc,
	&mission_end::heal_infection, &mission_fail::kill_npc);
  ORIGINS(ORIGIN_OPENER_NPC);
  ITEM("antibiotics");
  DEADLINE(24, 48); // 1 - 2 days

 MISSION("Retrieve Software", MGOAL_FIND_ANY_ITEM, 2, 800, false,
	&mission_place::near_town, &mission_start::place_npc_software,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);

 MISSION("Analyze Zombie Blood", MGOAL_FIND_ITEM, 8, 2500, false,
	&mission_place::always, &mission_start::reveal_hospital,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("software_blood_data");

 MISSION("Find Lost Dog", MGOAL_FIND_MONSTER, 3, 1000, false,
	&mission_place::near_town, &mission_start::place_dog,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC);

 MISSION("Kill Zombie Mom", MGOAL_KILL_MONSTER, 5, 1200, true,
	&mission_place::near_town, &mission_start::place_zombie_mom,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);

 MISSION("Reach Safety", MGOAL_GO_TO, 1, 0, false,
	&mission_place::always, &mission_start::find_safety,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_NULL);

//patriot mission 1
MISSION("Find Flag", MGOAL_FIND_ITEM, 2, 1000, false,
	&mission_place::always, &mission_start::standard,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  FOLLOWUP(MISSION_GET_BLACK_BOX);
  ITEM("american_flag");

//patriot mission 2
 MISSION("Retrieve Military Black Box", MGOAL_FIND_ITEM, 2, 1000, false,
	&mission_place::always, &mission_start::standard,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  FOLLOWUP(MISSION_GET_BLACK_BOX_TRANSCRIPT);
  ITEM("black_box");

//patriot mission 3
 MISSION("Retrieve Black Box Transcript", MGOAL_FIND_ITEM, 2, 1500, false,
	&mission_place::always, &mission_start::reveal_lab_black_box,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  ITEM("black_box_transcript");

 MISSION("Find Relic", MGOAL_FIND_ITEM, 2, 1000, false,
	&mission_place::always, &mission_start::standard,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_ANY_NPC);
  ITEM("small_relic");

 MISSION("Find Weather Log", MGOAL_FIND_ITEM, 2, 500, false,
	&mission_place::always, &mission_start::standard,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  ITEM("record_weather");

//humanitarian 1
 MISSION("Find Patient Records", MGOAL_FIND_ITEM, 2, 600, false,
	&mission_place::always, &mission_start::standard,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  FOLLOWUP(MISSION_REACH_FEMA_CAMP);
  ITEM("record_patient");

//humanitarian 2
 MISSION("Reach FEMA Camp", MGOAL_GO_TO_TYPE, 2, 600, false,
	&mission_place::always, &mission_start::join,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  DESTINATION(ot_fema);
  FOLLOWUP(MISSION_REACH_FARM_HOUSE);

//humanitarian 3
 MISSION("Reach Farm House", MGOAL_GO_TO_TYPE, 2, 600, false,
	&mission_place::always, &mission_start::join,
	&mission_end::leave, &mission_fail::standard);
  ORIGINS(ORIGIN_SECONDARY);
  DESTINATION(ot_farm);

 MISSION("Find Corporate Accounts", MGOAL_FIND_ITEM, 2, 1400, false,
	&mission_place::always, &mission_start::standard,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);
  ITEM("record_accounting");
  
 MISSION("Kill Jabberwock", MGOAL_KILL_MONSTER, 5, 2500, true,
	&mission_place::always, &mission_start::place_jabberwock,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_OPENER_NPC, ORIGIN_ANY_NPC);

 MISSION("Find a Book", MGOAL_FIND_ANY_ITEM, 2, 800, false,
	&mission_place::always, &mission_start::place_book,
	&mission_end::standard, &mission_fail::standard);
  ORIGINS(ORIGIN_ANY_NPC);
}
