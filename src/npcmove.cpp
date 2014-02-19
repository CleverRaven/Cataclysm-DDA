#include <sstream>
#include "npc.h"
#include "rng.h"
#include "game.h"
#include "line.h"
#include "debug.h"
#include "overmapbuffer.h"

#define dbg(x) dout((DebugLevel)(x),D_NPC) << __FILE__ << ":" << __LINE__ << ": "
#define TARGET_PLAYER -2

// A list of items used for escape, in order from least to most valuable
#ifndef NUM_ESCAPE_ITEMS
#define NUM_ESCAPE_ITEMS 11
itype_id ESCAPE_ITEMS[NUM_ESCAPE_ITEMS] = {
 "cola", "caffeine", "energy_drink", "canister_goo", "smokebomb",
 "smokebomb_act", "adderall", "coke", "meth", "teleporter",
 "pheromone"
};
#endif

// A list of alternate attack items (e.g. grenades), from least to most valuable
#ifndef NUM_ALT_ATTACK_ITEMS
#define NUM_ALT_ATTACK_ITEMS 18
itype_id ALT_ATTACK_ITEMS[NUM_ALT_ATTACK_ITEMS] = {
 "knife_combat", "spear_wood", "molotov", "pipebomb", "grenade",
 "gasbomb", "bot_manhack", "tazer", "dynamite", "granade", "mininuke",
 "molotov_lit", "pipebomb_act", "grenade_act", "gasbomb_act",
 "dynamite_act", "granade_act", "mininuke_act"
};
#endif

const int avoidance_vehicles_radius = 5;

std::string npc_action_name(npc_action action);
bool thrown_item(item *used);

// Used in npc::drop_items()
struct ratio_index
{
 double ratio;
 int index;
 ratio_index(double R, int I) : ratio (R), index (I) {};
};

// class npc functions!

void npc::move()
{
 npc_action action = npc_undecided;
 int danger = 0, total_danger = 0, target = -1;

 choose_monster_target(target, danger, total_danger);
 if (g->debugmon)
  debugmsg("NPC %s: target = %d, danger = %d, range = %d",
           name.c_str(), target, danger, confident_range(-1));

 if (is_enemy()) {
  int pl_danger = player_danger( &(g->u) );
  if (pl_danger > danger || target == -1) {
   target = TARGET_PLAYER;
   danger = pl_danger;
   if (g->debugmon)
    debugmsg("NPC %s: Set target to PLAYER, danger = %d", name.c_str(), danger);
  }
 }
// TODO: Place player-aiding actions here, with a weight

 /* NPCs are fairly suicidal so at this point we will do a quick check to see if
  * something nasty is going to happen.
  */

 int vehicle = vehicle_danger(avoidance_vehicles_radius);
 if (vehicle) {
  // TODO: Think about how this actually needs to work, for now assume flee from player
  target = TARGET_PLAYER;
 }

 // TODO: morale breaking when surrounded by hostiles
 //if (!bravery_check(danger) || !bravery_check(total_danger) ||
 // TODO: near by active explosives spotted

 if (target == TARGET_PLAYER && attitude == NPCATT_FLEE)
  action = method_of_fleeing(target);
 else if (danger > 0 || (target == TARGET_PLAYER && attitude == NPCATT_KILL))
  action = method_of_attack(target, danger);

 else { // No present danger
  action = address_needs(danger);
  if (g->debugmon)
   debugmsg("address_needs %s", npc_action_name(action).c_str());
  if (action == npc_undecided)
   action = address_player();
  if (g->debugmon)
   debugmsg("address_player %s", npc_action_name(action).c_str());
  if (action == npc_undecided) {
   if (mission == NPC_MISSION_SHELTER || has_disease("infection"))
    action = npc_pause;
   else if (has_new_items)
    action = scan_new_items(target);
   else if (!fetching_item)
    find_item();
   if (g->debugmon)
    debugmsg("find_item %s", npc_action_name(action).c_str());
   // check if in vehicle before rushing off to fetch things
   if (is_following() && g->u.in_vehicle)
    action = npc_follow_embarked;
   else if (fetching_item) // Set to true if find_item() found something
    action = npc_pickup;
   else if (is_following()) // No items, so follow the player?
    action = npc_follow_player;
   else // Do our long-term action
    action = long_term_goal_action();
   if (g->debugmon)
    debugmsg("long_term_goal_action %s", npc_action_name(action).c_str());
  }
 }

/* Sometimes we'll be following the player at this point, but close enough that
 * "following" means standing still.  If that's the case, if there are any
 * monsters around, we should attack them after all!
 *
 * If we are following a embarked player and we are in a vehicle then shoot anyway
 * as we are most likely riding shotgun
 */
 if (danger > 0 && (
     (action == npc_follow_embarked && in_vehicle) ||
     (action == npc_follow_player &&
       rl_dist(posx, posy, g->u.posx, g->u.posy) <= follow_distance())
    ))
  action = method_of_attack(target, danger);

 if (g->debugmon)
  debugmsg("%s chose action %s.", name.c_str(), npc_action_name(action).c_str());

 execute_action(action, target);
}

void npc::execute_action(npc_action action, int target)
{
 int oldmoves = moves;
 int tarx = posx, tary = posy;
 int linet, light = g->light_level();
 if (target == -2) {
  tarx = g->u.posx;
  tary = g->u.posy;
 } else if (target >= 0) {
  tarx = g->zombie(target).posx();
  tary = g->zombie(target).posy();
 }
/*
  debugmsg("%s ran execute_action() with target = %d! Action %s",
           name.c_str(), target, npc_action_name(action).c_str());
*/

 std::vector<point> line;
 if (tarx != posx || tary != posy) {
  int linet, dist = sight_range(g->light_level());
  if (g->m.sees(posx, posy, tarx, tary, dist, linet))
   line = line_to(posx, posy, tarx, tary, linet);
  else
   line = line_to(posx, posy, tarx, tary, 0);
 }

 switch (action) {

 case npc_pause:
  move_pause();
  break;

 case npc_reload: {
  moves -= weapon.reload_time(*this);
  int ammo_index = weapon.pick_reload_ammo(*this, false);
  if (!weapon.reload(*this, ammo_index))
   debugmsg("NPC reload failed.");
  recoil = 6;
  if (g->u_see(posx, posy)) {
   g->add_msg(_("%s reloads their %s."), name.c_str(), weapon.tname().c_str());
   }
  } break;

 case npc_sleep:
/* TODO: Open a dialogue with the player, allowing us to ask if it's alright if
 * we get some sleep, how long watch shifts should be, etc.
 */
  //add_disease("lying_down", 300);
  if (is_friend() && g->u_see(posx, posy))
   say(_("I'm going to sleep."));
  break;

 case npc_pickup:
  pick_up_item();
  break;

 case npc_escape_item:
  use_escape_item(choose_escape_item());
  break;

 case npc_wield_melee:
  wield_best_melee();
  break;

 case npc_wield_loaded_gun:
 {
  item& it = inv.most_loaded_gun();
  if (it.is_null()) {
   debugmsg("NPC tried to wield a loaded gun, but has none!");
   move_pause();
  } else
   wield(it.invlet);
 } break;

 case npc_wield_empty_gun:
 {
  bool ammo_found = false;
  int index = -1;
  invslice slice = inv.slice();
  for (int i = 0; i < slice.size(); i++) {
   item& it = slice[i]->front();
   bool am = (it.is_gun() &&
             has_ammo( (dynamic_cast<it_gun*>(it.type))->ammo ).size() > 0);
   if (it.is_gun() && (!ammo_found || am)) {
    index = i;
    ammo_found = (ammo_found || am);
   }
  }
  if (index == -1) {
   debugmsg("NPC tried to wield a gun, but has none!");
   move_pause();
  } else
   wield(slice[index]->front().invlet);
 } break;

 case npc_heal:
  heal_self();
  break;

 case npc_use_painkiller:
  use_painkiller();
  break;

 case npc_eat:
  pick_and_eat();
  break;

 case npc_drop_items:
/*
  drop_items(weight_carried() - weight_capacity(),
                volume_carried() - volume_capacity());
*/
  move_pause();
  break;

 case npc_flee:
// TODO: More intelligent fleeing
  move_away_from(tarx, tary);
  break;

 case npc_melee:
  update_path(tarx, tary);
  if (path.size() > 1)
   move_to_next();
  else if (path.size() == 1) {
   if (target >= 0)
    melee_monster(target);
   else if (target == TARGET_PLAYER)
    melee_player(g->u);
  } else
   look_for_player(g->u);
  break;

 case npc_shoot:
  g->fire(*this, tarx, tary, line, false);
  break;

 case npc_shoot_burst:
  g->fire(*this, tarx, tary, line, true);
  break;

 case npc_alt_attack:
  alt_attack(target);
  break;

 case npc_look_for_player:
  if (saw_player_recently() && g->m.sees(posx, posy, plx, ply, light, linet)) {
// (plx, ply) is the point where we last saw the player
   update_path(plx, ply);
   move_to_next();
  } else
   look_for_player(g->u);
  break;

 case npc_heal_player:
  update_path(g->u.posx, g->u.posy);
  if (path.size() == 1) // We're adjacent to u, and thus can heal u
   heal_player(g->u);
  else if (path.size() > 0)
   move_to_next();
  else
   move_pause();
  break;

 case npc_follow_player:
  update_path(g->u.posx, g->u.posy);
  if (path.size() <= follow_distance()) // We're close enough to u.
   move_pause();
  else if (path.size() > 0)
   move_to_next();
  else
   move_pause();
  break;

 case npc_follow_embarked:
  if (in_vehicle)
   move_pause();
  else {
   int p1;
   vehicle *veh = g->m.veh_at(g->u.posx, g->u.posy, p1);

   if (!veh) {
    debugmsg("Following an embarked player with no vehicle at their location?");
    // TODO: change to wait? - for now pause
    move_pause();
   } else {
    int p2 = veh->free_seat();
    if (p2 < 0) {
     // TODO: be angry at player, switch to wait or leave - for now pause
     move_pause();
    } else {
     int px = veh->global_x() + veh->parts[p2].precalc_dx[0];
     int py = veh->global_y() + veh->parts[p2].precalc_dy[0];
     update_path(px, py);

     // TODO: replace extra hop distance with finding the correct door
     //       Hop in the last few squares is mostly to avoid player clash
     if (path.size() <= 2) {
      g->m.board_vehicle(px, py, this);
      move_pause();
     } else
      move_to_next();
    }
   }
  }
  break;

 case npc_talk_to_player:
  talk_to_u();
  moves = 0;
  break;

 case npc_mug_player:
  update_path(g->u.posx, g->u.posy);
  if (path.size() == 1) // We're adjacent to u, and thus can mug u
   mug_player(g->u);
  else if (path.size() > 0)
   move_to_next();
  else
   move_pause();
  break;

 case npc_goto_destination:
  go_to_destination();
  break;

 case npc_avoid_friendly_fire:
  avoid_friendly_fire(target);
  break;

 case npc_base_idle:
  // TODO: patrol or sleep or something?
  move_pause();
  break;

 default:
  debugmsg("Unknown NPC action (%d)", action);
 }

 if (oldmoves == moves) {
  dbg(D_ERROR) << "map::veh_at: NPC didn't use its moves.";
  debugmsg("NPC didn't use its moves.  Action %d.  Turning on debug mode.", action);
  g->debugmon = true;
 }
}

