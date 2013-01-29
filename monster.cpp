#include "monster.h"
#include "map.h"
#include "mondeath.h"
#include "output.h"
#include "game.h"
#include "rng.h"
#include "item.h"
#include <sstream>
#include <fstream>
#include <stdlib.h>

#if (defined _WIN32 || defined WINDOWS)
	#include "catacurse.h"
#elif (defined __CYGWIN__)
      #include "ncurses/curses.h"
#else
	#include <curses.h>
#endif

#define SGN(a) (((a)<0) ? -1 : 1)
#define SQR(a) ((a)*(a))

monster::monster()
{
 posx = 20;
 posy = 10;
 wandx = -1;
 wandy = -1;
 wandf = 0;
 hp = 60;
 moves = 0;
 sp_timeout = 0;
 spawnmapx = -1;
 spawnmapy = -1;
 spawnposx = -1;
 spawnposy = -1;
 friendly = 0;
 anger = 0;
 morale = 2;
 faction_id = -1;
 mission_id = -1;
 dead = false;
 made_footstep = false;
 unique_name = "";
}

monster::monster(mtype *t)
{
 posx = 20;
 posy = 10;
 wandx = -1;
 wandy = -1;
 wandf = 0;
 type = t;
 moves = type->speed;
 speed = type->speed;
 hp = type->hp;
 sp_timeout = rng(0, type->sp_freq);
 spawnmapx = -1;
 spawnmapy = -1;
 spawnposx = -1;
 spawnposy = -1;
 friendly = 0;
 anger = t->agro;
 morale = t->morale;
 faction_id = -1;
 mission_id = -1;
 dead = false;
 made_footstep = false;
 unique_name = "";
}

monster::monster(mtype *t, int x, int y)
{
 posx = x;
 posy = y;
 wandx = -1;
 wandy = -1;
 wandf = 0;
 type = t;
 moves = type->speed;
 speed = type->speed;
 hp = type->hp;
 sp_timeout = type->sp_freq;
 spawnmapx = -1;
 spawnmapy = -1;
 spawnposx = -1;
 spawnposy = -1;
 friendly = 0;
 anger = type->agro;
 morale = type->morale;
 faction_id = -1;
 mission_id = -1;
 dead = false;
 made_footstep = false;
 unique_name = "";
}

monster::~monster()
{
}

void monster::poly(mtype *t)
{
 double hp_percentage = double(hp) / double(type->hp);
 type = t;
 moves = 0;
 speed = type->speed;
 anger = type->agro;
 morale = type->morale;
 hp = int(hp_percentage * type->hp);
 sp_timeout = type->sp_freq;
}

void monster::spawn(int x, int y)
{
 posx = x;
 posy = y;
}

std::string monster::name()
{
 if (!type) {
  debugmsg ("monster::name empty type!");
  return std::string();
 }
 if (unique_name != "")
  return type->name + ": " + unique_name;
 return type->name;
}

std::string monster::name_with_armor()
{
 std::string ret = type->name;
 if (type->species == species_insect)
  ret += "'s carapace";
 else {
  switch (type->mat) {
   case VEGGY: ret += "'s thick bark";    break;
   case FLESH: ret += "'s thick hide";    break;
   case IRON:
   case STEEL: ret += "'s armor plating"; break;
  }
 }
 return ret;
}

