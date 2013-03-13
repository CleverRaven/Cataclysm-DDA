#ifndef _MONGROUP_H_
#define _MONGROUP_H_

#include "mtype.h"
#include <vector>
#include <map>

typedef std::map<mon_id, int> FreqDef;
typedef FreqDef::iterator FreqDef_iter;

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

struct MonsterGroup
{
    mon_id defaultMonster;
    FreqDef  monsters;
};

//Adding a group:
//  1: Declare it here
//  2: Define it in game::init_mongroups() (mongroupdef.cpp)
enum MonsterGroupType
{
    GROUP_NULL = 0,
    GROUP_FOREST,
    GROUP_ANT,
    GROUP_BEE,
    GROUP_WORM,
    GROUP_ZOMBIE,
    GROUP_TRIFFID,
    GROUP_FUNGI,
    GROUP_GOO,
    GROUP_CHUD,
    GROUP_SEWER,
    GROUP_SWAMP,
    GROUP_LAB,
    GROUP_NETHER,
    GROUP_SPIRAL,
    GROUP_VANILLA,
    GROUP_SPIDER,
    GROUP_ROBOT,
    GROUP_COUNT
};

struct mongroup {
 MonsterGroupType type; //
 int posx, posy;
 unsigned char radius;
 unsigned int population;
 bool dying;
 bool diffuse;   // group size ind. of dist. from center and radius invariant
 mongroup(MonsterGroupType ptype, int pposx, int pposy, unsigned char prad,
          unsigned int ppop) {
  type = ptype;
  posx = pposx;
  posy = pposy;
  radius = prad;
  population = ppop;
  dying = false;
  diffuse = false;
 }
 bool is_safe() { return (type == GROUP_NULL || type == GROUP_FOREST); };
};

mon_id GetMonsterFromGroup(MonsterGroupType);
bool IsMonsterInGroup(MonsterGroupType , mon_id);
MonsterGroupType Monster2Group(mon_id);

#endif
