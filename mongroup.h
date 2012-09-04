#ifndef _MONGROUP_H_
#define _MONGROUP_H_

#include "mtype.h"
#include <vector>

enum moncat_id {
 mcat_null = 0,
 mcat_forest,
 mcat_ant,
 mcat_bee,
 mcat_worm,
 mcat_zombie,
 mcat_triffid,
 mcat_fungi,
 mcat_goo,
 mcat_chud,
 mcat_sewer,
 mcat_swamp,
 mcat_lab,
 mcat_nether,
 mcat_spiral,
 mcat_vanilla_zombie,	// Defense mode only
 mcat_spider,		// Defense mode only
 mcat_robot,		// Defense mode only
 num_moncats
};

bool moncat_is_safe(moncat_id id);

struct mongroup {
 moncat_id type;
 int posx, posy;
 unsigned char radius;
 unsigned int population;
 bool dying;
 mongroup(moncat_id ptype, int pposx, int pposy, unsigned char prad,
          unsigned int ppop) {
  type = ptype;
  posx = pposx;
  posy = pposy;
  radius = prad;
  population = ppop;
  dying = false;
 }
 bool is_safe() { return moncat_is_safe(type); };
};

#endif
