#include <vector>
#include <string>
#include "game.h"
#include "keypress.h"
#include "output.h"
#include "line.h"
#include "skill.h"
#include "rng.h"
#include "item.h"
#include "options.h"

int time_to_fire(player &p, it_gun* firing);
int recoil_add(player &p);
void make_gun_sound_effect(game *g, player &p, bool burst);
int calculate_range(player &p, int tarx, int tary);
double calculate_missed_by(player &p, int trange);
void shoot_monster(game *g, player &p, monster &mon, int &dam, double goodhit);
void shoot_player(game *g, player &p, player *h, int &dam, double goodhit);

void splatter(game *g, std::vector<point> trajectory, int dam,
              monster* mon = NULL);

void ammo_effects(game *g, int x, int y, long flags);

void game::fire(player &p, int tarx, int tary, std::vector<point> &trajectory,
                bool burst)
{
 item ammotmp;
 if (p.weapon.has_flag(IF_CHARGE)) { // It's a charger gun, so make up a type
// Charges maxes out at 8.
  it_ammo *tmpammo = dynamic_cast<it_ammo*>(itypes[itm_charge_shot]);
  tmpammo->damage = p.weapon.charges * p.weapon.charges;
  tmpammo->pierce = (p.weapon.charges >= 4 ? (p.weapon.charges - 3) * 2.5 : 0);
  tmpammo->range = 5 + p.weapon.charges * 5;
  if (p.weapon.charges <= 4)
   tmpammo->accuracy = 14 - p.weapon.charges * 2;
  else // 5, 12, 21, 32
   tmpammo->accuracy = p.weapon.charges * (p.weapon.charges - 4);
  tmpammo->recoil = tmpammo->accuracy * .8;
  tmpammo->item_flags = 0;
  if (p.weapon.charges == 8)
   tmpammo->item_flags |= mfb(IF_AMMO_EXPLOSIVE_BIG);
  else if (p.weapon.charges >= 6)
   tmpammo->item_flags |= mfb(IF_AMMO_EXPLOSIVE);
  if (p.weapon.charges >= 5)
   tmpammo->item_flags |= mfb(IF_AMMO_FLAME);
  else if (p.weapon.charges >= 4)
   tmpammo->item_flags |= mfb(IF_AMMO_INCENDIARY);

  ammotmp = item(tmpammo, 0);
  p.weapon.curammo = tmpammo;
  p.weapon.active = false;
  p.weapon.charges = 0;

 } else // Just a normal gun. If we're here, we know curammo is valid.
  ammotmp = item(p.weapon.curammo, 0);

 ammotmp.charges = 1;
 if (!p.weapon.is_gun()) {
  debugmsg("%s tried to fire a non-gun (%s).", p.name.c_str(),
                                               p.weapon.tname().c_str());
  return;
 }
 bool is_bolt = false;
 unsigned int flags = p.weapon.curammo->item_flags;
// Bolts and arrows are silent
 if (p.weapon.curammo->type == AT_BOLT || p.weapon.curammo->type == AT_ARROW)
  is_bolt = true;
// TODO: Move this check to game::plfire
 if ((p.weapon.has_flag(IF_STR8_DRAW)  && p.str_cur <  4) ||
     (p.weapon.has_flag(IF_STR10_DRAW) && p.str_cur <  5)   ) {
  add_msg("You're not strong enough to draw the bow!");
  return;
 }

 int x = p.posx, y = p.posy;
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 if (p.has_trait(PF_TRIGGERHAPPY) && one_in(30))
  burst = true;
 if (burst && p.weapon.burst_size() < 2)
  burst = false; // Can't burst fire a semi-auto

 int junk = 0;
 bool u_see_shooter = u_see(p.posx, p.posy, junk);
// Use different amounts of time depending on the type of gun and our skill
 p.moves -= time_to_fire(p, firing);
// Decide how many shots to fire
 int num_shots = 1;
 if (burst)
  num_shots = p.weapon.burst_size();
 if (num_shots > p.weapon.charges && !p.weapon.has_flag(IF_CHARGE))
  num_shots = p.weapon.charges;

 if (num_shots == 0)
  debugmsg("game::fire() - num_shots = 0!");
 
 // Make a sound at our location - Zombies will chase it
 make_gun_sound_effect(this, p, burst);
// Set up a timespec for use in the nanosleep function below
 timespec ts;
 ts.tv_sec = 0;
 ts.tv_nsec = BULLET_SPEED;

 bool missed = false;
 int tart;
 for (int curshot = 0; curshot < num_shots; curshot++) {
// Burst-fire weapons allow us to pick a new target after killing the first
  if (curshot > 0 &&
      (mon_at(tarx, tary) == -1 || z[mon_at(tarx, tary)].hp <= 0)) {
   std::vector<point> new_targets;
   int mondex;
   for (int radius = 1; radius <= 2 + p.sklevel[sk_gun] && new_targets.empty();
        radius++) {
    for (int diff = 0 - radius; diff <= radius; diff++) {
     mondex = mon_at(tarx + diff, tary - radius);
     if (mondex != -1 && z[mondex].hp > 0 && z[mondex].friendly == 0)
      new_targets.push_back( point(tarx + diff, tary - radius) );

     mondex = mon_at(tarx + diff, tary + radius);
     if (mondex != -1 && z[mondex].hp > 0 && z[mondex].friendly == 0)
      new_targets.push_back( point(tarx + diff, tary + radius) );

     if (diff != 0 - radius && diff != radius) { // Corners were already checked
      mondex = mon_at(tarx - radius, tary + diff);
      if (mondex != -1 && z[mondex].hp > 0 && z[mondex].friendly == 0)
       new_targets.push_back( point(tarx - radius, tary + diff) );

      mondex = mon_at(tarx + radius, tary + diff);
      if (mondex != -1 && z[mondex].hp > 0 && z[mondex].friendly == 0)
       new_targets.push_back( point(tarx + radius, tary + diff) );
     }
    }
   }
   if (!new_targets.empty()) {
    int target_picked = rng(0, new_targets.size() - 1);
    tarx = new_targets[target_picked].x;
    tary = new_targets[target_picked].y;
    if (m.sees(p.posx, p.posy, tarx, tary, 0, tart))
     trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
    else
     trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
   } else if ((!p.has_trait(PF_TRIGGERHAPPY) || one_in(3)) &&
              (p.sklevel[sk_gun] >= 7 || one_in(7 - p.sklevel[sk_gun])))
    return; // No targets, so return
  }
// Use up a round (or 100)
  if (p.weapon.has_flag(IF_FIRE_100))
   p.weapon.charges -= 100;
  else
   p.weapon.charges--;

  int trange = calculate_range(p, tarx, tary);
  double missed_by = calculate_missed_by(p, trange);
// Calculate a penalty based on the monster's speed
  double monster_speed_penalty = 1.;
  int target_index = mon_at(tarx, tary);
  if (target_index != -1) {
   monster_speed_penalty = double(z[target_index].speed) / 80.;
   if (monster_speed_penalty < 1.)
    monster_speed_penalty = 1.;
  }

  if (curshot > 0) {
   if (recoil_add(p) % 2 == 1)
    p.recoil++;
   p.recoil += recoil_add(p) / 2;
  } else
   p.recoil += recoil_add(p);

  if (missed_by >= 1.) {
// We missed D:
// Shoot a random nearby space?
   tarx += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
   tary += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
   if (m.sees(p.posx, p.posy, x, y, -1, tart))
    trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
   else
    trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
   missed = true;
   if (!burst) {
    if (&p == &u)
     add_msg("You miss!");
    else if (u_see_shooter)
     add_msg("%s misses!", p.name.c_str());
   }
  } else if (missed_by >= .7 / monster_speed_penalty) {
// Hit the space, but not necessarily the monster there
   missed = true;
   if (!burst) {
    if (&p == &u)
     add_msg("You barely miss!");
    else if (u_see_shooter)
     add_msg("%s barely misses!", p.name.c_str());
   }
  }

  int dam = p.weapon.gun_damage();
  for (int i = 0; i < trajectory.size() &&
       (dam > 0 || (flags & IF_AMMO_FLAME)); i++) {
   if (i > 0)
    m.drawsq(w_terrain, u, trajectory[i-1].x, trajectory[i-1].y, false, true);
// Drawing the bullet uses player u, and not player p, because it's drawn
// relative to YOUR position, which may not be the gunman's position.
   if (u_see(trajectory[i].x, trajectory[i].y, junk)) {
    char bullet = '*';
    if (flags & mfb(IF_AMMO_FLAME))
     bullet = '#';
    mvwputch(w_terrain, trajectory[i].y + SEEY - u.posy,
                        trajectory[i].x + SEEX - u.posx, c_red, bullet);
    wrefresh(w_terrain);
    if (&p == &u)
     nanosleep(&ts, NULL);
   }
   
   if (dam <= 0) { // Ran out of momentum.
    ammo_effects(this, trajectory[i].x, trajectory[i].y, flags);
    if (is_bolt &&
        ((p.weapon.curammo->m1 == WOOD && !one_in(4)) ||
         (p.weapon.curammo->m1 != WOOD && !one_in(15))))
     m.add_item(trajectory[i].x, trajectory[i].y, ammotmp);
    if (p.weapon.charges == 0)
     p.weapon.curammo = NULL;
    return;
   }

   int tx = trajectory[i].x, ty = trajectory[i].y;
// If there's a monster in the path of our bullet, and either our aim was true,
//  OR it's not the monster we were aiming at and we were lucky enough to hit it
   int mondex = mon_at(tx, ty);
// If we shot us a monster...
   if (mondex != -1 && (!z[mondex].has_flag(MF_DIGS) ||
       rl_dist(p.posx, p.posy, z[mondex].posx, z[mondex].posy) <= 1) &&
       ((!missed && i == trajectory.size() - 1) ||
        one_in((5 - int(z[mondex].type->size))))) {

    double goodhit = missed_by;
    if (i < trajectory.size() - 1) // Unintentional hit
     goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;

// Penalize for the monster's speed
    if (z[mondex].speed > 80)
     goodhit *= double( double(z[mondex].speed) / 80.);
    
    std::vector<point> blood_traj = trajectory;
    blood_traj.insert(blood_traj.begin(), point(p.posx, p.posy));
    splatter(this, blood_traj, dam, &z[mondex]);
    shoot_monster(this, p, z[mondex], dam, goodhit);

   } else if ((!missed || one_in(3)) &&
              (npc_at(tx, ty) != -1 || (u.posx == tx && u.posy == ty)))  {
    double goodhit = missed_by;
    if (i < trajectory.size() - 1) // Unintentional hit
     goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;
    player *h;
    if (u.posx == tx && u.posy == ty)
     h = &u;
    else
     h = &(active_npc[npc_at(tx, ty)]);

    std::vector<point> blood_traj = trajectory;
    blood_traj.insert(blood_traj.begin(), point(p.posx, p.posy));
    splatter(this, blood_traj, dam);
    shoot_player(this, p, h, dam, goodhit);

   } else
    m.shoot(this, tx, ty, dam, i == trajectory.size() - 1, flags);
  } // Done with the trajectory!

  int lastx = trajectory[trajectory.size() - 1].x;
  int lasty = trajectory[trajectory.size() - 1].y;
  ammo_effects(this, lastx, lasty, flags);

  if (m.move_cost(lastx, lasty) == 0) {
   lastx = trajectory[trajectory.size() - 2].x;
   lasty = trajectory[trajectory.size() - 2].y;
  }
  if (is_bolt &&
      ((p.weapon.curammo->m1 == WOOD && !one_in(5)) ||
       (p.weapon.curammo->m1 != WOOD && !one_in(15))  ))
    m.add_item(lastx, lasty, ammotmp);
 }

 if (p.weapon.charges == 0)
  p.weapon.curammo = NULL;
}


