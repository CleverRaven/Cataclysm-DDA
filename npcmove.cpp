#include "npc.h"
#include "rng.h"
#include "game.h"
#include "line.h"

std::string npc_action_name(npc_action action);

void npc::move(game *g)
{
 int light = g->light_level();
 int linet;
 npc_action action = npc_pause;
 target = choose_monster_target(g);	// Set a target
// If we aren't moving towards an item or a monster, find an item
 if (fetching_item && target == NULL)
  find_items(g);

 if (want_to_attack_player(g))
  action = method_of_attacking_player(g);
 else if (target != NULL)
  action = method_of_attacking_monster(g);
 else if (mission == MISSION_SHOPKEEP)
  action = npc_pause;
 else if (can_reload())
  action = npc_reload;
 else if (attitude == NPCATT_HEAL && g->sees_u(posx, posy, linet)) {
  if (path.empty())
   path = g->m.route(posx, posy, g->u.posx, g->u.posy);
  action = npc_heal_player;
 } else if (attitude == NPCATT_MUG && g->sees_u(posx, posy, linet)) {
  if (path.empty() || g->u.posx != plx || g->u.posy != ply)
   path = g->m.route(posx, posy, g->u.posx, g->u.posy);
  action = npc_mug_player;
  plx = g->u.posx;
  ply = g->u.posy;
 } else if (fetching_item)
  action = npc_pickup;
 else
  action = long_term_goal_action(g);

 if (g->debugmon)
  debugmsg("%s (%d:%d) chose action %s.  Mission is %d.", name.c_str(),
           posx, posy, npc_action_name(action).c_str(), mission);

 int oldmoves = moves;

 switch (action) {

 case npc_pause:
  move_pause();
  break;

 case npc_reload:
  moves -= weapon.reload_time(*this);
  weapon.reload(*this, false);
  recoil = 6;
  if (g->u_see(posx, posy, linet))
   g->add_msg("%s reloads %s %s.", name.c_str(), (male ? "his" : "her"),
              weapon.tname().c_str());
  break;

 case npc_pickup:
  pickup_items(g);
  break;

 case npc_flee_monster:
  if (target == NULL)
   debugmsg("%s tried to flee a null monster!", name.c_str());
  move_away_from(g, target->posx, target->posy);
  break;

 case npc_melee_monster:
  if (path.size() > 1)
   move_to_next_in_path(g);
  else if (path.size() <= 1)
   melee_monster(g, target);
  else
   move_pause();
  break;
  
 case npc_shoot_monster:
  if (trig_dist(posx, posy, target->posx, target->posy)<=confident_range() /3 &&
      target->hp >= weapon.curammo->damage * 3)
   g->fire(*this, target->posx, target->posy, path, true); // Burst fire
  else
   g->fire(*this, target->posx, target->posy, path, false);// Normal
  break;

 case npc_alt_attack_monster:
  alt_attack(g, target, NULL);
  break;

 case npc_look_for_player:
  if (saw_player_recently() && g->m.sees(posx, posy, plx, ply, light, linet)) {
// (plx, ply) is the point where we last saw the player
   if (path.empty())
    path = line_to(posx, posy, plx, ply, linet);
   move_to_next_in_path(g);
  } else
   look_for_player(g);
  break;

 case npc_flee_player:
  move_away_from(g, g->u.posx, g->u.posy);
  break;

 case npc_melee_player:
  if (path.size() == 1)	// We're adjacent to u, and thus can sock u one
   melee_player(g, g->u);
  else if (path.size() > 0)
   move_to_next_in_path(g);
  else
   move_pause();
  break;

 case npc_shoot_player:
  g->add_msg("%s shoots at you with %s %s!", name.c_str(),
             (male ? "his" : "her"), weapon.tname().c_str());
// If we're at 1/3rd of our confident range, burst fire at the player
  g->fire(*this, g->u.posx, g->u.posy, path,
        (trig_dist(posx, posy, g->u.posx, g->u.posy) <= confident_range() / 3));
  break;

 case npc_alt_attack_player:
  alt_attack(g, NULL, &(g->u));
  break;

 case npc_heal_player:
  if (path.size() == 1)	// We're adjacent to u, and thus can heal u
   heal_player(g, g->u);
  else if (path.size() > 0)
   move_to_next_in_path(g);
  else
   move_pause();
  break;

 case npc_follow_player:
  if (path.size() <= follow_distance())	// We're close enough to u.
   move_pause();
  else if (path.size() > 0)
   move_to_next_in_path(g);
  else
   move_pause();
  break;

 case npc_talk_to_player:
  talk_to_u(g);
  break;

 case npc_mug_player:
  if (path.size() == 1)	// We're adjacent to u, and thus can mug u
   mug_player(g, g->u);
  else if (path.size() > 0)
   move_to_next_in_path(g);
  else
   move_pause();
  break;

 case npc_goto_destination:
  go_to_destination(g);
  break;

 default:
  debugmsg("Unknown NPC action (%d)", action);
 }

 if (oldmoves == moves) {
  debugmsg("NPC didn't use its moves.  Turning on debug mode.");
  g->debugmon = true;
 }
}