void monster::print_info(game *g, WINDOW* w)
{
// First line of w is the border; the next two are terrain info, and after that
// is a blank line. w is 13 characters tall, and we can't use the last one
// because it's a border as well; so we have lines 4 through 11.
// w is also 48 characters wide - 2 characters for border = 46 characters for us
 mvwprintz(w, 6, 1, c_white, "%s ", type->name.c_str());
 switch (attitude(&(g->u))) {
  case MATT_FRIEND:
   wprintz(w, h_white, "Friendly! ");
   break;
  case MATT_FLEE:
   wprintz(w, c_green, "Fleeing! ");
   break;
  case MATT_IGNORE:
   wprintz(w, c_ltgray, "Ignoring ");
   break;
  case MATT_FOLLOW:
   wprintz(w, c_yellow, "Tracking ");
   break;
  case MATT_ATTACK:
   wprintz(w, c_red, "Hostile! ");
   break;
  default:
   wprintz(w, h_red, "BUG: Behavior unnamed ");
   break;
 }
 if (has_effect(ME_DOWNED))
  wprintz(w, h_white, "On ground");
 else if (has_effect(ME_STUNNED))
  wprintz(w, h_white, "Stunned");
 else if (has_effect(ME_BEARTRAP))
  wprintz(w, h_white, "Trapped");
 std::string damage_info;
 nc_color col;
 if (hp == type->hp) {
  damage_info = "It is uninjured";
  col = c_green;
 } else if (hp >= type->hp * .8) {
  damage_info = "It is lightly injured";
  col = c_ltgreen;
 } else if (hp >= type->hp * .6) {
  damage_info = "It is moderately injured";
  col = c_yellow;
 } else if (hp >= type->hp * .3) {
  damage_info = "It is heavily injured";
  col = c_yellow;
 } else if (hp >= type->hp * .1) {
  damage_info = "It is severly injured";
  col = c_ltred;
 } else {
  damage_info = "it is nearly dead";
  col = c_red;
 }
 mvwprintz(w, 7, 1, col, damage_info.c_str());

 std::string tmp = type->description;
 std::string out;
 size_t pos;
 int line = 8;
 do {
  pos = tmp.find_first_of('\n');
  out = tmp.substr(0, pos);
  mvwprintz(w, line, 1, c_white, out.c_str());
  tmp = tmp.substr(pos + 1);
  line++;
 } while (pos != std::string::npos && line < 12);
}

char monster::symbol()
{
 return type->sym;
}

void monster::draw(WINDOW *w, int plx, int ply, bool inv)
{
 int x = SEEX + posx - plx;
 int y = SEEY + posy - ply;
 nc_color color = type->color;
 if (friendly != 0 && !inv)
  mvwputch_hi(w, y, x, color, type->sym);
 else if (inv)
  mvwputch_inv(w, y, x, color, type->sym);
 else {
  color = color_with_effects();
  mvwputch(w, y, x, color, type->sym);
 }
}

nc_color monster::color_with_effects()
{
 nc_color ret = type->color;
 if (has_effect(ME_BEARTRAP) || has_effect(ME_STUNNED) || has_effect(ME_DOWNED))
  ret = hilite(ret);
 if (has_effect(ME_ONFIRE))
  ret = red_background(ret);
 return ret;
}

bool monster::has_flag(m_flag f)
{
 for (int i = 0; i < type->flags.size(); i++) {
  if (type->flags[i] == f)
   return true;
 }
 return false;
}

bool monster::can_see()
{
 return has_flag(MF_SEES) && !has_effect(ME_BLIND);
}

bool monster::can_hear()
{
 return has_flag(MF_HEARS) && !has_effect(ME_DEAF);
}

bool monster::made_of(material m)
{
 if (type->mat == m)
  return true;
 return false;
}
 
void monster::load_info(std::string data, std::vector <mtype*> *mtypes)
{
 std::stringstream dump;
 int idtmp, plansize;
 dump << data;
 dump >> idtmp >> posx >> posy >> wandx >> wandy >> wandf >> moves >> speed >>
         hp >> sp_timeout >> plansize >> friendly >> faction_id >> mission_id >>
         dead >> anger >> morale;
 type = (*mtypes)[idtmp];
 point ptmp;
 for (int i = 0; i < plansize; i++) {
  dump >> ptmp.x >> ptmp.y;
  plans.push_back(ptmp);
 }
}

std::string monster::save_info()
{
 std::stringstream pack;
 pack << int(type->id) << " " << posx << " " << posy << " " << wandx << " " <<
         wandy << " " << wandf << " " << moves << " " << speed << " " << hp <<
         " " << sp_timeout << " " << plans.size() << " " << friendly << " " <<
          faction_id << " " << mission_id << " " << dead << " " << anger <<
         " " << morale;
 for (int i = 0; i < plans.size(); i++) {
  pack << " " << plans[i].x << " " << plans[i].y;
 }
 return pack.str();
}