void game::throw_item(player &p, int tarx, int tary, item &thrown,
                      std::vector<point> &trajectory)
{
 int deviation = 0;
 int trange = 1.5 * rl_dist(p.posx, p.posy, tarx, tary);

// Throwing attempts below "Basic Competency" level are extra-bad
 if (p.sklevel[sk_throw] < 3)
  deviation += rng(0, 8 - p.sklevel[sk_throw]);

 if (p.sklevel[sk_throw] < 8)
  deviation += rng(0, 8 - p.sklevel[sk_throw]);
 else
  deviation -= p.sklevel[sk_throw] - 6;

 deviation += p.throw_dex_mod();

 if (p.per_cur < 6)
  deviation += rng(0, 8 - p.per_cur);
 else if (p.per_cur > 8)
  deviation -= p.per_cur - 8;

 deviation += rng(0, p.encumb(bp_hands) * 2 + p.encumb(bp_eyes) + 1);
 if (thrown.volume() > 5)
  deviation += rng(0, 1 + (thrown.volume() - 5) / 4);
 if (thrown.volume() == 0)
  deviation += rng(0, 3);

 deviation += rng(0, 1 + abs(p.str_cur - thrown.weight()));

 double missed_by = .01 * deviation * trange;
 bool missed = false;
 int tart;

 if (missed_by >= 1) {
// We missed D:
// Shoot a random nearby space?
  if (missed_by > 9)
   missed_by = 9;
  tarx += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
  tary += rng(0 - int(sqrt(double(missed_by))), int(sqrt(double(missed_by))));
  if (m.sees(p.posx, p.posy, tarx, tary, -1, tart))
   trajectory = line_to(p.posx, p.posy, tarx, tary, tart);
  else
   trajectory = line_to(p.posx, p.posy, tarx, tary, 0);
  missed = true;
  if (!p.is_npc())
   add_msg("You miss!");
 } else if (missed_by >= .6) {
// Hit the space, but not necessarily the monster there
  missed = true;
  if (!p.is_npc())
   add_msg("You barely miss!");
 }

 std::string message;
 int dam = (thrown.weight() / 4 + thrown.type->melee_dam / 2 + p.str_cur / 2) /
            double(2 + double(thrown.volume() / 4));
 if (dam > thrown.weight() * 3)
  dam = thrown.weight() * 3;

 int i = 0, tx = 0, ty = 0;
 for (i = 0; i < trajectory.size() && dam > -10; i++) {
  message = "";
  double goodhit = missed_by;
  tx = trajectory[i].x;
  ty = trajectory[i].y;
// If there's a monster in the path of our item, and either our aim was true,
//  OR it's not the monster we were aiming at and we were lucky enough to hit it
  if (mon_at(tx, ty) != -1 &&
      (!missed || one_in(7 - int(z[mon_at(tx, ty)].type->size)))) {
   if (rng(0, 100) < 20 + p.sklevel[sk_throw] * 12 &&
       thrown.type->melee_cut > 0) {
    if (!p.is_npc()) {
     message += " You cut the ";
     message += z[mon_at(tx, ty)].name();
     message += "!";
    }
    if (thrown.type->melee_cut > z[mon_at(tx, ty)].armor_cut())
     dam += (thrown.type->melee_cut - z[mon_at(tx, ty)].armor_cut());
   }
   if (thrown.made_of(GLASS) && !thrown.active && // active = molotov, etc.
       rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
    if (u_see(tx, ty, tart))
     add_msg("The %s shatters!", thrown.tname().c_str());
    for (int i = 0; i < thrown.contents.size(); i++)
     m.add_item(tx, ty, thrown.contents[i]);
    sound(tx, ty, 16, "glass breaking!");
    int glassdam = rng(0, thrown.volume() * 2);
    if (glassdam > z[mon_at(tx, ty)].armor_cut())
     dam += (glassdam - z[mon_at(tx, ty)].armor_cut());
   } else
    m.add_item(tx, ty, thrown);
   if (i < trajectory.size() - 1)
    goodhit = double(double(rand() / RAND_MAX) / 2);
   if (goodhit < .1 && !z[mon_at(tx, ty)].has_flag(MF_NOHEAD)) {
    message = "Headshot!";
    dam = rng(dam, dam * 3);
    p.practice(sk_throw, 5);
   } else if (goodhit < .2) {
    message = "Critical!";
    dam = rng(dam, dam * 2);
    p.practice(sk_throw, 2);
   } else if (goodhit < .4)
    dam = rng(int(dam / 2), int(dam * 1.5));
   else if (goodhit < .5) {
    message = "Grazing hit.";
    dam = rng(0, dam);
   }
   if (!p.is_npc())
    add_msg("%s You hit the %s for %d damage.",
            message.c_str(), z[mon_at(tx, ty)].name().c_str(), dam);
   else if (u_see(tx, ty, tart))
    add_msg("%s hits the %s for %d damage.", message.c_str(),
            z[mon_at(tx, ty)].name().c_str(), dam);
   if (z[mon_at(tx, ty)].hurt(dam))
    kill_mon(mon_at(tx, ty), !p.is_npc());
   return;
  } else // No monster hit, but the terrain might be.
   m.shoot(this, tx, ty, dam, false, 0);
  if (m.move_cost(tx, ty) == 0) {
   if (i > 0) {
    tx = trajectory[i - 1].x;
    ty = trajectory[i - 1].y;
   } else {
    tx = u.posx;
    ty = u.posy;
   }
   i = trajectory.size();
  }
 }
 if (m.move_cost(tx, ty) == 0) {
  if (i > 1) {
   tx = trajectory[i - 2].x;
   ty = trajectory[i - 2].y;
  } else {
   tx = u.posx;
   ty = u.posy;
  }
 }
 if (thrown.made_of(GLASS) && !thrown.active && // active means molotov, etc
     rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
  if (u_see(tx, ty, tart))
   add_msg("The %s shatters!", thrown.tname().c_str());
  for (int i = 0; i < thrown.contents.size(); i++)
   m.add_item(tx, ty, thrown.contents[i]);
  sound(tx, ty, 16, "glass breaking!");
 } else {
  sound(tx, ty, 8, "thud.");
  m.add_item(tx, ty, thrown);
 }
}