monster* npc::choose_monster_target(game *g)
{
 int closest = 1000, plclosest = 1000, lowHP = 10000, pllowHP = 10000;
 int index = -1, plindex = -1;
 int distance;
 int linet;
 bool defend_u = g->sees_u(posx, posy, linet) && !want_to_attack_player(g);

 for (int i = 0; i < g->z.size(); i++) {
  if (!g->z[i].is_fleeing(g->u) && !g->z[i].is_fleeing(*this) &&
      g->pl_sees(this, &(g->z[i]), linet) && g->z[i].friendly == 0) {
   if (defend_u) {
    distance = trig_dist(g->u.posx, g->u.posy, g->z[i].posx, g->z[i].posy);
    if (distance < plclosest) {
     plindex = i;
     plclosest = distance;
     pllowHP = g->z[i].hp;
    } else if (distance == plclosest && g->z[i].hp < pllowHP) {
     plindex = i;
     pllowHP = g->z[i].hp;
    }
   }
   distance = trig_dist(posx, posy, g->z[i].posx, g->z[i].posy);
   if (distance < closest) {
    index = i;
    closest = distance;
    lowHP = g->z[i].hp;
   } else if (distance == closest && g->z[i].hp < lowHP) {
    index = i;
    lowHP = g->z[i].hp;
   }
  }
 }
 if (plindex != -1) {	// Maybe defend the player.
  int importance = (g->u.hp_percentage() - hp_percentage()) / 20;
  importance -= op_of_u.value / 2;
  if (attitude == NPCATT_DEFEND)
   importance -= 5;

  if (bravery_check(importance))
   return &(g->z[plindex]);
 }
 if (index != -1)
  return &(g->z[index]);
 return NULL;
}

void npc::find_items(game *g)
{
 int dist = g->light_level();
 int linet, top_value = 0;
 int minx = posx - dist, maxx = posx + dist;
 int miny = posy - dist, maxy = posy + dist;
 int itx = 999, ity = 999;

 fetching_item = false;

// Make sure the bounds of our search aren't outside the map!
 if (minx < 0)
  minx = 0;
 if (miny < 0)
  miny = 0;
 if (maxx > SEEX * 3 - 1)
  maxx = SEEX * 3 - 1;
 if (maxy > SEEY * 3 - 1)
  maxy = SEEY * 3 - 1;

 for (int x = minx; x <= maxx; x++) {
  for (int y = miny; y <= maxy; y++) {
   if (g->m.sees(posx, posy, x, y, dist, linet)) {
    for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
     item *tmp = &(g->m.i_at(x, y)[i]);
// We value it enough to pick it up
     if (value(*tmp) + personality.collector >= NPC_HI_VALUE &&
         value(*tmp) > top_value &&
         volume_carried() + tmp->volume() <= volume_capacity() &&
         weight_carried() + tmp->weight() <= weight_capacity()   ) {
      top_value = value(g->m.i_at(x, y)[i]);
      itx = x;
      ity = y;
      fetching_item = true;
     }
    }
   }
  }
 }
 if (fetching_item)	// We found something we want
  path = g->m.route(posx, posy, itx, ity);
}

