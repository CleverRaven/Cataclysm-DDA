#include "mission.h"
#include "setvector.h"
#include "game.h"

void game::init_missions()
{
 #define MISSION(name, goal, diff, val, urgent, place, start, end) \
 id++; mission_types.push_back( \
	mission_type(id, name, goal, diff, val, urgent, place, start, end) )

 #define ORIGINS(...) setvector(mission_types[id].origins, __VA_ARGS__)
 #define ITEM(itid)     mission_types[id].item_id = itid

// DEADLINE defines the low and high end time limits, in hours
// Omitting DEADLINE means the mission never times out
 #define DEADLINE(low, high) mission_types[id].deadline_low  = low  * 600;\
                             mission_types[id].deadline_high = high * 600
 //#define NPCS   (...) setvector(missions[id].npc


// The order of missions should match enum mission_id in mission.h
 int id = -1;

 MISSION("Null mission", MGOAL_NULL, 0, 0, false,
         &mission_place::never, &mission_start::standard,
         &mission_end::standard);

 MISSION("Find Antibiotics", MGOAL_FIND_ITEM, 2, 2000, true,
	&mission_place::always, &mission_start::standard,
	&mission_end::heal_infection);
  ORIGINS(ORIGIN_OPENER_NPC, NULL);
  ITEM(itm_antibiotics);
  DEADLINE(48, 72); // 2 - 3 days
}