std::vector<point> game::target(int &x, int &y, int lowx, int lowy, int hix,
                                int hiy, std::vector <monster> t, int &target,
                                item *relevent)
{
 std::vector<point> ret;
 int tarx, tary, tart, junk;
 int sight_dist = u.sight_range(light_level());

// First, decide on a target among the monsters, if there are any in range
 if (t.size() > 0) {
// Check for previous target
  if (target == -1) {
// If no previous target, target the closest there is
   double closest = -1;
   double dist;
   for (int i = 0; i < t.size(); i++) {
    dist = rl_dist(t[i].posx, t[i].posy, u.posx, u.posy);
    if (closest < 0 || dist < closest) {
     closest = dist;
     target = i;
    }
   }
  }
  x = t[target].posx;
  y = t[target].posy;
 } else
  target = -1;	// No monsters in range, don't use target, reset to -1

 WINDOW* w_target = newwin(13, 48, 12, SEEX * 2 + 8);
 wborder(w_target, LINE_XOXO, LINE_XOXO, LINE_OXOX, LINE_OXOX,
                 LINE_OXXO, LINE_OOXX, LINE_XXOO, LINE_XOOX );
 if (!relevent) // currently targetting vehicle to refill with fuel
  mvwprintz(w_target, 1, 1, c_red, "Select a vehicle");
 else
 if (relevent == &u.weapon && relevent->is_gun())
  mvwprintz(w_target, 1, 1, c_red, "Firing %s (%d)", // - %s (%d)",
            u.weapon.tname().c_str(),// u.weapon.curammo->name.c_str(),
            u.weapon.charges);
 else
  mvwprintz(w_target, 1, 1, c_red, "Throwing %s", relevent->tname().c_str());
 mvwprintz(w_target, 2, 1, c_white,
           "Move cursor to target with directional keys.");
 if (relevent) {
  mvwprintz(w_target, 3, 1, c_white,
            "'<' '>' Cycle targets; 'f' or '.' to fire.");
  mvwprintz(w_target, 4, 1, c_white, 
            "'0' target self; '*' toggle snap-to-target");
 }

 wrefresh(w_target);
 char ch;
 bool snap_to_target = OPTIONS[OPT_SNAP_TO_TARGET];
// The main loop.
 do {
  point center;
  if (snap_to_target)
   center = point(x, y);
  else
   center = point(u.posx, u.posy);
// Clear the target window.
  for (int i = 5; i < 12; i++) {
   for (int j = 1; j < 46; j++)
    mvwputch(w_target, i, j, c_white, ' ');
  }
  m.draw(this, w_terrain, center);
// Draw the Monsters
  for (int i = 0; i < z.size(); i++) {
   if (u_see(&(z[i]), tart) && z[i].posx >= lowx && z[i].posy >= lowy &&
                               z[i].posx <=  hix && z[i].posy <=  hiy)
    z[i].draw(w_terrain, center.x, center.y, false);
  }
// Draw the NPCs
  for (int i = 0; i < active_npc.size(); i++) {
   if (u_see(active_npc[i].posx, active_npc[i].posy, tart))
    active_npc[i].draw(w_terrain, center.x, center.y, false);
  }
  if (x != u.posx || y != u.posy) {
// Calculate the return vector (and draw it too)
/*
   for (int i = 0; i < ret.size(); i++)
    m.drawsq(w_terrain, u, ret[i].x, ret[i].y, false, true, center.x, center.y);
*/
// Draw the player
   int atx = SEEX + u.posx - center.x, aty = SEEY + u.posy - center.y;
   if (atx >= 0 && atx < SEEX * 2 + 1 && aty >= 0 && aty < SEEY * 2 + 1)
    mvwputch(w_terrain, aty, atx, u.color(), '@');

   if (m.sees(u.posx, u.posy, x, y, -1, tart)) {// Selects a valid line-of-sight
    ret = line_to(u.posx, u.posy, x, y, tart); // Sets the vector to that LOS
// Draw the trajectory
    for (int i = 0; i < ret.size(); i++) {
     if (abs(ret[i].x - u.posx) <= sight_dist &&
         abs(ret[i].y - u.posy) <= sight_dist   ) {
      int mondex = mon_at(ret[i].x, ret[i].y),
          npcdex = npc_at(ret[i].x, ret[i].y);
// NPCs and monsters get drawn with inverted colors
      if (mondex != -1 && u_see(&(z[mondex]), tart))
       z[mondex].draw(w_terrain, center.x, center.y, true);
      else if (npcdex != -1)
       active_npc[npcdex].draw(w_terrain, center.x, center.y, true);
      else
       m.drawsq(w_terrain, u, ret[i].x, ret[i].y, true,true,center.x, center.y);
     }
    }
   }

   if (!relevent) { // currently targetting vehicle to refill with fuel
    vehicle *veh = m.veh_at(x, y);
    if (veh)
     mvwprintw(w_target, 5, 1, "There is a %s", veh->name.c_str());
   } else
    mvwprintw(w_target, 5, 1, "Range: %d", rl_dist(u.posx, u.posy, x, y));

   if (mon_at(x, y) == -1) {
    mvwprintw(w_status, 0, 9, "                             ");
    if (snap_to_target)
     mvwputch(w_terrain, SEEY, SEEX, c_red, '*');
    else
     mvwputch(w_terrain, y + SEEY - u.posy, x + SEEX - u.posx, c_red, '*');
   } else if (u_see(&(z[mon_at(x, y)]), tart))
    z[mon_at(x, y)].print_info(this, w_target);
  }
  wrefresh(w_target);
  wrefresh(w_terrain);
  wrefresh(w_status);
  refresh();
  ch = input();
  get_direction(this, tarx, tary, ch);
  if (tarx != -2 && tary != -2 && ch != '.') {	// Direction character pressed
   int mondex = mon_at(x, y), npcdex = npc_at(x, y);
   if (mondex != -1 && u_see(&(z[mondex]), tart))
    z[mondex].draw(w_terrain, center.x, center.y, false);
   else if (npcdex != -1)
    active_npc[npcdex].draw(w_terrain, center.x, center.y, false);
   else if (m.sees(u.posx, u.posy, x, y, -1, junk))
    m.drawsq(w_terrain, u, x, y, false, true, center.x, center.y);
   else
    mvwputch(w_terrain, SEEY, SEEX, c_black, 'X');
   x += tarx;
   y += tary;
   if (x < lowx)
    x = lowx;
   else if (x > hix)
    x = hix;
   if (y < lowy)
    y = lowy;
   else if (y > hiy)
    y = hiy;
  } else if ((ch == '<') && (target != -1)) {
   target--;
   if (target == -1) target = t.size() - 1;
   x = t[target].posx;
   y = t[target].posy;
  } else if ((ch == '>') && (target != -1)) {
   target++;
   if (target == t.size()) target = 0;
   x = t[target].posx;
   y = t[target].posy;
  } else if (ch == '.' || ch == 'f' || ch == 'F' || ch == '\n') {
   for (int i = 0; i < t.size(); i++) {
    if (t[i].posx == x && t[i].posy == y)
     target = i;
   }
   return ret;
  } else if (ch == '0') {
   x = u.posx;
   y = u.posy;
  } else if (ch == '*')
   snap_to_target = !snap_to_target;
  else if (ch == KEY_ESCAPE || ch == 'q') { // return empty vector (cancel)
   ret.clear();
   return ret;
  }
 } while (true);
}