void npc::pickup_items(game *g)
{
 int light = g->light_level(), dist = path.size(), linet;
 int itx = posx, ity = posy;
 if (dist > 0) {
  itx = path.back().x;
  ity = path.back().y;
 }
 bool debug = g->debugmon;
 bool u_see_me = g->u_see(posx, posy, linet);
 bool u_see_item = g->u_see(itx, ity, linet);
 
 if (!fetching_item) {
  debugmsg("Ran pickup_items(), but there's no item in mind!");
  return;
 }
 if (dist <= 1) {	// We can pick it up!
  if (debug)
   debugmsg("Distance to item < 1; picking up items.");
  item *tmp;
  for (int i = 0; i < g->m.i_at(itx, ity).size(); i++) {
   tmp = &(g->m.i_at(itx, ity)[i]);
   if (value(*tmp) >= NPC_HI_VALUE &&
       volume_carried() + tmp->volume() <= volume_capacity() &&
       weight_carried() + tmp->weight() <= weight_capacity()) {
    i_add(*tmp);
    if (u_see_me || u_see_item)
     g->add_msg("%s picks up %s%s.", (u_see_me ? name.c_str() : "Someone"),
                                     (u_see_item ? "a " : "something"),
                                     (u_see_item ? tmp->tname().c_str() : ""));
      
    g->m.i_rem(itx, ity, i);
    i--;
   }
  }
  moves -= 100;
 } else if (g->m.sees(posx, posy, itx, ity, light, linet)) {
  if (debug)
   debugmsg("We see our item(s).  NPC @ (%d:%d); items @ (%d:%d) (dist %d",
            posx, posy, itx, ity, dist);
  if (path.size() == 0) {
   if (g->m.move_cost(itx, ity) > 0)
    path = g->m.route(posx, posy, itx, ity);
   else {	// Move to a valid adjacent point
    std::vector<point> valid;
    std::vector<int> dist;
// Populate valid with the valid points, dist with their distances from us
    for (int x = itx - 1; x <= itx + 1; x++) {
     for (int y = ity - 1; y <= ity + 1; y++) {
      if (g->m.move_cost(x, y) > 0) {
       valid.push_back(point(x, y));
       dist.push_back(rl_dist(posx, posy, x, y));
      }
     }
    }
    if (valid.empty()) {
     debugmsg("Oh no!  Couldn't find an open space next to %d:%d", itx, ity);
     fetching_item = false;
     path.clear();
     return;
    }
// Populate "indices" with the indices of the closest points
    int best_dist = dist[0];
    std::vector<int> indices;
    indices.push_back(0);
    for (int i = 1; i < dist.size(); i++) {
     if (dist[i] < best_dist) {
      best_dist = dist[i];
      indices.clear();
      indices.push_back(i);
     } else if (dist[i] == best_dist)
      indices.push_back(i);
    }
// Pick one of those closest points, and move to it
    int index = indices[rng(0, indices.size() - 1)];
    path = g->m.route(posx, posy, valid[index].x, valid[index].y);
    fetching_item = true;
   }
  }
  if (path.size() > 0)
   move_to_next_in_path(g);
  else
   move_pause();
 } else {	// Can't see that item anymore, so look for a new one
  if (debug)
   debugmsg("Can't see our desired item!  Gonna pause.");
  find_items(g);
  move_pause();
 }
}

bool npc::want_to_attack_player(game *g)
{
 int j;
 if ((attitude == NPCATT_KILL || attitude == NPCATT_FLEE) &&
     g->m.sees(posx, posy, g->u.posx, g->u.posy, g->light_level(), j) &&
     (target == NULL || rl_dist(posx, posy, g->u.posx, g->u.posy) > 3))
  return true;
 return false;
}

int npc::follow_distance()
{
 int ret = 4;
 if (weapon.is_gun())
  ret = 7;
 ret = (ret * hp_percentage()) / 100;
 return (ret < 2 ? 2 : ret);
}

npc_action npc::method_of_attacking_player(game *g)
{
 if (g->debugmon)
  debugmsg("method_of_attacking_player()");
 int linet;
 int dist = trig_dist(posx, posy, g->u.posx, g->u.posy);
 bool can_see_u = g->sees_u(posx, posy, linet); 
// Set our path to the player
 if (can_see_u)
  path = line_to(posx, posy, g->u.posx, g->u.posy, linet);
/*
 else if (saw_player_recently() &&
          g->m.sees(posx, posy, plx, ply, g->light_level(), linet))
  path = line_to(posx, posy, plx, ply, linet);
*/
 else {	// We can't see u, and we haven't seen u recently
 // path.clear();
  if (attitude == NPCATT_FLEE)
   return long_term_goal_action(g);// We're probably safe from u...
  //return npc_look_for_player;
 }

 if (can_see_u) {
  if (attitude == NPCATT_FLEE)
   return npc_flee_player;
  if (weapon.is_gun()) {
   if (weapon.charges > 0) {
    if (dist <= confident_range())
     return npc_shoot_player;
    else if (can_reload() &&
             dist - confident_range() >= weapon.reload_time(*this))
// It takes less time to reload than to get within confident range
     return npc_reload;
    else
     return npc_melee_player;
   } else {	// We're wielding a gun, but it's not loaded
    if (path.size() <= 2)
     return npc_melee_player;
    else if (can_reload() && !g->u.weapon.is_gun() &&
             (dist >= weapon.reload_time(*this) || g->u.hp_percentage() <= 25))
     return npc_reload; // If we have time to reload OR you're almost dead
    else if (alt_attack_available())	// Grenades, etc.
     return npc_alt_attack_player;
    else
     return npc_flee_player;	// Get away, then maybe reload
   }
  } else {	// Our weapon is not a gun
   if (path.size() <= 4 || !g->u.weapon.is_gun()) // Safe to attack
    return npc_melee_player;
   else if (alt_attack_available())	// Grenades, etc.
    return npc_alt_attack_player;
   else
    return npc_flee_player;
  }
 } else {	// We can't see you!
  if (attitude == NPCATT_FLEE)
   return npc_flee_player;
  return npc_melee_player;	// Follow path to plx, ply
 }
}