void monster::debug(player &u)
{
 char buff[2];
 debugmsg("%s has %d steps planned.", name().c_str(), plans.size());
 debugmsg("%s Moves %d Speed %d HP %d",name().c_str(), moves, speed, hp);
 for (int i = 0; i < plans.size(); i++) {
  sprintf(buff, "%d", i);
  if (i < 10) mvaddch(plans[i].y - SEEY + u.posy, plans[i].x - SEEX + u.posx,
                      buff[0]);
  else mvaddch(plans[i].y - SEEY + u.posy, plans[i].x - SEEX + u.posx, buff[1]);
 }
 getch();
}

void monster::shift(int sx, int sy)
{
 posx -= sx * SEEX;
 posy -= sy * SEEY;
 for (int i = 0; i < plans.size(); i++) {
  plans[i].x -= sx * SEEX;
  plans[i].y -= sy * SEEY;
 }
}

bool monster::is_fleeing(player &u)
{
 if (has_effect(ME_RUN))
  return true;
 monster_attitude att = attitude(&u);
 return (att == MATT_FLEE ||
         (att == MATT_FOLLOW && rl_dist(posx, posy, u.posx, u.posy) <= 4));
}

monster_attitude monster::attitude(player *u)
{
 if (friendly != 0)
  return MATT_FRIEND;
 if (has_effect(ME_RUN))
  return MATT_FLEE;

 int effective_anger  = anger;
 int effective_morale = morale;

 if (u != NULL) {

  if (((type->species == species_mammal && u->has_trait(PF_PHEROMONE_MAMMAL)) ||
       (type->species == species_insect && u->has_trait(PF_PHEROMONE_INSECT)))&&
      effective_anger >= 10)
   effective_anger -= 20;

  if (u->has_trait(PF_TERRIFYING))
   effective_morale -= 10;

  if (u->has_trait(PF_ANIMALEMPATH) && has_flag(MF_ANIMAL)) {
   if (effective_anger >= 10)
    effective_anger -= 10;
   if (effective_anger < 10)
    effective_morale += 5;
  }

 }

 if (effective_morale < 0) {
  if (effective_morale + effective_anger > 0)
   return MATT_FOLLOW;
  return MATT_FLEE;
 }

 if (effective_anger < 0)
  return MATT_IGNORE;

 if (effective_anger < 10)
  return MATT_FOLLOW;

 return MATT_ATTACK;
}

void monster::process_triggers(game *g)
{
 anger += trigger_sum(g, &(type->anger));
 anger -= trigger_sum(g, &(type->placate));
 if (morale < 0) {
  if (morale < type->morale && one_in(20))
  morale++;
 } else
  morale -= trigger_sum(g, &(type->fear));
}

// This Adjustes anger/morale levels given a single trigger.
void monster::process_trigger(monster_trigger trig, int amount)
{
 for (int i = 0; i < type->anger.size(); i++) {
  if (type->anger[i] == trig)
   anger += amount;
 }
 for (int i = 0; i < type->placate.size(); i++) {
  if (type->placate[i] == trig)
   anger -= amount;
 }
 for (int i = 0; i < type->fear.size(); i++) {
  if (type->fear[i] == trig)
   morale -= amount;
 }
}


