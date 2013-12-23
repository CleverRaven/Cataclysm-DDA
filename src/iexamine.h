//
//  iexamine.h
//  Cataclysm
//
//  Livingstone
//

#ifndef __Cataclysmic__iexamine__
#define __Cataclysmic__iexamine__

 //#include <iostream>

class game;
class item;
class player;
class map;

class iexamine
{
public:
 void none (player *p, map *m, int examx, int examy);

 void gaspump (player *p, map *m, int examx, int examy);
 void toilet (player *p, map *m, int examx, int examy);
 void deep_fryer(player *p, map *m, int examx, int examy);
 void elevator (player *p, map *m, int examx, int examy);
 void controls_gate(player *p, map *m, int examx, int examy);
 void cardreader (player *p, map *m, int examx, int examy);
 void rubble (player *p, map *m, int examx, int examy);
 void chainfence (player *p, map *m, int examx, int examy);
 void tent (player *p, map *m, int examx, int examy);
 void shelter (player *p, map *m, int examx, int examy);
 void wreckage (player *p, map *m, int examx, int examy);
 void pit (player *p, map *m, int examx, int examy);
 void pit_covered (player *p, map *m, int examx, int examy);
 void fence_post (player *p, map *m, int examx, int examy);
 void remove_fence_rope (player *p, map *m, int examx, int examy);
 void remove_fence_wire (player *p, map *m, int examx, int examy);
 void remove_fence_barbed (player *p, map *m, int examx, int examy);
 void slot_machine (player *p, map *m, int examx, int examy);
 void safe (player *p, map *m, int examx, int examy);
 void bulletin_board (player *p, map *m, int examx, int examy);
 void fault (player *p, map *m, int examx, int examy);
 void pedestal_wyrm (player *p, map *m, int examx, int examy);
 void pedestal_temple (player *p, map *m, int examx, int examy);
 void fswitch (player *p, map *m, int examx, int examy);
 void flower_poppy (player *p, map *m, int examx, int examy);
 void fungus (player *p, map *m, int examx, int examy);
 void dirtmound (player *p, map *m, int examx, int examy);
 void aggie_plant (player *p, map *m, int examx, int examy);
 void pick_plant(player *p, map *m, int examx, int examy, std::string itemType, int new_ter, bool seeds = false);
 void tree_apple (player *p, map *m, int examx, int examy);
 void shrub_blueberry (player *p, map *m, int examx, int examy);
 void shrub_strawberry (player *p, map *m, int examx, int examy);
 void shrub_marloss (player *p, map *m, int examx, int examy);
 void shrub_wildveggies (player *p, map *m, int examx, int examy);
 void recycler (player *p, map *m, int examx, int examy);
 void trap(player *p, map *m, int examx, int examy);
 void water_source (player *p, map *m, const int examx, const int examy);
 void acid_source (player *p, map *m, const int examx, const int examy);
};

typedef void (iexamine::*iexamine_function)(player*, map*, int, int);
iexamine_function iexamine_function_from_string(std::string function_name);

#endif /* defined(__Cataclysmic__iexamine__) */