void npc::choose_monster_target(int &enemy, int &danger,
                                int &total_danger)
{
 int linet = 0;
 bool defend_u = g->sees_u(posx, posy, linet) && is_defending();
 int highest_priority = 0;
 total_danger = 0;

 for (int i = 0; i < g->num_zombies(); i++) {
  monster *mon = &(g->zombie(i));
  if (this->sees(mon, linet)) {
   int distance = (100 * rl_dist(posx, posy, mon->posx(), mon->posy())) /
                  mon->speed;
   double hp_percent = (mon->type->hp - mon->hp) / mon->type->hp;
   int priority = mon->type->difficulty * (1 + hp_percent) - distance;
   int monster_danger = (mon->type->difficulty * mon->hp) / mon->type->hp;
   if (!mon->is_fleeing(*this))
    monster_danger++;

   if (mon->friendly != 0) {
    priority = -999;
    monster_danger *= -1;
   }/* else if (mon->speed < current_speed()) {
    priority -= 10;
    monster_danger -= 10;
   } else
    priority *= 1 + (.1 *use_escape_item distance);
*/
   total_danger += int(monster_danger / (distance == 0 ? 1 : distance));

   bool okay_by_rules = true;
   if (is_following()) {
    switch (combat_rules.engagement) {
     case ENGAGE_NONE:
      okay_by_rules = false;
      break;
     case ENGAGE_CLOSE:
      okay_by_rules = (distance <= 6);
      break;
     case ENGAGE_WEAK:
      okay_by_rules = (mon->hp <= average_damage_dealt());
      break;
     case ENGAGE_HIT:
      okay_by_rules = (mon->has_effect("hit_by_player"));
      break;
    }
   }

   if (okay_by_rules && monster_danger > danger) {
    danger = monster_danger;
    if (enemy == -1) {
     highest_priority = priority;
     enemy = i;
    }
   }

   if (okay_by_rules && priority > highest_priority) {
    highest_priority = priority;
    enemy = i;
   } else if (okay_by_rules && defend_u) {
    priority = mon->type->difficulty * (1 + hp_percent);
    distance = (100 * rl_dist(g->u.posx, g->u.posy, mon->posx(), mon->posy())) /
               mon->speed;
    priority -= distance;
    if (mon->speed < get_speed())
     priority -= 10;
    priority *= (personality.bravery + personality.altruism + op_of_u.value) /
                15;
    if (priority > highest_priority) {
     highest_priority = priority;
     enemy = i;
    }
   }
  }
 }
}

npc_action npc::method_of_fleeing(int enemy)
{
 int speed = (enemy == TARGET_PLAYER ? g->u.get_speed() :
                                       g->zombie(enemy).speed);
 point enemy_loc = (enemy == TARGET_PLAYER ? point(g->u.posx, g->u.posy) :
                    point(g->zombie(enemy).posx(), g->zombie(enemy).posy()));
 int distance = rl_dist(posx, posy, enemy_loc.x, enemy_loc.y);

 if (choose_escape_item() >= 0) // We have an escape item!
  return npc_escape_item;

 if (speed > 0 && (100 * distance) / speed <= 4 && speed > get_speed())
  return method_of_attack(enemy, -1); // Can't outrun, so attack

 return npc_flee;
}

npc_action npc::method_of_attack(int target, int danger)
{
 int tarx, tary;
 bool can_use_gun = (!is_following() || combat_rules.use_guns);
 bool use_silent = (is_following() && combat_rules.use_silent);

 if (target == TARGET_PLAYER) {
  tarx = g->u.posx;
  tary = g->u.posy;
 } else if (target >= 0) {
  tarx = g->zombie(target).posx();
  tary = g->zombie(target).posy();
 } else { // This function shouldn't be called...
  debugmsg("Ran npc::method_of_attack without a target!");
  return npc_pause;
 }

 int dist = rl_dist(posx, posy, tarx, tary), target_HP;
 if (target == TARGET_PLAYER)
  target_HP = g->u.hp_percentage() * g->u.hp_max[hp_torso];
 else
  target_HP = g->zombie(target).hp;

 if (can_use_gun) {
  if (need_to_reload() && can_reload())
   return npc_reload;
  if (emergency(danger_assessment()) && alt_attack_available())
   return npc_alt_attack;
  if (weapon.is_gun() && (!use_silent || weapon.is_silent()) && weapon.charges > 0) {
   it_gun* gun = dynamic_cast<it_gun*>(weapon.type);
   if (dist > confident_range()) {
    if (can_reload() && (enough_time_to_reload(target, weapon) || in_vehicle))
     return npc_reload;
    else if (in_vehicle && dist > 1)
     return npc_pause;
    else
     return npc_melee;
   }
   int junk = 0;
   if (!wont_hit_friend(tarx, tary))
    if (in_vehicle)
     if (can_reload())
      return npc_reload;
     else
      return npc_pause; // wait for clear shot
    else
     return npc_avoid_friendly_fire;
   else if (rl_dist(posx,posy,tarx,tary) > weapon.range() &&
            g->m.sees( posx, posy, tarx, tary, weapon.range(), junk )) {
       return npc_melee; // If out of range, move closer to the target
   } else if (dist <= confident_range() / 3 && weapon.charges >= gun->burst &&
            gun->burst > 1 &&
            ((weapon.curammo && target_HP >= weapon.curammo->damage * 3) || emergency(danger * 2)))
    return npc_shoot_burst;
   else
    return npc_shoot;
  }
 }

// Check if there's something better to wield
 bool has_empty_gun = false, has_better_melee = false;
 std::vector<item*> empty_guns;
 invslice slice = inv.slice();
 for (int i = 0; i < slice.size(); i++) {
  item& it = slice[i]->front();
  bool allowed = can_use_gun && it.is_gun() && (!use_silent || weapon.is_silent());
  if (allowed && it.charges > 0)
   return npc_wield_loaded_gun;
  else if (allowed && enough_time_to_reload(target, it)) {
   has_empty_gun = true;
   empty_guns.push_back(&it);
  } else if (it.melee_value(this) > weapon.melee_value(this) * 1.1)
   has_better_melee = true;
 }

 bool has_ammo_for_empty_gun = false;
 for (int i = 0; i < empty_guns.size(); i++) {
  for (int j = 0; j < slice.size(); j++) {
   item& it = slice[j]->front();
   if (it.is_ammo() &&
       it.ammo_type() == empty_guns[i]->ammo_type())
    has_ammo_for_empty_gun = true;
  }
 }

 if (has_empty_gun && has_ammo_for_empty_gun)
  return npc_wield_empty_gun;
 else if (has_better_melee)
  return npc_wield_melee;

 if (in_vehicle && dist > 1)
  return npc_pause;
 return npc_melee;
}