void game::hit_monster_with_flags(monster &z, unsigned int flags)
{
 if (flags & mfb(IF_AMMO_FLAME)) {

  if (z.made_of(VEGGY) || z.made_of(COTTON) || z.made_of(WOOL) ||
      z.made_of(PAPER) || z.made_of(WOOD))
   z.add_effect(ME_ONFIRE, rng(8, 20));
  else if (z.made_of(FLESH))
   z.add_effect(ME_ONFIRE, rng(5, 10));
  
 } else if (flags & mfb(IF_AMMO_INCENDIARY)) {

  if (z.made_of(VEGGY) || z.made_of(COTTON) || z.made_of(WOOL) ||
      z.made_of(PAPER) || z.made_of(WOOD))
   z.add_effect(ME_ONFIRE, rng(2, 6));
  else if (z.made_of(FLESH) && one_in(4))
   z.add_effect(ME_ONFIRE, rng(1, 4));

 }
}

int time_to_fire(player &p, it_gun* firing)
{
 int time = 0;
 switch (firing->skill_used) {

 case sk_pistol:
  if (p.sklevel[sk_pistol] > 6)
   time = 10;
  else
   time = (80 - 10 * p.sklevel[sk_pistol]);
  break;

 case sk_shotgun:
  if (p.sklevel[sk_shotgun] > 3)
   time = 70;
  else
   time = (150 - 25 * p.sklevel[sk_shotgun]);
  break;

 case sk_smg:
  if (p.sklevel[sk_smg] > 5)
   time = 20;
  else
   time = (80 - 10 * p.sklevel[sk_smg]);
  break;

 case sk_rifle:
  if (p.sklevel[sk_rifle] > 8)
   time = 30;
  else
   time = (150 - 15 * p.sklevel[sk_rifle]);
  break;

 case sk_archery:
  if (p.sklevel[sk_archery] > 8)
   time = 20;
  else
   time = (220 - 25 * p.sklevel[sk_archery]);
  break;

 case sk_launcher:
  if (p.sklevel[sk_launcher] > 8)
   time = 30;
  else
   time = (200 - 20 * p.sklevel[sk_launcher]);
  break;

 default:
  debugmsg("Why is shooting %s using %s skill?", (firing->name).c_str(),
		skill_name(firing->skill_used).c_str());
  time =  0;
 }

 return time;
}