npc_action npc::method_of_attacking_monster(game *g)
{
 if (target == NULL) {
  debugmsg("Tried to figure out how to attack a null monster!");
  return npc_pause;
 }
 if (g->debugmon)
  debugmsg("method_of_attacking_monster(); %s (%d:%d)", target->name().c_str(), target->posx, target->posy);
 int linet, light = g->light_level();
 if (g->m.sees(posx, posy, target->posx, target->posy, light, linet))
  path = line_to(posx, posy, target->posx, target->posy, linet);
 else {
  target = choose_monster_target(g);
  if (g->m.sees(posx, posy, target->posx, target->posy, light, linet))
   path = line_to(posx, posy, target->posx, target->posy, linet);
  else
   return npc_pause;
 }
 int dist = trig_dist(posx, posy, target->posx, target->posy);
 int rldist = rl_dist(posx, posy, target->posx, target->posy);
 if (weapon.is_gun()) {
  if (weapon.charges > 0) {
   if (dist <= confident_range()) {
    if (wont_shoot_friend(g))
     return npc_shoot_monster;
    else {	// u or a friendly NPC is in the way
     std::string saytext = std::string("Move so I can shoot that") +
                           target->name().c_str() + "!";
     say(g, saytext);
     if (can_reload())
      return npc_reload;
     return npc_flee_monster;
    }
   } else {	// The monster is outside of our confident range
    if (can_reload() && rldist * 1.5 <= weapon.reload_time(*this))
     return npc_reload;	// Plenty of time to reload
    else if (target->speed >= 100)
     return npc_melee_monster;	// They're fast, don't ignore them
    else
     return long_term_goal_action(g); // Ignore the monster
   }
  } else {	// Gun isn't loaded
   if (can_reload() && rldist * 1.5 <= weapon.reload_time(*this))
    return npc_reload;	// Plenty of time to reload
   else if (target->speed >= 90 && hp_percentage() >= 75)
    return npc_melee_monster;
   else
    return long_term_goal_action(g);	// Ignore the monster
  }
 } else {	// Our weapon isn't a gun
  if ((rldist * 100) / target->speed <= 6) { // Close enough to care
   if (hp_percentage() + sklevel[sk_melee] * 10 >= 80)	// We're confident
    return npc_melee_monster;
   else
    return npc_flee_monster;
  } else	// Too far away or slow, ignore them for now
   return long_term_goal_action(g);
 }
}

bool npc::can_reload()
{
 if (!weapon.is_gun())
  return false;
 it_gun* gun = dynamic_cast<it_gun*> (weapon.type);
 return (weapon.charges < gun->clip && has_ammo(gun->ammo).size() > 0);
}

bool npc::saw_player_recently()
{
 return (plx != 999 && ply != 999);
}

bool npc::has_destination()
{
 return (goalx != 999 && goaly != 999);
}

bool npc::alt_attack_available()
{
 if (has_number(itm_grenade,  1) || has_number(itm_grenade_act,  1)  ||
     has_number(itm_molotov,  1) || has_number(itm_molotov_lit,  1)  ||
     has_number(itm_gasbomb,  1) || has_number(itm_gasbomb_act,  1)  ||
     has_number(itm_dynamite, 1) || has_number(itm_dynamite_act, 1)  ||
     has_number(itm_mininuke, 1) || has_number(itm_mininuke_act, 1)    )
  return true;
 return false;
}

npc_action npc::long_term_goal_action(game *g)
{
 if (g->debugmon)
  debugmsg("long_term_goal_action()");
 int linet;
 int light = g->light_level();
 path.clear();
 if (mission == MISSION_SHOPKEEP)
  return npc_pause;	// Shopkeeps just stay put.
 if (attitude == NPCATT_FOLLOW || attitude == NPCATT_FOLLOW_RUN ||
     attitude == NPCATT_SLAVE) {
  if (g->sees_u(posx, posy, linet)) {
   path = line_to(posx, posy, g->u.posx, g->u.posy, linet);
   return npc_follow_player;
  } else if (path.empty() && saw_player_recently() &&
             g->m.sees(posx, posy, plx, ply, light, linet)) {
   path = line_to(posx, posy, plx, ply, linet);
   return npc_follow_player;
  } else if (path.empty())
   return npc_look_for_player;
  return npc_follow_player;
 } else if (attitude == NPCATT_TALK || attitude == NPCATT_TRADE) {
  if (g->sees_u(posx, posy, linet))
   path = line_to(posx, posy, g->u.posx, g->u.posy, linet);
  else if (saw_player_recently() &&
           g->m.sees(posx, posy, plx, ply, light, linet))
   path = line_to(posx, posy, plx, ply, linet);
  if (path.size() <= 6 && path.size() > 0)	// Close enough to talk
   return npc_talk_to_player;
  else if (!path.empty()) {
   if (one_in(20))
    say(g, "Come here, let's talk!");
   return npc_follow_player;
  } else
   return npc_look_for_player;
 } else {	// Nothing in particular we care to do with the player
  if (!has_destination())
   set_destination(g);
  return npc_goto_destination;
 }
}