int monster::trigger_sum(game *g, std::vector<monster_trigger> *triggers)
{
 int ret = 0;
 bool check_terrain = false, check_meat = false, check_fire = false;
 for (int i = 0; i < triggers->size(); i++) {

  switch ((*triggers)[i]) {
  case MTRIG_TIME:
   if (one_in(20))
    ret++;
   break;

  case MTRIG_MEAT:
   check_terrain = true;
   check_meat = true;
   break;

  case MTRIG_PLAYER_CLOSE:
   if (rl_dist(posx, posy, g->u.posx, g->u.posy) <= 5)
    ret += 5;
   for (int i = 0; i < g->active_npc.size(); i++) {
    if (rl_dist(posx, posy, g->active_npc[i].posx, g->active_npc[i].posy) <= 5)
     ret += 5;
   }
   break;

  case MTRIG_FIRE:
   check_terrain = true;
   check_fire = true;
   break;

  case MTRIG_PLAYER_WEAK:
   if (g->u.hp_percentage() <= 70)
    ret += 10 - int(g->u.hp_percentage() / 10);
   break;

  default:
   break; // The rest are handled when the impetus occurs
  }
 }

 if (check_terrain) {
  for (int x = posx - 3; x <= posx + 3; x++) {
   for (int y = posy - 3; y <= posy + 3; y++) {
    if (check_meat) {
     std::vector<item> *items = &(g->m.i_at(x, y));
     for (int n = 0; n < items->size(); n++) {
      if ((*items)[n].type->id == itm_corpse ||
          (*items)[n].type->id == itm_meat ||
          (*items)[n].type->id == itm_meat_tainted) {
       ret += 3;
       check_meat = false;
      }
     }
    }
    if (check_fire) {
     if (g->m.field_at(x, y).type == fd_fire)
      ret += 5 * g->m.field_at(x, y).density;
    }
   }
  }
  if (check_fire) {
   if (g->u.has_amount(itm_torch_lit, 1))
    ret += 49;
  }
 }

 return ret;
}

int monster::hit(game *g, player &p, body_part &bp_hit)
{
 int numdice = type->melee_skill;
 if (dice(numdice, 10) <= dice(p.dodge(g), 10) && !one_in(20)) {
  if (numdice > p.sklevel[sk_dodge])
   p.practice(sk_dodge, 10);
  return 0;	// We missed!
 }
 p.practice(sk_dodge, 5);
 int ret = 0;
 int highest_hit;
 switch (type->size) {
 case MS_TINY:
  highest_hit = 3;
 break;
 case MS_SMALL:
  highest_hit = 12;
 break;
 case MS_MEDIUM:
  highest_hit = 20;
 break;
 case MS_LARGE:
  highest_hit = 28;
 break;
 case MS_HUGE:
  highest_hit = 35;
 break;
 }

 if (has_flag(MF_DIGS))
  highest_hit -= 8;
 if (has_flag(MF_FLIES))
  highest_hit += 20;
 if (highest_hit <= 1)
  highest_hit = 2;
 if (highest_hit > 20)
  highest_hit = 20;

 int bp_rand = rng(0, highest_hit - 1);
      if (bp_rand <=  2)
  bp_hit = bp_legs;
 else if (bp_rand <= 10)
  bp_hit = bp_torso;
 else if (bp_rand <= 14)
  bp_hit = bp_arms;
 else if (bp_rand <= 16)
  bp_hit = bp_mouth;
 else if (bp_rand == 17)
  bp_hit = bp_eyes;
 else
  bp_hit = bp_head;
 ret += dice(type->melee_dice, type->melee_sides);
 return ret;
}

void monster::hit_monster(game *g, int i)
{
 int junk;
 monster* target = &(g->z[i]);
 
 int numdice = type->melee_skill;
 int dodgedice = target->dodge() * 2;
 switch (target->type->size) {
  case MS_TINY:  dodgedice += 6; break;
  case MS_SMALL: dodgedice += 3; break;
  case MS_LARGE: dodgedice -= 2; break;
  case MS_HUGE:  dodgedice -= 4; break;
 }

 if (dice(numdice, 10) <= dice(dodgedice, 10)) {
  if (g->u_see(this, junk))
   g->add_msg("The %s misses the %s!", name().c_str(), target->name().c_str());
  return;
 }
 if (g->u_see(this, junk))
  g->add_msg("The %s hits the %s!", name().c_str(), target->name().c_str());
 int damage = dice(type->melee_dice, type->melee_sides);
 if (target->hurt(damage))
  g->kill_mon(i, (friendly != 0));
}
 

bool monster::hurt(int dam)
{
 hp -= dam;
 if (hp < 1)
  return true;
 if (dam > 0)
  process_trigger(MTRIG_HURT, 1 + int(dam / 3));
 return false;
}

int monster::armor_cut()
{
// TODO: Add support for worn armor?
 return int(type->armor_cut);
}

