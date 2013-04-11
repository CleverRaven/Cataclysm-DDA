#ifndef _CRAFTING_H_
#define _CRAFTING_H_

#include <string>
#include <vector>
#include <map>
#include "itype.h"
#include "skill.h"
#include "rng.h"

#define MAX_DISPLAYED_RECIPES 18

enum craft_cat {
CC_NULL = 0,
CC_WEAPON,
CC_AMMO,
CC_FOOD,
CC_DRINK,
CC_CHEM,
CC_ELECTRONIC,
CC_ARMOR,
CC_MISC,
CC_NONCRAFT,
NUM_CC
};

struct component
{
 itype_id type;
 int count;
 component() { type = "null"; count = 0; }
 component(itype_id TYPE, int COUNT) : type (TYPE), count (COUNT) {}
};

struct recipe {
  int id;
  itype_id result;
  Skill *sk_primary;
  Skill *sk_secondary;
  int difficulty;
  int time;
  bool reversible; // can the item be disassembled?
  bool autolearn; // do we learn it just by leveling skills?

  std::vector<component> tools[20];
  std::vector<component> components[20];

  recipe() {
    id = 0;
    result = "null";
    sk_primary = NULL;
    sk_secondary = NULL;
    difficulty = 0;
    time = 0;
    reversible = false;
    autolearn = false;
  }

recipe(int pid, itype_id pres, const char *p1, const char *p2,
       int pdiff, int ptime, bool preversible, bool pautolearn) :
  id (pid), result (pres), difficulty (pdiff), time (ptime), reversible (preversible), autolearn (pautolearn)
  {
    sk_primary = p1?Skill::skill(p1):NULL;
    sk_secondary = p2?Skill::skill(p2):NULL;
  }
};


typedef std::vector<recipe*> recipe_list;
typedef std::map<craft_cat, recipe_list> recipe_map;

#endif