npc_action npc::address_needs(int danger)
{
 if (has_healing_item()) {
  for (int i = 0; i < num_hp_parts; i++) {
   hp_part part = hp_part(i);
   if ((part == hp_head  && hp_cur[i] <= 35) ||
       (part == hp_torso && hp_cur[i] <= 25) ||
       hp_cur[i] <= 15)
    return npc_heal;
  }
 }

 if (has_painkiller() && !took_painkiller() && pain - pkill >= 15)
  return npc_use_painkiller;

 if (can_reload())
  return npc_reload;

 if ((danger <= NPC_DANGER_VERY_LOW && (hunger > 40 || thirst > 40)) ||
     thirst > 80 || hunger > 160)
  return npc_eat;

/*
 if (weight_carried() > weight_capacity() ||
     volume_carried() > volume_capacity())
  return npc_drop_items;
*/

/*
 if (danger <= 0 && fatigue > 191)
  return npc_sleep;
*/

// TODO: Mutation & trait related needs
// e.g. finding glasses; getting out of sunlight if we're an albino; etc.

 return npc_undecided;
}

npc_action npc::address_player()
{
 int linet;
 if ((attitude == NPCATT_TALK || attitude == NPCATT_TRADE) &&
     g->sees_u(posx, posy, linet)) {
  if (rl_dist(posx, posy, g->u.posx, g->u.posy) <= 6)
   return npc_talk_to_player; // Close enough to talk to you
  else {
   if (one_in(10))
    say("<lets_talk>");
   return npc_follow_player;
  }
 }

 if (attitude == NPCATT_MUG && g->sees_u(posx, posy, linet)) {
  if (one_in(3))
   say(_("Don't move a <swear> muscle..."));
  return npc_mug_player;
 }

 if (attitude == NPCATT_WAIT_FOR_LEAVE) {
  patience--;
  if (patience <= 0) {
   patience = 0;
   attitude = NPCATT_KILL;
   return method_of_attack(TARGET_PLAYER, player_danger( &(g->u) ));
  }
  return npc_undecided;
 }

 if (attitude == NPCATT_FLEE)
  return npc_flee;

 if (attitude == NPCATT_LEAD) {
  if (rl_dist(posx, posy, g->u.posx, g->u.posy) >= 12 ||
      !g->sees_u(posx, posy, linet)) {
   int intense = disease_intensity("catch_up");
   if (intense < 10) {
    say("<keep_up>");
    add_disease("catch_up", 5, false, 1, 15);
    return npc_pause;
   } else if (intense == 10) {
    say("<im_leaving_you>");
    add_disease("catch_up", 5, false, 1, 15);
    return npc_pause;
   } else
    return npc_goto_destination;
  } else
   return npc_goto_destination;
 }
 return npc_undecided;
}

npc_action npc::long_term_goal_action()
{
 if (g->debugmon)
  debugmsg("long_term_goal_action()");
 path.clear();

 if (mission == NPC_MISSION_SHOPKEEP || mission == NPC_MISSION_SHELTER)
  return npc_pause; // Shopkeeps just stay put.

// TODO: Follow / look for player

 if (mission == NPC_MISSION_BASE)
  return npc_base_idle;

 if (!has_destination())
  set_destination();
 return npc_goto_destination;

 return npc_undecided;
}


bool npc::alt_attack_available()
{
 for (int i = 0; i < NUM_ALT_ATTACK_ITEMS; i++) {
  if ((!is_following() || combat_rules.use_grenades ||
       !(itypes[ALT_ATTACK_ITEMS[i]]->item_tags.count("GRENADE"))) &&
      has_amount(ALT_ATTACK_ITEMS[i], 1))
   return true;
 }
 return false;
}

signed char npc::choose_escape_item()
{
 int best = -1, ret = -1;
 invslice slice = inv.slice();
 for (int i = 0; i < slice.size(); i++) {
  item& it = slice[i]->front();
  for (int j = 0; j < NUM_ESCAPE_ITEMS; j++) {
   it_comest* food = NULL;
   if (it.is_food())
    food = dynamic_cast<it_comest*>(it.type);
   if (it.type->id == ESCAPE_ITEMS[j] &&
       (food == NULL || stim < food->stim ||            // Avoid guzzling down
        (food->stim >= 10 && stim < food->stim * 2)) && //  Adderall etc.
       (j > best || (j == best && it.charges < slice[ret]->front().charges))) {
    ret = i;
    best = j;
    j = NUM_ESCAPE_ITEMS;
   }
  }
 }
 // Protect us from accessing an invalid index.
 if (ret == -1) { return ret; }

 return slice[ret]->front().invlet;
}

void npc::use_escape_item(signed char invlet)
{
 if (invlet == 0) {
  debugmsg("%s tried to use item with null invlet", name.c_str());
  move_pause();
  return;
 }
 if (invlet == -1) {
  // No item found.
  return;
 }

/* There is a static list of items that NPCs consider to be "escape items," so
 * we can just use a switch here to decide what to do based on type.  See
 * ESCAPE_ITEMS, defined in npc.h
 */

 item* used = &(inv.item_by_letter(invlet));

 if (used->is_food() || used->is_food_container()) {
  consume(invlet);
  return;
 }

 if (used->is_tool()) {
  it_tool* tool = dynamic_cast<it_tool*>(used->type);
  int charges_used = tool->use.call(this, used, false);
  if( charges_used ) {
      used->charges -= charges_used;
      if (used->invlet == 0) {
          inv.remove_item(used);
      }
  }
  return;
 }

 debugmsg("NPC tried to use %s (%c) but it has no use?", used->tname().c_str(),
          invlet);
 move_pause();
}

// Index defaults to 0, i.e., wielded weapon
int npc::confident_range(char invlet)
{

 if (invlet == 0 && (!weapon.is_gun() || weapon.charges <= 0))
  return 1;

 double deviation = 0;
 int max = 0;
 if (invlet == 0) {
  it_gun* firing = dynamic_cast<it_gun*>(weapon.type);
// We want at least 50% confidence that missed_by will be < .5.
// missed_by = .00325 * deviation * range <= .5; deviation * range <= 156
// (range <= 156 / deviation) is okay, so confident range is (156 / deviation)
// Here we're using max values for deviation followed by *.5, for around-50% estimate.
// See game::fire (ranged.cpp) for where these computations come from

  if (skillLevel(firing->skill_used) < 8) {
    deviation += 3 * (8 - skillLevel(firing->skill_used));
  }
  if (skillLevel("gun") < 9) {
    deviation += 9 - skillLevel("gun");
  }

  deviation += ranged_dex_mod();
  deviation += ranged_per_mod();

  deviation += 2 * encumb(bp_arms) + 4 * encumb(bp_eyes);

  if (weapon.curammo == NULL) // This shouldn't happen, but it does sometimes
   debugmsg("%s has NULL curammo!", name.c_str()); // TODO: investigate this bug
  else {
   deviation += weapon.curammo->dispersion;
   max = weapon.range();
  }
  deviation += firing->dispersion;
  deviation += recoil;

 } else { // We aren't firing a gun, we're throwing something!

  item *thrown = &(inv.item_by_letter(invlet));
  max = throw_range(invlet); // The max distance we can throw
  deviation = 0;
  if (skillLevel("throw") < 8)
   deviation += 8 - skillLevel("throw");
  else
   deviation -= skillLevel("throw") - 6;

  deviation += throw_dex_mod();

  if (per_cur < 6)
   deviation += 8 - per_cur;
  else if (per_cur > 8)
   deviation -= per_cur - 8;

  deviation += encumb(bp_hands) * 2 + encumb(bp_eyes) + 1;
  if (thrown->volume() > 5)
   deviation += 1 + (thrown->volume() - 5) / 4;
  if (thrown->volume() == 0)
   deviation += 3;

  deviation += 1 + abs(str_cur - (thrown->weight() / 113));
 }
 //Account for rng's, *.5 for 50%
 deviation /= 2;

// Using 180 for now for extra-confident NPCs.
 int ret = (max > int(180 / deviation) ? max : int(180 / deviation));
 if (weapon.curammo && ret > weapon.range(this))
  return weapon.range(this);
 return ret;
}

