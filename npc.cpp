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

std::vector<item> starting_clothes(npc_class type, bool male, game *g);
std::vector<item> starting_inv(npc *me, npc_class type, game *g);

npc::npc()
{
 id = -1;
 omx = 0;
 omy = 0;
 mapx = 0;
 mapy = 0;
 posx = -1;
 posy = -1;
 wandx = 0;
 wandy = 0;
 wandf = 0;
 plx = 999;
 ply = 999;
 itx = -1;
 ity = -1;
 goalx = 999;
 goaly = 999;
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
 marked_for_death = false;
 dead = false;
 moves = 100;
 mission = NPC_MISSION_NULL;
 myclass = NC_NONE;
 patience = 0;
 for (int i = 0; i < num_skill_types; i++)
  sklevel[i] = 0;
}

npc::npc(const npc &rhs)
{
 *this = rhs;
}

npc::~npc()
{

}

npc& npc::operator= (const npc & rhs)
{
 player::operator=(rhs);

 attitude = rhs.attitude;
 myclass = rhs.myclass;
 wandx = rhs.wandx;
 wandy = rhs.wandy;
 wandf = rhs.wandf;

 // Location:
 omx = rhs.omx;
 omy = rhs.omy;
 omz = rhs.omz;
 mapx = rhs.mapx;
 mapy = rhs.mapy;
 plx = rhs.plx;
 ply = rhs.ply;
 plt = rhs.plt;
 itx = rhs.itx;
 ity = rhs.ity;
 goalx = rhs.goalx;
 goaly = rhs.goaly;

 path = rhs.path;

 fetching_item = rhs.fetching_item;
 has_new_items = rhs.has_new_items;
 worst_item_value = rhs.worst_item_value;

 fac_id = rhs.fac_id;
 my_fac = rhs.my_fac;
 mission = rhs.mission;
 personality = rhs.personality;
 op_of_u = rhs.op_of_u;
 chatbin = rhs.chatbin;
 patience = rhs.patience;
 combat_rules = rhs.combat_rules;
 marked_for_death = rhs.marked_for_death;
 dead = rhs.dead;

 needs = rhs.needs;

 flags = rhs.flags;

 posx = rhs.posx;
 posy = rhs.posy;

 weapon = rhs.weapon;
 ret_null = rhs.ret_null;
 inv = rhs.inv;
 worn.clear();
 for (int i = 0; i < rhs.worn.size(); i++)
  worn.push_back(rhs.worn[i]);

 needs.clear();
 for (int i = 0; i < rhs.needs.size(); i++)
  needs.push_back(rhs.needs[i]);

 path.clear();
 for (int i = 0; i < rhs.path.size(); i++)
  path.push_back(rhs.path[i]);

 for (int i = 0; i < num_hp_parts; i++) {
  hp_cur[i] = rhs.hp_cur[i];
  hp_max[i] = rhs.hp_max[i];
 }

 for (int i = 0; i < num_skill_types; i++)
  sklevel[i] = rhs.sklevel[i];

 styles.clear();
 for (int i = 0; i < rhs.styles.size(); i++)
  styles.push_back(rhs.styles[i]);

 
 return *this;
}

std::string npc::save_info()
{
 std::stringstream dump;
// The " || " is what tells npc::load_info() that it's down reading the name
 dump << id << " " << name << " || " << posx << " " << posy << " " << str_cur <<
         " " << str_max << " " << dex_cur << " " << dex_max << " " << int_cur <<
         " " << int_max << " " << per_cur << " " << per_max << " " << hunger <<
         " " << thirst << " " << fatigue << " " << stim << " " << pain << " " <<
         pkill << " " <<  radiation << " " << cash << " " << recoil << " " <<
         scent << " " << moves << " " << underwater << " " << dodges_left <<
         " " << oxygen << " " << (marked_for_death ? "1" : "0") << " " <<
         (dead ? "1" : "0") << " " << myclass << " " << patience << " ";

 for (int i = 0; i < PF_MAX2; i++)
  dump << my_traits[i] << " ";
 for (int i = 0; i < num_hp_parts; i++)
  dump << hp_cur[i] << " " << hp_max[i] << " ";
 for (int i = 0; i < num_skill_types; i++)
  dump << int(sklevel[i]) << " " << skexercise[i] << " ";

 dump << styles.size() << " ";
 for (int i = 0; i < styles.size(); i++)
  dump << int(styles[i]) << " ";

 dump << illness.size() << " ";
 for (int i = 0; i < illness.size();  i++)
  dump << int(illness[i].type) << " " << illness[i].duration << " ";

 dump << addictions.size() << " ";
 for (int i = 0; i < addictions.size(); i++)
  dump << int(addictions[i].type) << " " << addictions[i].intensity << " " <<
          addictions[i].sated << " ";

 dump << my_bionics.size() << " ";
 for (int i = 0; i < my_bionics.size(); i++)
  dump << int(my_bionics[i].id) << " " << my_bionics[i].invlet << " " <<
          int(my_bionics[i].powered) << " " << my_bionics[i].charge << " ";

// NPC-specific stuff
 dump << int(personality.aggression) << " " << int(personality.bravery) <<
         " " << int(personality.collector) << " " <<
         int(personality.altruism) << " " << wandx << " " << wandy << " " <<
         wandf << " " << omx << " " << omy << " " << omz << " " << mapx <<
         " " << mapy << " " << plx << " " << ply << " " <<  goalx << " " <<
         goaly << " " << int(mission) << " " << int(flags) << " ";
 if (my_fac == NULL)
  dump << -1;
 else
  dump << my_fac->id;
 dump << " " << attitude << " " << " " << op_of_u.save_info() << " " <<
         chatbin.save_info() << " ";

 dump << combat_rules.save_info();
 
// Inventory size, plus armor size, plus 1 for the weapon
 dump << std::endl << inv.num_items() + worn.size() + 1 << std::endl;
 for (int i = 0; i < inv.size(); i++) {
  for (int j = 0; j < inv.stack_at(i).size(); j++) {
   dump << "I " << inv.stack_at(i)[j].save_info() << std::endl;
   for (int k = 0; k < inv.stack_at(i)[j].contents.size(); k++)
    dump << "C " << inv.stack_at(i)[j].contents[k].save_info() << std::endl;
  }
 }
 dump << "w " << weapon.save_info() << std::endl;
 for (int i = 0; i < worn.size(); i++)
  dump << "W " << worn[i].save_info() << std::endl;
 for (int j = 0; j < weapon.contents.size(); j++)
  dump << "c " << weapon.contents[j].save_info() << std::endl;

 return dump.str();
}