void make_gun_sound_effect(game *g, player &p, bool burst)
{
 std::string gunsound;
 int noise = p.weapon.noise();
 if (noise < 5) {
  if (burst)
   gunsound = "Brrrip!";
  else
   gunsound = "plink!";
 } else if (noise < 25) {
  if (burst)
   gunsound = "Brrrap!";
  else
   gunsound = "bang!";
 } else if (noise < 60) {
  if (burst)
   gunsound = "P-p-p-pow!";
  else
   gunsound = "blam!";
 } else {
  if (burst)
   gunsound = "Kaboom!!";
  else
   gunsound = "kerblam!";
 }
 if (p.weapon.curammo->type == AT_FUSION || p.weapon.curammo->type == AT_BATT ||
     p.weapon.curammo->type == AT_PLUT)
  g->sound(p.posx, p.posy, 8, "Fzzt!");
 else if (p.weapon.curammo->type == AT_40MM)
  g->sound(p.posx, p.posy, 8, "Thunk!");
 else if (p.weapon.curammo->type == AT_GAS)
  g->sound(p.posx, p.posy, 4, "Fwoosh!");
 else if (p.weapon.curammo->type != AT_BOLT &&
          p.weapon.curammo->type != AT_ARROW)
  g->sound(p.posx, p.posy, noise, gunsound);
}