// Index defaults to -1, i.e., wielded weapon
bool npc::wont_hit_friend(int tarx, int tary, char invlet)
{
 int linet = 0, dist = sight_range(g->light_level());
 int confident = confident_range(invlet);
 if (rl_dist(posx, posy, tarx, tary) == 1)
  return true; // If we're *really* sure that our aim is dead-on

 std::vector<point> traj;
 if (g->m.sees(posx, posy, tarx, tary, dist, linet))
  traj = line_to(posx, posy, tarx, tary, linet);
 else
  traj = line_to(posx, posy, tarx, tary, 0);

 for (int i = 0; i < traj.size(); i++) {
  int dist = rl_dist(posx, posy, traj[i].x, traj[i].y);
  int deviation = 1 + int(dist / confident);
  for (int x = traj[i].x - deviation; x <= traj[i].x + deviation; x++) {
   for (int y = traj[i].y - deviation; y <= traj[i].y + deviation; y++) {
// Hit the player?
    if (is_friend() && g->u.posx == x && g->u.posy == y)
     return false;
// Hit a friendly monster?
/*
    for (int n = 0; n < g->num_zombies(); n++) {
     if (g->zombie(n).friendly != 0 && g->zombie(n).posx == x && g->zombie(n).posy == y)
      return false;
    }
*/
// Hit an NPC that's on our team?
/*
    for (int n = 0; n < g->active_npc.size(); n++) {
     npc* guy = &(g->active_npc[n]);
     if (guy != this && (is_friend() == guy->is_friend()) &&
         guy->posx == x && guy->posy == y)
      return false;
    }
*/
   }
  }
 }
 return true;
}

bool npc::can_reload()
{
 if (!weapon.is_gun())
  return false;
 it_gun* gun = dynamic_cast<it_gun*> (weapon.type);
 return (weapon.charges < gun->clip && has_ammo(gun->ammo).size() > 0);
}

bool npc::need_to_reload()
{
 if (!weapon.is_gun())
  return false;
 it_gun* gun = dynamic_cast<it_gun*> (weapon.type);

 return (weapon.charges < gun->clip * .1);
}

bool npc::enough_time_to_reload(int target, item &gun)
{
 int rltime = gun.reload_time(*this);
 double turns_til_reloaded = rltime / get_speed();
 int dist, speed, linet;

 if (target == TARGET_PLAYER) {
  if (g->sees_u(posx, posy, linet) && g->u.weapon.is_gun() && rltime > 200)
   return false; // Don't take longer than 2 turns if player has a gun
  dist = rl_dist(posx, posy, g->u.posx, g->u.posy);
  speed = speed_estimate(g->u.get_speed());
 } else if (target >= 0) {
  dist = rl_dist(posx, posy, g->zombie(target).posx(), g->zombie(target).posy());
  speed = speed_estimate(g->zombie(target).speed);
 } else
  return true; // No target, plenty of time to reload

 double turns_til_reached = (dist * 100) / speed;

 return (turns_til_reloaded < turns_til_reached);
}

void npc::update_path(int x, int y)
{
 if (path.empty()) {
  path = g->m.route(posx, posy, x, y);
  return;
 }
 point last = path[path.size() - 1];
 if (last.x == x && last.y == y)
  return; // Our path already leads to that point, no need to recalculate
 path = g->m.route(posx, posy, x, y);
 if (!path.empty() && path[0].x == posx && path[0].y == posy)
  path.erase(path.begin());
}

bool npc::can_move_to(int x, int y)
{
 return ((g->m.move_cost(x, y) > 0 || g->m.has_flag("BASHABLE", x, y)) &&
     rl_dist(posx, posy, x, y) <= 1);
}

void npc::move_to(int x, int y)
{

 if (has_effect("downed")) {
  moves -= 100;
  return;
 }
 if (has_disease("bouldering")) {
  moves -= 20;
  if (moves < 0)
   moves = 0;
 }
 if (recoil > 0) { // Start by dropping recoil a little
  if (int(str_cur / 2) + skillLevel("gun") >= recoil)
   recoil = 0;
  else {
   recoil -= int(str_cur / 2) + skillLevel("gun");
   recoil = int(recoil / 2);
  }
 }
 if (has_effect("stunned")) {
  x = rng(posx - 1, posx + 1);
  y = rng(posy - 1, posy + 1);
 }
 if (rl_dist(posx, posy, x, y) > 1) {
/*
  debugmsg("Tried to move_to more than one space! (%d, %d) to (%d, %d)",
           posx, posy, x, y);
  debugmsg("Route is size %d.", path.size());
*/
  int linet;
  std::vector<point> newpath;
  if (g->m.sees(posx, posy, x, y, -1, linet))
   newpath = line_to(posx, posy, x, y, linet);
  x = newpath[0].x;
  y = newpath[0].y;
 }
 if (x == posx && y == posy) { // We're just pausing!
  moves -= 100;
 } else if (g->mon_at(x, y) != -1) { // Shouldn't happen, but it might.
  //monster *m = &(g->zombie(g->mon_at(x, y)));
  //debugmsg("Bumped into a monster, %d, a %s",g->mon_at(x, y),m->name().c_str());
  melee_monster(g->mon_at(x, y));
 } else if (g->u.posx == x && g->u.posy == y) {
  say("<let_me_pass>");
  moves -= 100;
 } else if (g->npc_at(x, y) != -1) {
// TODO: Determine if it's an enemy NPC (hit them), or a friendly in the way
  moves -= 100;
 } else {
  if (in_vehicle) {
      // TODO: handle this nicely - npcs should not jump from moving vehicles
      g->m.unboard_vehicle(posx, posy);
  }
  else
  {
     vehicle *tmp = g->m.veh_at(x, y);
     if(tmp != NULL) {
         if(tmp->velocity > 0) {
             moves -= 100;
             return;
         }
     }
 }
  if (g->m.move_cost(x, y) > 0) {
  posx = x;
  posy = y;
  bool diag = trigdist && posx != x && posy != y;
  moves -= run_cost(g->m.combined_movecost(posx, posy, x, y), diag);
 } else if (g->m.open_door(x, y, (g->m.ter(posx, posy) == t_floor)))
  moves -= 100;
 else if (g->m.has_flag("BASHABLE", x, y)) {
  moves -= 110;
  std::string bashsound;
  int smashskill = int(str_cur / 2 + weapon.type->melee_dam);
  g->m.bash(x, y, smashskill, bashsound);
  g->sound(x, y, 18, bashsound);
 } else {
     int frubble = g->m.get_field_strength( point(x, y), fd_rubble );
     if (frubble > 0 ) {
        g->u.add_disease("bouldering", 100, false, frubble, 3);
     } else {
        g->u.rem_disease("bouldering");
     }
     moves -= 100;
  }
 }
}

void npc::move_to_next()
{
 if (path.empty()) {
 if (g->debugmon)
   debugmsg("npc::move_to_next() called with an empty path!");
  move_pause();
  return;
 }
 while (posx == path[0].x && posy == path[0].y)
  path.erase(path.begin());
 move_to(path[0].x, path[0].y);
 if (posx == path[0].x && posy == path[0].y) // Move was successful
  path.erase(path.begin());
}

// TODO: Rewrite this.  It doesn't work well and is ugly.
void npc::avoid_friendly_fire(int target)
{
 int tarx, tary;
 if (target == TARGET_PLAYER) {
  tarx = g->u.posx;
  tary = g->u.posy;
 } else if (target >= 0) {
  tarx = g->zombie(target).posx();
  tary = g->zombie(target).posy();
  if (!one_in(3))
   say(_("<move> so I can shoot that %s!"), g->zombie(target).name().c_str());
 } else {
  debugmsg("npc::avoid_friendly_fire() called with no target!");
  move_pause();
  return;
 }

 int xdir = (tarx > posx ? 1 : -1), ydir = (tary > posy ? 1 : -1);
 direction dir_to_target = direction_from(posx, posy, tarx, tary);
 std::vector<point> valid_moves;
/* Ugh, big ugly switch.  This fills valid_moves with a list of moves from most
 * desirable to least; the only two moves excluded are those along the line of
 * sight.
 * TODO: Use some math instead of a big ugly switch.
 */
 switch (dir_to_target) {
 case NORTH:
  valid_moves.push_back(point(posx + xdir, posy));
  valid_moves.push_back(point(posx - xdir, posy));
  valid_moves.push_back(point(posx + xdir, posy + 1));
  valid_moves.push_back(point(posx - xdir, posy + 1));
  valid_moves.push_back(point(posx + xdir, posy - 1));
  valid_moves.push_back(point(posx - xdir, posy - 1));
  break;
 case NORTHEAST:
  valid_moves.push_back(point(posx + 1, posy + 1));
  valid_moves.push_back(point(posx - 1, posy - 1));
  valid_moves.push_back(point(posx - 1, posy    ));
  valid_moves.push_back(point(posx    , posy + 1));
  valid_moves.push_back(point(posx + 1, posy    ));
  valid_moves.push_back(point(posx    , posy - 1));
  break;
 case EAST:
  valid_moves.push_back(point(posx, posy - 1));
  valid_moves.push_back(point(posx, posy + 1));
  valid_moves.push_back(point(posx - 1, posy - 1));
  valid_moves.push_back(point(posx - 1, posy + 1));
  valid_moves.push_back(point(posx + 1, posy - 1));
  valid_moves.push_back(point(posx + 1, posy + 1));
  break;
 case SOUTHEAST:
  valid_moves.push_back(point(posx + 1, posy - 1));
  valid_moves.push_back(point(posx - 1, posy + 1));
  valid_moves.push_back(point(posx + 1, posy    ));
  valid_moves.push_back(point(posx    , posy + 1));
  valid_moves.push_back(point(posx - 1, posy    ));
  valid_moves.push_back(point(posx    , posy - 1));
  break;
 case SOUTH:
  valid_moves.push_back(point(posx + xdir, posy));
  valid_moves.push_back(point(posx - xdir, posy));
  valid_moves.push_back(point(posx + xdir, posy - 1));
  valid_moves.push_back(point(posx - xdir, posy - 1));
  valid_moves.push_back(point(posx + xdir, posy + 1));
  valid_moves.push_back(point(posx - xdir, posy + 1));
  break;
 case SOUTHWEST:
  valid_moves.push_back(point(posx + 1, posy + 1));
  valid_moves.push_back(point(posx - 1, posy - 1));
  valid_moves.push_back(point(posx + 1, posy    ));
  valid_moves.push_back(point(posx    , posy - 1));
  valid_moves.push_back(point(posx - 1, posy    ));
  valid_moves.push_back(point(posx    , posy + 1));
  break;
 case WEST:
  valid_moves.push_back(point(posx    , posy + ydir));
  valid_moves.push_back(point(posx    , posy - ydir));
  valid_moves.push_back(point(posx + 1, posy + ydir));
  valid_moves.push_back(point(posx + 1, posy - ydir));
  valid_moves.push_back(point(posx - 1, posy + ydir));
  valid_moves.push_back(point(posx - 1, posy - ydir));
  break;
 case NORTHWEST:
  valid_moves.push_back(point(posx + 1, posy - 1));
  valid_moves.push_back(point(posx - 1, posy + 1));
  valid_moves.push_back(point(posx - 1, posy    ));
  valid_moves.push_back(point(posx    , posy - 1));
  valid_moves.push_back(point(posx + 1, posy    ));
  valid_moves.push_back(point(posx    , posy + 1));
  break;
 }

 for (int i = 0; i < valid_moves.size(); i++) {
  if (can_move_to(valid_moves[i].x, valid_moves[i].y)) {
   move_to(valid_moves[i].x, valid_moves[i].y);
   return;
  }
 }

/* If we're still in the function at this point, maneuvering can't help us. So,
 * might as well address some needs.
 * We pass a <danger> value of NPC_DANGER_VERY_LOW + 1 so that we won't start
 * eating food (or, god help us, sleeping).
 */
 npc_action action = address_needs(NPC_DANGER_VERY_LOW + 1);
 if (action == npc_undecided)
  move_pause();
 execute_action(action, target);
}

