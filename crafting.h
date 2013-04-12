#ifndef _CRAFTING_H_
#define _CRAFTING_H_

#include <string>
#include <vector>
#include <map>
#include "itype.h"
#include "skill.h"
#include "rng.h"

#define MAX_DISPLAYED_RECIPES 18

typedef std::string craft_cat;

struct component
{
 itype_id type;
 int count;
 component() { type = "null"; count = 0; }
 component(itype_id TYPE, int COUNT) : type (TYPE), count (COUNT) {}
};

struct recipe {
  std::string ident;
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

recipe(std::string pident, int pid, itype_id pres, const char *p1, const char *p2,
       int pdiff, int ptime, bool preversible, bool pautolearn) :
  ident (pident), id (pid), result (pres), difficulty (pdiff), time (ptime), reversible (preversible),
  autolearn (pautolearn)
  {
    sk_primary = p1?Skill::skill(p1):NULL;
    sk_secondary = p2?Skill::skill(p2):NULL;
  }
};


typedef std::vector<recipe*> recipe_list;
typedef std::map<craft_cat, recipe_list> recipe_map;

#endif
