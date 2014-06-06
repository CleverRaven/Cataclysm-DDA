#ifndef _CRAFTING_H_
#define _CRAFTING_H_

#include <string>
#include <vector>
#include <map>
#include <list>
#include "itype.h"
#include "skill.h"
#include "rng.h"
#include "json.h"

#define MAX_DISPLAYED_RECIPES 18

typedef std::string craft_cat;
typedef std::string craft_subcat;

struct component
{
 itype_id type;
 int count;
 int available; // -1 means the player doesn't have the item, 1 means they do,
            // 0 means they have item but not enough for both tool and component
 component() { type = "null"; count = 0; available = -1;}
 component(itype_id TYPE, int COUNT) : type (TYPE), count (COUNT), available(-1) {}
};

struct quality_requirement
{
  std::string id;
  int count;
  bool available;
  int level;

  quality_requirement() { id = "UNKNOWN"; count = 0; available = false; level = 0;}
  quality_requirement(std::string new_id, int new_count, int new_level){
    id = new_id;
    count = new_count;
    level = new_level;
    available = false;
  }
};

struct quality
{
    std::string id;
    std::string name;
};

struct recipe {
  std::string ident;
  int id;
  itype_id result;
  craft_cat cat;
  craft_subcat subcat;
  Skill *skill_used;
  std::map<Skill*,int> required_skills;
  int difficulty;
  int time;
  bool reversible; // can the item be disassembled?
  bool autolearn; // do we learn it just by leveling skills?
  int learn_by_disassembly; // what level (if any) do we learn it by disassembly?
  int result_mult; // used by certain batch recipes that create more than one stack of the result

  std::vector<std::vector<component> > tools;
  std::vector<quality_requirement> qualities;
  std::vector<std::vector<component> > components;
  // only used during loading json data: books and the skill needed
  // to learn this recipe from.
  std::vector<std::pair<std::string, int> > booksets;

  //Create a string list to describe the skill requirements fir this recipe
  // Format: skill_name(amount), skill_name(amount)
  std::string required_skills_string();

  recipe() {
    id = 0;
    result = "null";
    skill_used = NULL;
    difficulty = 0;
    time = 0;
    reversible = false;
    autolearn = false;
    learn_by_disassembly = -1;
    result_mult = 1;
  }

recipe(std::string pident, int pid, itype_id pres, craft_cat pcat, craft_subcat psubcat, std::string &to_use,
       std::map<std::string,int> &to_require, int pdiff, int ptime, bool preversible, bool pautolearn,
       int plearn_dis, int pmult) :
  ident (pident), id (pid), result (pres), cat(pcat), subcat(psubcat), difficulty (pdiff), time (ptime),
  reversible (preversible), autolearn (pautolearn), learn_by_disassembly (plearn_dis), result_mult(pmult) {
    skill_used = to_use.size()?Skill::skill(to_use):NULL;
    if(!to_require.empty()){
        for(std::map<std::string,int>::iterator iter=to_require.begin(); iter!=to_require.end(); ++iter){
            required_skills[Skill::skill(iter->first)] = iter->second;
        }
    }
  }

  // Create an item instance as if the recipe was just finished,
  // Contain charges multiplier
  item create_result() const;
};

typedef std::vector<recipe*> recipe_list;
typedef std::map<craft_cat, recipe_list> recipe_map;

class item;
// removes any (removable) ammo from the item and stores it in the
// players inventory.
void remove_ammo(item *dis_item, player &p);
// same as above but for each item in the list
void remove_ammo(std::list<item> &dis_items, player &p);

void load_recipe_category(JsonObject &jsobj);
void reset_recipe_categories();
void load_recipe(JsonObject &jsobj);
void reset_recipes();
recipe* recipe_by_name(std::string name);
void finalize_recipes();
void reset_recipes_qualities();

extern recipe_map recipes; // The list of valid recipes

void load_quality(JsonObject &jo);
extern std::map<std::string,quality> qualities;

void check_recipe_definitions();

// Check that all components are known, print a message if not containing the display_name name
void check_component_list(const std::vector<std::vector<component> > &vec, const std::string &display_name);

#endif