void npc::move_away_from(int x, int y)
{
 std::vector<point> options;
 int dx = 0, dy = 0;
 if (x < posx)
  dx = 1;
 else if (x > posx)
  dx = -1;
 if (y < posy)
  dy = 1;
 else if (y < posy)
  dy = -1;

 options.push_back( point(posx + dx, posy + dy) );
 if (abs(x - posx) > abs(y - posy)) {
  options.push_back( point(posx + dx, posy) );
  options.push_back( point(posx, posy + dy) );
  options.push_back( point(posx + dx, posy - dy) );
 } else {
  options.push_back( point(posx, posy + dy) );
  options.push_back( point(posx + dx, posy) );
  options.push_back( point(posx - dx, posy + dy) );
 }

 for (int i = 0; i < options.size(); i++) {
  if (can_move_to(options[i].x, options[i].y))
   move_to(options[i].x, options[i].y);
 }
 move_pause();
}

void npc::move_pause()
{
 moves = 0;
 if (recoil > 0) {
  if (str_cur + 2 * skillLevel("gun") >= recoil)
   recoil = 0;
  else {
   recoil -= str_cur + 2 * skillLevel("gun");
   recoil = int(recoil / 2);
  }
 }
}

void npc::find_item()
{
 fetching_item = false;
 int best_value = minimum_item_value();
 int range = sight_range(g->light_level());
 if (range > 12)
  range = 12;
 int minx = posx - range, maxx = posx + range,
     miny = posy - range, maxy = posy + range;
 int index = -1, linet;
 if (minx < 0)
  minx = 0;
 if (miny < 0)
  miny = 0;
 if (maxx >= SEEX * MAPSIZE)
  maxx = SEEX * MAPSIZE - 1;
 if (maxy >= SEEY * MAPSIZE)
  maxy = SEEY * MAPSIZE - 1;

 for (int x = minx; x <= maxx; x++) {
  for (int y = miny; y <= maxy; y++) {
   if (g->m.sees(posx, posy, x, y, range, linet)) {
    for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
     int itval = value(g->m.i_at(x, y)[i]);
     int wgt = g->m.i_at(x, y)[i].weight(), vol = g->m.i_at(x, y)[i].volume();
     if (itval > best_value &&
         //(itval > worst_item_value ||
          (can_pickWeight(wgt) && can_pickVolume(vol))) {
      itx = x;
      ity = y;
      index = i;
      best_value = itval;
      fetching_item = true;
     }
    }
   }
  }
 }

 if (fetching_item && is_following())
  say(_("Hold on, I want to pick up that %s."),
      g->m.i_at(itx, ity)[index].tname().c_str());
}

void npc::pick_up_item()
{
 if (g->debugmon) {
  debugmsg("%s::pick_up_item(); [%d, %d] => [%d, %d]", name.c_str(), posx, posy,
           itx, ity);
 }
 update_path(itx, ity);

 if (path.size() > 1) {
  if (g->debugmon)
   debugmsg("Moving; [%d, %d] => [%d, %d]", posx, posy, path[0].x, path[0].y);
  move_to_next();
  return;
 }
// We're adjacent to the item; grab it!
 moves -= 100;
 fetching_item = false;
 std::vector<item> *items = &(g->m.i_at(itx, ity));
 int total_volume = 0, total_weight = 0; // How much the items will add
 std::vector<int> pickup; // Indices of items we want

 for (int i = 0; i < items->size(); i++) {
  int itval = value((*items)[i]), vol = (*items)[i].volume(),
      wgt = (*items)[i].weight();
  if (itval >= minimum_item_value() &&// (itval >= worst_item_value ||
      (can_pickVolume(total_volume + vol) &&
       can_pickWeight(total_weight + wgt))) {
   pickup.push_back(i);
   total_volume += vol;
   total_weight += wgt;
  }
 }
/*
 if (total_volume + volume_carried() > volume_capacity() ||
     total_weight + weight_carried() > weight_capacity()) {
  int wgt_to_drop = weight_carried() + total_weight - weight_capacity();
  int vol_to_drop = volume_carried() + total_volume - volume_capacity();
  drop_items(wgt_to_drop, vol_to_drop);
 }
*/
// Describe the pickup to the player
 bool u_see_me = g->u_see(posx, posy), u_see_items = g->u_see(itx, ity);
 if (u_see_me) {
  if (pickup.size() == 1) {
   if (u_see_items)
    g->add_msg(_("%s picks up a %s."), name.c_str(),
               (*items)[pickup[0]].tname().c_str());
   else
    g->add_msg(_("%s picks something up."), name.c_str());
  } else if (pickup.size() == 2) {
   if (u_see_items)
    g->add_msg(_("%s picks up a %s and a %s."), name.c_str(),
               (*items)[pickup[0]].tname().c_str(),
               (*items)[pickup[1]].tname().c_str());
   else
    g->add_msg(_("%s picks up a couple of items."), name.c_str());
  } else
   g->add_msg(_("%s picks up several items."), name.c_str());
 } else if (u_see_items) {
  if (pickup.size() == 1)
   g->add_msg(_("Someone picks up a %s."), (*items)[pickup[0]].tname().c_str());
  else if (pickup.size() == 2)
   g->add_msg(_("Someone picks up a %s and a %s"),
              (*items)[pickup[0]].tname().c_str(),
              (*items)[pickup[1]].tname().c_str());
  else
   g->add_msg(_("Someone picks up several items."));
 }

 for (int i = 0; i < pickup.size(); i++) {
  int itval = value((*items)[pickup[i]]);
  if (itval < worst_item_value)
   worst_item_value = itval;
  i_add((*items)[pickup[i]]);
 }
 for (int i = 0; i < pickup.size(); i++) {
  g->m.i_rem(itx, ity, pickup[i]);
  for (int j = i + 1; j < pickup.size(); j++) // Fix indices
   pickup[j]--;
 }
}

