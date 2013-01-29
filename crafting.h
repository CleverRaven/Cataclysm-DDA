#ifndef _CRAFTING_H_
#define _CRAFTING_H_

#include <string>
#include <vector>
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
 component() { type = itm_null; count = 0; }
 component(itype_id TYPE, int COUNT) : type (TYPE), count (COUNT) {}
};

struct recipe
{
 int id;
 itype_id result;
 craft_cat category;
 skill sk_primary;
 skill sk_secondary;
 int difficulty;
 int time;
 bool reversible; // can the item be disassembled?

 std::vector<component> tools[5];
 std::vector<component> components[10];

 recipe() { id = 0; result = itm_null; category = CC_NULL; sk_primary = sk_null;
            sk_secondary = sk_null; difficulty = 0; time = 0; reversible = false; }
 recipe(int pid, itype_id pres, craft_cat cat, skill p1, skill p2, int pdiff,
        int ptime, bool preversible) :
  id (pid), result (pres), category (cat), sk_primary (p1), sk_secondary (p2),
  difficulty (pdiff), time (ptime), reversible (preversible) {}
};

void consume_items(game *g, std::vector<component> components);
void consume_tools(game *g, std::vector<component> tools);

#endif