int monster::armor_bash()
{
 return int(type->armor_bash);
}

int monster::dodge()
{
 if (has_effect(ME_DOWNED))
  return 0;
 int ret = type->sk_dodge;
 if (has_effect(ME_BEARTRAP))
  ret /= 2;
 if (moves <= 0 - 100 - type->speed)
  ret = rng(0, ret);
 return ret;
}

int monster::dodge_roll()
{
 int numdice = dodge();
 
 switch (type->size) {
  case MS_TINY:  numdice += 6; break;
  case MS_SMALL: numdice += 3; break;
  case MS_LARGE: numdice -= 2; break;
  case MS_HUGE:  numdice -= 4; break;
 }

 numdice += int(speed / 80);
 return dice(numdice, 10);
}

int monster::fall_damage()
{
 if (has_flag(MF_FLIES))
  return 0;
 switch (type->size) {
  case MS_TINY:   return rng(0, 4);  break;
  case MS_SMALL:  return rng(0, 6);  break;
  case MS_MEDIUM: return dice(2, 4); break;
  case MS_LARGE:  return dice(2, 6); break;
  case MS_HUGE:   return dice(3, 5); break;
 }

 return 0;
}

void monster::die(game *g)
{
 if (!dead)
  dead = true;
// Drop goodies
 int total_chance = 0, total_it_chance, cur_chance, selected_location,
     selected_item;
 bool animal_done = false;
 std::vector<items_location_and_chance> it = g->monitems[type->id];
 std::vector<itype_id> mapit;
 if (type->item_chance != 0 && it.size() == 0)
  debugmsg("Type %s has item_chance %d but no items assigned!",
           type->name.c_str(), type->item_chance);
 else {
  for (int i = 0; i < it.size(); i++)
   total_chance += it[i].chance;

  while (rng(0, 99) < abs(type->item_chance) && !animal_done) {
   cur_chance = rng(1, total_chance);
   selected_location = -1;
   while (cur_chance > 0) {
    selected_location++;
    cur_chance -= it[selected_location].chance;
   }
   total_it_chance = 0;
   mapit = g->mapitems[it[selected_location].loc];
   for (int i = 0; i < mapit.size(); i++)
    total_it_chance += g->itypes[mapit[i]]->rarity;
   cur_chance = rng(1, total_it_chance);
   selected_item = -1;
   while (cur_chance > 0) {
    selected_item++;
    cur_chance -= g->itypes[mapit[selected_item]]->rarity;
   }
   g->m.add_item(posx, posy, g->itypes[mapit[selected_item]], 0);
   if (type->item_chance < 0)
    animal_done = true;	// Only drop ONE item.
  }
 } // Done dropping items

// If we're a queen, make nearby groups of our type start to die out
 if (has_flag(MF_QUEEN)) {
  std::vector<mongroup*> groups = g->cur_om.monsters_at(g->levx, g->levy);
  for (int i = 0; i < groups.size(); i++) {
   moncat_id moncat_type = groups[i]->type;
   bool match = false;
   for (int j = 0; !match && j < g->moncats[moncat_type].size(); j++) {
    if (g->moncats[moncat_type][j] == type->id)
     match = true;
   }
   if (match)
    groups[i]->dying = true;
  }
// Do it for overmap above/below too
  overmap tmp;
  if (g->cur_om.posz == 0)
   tmp = overmap(g, g->cur_om.posx, g->cur_om.posy, -1);
  else
   tmp = overmap(g, g->cur_om.posx, g->cur_om.posy, 0);

  groups = tmp.monsters_at(g->levx, g->levy);
  for (int i = 0; i < groups.size(); i++) {
   moncat_id moncat_type = groups[i]->type;
   bool match = false;
   for (int j = 0; !match && j < g->moncats[moncat_type].size(); j++) {
    if (g->moncats[moncat_type][j] == type->id)
     match = true;
   }
   if (match)
    groups[i]->dying = true;
  }
 }
// If we're a mission monster, update the mission
 if (mission_id != -1) {
  mission_type *misstype = g->find_mission_type(mission_id);
  if (misstype->goal == MGOAL_FIND_MONSTER)
   g->fail_mission(mission_id);
  if (misstype->goal == MGOAL_KILL_MONSTER)
   g->mission_step_complete(mission_id, 1);
 }
// Also, perform our death function
 mdeath md;
 (md.*type->dies)(g, this);
// If our species fears seeing one of our own die, process that
 int anger_adjust = 0, morale_adjust = 0;
 for (int i = 0; i < type->anger.size(); i++) {
  if (type->anger[i] == MTRIG_FRIEND_DIED)
   anger_adjust += 15;
 }
 for (int i = 0; i < type->placate.size(); i++) {
  if (type->placate[i] == MTRIG_FRIEND_DIED)
   anger_adjust -= 15;
 }
 for (int i = 0; i < type->fear.size(); i++) {
  if (type->fear[i] == MTRIG_FRIEND_DIED)
   morale_adjust -= 15;
 }
 if (anger_adjust != 0 && morale_adjust != 0) {
  int light = g->light_level();
  for (int i = 0; i < g->z.size(); i++) {
   int t = 0;
   if (g->m.sees(g->z[i].posx, g->z[i].posy, posx, posy, light, t)) {
    g->z[i].morale += morale_adjust;
    g->z[i].anger += anger_adjust;
   }
  }
 }
}

