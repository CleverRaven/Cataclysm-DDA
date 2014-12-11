#include <fstream>
#include <sstream>

#include "npc.h"
#include "rng.h"
#include "map.h"
#include "game.h"
#include "bodypart.h"
#include "skill.h"
#include "output.h"
#include "line.h"
#include "item_group.h"
#include "translations.h"
#include "monstergenerator.h"
#include "overmapbuffer.h"
#include "messages.h"
#include "json.h"
#include <algorithm>
#include <string>

std::vector<item> starting_clothes(npc_class type, bool male);
std::list<item> starting_inv(npc *me, npc_class type);

npc::npc()
{
 mapx = 0;
 mapy = 0;
 mapz = 0;
 posx = -1;
 posy = -1;
 wandx = 0;
 wandy = 0;
 wandf = 0;
 plx = 999;
 ply = 999;
 plt = 999;
 itx = -1;
 ity = -1;
 guardx = -1;
 guardy = -1;
 goal = no_goal_point;
 fatigue = 0;
 hunger = 0;
 thirst = 0;
 fetching_item = false;
 has_new_items = false;
 worst_item_value = 0;
 str_max = 0;
 dex_max = 0;
 int_max = 0;
 per_max = 0;
 my_fac = NULL;
 fac_id = "";
 miss_id = 0;
 marked_for_death = false;
 dead = false;
 hit_by_player = false;
 moves = 100;
 mission = NPC_MISSION_NULL;
 myclass = NC_NONE;
 patience = 0;
 restock = -1;
 for( auto &skill : Skill::skills ) {
     set_skill_level( skill, 0 );
 }
}

npc_map npc::_all_npc;

void npc::load_npc(JsonObject &jsobj)
{
    npc guy;
    guy.idz = jsobj.get_string("id");
    if (jsobj.has_string("name+"))
        guy.name = jsobj.get_string("name+");
    if (jsobj.has_string("gender")){
        if (jsobj.get_string("gender") == "male"){
            guy.male = true;
        }else{
            guy.male = false;
        }
    }
    if (jsobj.has_string("faction"))
        guy.fac_id = jsobj.get_string("faction");
    guy.myclass = npc_class(jsobj.get_int("class"));
    guy.attitude = npc_attitude(jsobj.get_int("attitude"));
    guy.mission = npc_mission(jsobj.get_int("mission"));
    guy.chatbin.first_topic = talk_topic(jsobj.get_int("chat"));
    if (jsobj.has_int("mission_offered")){
        guy.miss_id = jsobj.get_int("mission_offered");
    } else {
        guy.miss_id = 0;
    }
    _all_npc[guy.idz] = guy;
}

npc* npc::find_npc(std::string ident)
{
    npc_map::iterator found = _all_npc.find(ident);
    if (found != _all_npc.end()){
        return &(found->second);
    } else {
        debugmsg("Tried to get invalid npc template: %s", ident.c_str());
        static npc null_npc;
    return &null_npc;
    }
}

void npc::load_npc_template(std::string ident)
{
    npc_map::iterator found = _all_npc.find(ident);
    if (found != _all_npc.end()){
        idz = found->second.idz;
        myclass = found->second.myclass;
        randomize(myclass);
        std::string tmpname = found->second.name.c_str();
        if (tmpname[0] == ','){
            name = name + found->second.name;
        } else {
            name = found->second.name;
            //Assume if the name is unique, the gender might also be.
            male = found->second.male;
        }
        fac_id = found->second.fac_id;
        set_fac(fac_id);
        attitude = found->second.attitude;
        mission = found->second.mission;
        chatbin.first_topic = found->second.chatbin.first_topic;
        if (mission_id(found->second.miss_id) != MISSION_NULL){
            int mission_index = g->reserve_mission(mission_id(found->second.miss_id), getID());
            if (mission_index != -1)
                chatbin.missions.push_back(mission_index);
        }
        return;
    } else {
        debugmsg("Tried to get invalid npc: %s", ident.c_str());
        return;
    }
}

npc::~npc() { }

std::string npc::save_info()
{
    return serialize(); // also saves contents
}

void npc::load_info(std::string data)
{
    std::stringstream dump;
    dump << data;

    char check = dump.peek();
    if ( check == ' ' ) {
        // sigh..
        check = data[1];
    }
    if ( check == '{' ) {
        JsonIn jsin(dump);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad npc json\n%s", jsonerr.c_str() );
        }
        if (fac_id != ""){
            set_fac(fac_id);
        }
        return;
    } else {
        load_legacy(dump);
    }
}