void npc::set_destination(game *g)
{
 decide_needs();
 if (needs[0] == need_none) {	// We don't really need anything in particular
/*	Hold off on this for now. TODO: This
  std::vector<faction_value> values;
// Maybe find something based on what our faction values
  if (fac_has_value(FACVAL_EXPLORATION))
   values.push_back(FACVAL_EXPLORATION);
  if (fac_has_value(FACVAL_SHADOW))
   values.push_back(FACVAL_SHADOW);
  if (fac_has_value(FACVAL_DRUGS))
   values.push_back(FACVAL_DRUGS);
  if (fac_has_value(FACVAL_BIONICS))
   values.push_back(FACVAL_BIONICS);
  if (fac_has_value(FACVAL_DOCTORS))
   values.push_back(FACVAL_DOCTORS);
  if (fac_has_value(FACVAL_BOOKS))
   values.push_back(FACVAL_BOOKS);
  if (fac_has_value(FACVAL_ROBOTS))
   values.push_back(FACVAL_ROBOTS);

  if (values.size() == 0) {// Our faction doesn't collect anything in particular
*/
   goalx = mapx + rng(-2, 2);
   goaly = mapy + rng(-2, 2);
/*
  } else {	// Our faction DOES collect stuff!
   oter_id dest_type = ot_null;	// Type of terrain we're looking for
   switch (values[rng(0, values.size() - 1)]) {	// Pick one at random
    case FACVAL_SHADOW:		dest_type = ot_sub_station_north;	break;
    case FACVAL_DRUGS:
    case FACVAL_DOCTORS:	dest_type = ot_s_pharm_north;		break;
    case FACVAL_BIONICS:
    case FACVAL_ROBOTS:		dest_type = ot_s_hardware_north;	break;
    case FACVAL_BOOKS:		dest_type = ot_s_library_north;		break;
   }
   int dist = 0;
   point p = g->cur_om.find_closest(point(mapx, mapy),dest_type,4, dist, false);
   goalx = p.x;
   goaly = p.y;
  }
*/
 } else {	// We do have at least one pressing need
  oter_id dest_type = ot_null;	// Type of terrain we're looking for
  switch (needs[0]) {
   case need_ammo:
   case need_gun:	dest_type = ot_s_gun_north;		break;
   case need_weapon:	dest_type = ot_s_hardware_north;	break;
   case need_food:	dest_type = ot_s_grocery_north;		break;
   case need_drink:	dest_type = ot_s_liquor_north;		break;
  }
  int dist = 0;
  point p = g->cur_om.find_closest(point(mapx, mapy),dest_type,4, dist, false);
  goalx = p.x;
  goaly = p.y;
 }
}

void npc::go_to_destination(game *g)
{
 int sx = (goalx > mapx ? 1 : -1), sy = (goaly > mapy ? 1 : -1);
 if (goalx == mapx && goaly == mapy)	// We're at our desired map square!
  move_pause();
 else {
  if (goalx == mapx)
   sx = 0;
  if (goaly == mapy)
   sy = 0;
// sx and sy are now equal to the direction we need to move in
  int x = posx + 8 * sx, y = posy + 8 * sy, linet, light = g->light_level();
// x and y are now equal to a local square that's close by
  for (int i = 0; i < 8; i++) {
   for (int dx = 0 - i; dx <= i; dx++) {
    for (int dy = 0 - i; dy <= i; dy++) {
     if ((g->m.move_cost(x + dx, y + dy) > 0 ||
          g->m.has_flag(bashable, x + dx, y + dy) ||
          g->m.ter(x + dx, y + dy) == t_door_c) &&
         g->m.sees(posx, posy, x + dx, y + dy, light, linet)) {
      path = g->m.route(posx, posy, x + dx, y + dy);
      if (!path.empty() && can_move_to(g, path[0].x, path[0].y)) {
       move_to_next_in_path(g);
       return;
      } else {
       move_pause();
       return;
      }
     }
    }
   }
  }
  move_pause();
 }
}

bool npc::can_move_to(game *g, int x, int y)
{
 if ((g->m.move_cost(x, y) > 0 || g->m.has_flag(bashable, x, y)) &&
     rl_dist(posx, posy, x, y) <= 1)
  return true;
 return false;
}

