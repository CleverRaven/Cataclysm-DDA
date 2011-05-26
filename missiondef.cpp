#include "mission.h"
#include "setvector.h"
#include "game.h"

void game::init_missions()
{
 #define MISSION(name, goal, diff, place, start, end) \
 id++; mission_types.push_back( \
		mission_type(name, goal, diff, place, start, end) )

 #define ORIGINS(...) setvector(mission_types[id].origins, __VA_ARGS__)
 #define INTROS(...)  setvector(mission_types[id].intros,  __VA_ARGS__)
 #define ITEMS(...)   setvector(mission_types[id].items,   __VA_ARGS__)

// DEADLINE defines the low and high end time limits, in hours
// Omitting DEADLINE means the mission never times out
 #define DEADLINE(low, high) mission_types[id].deadline_low  = low  * 600;\
                             mission_types[id].deadline_high = high * 600
 //#define NPCS   (...) setvector(missions[id].npc


// The order of missions should match enum mission_id in mission.h
 int id = -1;

 MISSION("Null mission", MGOAL_NULL, 0,
         &mission_place::never, &mission_start::standard,
         &mission_end::standard);

 MISSION("Reach safety", MGOAL_GO_TO, 0, 
         &mission_place::danger, &mission_start::danger, &mission_end::demo);
   ORIGINS( ORIGIN_GAME_START, NULL);

 MISSION("Find your brother", MGOAL_GO_TO, 0,
	&mission_place::always, &mission_start::find_family,
	&mission_end::find_family);
   ORIGINS( ORIGIN_GAME_START, NULL);

 MISSION("Follow <GOODNPC> to safety", MGOAL_GO_TO, 0,
         &mission_place::danger, &mission_start::standard,
         &mission_end::standard);
   ORIGINS( ORIGIN_OPENER_NPC, NULL);
   INTROS( "You need to follow <GOODNPC> to a safe area.",
           "<GOODNPC> is leading you to safety.  Follow <HIM>.", NULL);
   //NPCS( MISSION_NPC_ANY);


 MISSION("Meet up with <GOODFAC>", MGOAL_GO_TO, 2,
         &mission_place::never, &mission_start::standard,
         &mission_end::join_faction);
   ORIGINS( ORIGIN_SECONDARY, NULL);
   INTROS( "<GOODNPC> is a member of <GOODFAC>.  Meet up with them.",
           "<GOODNPC> says you can find help with <GOODFAC>.", NULL);
    //NPCS( MISSION_NPC_FRIEND);
    //FRIENDFAC( MISSION_FAC_ANY);


 MISSION("Collect a cure", MGOAL_FIND_ITEM, 10,
         &mission_place::get_jelly, &mission_start::get_jelly,
         &mission_end::get_jelly);

   ORIGINS( ORIGIN_RADIO_TOWER, ORIGIN_NPC_MISC, ORIGIN_TOWN_BOARD, NULL);
   INTROS( "Royal jelly is needed urgently to cure fungal infection.",
           "We need royal jelly ASAP to cure fungitis.", NULL);
   ITEMS( itm_royal_jelly, NULL);
   DEADLINE( 24, 72);

/* In this version, the afflicted individual does not know that royal jelly
 * cures fungitis, and the nearest hive will not be marked on your map.
 */
 MISSION("Collect a cure", MGOAL_FIND_ITEM, 12,
         &mission_place::get_jelly, &mission_start::get_jelly_ignt,
         &mission_end::get_jelly_ignt);

   ORIGINS( ORIGIN_RADIO_TOWER, ORIGIN_NPC_MISC, ORIGIN_TOWN_BOARD, NULL);
   INTROS( "We have someone here who's caughing up spores, and need help.",
           "One of our <FRIENDS> is very sick; we think it's a parasite.",NULL);
   ITEMS( itm_royal_jelly, NULL);
   DEADLINE( 36, 84);


 MISSION("Find missing person", MGOAL_FIND_NPC, 8,
         &mission_place::lost_npc, &mission_start::lost_npc,
         &mission_end::lost_npc);

   ORIGINS( ORIGIN_RADIO_TOWER, ORIGIN_NPC_MISC, ORIGIN_NPC_FACTION,
            ORIGIN_TOWN_BOARD, NULL);
   INTROS( "We have a <FRIEND> who hasn't been seen for several days.",
           "We are worried about a <FRIEND> who's been missing for a while.",
           NULL);
   //NPCS( MISSION_NPC_ROOKIE);
 

 MISSION("Rescue kidnapping victim", MGOAL_FIND_NPC, 16,
         &mission_place::kidnap_victim, &mission_start::kidnap_victim,
         &mission_end::kidnap_victim);

   ORIGINS( ORIGIN_RADIO_TOWER, ORIGIN_NPC_FACTION, ORIGIN_TOWN_BOARD, NULL);
   INTROS( "<ENEMYFAC> has kidnapped a <FRIEND>, and we want them back.",
           "A <FRIEND> has been kidnapped by <ENEMYFAC>.  We need help.", NULL);
   //NPCS( MISSION_NPC_ANY);
   //ENEMYFAC(  MISSION_FAC_AGGRO);
   //FRIENDFAC( MISSION_FAC_ANY);
}