int calculate_range(player &p, int tarx, int tary)
{
 int trange = rl_dist(p.posx, p.posy, tarx, tary);
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 if (trange < int(firing->volume / 3) && firing->ammo != AT_SHOT)
  trange = int(firing->volume / 3);
 else if (p.has_bionic(bio_targeting)) {
  if (trange > LONG_RANGE)
   trange = int(trange * .65);
  else
   trange = int(trange * .8);
 }

 if (firing->skill_used == sk_rifle && trange > LONG_RANGE)
  trange = LONG_RANGE + .6 * (trange - LONG_RANGE);

 return trange;
}

double calculate_missed_by(player &p, int trange)
{
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
// Calculate deviation from intended target (assuming we shoot for the head)
  double deviation = 0.; // Measured in quarter-degrees
// Up to 1.5 degrees for each skill point < 4; up to 1.25 for each point > 4
  if (p.sklevel[firing->skill_used] < 4)
   deviation += rng(0, 6 * (4 - p.sklevel[firing->skill_used]));
  else if (p.sklevel[firing->skill_used] > 4)
   deviation -= rng(0, 5 * (p.sklevel[firing->skill_used] - 4));

  if (p.sklevel[sk_gun] < 3)
   deviation += rng(0, 3 * (3 - p.sklevel[sk_gun]));
  else
   deviation -= rng(0, 2 * (p.sklevel[sk_gun] - 3));

  deviation += p.ranged_dex_mod();
  deviation += p.ranged_per_mod();

  deviation += rng(0, 2 * p.encumb(bp_arms)) + rng(0, 4 * p.encumb(bp_eyes));

  deviation += rng(0, p.weapon.curammo->accuracy);
  deviation += rng(0, p.weapon.accuracy());
  int adj_recoil = p.recoil + p.driving_recoil;
  deviation += rng(int(adj_recoil / 4), adj_recoil);

// .013 * trange is a computationally cheap version of finding the tangent.
// (note that .00325 * 4 = .013; .00325 is used because deviation is a number
//  of quarter-degrees)
// It's also generous; missed_by will be rather short.
  return (.00325 * deviation * trange);
}

