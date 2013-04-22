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
 void none		(game *g, player *p, map *m, int examx, int examy);

 void gaspump (game *g, player *p, map *m, int examx, int examy);
 void elevator (game *g, player *p, map *m, int examx, int examy);
 void controls_gate(game *g, player *p, map *m, int examx, int examy);
 void cardreader (game *g, player *p, map *m, int examx, int examy);
 void rubble (game *g, player *p, map *m, int examx, int examy);
 void chainfence (game *g, player *p, map *m, int examx, int examy);
 void tent (game *g, player *p, map *m, int examx, int examy);
 void shelter (game *g, player *p, map *m, int examx, int examy);
 void wreckage (game *g, player *p, map *m, int examx, int examy);
 void pit (game *g, player *p, map *m, int examx, int examy);
 void pit_spiked (game *g, player *p, map *m, int examx, int examy);
 void pit_covered (game *g, player *p, map *m, int examx, int examy);
 void pit_spiked_covered (game *g, player *p, map *m, int examx, int examy);
 void fence_post (game *g, player *p, map *m, int examx, int examy);
 void remove_fence_rope (game *g, player *p, map *m, int examx, int examy);
 void remove_fence_wire (game *g, player *p, map *m, int examx, int examy);
 void remove_fence_barbed (game *g, player *p, map *m, int examx, int examy);
 void slot_machine (game *g, player *p, map *m, int examx, int examy);
 void bulletin_board (game *g, player *p, map *m, int examx, int examy);
 void fault (game *g, player *p, map *m, int examx, int examy);
 void pedestal_wyrm (game *g, player *p, map *m, int examx, int examy);
 void pedestal_temple (game *g, player *p, map *m, int examx, int examy);
 void fswitch (game *g, player *p, map *m, int examx, int examy);
 void flower_poppy (game *g, player *p, map *m, int examx, int examy);
 void tree_apple (game *g, player *p, map *m, int examx, int examy);
 void shrub_blueberry (game *g, player *p, map *m, int examx, int examy);
 void shrub_wildveggies (game *g, player *p, map *m, int examx, int examy);
 void recycler (game *g, player *p, map *m, int examx, int examy);
 void trap(game *g, player *p, map *m, int examx, int examy);
 void water_source (game *g, player *p, map *m, const int examx, const int examy);
 
};

#endif /* defined(__Cataclysmic__iexamine__) */
