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
 int available; // -1 means the player doesn't have the item, 1 means they do,
            // 0 means they have item but not enough for both tool and component
 component() { type = "null"; count = 0; available = -1;}
 component(itype_id TYPE, int COUNT) : type (TYPE), count (COUNT), available(-1) {}
};

struct recipe {
  std::string ident;
  int id;
  itype_id result;
  craft_cat cat;
  Skill *skill_used;
  std::map<Skill*,int> required_skills;
  int difficulty;
  int time;
  bool reversible; // can the item be disassembled?
  bool autolearn; // do we learn it just by leveling skills?
  int learn_by_disassembly; // what level (if any) do we learn it by disassembly?

  std::vector<std::vector<component> > tools;
  std::vector<std::vector<component> > components;

  //Create a string list to describe the skill requirements fir this recipe
  // Format: skill_name(amount), skill_name(amount)
  std::string required_skills_string(){
      std::ostringstream skills_as_stream;
      if(required_skills.size()){
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
  }

recipe(std::string pident, int pid, itype_id pres, craft_cat pcat, std::string &to_use,
       std::map<std::string,int> &to_require, int pdiff, int ptime, bool preversible, bool pautolearn,
       int plearn_dis) :
  ident (pident), id (pid), result (pres), cat(pcat), difficulty (pdiff), time (ptime),
  reversible (preversible), autolearn (pautolearn), learn_by_disassembly (plearn_dis) {
    skill_used = to_use.size()?Skill::skill(to_use):NULL;
    if(to_require.size()){
        for(std::map<std::string,int>::iterator iter=to_require.begin(); iter!=to_require.end(); ++iter){
            required_skills[Skill::skill(iter->first)] = iter->second;
        }
    }
  }
};


typedef std::vector<recipe*> recipe_list;
typedef std::map<craft_cat, recipe_list> recipe_map;

#endif