void npc::drop_items(int weight, int volume)
{
 if (g->debugmon) {
  debugmsg("%s is dropping items-%d,%d (%d items, wgt %d/%d, vol %d/%d)",
           name.c_str(), weight, volume, inv.size(), weight_carried(),
           weight_capacity(), volume_carried(), volume_capacity());
 }

 int weight_dropped = 0, volume_dropped = 0;
 std::vector<ratio_index> rWgt, rVol; // Weight/Volume to value ratios

// First fill our ratio vectors, so we know which things to drop first
 invslice slice = inv.slice();
 for (int i = 0; i < slice.size(); i++) {
  item& it = slice[i]->front();
  double wgt_ratio, vol_ratio;
  if (value(it) == 0) {
   wgt_ratio = 99999;
   vol_ratio = 99999;
  } else {
   wgt_ratio = it.weight() / value(it);
   vol_ratio = it.volume() / value(it);
  }
  bool added_wgt = false, added_vol = false;
  for (int j = 0; j < rWgt.size() && !added_wgt; j++) {
   if (wgt_ratio > rWgt[j].ratio) {
    added_wgt = true;
    rWgt.insert(rWgt.begin() + j, ratio_index(wgt_ratio, i));
   }
  }
  if (!added_wgt)
   rWgt.push_back(ratio_index(wgt_ratio, i));
  for (int j = 0; j < rVol.size() && !added_vol; j++) {
   if (vol_ratio > rVol[j].ratio) {
    added_vol = true;
    rVol.insert(rVol.begin() + j, ratio_index(vol_ratio, i));
   }
  }
  if (!added_vol)
   rVol.push_back(ratio_index(vol_ratio, i));
 }

 std::stringstream item_name; // For description below
 int num_items_dropped = 0; // For description below
// Now, drop items, starting from the top of each list
 while (weight_dropped < weight || volume_dropped < volume) {
// weight and volume may be passed as 0 or a negative value, to indicate that
// decreasing that variable is not important.
  int dWeight = (weight <= 0 ? -1 : weight - weight_dropped);
  int dVolume = (volume <= 0 ? -1 : volume - volume_dropped);
  int index;
// Which is more important, weight or volume?
  if (dWeight > dVolume) {
   index = rWgt[0].index;
   rWgt.erase(rWgt.begin());
// Fix the rest of those indices.
   for (int i = 0; i < rWgt.size(); i++) {
    if (rWgt[i].index > index)
     rWgt[i].index--;
   }
  } else {
   index = rVol[0].index;
   rVol.erase(rVol.begin());
// Fix the rest of those indices.
   for (int i = 0; i < rVol.size(); i++) {
    if (i > rVol.size())
     debugmsg("npc::drop_items() - looping through rVol - Size is %d, i is %d",
              rVol.size(), i);
    if (rVol[i].index > index)
     rVol[i].index--;
   }
  }
  weight_dropped += slice[index]->front().weight();
  volume_dropped += slice[index]->front().volume();
  item dropped = i_remn(slice[index]->front().invlet);
  num_items_dropped++;
  if (num_items_dropped == 1)
   item_name << dropped.tname();
  else if (num_items_dropped == 2)
   item_name << _(" and ") << dropped.tname();
  g->m.add_item_or_charges(posx, posy, dropped);
 }
// Finally, describe the action if u can see it
 std::string item_name_str = item_name.str();
 if (g->u_see(posx, posy)) {
  if (num_items_dropped >= 3)
   g->add_msg(ngettext("%s drops %d item.", "%s drops %d items.", num_items_dropped), name.c_str(), num_items_dropped);
  else
   g->add_msg(_("%s drops a %s."), name.c_str(), item_name_str.c_str());
 }
 update_worst_item_value();
}

npc_action npc::scan_new_items(int target)
{
 bool can_use_gun = (!is_following() || combat_rules.use_guns);
 bool use_silent = (is_following() && combat_rules.use_silent);
 invslice slice = inv.slice();

// Check if there's something better to wield
 bool has_empty_gun = false, has_better_melee = false;
 std::vector<item*> empty_guns;
 for (int i = 0; i < slice.size(); i++) {
  item& it = slice[i]->front();
  bool allowed = can_use_gun && it.is_gun() && (!use_silent || it.is_silent());
  if (allowed && it.charges > 0)
   return npc_wield_loaded_gun;
  else if (allowed && enough_time_to_reload(target, it)) {
   has_empty_gun = true;
   empty_guns.push_back(&it);
  } else if (it.melee_value(this) > weapon.melee_value(this) * 1.1)
   has_better_melee = true;
 }

 bool has_ammo_for_empty_gun = false;
 for (int i = 0; i < empty_guns.size(); i++) {
  for (int j = 0; j < slice.size(); j++) {
   item& it = slice[j]->front();
   if (it.is_ammo() &&
       it.ammo_type() == empty_guns[i]->ammo_type())
    has_ammo_for_empty_gun = true;
  }
 }

 if (has_empty_gun && has_ammo_for_empty_gun)
  return npc_wield_empty_gun;
 else if (has_better_melee)
  return npc_wield_melee;

 return npc_pause;
}

void npc::melee_monster(int target)
{
 monster* monhit = &(g->zombie(target));
 melee_attack(*monhit, true);
}

void npc::melee_player(player &foe)
{
 melee_attack(foe, true);
}

void npc::wield_best_melee()
{
 item& it = inv.best_for_melee(this);
 if (it.is_null()) {
  debugmsg("npc::wield_best_melee failed to find a melee weapon.");
  move_pause();
  return;
 }
 wield(it.invlet);
}

void npc::alt_attack(int target)
{
 itype_id which = "null";
 int tarx, tary;
 if (target == TARGET_PLAYER) {
  tarx = g->u.posx;
  tary = g->u.posy;
 } else if (target >= 0) {
  tarx = g->zombie(target).posx();
  tary = g->zombie(target).posy();
 } else {
  debugmsg("npc::alt_attack() called with target = %d", target);
  move_pause();
  return;
 }
 int dist = rl_dist(posx, posy, tarx, tary);
/* ALT_ATTACK_ITEMS is an array which stores the itype_id of all alternate
 * items, from least to most important.
 * See npc.h for definition of ALT_ATTACK_ITEMS
 */
 for (int i = 0; i < NUM_ALT_ATTACK_ITEMS; i++) {
  if ((!is_following() || combat_rules.use_grenades ||
       !(itypes[ALT_ATTACK_ITEMS[i]]->item_tags.count("GRENADE"))) &&
      has_amount(ALT_ATTACK_ITEMS[i], 1))
   which = ALT_ATTACK_ITEMS[i];
 }

 if (which == "null") { // We ain't got shit!
// Not sure if this should ever occur.  For now, let's warn with a debug msg
  debugmsg("npc::alt_attack() couldn't find an alt attack item!");
  if (dist == 1) {
   if (target == TARGET_PLAYER)
    melee_player(g->u);
   else
    melee_monster(target);
  } else
   move_to(tarx, tary);
 }

 char invlet = 0;
 item *used = NULL;
 if (weapon.type->id == which) {
  used = &weapon;
  invlet = 0;
 } else {
  invslice slice = inv.slice();
  for (int i = 0; i < inv.size(); i++) {
   if (slice[i]->front().type->id == which) {
    used = &(slice[i]->front());
    invlet = used->invlet;
   }
  }
 }

// Are we going to throw this item?
 if (!thrown_item(used))
  activate_item(invlet);
 else { // We are throwing it!

  std::vector<point> trajectory;
  int linet, light = g->light_level();

  if (dist <= confident_range(invlet) && wont_hit_friend(tarx, tary, invlet)) {

   if (g->m.sees(posx, posy, tarx, tary, light, linet))
    trajectory = line_to(posx, posy, tarx, tary, linet);
   else
    trajectory = line_to(posx, posy, tarx, tary, 0);
   moves -= 125;
   if (g->u_see(posx, posy))
    g->add_msg(_("%s throws a %s."), name.c_str(), used->tname().c_str());

   int stack_size = -1;
   if( used->count_by_charges() ) {
       stack_size = used->charges;
       used->charges = 1;
   }
   g->throw_item(*this, tarx, tary, *used, trajectory);
   // Throw a single charge of a stacking object.
   if( stack_size == -1 || stack_size == 1 ) {
       i_remn(invlet);
   } else {
       used->charges = stack_size - 1;
   }
  } else if (!wont_hit_friend(tarx, tary, invlet)) {// Danger of friendly fire

   if (!used->active || used->charges > 2) // Safe to hold on to, for now
    avoid_friendly_fire(target); // Maneuver around player
   else { // We need to throw this live (grenade, etc) NOW! Pick another target?
    int conf = confident_range(invlet);
    for (int dist = 2; dist <= conf; dist++) {
     for (int x = posx - dist; x <= posx + dist; x++) {
      for (int y = posy - dist; y <= posy + dist; y++) {
       int newtarget = g->mon_at(x, y);
       int newdist = rl_dist(posx, posy, x, y);
// TODO: Change "newdist >= 2" to "newdist >= safe_distance(used)"
// Molotovs are safe at 2 tiles, grenades at 4, mininukes at 8ish
       if (newdist <= conf && newdist >= 2 && newtarget != -1 &&
           wont_hit_friend(x, y, invlet)) { // Friendlyfire-safe!
        alt_attack(newtarget);
        return;
       }
      }
     }
    }
/* If we have reached THIS point, there's no acceptible monster to throw our
 * grenade or whatever at.  Since it's about to go off in our hands, better to
 * just chuck it as far away as possible--while being friendly-safe.
 */
    int best_dist = 0;
    for (int dist = 2; dist <= conf; dist++) {
     for (int x = posx - dist; x <= posx + dist; x++) {
      for (int y = posy - dist; y <= posy + dist; y++) {
       int new_dist = rl_dist(posx, posy, x, y);
       if (new_dist > best_dist && wont_hit_friend(x, y, invlet)) {
        best_dist = new_dist;
        tarx = x;
        tary = y;
       }
      }
     }
    }
/* Even if tarx/tary didn't get set by the above loop, throw it anyway.  They
 * should be equal to the original location of our target, and risking friendly
 * fire is better than holding on to a live grenade / whatever.
 */
    if (g->m.sees(posx, posy, tarx, tary, light, linet))
     trajectory = line_to(posx, posy, tarx, tary, linet);
    else
     trajectory = line_to(posx, posy, tarx, tary, 0);
    moves -= 125;
    if (g->u_see(posx, posy))
     g->add_msg(_("%s throws a %s."), name.c_str(), used->tname().c_str());

    int stack_size = -1;
    if( used->count_by_charges() ) {
        stack_size = used->charges;
        used->charges = 1;
    }
    g->throw_item(*this, tarx, tary, *used, trajectory);

    // Throw a single charge of a stacking object.
    if( stack_size == -1 || stack_size == 1 ) {
        i_remn(invlet);
    } else {
        used->charges = stack_size - 1;
    }

    i_remn(invlet);
   }

  } else { // Within this block, our chosen target is outside of our range
   update_path(tarx, tary);
   move_to_next(); // Move towards the target
  }
 } // Done with throwing-item block
}

