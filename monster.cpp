#include "monster.h"
#include "map.h"
#include "mondeath.h"
#include "output.h"
#include "game.h"
#include "rng.h"
#include "item.h"
#include "item_factory.h"
#include "translations.h"
#include <sstream>
#include <fstream>
#include <stdlib.h>
#include <algorithm>
#include "cursesdef.h"

#include "picofunc.h"

#define SGN(a) (((a)<0) ? -1 : 1)
#define SQR(a) ((a)*(a))

monster::monster()
{
 _posx = 20;
 _posy = 10;
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
 no_extra_death_drops = false;
 dead = false;
 made_footstep = false;
 unique_name = "";
 hallucination = false;
}

monster::monster(mtype *t)
{
 _posx = 20;
 _posy = 10;
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
 no_extra_death_drops = false;
 dead = false;
 made_footstep = false;
 unique_name = "";
 hallucination = false;
}

monster::monster(mtype *t, int x, int y)
{
 _posx = x;
 _posy = y;
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
 no_extra_death_drops = false;
 dead = false;
 made_footstep = false;
 unique_name = "";
 hallucination = false;
}

monster::~monster()
{
}

bool monster::setpos(const int x, const int y, const bool level_change)
{
    bool ret = level_change ? true : g->update_zombie_pos(*this, x, y);
    _posx = x;
    _posy = y;
    return ret;
}