void npc::load_info(game *g, std::string data)
{
 std::stringstream dump;
 std::string tmpname;
 int deathtmp, deadtmp, classtmp;
 dump << data;
 dump >> id;
// Standard player stuff
 do {
  dump >> tmpname;
  if (tmpname != "||")
   name += tmpname + " ";
 } while (tmpname != "||");
 name = name.substr(0, name.size() - 1); // Strip off trailing " "
 dump >> posx >> posy >> str_cur >> str_max >> dex_cur >> dex_max >>
         int_cur >> int_max >> per_cur >> per_max >> hunger >> thirst >>
         fatigue >> stim >> pain >> pkill >> radiation >> cash >> recoil >>
         scent >> moves >> underwater >> dodges_left >> oxygen >> deathtmp >>
         deadtmp >> classtmp >> patience;

 if (deathtmp == 1)
  marked_for_death = true;
 else
  marked_for_death = false;

 if (deadtmp == 1)
  dead = true;
 else
  dead = false;

 myclass = npc_class(classtmp);

 for (int i = 0; i < PF_MAX2; i++)
  dump >> my_traits[i];

 for (int i = 0; i < num_hp_parts; i++)
  dump >> hp_cur[i] >> hp_max[i];
 for (int i = 0; i < num_skill_types; i++)
  dump >> sklevel[i] >> skexercise[i];

 int numill, numstyle;
 int typetmp;
 disease illtmp;
 dump >> numstyle;
 for (int i = 0; i < numstyle; i++) {
  dump >> typetmp;
  styles.push_back(itype_id(typetmp));
 }

 dump >> numill;
 for (int i = 0; i < numill; i++) {
  dump >> typetmp >> illtmp.duration;
  illtmp.type = dis_type(typetmp);
  illness.push_back(illtmp);
 }
 int numadd;
 addiction addtmp;
 dump >> numadd;
 for (int i = 0; i < numadd; i++) {
  dump >> typetmp >> addtmp.intensity >> addtmp.sated;
  addtmp.type = add_type(typetmp);
  addictions.push_back(addtmp);
 }
 int numbio;
 bionic biotmp;
 dump >> numbio;
 for (int i = 0; i < numbio; i++) {
  dump >> typetmp >> biotmp.invlet >> biotmp.powered >> biotmp.charge;
  biotmp.id = bionic_id(typetmp);
  my_bionics.push_back(biotmp);
 }
// Special NPC stuff
 int misstmp, flagstmp, tmpatt, agg, bra, col, alt;
 dump >> agg >> bra >> col >> alt >> wandx >> wandy >> wandf >> omx >> omy >>
         omz >> mapx >> mapy >> plx >> ply >> goalx >> goaly >> misstmp >>
         flagstmp >> fac_id >> tmpatt;
 personality.aggression = agg;
 personality.bravery = bra;
 personality.collector = col;
 personality.altruism = alt;
 mission = npc_mission(misstmp);
 flags = flagstmp;
 attitude = npc_attitude(tmpatt);

 op_of_u.load_info(dump);
 chatbin.load_info(dump);
 combat_rules.load_info(dump);
}