void npc::activate_item(char invlet)
{
 item *it = &(inv.item_by_letter(invlet));
 if (it->is_tool()) {
     it_tool* tool = dynamic_cast<it_tool*>(it->type);
     tool->use.call(this, it, false);
 } else if (it->is_food()) {
     it_comest* comest = dynamic_cast<it_comest*>(it->type);
     comest->use.call(this, it, false);
 }
}

bool thrown_item(item *used)
{
 if (used == NULL) {
  debugmsg("npcmove.cpp's thrown_item() called with NULL item");
  return false;
 }
 itype_id type = itype_id(used->type->id);
 return (used->active || type == "knife_combat" || type == "spear_wood");
}

void npc::heal_player(player &patient)
{
 int dist = rl_dist(posx, posy, patient.posx, patient.posy);

 if (dist > 1) { // We need to move to the player
  update_path(patient.posx, patient.posy);
  move_to_next();
 } else { // Close enough to heal!
  int lowest_HP = 400;
  hp_part worst = hp_head;
// Chose the worst-hurting body part
  for (int i = 0; i < num_hp_parts; i++) {
   int hp = patient.hp_cur[i];
// The head and torso are weighted more heavily than other body parts
   if (i == hp_head)
    hp = patient.hp_max[i] - 3 * (patient.hp_max[i] - hp);
   else if (i == hp_torso)
    hp = patient.hp_max[i] - 2 * (patient.hp_max[i] - hp);
   if (hp < lowest_HP) {
    lowest_HP = hp;
    worst = hp_part(i);
   }
  }

  bool u_see_me      = g->u_see(posx, posy),
       u_see_patient = g->u_see(patient.posx, patient.posy);
  if (patient.is_npc()) {
   if (u_see_me) {
    if (u_see_patient)
     g->add_msg(_("%s heals %s."),  name.c_str(), patient.name.c_str());
    else
     g->add_msg(_("%s heals someone."), name.c_str());
   } else if (u_see_patient)
    g->add_msg(_("Someone heals %s."), patient.name.c_str());
  } else if (u_see_me)
   g->add_msg(_("%s heals you."), name.c_str());
  else
   g->add_msg(_("Someone heals you."));

  int amount_healed = 0;
  if (has_amount("1st_aid", 1)) {
   switch (worst) {
    case hp_head:  amount_healed = 10 + 1.6 * skillLevel("firstaid"); break;
    case hp_torso: amount_healed = 20 + 3   * skillLevel("firstaid"); break;
    default:       amount_healed = 15 + 2   * skillLevel("firstaid");
   }
   use_charges("1st_aid", 1);
  } else if (has_amount("bandages", 1)) {
   switch (worst) {
    case hp_head:  amount_healed =  1 + 1.6 * skillLevel("firstaid"); break;
    case hp_torso: amount_healed =  4 + 3   * skillLevel("firstaid"); break;
    default:       amount_healed =  3 + 2   * skillLevel("firstaid");
   }
   use_charges("bandages", 1);
  }
  patient.heal(worst, amount_healed);
  moves -= 250;

  if (!patient.is_npc()) {
 // Test if we want to heal the player further
   if (op_of_u.value * 4 + op_of_u.trust + personality.altruism * 3 +
       (fac_has_value(FACVAL_CHARITABLE) ?  5 : 0) +
       (fac_has_job  (FACJOB_DOCTORS)    ? 15 : 0) - op_of_u.fear * 3 <  25) {
    attitude = NPCATT_FOLLOW;
    say(_("That's all the healing I can do."));
   } else
    say(_("Hold still, I can heal you more."));
  }
 }
}

void npc::heal_self()
{
 int lowest_HP = 400;
 hp_part worst = hp_head;
// Chose the worst-hurting body part
 for (int i = 0; i < num_hp_parts; i++) {
  int hp = hp_cur[i];
// The head and torso are weighted more heavily than other body parts
  if (i == hp_head)
   hp = hp_max[i] - 3 * (hp_max[i] - hp);
  else if (i == hp_torso)
   hp = hp_max[i] - 2 * (hp_max[i] - hp);
  if (hp < lowest_HP) {
   lowest_HP = hp;
   worst = hp_part(i);
  }
 }

 int amount_healed = 0;
 if (has_amount("1st_aid", 1)) {
  switch (worst) {
   case hp_head:  amount_healed = 10 + 1.6 * skillLevel("firstaid"); break;
   case hp_torso: amount_healed = 20 + 3   * skillLevel("firstaid"); break;
   default:       amount_healed = 15 + 2   * skillLevel("firstaid");
  }
  use_charges("1st_aid", 1);
 } else if (has_amount("bandages", 1)) {
  switch (worst) {
   case hp_head:  amount_healed =  1 + 1.6 * skillLevel("firstaid"); break;
   case hp_torso: amount_healed =  4 + 3   * skillLevel("firstaid"); break;
   default:       amount_healed =  3 + 2   * skillLevel("firstaid");
  }
  use_charges("bandages", 1);
 } else {
  debugmsg("NPC tried to heal self, but has no bandages / first aid");
  move_pause();
 }
 if (g->u_see(posx, posy))
  g->add_msg(_("%s heals %s."), name.c_str(), (male ? _("himself") : _("herself")));
 heal(worst, amount_healed);
 moves -= 250;
}

void npc::use_painkiller()
{
// First, find the best painkiller for our pain level
 item& it = inv.most_appropriate_painkiller(pain);

 if (it.is_null()) {
  debugmsg("NPC tried to use painkillers, but has none!");
  move_pause();
 } else {
  consume(it.invlet);
  moves = 0;
 }
}

void npc::pick_and_eat()
{
 int best_hunger = 999, best_thirst = 999, index = -1;
 bool thirst_more_important = (thirst > hunger * 1.5);
 invslice slice = inv.slice();
 for (int i = 0; i < slice.size(); i++) {
  int eaten_hunger = -1, eaten_thirst = -1;
  it_comest* food = NULL;
  item& it = slice[i]->front();
  if (it.is_food())
   food = dynamic_cast<it_comest*>(it.type);
  else if (it.is_food_container())
   food = dynamic_cast<it_comest*>(it.contents[0].type);
  if (food != NULL) {
   eaten_hunger = hunger - food->nutr;
   eaten_thirst = thirst - food->quench;
  }
  if (eaten_hunger > 0) { // <0 means we have a chance of puking
   if ((thirst_more_important && eaten_thirst < best_thirst) ||
       (!thirst_more_important && eaten_hunger < best_hunger) ||
       (eaten_thirst == best_thirst && eaten_hunger < best_hunger) ||
       (eaten_hunger == best_hunger && eaten_thirst < best_thirst)   ) {
    if (eaten_hunger < best_hunger)
     best_hunger = eaten_hunger;
    if (eaten_thirst < best_thirst)
     best_thirst = eaten_thirst;
    index = i;
   }
  }
 }

 if (index == -1) {
  debugmsg("NPC tried to eat food, but couldn't find any!");
  move_pause();
  return;
 }

 consume(index);
 moves = 0;
}

