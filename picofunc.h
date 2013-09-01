#ifndef _PICOFUNC_H_
#define _PICOFUNC_H_
#include "picojson.h"
/* these defines will go away when all of them are complete, which means
   #define jsonsave_full
   is a reality.
*/
#define jsonsave_item 1 // complete, optimized to skip values if default. Affects: game.sav and maps.txt
#define jsonsave_monster 1 // complete, unoptimized (not needed): game.sav
// #define jsonsave_player 1 // pending: .sav
// #define jsonsave_misc 1 // misc stuff in game.sav.
// #define jsonsave_invlet_cache 1 // pending: game.sav, overmap
// #define jsonsave_inventory 1 // requires jsonsave_invlet_cache: game.sav, overmap
/* 
   npcs can be jsonized chunks in overmap, instead of stringsteam chunks
*/
// #define jsonsave_npc 1 // pending
// #define 


#define pv(x) picojson::value(x)

bool picoverify();
bool picobool(const picojson::object & obj, std::string key, bool & var);
bool picoint(const picojson::object & obj, std::string key, int & var);
bool picostring(const picojson::object & obj, std::string key, std::string & var);

#endif
