#ifndef _CRAFTING_H_
#define _CRAFTING_H_

#include <string>
#include <vector>
#include <map>
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

  std::vector<std::vector<component> > tools;
  std::vector<quality_requirement> qualities;
  std::vector<std::vector<component> > components;

  //Create a string list to describe the skill requirements fir this recipe
  // Format: skill_name(amount), skill_name(amount)
  std::string required_skills_string(){
      std::ostringstream skills_as_stream;
      if(!required_skills.empty()){
          for(std::map<Skill*,int>::iterator iter=required_skills.begin(); iter!=required_skills.end();){
            skills_as_stream << iter->first->name() << "(" << iter->second << ")";
            ++iter;
            if(iter != required_skills.end()){
              skills_as_stream << ", ";
            }
          }
      }
      else{
        skills_as_stream << "N/A";
      }
      return skills_as_stream.str();
  }

  recipe() {
    id = 0;
    result = "null";
    skill_used = NULL;
    difficulty = 0;
    time = 0;
    reversible = false;
    autolearn = false;
    learn_by_disassembly = -1;
  }

recipe(std::string pident, int pid, itype_id pres, craft_cat pcat, craft_subcat psubcat, std::string &to_use,
       std::map<std::string,int> &to_require, int pdiff, int ptime, bool preversible, bool pautolearn,
       int plearn_dis) :
  ident (pident), id (pid), result (pres), cat(pcat), subcat(psubcat), difficulty (pdiff), time (ptime),
  reversible (preversible), autolearn (pautolearn), learn_by_disassembly (plearn_dis) {
    skill_used = to_use.size()?Skill::skill(to_use):NULL;
    if(!to_require.empty()){
        for(std::map<std::string,int>::iterator iter=to_require.begin(); iter!=to_require.end(); ++iter){
            required_skills[Skill::skill(iter->first)] = iter->second;
        }
    }
  }
};

typedef std::vector<recipe*> recipe_list;
typedef std::map<craft_cat, recipe_list> recipe_map;

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

#endif