bool monster::setpos(const point &p, const bool level_change)
{
    return setpos(p.x, p.y, level_change);
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
    _posx = x;
    _posy = y;
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

// MATERIALS-TODO: put description in materials.json?
std::string monster::name_with_armor()
{
 std::string ret;
 if (type->species == species_insect) {
     ret = string_format(_("%s's carapace"), type->name.c_str());
 }
 else {
     if (type->mat == "veggy") {
         ret = string_format(_("%s's thick bark"), type->name.c_str());
     } else if (type->mat == "flesh" || type->mat == "hflesh") {
         ret = string_format(_("%s's thick hide"), type->name.c_str());
     } else if (type->mat == "iron" || type->mat == "steel") {
         ret = string_format(_("%s's armor plating"), type->name.c_str());
     }
 }
 return ret;
}

void monster::print_info(game *g, WINDOW* w, int vStart)
{
// First line of w is the border; the next two are terrain info, and after that
// is a blank line. w is 13 characters tall, and we can't use the last one
// because it's a border as well; so we have lines 4 through 11.
// w is also 48 characters wide - 2 characters for border = 46 characters for us
// vStart added because 'help' text in targeting win makes helpful info hard to find
// at a glance.

 const int vEnd = vStart + 5; // TODO: parameterize this

 mvwprintz(w, vStart, 1, c_white, "%s ", type->name.c_str());
 switch (attitude(&(g->u))) {
  case MATT_FRIEND:
   wprintz(w, h_white, _("Friendly! "));
   break;
  case MATT_FLEE:
   wprintz(w, c_green, _("Fleeing! "));
   break;
  case MATT_IGNORE:
   wprintz(w, c_ltgray, _("Ignoring "));
   break;
  case MATT_FOLLOW:
   wprintz(w, c_yellow, _("Tracking "));
   break;
  case MATT_ATTACK:
   wprintz(w, c_red, _("Hostile! "));
   break;
  default:
   wprintz(w, h_red, "BUG: Behavior unnamed. (monster.cpp:print_info)");
   break;
 }
 if (has_effect(ME_DOWNED))
  wprintz(w, h_white, _("On ground"));
 else if (has_effect(ME_STUNNED))
  wprintz(w, h_white, _("Stunned"));
 else if (has_effect(ME_BEARTRAP))
  wprintz(w, h_white, _("Trapped"));
 std::string damage_info;
 nc_color col;
 if (hp >= type->hp) {
  damage_info = _("It is uninjured");
  col = c_green;
 } else if (hp >= type->hp * .8) {
  damage_info = _("It is lightly injured");
  col = c_ltgreen;
 } else if (hp >= type->hp * .6) {
  damage_info = _("It is moderately injured");
  col = c_yellow;
 } else if (hp >= type->hp * .3) {
  damage_info = _("It is heavily injured");
  col = c_yellow;
 } else if (hp >= type->hp * .1) {
  damage_info = _("It is severly injured");
  col = c_ltred;
 } else {
  damage_info = _("it is nearly dead");
  col = c_red;
 }
 mvwprintz(w, vStart+1, 1, col, damage_info.c_str());

    int line = vStart + 2;
    std::vector<std::string> lines = foldstring(type->description, getmaxx(w) - 2);
    int numlines = lines.size();
    for (int i = 0; i < numlines && line <= vEnd; i++, line++)
        mvwprintz(w, line, 1, c_white, lines[i].c_str());
}

char monster::symbol()
{
 return type->sym;
}

void monster::draw(WINDOW *w, int plx, int ply, bool inv)
{
 int x = getmaxx(w)/2 + posx() - plx;
 int y = getmaxy(w)/2 + posy() - ply;
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
 return type->has_flag(f);
}

bool monster::can_see()
{
 return has_flag(MF_SEES) && !has_effect(ME_BLIND);
}

bool monster::can_hear()
{
 return has_flag(MF_HEARS) && !has_effect(ME_DEAF);
}

bool monster::can_submerge()
{
  return (has_flag(MF_NO_BREATHE) || has_flag(MF_SWIMS) || has_flag(MF_AQUATIC))
          && !has_flag(MF_ELECTRONIC);
}

bool monster::can_drown()
{
 return !has_flag(MF_SWIMS) && !has_flag(MF_AQUATIC)
         && !has_flag(MF_NO_BREATHE) && !has_flag(MF_FLIES);
}

bool monster::made_of(std::string m)
{
 if (type->mat == m)
  return true;
 return false;
}

bool monster::made_of(phase_id p)
{
 if (type->phase == p)
  return true;
 return false;
}

void monster::load_info(std::string data, std::vector <mtype *> *mtypes)
{
    std::stringstream dump;
    int idtmp, plansize;
    dump << data;
    if ( dump.peek() == '{' ) {
        picojson::value pdata;
        dump >> pdata;
        std::string jsonerr = picojson::get_last_error();
        if ( ! jsonerr.empty() ) {
            debugmsg("Bad monster json\n%s", jsonerr.c_str() );
        } else {
            json_load(pdata, mtypes);
        }
        return;
    }

    dump >> idtmp >> _posx >> _posy >> wandx >> wandy >> wandf >> moves >> speed >>
         hp >> sp_timeout >> plansize >> friendly >> faction_id >> mission_id >>
         no_extra_death_drops >> dead >> anger >> morale;
    type = (*mtypes)[idtmp];
    point ptmp;
    plans.clear();
    for (int i = 0; i < plansize; i++) {
        dump >> ptmp.x >> ptmp.y;
        plans.push_back(ptmp);
    }
}

bool monster::json_load(picojson::value parsed, std::vector <mtype *> *mtypes)
{

    const picojson::object &data = parsed.get<picojson::object>();

    int idtmp;
    picoint(data, "typeid", idtmp);
    type = (*mtypes)[idtmp];

    picoint(data, "posx", _posx);
    picoint(data, "posy", _posy);
    picoint(data, "wandx", wandx);
    picoint(data, "wandy", wandy);
    picoint(data, "wandf", wandf);
    picoint(data, "moves", moves);
    picoint(data, "speed", speed);
    picoint(data, "hp", hp);
    picoint(data, "sp_timeout", sp_timeout);
    picoint(data, "friendly", friendly);
    picoint(data, "faction_id", faction_id);
    picoint(data, "mission_id", mission_id);
    picobool(data, "no_extra_death_drops", no_extra_death_drops);
    picobool(data, "dead", dead);
    picoint(data, "anger", anger);
    picoint(data, "morale", morale);

    plans.clear();
    picojson::object::const_iterator pvplans_it = data.find("plans");
    if ( pvplans_it != data.end() ) {
        const picojson::array &pvplans = pvplans_it->second.get<picojson::array>();
        for( picojson::array::const_iterator pvpoint = pvplans.begin(); pvpoint != pvplans.end(); ++pvpoint) {
            if ( ! (*pvpoint).is<picojson::array>() ) {
                continue;
            }
            point ptmp;
            if ( ! (*pvpoint).get<picojson::array>()[0].is<double>()  ||
                 ! (*pvpoint).get<picojson::array>()[1].is<double>() ) {
                continue;
            }
            ptmp.x = int ( (*pvpoint).get<picojson::array>()[0].get<double>() );
            ptmp.y = int ( (*pvpoint).get<picojson::array>()[1].get<double>() );
            plans.push_back(ptmp);
        }
    }
    return true;
}

void monster::json_load(picojson::value parsed, game * g) {
    std::vector <mtype *> *mt=&(g->mtypes);
    json_load(parsed, mt);
}

/*
 * Save, json ed; serialization that won't break as easily. In theory.
 */
picojson::value monster::json_save(bool save_contents)
{
    std::map<std::string, picojson::value> data;
    data["typeid"] = pv(int(type->id));
    data["posx"] = pv(_posx);
    data["posy"] = pv(_posy);
    data["wandx"] = pv(wandx);
    data["wandy"] = pv(wandy);
    data["wandf"] = pv(wandf);
    data["moves"] = pv(moves);
    data["speed"] = pv(speed);
    data["hp"] = pv(hp);
    data["sp_timeout"] = pv(sp_timeout);
    data["friendly"] = pv(friendly);
    data["faction_id"] = pv(faction_id);
    data["mission_id"] = pv(mission_id);
    data["no_extra_death_drops"] = pv( no_extra_death_drops );
    data["dead"] = pv(dead);
    data["anger"] = pv(anger);
    data["morale"] = pv(morale);

    if ( plans.size() > 0 ) {
        std::vector<picojson::value> pvplans;
        for(int i = 0; i < plans.size(); i++) {
            std::vector<picojson::value> pvpoint;
            pvpoint.push_back( pv( (int)plans[i].x ) );
            pvpoint.push_back( pv( (int)plans[i].y ) );
            pvplans.push_back( pv ( pvpoint ) );
        }
        data["plans"] = pv( pvplans );
    }

    if ( save_contents ) {
        std::vector<picojson::value> pinv;
        for(int i=0;i<inv.size();i++) {
            pinv.push_back( pv( inv[i].json_save(true)  ) );
        }
        data["inv"] = pv(pinv);
    }

    return picojson::value(data);
}

/*
 * save serialized monster data to a line.
 * This is useful after player.sav is fully jsonized, to save full static spawns in maps.txt 
 */
std::string monster::save_info()
{
    return json_save().serialize();
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
 _posx -= sx * SEEX;
 _posy -= sy * SEEY;
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
         (att == MATT_FOLLOW && rl_dist(_posx, _posy, u.posx, u.posy) <= 4));
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

  if (((type->species == species_mammal && u->has_trait("PHEROMONE_MAMMAL")) ||
       (type->species == species_insect && u->has_trait("PHEROMONE_INSECT")))&&
      effective_anger >= 10)
   effective_anger -= 20;

  if (u->has_trait("TERRIFYING"))
   effective_morale -= 10;

  if (u->has_trait("ANIMALEMPATH") && has_flag(MF_ANIMAL)) {
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

 if (effective_anger <= 0)
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
  case MTRIG_STALK:
   if (anger > 0 && one_in(20))
    ret++;
   break;

  case MTRIG_MEAT:
   check_terrain = true;
   check_meat = true;
   break;

  case MTRIG_PLAYER_CLOSE:
   if (rl_dist(_posx, _posy, g->u.posx, g->u.posy) <= 5)
    ret += 5;
   for (int i = 0; i < g->active_npc.size(); i++) {
    if (rl_dist(_posx, _posy, g->active_npc[i]->posx, g->active_npc[i]->posy) <= 5)
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
  for (int x = _posx - 3; x <= _posx + 3; x++) {
   for (int y = _posy - 3; y <= _posy + 3; y++) {
    if (check_meat) {
     std::vector<item> *items = &(g->m.i_at(x, y));
     for (int n = 0; n < items->size(); n++) {
      if ((*items)[n].type->id == "corpse" ||
          (*items)[n].type->id == "meat" ||
          (*items)[n].type->id == "meat_cooked" ||
          (*items)[n].type->id == "human_flesh") {
       ret += 3;
       check_meat = false;
      }
     }
    }
    if (check_fire) {
     if (g->m.field_at(x, y).findField(fd_fire))
      ret += 5 * g->m.field_at(x, y).findField(fd_fire)->getFieldDensity();
    }
   }
  }
  if (check_fire) {
   if (g->u.has_amount("torch_lit", 1))
    ret += 49;
  }
 }

 return ret;
}

int monster::hit(game *g, player &p, body_part &bp_hit) {
 int ret = 0;
 int highest_hit = 0;
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
 monster* target = &(g->zombie(i));
 moves -= 100;

 if (this == target) {
  return;
 }

 int numdice = type->melee_skill;
 int dodgedice = target->dodge() * 2;
 switch (target->type->size) {
  case MS_TINY:  dodgedice += 6; break;
  case MS_SMALL: dodgedice += 3; break;
  case MS_LARGE: dodgedice -= 2; break;
  case MS_HUGE:  dodgedice -= 4; break;
 }

 if (dice(numdice, 10) <= dice(dodgedice, 10)) {
  if (g->u_see(this))
   g->add_msg(_("The %s misses the %s!"), name().c_str(), target->name().c_str());
  return;
 }
 if (g->u_see(this))
  g->add_msg(_("The %s hits the %s!"), name().c_str(), target->name().c_str());
 int damage = dice(type->melee_dice, type->melee_sides);
 if (target->hurt(damage))
  g->kill_mon(i, (friendly != 0));
}


bool monster::hurt(int dam, int real_dam)
{
 hp -= dam;
 if( real_dam > 0 ) {
     hp = std::max( hp, -real_dam );
 }
 if (hp < 1) {
     return true;
 }
 if (dam > 0) {
     process_trigger(MTRIG_HURT, 1 + int(dam / 3));
 }
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
 if (!no_extra_death_drops) {
  drop_items_on_death(g);
 }

// If we're a queen, make nearby groups of our type start to die out
 if (has_flag(MF_QUEEN)) {
// Do it for overmap above/below too
  for(int z = 0; z >= -1; --z) {
      for (int x = -MAPSIZE/2; x <= MAPSIZE/2; x++)
      {
          for (int y = -MAPSIZE/2; y <= MAPSIZE/2; y++)
          {
                 std::vector<mongroup*> groups =
                     g->cur_om->monsters_at(g->levx+x, g->levy+y, z);
                 for (int i = 0; i < groups.size(); i++) {
                     if (MonsterGroupManager::IsMonsterInGroup
                         (groups[i]->type, mon_id(type->id)))
                         groups[i]->dying = true;
                 }
          }
      }
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
 if(is_hallucination()) {
   //Hallucinations always just disappear
   md.disappear(g, this);
   return;
 } else {
   (md.*type->dies)(g, this);
 }
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
  for (int i = 0; i < g->num_zombies(); i++) {
   int t = 0;
   if (g->m.sees(g->zombie(i).posx(), g->zombie(i).posy(), _posx, _posy, light, t)) {
    g->zombie(i).morale += morale_adjust;
    g->zombie(i).anger += anger_adjust;
   }
  }
 }
}

void monster::drop_items_on_death(game *g)
{
    if(is_hallucination()) {
        return;
    }
    int total_chance = 0, cur_chance, selected_location;
    bool animal_done = false;
    std::vector<items_location_and_chance> it = g->monitems[type->id];
    if (type->item_chance != 0 && it.size() == 0)
    {
        debugmsg("Type %s has item_chance %d but no items assigned!",
                 type->name.c_str(), type->item_chance);
        return;
    }

    for (int i = 0; i < it.size(); i++)
    {
        total_chance += it[i].chance;
    }

    while (rng(0, 99) < abs(type->item_chance) && !animal_done)
    {
        cur_chance = rng(1, total_chance);
        selected_location = -1;
        while (cur_chance > 0)
        {
            selected_location++;
            cur_chance -= it[selected_location].chance;
        }

        // We have selected a string representing an item group, now
        // get a random item tag from it and spawn it.
        Item_tag selected_item = item_controller->id_from(it[selected_location].loc);
        g->m.spawn_item(_posx, _posy, selected_item, 0);

        if (type->item_chance < 0)
        {
            animal_done = true; // Only drop ONE item.
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

// MATERIALS-TODO: use fire resistance
  case ME_ONFIRE:
   if (made_of("flesh"))
    hurt(rng(3, 8));
   if (made_of("veggy"))
    hurt(rng(10, 20));
   if (made_of("paper") || made_of("powder") || made_of("wood") || made_of("cotton") ||
       made_of("wool"))
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

bool monster::is_hallucination()
{
  return hallucination;
}