void monster::add_effect(monster_effect_type effect, int duration)
{
 for (int i = 0; i < effects.size(); i++) {
  if (effects[i].type == effect) {
   effects[i].duration += duration;
   return;
  }
 }
 effects.push_back(monster_effect(effect, duration));
}

bool monster::has_effect(monster_effect_type effect)
{
 for (int i = 0; i < effects.size(); i++) {
  if (effects[i].type == effect)
   return true;
 }
 return false;
}

void monster::rem_effect(monster_effect_type effect)
{
 for (int i = 0; i < effects.size(); i++) {
  if (effects[i].type == effect) {
   effects.erase(effects.begin() + i);
   i--;
  }
 }
}

void monster::process_effects(game *g)
{
 for (int i = 0; i < effects.size(); i++) {
  switch (effects[i].type) {
  case ME_POISONED:
   speed -= rng(0, 3);
   hurt(rng(1, 3));
   break;

  case ME_ONFIRE:
   if (made_of(FLESH))
    hurt(rng(3, 8));
   if (made_of(VEGGY))
    hurt(rng(10, 20));
   if (made_of(PAPER) || made_of(POWDER) || made_of(WOOD) || made_of(COTTON) ||
       made_of(WOOL))
    hurt(rng(15, 40));
   break;

  }
  if (effects[i].duration > 0) {
   effects[i].duration--;
   if (g->debugmon)
    debugmsg("Duration %d", effects[i].duration);
  }
  if (effects[i].duration == 0) {
   if (g->debugmon)
    debugmsg("Deleting");
   effects.erase(effects.begin() + i);
   i--;
  }
 }
}

bool monster::make_fungus(game *g)
{
 switch (mon_id(type->id)) {
 case mon_ant:
 case mon_ant_soldier:
 case mon_ant_queen:
 case mon_fly:
 case mon_bee:
 case mon_dermatik:
  poly(g->mtypes[mon_ant_fungus]);
  return true;
 case mon_zombie:
 case mon_zombie_shrieker:
 case mon_zombie_electric:
 case mon_zombie_spitter:
 case mon_zombie_fast:
 case mon_zombie_brute:
 case mon_zombie_hulk:
  poly(g->mtypes[mon_zombie_fungus]);
  return true;
 case mon_boomer:
  poly(g->mtypes[mon_boomer_fungus]);
  return true;
 case mon_triffid:
 case mon_triffid_young:
 case mon_triffid_queen:
  poly(g->mtypes[mon_fungaloid]);
  return true;
 default:
  return true;
 }
 return false;
}

void monster::make_friendly()
{
 plans.clear();
 friendly = rng(5, 30) + rng(0, 20);
}

void monster::add_item(item it)
{
 inv.push_back(it);
}