void npc::move_to(game *g, int x, int y)
{
 if (recoil > 0) {	// Start by dropping recoil a little
  if (int(str_cur / 2) + sklevel[sk_gun] >= recoil)
   recoil = 0;
  else {
   recoil -= int(str_cur / 2) + sklevel[sk_gun];
   recoil = int(recoil / 2);
  }
 }
 if (rl_dist(posx, posy, x, y) > 1) {
  debugmsg("Tried to move_to more than one space! (%d, %d) to (%d, %d)",
           posx, posy, x, y);
  //debugmsg("Route is size %d.", path.size());
  int linet;
  if (g->m.sees(posx, posy, x, y, -1, linet))
   path = line_to(posx, posy, x, y, linet);
  moves -= 100;
  return;
 }
 if (x == posx && y == posy)	// We're just pausing!
  moves -= 100;
 else if (g->mon_at(x, y) != -1) {	// Shouldn't happen, but it might.
  monster *m = &(g->z[g->mon_at(x, y)]);
  debugmsg("Bumped into a monster, %d, a %s",g->mon_at(x, y),m->name().c_str());
  melee_monster(g, m);
 } else if (g->u.posx == x && g->u.posy == y) {
  say(g, "Excuse me, let me pass.");
  moves -= 100;
 } else if (g->npc_at(x, y) != -1)
// TODO: Determine if it's an enemy NPC (hit them), or a friendly in the way
  moves -= 100;
 else if (g->m.move_cost(x, y) > 0) {
  posx = x;
  posy = y;
  moves -= g->m.move_cost(x, y) * 50;
 } else if (g->m.open_door(x, y, (g->m.ter(posx, posy) == t_floor)))
  moves -= 100;
 else if (g->m.has_flag(bashable, x, y)) {
  moves -= 110;
  std::string bashsound;
  int smashskill = int(str_cur / 2 + weapon.type->melee_dam);
  g->m.bash(x, y, smashskill, bashsound);
  g->sound(x, y, 18, bashsound);
 } else
  moves -= 100;
}

void npc::move_to_next_in_path(game *g)
{
 if (path.empty()) {
  debugmsg("Tried to move to the next space in an empty path!");
  move_pause();
  return;
 }
 move_to(g, path[0].x, path[0].y);
 if (posx == path[0].x && posy == path[0].y)
  path.erase(path.begin());
}

void npc::move_away_from(game *g, int x, int y)
{
 int sx = (x > posx ? -1 : 1), sy = (y > posy ? -1 : 1);
 int dx = abs(posx - x), dy = abs(posy - y);
 if (can_move_to(g, posx + sx, posy + sy))
  move_to(g, posx + sx, posy + sy);
 else {
  if (dx >= dy) {	// More important to flee via x
   if (can_move_to(g, posx + sx, posy))
    move_to(g, posx + sx, posy);
   else if (dx >= dy * 2 && can_move_to(g, posx + sx, posy - sy))
    move_to(g, posx + sx, posy - sy);
   else if (can_move_to(g, posx, posy + sy))
    move_to(g, posx, posy + sy);
   else if (dx > 3 && can_move_to(g, posx - sx, posy + sy))
    move_to(g, posx - sx, posy + sy);
   else
    move_pause();
  } else {
   if (can_move_to(g, posx, posy + sy))
    move_to(g, posx, posy + sy);
   else if (dy >= dx * 2 && can_move_to(g, posx - sx, posy + sy))
    move_to(g, posx - sx, posy + sy);
   else if (can_move_to(g, posx + sx, posy))
    move_to(g, posx + sx,  posy);
   else if (dy > 3 && can_move_to(g, posx + sx, posy - sy))
    move_to(g, posx + sx, posy - sy);
   else
    move_pause();
  }
 }
}

void npc::move_pause()
{
 moves = 0;
 recoil = 0;
}

void npc::melee_monster(game *g, monster *m)
{
 int dam = hit_mon(g, m);
 if (m->hurt(dam))
  g->kill_mon(g->mon_at(m->posx, m->posy));
}

