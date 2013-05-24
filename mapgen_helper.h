#ifndef _MAPGEN_HELPER_H_
#define _MAPGEN_HELPER_H_

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
  void set_dir(int dir, int val);
};

// helper functions for mapgen.cpp, so that we can avoid having a massive switch statement (sorta)
void mapgen_null(map *m);

#endif
