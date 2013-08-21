#ifndef _PLDATA_H_
#define _PLDATA_H_

#include "enums.h"
#include "translations.h"
#include <sstream>
#include <vector>
#include <map>

enum character_type {
 PLTYPE_CUSTOM,
 PLTYPE_RANDOM,
 PLTYPE_TEMPLATE,
 PLTYPE_NOW,
 PLTYPE_MAX
};

typedef std::string dis_type;

enum add_type {
 ADD_NULL,
 ADD_CAFFEINE, ADD_ALCOHOL, ADD_SLEEP, ADD_PKILLER, ADD_SPEED, ADD_CIG,
 ADD_COKE, ADD_CRACK,
};

struct disease
{
 dis_type type;
 int intensity;
 int duration;
 disease() { type = "null"; duration = 0; intensity = 0; }
 disease(dis_type t, int d, int i = 0) { type = t; duration = d; intensity = i;}
};

struct addiction
{
 add_type type;
 int intensity;
 int sated;
 addiction() { type = ADD_NULL; intensity = 0; sated = 600; }
 addiction(add_type t) { type = t; intensity = 1; sated = 600; }
 addiction(add_type t, int i) { type = t; intensity = i; sated = 600; }
};

enum activity_type {
 ACT_NULL = 0,
 ACT_RELOAD, ACT_READ, ACT_GAME, ACT_WAIT, ACT_CRAFT, ACT_LONGCRAFT,
 ACT_DISASSEMBLE, ACT_BUTCHER, ACT_FORAGE, ACT_BUILD, ACT_VEHICLE, ACT_REFILL_VEHICLE,
 ACT_TRAIN,
 NUM_ACTIVITIES
};

struct player_activity
{
 activity_type type;
 int moves_left;
 int index;
 char invlet;
 std::string name;
 bool continuous;
 bool ignore_trivial;
 std::vector<int> values;
 point placement;

 player_activity() { type = ACT_NULL; moves_left = 0; index = -1; invlet = 0;
                     name = ""; placement = point(-1, -1); continuous = false; }

 player_activity(activity_type t, int turns, int Index, char ch, std::string name_in)
 {
  type = t;
  moves_left = turns;
  index = Index;
  invlet = ch;
  name = name_in;
  placement = point(-1, -1);
  continuous = false;
  ignore_trivial = false;
 }

 player_activity(const player_activity &copy)
 {
  type = copy.type;
  moves_left = copy.moves_left;
  index = copy.index;
  invlet = copy.invlet;
  name = copy.name;
  placement = copy.placement;
  continuous = copy.continuous;
  ignore_trivial = copy.ignore_trivial;
  values.clear();
  for (int i = 0; i < copy.values.size(); i++)
   values.push_back(copy.values[i]);
 }

 std::string save_info()
 {
  std::stringstream ret;
  // name can be empty, so make sure we prepend something to it
  ret << type << " " << moves_left << " " << index << " " << invlet << " str:" << name << " "
         << placement.x << " " << placement.y << " " << values.size();
  for (int i = 0; i < values.size(); i++)
   ret << " " << values[i];

  return ret.str();
 }

 void load_info(std::stringstream &dump)
 {
  int tmp, tmptype;
  std::string tmpname;
  dump >> tmptype >> moves_left >> index >> invlet >> tmpname >> placement.x >> placement.y >> tmp;
  name = tmpname.substr(4);
  type = activity_type(tmptype);
  for (int i = 0; i < tmp; i++) {
   int tmp2;
   dump >> tmp2;
   values.push_back(tmp2);
  }
 }
};

struct trait {
    std::string name;
    int points;		// How many points it costs in character creation
    int visiblity;		// How visible it is
    int ugliness;		// How ugly it is
    bool startingtrait; // Starting Trait True/False
    std::string description;
};

extern std::map<std::string, trait> traits;

enum hp_part {
    hp_head = 0,
    hp_torso,
    hp_arm_l,
    hp_arm_r,
    hp_leg_l,
    hp_leg_r,
    num_hp_parts
};
#endif