void npc::alt_attack(game *g, monster *m, player *p)
{
// In order of importance
 itype_id options[10] = {itm_mininuke_act, itm_dynamite_act, itm_grenade_act,
                         itm_gasbomb_act, itm_molotov_lit, itm_mininuke,
                         itm_dynamite, itm_grenade, itm_gasbomb, itm_molotov};
 item used;
 bool throwing = false;
 int i, tx, ty, linet;
 if (m == NULL && p == NULL) {
  debugmsg("alt_attack() with m and p null!");
  return;
 } else if (m != NULL) {
  tx = m->posx;
  ty = m->posy;
 } else {
  tx = p->posx;
  ty = p->posy;
 }
 for (i = 0; i < 10; i++) {
  if (has_number(options[i], 1)) {
   if (i <= 4)
    throwing = true;
   if (throwing)
    used = i_rem(options[i]);
   else
    used = i_of_type(options[i]);
   i = 9;
  }
 }
 if (i == 8) {	// We didn't encounter anything we can use!
  debugmsg("alt_attack() didn't find anything usable");
  return;
 }
 if (throwing) {
  int light = g->light_level();
  std::vector<point> trajectory;
  if (g->m.sees(posx, posy, tx, ty, light, linet))
   trajectory = line_to(posx, posy, tx, ty, linet);
  else
   trajectory = line_to(posx, posy, tx, ty, 0);
  moves -= 125;
  if (g->u_see(posx, posy, linet))
   g->add_msg("%s throws a %s.", name.c_str(), used.tname().c_str());
  g->throw_item(*this, tx, ty, used, trajectory);
 } else {
  bool seen = g->u_see(posx, posy, linet);
  item *activated = &(i_of_type(itype_id(used.type->id)));
  switch (used.type->id) {
  case itm_mininuke:
   activated->make(g->itypes[itm_mininuke_act]);
   activated->charges = 5;
   activated->active = true;
   if (seen)
    g->add_msg("%s activates %s mininuke.", name.c_str(), (male ? "his":"her"));
   break;
  case itm_dynamite:
   activated->make(g->itypes[itm_dynamite_act]);
   activated->charges = 20;
   activated->active = true;
   if (seen)
    g->add_msg("%s lights %s dynamite.", name.c_str(), (male ? "his":"her"));
   break;
  case itm_gasbomb:
   activated->make(g->itypes[itm_gasbomb_act]);
   activated->charges = 20;
   activated->active = true;
   if (seen)
    g->add_msg("%s pulls the pin on %s gas canister.", name.c_str(),
               (male ? "his":"her"));
   break;
  case itm_grenade:
   activated->make(g->itypes[itm_gasbomb_act]);
   activated->charges = 5;
   activated->active = true;
   if (seen)
    g->add_msg("%s pulls the pin on %s grenade.", name.c_str(),
               (male ? "his":"her"));
   break;
  case itm_molotov:
   activated->make(g->itypes[itm_molotov_lit]);
   activated->charges = 1;
   activated->active = true;
   if (seen)
    g->add_msg("%s lights %s molotov.", name.c_str(), (male ? "his":"her"));
   break;
  }
 }
}

void npc::melee_player(game *g, player &foe)
{
 moves -= 80 + weapon.volume() * 2 + weapon.weight() + encumb(bp_torso);
 body_part bphit;
 int hit_dam = 0, hit_cut = 0, side = rng(0, 1);
 bool hitit = hit_player(g->u, bphit, hit_dam, hit_cut);
 if (hitit) {
  g->add_msg("%s hits your %s with %s %s.", name.c_str(),
             body_part_name(bphit, side).c_str(), (male ? "his" : "her"),
             weapname(false).c_str());
  g->u.hit(g, bphit, side, hit_dam, hit_cut);
 } else
  g->add_msg("%s swings %s %s at you, but misses.", name.c_str(),
             (male ? "his" : "her"), weapname(false).c_str());
}


void npc::heal_player(game *g, player &patient)
{
 int low_health = 400;
 hp_part worst;
// Chose the worst-hurting body part
 for (int i = 0; i < num_hp_parts; i++) {
  int hp = patient.hp_cur[i];
// The head and torso are weighted more heavily than other body parts
  if (i == hp_head)
   hp = patient.hp_max[i] - 3 * (patient.hp_max[i] - hp);
  else if (i == hp_torso)
   hp = patient.hp_max[i] - 2 * (patient.hp_max[i] - hp);
  if (hp < low_health) {
   low_health = hp;
   worst = hp_part(i);
  }
 }

 if (patient.is_npc())
  g->add_msg("%s heals %s.", name.c_str(), patient.name.c_str());
 else
  g->add_msg("%s heals you.", name.c_str());

 int amount_healed;
 if (has_amount(itm_1st_aid, 1)) {
  switch (worst) {
   case hp_head:  amount_healed = 10 + 1.6 * sklevel[sk_firstaid]; break;
   case hp_torso: amount_healed = 20 + 3   * sklevel[sk_firstaid]; break;
   default:       amount_healed = 15 + 2   * sklevel[sk_firstaid];
  }
  use_up(itm_1st_aid, 1);
 } else if (has_amount(itm_bandages, 1)) {
  switch (worst) {
   case hp_head:  amount_healed =  1 + 1.6 * sklevel[sk_firstaid]; break;
   case hp_torso: amount_healed =  4 + 3   * sklevel[sk_firstaid]; break;
   default:       amount_healed =  3 + 2   * sklevel[sk_firstaid];
  }
  use_up(itm_bandages, 1);
 }
 patient.hp_cur[worst] += amount_healed;
 if (patient.hp_cur[worst] > patient.hp_max[worst])
  patient.hp_cur[worst] = patient.hp_max[worst];

 if (!patient.is_npc()) {
// Test if we want to heal the player further
  if (op_of_u.value * 4 + op_of_u.trust + personality.altruism * 3 +
      (fac_has_value(FACVAL_CHARITABLE)    ?  5 : 0) +
      (fac_has_job  (FACJOB_DOCTORS)       ? 15 : 0) - op_of_u.fear * 3 <  25) {
   attitude = NPCATT_FOLLOW;
   say(g, "That's all the healing I can do.");
  } else
   say(g, "Hold still, I can heal you more.");
 }
}