void npc::randomize(npc_class type)
{
 this->setID(g->assign_npc_id());
 str_max = dice(4, 3);
 dex_max = dice(4, 3);
 int_max = dice(4, 3);
 per_max = dice(4, 3);
 ret_null = item("null", 0);
 weapon   = item("null", 0);
 inv.clear();
 personality.aggression = rng(-10, 10);
 personality.bravery =    rng( -3, 10);
 personality.collector =  rng( -1, 10);
 personality.altruism =   rng(-10, 10);
 cash = 100000 * rng(0, 10) + 10000 * rng(0, 20) + 100 * rng(0, 30) + + 1 * rng(0, 30), rng(0, 99);
 moves = 100;
 mission = NPC_MISSION_NULL;
 if (one_in(2))
  male = true;
 else
  male = false;
 pick_name();

 npc_class typetmp;
 if (type == NC_NONE){
  typetmp = npc_class(rng(0, NC_MAX - 1));
  if (typetmp != NC_SHOPKEEP) //Exclude unique classes from random NPCs here
    type = typetmp;
  if (one_in(5))
    type = NC_NONE;
 }

 myclass = type;
 switch (type) { // Type of character
 case NC_NONE: // Untyped; no particular specialization
     for( auto &skill : Skill::skills ) {
   int level = 0;
   if (one_in(3))
   {
    level = dice(4, 2) - rng(1, 4);
   }
   set_skill_level( skill, level );
  }
  break;

 case NC_EVAC_SHOPKEEP:
     for( auto &skill : Skill::skills ) {
   int level = 0;
   if (one_in(3))
   {
    level = dice(2, 2) - 2 + (rng(0, 1) * rng(0, 1));
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("mechanics", rng(0, 1));
  boost_skill_level("electronics", rng(1, 2));
  boost_skill_level("speech", rng(1, 3));
  boost_skill_level("barter", rng(3, 5));
  int_max += rng(0, 1) * rng(0, 1);
  per_max += rng(0, 1) * rng(0, 1);
  personality.collector += rng(1, 5);
  cash = 100000 * rng(1, 10)+ rng(1, 100000);
  this->restock = 14400*3;  //Every three days
  break;

 case NC_ARSONIST:
     for( auto &skill : Skill::skills ) {
   int level = dice(3, 2) - rng(0, 4);
   if (level < 0)
   {
    level = 0;
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("gun", rng(1, 3));
  boost_skill_level("pistol", rng(1, 3));
  boost_skill_level("throw", rng(0, 2));
  boost_skill_level("barter", rng(2, 4));
  int_max -= rng(0, 2);
  dex_max -= rng(0, 2);
  per_max += rng(0, 2);
  personality.aggression += rng(0, 1);
  personality.collector += rng(0, 2);
  cash = 25000 * rng(1, 10)+ rng(1, 1000);
  this->restock = 14400*3;  //Every three days
  break;

 case NC_HUNTER:
     for( auto &skill : Skill::skills ) {
   int level = dice(3, 2) - rng(0, 4);
   if (level < 0)
   {
    level = 0;
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("barter", rng(2, 5));
  boost_skill_level("gun", rng(2, 4));
  if (one_in(3)){
    boost_skill_level("rifle", rng(2, 4));
  } else {
    boost_skill_level("archery", rng(2, 4));
  }
  str_max -= rng(0, 2);
  dex_max -= rng(1, 3);
  per_max += rng(2, 4);
  cash = 15000 * rng(1, 10)+ rng(1, 1000);
  this->restock = 14400*3;  //Every three days
  break;

 case NC_HACKER:
     for( auto &skill : Skill::skills ) {
   int level = 0;
   if (one_in(3))
   {
    level = dice(2, 2) - rng(1, 2);
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("electronics", rng(1, 4));
  boost_skill_level("computer", rng(3, 6));
  str_max -= rng(0, 4);
  dex_max -= rng(0, 2);
  int_max += rng(1, 5);
  per_max -= rng(0, 2);
  personality.bravery -= rng(1, 3);
  personality.aggression -= rng(0, 2);
  break;

 case NC_DOCTOR:
     for( auto &skill : Skill::skills ) {
   int level = 0;
   if (one_in(3))
   {
    level = dice(3, 2) - rng(1, 3);
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("firstaid", rng(2, 6));
  str_max -= rng(0, 2);
  int_max += rng(0, 2);
  per_max += rng(0, 1) * rng(0, 1);
  personality.aggression -= rng(0, 4);
  if (one_in(4))
   flags |= mfb(NF_DRUGGIE);
  cash += 10000 * rng(0, 3) * rng(0, 3);
  break;

 case NC_TRADER:
     for( auto &skill : Skill::skills ) {
   int level = 0;
   if (one_in(3))
   {
    level = dice(2, 2) - 2 + (rng(0, 1) * rng(0, 1));
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("mechanics", rng(0, 2));
  boost_skill_level("electronics", rng(0, 2));
  boost_skill_level("speech", rng(0, 3));
  boost_skill_level("barter", rng(2, 5));
  int_max += rng(0, 1) * rng(0, 1);
  per_max += rng(0, 1) * rng(0, 1);
  personality.collector += rng(1, 5);
  cash += 25000 * rng(1, 10);
  break;

 case NC_NINJA:
     for( auto &skill : Skill::skills ) {
   int level = 0;
   if (one_in(3))
   {
    level = dice(2, 2) - rng(1, 2);
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("dodge", rng(2, 4));
  boost_skill_level("melee", rng(1, 4));
  boost_skill_level("unarmed", rng(4, 6));
  boost_skill_level("throw", rng(0, 2));
  str_max -= rng(0, 1);
  dex_max += rng(0, 2);
  per_max += rng(0, 2);
  personality.bravery += rng(0, 3);
  personality.collector -= rng(1, 6);
  // TODO: give ninja his styles back
  break;

 case NC_COWBOY:
     for( auto &skill : Skill::skills ) {
   int level = dice(3, 2) - rng(0, 4);
   if (level < 0)
   {
    level = 0;
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("gun", rng(1, 3));
  boost_skill_level("pistol", rng(1, 3));
  boost_skill_level("rifle", rng(0, 2));
  int_max -= rng(0, 2);
  str_max += rng(0, 1);
  per_max += rng(0, 2);
  personality.aggression += rng(0, 2);
  personality.bravery += rng(1, 5);
  break;

 case NC_SCIENTIST:
     for( auto &skill : Skill::skills ) {
   int level = dice(3, 2) - 4;
   if (level < 0)
   {
    level = 0;
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("computer", rng(0, 3));
  boost_skill_level("electronics", rng(0, 3));
  boost_skill_level("firstaid", rng(0, 1));
  switch (rng(1, 3)) { // pick a speciality
   case 1: boost_skill_level("computer", rng(2, 6)); break;
   case 2: boost_skill_level("electronics", rng(2, 6)); break;
   case 3: boost_skill_level("firstaid", rng(2, 6)); break;
  }
  if (one_in(4))
   flags |= mfb(NF_TECHNOPHILE);
  if (one_in(3))
   flags |= mfb(NF_BOOKWORM);
  str_max -= rng(1, 3);
  dex_max -= rng(0, 1);
  int_max += rng(2, 5);
  personality.aggression -= rng(1, 5);
  personality.bravery -= rng(2, 8);
  personality.collector += rng (0, 2);
  break;

 case NC_BOUNTY_HUNTER:
     for( auto &skill : Skill::skills ) {
   int level = dice(3, 2) - 3;
   if (level > 0 && one_in(3))
   {
    level--;
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("gun", rng(2, 4));
  boost_skill_level(Skill::random_skill_with_tag("gun"), rng(3, 5));
  personality.aggression += rng(1, 6);
  personality.bravery += rng(0, 5);
  break;

 case NC_THUG:
     for( auto &skill : Skill::skills ) {
   int level = dice(3, 2) - 3;
   if (level > 0 && one_in(3))
   {
    level--;
   }
   set_skill_level( skill, level );
  }
  str_max -= rng(2, 4);
  dex_max -= rng(0, 2);
  boost_skill_level("dodge", rng(1, 3));
  boost_skill_level("melee", rng(2, 4));
  boost_skill_level("unarmed", rng(1, 3));
  boost_skill_level("bashing", rng(1, 5));
  boost_skill_level("stabbing", rng(1, 5));
  boost_skill_level("unarmed", rng(1, 3));
  personality.aggression += rng(1, 6);
  personality.bravery += rng(0, 5);
  break;

 case NC_SCAVENGER:
     for( auto &skill : Skill::skills ) {
   int level = dice(3, 2) - 3;
   if (level > 0 && one_in(3))
   {
    level--;
   }
   set_skill_level( skill, level );
  }
  boost_skill_level("gun", rng(2, 4));
  boost_skill_level("pistol", rng(2, 5));
  boost_skill_level("rifle", rng(0, 3));
  boost_skill_level("archery", rng(0, 3));
  personality.aggression += rng(1, 3);
  personality.bravery += rng(1, 4);
  break;
  
 default:
    //Suppress warnings
    break;

 }
  //A universal barter boost to keep NPCs competitive with players
 //The int boost from trade wasn't active... now that it is, most
 //players will vastly outclass npcs in trade without a little help.
 boost_skill_level("barter", rng(2, 4));
 
 for (int i = 0; i < num_hp_parts; i++) {
  hp_max[i] = 60 + str_max * 3;
  hp_cur[i] = hp_max[i];
 }
 starting_weapon(type);
 worn = starting_clothes(type, male);
 inv.clear();
 inv.add_stack(starting_inv(this, type));
 update_worst_item_value();
}

void npc::randomize_from_faction(faction *fac)
{
// Personality = aggression, bravery, altruism, collector
 my_fac = fac;
 fac_id = fac->id;
 randomize();

 switch (fac->goal) {
  case FACGOAL_DOMINANCE:
   personality.aggression += rng(0, 3);
   personality.bravery += rng(1, 4);
   personality.altruism -= rng(0, 2);
   break;
  case FACGOAL_CLEANSE:
   personality.aggression -= rng(0, 3);
   personality.bravery += rng(2, 4);
   personality.altruism += rng(0, 3);
   personality.collector -= rng(0, 2);
   break;
  case FACGOAL_SHADOW:
   personality.bravery += rng(4, 7);
   personality.collector -= rng(0, 3);
   int_max += rng(0, 2);
   per_max += rng(0, 2);
   break;
  case FACGOAL_APOCALYPSE:
   personality.aggression += rng(2, 5);
   personality.bravery += rng(4, 8);
   personality.altruism -= rng(1, 4);
   personality.collector -= rng(2, 5);
   break;
  case FACGOAL_ANARCHY:
   personality.aggression += rng(3, 6);
   personality.bravery += rng(0, 4);
   personality.altruism -= rng(3, 8);
   personality.collector -= rng(3, 6);
   int_max -= rng(1, 3);
   per_max -= rng(0, 2);
   str_max += rng(0, 3);
   break;
  case FACGOAL_KNOWLEDGE:
   if (one_in(2))
    randomize(NC_SCIENTIST);
   personality.aggression -= rng(2, 5);
   personality.bravery -= rng(1, 4);
   personality.collector += rng(2, 4);
   int_max += rng(2, 5);
   str_max -= rng(1, 4);
   per_max -= rng(0, 2);
   dex_max -= rng(0, 2);
   break;
  case FACGOAL_NATURE:
   personality.aggression -= rng(0, 3);
   personality.collector -= rng(1, 4);
   break;
  case FACGOAL_CIVILIZATION:
   personality.aggression -= rng(2, 4);
   personality.altruism += rng(1, 5);
   personality.collector += rng(1, 5);
   break;
  default:
    //Suppress warnings
    break;
 }
// Jobs
 if (fac->has_job(FACJOB_EXTORTION)) {
  personality.aggression += rng(0, 3);
  personality.bravery -= rng(0, 2);
  personality.altruism -= rng(2, 6);
 }
 if (fac->has_job(FACJOB_INFORMATION)) {
  int_max += rng(0, 4);
  per_max += rng(0, 4);
  personality.aggression -= rng(0, 4);
  personality.collector += rng(1, 3);
 }
 if (fac->has_job(FACJOB_TRADE) || fac->has_job(FACJOB_CARAVANS)) {
  if (!one_in(3))
   randomize(NC_TRADER);
  personality.aggression -= rng(1, 5);
  personality.collector += rng(1, 4);
  personality.altruism -= rng(0, 3);
 }
 if (fac->has_job(FACJOB_SCAVENGE))
  personality.collector += rng(4, 8);
 if (fac->has_job(FACJOB_MERCENARIES)) {
  if (!one_in(3)) {
   switch (rng(1, 3)) {
    case 1: randomize(NC_NINJA);  break;
    case 2: randomize(NC_COWBOY);  break;
    case 3: randomize(NC_BOUNTY_HUNTER); break;
   }
  }
  personality.aggression += rng(0, 2);
  personality.bravery += rng(2, 4);
  personality.altruism -= rng(2, 4);
  str_max += rng(0, 2);
  per_max += rng(0, 2);
  dex_max += rng(0, 2);
 }
 if (fac->has_job(FACJOB_ASSASSINS)) {
  personality.bravery -= rng(0, 2);
  personality.altruism -= rng(1, 3);
  per_max += rng(1, 3);
  dex_max += rng(0, 2);
 }
 if (fac->has_job(FACJOB_RAIDERS)) {
  if (one_in(3))
   randomize(NC_COWBOY);
  personality.aggression += rng(3, 5);
  personality.bravery += rng(0, 2);
  personality.altruism -= rng(3, 6);
  str_max += rng(0, 3);
  int_max -= rng(0, 2);
 }
 if (fac->has_job(FACJOB_THIEVES)) {
  if (one_in(3))
   randomize(NC_NINJA);
  personality.aggression -= rng(2, 5);
  personality.bravery -= rng(1, 3);
  personality.altruism -= rng(1, 4);
  str_max -= rng(0, 2);
  per_max += rng(1, 4);
  dex_max += rng(1, 3);
 }
 if (fac->has_job(FACJOB_DOCTORS)) {
  if (!one_in(4))
   randomize(NC_DOCTOR);
  personality.aggression -= rng(3, 6);
  personality.bravery += rng(0, 4);
  personality.altruism += rng(0, 4);
  int_max += rng(2, 4);
  per_max += rng(0, 2);
  boost_skill_level("firstaid", rng(1, 5));
 }
 if (fac->has_job(FACJOB_FARMERS)) {
  personality.aggression -= rng(2, 4);
  personality.altruism += rng(0, 3);
  str_max += rng(1, 3);
 }
 if (fac->has_job(FACJOB_DRUGS)) {
  personality.aggression -= rng(0, 2);
  personality.bravery -= rng(0, 3);
  personality.altruism -= rng(1, 4);
 }
 if (fac->has_job(FACJOB_MANUFACTURE)) {
  personality.aggression -= rng(0, 2);
  personality.bravery -= rng(0, 2);
  switch (rng(1, 4)) {
   case 1: boost_skill_level("mechanics", dice(2, 4));   break;
   case 2: boost_skill_level("electronics", dice(2, 4)); break;
   case 3: boost_skill_level("cooking", dice(2, 4));     break;
   case 4: boost_skill_level("tailor", dice(2,  4));     break;
  }
 }

 if (fac->has_value(FACVAL_CHARITABLE)) {
  personality.aggression -= rng(2, 5);
  personality.bravery += rng(0, 4);
  personality.altruism += rng(2, 5);
 }
 if (fac->has_value(FACVAL_LONERS)) {
  personality.aggression -= rng(1, 3);
  personality.altruism -= rng(1, 4);
 }
 if (fac->has_value(FACVAL_EXPLORATION)) {
  per_max += rng(0, 4);
  personality.aggression -= rng(0, 2);
 }
 if (fac->has_value(FACVAL_ARTIFACTS)) {
  personality.collector += rng(2, 5);
  personality.altruism -= rng(0, 2);
 }
 if (fac->has_value(FACVAL_BIONICS)) {
  str_max += rng(0, 2);
  dex_max += rng(0, 2);
  per_max += rng(0, 2);
  int_max += rng(0, 4);
  if (one_in(3)) {
   boost_skill_level("mechanics", dice(2, 3));
   boost_skill_level("electronics", dice(2, 3));
   boost_skill_level("firstaid", dice(2, 3));
  }
 }
 if (fac->has_value(FACVAL_BOOKS)) {
  str_max -= rng(0, 2);
  per_max -= rng(0, 3);
  int_max += rng(0, 4);
  personality.aggression -= rng(1, 4);
  personality.bravery -= rng(0, 3);
  personality.collector += rng(0, 3);
 }
 if (fac->has_value(FACVAL_TRAINING)) {
  str_max += rng(0, 3);
  dex_max += rng(0, 3);
  per_max += rng(0, 2);
  int_max += rng(0, 2);
  for( auto &skill : Skill::skills ) {
   if (one_in(3))
       boost_skill_level( skill, rng( 2, 4 ) );
  }
 }
 if (fac->has_value(FACVAL_ROBOTS)) {
  int_max += rng(0, 3);
  personality.aggression -= rng(0, 3);
  personality.collector += rng(0, 3);
 }
 if (fac->has_value(FACVAL_TREACHERY)) {
  personality.aggression += rng(0, 3);
  personality.altruism -= rng(2, 5);
 }
 if (fac->has_value(FACVAL_STRAIGHTEDGE)) {
  personality.collector -= rng(0, 2);
  str_max += rng(0, 1);
  per_max += rng(0, 2);
  int_max += rng(0, 3);
 }
 if (fac->has_value(FACVAL_LAWFUL)) {
  personality.aggression -= rng(3, 7);
  personality.altruism += rng(1, 5);
  int_max += rng(0, 2);
 }
 if (fac->has_value(FACVAL_CRUELTY)) {
  personality.aggression += rng(3, 6);
  personality.bravery -= rng(1, 4);
  personality.altruism -= rng(2, 5);
 }
}

void npc::set_fac(std::string fac_name)
{
    my_fac = g->faction_by_ident(fac_name);
    fac_id = my_fac->id;
}

// item id from group "<class-name>_<what>" or from fallback group
// may still be a null item!
item random_item_from( npc_class type, const std::string &what, const std::string &fallback )
{
    auto result = item_group::item_from( npc_class_name_str( type ) + "_" + what );
    if( result.is_null() ) {
        result = item_group::item_from( fallback );
    }
    return result;
}

// item id from "<class-name>_<what>" or from "npc_<what>"
item random_item_from( npc_class type, const std::string &what )
{
    return random_item_from( type, what, "npc_" + what );
}

// item id from "<class-name>_<what>_<gender>" or from "npc_<what>_<gender>"
item get_clothing_item( npc_class type, const std::string &what, bool male )
{
    if( male ) {
        return random_item_from( type, what + "_male", "npc_" + what + "_male" );
    } else {
        return random_item_from( type, what + "_female", "npc_" + what + "_female" );
    }
}

std::vector<item> starting_clothes( npc_class type, bool male )
{
    std::vector<item> ret;

    item pants = get_clothing_item( type, "pants", male);
    item shirt = get_clothing_item( type, "shirt", male );
    item gloves = random_item_from( type, "gloves" );
    item coat = random_item_from( type, "coat" );
    item shoes = random_item_from( type, "shoes" );
    item mask = random_item_from( type, "masks" );
    // Why is the alternative group not named "npc_glasses" but "npc_eyes"?
    item glasses = random_item_from( type, "glasses", "npc_eyes" );
    item hat = random_item_from( type, "hat" );
    item extras = random_item_from( type, "extra" );

    // Fill in the standard things we wear
    ret.push_back( shoes );
    ret.push_back( pants );
    ret.push_back( shirt );
    ret.push_back( coat );
    ret.push_back( gloves );
    // Bad to wear a mask under a motorcycle helmet
    if( hat.typeId() != "helmet_motor" ) {
        ret.push_back( mask );
    }
    ret.push_back( glasses );
    ret.push_back( hat );
    ret.push_back( extras );

    // the player class and other code all over the place assume that the
    // worn vector contains *only* armor items. It will *crash* when there
    // is a non-armor item!
    // Also: the above might have added null-items that must be filtered out.
    for( auto it = ret.begin(); it != ret.end(); ) {
        if( !it->is_null() && it->is_armor() ) {
            if( one_in( 3 ) && it->has_flag( "VARSIZE" ) ) {
                it->item_tags.insert( "FIT" );
            }
            ++it;
        } else {
            it = ret.erase( it );
        }
    }
 return ret;
}

std::list<item> starting_inv(npc *me, npc_class type)
{
 int total_space = me->volume_capacity() - 2;
 std::list<item> ret;
 ret.push_back( item("lighter", 0, false) );
 itype_id tmp;
 item tmpitem;

// First, if we're wielding a gun, get some ammo for it
 if (me->weapon.is_gun()) {
  it_gun *gun = dynamic_cast<it_gun*>(me->weapon.type);
  tmp = default_ammo(gun->ammo);
  if (tmp == "" || tmp == "UPS"){
    add_msg( m_debug, "Unknown ammo type for spawned NPC: '%s'", tmp.c_str() );
  }else {
      item itammo( tmp, 0 );
      itammo = itammo.in_its_container();
      if( itammo.made_of( LIQUID ) ) {
          item container( "bottle_plastic", 0 );
          container.put_in( itammo );
          itammo = container;
      }
      if (total_space >= itammo.volume()) {
       ret.push_back(itammo);
       total_space -= ret.back().volume();
      }
      while ((type == NC_COWBOY || type == NC_BOUNTY_HUNTER || !one_in(3)) &&
             !one_in(2) && total_space >= itammo.volume()) {
       ret.push_back(itammo);
       total_space -= ret.back().volume();
      }
  }
 }

 int stopChance = 25;
 if (type == NC_ARSONIST)
  ret.push_back(item("molotov", 0));
 if (type == NC_EVAC_SHOPKEEP || type == NC_TRADER){
  total_space += 30;
  stopChance = 40;
 }

 while (total_space > 0 && !one_in(stopChance)) {
    tmpitem = random_item_from( type, "_misc" );
    if( tmpitem.is_null() ) {
        continue;
    }
    if( one_in( 3 ) && tmpitem.has_flag( "VARSIZE" ) ) {
        tmpitem.item_tags.insert( "FIT" );
    }
    if (total_space >= tmpitem.volume()) {
        ret.push_back(tmpitem);
        ret.back() = ret.back().in_its_container();
        total_space -= ret.back().volume();
    }
 }

 for (std::list<item>::iterator iter = ret.begin(); iter != ret.end(); ++iter) {
  if(item_group::group_contains_item("trader_avoid", iter->type->id)) {
   iter = ret.erase(iter);
   --iter;
  }
 }
 return ret;
}

void npc::spawn_at(int x, int y, int z)
{
    mapx = x;
    mapy = y;
    mapz = z;
    posx = rng(0, SEEX - 1);
    posy = rng(0, SEEY - 1);
    const point pos_om = overmapbuffer::sm_to_om_copy( mapx, mapy );
    overmap &om = overmap_buffer.get( pos_om.x, pos_om.y );
    om.npcs.push_back(this);
}

void npc::spawn_at_random_city(overmap *o)
{
    int x, y;
    if(o->cities.empty()) {
        x = rng(0, OMAPX * 2 - 1);
        y = rng(0, OMAPY * 2 - 1);
    } else {
        int city_index = rng(0, o->cities.size() - 1);
        int s = o->cities[city_index].s;
        x = o->cities[city_index].x + rng(-s, +s);
        y = o->cities[city_index].y + rng(-s, +s);
    }
    x += o->pos().x * OMAPX * 2;
    y += o->pos().y * OMAPY * 2;
    spawn_at(x, y, 0);
}

tripoint npc::global_sm_location() const
{
    tripoint t = global_square_location();
    overmapbuffer::ms_to_sm(t.x, t.y);
    return t;
}

tripoint npc::global_omt_location() const
{
    tripoint t = global_square_location();
    overmapbuffer::ms_to_omt(t.x, t.y);
    return t;
}

tripoint npc::global_square_location() const
{
    return tripoint( mapx * SEEX + posx, mapy * SEEY + posy, mapz );
}

void npc::place_on_map()
{
    // The global absolute position (in map squares) of the npc is *always*
    // "mapx * SEEX + posx" (analog for y).
    // The main map assumes that pos[xy] is in its own (local to the main map)
    // coordinate system. We have to change pos[xy] to match that assumption,
    // but also have to change map[xy] to keep the global position of the npc
    // unchanged.
    const int dmx = mapx - g->get_abs_levx();
    const int dmy = mapy - g->get_abs_levy();
    mapx -= dmx; // == g->get_abs_levx()
    mapy -= dmy;
    posx += dmx * SEEX; // value of "mapx * SEEX + posx" is unchanged
    posy += dmy * SEEY;

    //places the npc at the nearest empty spot near (posx, posy). Searches in a spiral pattern for a suitable location.
    int x = 0, y = 0, dx = 0, dy = -1;
    int temp;
    while(!g->is_empty(posx + x, posy + y))
    {
        if ((x == y) || ((x < 0) && (x == -y)) || ((x > 0) && (x == 1-y)))
        {//change direction
            temp = dx;
            dx = -dy;
            dy = temp;
        }
        x += dx;
        y += dy;
    }//end search, posx + x , posy + y contains a free spot.
    //place the npc at the free spot.
    posx += x;
    posy += y;
}

Skill* npc::best_skill()
{
 std::vector<Skill*> best_skills;
 int highest = 0;
 for( auto &skill : Skill::skills ) {
  //Should check to see if the skill has a "combat_skill" tag
     if( ( skill )->is_combat_skill() ) {
         if( skillLevel( skill ) > highest ) {
             highest = skillLevel( skill );
    best_skills.clear();
    best_skills.push_back( skill );
         } else if( skillLevel( skill ) == highest ) {
             best_skills.push_back( skill );
   }
  }
 }
 int index = rng(0, best_skills.size() - 1);
 return best_skills[index];
}

void npc::starting_weapon(npc_class type)
{
    Skill* best = best_skill();
    item sel_weapon;
    if (best->ident() == "bashing"){
        sel_weapon = random_item_from( type, "bashing" );
    } else if (best->ident() == "cutting"){
        sel_weapon = random_item_from( type, "cutting" );
    } else if (best->ident() == "stabbing"){
        sel_weapon = random_item_from( type, "stabbing" );
    } else if (best->ident() == "throw"){
        sel_weapon = random_item_from( type, "throw" );
    } else if (best->ident() == "archery"){
        sel_weapon = random_item_from( type, "archery" );
    }else if (best->ident() == "pistol"){
        sel_weapon = random_item_from( type, "pistols", "pistols" );
    }else if (best->ident() == "shotgun"){
        sel_weapon = random_item_from( type, "shotgun", "shotguns" );
    }else if (best->ident() == "smg"){
        sel_weapon = random_item_from( type, "smg", "smg" );
    }else if (best->ident() == "rifle"){
        sel_weapon = random_item_from( type, "rifle", "rifles" );
    }else if (best->ident() == "launcher"){
        sel_weapon = random_item_from( type, "launcher" );
    }

    if (sel_weapon.is_null()){
        sel_weapon = random_item_from( type, "weapon_random" );
    }
    weapon = sel_weapon;

    if (weapon.is_gun())
    {
        it_gun* gun = dynamic_cast<it_gun*>(weapon.type);
        const std::string tmp = default_ammo( gun->ammo );
        if( tmp != "" ) {
            weapon.charges = gun->clip;
            weapon.curammo = dynamic_cast<it_ammo*>( item::find_type( tmp ) );
        }
    }
}

bool npc::wear_if_wanted(item it)
{
    if (!it.is_armor()) {
        return false;
    }

    int max_encumb[num_bp] = {2, 3, 3, 4, 3, 3, 3, 2};
    bool encumb_ok = true;
    for (int i = 0; i < num_bp && encumb_ok; i++) {
        const auto bp = static_cast<body_part>( i );
        if (it.covers(bp) && encumb(bp) + it.get_encumber() > max_encumb[i]) {
            encumb_ok = false;
        }
    }
    if (encumb_ok) {
        worn.push_back(it);
        return true;
    }
    // Otherwise, maybe we should take off one or more items and replace them
    std::vector<int> removal;
    for (size_t i = 0; i < worn.size(); i++) {
        for (int j = 0; j < num_bp; j++) {
            const auto bp = static_cast<body_part>( j );
            if (it.covers(bp) && worn[i].covers(bp)) {
                removal.push_back(i);
                j = num_bp;
            }
        }
    }
    for (auto &i : removal) {
        if (true) {
            inv.push_back(worn[i]);
            worn.push_back(it);
            return true;
        }
    }
    return false;
}
//to placate clang++
bool npc::wield(item* it, bool)
{
    return this->wield(it);
}

bool npc::wield(item* it)
{
    if ( !weapon.is_null() ) {
        if ( volume_carried() + weapon.volume() <= volume_capacity() ) {
            i_add( remove_weapon() );
            moves -= 15;
        } else { // No room for weapon, so we drop it
            g->m.add_item_or_charges( posx, posy, remove_weapon() );
        }
    }
    moves -= 15;
    weapon = inv.remove_item(it);
    if ( g->u_see( posx, posy ) ) {
        add_msg( m_info, _( "%1$s wields a %2$s." ), name.c_str(), weapon.tname().c_str() );
    }
    return true;
}

void npc::perform_mission()
{
 switch (mission) {
 case NPC_MISSION_RESCUE_U:
  if (int(calendar::turn) % 24 == 0) {
   if (mapx > g->get_abs_levx())
    mapx--;
   else if (mapx < g->get_abs_levx())
    mapx++;
   if (mapy > g->get_abs_levy())
    mapy--;
   else if (mapy < g->get_abs_levy())
    mapy++;
   attitude = NPCATT_DEFEND;
  }
  break;
 case NPC_MISSION_SHOPKEEP:
  break; // Just stay where we are
 default: // Random Walk
  if (int(calendar::turn) % 24 == 0) {
   mapx += rng(-1, 1);
   mapy += rng(-1, 1);
  }
 }
}

void npc::form_opinion(player *u)
{
// FEAR
 if (u->weapon.is_gun()) {
  if (weapon.is_gun())
   op_of_u.fear += 2;
  else
   op_of_u.fear += 6;
 } else if (u->weapon.type->melee_dam >= 12 || u->weapon.type->melee_cut >= 12)
  op_of_u.fear += 2;
 else if (u->unarmed_attack()) // Unarmed
  op_of_u.fear -= 3;

 if (u->str_max >= 16)
  op_of_u.fear += 2;
 else if (u->str_max >= 12)
  op_of_u.fear += 1;
 else if (u->str_max <= 5)
  op_of_u.fear -= 1;
 else if (u->str_max <= 3)
  op_of_u.fear -= 3;

 for (int i = 0; i < num_hp_parts; i++) {
  if (u->hp_cur[i] <= u->hp_max[i] / 2)
   op_of_u.fear--;
  if (hp_cur[i] <= hp_max[i] / 2)
   op_of_u.fear++;
 }

 if (has_trait("SAPIOVORE")) {
    op_of_u.fear += 10; // Sapiovores = Scary
 }
 if (u->has_trait("PRETTY"))
  op_of_u.fear += 1;
 else if (u->has_trait("BEAUTIFUL"))
  op_of_u.fear += 2;
 else if (u->has_trait("BEAUTIFUL2"))
  op_of_u.fear += 3;
 else if (u->has_trait("BEAUTIFUL3"))
  op_of_u.fear += 4;
 else if (u->has_trait("UGLY"))
  op_of_u.fear -= 1;
 else if (u->has_trait("DEFORMED"))
  op_of_u.fear += 3;
 else if (u->has_trait("DEFORMED2"))
  op_of_u.fear += 6;
 else if (u->has_trait("DEFORMED3"))
  op_of_u.fear += 9;
 if (u->has_trait("TERRIFYING"))
  op_of_u.fear += 6;

 if (u->stim > 20)
  op_of_u.fear++;

 if (u->has_effect("drunk"))
  op_of_u.fear -= 2;

// TRUST
 if (op_of_u.fear > 0)
  op_of_u.trust -= 3;
 else
  op_of_u.trust += 1;

 if (u->weapon.is_gun())
  op_of_u.trust -= 2;
 else if (u->unarmed_attack())
  op_of_u.trust += 2;

 if (u->has_effect("high"))
  op_of_u.trust -= 1;
 if (u->has_effect("drunk"))
  op_of_u.trust -= 2;
 if (u->stim > 20 || u->stim < -20)
  op_of_u.trust -= 1;
 if (u->pkill > 30)
  op_of_u.trust -= 1;

 if (u->has_trait("PRETTY"))
  op_of_u.trust += 1;
 else if (u->has_trait("BEAUTIFUL"))
  op_of_u.trust += 3;
 else if (u->has_trait("BEAUTIFUL2"))
  op_of_u.trust += 5;
 else if (u->has_trait("BEAUTIFUL3"))
  op_of_u.trust += 7;
 else if (u->has_trait("UGLY"))
  op_of_u.trust -= 1;
 else if (u->has_trait("DEFORMED"))
  op_of_u.trust -= 3;
 else if (u->has_trait("DEFORMED2"))
  op_of_u.trust -= 6;
 else if (u->has_trait("DEFORMED3"))
  op_of_u.trust -= 9;

    // VALUE
    op_of_u.value = 0;
    for (int i = 0; i < num_hp_parts; i++) {
        if (hp_cur[i] < hp_max[i] * .8) {
            op_of_u.value++;
        }
    }
    decide_needs();
    for (auto &i : needs) {
        if (i == need_food || i == need_drink) {
            op_of_u.value += 2;
        }
    }

 if (op_of_u.fear < personality.bravery + 10 &&
     op_of_u.fear - personality.aggression > -10 && op_of_u.trust > -8)
  attitude = NPCATT_TALK;
 else if (op_of_u.fear - 2 * personality.aggression - personality.bravery < -30)
  attitude = NPCATT_KILL;
 else if (my_fac != NULL && my_fac->likes_u < -10)
    attitude = NPCATT_KILL;
 else
  attitude = NPCATT_FLEE;
}

talk_topic npc::pick_talk_topic(player *u)
{
 //form_opinion(u);
 (void)u;
 if (personality.aggression > 0) {
  if (op_of_u.fear * 2 < personality.bravery && personality.altruism < 0)
   return TALK_MUG;
  if (personality.aggression + personality.bravery - op_of_u.fear > 0)
   return TALK_STRANGER_AGGRESSIVE;
 }
 if (op_of_u.fear * 2 > personality.altruism + personality.bravery)
  return TALK_STRANGER_SCARED;
 if (op_of_u.fear * 2 > personality.bravery + op_of_u.trust)
  return TALK_STRANGER_WARY;
 if (op_of_u.trust - op_of_u.fear +
     (personality.bravery + personality.altruism) / 2 > 0)
  return TALK_STRANGER_FRIENDLY;

 return TALK_STRANGER_NEUTRAL;
}

int npc::player_danger(player *u) const
{
 int ret = 0;
 if (u->weapon.is_gun()) {
  if (weapon.is_gun())
   ret += 4;
  else
   ret += 8;
 } else if (u->weapon.type->melee_dam >= 12 || u->weapon.type->melee_cut >= 12)
  ret++;
 else if (u->weapon.type->id == "null") // Unarmed
  ret -= 3;

 if (u->str_cur > 20) // Superhuman strength!
  ret += 4;
 if (u->str_max >= 16)
  ret += 2;
 else if (u->str_max >= 12)
  ret += 1;
 else if (u->str_max <= 5)
  ret -= 2;
 else if (u->str_max <= 3)
  ret -= 4;

 for (int i = 0; i < num_hp_parts; i++) {
  if (u->hp_cur[i] <= u->hp_max[i] / 2)
   ret--;
  if (hp_cur[i] <= hp_max[i] / 2)
   ret++;
 }

 if (u->has_trait("TERRIFYING"))
  ret += 2;

 if (u->stim > 20)
  ret++;

 if (u->has_effect("drunk"))
  ret -= 2;

 return ret;
}

int npc::vehicle_danger(int radius) const
{
    VehicleList vehicles = g->m.get_vehicles(posx - radius, posy - radius, posx + radius, posy + radius);

 int danger = 0;

 // TODO: check for most dangerous vehicle?
 for(size_t i = 0; i < vehicles.size(); ++i)
  if (vehicles[i].v->velocity > 0)
  {
   float facing = vehicles[i].v->face.dir();

   int ax = vehicles[i].v->global_x();
   int ay = vehicles[i].v->global_y();
   int bx = ax + cos (facing * M_PI / 180.0) * radius;
   int by = ay + sin (facing * M_PI / 180.0) * radius;

   // fake size
   /* This will almost certainly give the wrong size/location on customized
    * vehicles. This should just count frames instead. Or actually find the
    * size. */
   vehicle_part last_part = vehicles[i].v->parts.back();
   int size = std::max(last_part.mount_dx, last_part.mount_dy);

   float normal = sqrt((float)((bx - ax) * (bx - ax) + (by - ay) * (by - ay)));
   int closest = abs((posx - ax) * (by - ay) - (posy - ay) * (bx - ax)) / normal;

   if (size > closest)
    danger = i;
  }

 return danger;
}

bool npc::turned_hostile() const
{
 return (op_of_u.anger >= hostile_anger_level());
}

int npc::hostile_anger_level() const
{
 return (20 + op_of_u.fear - personality.aggression);
}

void npc::make_angry()
{
    // Make associated faction, if any, angry at the player too.
    if( my_fac != NULL ) {
        my_fac->likes_u -= 50;
        my_fac->respects_u -= 50;
    }
    if( is_enemy() ) {
        return; // We're already angry!
    }
    if( op_of_u.fear > 10 + personality.aggression + personality.bravery ) {
        attitude = NPCATT_FLEE; // We don't want to take u on!
    } else {
        attitude = NPCATT_KILL; // Yeah, we think we could take you!
    }
}

// STUB
bool npc::wants_to_travel_with(player *p) const
{
    (void)p; // TODO: implement
    return true;
}

int npc::assigned_missions_value()
{
    int ret = 0;
    for (auto &i : chatbin.missions_assigned) {
        ret += g->find_mission(i)->value;
    }
    return ret;
}

std::vector<Skill*> npc::skills_offered_to(player *p)
{
 std::vector<Skill*> ret;
 if (p == NULL)
  return ret;
 for( auto &skill : Skill::skills ) {
     if( p->skillLevel( skill ) < skillLevel( skill ) ) {
         ret.push_back( skill );
  }
 }
 return ret;
}

std::vector<itype_id> npc::styles_offered_to(player *p)
{
    std::vector<itype_id> ret;
    if (p == NULL) {
        return ret;
    }
    for (auto &i : ma_styles) {
        bool found = false;
        for (auto &j : p->ma_styles) {
            if (j == i) {
                found = true;
                break;
            }
        }
        if (!found) {
            ret.push_back( i );
        }
    }
    return ret;
}


int npc::minutes_to_u() const
{
    // TODO: what about different z-levels?
    int ret = square_dist( mapx, mapy, g->get_abs_levx(), g->get_abs_levy() );
    // TODO: someone should explain this calculation. Is 24 supposed to be SEEX*2?
 ret *= 24;
 ret /= 10;
 while (ret % 5 != 0) // Round up to nearest five-minute interval
  ret++;
 return ret;
}

bool npc::fac_has_value(faction_value value)
{
 if (my_fac == NULL)
  return false;
 return my_fac->has_value(value);
}

bool npc::fac_has_job(faction_job job)
{
 if (my_fac == NULL)
  return false;
 return my_fac->has_job(job);
}


void npc::decide_needs()
{
    int needrank[num_needs];
    for( auto &elem : needrank )
        elem = 20;
    if (weapon.is_gun()) {
        it_gun* gun = dynamic_cast<it_gun*>(weapon.type);
        needrank[need_ammo] = 5 * has_ammo(gun->ammo).size();
    }
    if (weapon.type->id == "null" && skillLevel("unarmed") < 4) {
        needrank[need_weapon] = 1;
    } else {
        needrank[need_weapon] = weapon.type->melee_dam + weapon.type->melee_cut +
                                weapon.type->m_to_hit;
    }
    if (!weapon.is_gun()) {
        needrank[need_gun] = skillLevel("unarmed") + skillLevel("melee") +
                            skillLevel("bashing") + skillLevel("cutting") -
                            skillLevel("gun") * 2 + 5;
    }
    needrank[need_food] = 15 - hunger;
    needrank[need_drink] = 15 - thirst;
    invslice slice = inv.slice();
    for (auto &i : slice) {
        it_comest* food = NULL;
        if (i->front().is_food()) {
            food = dynamic_cast<it_comest*>(i->front().type);
        } else if (i->front().is_food_container()) {
            food = dynamic_cast<it_comest*>(i->front().contents[0].type);
        }
        if (food != NULL) {
            needrank[need_food] += food->nutr / 4;
            needrank[need_drink] += food->quench / 4;
        }
    }
    needs.clear();
    size_t j;
    bool serious = false;
    for (int i = 1; i < num_needs; i++) {
        if (needrank[i] < 10) {
            serious = true;
        }
    }
    if (!serious) {
        needs.push_back(need_none);
        needrank[0] = 10;
    }
    for (int i = 1; i < num_needs; i++) {
        if (needrank[i] < 20) {
            for (j = 0; j < needs.size(); j++) {
                if (needrank[i] < needrank[needs[j]]) {
                    needs.insert(needs.begin() + j, npc_need(i));
                    j = needs.size() + 1;
                }
            }
            if (j == needs.size()) {
                needs.push_back(npc_need(i));
            }
        }
    }
}

void npc::say(std::string line, ...) const
{
 va_list ap;
 va_start(ap, line);
 line = vstring_format(line, ap);
 va_end(ap);
 parse_tags(line, &(g->u), this);
 if (g->u_see(posx, posy)) {
  add_msg(_("%1$s says: \"%2$s\""), name.c_str(), line.c_str());
  g->sound(posx, posy, 16, "");
 } else {
  std::string sound = string_format(_("%1$s saying \"%2$s\""), name.c_str(), line.c_str());
  g->sound(posx, posy, 16, sound);
 }
}

void npc::init_selling(std::vector<item*> &items, std::vector<int> &prices)
{
    bool found_lighter = false;
    invslice slice = inv.slice();
    for (auto &i : slice) {
        if (i->front().type->id == "lighter" && !found_lighter) {
            found_lighter = true;
        } else {
            int val = value(i->front()) - (i->front().price() / 50);
            if (val <= NPC_LOW_VALUE || mission == NPC_MISSION_SHOPKEEP) {
                items.push_back(&i->front());
                prices.push_back(i->front().price());
            }
        }
    }
}

void npc::init_buying(inventory& you, std::vector<item*> &items, std::vector<int> &prices)
{
    invslice slice = you.slice();
    for (auto &i : slice) {
        int val = value(i->front());
        if (val >= NPC_HI_VALUE) {
            items.push_back(&i->front());
            prices.push_back(i->front().price());
        }
    }
}

void npc::shop_restock(){
    items_location from = "NULL";
    int total_space = volume_capacity() - 2;
    std::list<item> ret;
    //list all merchant types here along with the item group they pull from and how much extra space they should have
    //guards and other fixed npcs may need a small supply of food daily...
    switch (this->myclass) {
        case NC_EVAC_SHOPKEEP:
            from = "NC_EVAC_SHOPKEEP_misc";
            total_space += rng(30,40);
            this-> cash = 100000 * rng(1, 10)+ rng(1, 100000);
        case NC_ARSONIST:
            from = "NC_ARSONIST_misc";
            this-> cash = 25000 * rng(1, 10)+ rng(1, 1000);
            ret.push_back(item("molotov", 0));
        case NC_HUNTER:
            from = "NC_HUNTER_misc";
            this-> cash = 15000 * rng(1, 10)+ rng(1, 1000);
        default:
            //Suppress warnings
            break;
    }
    if (from == "NULL")
        return;
    while (total_space > 0 && !one_in(50)) {
        item tmpit = item_group::item_from( from, 0 );
        if( !tmpit.is_null() && total_space >= tmpit.volume()) {
            ret.push_back(tmpit);
            total_space -= tmpit.volume();
        }
    }
    this->inv.clear();
    this->inv.add_stack(ret);
    this->update_worst_item_value();
}


int npc::minimum_item_value()
{
 int ret = 20;
 ret -= personality.collector;
 return ret;
}

void npc::update_worst_item_value()
{
    worst_item_value = 99999;
    int inv_val = inv.worst_item_value(this);
    if (inv_val < worst_item_value)
    {
        worst_item_value = inv_val;
    }
}

int npc::value(const item &it)
{
 int ret = it.price() / 50;
 Skill* best = best_skill();
 if (best->ident() != "unarmed") {
  int weapon_val = it.weapon_value(this) - weapon.weapon_value(this);
  if (weapon_val > 0)
   ret += weapon_val;
 }

 if (it.is_food()) {
  it_comest* comest = dynamic_cast<it_comest*>(it.type);
  if (comest->nutr > 0 || comest->quench > 0)
   ret++;
  if (hunger > 40)
   ret += (comest->nutr + hunger - 40) / 6;
  if (thirst > 40)
   ret += (comest->quench + thirst - 40) / 4;
 }

 if (it.is_ammo()) {
  it_ammo* ammo = dynamic_cast<it_ammo*>(it.type);
  it_gun* gun;
  if (weapon.is_gun()) {
   gun = dynamic_cast<it_gun*>(weapon.type);
   if (ammo->type == gun->ammo)
    ret += 14;
  }
  if (has_gun_for_ammo(ammo->type)) {
   // TODO consider making this cumulative (once was)
   ret += 14;
  }
 }

 if (it.is_book()) {
        auto &book = *it.type->book;
        if( book.intel <= int_cur ) {
            ret += book.fun;
            if( book.skill != nullptr && skillLevel( book.skill ) < book.level &&
                skillLevel( book.skill ) >= book.req ) {
                ret += book.level * 3;
            }
        }
 }

// TODO: Sometimes we want more than one tool?  Also we don't want EVERY tool.
 if (it.is_tool() && !has_amount(itype_id(it.type->id), 1)) {
  ret += 8;
 }

// TODO: Artifact hunting from relevant factions
// ALSO TODO: Bionics hunting from relevant factions
 if (fac_has_job(FACJOB_DRUGS) && it.is_food() &&
     (dynamic_cast<it_comest*>(it.type))->addict >= 5)
  ret += 10;
 if (fac_has_job(FACJOB_DOCTORS) && it.type->id >= "bandages" &&
     it.type->id <= "prozac")
  ret += 10;
 if (fac_has_value(FACVAL_BOOKS) && it.is_book())
  ret += 14;
 if (fac_has_job(FACJOB_SCAVENGE)) { // Computed last for _reasons_.
  ret += 6;
  ret *= 1.3;
 }
 return ret;
}

bool npc::has_healing_item()
{
    return inv.has_amount("bandages", 1) || inv.has_amount("1st_aid", 1);
}

bool npc::has_painkiller()
{
    return inv.has_enough_painkiller(pain);
}

bool npc::took_painkiller() const
{
 return (has_effect("pkill1") || has_effect("pkill2") ||
         has_effect("pkill3") || has_effect("pkill_l"));
}

bool npc::is_friend() const
{
 if (attitude == NPCATT_FOLLOW || attitude == NPCATT_DEFEND ||
     attitude == NPCATT_LEAD)
  return true;
 return false;
}

bool npc::is_following() const
{
 switch (attitude) {
 case NPCATT_FOLLOW:
 case NPCATT_FOLLOW_RUN:
 case NPCATT_DEFEND:
 case NPCATT_SLAVE:
 case NPCATT_WAIT:
  return true;
 default:
  return false;
 }
}

bool npc::is_leader() const
{
 return (attitude == NPCATT_LEAD);
}

bool npc::is_enemy() const
{
 if (attitude == NPCATT_KILL || attitude == NPCATT_MUG ||
     attitude == NPCATT_FLEE)
  return true;
 return  false;
}

bool npc::is_defending() const
{
 return (attitude == NPCATT_DEFEND);
}

Creature::Attitude npc::attitude_to( const Creature &other ) const
{
    if( other.is_npc() ) {
        // No npc vs npc action, so simply ignore other npcs
        return A_NEUTRAL;
    } else if( other.is_player() ) {
        // For now, make it symmetric.
        return other.attitude_to( *this );
    }
    // Fallback to use the same logic as player, even through it's wrong:
    // Hostile (towards the player) npcs should see friendly monsters as hostile, too.
    return player::attitude_to( other );
}

int npc::danger_assessment()
{
    int ret = 0;
    int sightdist = g->light_level(), junk;
    for (size_t i = 0; i < g->num_zombies(); i++) {
        if (g->m.sees(posx, posy, g->zombie(i).posx(), g->zombie(i).posy(), sightdist, junk)) {
            ret += g->zombie(i).type->difficulty;
        }
    }
    ret /= 10;
    if (ret <= 2) {
        ret = -10 + 5 * ret; // Low danger if no monsters around
    }
    // Mod for the player
    if (is_enemy()) {
        if (rl_dist(posx, posy, g->u.posx, g->u.posy) < 10) {
            if (g->u.weapon.is_gun()) {
                ret += 10;
            } else {
                ret += 10 - rl_dist(posx, posy, g->u.posx, g->u.posy);
            }
        }
    } else if (is_friend()) {
        if (rl_dist(posx, posy, g->u.posx, g->u.posy) < 8) {
            if (g->u.weapon.is_gun()) {
                ret -= 8;
            } else {
                ret -= 8 - rl_dist(posx, posy, g->u.posx, g->u.posy);
            }
        }
    }
    for (int i = 0; i < num_hp_parts; i++) {
        if (i == hp_head || i == hp_torso) {
            if (hp_cur[i] < hp_max[i] / 4) {
                ret += 5;
            } else if (hp_cur[i] < hp_max[i] / 2) {
                ret += 3;
            } else if (hp_cur[i] < hp_max[i] * .9) {
                ret += 1;
            }
        } else {
            if (hp_cur[i] < hp_max[i] / 4) {
                ret += 2;
            } else if (hp_cur[i] < hp_max[i] / 2) {
                ret += 1;
            }
        }
    }
    return ret;
}

int npc::average_damage_dealt()
{
 int ret = base_damage();
 ret += weapon.damage_cut() + weapon.damage_bash() / 2;
 ret *= (base_to_hit() + weapon.type->m_to_hit);
 ret /= 15;
 return ret;
}

bool npc::bravery_check(int diff)
{
 return (dice(10 + personality.bravery, 6) >= dice(diff, 4));
}

bool npc::emergency(int danger)
{
 return (danger > (personality.bravery * 3 * hp_percentage()) / 100);
}

//Check if this npc is currently in the list of active npcs.
//Active npcs are the npcs near the player that are actively simulated.
bool npc::is_active() const
{
    return std::find(g->active_npc.begin(), g->active_npc.end(), this) != g->active_npc.end();
}

void npc::told_to_help()
{
 if (!is_following() && personality.altruism < 0) {
  say(_("Screw you!"));
  return;
 }
 if (is_following()) {
  if (personality.altruism + 4 * op_of_u.value + personality.bravery >
      danger_assessment()) {
   say(_("I've got your back!"));
   attitude = NPCATT_DEFEND;
  }
  return;
 }
 if (int((personality.altruism + personality.bravery) / 4) >
     danger_assessment()) {
  say(_("Alright, I got you covered!"));
  attitude = NPCATT_DEFEND;
 }
}

void npc::told_to_wait()
{
 if (!is_following()) {
  debugmsg("%s told to wait, but isn't following", name.c_str());
  return;
 }
 if (5 + op_of_u.value + op_of_u.trust + personality.bravery * 2 >
     danger_assessment()) {
  say(_("Alright, I'll wait here."));
  if (one_in(3))
   op_of_u.trust--;
  attitude = NPCATT_WAIT;
 } else {
  if (one_in(2))
   op_of_u.trust--;
  say(_("No way, man!"));
 }
}

void npc::told_to_leave()
{
 if (!is_following()) {
  debugmsg("%s told to leave, but isn't following", name.c_str());
  return;
 }
 if (danger_assessment() - personality.bravery > op_of_u.value) {
  say(_("No way, I need you!"));
  op_of_u.trust -= 2;
 } else {
  say(_("Alright, see you later."));
  op_of_u.trust -= 2;
  op_of_u.value -= 1;
 }
}

int npc::follow_distance() const
{
 return 4; // TODO: Modify based on bravery, weapon wielded, etc.
}

int npc::speed_estimate(int speed) const
{
 if (per_cur == 0)
  return rng(0, speed * 2);
// Up to 80% deviation if per_cur is 1;
// Up to 10% deviation if per_cur is 8;
// Up to 4% deviation if per_cur is 20;
 int deviation = speed / (double)(per_cur * 1.25);
 int low = speed - deviation, high = speed + deviation;
 return rng(low, high);
}

nc_color npc::basic_symbol_color() const
{
    if( attitude == NPCATT_KILL ) {
        return c_red;
    } else if( is_friend() ) {
        return c_green;
    } else if( is_following() ) {
        return c_ltgreen;
    }
    return c_pink;
}

int npc::print_info(WINDOW* w, int line, int vLines, int column) const
{
    const int last_line = line + vLines;
    // First line of w is the border; the next 4 are terrain info, and after that
    // is a blank line. w is 13 characters tall, and we can't use the last one
    // because it's a border as well; so we have lines 6 through 11.
    // w is also 48 characters wide - 2 characters for border = 46 characters for us
    mvwprintz(w, line++, column, c_white, _("NPC: %s"), name.c_str());
    if (weapon.type->id == "null") {
        mvwprintz(w, line++, column, c_red, _("Wielding %s"), weapon.tname().c_str());
    } else {
        mvwprintz(w, line++, column, c_red, _("Wielding a %s"), weapon.tname().c_str());
    }
    std::string wearing;
    std::stringstream wstr;
    wstr << _("Wearing: ");
    bool first = true;
    for (auto &i : worn) {
        if (!first) {
            wstr << _(", ");
        } else {
            first = false;
        }
        wstr << i.tname();
    }
    wearing = wstr.str();
    size_t split;
    do {
        split = (wearing.length() <= 46) ? std::string::npos :
                                     wearing.find_last_of(' ', 46);
        if (split == std::string::npos) {
            mvwprintz(w, line, column, c_blue, wearing.c_str());
        } else {
            mvwprintz(w, line, column, c_blue, wearing.substr(0, split).c_str());
        }
        wearing = wearing.substr(split + 1);
        line++;
    } while (split != std::string::npos && line <= last_line);

    return line;
}

std::string npc::short_description() const
{
    std::stringstream ret;
    ret << _("Wielding: ") << weapon.tname() << ";   " << _("Wearing: ");
    bool first = true;
    for (auto &i : worn) {
        if (!first) {
            ret << _(", ");
        } else {
            first = false;
        }
        ret << i.tname();
    }

    return ret.str();
}

std::string npc::opinion_text() const
{
 std::stringstream ret;
 if (op_of_u.trust <= -10)
  ret << _("Completely untrusting");
 else if (op_of_u.trust <= -6)
  ret << _("Very untrusting");
 else if (op_of_u.trust <= -3)
  ret << _("Untrusting");
 else if (op_of_u.trust <= 2)
  ret << _("Uneasy");
 else if (op_of_u.trust <= 5)
  ret << _("Trusting");
 else if (op_of_u.trust < 10)
  ret << _("Very trusting");
 else
  ret << _("Completely trusting");

 ret << " (" << _("Trust: ") << op_of_u.trust << "); ";

 if (op_of_u.fear <= -10)
  ret << _("Thinks you're laughably harmless");
 else if (op_of_u.fear <= -6)
  ret << _("Thinks you're harmless");
 else if (op_of_u.fear <= -3)
  ret << _("Unafraid");
 else if (op_of_u.fear <= 2)
  ret << _("Wary");
 else if (op_of_u.fear <= 5)
  ret << _("Afraid");
 else if (op_of_u.fear < 10)
  ret << _("Very afraid");
 else
  ret << _("Terrified");

 ret << " (" << _("Fear: ") << op_of_u.fear << "); ";

 if (op_of_u.value <= -10)
  ret << _("Considers you a major liability");
 else if (op_of_u.value <= -6)
  ret << _("Considers you a burden");
 else if (op_of_u.value <= -3)
  ret << _("Considers you an annoyance");
 else if (op_of_u.value <= 2)
  ret << _("Doesn't care about you");
 else if (op_of_u.value <= 5)
  ret << _("Values your presence");
 else if (op_of_u.value < 10)
  ret << _("Treasures you");
 else
  ret << _("Best Friends Forever!");

 ret << " (" << _("Value: ") << op_of_u.value << "); ";

 if (op_of_u.anger <= -10)
  ret << _("You can do no wrong!");
 else if (op_of_u.anger <= -6)
  ret << _("You're good people");
 else if (op_of_u.anger <= -3)
  ret << _("Thinks well of you");
 else if (op_of_u.anger <= 2)
  ret << _("Ambivalent");
 else if (op_of_u.anger <= 5)
  ret << _("Pissed off");
 else if (op_of_u.anger < 10)
  ret << _("Angry");
 else
  ret << _("About to kill you");

 ret << " (" << _("Anger: ") << op_of_u.anger << ")";

 return ret.str();
}

void npc::shift(int sx, int sy)
{
    posx -= sx * SEEX;
    posy -= sy * SEEY;
    const point pos_om_old = overmapbuffer::sm_to_om_copy( mapx, mapy );
    mapx += sx;
    mapy += sy;
    const point pos_om_new = overmapbuffer::sm_to_om_copy( mapx, mapy );
    if( pos_om_old != pos_om_new ) {
        overmap &om_old = overmap_buffer.get( pos_om_old.x, pos_om_old.y );
        overmap &om_new = overmap_buffer.get( pos_om_new.x, pos_om_new.y );
        auto a = std::find(om_old.npcs.begin(), om_old.npcs.end(), this);
        if (a != om_old.npcs.end()) {
            om_old.npcs.erase( a );
            om_new.npcs.push_back( this );
        } else {
            // Don't move the npc pointer around to avoid having two overmaps
            // with the same npc pointer
            debugmsg( "could not find npc %s on its old overmap", name.c_str() );
        }
    }
    itx -= sx * SEEX;
    ity -= sy * SEEY;
    plx -= sx * SEEX;
    ply -= sy * SEEY;
    path.clear();
}

bool npc::is_dead() const
{
    return dead || is_dead_state();
}

void npc::die(Creature* nkiller) {
    if( dead ) {
        // We are already dead, don't die again, note that npc::dead is
        // *only* set to true in this function!
        return;
    }
    dead = true;
    if( nkiller != NULL && !nkiller->is_fake() ) {
        killer = nkiller;
    }
    if (in_vehicle) {
        g->m.unboard_vehicle(posx, posy);
    }

    if (g->u_see(posx, posy)) {
        add_msg(_("%s dies!"), name.c_str());
    }
    if( killer == &g->u ){
        if (is_friend()) {
            if (g->u.has_trait("SAPIOVORE")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed a friendly ape, %s.  Better eaten than eating."),
                                      pgettext("memorial_female", "Killed a friendly ape, %s.  Better eaten than eating."),
                                      name.c_str());
            }
            else if(!g->u.has_trait("PSYCHOPATH")) {
                // Very long duration, about 7d, decay starts after 10h.
                g->u.add_memorial_log(pgettext("memorial_male", "Killed a friend, %s."),
                                      pgettext("memorial_female", "Killed a friend, %s."),
                                      name.c_str());
                g->u.add_morale(MORALE_KILLED_FRIEND, -500, 0, 10000, 600);
            } else if(!g->u.has_trait("CANNIBAL") && g->u.has_trait("PSYCHOPATH")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed someone foolish enough to call you friend, %s. Didn't care."),
                                      pgettext("memorial_female", "Killed someone foolish enough to call you friend, %s. Didn't care."),
                                      name.c_str());
            } else {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed a delicious-looking friend, %s, in cold blood."),
                                      pgettext("memorial_female", "Killed a delicious-looking friend, %s, in cold blood."),
                                      name.c_str());
            }
        } else if (!is_enemy() || this->hit_by_player) {
            if (g->u.has_trait("SAPIOVORE")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Caught and killed an ape.  Prey doesn't have a name."),
                                      pgettext("memorial_female", "Caught and killed an ape.  Prey doesn't have a name."));
            }
            else if(!g->u.has_trait("CANNIBAL") && !g->u.has_trait("PSYCHOPATH")) {
                // Very long duration, about 3.5d, decay starts after 5h.
                g->u.add_memorial_log(pgettext("memorial_male","Killed an innocent person, %s, in cold blood and felt terrible afterwards."),
                                      pgettext("memorial_female","Killed an innocent person, %s, in cold blood and felt terrible afterwards."),
                                      name.c_str());
                g->u.add_morale(MORALE_KILLED_INNOCENT, -100, 0, 5000, 300);
            } else if(!g->u.has_trait("CANNIBAL") && g->u.has_trait("PSYCHOPATH")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed an innocent, %s, in cold blood. They were weak."),
                                      pgettext("memorial_female", "Killed an innocent, %s, in cold blood. They were weak."),
                                      name.c_str());
            } else if(g->u.has_trait("CANNIBAL") && !g->u.has_trait("PSYCHOPATH")) {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed an innocent, %s."),
                                      pgettext("memorial_female", "Killed an innocent, %s."),
                                      name.c_str());
                g->u.add_morale(MORALE_KILLED_INNOCENT, -5, 0, 500, 300);
            } else {
                g->u.add_memorial_log(pgettext("memorial_male", "Killed a delicious-looking innocent, %s, in cold blood."),
                                      pgettext("memorial_female", "Killed a delicious-looking innocent, %s, in cold blood."),
                                      name.c_str());
            }
        }
    }

    place_corpse();

    mission_type *type;
    for (auto &i : g->active_missions) {
        type = i.type;
        //complete the mission if you needed killing
        if (type->goal == MGOAL_ASSASSINATE && i.target_npc_id == getID()) {
                g->mission_step_complete( i.uid, 1);
        }
        //fail the mission if the mission giver dies or the recruit target
        if (i.npc_id == getID() || (i.target_npc_id == getID() && type->goal == MGOAL_RECRUIT_NPC)) {
            g->fail_mission( i.uid );
        }
    }
}

std::string npc_attitude_name(npc_attitude att)
{
 switch (att) {
 case NPCATT_NULL:          // Don't care/ignoring player
  return _("Ignoring");
 case NPCATT_TALK:          // Move to and talk to player
  return _("Wants to talk");
 case NPCATT_TRADE:         // Move to and trade with player
  return _("Wants to trade");
 case NPCATT_FOLLOW:        // Follow the player
  return _("Following");
 case NPCATT_FOLLOW_RUN:    // Follow the player, don't shoot monsters
  return _("Following & ignoring monsters");
 case NPCATT_LEAD:          // Lead the player, wait for them if they're behind
  return _("Leading");
 case NPCATT_WAIT:          // Waiting for the player
  return _("Waiting for you");
 case NPCATT_DEFEND:        // Kill monsters that threaten the player
  return _("Defending you");
 case NPCATT_MUG:           // Mug the player
  return _("Mugging you");
 case NPCATT_WAIT_FOR_LEAVE:// Attack the player if our patience runs out
  return _("Waiting for you to leave");
 case NPCATT_KILL:          // Kill the player
  return _("Attacking to kill");
 case NPCATT_FLEE:          // Get away from the player
  return _("Fleeing");
 case NPCATT_SLAVE:         // Following the player under duress
  return _("Enslaved");
 case NPCATT_HEAL:          // Get to the player and heal them
  return _("Healing you");

 case NPCATT_MISSING:       // Special; missing NPC as part of mission
  return _("Missing NPC");
 case NPCATT_KIDNAPPED:     // Special; kidnapped NPC as part of mission
  return _("Kidnapped");
 default:
  return _("Unknown");
 }
 return _("Unknown");
}

std::string npc_class_name_str(npc_class classtype)
{
    switch(classtype) {
    case NC_NONE:
        return "NC_NONE";
    case NC_EVAC_SHOPKEEP: // Found in the evacuation center.
        return "NC_EVAC_SHOPKEEP";
    case NC_ARSONIST: // Found in the evacuation center.
        return "NC_ARSONIST";
    case NC_SHOPKEEP: // Found in towns.  Stays in his shop mostly.
        return "NC_SHOPKEEP";
    case NC_HACKER: // Weak in combat but has hacking skills and equipment
        return "NC_HACKER";
    case NC_DOCTOR: // Found in towns, or roaming.  Stays in the clinic.
        return "NC_DOCTOR";
    case NC_TRADER: // Roaming trader, journeying between towns.
        return "NC_TRADER";
    case NC_NINJA: // Specializes in unarmed combat, carries few items
        return "NC_NINJA";
    case NC_COWBOY: // Gunslinger and survivalist
        return "NC_COWBOY";
    case NC_SCIENTIST: // Uses intelligence-based skills and high-tech items
        return "NC_SCIENTIST";
    case NC_BOUNTY_HUNTER: // Resourceful and well-armored
        return "NC_BOUNTY_HUNTER";
    case NC_THUG:   // Moderate melee skills and poor equipment
        return "NC_THUG";
    case NC_SCAVENGER: // Good with pistols light weapons
        return "NC_SCAVENGER";
    case NC_HUNTER: // Good with bows and rifles
        return "NC_HUNTER";
    default:
        //Suppress warnings
        break;
    }
    return "Unknown class";
}

std::string npc_class_name(npc_class classtype)
{
    switch(classtype) {
    case NC_NONE:
        return _("No class");
    case NC_EVAC_SHOPKEEP: // Found in the evacuation center.
        return _("Merchant");
    case NC_ARSONIST: // Found in the evacuation center.
        return _("Arsonist");
    case NC_SHOPKEEP: // Found in towns.  Stays in his shop mostly.
        return _("Shopkeep");
    case NC_HACKER: // Weak in combat but has hacking skills and equipment
        return _("Hacker");
    case NC_DOCTOR: // Found in towns, or roaming.  Stays in the clinic.
        return _("Doctor");
    case NC_TRADER: // Roaming trader, journeying between towns.
        return _("Trader");
    case NC_NINJA: // Specializes in unarmed combat, carries few items
        return _("Ninja");
    case NC_COWBOY: // Gunslinger and survivalist
        return _("Cowboy");
    case NC_SCIENTIST: // Uses intelligence-based skills and high-tech items
        return _("Scientist");
    case NC_BOUNTY_HUNTER: // Resourceful and well-armored
        return _("Bounty Hunter");
    case NC_THUG:   // Moderate melee skills and poor equipment
        return _("Thug");
    case NC_SCAVENGER: // Good with pistols light weapons
        return _("Scavenger");
    case NC_HUNTER: // Good with bows and rifles
        return _("Hunter");
    default:
        //Suppress warnings
        break;
    }
    return _("Unknown class");
}

void npc::setID (int i)
{
    this->player::setID(i);
}

//message related stuff
void npc::add_msg_if_npc(const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    std::string processed_npc_string = vstring_format(msg, ap);
    // These strings contain the substring <npcname>,
    // if present replace it with the actual npc name.
    size_t offset = processed_npc_string.find("<npcname>");
    if (offset != std::string::npos) {
        processed_npc_string.replace(offset, 9, name);
    }
    add_msg(processed_npc_string.c_str());

    va_end(ap);
};
void npc::add_msg_player_or_npc(const char *, const char* npc_str, ...) const
{
    va_list ap;

    va_start(ap, npc_str);

    if (g->u_see(this)) {
        std::string processed_npc_string = vstring_format(npc_str, ap);
        // These strings contain the substring <npcname>,
        // if present replace it with the actual npc name.
        size_t offset = processed_npc_string.find("<npcname>");
        if (offset != std::string::npos) {
            processed_npc_string.replace(offset, 9, disp_name());
        }
        add_msg(processed_npc_string.c_str());
    }

    va_end(ap);
};
void npc::add_msg_if_npc(game_message_type type, const char *msg, ...) const
{
    va_list ap;
    va_start(ap, msg);
    std::string processed_npc_string = vstring_format(msg, ap);
    // These strings contain the substring <npcname>,
    // if present replace it with the actual npc name.
    size_t offset = processed_npc_string.find("<npcname>");
    if (offset != std::string::npos) {
        processed_npc_string.replace(offset, 9, name);
    }
    add_msg(type, processed_npc_string.c_str());

    va_end(ap);
};
void npc::add_msg_player_or_npc(game_message_type type, const char *, const char* npc_str, ...) const
{
    va_list ap;

    va_start(ap, npc_str);

    if (g->u_see(this)) {
        std::string processed_npc_string = vstring_format(npc_str, ap);
        // These strings contain the substring <npcname>,
        // if present replace it with the actual npc name.
        size_t offset = processed_npc_string.find("<npcname>");
        if (offset != std::string::npos) {
            processed_npc_string.replace(offset, 9, disp_name());
        }
        add_msg(type, processed_npc_string.c_str());
    }

    va_end(ap);
};

const tripoint npc::no_goal_point(INT_MIN, INT_MIN, INT_MIN);