void npc::mug_player(player &mark)
{
 if (rl_dist(posx, posy, mark.posx, mark.posy) > 1) { // We have to travel
  update_path(mark.posx, mark.posy);
  move_to_next();
 } else {
  bool u_see_me   = g->u_see(posx, posy),
       u_see_mark = g->u_see(mark.posx, mark.posy);
  if (mark.cash > 0) {
   cash += mark.cash;
   mark.cash = 0;
   moves = 0;
// Describe the action
   if (mark.is_npc()) {
    if (u_see_me) {
     if (u_see_mark)
      g->add_msg(_("%s takes %s's money!"), name.c_str(), mark.name.c_str());
     else
      g->add_msg(_("%s takes someone's money!"), name.c_str());
    } else if (u_see_mark)
     g->add_msg(_("Someone takes %s's money!"), mark.name.c_str());
   } else {
    if (u_see_me)
     g->add_msg(_("%s takes your money!"), name.c_str());
    else
     g->add_msg(_("Someone takes your money!"));
   }
  } else { // We already have their money; take some goodies!
// value_mod affects at what point we "take the money and run"
// A lower value means we'll take more stuff
   double value_mod = 1 - double((10 - personality.bravery)    * .05) -
                          double((10 - personality.aggression) * .04) -
                          double((10 - personality.collector)  * .06);
   if (!mark.is_npc()) {
    value_mod += double(op_of_u.fear * .08);
    value_mod -= double((8 - op_of_u.value) * .07);
   }
   int best_value = minimum_item_value() * value_mod;
   char invlet = 0;
   invslice slice = mark.inv.slice();
   for (int i = 0; i < slice.size(); i++) {
    if (value(slice[i]->front()) >= best_value &&
        can_pickVolume(slice[i]->front().volume()) &&
        can_pickWeight(slice[i]->front().weight())) {
     best_value = value(slice[i]->front());
     invlet = slice[i]->front().invlet;
    }
   }
   if (invlet == 0) { // Didn't find anything worthwhile!
    attitude = NPCATT_FLEE;
    if (!one_in(3))
     say("<done_mugging>");
    moves -= 100;
   } else {
    bool u_see_me   = g->u_see(posx, posy),
         u_see_mark = g->u_see(mark.posx, mark.posy);
    item stolen = mark.i_remn(invlet);
    if (mark.is_npc()) {
     if (u_see_me) {
      if (u_see_mark)
       g->add_msg(_("%s takes %s's %s."), name.c_str(), mark.name.c_str(),
                  stolen.tname().c_str());
      else
       g->add_msg(_("%s takes something from somebody."), name.c_str());
     } else if (u_see_mark)
      g->add_msg(_("Someone takes %s's %s."), mark.name.c_str(),
                 stolen.tname().c_str());
    } else {
     if (u_see_me)
      g->add_msg(_("%s takes your %s."), name.c_str(), stolen.tname().c_str());
     else
      g->add_msg(_("Someone takes your %s."), stolen.tname().c_str());
    }
    i_add(stolen);
    moves -= 100;
    if (!mark.is_npc())
     op_of_u.value -= rng(0, 1); // Decrease the value of the player
   }
  }
 }
}

void npc::look_for_player(player &sought)
{
 int linet, range = sight_range(g->light_level());
 if (g->m.sees(posx, posy, sought.posx, sought.posy, range, linet)) {
  if (sought.is_npc())
   debugmsg("npc::look_for_player() called, but we can see %s!",
            sought.name.c_str());
  else
   debugmsg("npc::look_for_player() called, but we can see u!");
  move_pause();
  return;
 }

 if (!path.empty()) {
  point dest = path[path.size() - 1];
  if (!g->m.sees(posx, posy, dest.x, dest.y, range, linet)) {
   move_to_next();
   return;
  }
  path.clear();
 }
 std::vector<point> possibilities;
 for (int x = 1; x < SEEX * MAPSIZE; x += 11) { // 1, 12, 23, 34
  for (int y = 1; y < SEEY * MAPSIZE; y += 11) {
   if (g->m.sees(posx, posy, x, y, range, linet))
    possibilities.push_back(point(x, y));
  }
 }
 if (possibilities.size() == 0) { // We see all the spots we'd like to check!
  say("<wait>");
  move_pause();
 } else {
  if (one_in(6))
   say("<wait>");
  int index = rng(0, possibilities.size() - 1);
  update_path(possibilities[index].x, possibilities[index].y);
  move_to_next();
 }
}

bool npc::saw_player_recently()
{
 return (plx >= 0 && plx < SEEX * MAPSIZE && ply >= 0 && ply < SEEY * MAPSIZE &&
         plt > 0);
}

bool npc::has_destination()
{
    return goal != no_goal_point;
}

void npc::reach_destination()
{
    goal = no_goal_point;
}

void npc::set_destination()
{
/* TODO: Make NPCs' movement more intelligent.
 * Right now this function just makes them attempt to address their needs:
 *  if we need ammo, go to a gun store, if we need food, go to a grocery store,
 *  and if we don't have any needs, pick a random spot.
 * What it SHOULD do is that, if there's time; but also look at our mission and
 *  our faction to determine more meaningful actions, such as attacking a rival
 *  faction's base, or meeting up with someone friendly.  NPCs should also
 *  attempt to reach safety before nightfall, and possibly similar goals.
 * Also, NPCs should be able to assign themselves missions like "break into that
 *  lab" or "map that river bank."
 */

 // all of the following luxuries are at ground level.
 // so please wallow in hunger & fear if below ground.
 if(g->levz != 0){
  goal = no_goal_point;
  return;
 }

 decide_needs();
 if (needs.empty()) // We don't need anything in particular.
  needs.push_back(need_none);
 std::vector<std::string> options;
 switch(needs[0]) {
  case need_ammo:   options.push_back("house");
  case need_gun:    options.push_back("s_gun"); break;

  case need_weapon: options.push_back("s_gun");
                    options.push_back("s_sports");
                    options.push_back("s_hardware"); break;

  case need_drink:  options.push_back("s_gas");
                    options.push_back("s_pharm");
                    options.push_back("s_liquor");
  case need_food:   options.push_back("s_grocery"); break;

  default:      options.push_back("house");
                options.push_back("s_gas");
                options.push_back("s_pharm");
                options.push_back("s_hardware");
                options.push_back("s_sports");
                options.push_back("s_liquor");
                options.push_back("s_gun");
                options.push_back("s_library");
 }

 std::string dest_type = options[rng(0, options.size() - 1)];

 int dist = 0;
 const point p = overmap_buffer.find_closest(global_omt_location(), dest_type, dist, false);
 goal.x = p.x;
 goal.y = p.y;
 goal.z = g->levz;
}

void npc::go_to_destination()
{
 const tripoint omt_pos = global_omt_location();
 int sx = (goal.x > omt_pos.x ? 1 : -1), sy = (goal.y > omt_pos.y ? 1 : -1);
 if (goal.x == omt_pos.x && goal.y == omt_pos.y) { // We're at our desired map square!
  move_pause();
  reach_destination();
 } else {
  if (goal.x == omt_pos.x)
   sx = 0;
  if (goal.y == omt_pos.y)
   sy = 0;
// sx and sy are now equal to the direction we need to move in
  int x = posx + 8 * sx, y = posy + 8 * sy, linet, light = g->light_level();
// x and y are now equal to a local square that's close by
  for (int i = 0; i < 8; i++) {
   for (int dx = 0 - i; dx <= i; dx++) {
    for (int dy = 0 - i; dy <= i; dy++) {
     if ((g->m.move_cost(x + dx, y + dy) > 0 ||
          g->m.has_flag("BASHABLE", x + dx, y + dy) ||
          g->m.ter(x + dx, y + dy) == t_door_c) &&
         g->m.sees(posx, posy, x + dx, y + dy, light, linet)) {
      path = g->m.route(posx, posy, x + dx, y + dy);
      if (!path.empty() && can_move_to(path[0].x, path[0].y)) {
       move_to_next();
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

std::string npc_action_name(npc_action action)
{
 switch (action) {
  case npc_undecided:           return _("Undecided");
  case npc_pause:               return _("Pause");
  case npc_reload:              return _("Reload");
  case npc_sleep:               return _("Sleep");
  case npc_pickup:              return _("Pick up items");
  case npc_escape_item:         return _("Use escape item");
  case npc_wield_melee:         return _("Wield melee weapon");
  case npc_wield_loaded_gun:    return _("Wield loaded gun");
  case npc_wield_empty_gun:     return _("Wield empty gun");
  case npc_heal:                return _("Heal self");
  case npc_use_painkiller:      return _("Use painkillers");
  case npc_eat:                 return _("Eat");
  case npc_drop_items:          return _("Drop items");
  case npc_flee:                return _("Flee");
  case npc_melee:               return _("Melee");
  case npc_shoot:               return _("Shoot");
  case npc_shoot_burst:         return _("Fire a burst");
  case npc_alt_attack:          return _("Use alternate attack");
  case npc_look_for_player:     return _("Look for player");
  case npc_heal_player:         return _("Heal player");
  case npc_follow_player:       return _("Follow player");
  case npc_follow_embarked:     return _("Follow player (embarked)");
  case npc_talk_to_player:      return _("Talk to player");
  case npc_mug_player:          return _("Mug player");
  case npc_goto_destination:    return _("Go to destination");
  case npc_avoid_friendly_fire: return _("Avoid friendly fire");
  default:    return "Unnamed action";
 }
}