int recoil_add(player &p)
{
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 int ret = p.weapon.recoil();
 ret -= rng(p.str_cur / 2, p.str_cur);
 ret -= rng(0, p.sklevel[firing->skill_used] / 2);
 if (ret > 0)
  return ret;
 return 0;
}

void shoot_monster(game *g, player &p, monster &mon, int &dam, double goodhit)
{
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 std::string message;
 int junk;
 bool u_see_mon = g->u_see(&(mon), junk);
 if (mon.has_flag(MF_HARDTOSHOOT) && !one_in(4) &&
     !p.weapon.curammo->m1 == LIQUID && 
     p.weapon.curammo->accuracy >= 4) { // Buckshot hits anyway
  if (u_see_mon)
   g->add_msg("The shot passes through the %s without hitting.",
           mon.name().c_str());
  goodhit = 1;
 } else { // Not HARDTOSHOOT
// Armor blocks BEFORE any critical effects.
  int zarm = mon.armor_cut();
  zarm -= p.weapon.curammo->pierce;
  if (p.weapon.curammo->m1 == LIQUID)
   zarm = 0;
  else if (p.weapon.curammo->accuracy < 4) // Shot doesn't penetrate armor well
   zarm *= rng(2, 4);
  if (zarm > 0)
   dam -= zarm;
  if (dam <= 0) {
   if (u_see_mon)
    g->add_msg("The shot reflects off the %s!",
            mon.name_with_armor().c_str());
   dam = 0;
   goodhit = 1;
  }
  if (goodhit < .1 && !mon.has_flag(MF_NOHEAD)) {
   message = "Headshot!";
   dam = rng(5 * dam, 8 * dam);
   p.practice(firing->skill_used, 5);
  } else if (goodhit < .2) {
   message = "Critical!";
   dam = rng(dam * 2, dam * 3);
   p.practice(firing->skill_used, 2);
  } else if (goodhit < .4) {
   dam = rng(int(dam * .9), int(dam * 1.5));
   p.practice(firing->skill_used, rng(0, 2));
  } else if (goodhit <= .7) {
   message = "Grazing hit.";
   dam = rng(0, dam);
  } else
   dam = 0;
// Find the zombie at (x, y) and hurt them, MAYBE kill them!
  if (dam > 0) {
   mon.moves -= dam * 5;
   if (&p == &(g->u) && u_see_mon)
    g->add_msg("%s You hit the %s for %d damage.", message.c_str(),
            mon.name().c_str(), dam);
   else if (u_see_mon)
    g->add_msg("%s %s shoots the %s.", message.c_str(), p.name.c_str(),
            mon.name().c_str());
   if (mon.hurt(dam))
    g->kill_mon(g->mon_at(mon.posx, mon.posy), (&p == &(g->u)));
   else if (p.weapon.curammo->item_flags != 0)
    g->hit_monster_with_flags(mon, p.weapon.curammo->item_flags);
   dam = 0;
  }
 }
}