void npc::randomize(game *g, npc_class type)
{
 id = g->assign_npc_id();
 str_max = dice(4, 3);
 dex_max = dice(4, 3);
 int_max = dice(4, 3);
 per_max = dice(4, 3);
 ret_null = item(g->itypes[0], 0);
 weapon   = item(g->itypes[0], 0);
 inv.clear();
 personality.aggression = rng(-10, 10);
 personality.bravery =    rng( -3, 10);
 personality.collector =  rng( -1, 10);
 personality.altruism =   rng(-10, 10);
 //cash = 100 * rng(0, 20) + 10 * rng(0, 30) + rng(0, 50);
 cash = 0;
 moves = 100;
 mission = NPC_MISSION_NULL;
 if (one_in(2))
  male = true;
 else
  male = false;
 pick_name();

 if (type == NC_NONE)
  type = npc_class(rng(0, NC_MAX - 1));
 if (one_in(5))
  type = NC_NONE;

 myclass = type;
 switch (type) {	// Type of character
 case NC_NONE:	// Untyped; no particular specialization
  for (int i = 1; i < num_skill_types; i++) {
   sklevel[i] = dice(4, 2) - rng(1, 4);
   if (!one_in(3))
    sklevel[i] = 0;
  }
  break;

 case NC_HACKER:
  for (int i = 1; i < num_skill_types; i++) {
   sklevel[i] = dice(2, 2) - rng(1, 2);
   if (!one_in(3))
    sklevel[i] = 0;
  }
  sklevel[sk_electronics] += rng(1, 4);
  sklevel[sk_computer] += rng(3, 6);
  str_max -= rng(0, 4);
  dex_max -= rng(0, 2);
  int_max += rng(1, 5);
  per_max -= rng(0, 2);
  personality.bravery -= rng(1, 3);
  personality.aggression -= rng(0, 2);
  break;

 case NC_DOCTOR:
  for (int i = 1; i < num_skill_types; i++) {
   sklevel[i] = dice(3, 2) - rng(1, 3);
   if (!one_in(3))
    sklevel[i] = 0;
  }
  sklevel[sk_firstaid] += rng(2, 6);
  str_max -= rng(0, 2);
  int_max += rng(0, 2);
  per_max += rng(0, 1) * rng(0, 1);
  personality.aggression -= rng(0, 4);
  if (one_in(4))
   flags |= mfb(NF_DRUGGIE);
  cash += 100 * rng(0, 3) * rng(0, 3);
  break;

 case NC_TRADER:
  for (int i = 1; i < num_skill_types; i++) {
   sklevel[i] = dice(2, 2) - 2 + (rng(0, 1) * rng(0, 1));
   if (!one_in(3))
    sklevel[i] = 0;
  }
  sklevel[sk_mechanics] += rng(0, 2);
  sklevel[sk_electronics] +=  rng(0, 2);
  sklevel[sk_speech] += rng(0, 3);
  sklevel[sk_barter] += rng(2, 5);
  int_max += rng(0, 1) * rng(0, 1);
  per_max += rng(0, 1) * rng(0, 1);
  personality.collector += rng(1, 5);
  cash += 250 * rng(1, 10);
  break;

 case NC_NINJA:
  for (int i = 1; i < num_skill_types; i++) {
   sklevel[i] = dice(2, 2) - rng(1, 2);
   if (!one_in(3))
    sklevel[i] = 0;
  }
  sklevel[sk_dodge] += rng(2, 4);
  sklevel[sk_melee] += rng(1, 4);
  sklevel[sk_unarmed] += rng(4, 6);
  sklevel[sk_throw] += rng(0, 2);
  str_max -= rng(0, 1);
  dex_max += rng(0, 2);
  per_max += rng(0, 2);
  personality.bravery += rng(0, 3);
  personality.collector -= rng(1, 6);
  do
   styles.push_back( itype_id( rng(itm_style_karate, itm_style_zui_quan) ) );
  while (one_in(2));
  break;

 case NC_COWBOY:
  for (int i = 1; i < num_skill_types; i++) {
   sklevel[i] = dice(3, 2) - rng(0, 4);
   if (sklevel[i] < 0)
    sklevel[i] = 0;
  }
  sklevel[sk_gun] += rng(1, 3);
  sklevel[sk_pistol] += rng(1, 3);
  sklevel[sk_rifle] += rng(0, 2);
  int_max -= rng(0, 2);
  str_max += rng(0, 1);
  per_max += rng(0, 2);
  personality.aggression += rng(0, 2);
  personality.bravery += rng(1, 5);
  break;

 case NC_SCIENTIST:
  for (int i = 1; i < num_skill_types; i++) {
   sklevel[i] = dice(3, 2) - 4;
   if (sklevel[i] < 0)
    sklevel[i] = 0;
  }
  sklevel[sk_computer] += rng(0, 3);
  sklevel[sk_electronics] += rng(0, 3);
  sklevel[sk_firstaid] += rng(0, 1);
  switch (rng(1, 3)) { // pick a specialty
   case 1: sklevel[sk_computer] += rng(2, 6); break;
   case 2: sklevel[sk_electronics] += rng(2, 6); break;
   case 3: sklevel[sk_firstaid] += rng(2, 6); break;
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
  for (int i = 1; i < num_skill_types; i++) {
   sklevel[i] = dice(3, 2) - 3;
   if (sklevel[i] > 0 && one_in(3))
    sklevel[i]--;
  }
  sklevel[sk_gun] += rng(2, 4);
  sklevel[rng(sk_pistol, sk_rifle)] += rng(3, 5);
  personality.aggression += rng(1, 6);
  personality.bravery += rng(0, 5);
  break;
 }
 for (int i = 0; i < num_hp_parts; i++) {
  hp_max[i] = 60 + str_max * 3;
  hp_cur[i] = hp_max[i];
 }
 starting_weapon(g);
 worn = starting_clothes(type, male, g);
 inv.clear();
 inv.add_stack(starting_inv(this, type, g));
 update_worst_item_value();
}

void npc::randomize_from_faction(game *g, faction *fac)
{
// Personality = aggression, bravery, altruism, collector
 my_fac = fac;
 randomize(g);

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
    randomize(g, NC_SCIENTIST);
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
   randomize(g, NC_TRADER);
  personality.aggression -= rng(1, 5);
  personality.collector += rng(1, 4);
  personality.altruism -= rng(0, 3);
 }
 if (fac->has_job(FACJOB_SCAVENGE))
  personality.collector += rng(4, 8);
 if (fac->has_job(FACJOB_MERCENARIES)) {
  if (!one_in(3)) {
   switch (rng(1, 3)) {
    case 1: randomize(g, NC_NINJA);		break;
    case 2: randomize(g, NC_COWBOY);		break;
    case 3: randomize(g, NC_BOUNTY_HUNTER);	break;
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
   randomize(g, NC_COWBOY);
  personality.aggression += rng(3, 5);
  personality.bravery += rng(0, 2);
  personality.altruism -= rng(3, 6);
  str_max += rng(0, 3);
  int_max -= rng(0, 2);
 }
 if (fac->has_job(FACJOB_THIEVES)) {
  if (one_in(3))
   randomize(g, NC_NINJA);
  personality.aggression -= rng(2, 5);
  personality.bravery -= rng(1, 3);
  personality.altruism -= rng(1, 4);
  str_max -= rng(0, 2);
  per_max += rng(1, 4);
  dex_max += rng(1, 3);
 }
 if (fac->has_job(FACJOB_DOCTORS)) {
  if (!one_in(4))
   randomize(g, NC_DOCTOR);
  personality.aggression -= rng(3, 6);
  personality.bravery += rng(0, 4);
  personality.altruism += rng(0, 4);
  int_max += rng(2, 4);
  per_max += rng(0, 2);
  sklevel[sk_firstaid] += rng(1, 5);
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
   case 1: sklevel[sk_mechanics] += dice(2, 4);		break;
   case 2: sklevel[sk_electronics] += dice(2, 4);	break;
   case 3: sklevel[sk_cooking] += dice(2, 4);		break;
   case 4: sklevel[sk_tailor] += dice(2,  4);		break;
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
   sklevel[sk_mechanics] += dice(2, 3);
   sklevel[sk_electronics] += dice(2, 3);
   sklevel[sk_firstaid] += dice(2, 3);
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
  for (int i = 1; i < num_skill_types; i++) {
   if (one_in(3))
    sklevel[i] += rng(2, 4);
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

void npc::make_shopkeep(game *g, oter_id type)
{
 randomize(g, NC_TRADER);
 itype_id item_type;
 item tmp;
 std::vector<items_location> pool;
 bool done = false;
 switch (type) {
/*
 case ot_set_food:
  pool.push_back(mi_snacks);
  for (int i = 0; i < 4; i++)	// Weighted to be more likely
   pool.push_back(mi_cannedfood);
  pool.push_back(mi_pasta);
  pool.push_back(mi_produce);
  pool.push_back(mi_alcohol);
  break;
  break;
 case ot_set_weapons:
  pool.push_back(mi_weapons);
  break;
 case ot_set_guns:
  pool.push_back(mi_allguns);
  break;
 case ot_set_clinic:
  pool.push_back(mi_softdrugs);
  pool.push_back(mi_harddrugs);
  break;
 case ot_set_clothing:
  pool.push_back(mi_allclothes);
  break;
 case ot_set_general:
  pool.push_back(mi_cleaning);
  pool.push_back(mi_hardware);
  pool.push_back(mi_tools);
  pool.push_back(mi_bigtools);
  pool.push_back(mi_mischw);
  pool.push_back(mi_gunxtras);
  pool.push_back(mi_electronics);
  for (int i = 0; i < 4; i++)
   pool.push_back(mi_survival_tools);
  break;
 case ot_set_library:
  pool.push_back(mi_magazines);
  pool.push_back(mi_novels);
  pool.push_back(mi_novels);
  pool.push_back(mi_manuals);
  pool.push_back(mi_manuals);
  pool.push_back(mi_textbooks);
  break;
 case ot_set_bionics:
  pool.push_back(mi_electronics);
  pool.push_back(mi_bionics);
  break;
*/
 }

 if (pool.size() > 0) {
  do {
   items_location place = pool[rng(0, pool.size() - 1)];
   item_type = g->mapitems[place][rng(0, g->mapitems[place].size() - 1)];
   tmp = item(g->itypes[item_type], g->turn);
   if (volume_carried() + tmp.volume() > volume_capacity() ||
       weight_carried() + tmp.weight() > weight_capacity()   )
    done = true;
   else
    inv.push_back(tmp);
  } while (!done);
 }
 mission = NPC_MISSION_SHOPKEEP;
 update_worst_item_value();
}

std::vector<item> starting_clothes(npc_class type, bool male, game *g)
{
 std::vector<item> ret;
 itype_id pants = itm_null, shoes = itm_null, shirt = itm_null,
                  gloves = itm_null, coat = itm_null, mask = itm_null,
                  glasses = itm_null, hat = itm_null;

 switch(rng(0, (male ? 3 : 4))) {
  case 0: pants = itm_jeans; break;
  case 1: pants = itm_pants; break;
  case 2: pants = itm_pants_leather; break;
  case 3: pants = itm_pants_cargo; break;
  case 4: pants = itm_skirt; break;
 }
 switch (rng(0, 3)) {
  case 0: shirt = itm_tshirt; break;
  case 1: shirt = itm_polo_shirt; break;
  case 2: shirt = itm_dress_shirt; break;
  case 3: shirt = itm_tank_top; break;
 }
 switch(rng(0, 10)) {
  case  8: gloves = itm_gloves_leather; break;
  case  9: gloves = itm_gloves_fingerless; break;
  case 10: gloves = itm_fire_gauntlets; break;
 }
 switch (rng(0, 6)) {
  case 2: coat = itm_hoodie; break;
  case 3: coat = itm_jacket_light; break;
  case 4: coat = itm_jacket_jean; break;
  case 5: coat = itm_jacket_leather; break;
  case 6: coat = itm_trenchcoat; break;
 }
 if (one_in(30))
  coat = itm_kevlar;
 shoes = itm_sneakers;
 mask = itm_null;
 if (one_in(8)) {
  switch(rng(0, 2)) {
   case 0: mask = itm_mask_dust; break;
   case 1: mask = itm_bandana; break;
   case 2: mask = itm_mask_filter; break;
  }
 }
 glasses = itm_null;
 if (one_in(8))
  glasses = itm_glasses_safety;
 hat = itm_null;
 if (one_in(6)) {
  switch(rng(0, 5)) {
   case 0: hat = itm_hat_ball; break;
   case 1: hat = itm_hat_hunting; break;
   case 2: hat = itm_hat_hard; break;
   case 3: hat = itm_helmet_bike; break;
   case 4: hat = itm_helmet_riot; break;
   case 5: hat = itm_helmet_motor; break;
  }
 }

// Now, more specific stuff for certain classes.
 switch (type) {
 case NC_DOCTOR:
  if (one_in(2))
   pants = itm_pants;
  if (one_in(3))
   shirt = (one_in(2) ? itm_polo_shirt : itm_dress_shirt);
  if (!one_in(8))
   coat = itm_coat_lab;
  if (one_in(3))
   mask = itm_mask_dust;
  if (one_in(4))
   glasses = itm_glasses_safety;
  if (gloves != itm_null || one_in(3))
   gloves = itm_gloves_medical;
  break;

 case NC_TRADER:
  if (one_in(2))
   pants = itm_pants_cargo;
  switch (rng(0, 8)) {
   case 1: coat = itm_hoodie; break;
   case 2: coat = itm_jacket_jean; break;
   case 3: case 4: coat = itm_vest; break;
   case 5: case 6: case 7: case 8: coat = itm_trenchcoat; break;
  }
  break;

 case NC_NINJA:
  if (one_in(4))
   shirt = itm_null;
  else if (one_in(3))
   shirt = itm_tank_top;
  if (one_in(5))
   gloves = itm_gloves_leather;
  if (one_in(2))
   mask = itm_bandana;
  if (one_in(3))
   hat = itm_null;
  break;

 case NC_COWBOY:
  if (one_in(2))
   shoes = itm_boots;
  if (one_in(2))
   pants = itm_jeans;
  if (one_in(3))
   shirt = itm_tshirt;
  if (one_in(4))
   gloves = itm_gloves_leather;
  if (one_in(4))
   coat = itm_jacket_jean;
  if (one_in(3))
   hat = itm_hat_boonie;
  break;

 case NC_SCIENTIST:
  if (one_in(4))
   glasses = itm_glasses_eye;
  else if (one_in(2))
   glasses = itm_glasses_safety;
  if (one_in(5))
   coat = itm_coat_lab;
  break;

 case NC_BOUNTY_HUNTER:
  if (one_in(3))
   pants = itm_pants_cargo;
  if (one_in(2))
   shoes = itm_boots_steel;
  if (one_in(4))
   coat = itm_jacket_leather;
  if (one_in(4))
   mask = itm_mask_filter;
  if (one_in(5))
   glasses = itm_goggles_ski;
  if (one_in(3)) {
   mask = itm_null;
   hat = itm_helmet_motor;
  }
  break;
 }
// Fill in the standard things we wear
 if (shoes != itm_null)
  ret.push_back(item(g->itypes[shoes], 0));
 if (pants != itm_null)
  ret.push_back(item(g->itypes[pants], 0));
 if (shirt != itm_null)
  ret.push_back(item(g->itypes[shirt], 0));
 if (coat != itm_null)
  ret.push_back(item(g->itypes[coat], 0));
 if (gloves != itm_null)
  ret.push_back(item(g->itypes[gloves], 0));
// Bad to wear a mask under a motorcycle helmet
 if (mask != itm_null && hat != itm_helmet_motor)
  ret.push_back(item(g->itypes[mask], 0));
 if (glasses != itm_null)
  ret.push_back(item(g->itypes[glasses], 0));
 if (hat != itm_null)
  ret.push_back(item(g->itypes[hat], 0));

// Second pass--for extra stuff like backpacks, etc
 switch (type) {
 case NC_NONE:
 case NC_DOCTOR:
 case NC_SCIENTIST:
  if (one_in(10))
   ret.push_back(item(g->itypes[itm_backpack], 0));
  break;
 case NC_COWBOY:
 case NC_BOUNTY_HUNTER:
  if (one_in(2))
   ret.push_back(item(g->itypes[itm_backpack], 0));
  break;
 case NC_TRADER:
  if (!one_in(15))
   ret.push_back(item(g->itypes[itm_backpack], 0));
  break;
 }

 return ret;
}

std::vector<item> starting_inv(npc *me, npc_class type, game *g)
{
 int total_space = me->volume_capacity() - 2;
 std::vector<item> ret;
 ret.push_back( item(g->itypes[itm_lighter], 0) );
 itype_id tmp;

// First, if we're wielding a gun, get some ammo for it
 if (me->weapon.is_gun()) {
  it_gun *gun = dynamic_cast<it_gun*>(me->weapon.type);
  tmp = default_ammo(gun->ammo);
  if (total_space >= g->itypes[tmp]->volume) {
   ret.push_back(item(g->itypes[tmp], 0));
   total_space -= ret[ret.size() - 1].volume();
  }
  while ((type == NC_COWBOY || type == NC_BOUNTY_HUNTER || !one_in(3)) &&
         !one_in(4) && total_space >= g->itypes[tmp]->volume) {
   ret.push_back(item(g->itypes[tmp], 0));
   total_space -= ret[ret.size() - 1].volume();
  }
 }
 if (type == NC_TRADER) {	// Traders just have tons of random junk
  while (total_space > 0 && !one_in(50)) {
   tmp = itype_id(rng(2, num_items - 1));
   if (total_space >= g->itypes[tmp]->volume) {
    ret.push_back(item(g->itypes[tmp], 0));
    ret[ret.size() - 1] = ret[ret.size() - 1].in_its_container(&g->itypes);
    total_space -= ret[ret.size() - 1].volume();
   }
  }
 }
 int index;
 items_location from;
 if (type == NC_HACKER) {
  from = mi_npc_hacker;
  while(total_space > 0 && !one_in(10)) {
   index = rng(0, g->mapitems[from].size() - 1);
   tmp = g->mapitems[from][index];
   item tmpit(g->itypes[tmp], 0);
   tmpit = tmpit.in_its_container(&g->itypes);
   if (total_space >= tmpit.volume()) {
    ret.push_back(tmpit);
    total_space -= tmpit.volume();
   }
  }
 }
 if (type == NC_DOCTOR) {
  while(total_space > 0 && !one_in(10)) {
   if (one_in(3))
    from = mi_softdrugs;
   else
    from = mi_harddrugs;
   index = rng(0, g->mapitems[from].size() - 1);
   tmp = g->mapitems[from][index];
   item tmpit(g->itypes[tmp], 0);
   tmpit = tmpit.in_its_container(&g->itypes);
   if (total_space >= tmpit.volume()) {
    ret.push_back(tmpit);
    total_space -= tmpit.volume();
   }
  }
 }
// TODO: More specifics.

 while (total_space > 0 && !one_in(8)) {
  tmp = itype_id(rng(4, num_items - 1));
  if (total_space >= g->itypes[tmp]->volume) {
   ret.push_back(item(g->itypes[tmp], 0));
   ret[ret.size() - 1] = ret[ret.size() - 1].in_its_container(&g->itypes);
   total_space -= ret[ret.size() - 1].volume();
  }
 }

 for (int i = 0; i < ret.size(); i++) {
  for (int j = 0; j < g->mapitems[mi_trader_avoid].size(); j++) {
   if (ret[i].type->id == g->mapitems[mi_trader_avoid][j]) {
    ret.erase(ret.begin() + i);
    i--;
    j = 0;
   }
  }
 }
 
 return ret;
}

void npc::spawn_at(overmap *o, int x, int y)
{
// First, specify that we are in this overmap!
 omx = o->posx;
 omy = o->posy;
 mapx = x;
 mapy = y;
 if (x == -1 || y == -1) {
  int city_index = rng(0, o->cities.size() - 1);
  int x = o->cities[city_index].x;
  int y = o->cities[city_index].y;
  int s = o->cities[city_index].s;
  if (x == -1)
   mapx = rng(x - s, x + s);
  if (y == -1)
   mapy = rng(y - s, y + s);
 }
}

skill npc::best_skill()
{
 std::vector<int> best_skills;
 int highest = 0;
 for (int i = sk_unarmed; i <= sk_rifle; i++) {
  if (sklevel[i] > highest && i != sk_gun) {
   highest = sklevel[i];
   best_skills.clear();
  }
  if (sklevel[i] == highest && i != sk_gun)
   best_skills.push_back(i);
 }
 int index = rng(0, best_skills.size() - 1);
 return skill(best_skills[index]);
}

void npc::starting_weapon(game *g)
{
 if (!styles.empty()) {
  weapon.make(g->itypes[styles[rng(0, styles.size() - 1)]]);
  return;
 }
 skill best = best_skill();
 int index;
 switch (best) {
 case sk_bashing:
  switch (rng(0, 5)) {
   case 0: weapon.make(g->itypes[itm_hammer]);        break;
   case 1: weapon.make(g->itypes[itm_wrench]);        break;
   case 2: weapon.make(g->itypes[itm_hammer_sledge]); break;
   case 3: weapon.make(g->itypes[itm_pipe]);          break;
   case 4: weapon.make(g->itypes[itm_bat]);           break;
   case 5: weapon.make(g->itypes[itm_crowbar]);       break;
  }
  break;
 case sk_cutting:
  switch (rng(0, 5)) {
   case 0: weapon.make(g->itypes[itm_knife_butcher]); break;
   case 1: weapon.make(g->itypes[itm_hatchet]);       break;
   case 2: weapon.make(g->itypes[itm_ax]);            break;
   case 3: weapon.make(g->itypes[itm_machete]);       break;
   case 4: weapon.make(g->itypes[itm_knife_combat]);  break;
   case 5: weapon.make(g->itypes[itm_katana]);        break;
  }
  break;
 case sk_throw:
// TODO: Some throwing weapons... grenades?
  break;
 case sk_pistol:
  index = rng(0, g->mapitems[mi_pistols].size() - 1);
  weapon.make(g->itypes[(g->mapitems[mi_pistols])[index]]);
  break;
 case sk_shotgun:
  index = rng(0, g->mapitems[mi_shotguns].size() - 1);
  weapon.make(g->itypes[(g->mapitems[mi_shotguns])[index]]);
  break;
 case sk_smg:
  index = rng(0, g->mapitems[mi_smg].size() - 1);
  weapon.make(g->itypes[(g->mapitems[mi_smg])[index]]);
  break;
 case sk_rifle:
  index = rng(0, g->mapitems[mi_rifles].size() - 1);
  weapon.make(g->itypes[(g->mapitems[mi_rifles])[index]]);
  break;
 }
 if (weapon.is_gun()) {
  it_gun* gun = dynamic_cast<it_gun*>(weapon.type);
  weapon.charges = gun->clip;
  weapon.curammo = dynamic_cast<it_ammo*>(g->itypes[default_ammo(gun->ammo)]);
 }
}

bool npc::wear_if_wanted(item it)
{
 if (!it.is_armor())
  return false;

 it_armor* armor = dynamic_cast<it_armor*>(it.type);
 int max_encumb[num_bp] = {2, 3, 3, 4, 3, 3, 3, 2};
 bool encumb_ok = true;
 for (int i = 0; i < num_bp && encumb_ok; i++) {
  if (armor->covers & mfb(i) && encumb(body_part(i)) + armor->encumber >
       max_encumb[i])
   encumb_ok = false;
 }
 if (encumb_ok) {
  worn.push_back(it);
  return true;
 }
// Otherwise, maybe we should take off one or more items and replace them
 std::vector<int> removal;
 for (int i = 0; i < worn.size(); i++) {
  for (int j = 0; j < num_bp; j++) {
   if (armor->covers & mfb(j) &&
       dynamic_cast<it_armor*>(worn[i].type)->covers & mfb(j)) {
    removal.push_back(i);
    j = num_bp;
   }
  }
 }
 for (int i = 0; i < removal.size(); i++) {
  if (true) {
//  if (worn[removal[i]].value_to(this) < it.value_to(this)) {
   inv.push_back(worn[removal[i]]);
   worn.push_back(it);
   return true;
  }
 }
 return false;
} 

bool npc::wield(game *g, int index)
{
 if (index < 0) { // Wielding a style
  index = 0 - index - 1;
  if (index >= styles.size()) {
   debugmsg("npc::wield(%d) [styles.size() = %d]", index, styles.size());
   return false;
  }
  if (volume_carried() + weapon.volume() <= volume_capacity()) {
   i_add(remove_weapon());
   moves -= 15; // Extra penalty for putting weapon away
  } else // No room for weapon, so we drop it
   g->m.add_item(posx, posy, remove_weapon());
  moves -= 15;
  weapon.make( g->itypes[styles[index]] );
  int linet;
  if (g->u_see(posx, posy, linet))
   g->add_msg("%s assumes a %s stance.", name.c_str(), weapon.tname().c_str());
  return true;
 }

 if (index >= inv.size()) {
  debugmsg("npc::wield(%d) [inv.size() = %d]", index, inv.size());
  return false;
 }
 if (volume_carried() + weapon.volume() <= volume_capacity()) {
  i_add(remove_weapon());
  moves -= 15;
 } else // No room for weapon, so we drop it
  g->m.add_item(posx, posy, remove_weapon());
 moves -= 15;
 weapon = inv[index];
 i_remn(index);
 int linet;
 if (g->u_see(posx, posy, linet))
  g->add_msg("%s wields a %s.", name.c_str(), weapon.tname().c_str());
 return true;
}

void npc::perform_mission(game *g)
{
 switch (mission) {
 case NPC_MISSION_RESCUE_U:
  if (int(g->turn) % 24 == 0) {
   if (mapx > g->levx)
    mapx--;
   else if (mapx < g->levx)
    mapx++;
   if (mapy > g->levy)
    mapy--;
   else if (mapy < g->levy)
    mapy++;
   attitude = NPCATT_DEFEND;
  }
  break;
 case NPC_MISSION_SHOPKEEP:
  break;	// Just stay where we are
 default:	// Random Walk
  if (int(g->turn) % 24 == 0) {
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
 else if (u->unarmed_attack())	// Unarmed
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

 if (u->has_trait(PF_DEFORMED2))
  op_of_u.fear += 1;
 if (u->has_trait(PF_TERRIFYING))
  op_of_u.fear += 2;

 if (u->stim > 20)
  op_of_u.fear++;

 if (u->has_disease(DI_DRUNK))
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

 if (u->has_disease(DI_HIGH))
  op_of_u.trust -= 1;
 if (u->has_disease(DI_DRUNK))
  op_of_u.trust -= 2;
 if (u->stim > 20 || u->stim < -20)
  op_of_u.trust -= 1;
 if (u->pkill > 30)
  op_of_u.trust -= 1;

 if (u->has_trait(PF_DEFORMED))
  op_of_u.trust -= 1;
 if (u->has_trait(PF_DEFORMED2))
  op_of_u.trust -= 3;

// VALUE
 op_of_u.value = 0;
 for (int i = 0; i < num_hp_parts; i++) {
  if (hp_cur[i] < hp_max[i] * .8)
   op_of_u.value++;
 }
 decide_needs();
 for (int i = 0; i < needs.size(); i++) {
  if (needs[i] == need_food || needs[i] == need_drink)
   op_of_u.value += 2;
 }

 if (op_of_u.fear < personality.bravery + 10 &&
     op_of_u.fear - personality.aggression > -10 && op_of_u.trust > -8)
  attitude = NPCATT_TALK;
 else if (op_of_u.fear - 2 * personality.aggression - personality.bravery < -30)
  attitude = NPCATT_KILL;
 else
  attitude = NPCATT_FLEE;
}

talk_topic npc::pick_talk_topic(player *u)
{
 //form_opinion(u);
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

int npc::player_danger(player *u)
{
 int ret = 0;
 if (u->weapon.is_gun()) {
  if (weapon.is_gun())
   ret += 4;
  else
   ret += 8;
 } else if (u->weapon.type->melee_dam >= 12 || u->weapon.type->melee_cut >= 12)
  ret++;
 else if (u->weapon.type->id == 0)	// Unarmed
  ret -= 3;

 if (u->str_cur > 20)	// Superhuman strength!
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

 if (u->has_trait(PF_TERRIFYING))
  ret += 2;

 if (u->stim > 20)
  ret++;

 if (u->has_disease(DI_DRUNK))
  ret -= 2;

 return ret;
}

bool npc::turned_hostile()
{
 return (op_of_u.anger >= hostile_anger_level());
}

int npc::hostile_anger_level()
{
 return (20 + op_of_u.fear - personality.aggression);
}

void npc::make_angry()
{
 if (is_enemy())
  return; // We're already angry!
 if (op_of_u.fear > 10 + personality.aggression + personality.bravery)
  attitude = NPCATT_FLEE; // We don't want to take u on!
 else
  attitude = NPCATT_KILL; // Yeah, we think we could take you!
}

bool npc::wants_to_travel_with(player *p)
{
/*
 int target = 8 + personality.bravery * 3 - personality.altruism * 2 -
              personality.collector * .5;
 int total = op_of_u.value * 3 + p->convince_score();
 return (total >= target);
*/
 return true;
}

int npc::assigned_missions_value(game *g)
{
 int ret = 0;
 for (int i = 0; i < chatbin.missions_assigned.size(); i++)
  ret += g->find_mission(chatbin.missions_assigned[i])->value;
 return ret;
}

std::vector<skill> npc::skills_offered_to(player *p)
{
 std::vector<skill> ret;
 if (p == NULL)
  return ret;
 for (int i = 0; i < num_skill_types; i++) {
  if (sklevel[i] > p->sklevel[i])
   ret.push_back( skill(i) );
 }
 return ret;
}

std::vector<itype_id> npc::styles_offered_to(player *p)
{
 std::vector<itype_id> ret;
 if (p == NULL)
  return ret;
 for (int i = 0; i < styles.size(); i++) {
  bool found = false;
  for (int j = 0; j < p->styles.size() && !found; j++) {
   if (p->styles[j] == styles[i])
    found = true;
  }
  if (!found)
   ret.push_back( styles[i] );
 }
 return ret;
}


int npc::minutes_to_u(game *g)
{
 int ret = abs(mapx - g->levx);
 if (abs(mapy - g->levy) < ret)
  ret = abs(mapy - g->levy);
 ret *= 24;
 ret /= 10;
 while (ret % 5 != 0)	// Round up to nearest five-minute interval
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
 for (int i = 0; i < num_needs; i++)
  needrank[i] = 20;
 if (weapon.is_gun()) {
  it_gun* gun = dynamic_cast<it_gun*>(weapon.type);
  needrank[need_ammo] = 5 * has_ammo(gun->ammo).size();
 }
 if (weapon.type->id == 0 && sklevel[sk_unarmed] < 4)
  needrank[need_weapon] = 1;
 else
  needrank[need_weapon] = weapon.type->melee_dam + weapon.type->melee_cut +
                          weapon.type->m_to_hit;
 if (!weapon.is_gun())
  needrank[need_gun] = sklevel[sk_unarmed] + sklevel[sk_melee] +
                       sklevel[sk_bashing] + sklevel[sk_cutting] -
                       sklevel[sk_gun] * 2 + 5;
 needrank[need_food] = 15 - hunger;
 needrank[need_drink] = 15 - thirst;
 for (int i = 0; i < inv.size(); i++) {
  it_comest* food = NULL;
  if (inv[i].is_food())
   food = dynamic_cast<it_comest*>(inv[i].type);
  else if (inv[i].is_food_container())
   food = dynamic_cast<it_comest*>(inv[i].contents[0].type);
  if (food != NULL) {
   needrank[need_food] += food->nutr / 4;
   needrank[need_drink] += food->quench / 4;
  }
 }
 needs.clear();
 int j;
 bool serious = false;
 for (int i = 1; i < num_needs; i++) {
  if (needrank[i] < 10)
   serious = true;
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
   if (j == needs.size())
    needs.push_back(npc_need(i));
  }
 }
}

void npc::say(game *g, std::string line, ...)
{
 va_list ap;
 va_start(ap, line);
 char buff[8192];
 vsprintf(buff, line.c_str(), ap);
 va_end(ap);
 line = buff;
 int junk;
 parse_tags(line, &(g->u), this);
 if (g->u_see(posx, posy, junk)) {
  g->add_msg("%s says, \"%s\"", name.c_str(), line.c_str());
  g->sound(posx, posy, 16, "");
 } else {
  std::string sound = name + " saying, \"" + line + "\"";
  g->sound(posx, posy, 16, sound);
 }
}

void npc::init_selling(std::vector<int> &indices, std::vector<int> &prices)
{
 int val, price;
 bool found_lighter = false;
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == itm_lighter && !found_lighter)
   found_lighter = true;
  else {
   val = value(inv[i]) - (inv[i].price() / 50);
   if (val <= NPC_LOW_VALUE || mission == NPC_MISSION_SHOPKEEP) {
    indices.push_back(i);
    price = inv[i].price() / (price_adjustment(sklevel[sk_barter]));
    prices.push_back(price);
   }
  }
 }
}

void npc::init_buying(inventory you, std::vector<int> &indices,
                      std::vector<int> &prices)
{
 int val, price;
 for (int i = 0; i < you.size(); i++) {
  val = value(you[i]);
  if (val >= NPC_HI_VALUE) {
   indices.push_back(i);
   price = you[i].price();
   if (val >= NPC_VERY_HI_VALUE)
    price *= 2;
   price *= price_adjustment(sklevel[sk_barter]);
   prices.push_back(price);
  }
 }
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
 for (int i = 0; i < inv.size(); i++) {
  int itval = value(inv[i]);
  if (itval < worst_item_value)
   worst_item_value = itval;
 }
}

int npc::value(item &it)
{
 int ret = it.price() / 50;
 skill best = best_skill();
 if (best != sk_unarmed) {
  int weapon_val = it.weapon_value(sklevel) - weapon.weapon_value(sklevel);
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
  for (int i = 0; i < inv.size(); i++) {
   if (inv[i].is_gun()) {
    gun = dynamic_cast<it_gun*>(inv[i].type);
    if (ammo->type == gun->ammo)
     ret += 14;
   }
  }
 }

 if (it.is_book()) {
  it_book* book = dynamic_cast<it_book*>(it.type);
  if (book->intel <= int_cur) {
   ret += book->fun;
   if (sklevel[book->type] < book->level && sklevel[book->type] >= book->req)
    ret += book->level * 3;
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
 if (fac_has_job(FACJOB_DOCTORS) && it.type->id >= itm_bandages &&
     it.type->id <= itm_prozac)
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
 for (int i = 0; i < inv.size(); i++) {
  if (inv[i].type->id == itm_bandages || inv[i].type->id == itm_1st_aid)
   return true;
 }
 return false;
}

bool npc::has_painkiller()
{
 for (int i = 0; i < inv.size(); i++) {
  if ((pain <= 35 && inv[i].type->id == itm_aspirin) ||
      (pain >= 50 && inv[i].type->id == itm_oxycodone) ||
      inv[i].type->id == itm_tramadol || inv[i].type->id == itm_codeine)
   return true;
 }
 return false;
}

bool npc::took_painkiller()
{
 return (has_disease(DI_PKILL1) || has_disease(DI_PKILL2) ||
         has_disease(DI_PKILL3) || has_disease(DI_PKILL_L));
}

bool npc::is_friend()
{
 if (attitude == NPCATT_FOLLOW || attitude == NPCATT_DEFEND ||
     attitude == NPCATT_LEAD)
  return true;
 return false;
}

bool npc::is_following()
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

bool npc::is_leader()
{
 return (attitude == NPCATT_LEAD);
}

bool npc::is_enemy()
{
 if (attitude == NPCATT_KILL || attitude == NPCATT_MUG ||
     attitude == NPCATT_FLEE)
  return true;
 return  false;
}

bool npc::is_defending()
{
 return (attitude == NPCATT_DEFEND);
}

int npc::danger_assessment(game *g)
{
 int ret = 0;
 int sightdist = g->light_level(), junk;
 for (int i = 0; i < g->z.size(); i++) {
  if (g->m.sees(posx, posy, g->z[i].posx, g->z[i].posy, sightdist, junk))
   ret += g->z[i].type->difficulty;
 }
 ret /= 10;
 if (ret <= 2)
  ret = -10 + 5 * ret;	// Low danger if no monsters around

// Mod for the player
 if (is_enemy()) {
  if (rl_dist(posx, posy, g->u.posx, g->u.posy) < 10) {
   if (g->u.weapon.is_gun())
    ret += 10;
   else 
    ret += 10 - rl_dist(posx, posy, g->u.posx, g->u.posy);
  }
 } else if (is_friend()) {
  if (rl_dist(posx, posy, g->u.posx, g->u.posy) < 8) {
   if (g->u.weapon.is_gun())
    ret -= 8;
   else 
    ret -= 8 - rl_dist(posx, posy, g->u.posx, g->u.posy);
  }
 }

 for (int i = 0; i < num_hp_parts; i++) {
  if (i == hp_head || i == hp_torso) {
        if (hp_cur[i] < hp_max[i] / 4)
    ret += 5;
   else if (hp_cur[i] < hp_max[i] / 2)
    ret += 3;
   else if (hp_cur[i] < hp_max[i] * .9)
    ret += 1;
  } else {
        if (hp_cur[i] < hp_max[i] / 4)
    ret += 2;
   else if (hp_cur[i] < hp_max[i] / 2)
    ret += 1;
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

void npc::told_to_help(game *g)
{
 if (!is_following() && personality.altruism < 0) {
  say(g, "Screw you!");
  return;
 }
 if (is_following()) {
  if (personality.altruism + 4 * op_of_u.value + personality.bravery >
      danger_assessment(g)) {
   say(g, "I've got your back!");
   attitude = NPCATT_DEFEND;
  }
  return;
 }
 if (int((personality.altruism + personality.bravery) / 4) >
     danger_assessment(g)) {
  say(g, "Alright, I got you covered!");
  attitude = NPCATT_DEFEND;
 }
}

void npc::told_to_wait(game *g)
{
 if (!is_following()) {
  debugmsg("%s told to wait, but isn't following", name.c_str());
  return;
 }
 if (5 + op_of_u.value + op_of_u.trust + personality.bravery * 2 >
     danger_assessment(g)) {
  say(g, "Alright, I'll wait here.");
  if (one_in(3))
   op_of_u.trust--;
  attitude = NPCATT_WAIT;
 } else {
  if (one_in(2))
   op_of_u.trust--;
  say(g, "No way, man!");
 }
}
 
void npc::told_to_leave(game *g)
{
 if (!is_following()) {
  debugmsg("%s told to leave, but isn't following", name.c_str());
  return;
 }
 if (danger_assessment(g) - personality.bravery > op_of_u.value) {
  say(g, "No way, I need you!");
  op_of_u.trust -= 2;
 } else {
  say(g, "Alright, see you later.");
  op_of_u.trust -= 2;
  op_of_u.value -= 1;
 }
}

int npc::follow_distance()
{
 return 4; // TODO: Modify based on bravery, weapon wielded, etc.
}

int npc::speed_estimate(int speed)
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

void npc::draw(WINDOW* w, int ux, int uy, bool inv)
{
 int x = SEEX + posx - ux;
 int y = SEEY + posy - uy;
 nc_color col = c_pink;
 if (attitude == NPCATT_KILL)
  col = c_red;
 if (is_friend())
  col = c_green;
 else if (is_following())
  col = c_ltgreen;
 if (inv)
  mvwputch_inv(w, y, x, col, '@');
 else
  mvwputch    (w, y, x, col, '@');
}

void npc::print_info(WINDOW* w)
{
// First line of w is the border; the next 4 are terrain info, and after that
// is a blank line. w is 13 characters tall, and we can't use the last one
// because it's a border as well; so we have lines 6 through 11.
// w is also 48 characters wide - 2 characters for border = 46 characters for us
 mvwprintz(w, 6, 1, c_white, "NPC: %s", name.c_str());
 mvwprintz(w, 7, 1, c_red, "Wielding %s%s", (weapon.type->id == 0 ? "" : "a "),
                                     weapon.tname().c_str());
 std::string wearing;
 std::stringstream wstr;
 wstr << "Wearing: ";
 for (int i = 0; i < worn.size(); i++) {
  if (i > 0)
   wstr << ", ";
  wstr << worn[i].tname();
 }
 wearing = wstr.str();
 int line = 8;
 size_t split;
 do {
  split = (wearing.length() <= 46) ? std::string::npos :
                                     wearing.find_last_of(' ', 46);
  if (split == std::string::npos)
   mvwprintz(w, line, 1, c_blue, wearing.c_str());
  else
   mvwprintz(w, line, 1, c_blue, wearing.substr(0, split).c_str());
  wearing = wearing.substr(split + 1);
  line++;
 } while (split != std::string::npos && line <= 11);
}

std::string npc::short_description()
{
 std::stringstream ret;
 ret << "Wielding " << weapon.tname() << ";   " << "Wearing: ";
 for (int i = 0; i < worn.size(); i++) {
  if (i > 0)
   ret << ", ";
  ret << worn[i].tname();
 }

 return ret.str();
}

std::string npc::opinion_text()
{
 std::stringstream ret;
 if (op_of_u.trust <= -10)
  ret << "Completely untrusting";
 else if (op_of_u.trust <= -6)
  ret << "Very untrusting";
 else if (op_of_u.trust <= -3)
  ret << "Untrusting";
 else if (op_of_u.trust <= 2)
  ret << "Uneasy";
 else if (op_of_u.trust <= 5)
  ret << "Trusting";
 else if (op_of_u.trust < 10)
  ret << "Very trusting";
 else
  ret << "Completely trusting";

 ret << " (Trust " << op_of_u.trust << "); ";

 if (op_of_u.fear <= -10)
  ret << "Thinks you're laughably harmless";
 else if (op_of_u.fear <= -6)
  ret << "Thinks you're harmless";
 else if (op_of_u.fear <= -3)
  ret << "Unafraid";
 else if (op_of_u.fear <= 2)
  ret << "Wary";
 else if (op_of_u.fear <= 5)
  ret << "Afraid";
 else if (op_of_u.fear < 10)
  ret << "Very afraid";
 else
  ret << "Terrified";

 ret << " (Fear " << op_of_u.fear << "); ";

 if (op_of_u.value <= -10)
  ret << "Considers you a major liability";
 else if (op_of_u.value <= -6)
  ret << "Considers you a burden";
 else if (op_of_u.value <= -3)
  ret << "Considers you an annoyance";
 else if (op_of_u.value <= 2)
  ret << "Doesn't care about you";
 else if (op_of_u.value <= 5)
  ret << "Values your presence";
 else if (op_of_u.value < 10)
  ret << "Treasures you";
 else
  ret << "Best Friends Forever!";

 ret << " (Value " << op_of_u.value << "); ";

 if (op_of_u.anger <= -10)
  ret << "You can do no wrong!";
 else if (op_of_u.anger <= -6)
  ret << "You're good people";
 else if (op_of_u.anger <= -3)
  ret << "Thinks well of you";
 else if (op_of_u.anger <= 2)
  ret << "Ambivalent";
 else if (op_of_u.anger <= 5)
  ret << "Pissed off";
 else if (op_of_u.anger < 10)
  ret << "Angry";
 else
  ret << "About to kill you";

 ret << " (Anger " << op_of_u.anger << ")";

 return ret.str();
}

void npc::shift(int sx, int sy)
{
 posx -= sx * SEEX;
 posy -= sy * SEEY;
 mapx += sx;
 mapy += sy;
 itx -= sx * SEEX;
 ity -= sy * SEEY;
 plx -= sx * SEEX;
 ply -= sy * SEEY;
 path.clear();
}

void npc::die(game *g, bool your_fault)
{
 if (dead)
  return;
 dead = true;
 int j;
 if (g->u_see(posx, posy, j))
  g->add_msg("%s dies!", name.c_str());
 if (your_fault && !g->u.has_trait(PF_HEARTLESS)) {
  if (is_friend())
   g->u.add_morale(MORALE_KILLED_FRIEND, -500);
  else if (!is_enemy())
   g->u.add_morale(MORALE_KILLED_INNOCENT, -100);
 }

 item my_body;
 my_body.make_corpse(g->itypes[itm_corpse], g->mtypes[mon_null], g->turn);
 my_body.name = name;
 g->m.add_item(posx, posy, my_body);
 for (int i = 0; i < inv.size(); i++)
  g->m.add_item(posx, posy, inv[i]);
 for (int i = 0; i < worn.size(); i++)
  g->m.add_item(posx, posy, worn[i]);
 if (weapon.type->id != itm_null)
  g->m.add_item(posx, posy, weapon);

 for (int i = 0; i < g->active_missions.size(); i++) {
  if (g->active_missions[i].npc_id == id)
   g->fail_mission( g->active_missions[i].uid );
 }
  
}

std::string npc_attitude_name(npc_attitude att)
{
 switch (att) {
 case NPCATT_NULL:	// Don't care/ignoring player
  return "Ignoring";
 case NPCATT_TALK:		// Move to and talk to player
  return "Wants to talk";
 case NPCATT_TRADE:		// Move to and trade with player
  return "Wants to trade";
 case NPCATT_FOLLOW:		// Follow the player
  return "Following";
 case NPCATT_FOLLOW_RUN:	// Follow the player, don't shoot monsters
  return "Following & ignoring monsters";
 case NPCATT_LEAD:		// Lead the player, wait for them if they're behind
  return "Leading";
 case NPCATT_WAIT:		// Waiting for the player
  return "Waiting for you";
 case NPCATT_DEFEND:		// Kill monsters that threaten the player
  return "Defending you";
 case NPCATT_MUG:		// Mug the player
  return "Mugging you";
 case NPCATT_WAIT_FOR_LEAVE:	// Attack the player if our patience runs out
  return "Waiting for you to leave";
 case NPCATT_KILL:		// Kill the player
  return "Attacking to kill";
 case NPCATT_FLEE:		// Get away from the player
  return "Fleeing";
 case NPCATT_SLAVE:		// Following the player under duress
  return "Enslaved";
 case NPCATT_HEAL:		// Get to the player and heal them
  return "Healing you";

 case NPCATT_MISSING:	// Special; missing NPC as part of mission
  return "Missing NPC";
 case NPCATT_KIDNAPPED:	// Special; kidnapped NPC as part of mission
  return "Kidnapped";
 default:
  return "Unknown";
 }
 return "Unknown";
}

std::string npc_class_name(npc_class classtype)
{
 switch(classtype) {
 case NC_NONE:
  return "No class";
 case NC_SHOPKEEP:	// Found in towns.  Stays in his shop mostly.
  return "Shopkeep";
 case NC_HACKER:	// Weak in combat but has hacking skills and equipment
  return "Hacker";
 case NC_DOCTOR:	// Found in towns, or roaming.  Stays in the clinic.
  return "Doctor";
 case NC_TRADER:	// Roaming trader, journeying between towns.
  return "Trader";
 case NC_NINJA:	// Specializes in unarmed combat, carries few items
  return "Ninja";
 case NC_COWBOY:	// Gunslinger and survivalist
  return "Cowboy";
 case NC_SCIENTIST:	// Uses intelligence-based skills and high-tech items
  return "Scientist";
 case NC_BOUNTY_HUNTER: // Resourceful and well-armored
  return "Bounty Hunter";
 }
 return "Unknown class";
}
