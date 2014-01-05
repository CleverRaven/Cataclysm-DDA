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
#include "monstergenerator.h"
#include "json.h"

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
 ignoring = 0;
 keep = 0;
 ammo = 100;
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
 size = type->size;
 Creature::set_speed_base(speed);
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
 ignoring = 0;
 ammo = 100;
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
 size = type->size;
 Creature::set_speed_base(speed);
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
 ignoring = 0;
 ammo = 100;
}

monster::~monster()
{
}

bool monster::setpos(const int x, const int y, const bool level_change)
{
    if (!level_change && x == _posx && y == _posy) {
        return true;
    }
    bool ret = level_change ? true : g->update_zombie_pos(*this, x, y);
    _posx = x;
    _posy = y;
    return ret;
}

bool monster::setpos(const point &p, const bool level_change)
{
    return setpos(p.x, p.y, level_change);
}

point monster::pos()
{
    return point(_posx, _posy);
}

void monster::poly(mtype *t)
{
 double hp_percentage = double(hp) / double(type->hp);
 type = t;
 moves = 0;
 speed = type->speed;
 Creature::set_speed_base(speed);
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
 if (type->in_species("INSECT")) {
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

std::string monster::disp_name() {
    char buffer[256];
    sprintf(buffer, _("the %s"), name().c_str());
    return buffer;
}

std::string monster::skin_name() {
    return name_with_armor();
}

void monster::get_HP_Bar(nc_color &color, std::string &text)
{
    ::get_HP_Bar(hp, type->hp, color, text, true);
}

void monster::get_Attitude(nc_color &color, std::string &text)
{
    switch (attitude(&(g->u))) {
        case MATT_FRIEND:
            color = h_white;
            text = _("Friendly ");
            break;
        case MATT_FLEE:
            color = c_green;
            text = _("Fleeing! ");
            break;
        case MATT_IGNORE:
            color = c_ltgray;
            text = _("Ignoring ");
            break;
        case MATT_FOLLOW:
            color = c_yellow;
            text = _("Tracking ");
            break;
        case MATT_ATTACK:
            color = c_red;
            text = _("Hostile! ");
            break;
        default:
            color = h_red;
            text = "BUG: Behavior unnamed. (monster.cpp:get_Attitude)";
            break;
    }
}

int monster::print_info(WINDOW* w, int vStart, int vLines, int column)
{
// First line of w is the border; the next two are terrain info, and after that
// is a blank line. w is 13 characters tall, and we can't use the last one
// because it's a border as well; so we have lines 4 through 11.
// w is also 48 characters wide - 2 characters for border = 46 characters for us
// vStart added because 'help' text in targeting win makes helpful info hard to find
// at a glance.

 const int vEnd = vStart + vLines;

 mvwprintz(w, vStart++, column, c_white, "%s ", type->name.c_str());
 nc_color color = c_white;
 std::string attitude = "";

 get_Attitude(color, attitude);
 wprintz(w, color, attitude.c_str());

 if (has_effect("downed"))
  wprintz(w, h_white, _("On ground"));
 else if (has_effect("stunned"))
  wprintz(w, h_white, _("Stunned"));
 else if (has_effect("beartrap"))
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
  damage_info = _("It is severely injured");
  col = c_ltred;
 } else {
  damage_info = _("it is nearly dead");
  col = c_red;
 }
 mvwprintz(w, vStart++, column, col, damage_info.c_str());

    std::vector<std::string> lines = foldstring(type->description, getmaxx(w) - 1 - column);
    int numlines = lines.size();
    for (int i = 0; i < numlines && vStart <= vEnd; i++)
        mvwprintz(w, vStart++, column, c_white, lines[i].c_str());

    return vStart;
}

char monster::symbol()
{
    return type->sym;
}

nc_color monster::basic_symbol_color()
{
    return type->color;
}

nc_color monster::symbol_color()
{
    return color_with_effects();
}

bool monster::is_symbol_highlighted()
{
    return (friendly != 0);
}

nc_color monster::color_with_effects()
{
 nc_color ret = type->color;
 if (has_effect("beartrap") || has_effect("stunned") || has_effect("downed"))
  ret = hilite(ret);
 if (has_effect("onfire"))
  ret = red_background(ret);
 return ret;
}

bool monster::has_flag(const m_flag f) const
{
 return type->has_flag(f);
}

bool monster::can_see()
{
 return has_flag(MF_SEES) && !has_effect("blind");
}

bool monster::can_hear()
{
 return has_flag(MF_HEARS) && !has_effect("deaf");
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

bool monster::digging()
{
    return has_flag(MF_DIGS) || (has_flag(MF_CAN_DIG) && g->m.has_flag("DIGGABLE", posx(), posy()));
}

int monster::vision_range(const int x, const int y) const
{
    int range = g->light_level();
    // Set to max possible value if the target is lit brightly
    if (g->m.light_at(x, y) >= LL_LOW)
        range = DAYLIGHT_LEVEL;

    if(has_flag(MF_VIS10)) {
        range -= 50;
    } else if(has_flag(MF_VIS20)) {
        range -= 40;
    } else if(has_flag(MF_VIS30)) {
        range -= 30;
    } else if(has_flag(MF_VIS40)) {
        range -= 20;
    } else if(has_flag(MF_VIS50)) {
        range -= 10;
    }
    range = std::max(range, 1);

    return range;
}

bool monster::sees_player(int & tc, player * p) const {
    if ( p == NULL ) {
        p = &g->u;
    }
    const int range = vision_range(p->posx, p->posy);
    // * p->visibility() / 100;
    return (
        g->m.sees( _posx, _posy, p->posx, p->posy, range, tc ) &&
        p->is_invisible() == false
    );
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

void monster::load_info(std::string data)
{
    std::stringstream dump;
    dump << data;
    if ( dump.peek() == '{' ) {
        JsonIn jsin(dump);
        try {
            deserialize(jsin);
        } catch (std::string jsonerr) {
            debugmsg("Bad monster json\n%s", jsonerr.c_str() );
        }
        return;
    } else {
        load_legacy(dump);
    }
}

/*
 * save serialized monster data to a line.
 * This is useful after player.sav is fully jsonized, to save full static spawns in maps.txt
 */
std::string monster::save_info()
{
    // saves contents
    return serialize();
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

point monster::move_target()
{
    if (plans.empty()) {
        // if we have no plans, pretend it's intentional
        return point(_posx, _posy);
    }
    return point(plans.back().x, plans.back().y);
}

bool monster::is_fleeing(player &u)
{
 if (has_effect("run"))
  return true;
 monster_attitude att = attitude(&u);
 return (att == MATT_FLEE ||
         (att == MATT_FOLLOW && rl_dist(_posx, _posy, u.posx, u.posy) <= 4));
}

monster_attitude monster::attitude(player *u)
{
 if (friendly != 0)
  return MATT_FRIEND;
 if (has_effect("run"))
  return MATT_FLEE;

 int effective_anger  = anger;
 int effective_morale = morale;

 if (u != NULL) {

  if (((type->in_species("MAMMAL") && u->has_trait("PHEROMONE_MAMMAL")) ||
       (type->in_species("INSECT") && u->has_trait("PHEROMONE_INSECT")))&&
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
  if (u->has_trait("ANIMALDISCORD") && has_flag(MF_ANIMAL)) {
   if (effective_anger >= 10)
    effective_anger += 10;
   if (effective_anger < 10)
    effective_morale -= 5;
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

void monster::process_triggers()
{
 anger += trigger_sum(&(type->anger));
 anger -= trigger_sum(&(type->placate));
 if (morale < 0) {
  if (morale < type->morale && one_in(20))
  morale++;
 } else
  morale -= trigger_sum(&(type->fear));
}

// This Adjustes anger/morale levels given a single trigger.
void monster::process_trigger(monster_trigger trig, int amount)
{
    if (type->has_anger_trigger(trig)){
        anger += amount;
    }
    if (type->has_fear_trigger(trig)){
        morale -= amount;
    }
    if (type->has_placate_trigger(trig)){
        anger -= amount;
    }
}


int monster::trigger_sum(std::set<monster_trigger> *triggers)
{
 int ret = 0;
 bool check_terrain = false, check_meat = false, check_fire = false;
 for (std::set<monster_trigger>::iterator trig = triggers->begin(); trig != triggers->end(); ++trig)
 {
     switch (*trig){
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
      ret += ( 5 * g->m.get_field_strength( point(x, y), fd_fire) );
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

bool monster::is_underwater() const {
    return false; //TODO: actually make this work
}

bool monster::is_on_ground() {
    return false; //TODO: actually make this work
}

bool monster::has_weapon() {
    return false; // monsters will never have weapons, silly
}

bool monster::is_warm() {
    return has_flag(MF_WARM);
}

bool monster::is_dead_state() {
    return hp <= 0;
}

bool monster::block_hit(body_part &, int &, damage_instance &) {
    return false;
}

void monster::absorb_hit(body_part, int, damage_instance &dam) {
    for (std::vector<damage_unit>::iterator it = dam.damage_units.begin();
            it != dam.damage_units.end(); ++it) {
        it->amount -= std::min(resistances(*this).get_effective_resist(*it),
                it->amount);
    }
}


int monster::hit(Creature &p, body_part &bp_hit) {
 int ret = 0;
 int highest_hit = 0;

    //If the player is knocked down or the monster can fly, any body part is a valid target
    if(p.is_on_ground() || has_flag(MF_FLIES)){
        highest_hit = 20;
    } else {
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
        if (digging()){
            highest_hit -= 8;
        }
        if (highest_hit <= 1){
            highest_hit = 2;
        }
    }

    if (highest_hit > 20){
        highest_hit = 20;
    }


    int bp_rand = rng(0, highest_hit - 1);
    if (bp_rand <=  2){
        bp_hit = bp_legs;
    } else if (bp_rand <= 10){
        bp_hit = bp_torso;
    } else if (bp_rand <= 14){
        bp_hit = bp_arms;
    } else if (bp_rand <= 16){
        bp_hit = bp_mouth;
    } else if (bp_rand == 17){
        bp_hit = bp_eyes;
    } else{
        bp_hit = bp_head;
    }
    ret += dice(type->melee_dice, type->melee_sides);
    return ret;
}


void monster::melee_attack(Creature &target, bool) {
    mod_moves(-100);
    if (type->melee_dice == 0) { // We don't attack, so just return
        return;
    }
    add_effect("hit_by_player", 3); // Make us a valid target for a few turns

    if (has_flag(MF_HIT_AND_RUN)) {
        add_effect("run", 4);
    }

    bool u_see_me = g->u_see(this);

    body_part bp_hit;
    //int highest_hit = 0;
    int hitstat = std::max(type->melee_skill - 2,0);
    int hitroll = dice(hitstat,10);

    damage_instance damage;
    if(!is_hallucination()) {
        if (type->melee_dice > 0) {
            damage.add_damage(DT_BASH,
                    dice(type->melee_dice,type->melee_sides));
        }
        if (type->melee_cut > 0) {
            damage.add_damage(DT_CUT, type->melee_cut);
        }
    }

    /* TODO: height-related bodypart selection
    //If the player is knocked down or the monster can fly, any body part is a valid target
    if(target.is_on_ground() || has_flag(MF_FLIES)){
        highest_hit = 20;
    }
    else {
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
        if (digging()){
            highest_hit -= 8;
        }
        if (highest_hit <= 1){
            highest_hit = 2;
        }
    }

    if (highest_hit > 20){
        highest_hit = 20;
    }

    int bp_rand = rng(0, highest_hit - 1);
    if (bp_rand <=  2){
        bp_hit = bp_legs;
    } else if (bp_rand <= 10){
        bp_hit = bp_torso;
    } else if (bp_rand <= 14){
        bp_hit = bp_arms;
    } else if (bp_rand <= 16){
        bp_hit = bp_mouth;
    } else if (bp_rand == 18){
        bp_hit = bp_eyes;
    } else{
        bp_hit = bp_head;
    }
    */

    dealt_damage_instance dealt_dam;
    int hitspread = target.deal_melee_attack(this, hitroll, false, damage, dealt_dam);
    bp_hit = dealt_dam.bp_hit;

    //Hallucinations always produce messages but never actually deal damage
    if (is_hallucination() || dealt_dam.total_damage() > 0) {
        if (target.is_player()) {
            if (u_see_me) {
                g->add_msg(_("The %1$s hits your %2$s."), name().c_str(),
                        body_part_name(bp_hit, random_side(bp_hit)).c_str());
            } else {
                g->add_msg(_("Something hits your %s."),
                        body_part_name(bp_hit, random_side(bp_hit)).c_str());
            }
        } else {
            if (u_see_me) {
                g->add_msg(_("The %1$s hits %2$s's %3$s."), name().c_str(),
                            target.disp_name().c_str(),
                            body_part_name(bp_hit, random_side(bp_hit)).c_str());
            }
        }
    } else if (hitspread < 0) { // a miss
        // TODO: characters practice dodge when a hit misses 'em
        if (target.is_player()) {
            if (u_see_me) {
                g->add_msg(_("You dodge %1$s."), disp_name().c_str());
            } else {
                g->add_msg(_("You dodge an attack from an unseen source."));
            }
        } else {
            if (u_see_me) {
                g->add_msg(_("The %1$s dodges %2$s's attack."), name().c_str(),
                            target.disp_name().c_str());
            }
        }

    }

    if (is_hallucination()) {
        if(one_in(7)) {
            die();
        }
        return;
    }

    // Adjust anger/morale of same-species monsters, if appropriate
    int anger_adjust = 0, morale_adjust = 0;
    if (type->has_anger_trigger(MTRIG_FRIEND_ATTACKED)){
        anger_adjust += 15;
    }
    if (type->has_fear_trigger(MTRIG_FRIEND_ATTACKED)){
        morale_adjust -= 15;
    }
    if (type->has_placate_trigger(MTRIG_FRIEND_ATTACKED)){
        anger_adjust -= 15;
    }

    if (anger_adjust != 0 && morale_adjust != 0)
    {
        for (int i = 0; i < g->num_zombies(); i++)
        {
            g->zombie(i).morale += morale_adjust;
            g->zombie(i).anger += anger_adjust;
        }
    }
}

void monster::hit_monster(int i)
{
 monster* target = &(g->zombie(i));
 moves -= 100;

 if (this == target) {
  return;
 }

 int numdice = type->melee_skill;
 int dodgedice = target->get_dodge() * 2;
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

int monster::deal_melee_attack(Creature *source, int hitroll, bool crit,
                               const damage_instance& d, dealt_damage_instance &dealt_dam) {
    if (has_flag(MF_ELECTRIC)) { // shockers electrocute melee attackers
        if (source != NULL && source->is_player() &&
                !g->u.wearing_something_on(bp_hands) &&
                (g->u.weapon.conductive() || g->u.unarmed_attack())
                ) {
            damage_instance shock;
            shock.add_damage(DT_ELECTRIC, rng(0,1));
            source->deal_damage(this, bp_arms, 1, shock);
            g->add_msg_if_player(source, _("Contact with %s shocks you!"),
                    disp_name().c_str());
        }
    }
    return Creature::deal_melee_attack(source, hitroll, crit, d, dealt_dam);
}

int monster::deal_projectile_attack(Creature *source, double missed_by,
                                    const projectile& proj, dealt_damage_instance &dealt_dam) {
    bool u_see_mon = g->u_see(this);
    if (has_flag(MF_HARDTOSHOOT) && !one_in(10 - 10 * (.8 - missed_by)) && // Maxes out at 50% chance with perfect hit
            !proj.wide) {
        if (u_see_mon)
            g->add_msg(_("The shot passes through the %s without hitting."),
            disp_name().c_str());
        return 0;
    }
    // Not HARDTOSHOOT
    // if it's a headshot with no head, make it not a headshot
    if (missed_by < 0.2 && has_flag(MF_NOHEAD)) {
        missed_by = 0.2;
    }
    return Creature::deal_projectile_attack(source, missed_by, proj, dealt_dam);
}

void monster::deal_damage_handle_type(const damage_unit& du, body_part bp, int& damage, int& pain) {
    switch (du.type) {
    case DT_ELECTRIC:
        if (has_flag(MF_ELECTRIC)) {
            damage += 0; // immunity
            pain += 0;
            return; // returns, since we don't want a fallthrough
        }
        break;
    case DT_COLD:
        if (!has_flag(MF_WARM)) {
            damage += 0; // immunity
            pain += 0;
            return;
        }
        break;
    case DT_BASH:
        if (has_flag(MF_PLASTIC)) {
            damage += du.amount / rng(2,4); // lessened effect
            pain += du.amount / 4;
            return;
        }
        break;
    }

    Creature::deal_damage_handle_type(du, bp, damage, pain);
}

void monster::apply_damage(Creature* source, body_part bp, int side, int amount) {
    if (is_dead_state()) return; // don't do any more damage if we're already dead
    hurt(bp, side, amount);
    if (is_dead_state()) die(source);
}

void monster::hurt(body_part, int, int dam) {
    hurt(dam);
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

int monster::get_armor_cut(body_part bp)
{
    (void) bp;
// TODO: Add support for worn armor?
 return int(type->armor_cut) + armor_bash_bonus;
}

int monster::get_armor_bash(body_part bp)
{
    (void) bp;
 return int(type->armor_bash) + armor_cut_bonus;
}

int monster::hit_roll() {
    return 0;
}

int monster::get_dodge()
{
 if (has_effect("downed"))
  return 0;
 int ret = type->sk_dodge;
 if (has_effect("beartrap"))
  ret /= 2;
 if (moves <= 0 - 100 - type->speed)
  ret = rng(0, ret);
 return ret + get_dodge_bonus();
}

int monster::dodge_roll()
{
 int numdice = get_dodge();

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

void monster::die(Creature* nkiller) {
    if( nkiller != NULL && !nkiller->is_fake() ) {
        killer = nkiller;
    }
    g->kill_mon(*this, nkiller != NULL && nkiller->is_player());
}

void monster::die()
{
 if (!dead)
  dead = true;
 if (!no_extra_death_drops) {
  drop_items_on_death();
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
                         (groups[i]->type, (type->id)))
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
 // temporary copy as the death function might invalidate this when
 monster tmp_copy(*this);
// Also, perform our death function
 mdeath md;
 if(is_hallucination()) {
   //Hallucinations always just disappear
   md.disappear(this);
   return;
 } else {
   (md.*type->dies)(this);
 }
// If our species fears seeing one of our own die, process that
 int anger_adjust = 0, morale_adjust = 0;
 if (tmp_copy.type->has_anger_trigger(MTRIG_FRIEND_DIED)){
    anger_adjust += 15;
 }
 if (tmp_copy.type->has_fear_trigger(MTRIG_FRIEND_DIED)){
    morale_adjust -= 15;
 }
 if (tmp_copy.type->has_placate_trigger(MTRIG_FRIEND_DIED)){
    anger_adjust -= 15;
 }

 if (anger_adjust != 0 && morale_adjust != 0) {
  int light = g->light_level();
  for (int i = 0; i < g->num_zombies(); i++) {
   int t = 0;
   if (g->m.sees(g->zombie(i).posx(), g->zombie(i).posy(), tmp_copy._posx, tmp_copy._posy, light, t)) {
    g->zombie(i).morale += morale_adjust;
    g->zombie(i).anger += anger_adjust;
   }
  }
 }
}

void monster::drop_items_on_death()
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
        g->m.spawn_item(_posx, _posy, selected_item);

        if (type->item_chance < 0)
        {
            animal_done = true; // Only drop ONE item.
        }
    }
}

void monster::process_effects()
{
    for (std::vector<effect>::iterator it = effects.begin();
            it != effects.end(); ++it) {
        std::string id = it->get_id();
        if (id == "poisoned") {
            speed -= rng(0, 3);
            hurt(rng(1, 3));

        // MATERIALS-TODO: use fire resistance
        } else if (id == "onfire") {
            if (made_of("flesh"))
                hurt(rng(3, 8));
            if (made_of("veggy"))
                hurt(rng(10, 20));
            if (made_of("paper") || made_of("powder") || made_of("wood") || made_of("cotton") ||
                made_of("wool"))
                hurt(rng(15, 40));
        }
    }

    Creature::process_effects();
}

bool monster::make_fungus()
{
    char polypick = 0;
    std::string tid = type->id;
    if (tid == "mon_ant" || tid == "mon_ant_soldier" || tid == "mon_ant_queen" || tid == "mon_fly" || tid == "mon_bee" || tid == "mon_dermatik")
    {
        polypick = 1;
    }else if (tid == "mon_zombie" || tid == "mon_zombie_shrieker" || tid == "mon_zombie_electric" || tid == "mon_zombie_spitter" || tid == "mon_zombie_fast" ||
              tid == "mon_zombie_brute" || tid == "mon_zombie_hulk"){
        polypick = 2;
    }else if (tid == "mon_boomer"){
        polypick = 3;
    }else if (tid == "mon_triffid" || tid == "mon_triffid_young" || tid == "mon_triffid_queen"){
        polypick = 4;
    }
    switch (polypick) {
        case 1: // bugs, why do they all turn into fungal ants?
            poly(GetMType("mon_ant_fungus"));
            return true;
        case 2: // zombies, non-boomer
            poly(GetMType("mon_zombie_fungus"));
            return true;
        case 3:
            poly(GetMType("mon_boomer_fungus"));
            return true;
        case 4:
            poly(GetMType("mon_fungaloid"));
            return true;
        default:
            return true;
    }
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

bool monster::getkeep()
{
    return keep;
}

void monster::setkeep(bool r)
{
    keep = r;
}