void shoot_player(game *g, player &p, player *h, int &dam, double goodhit)
{
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 body_part hit;
 int side = rng(0, 1), junk;
 if (goodhit < .05) {
  hit = bp_eyes;
  dam = rng(3 * dam, 5 * dam);
  p.practice(firing->skill_used, 5);
 } else if (goodhit < .1) {
  if (one_in(6))
   hit = bp_eyes;
  else if (one_in(4))
   hit = bp_mouth;
  else
   hit = bp_head;
  dam = rng(2 * dam, 5 * dam);
  p.practice(firing->skill_used, 5);
 } else if (goodhit < .2) {
  hit = bp_torso;
  dam = rng(dam, 2 * dam);
  p.practice(firing->skill_used, 2);
 } else if (goodhit < .4) {
  if (one_in(3))
   hit = bp_torso;
  else if (one_in(2))
   hit = bp_arms;
  else
   hit = bp_legs;
  dam = rng(int(dam * .9), int(dam * 1.5));
  p.practice(firing->skill_used, rng(0, 1));
 } else if (goodhit < .5) {
  if (one_in(2))
   hit = bp_arms;
  else
   hit = bp_legs;
  dam = rng(dam / 2, dam);
 } else {
  dam = 0;
 }
 if (dam > 0) {
  h->moves -= rng(0, dam);
  if (h == &(g->u))
   g->add_msg("%s shoots your %s for %d damage!", p.name.c_str(),
              body_part_name(hit, side).c_str(), dam);
  else {
   if (&p == &(g->u))
    g->add_msg("You shoot %s's %s.", h->name.c_str(),
               body_part_name(hit, side).c_str());
   else if (g->u_see(h->posx, h->posy, junk))
    g->add_msg("%s shoots %s's %s.",
               (g->u_see(p.posx, p.posy, junk) ? p.name.c_str() : "Someone"),
               h->name.c_str(), body_part_name(hit, side).c_str());
  }
  h->hit(g, hit, side, 0, dam);
/*
  if (h != &(g->u)) {
   int npcdex = g->npc_at(h->posx, h->posy);
   if (g->active_npc[npcdex].hp_cur[hp_head]  <= 0 ||
       g->active_npc[npcdex].hp_cur[hp_torso] <= 0   ) {
    g->active_npc[npcdex].die(g, !p.is_npc());
    g->active_npc.erase(g->active_npc.begin() + npcdex);
   }
  }
*/
 }
}

void splatter(game *g, std::vector<point> trajectory, int dam, monster* mon)
{
 field_id blood = fd_blood;
 if (mon != NULL) {
  if (!mon->made_of(FLESH))
   return;
  if (mon->type->dies == &mdeath::boomer)
   blood = fd_bile;
  else if (mon->type->dies == &mdeath::acid)
   blood = fd_acid;
 }

 int distance = 1;
 if (dam > 50)
  distance = 3;
 else if (dam > 20)
  distance = 2;

 std::vector<point> spurt = continue_line(trajectory, distance);

 for (int i = 0; i < spurt.size(); i++) {
  int tarx = spurt[i].x, tary = spurt[i].y;
  if (g->m.field_at(tarx, tary).type == blood &&
      g->m.field_at(tarx, tary).density < 3)
   g->m.field_at(tarx, tary).density++;
  else
   g->m.add_field(g, tarx, tary, blood, 1);
 }
}

void ammo_effects(game *g, int x, int y, long flags)
{
 if (flags & mfb(IF_AMMO_EXPLOSIVE))
  g->explosion(x, y, 24, 0, false);

 if (flags & mfb(IF_AMMO_FRAG))
  g->explosion(x, y, 12, 28, false);

 if (flags & mfb(IF_AMMO_NAPALM))
  g->explosion(x, y, 18, 0, true);

 if (flags & mfb(IF_AMMO_EXPLOSIVE_BIG))
  g->explosion(x, y, 40, 0, false);

 if (flags & mfb(IF_AMMO_TEARGAS)) {
  for (int i = -2; i <= 2; i++) {
   for (int j = -2; j <= 2; j++)
    g->m.add_field(g, x + i, y + j, fd_tear_gas, 3);
  }
 }
 
 if (flags & mfb(IF_AMMO_SMOKE)) {
  for (int i = -1; i <= 1; i++) {
   for (int j = -1; j <= 1; j++)
    g->m.add_field(g, x + i, y + j, fd_smoke, 3);
  }
 }

 if (flags & mfb(IF_AMMO_FLASHBANG))
  g->flashbang(x, y);

 if (flags & mfb(IF_AMMO_FLAME)) {
  if (g->m.add_field(g, x, y, fd_fire, 1))
   g->m.field_at(x, y).age = 800;
 }

}
