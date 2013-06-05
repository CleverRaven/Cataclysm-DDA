#ifndef _BUILDING_GENERATION_H_
#define _BUILDING_GENERATION_H_

#include "omdata.h"
#include "map.h"

struct mapgendata
{
public:
  oter_id t_nesw[4];
  int n_fac; // dir == 0
  int e_fac; // dir == 1
  int s_fac; // dir == 2
  int w_fac; // dir == 3
  
  mapgendata(oter_id north, oter_id east, oter_id south, oter_id west);
  void set_dir(int dir_in, int val);
  void fill(int val);
  int& dir(int dir_in);
};

ter_id grass_or_dirt();
ter_id dirt_or_pile();

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null(map *m);
void mapgen_crater(map *m, mapgendata dat);
void mapgen_field(map *m, int turn);
void mapgen_dirtlot(map *m, game *g);
void mapgen_forest_general(map *m, oter_id terrain_type, mapgendata dat, int turn);

#endif