void npc::mug_player(game *g, player &mark)
{
 int linet;
 if (mark.cash > 0) {
  moves -= 100;
  if (mark.is_npc() && g->u_see(posx, posy, linet))
   g->add_msg("%s takes %d's money.", name.c_str(), mark.name.c_str());
  else if (!mark.is_npc())
   g->add_msg("%s takes all of your money.", name.c_str());
  cash += mark.cash;
  mark.cash = 0;
 } else if (mark.weapon.type->id != 0 && mark.weapon.type->id < num_items) {
  if (mark.is_npc() && g->u_see(posx, posy, linet))
   g->add_msg("%s takes %d's %d.", name.c_str(), mark.name.c_str(),
              mark.weapon.tname().c_str());
  else if (!mark.is_npc())
   g->add_msg("%s takes your %d.", name.c_str(), mark.weapon.tname().c_str());
  if (volume_carried() + mark.weapon.volume() <= volume_capacity() &&
      weight_carried() + mark.weapon.weight() <= weight_capacity()   )
   i_add(mark.weapon);
  else {
   if (g->u_see(posx, posy, linet))
    g->add_msg("%s drops it.", (male ? "He" : "She"));
   g->m.add_item(posx, posy, mark.weapon);
  }
  mark.remove_weapon();
 } else {	// We already took your weapon; start on inventory
  int best_value = 0, index = -1;
  for (int i = 0; i < mark.inv.size(); i++) {
   int itvalue = value(mark.inv[i]);
   if (itvalue >= NPC_LOW_VALUE &&
       volume_carried() + mark.inv[i].volume() <= volume_capacity() &&
       weight_carried() + mark.inv[i].weight() <= weight_capacity() &&
       (itvalue > best_value || (itvalue == best_value &&
                            mark.inv[i].volume() < mark.inv[index].volume()))) {
    best_value = itvalue;
    index = i;
   }
  }
  if (index == -1) {	// Nothing left to steal!
   attitude = NPCATT_FLEE;
   move_away_from(g, mark.posx, mark.posy);
  } else {
   moves -= 100;
   if (mark.is_npc() && g->u_see(posx, posy, linet))
    g->add_msg("%s takes %s's %s.", name.c_str(), mark.name.c_str(),
               mark.inv[index].tname().c_str());
   else if (!mark.is_npc())
     g->add_msg("%s takes your %s.", name.c_str(),
                mark.inv[index].tname().c_str());
   i_add(mark.inv[index]);
   mark.i_remn(index);
  }
 }
}

void npc::look_for_player(game *g)
{
 if (g->turn % 6 == 0 && one_in(2)) {
// Call out to the player
  switch (rng(0, 4)) {
   case 0: say(g, "Where are you?");				break;
   case 1: say(g, "Hey, where are you?");			break;
   case 2: say(g, "We need to stick together, come back!");	break;
   case 3: say(g, "Wait up!");					break;
   case 4: say(g, "Don't leave me!");				break;
  }
 }

 goalx = mapx + rng(-1, 1);
 goaly = mapy + rng(-1, 1);
 go_to_destination(g);
}

std::string npc_action_name(npc_action action)
{
 switch (action) {
  case npc_pause:		return "pause";
  case npc_reload:		return "reload";
  case npc_pickup:		return "pickup";
  case npc_flee_monster:	return "flee monster";
  case npc_melee_monster:	return "melee monster";
  case npc_shoot_monster:	return "shoot monster";
  case npc_alt_attack_monster:	return "alt attack monster";
  case npc_look_for_player:	return "look for player";
  case npc_heal_player:		return "heal player";
  case npc_follow_player:	return "follow player";
  case npc_talk_to_player:	return "talk to player";
  case npc_flee_player:		return "flee player";
  case npc_mug_player:		return "mug player";
  case npc_melee_player:	return "melee player";
  case npc_shoot_player:	return "shoot player";
  case npc_alt_attack_player:	return "alt attack player";
  case npc_goto_destination:	return "goto destination";
  default:			return "unknown";
 }
};

