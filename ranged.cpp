#include <vector>
#include <string>
#include "game.h"
#include "keypress.h"
#include "output.h"
#include "line.h"
#include "skill.h"
#include "rng.h"

void game::fire(player &p, int tarx, int tary, std::vector<point> &trajectory,
                bool burst)
{
 // If we aren't wielding a loaded gun, we can't shoot!
 item ammotmp = item(p.weapon.curammo, 0);
 ammotmp.charges = 1;
 if (!p.weapon.is_gun()) {
  debugmsg("%s tried to fire a non-gun (%s).", p.name.c_str(),
                                               p.weapon.tname().c_str());
  return;
 }
 bool is_bolt = false;
 if (p.weapon.curammo->type == AT_BOLT)	// Bolts are silent
  is_bolt = true;

 int x = p.posx, y = p.posy;
 it_gun* firing = dynamic_cast<it_gun*>(p.weapon.type);
 if (p.has_trait(PF_TRIGGERHAPPY) && one_in(40))
  burst = true;
 if (burst && p.weapon.burst_size() < 2)
  burst = false;	// Can't burst fire a semi-auto

 int junk = 0;
 bool u_see_shooter = u_see(p.posx, p.posy, junk);
// Use different amounts of time depending on the type of gun and our skill
 switch (firing->skill_used) {
 case (sk_pistol):
  if (p.sklevel[sk_pistol] > 8)
   p.moves -= 10;
  else
   p.moves -= (50 - 5 * p.sklevel[sk_pistol]);
  break;
 case (sk_shotgun):
  if (p.sklevel[sk_shotgun] > 3)
   p.moves -= 70;
  else
   p.moves -= (150 - 25 * p.sklevel[sk_shotgun]);
 break;
 case (sk_smg):
  if (p.sklevel[sk_smg] > 5)
   p.moves -= 20;
  else
   p.moves -= (80 - 10 * p.sklevel[sk_smg]);
 break;
 case (sk_rifle):
  if (p.sklevel[sk_rifle] > 8)
   p.moves -= 30;
  else
   p.moves -= (150 - 15 * p.sklevel[sk_rifle]);
 break;
 default:
  debugmsg("Why is shooting %s using %s skill?", (firing->name).c_str(),
		skill_name(firing->skill_used).c_str());
 }

// Decide how many shots to fire
 int num_shots = 1;
 if (burst)
  num_shots = p.weapon.burst_size();
 if (num_shots > p.weapon.charges)
  num_shots = p.weapon.charges;

 if (num_shots == 0)
  debugmsg("game::fire() - num_shots = 0!");
 
 // Make a sound at our location - Zombies will chase it
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
  sound(p.posx, u.posy, 8, "Fzzt!");
 else if (!is_bolt)
  sound(p.posx, p.posy, noise, gunsound);
// Set up a timespec for use in the nanosleep function below
 timespec ts;
 ts.tv_sec = 0;
 ts.tv_nsec = BULLET_SPEED;

// Use up some ammunition
 p.weapon.charges -= num_shots;
 double deviation;
 double missed_by;
 int trange = trig_dist(p.posx, p.posy, tarx, tary);
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
 std::string message = "";
 bool missed;
 int tart;
 for (int curshot = 0; curshot < num_shots; curshot++) {
// Calculate deviation from intended target (assuming we shoot for the head)
  deviation = 0;
// Up to 1.5 degrees for each skill point < 4; up to 1.25 for each point > 4
  if (p.sklevel[firing->skill_used] < 4)
   deviation += rng(0, 6 * (4 - p.sklevel[firing->skill_used]));
  else
   deviation -= rng(0, 5 * (4 - p.sklevel[firing->skill_used] - 4));

  if (p.sklevel[sk_gun] < 3)
   deviation += rng(0, 3 * (3 - p.sklevel[sk_gun]));
  else
   deviation -= rng(0, 2 * (p.sklevel[sk_gun] - 3));

  deviation += p.ranged_dex_mod();
  deviation += p.ranged_per_mod();

  deviation += rng(0, 2 * p.encumb(bp_arms)) + rng(0, 4 * p.encumb(bp_eyes));

  deviation += rng(0, p.weapon.curammo->accuracy);
  deviation += rng(0, p.weapon.accuracy());
  deviation += rng(int(p.recoil / 4), p.recoil);

  int recoil_add = p.weapon.recoil();
  recoil_add -= rng(p.str_cur / 2, p.str_cur);
  recoil_add -= rng(0, p.sklevel[firing->skill_used] / 2);
  if (recoil_add > 0)
   p.recoil += recoil_add;

// .013 * trange is a computationally cheap version of finding the tangent.
// (note that .00325 * 4 = .013; .00325 is used because deviation is a number
//  of quarter-degrees)
// It's also generous; missed_by will be rather short.
  missed_by = .00325 * deviation * trange;
  missed = false;
  if (debugmon)
   debugmsg("\
missed_by %f deviation %f trange %d charges %d myx %d myy %d monx %d mony %d",
missed_by, deviation, trange, p.weapon.charges, p.posx, p.posy, tarx, tary);
  if (missed_by >= 1) {
// We missed D:
// Shoot a random nearby space?
   tarx += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
   tary += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
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
  } else if (missed_by >= .7) {
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
  for (int i = 0; i < trajectory.size() && dam > 0; i++) {
   if (i > 0)
    m.drawsq(w_terrain, u, trajectory[i-1].x, trajectory[i-1].y, false, true);
// Drawing the bullet uses player u, and not player p, because it's drawn
// relative to YOUR position, which may not be the gunman's position.
   if (u_see(trajectory[i].x, trajectory[i].y, junk)) {
    mvwputch(w_terrain, trajectory[i].y + SEEY - u.posy,
                        trajectory[i].x + SEEX - u.posx, c_red, '`');
    wrefresh(w_terrain);
    nanosleep(&ts, NULL);
   }
   
   if (dam <= 0) {
    if (is_bolt &&
        ((u.weapon.curammo->m1 == WOOD && !one_in(5)) ||
         (u.weapon.curammo->m1 != WOOD && !one_in(15))))
     m.add_item(trajectory[i].x, trajectory[i].y, ammotmp);
    return;
   }
   message = "";
   int tx = trajectory[i].x;
   int ty = trajectory[i].y;
// If there's a monster in the path of our bullet, and either our aim was true,
//  OR it's not the monster we were aiming at and we were lucky enough to hit it
   int mondex = mon_at(tx, ty);
   if (mondex != -1 && (!z[mondex].has_flag(MF_DIGS) ||
       rl_dist(p.posx, p.posy, z[mondex].posx, z[mondex].posy) <= 1) &&
       (!missed || one_in((7 - int(z[mon_at(tx, ty)].type->size))))) {
// If we shot us a monster...
// Calculate damage depending on how good our hit was
    double goodhit = missed_by;
    bool u_see_mon = u_see(&(z[mondex]), junk);
    if (i < trajectory.size() - 1)
     goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;
    if (z[mondex].has_flag(MF_HARDTOSHOOT) && !one_in(6) &&
        p.weapon.curammo->accuracy >= 4) { // Shot hits anyway
     if (u_see_mon)
      add_msg("The shot passes through the %s without hitting.",
              z[mondex].name().c_str());
     goodhit = 1;
    } else {
// Armor blocks BEFORE any critical effects.
     int zarm = z[mondex].armor();
     zarm -= p.weapon.curammo->pierce;
     if (p.weapon.curammo->accuracy < 4) // Shot doesn't penetrate armor well
      zarm *= rng(2, 4);
     if (zarm > 0)
      dam -= zarm;
     if (goodhit <= .5 && dam < 0) {
      if (u_see_mon)
       add_msg("The shot reflects off the %s%s!",
               z[mondex].name().c_str(),
               z[mondex].made_of(FLESH) ? "'s armored hide" : "");
      dam = 0;
      goodhit = 1;
     }
     if (goodhit < .1 && !z[mondex].has_flag(MF_NOHEAD)) {
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
      z[mondex].moves -= dam * 8;
      if (&p == &u && u_see_mon)
       add_msg("%s You hit the %s for %d damage.", message.c_str(),
               z[mondex].name().c_str(), dam);
      else if (u_see_mon)
       add_msg("%s %s shoots the %s.", message.c_str(), p.name.c_str(),
               z[mondex].name().c_str());
      if (z[mondex].hurt(dam))
       kill_mon(mondex);
      dam = 0;
     }
    }
   } else if ((!missed || one_in(5)) && (npc_at(tx, ty) != -1 ||
                                         (u.posx == tx && u.posy == ty)))  {
    double goodhit = missed_by;
    body_part hit;
    player *h;
    if (u.posx == tx && u.posy == ty)
     h = &u;
    else
     h = &(active_npc[npc_at(tx, ty)]);
    int side = rng(0, 1);
    if (i < trajectory.size() - 1)
     goodhit = double(rand() / (RAND_MAX + 1.0)) / 2;
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
     dam = rng(1.5 * dam, 3 * dam);
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
     if (h == &u) {
      add_msg("%s %s shoots your %s!", message.c_str(), p.name.c_str(),
              body_part_name(hit, side).c_str());
     } else {
      h->moves -= dam * 4;
      if (hit == bp_legs)
       h->moves -= dam * 4;
      if (&p == &u)
       add_msg("%s You hit %s's %s for %d damage.",
               message.c_str(), h->name.c_str(),
               body_part_name(hit, side).c_str(), dam);
      else if (u_see(tx, ty, junk))
       add_msg("%s %s shoots %s's %s for %d damage.",
               message.c_str(), p.name.c_str(), h->name.c_str(),
               body_part_name(hit, side).c_str(), dam);
     }
     h->hit(this, hit, side, 0, dam);
     if (h != &u) {
      int npcdex = npc_at(tx, ty);
      if (active_npc[npcdex].hp_cur[hp_head] <= 0 ||
          active_npc[npcdex].hp_cur[hp_torso] <= 0) {
       active_npc[npcdex].die(this, !p.is_npc());
       active_npc.erase(active_npc.begin() + npcdex);
      }
     }
    }
   } else
    m.shoot(this, tx, ty, dam, i == trajectory.size() - 1);
  }
  if (is_bolt &&
      ((u.weapon.curammo->m1 == WOOD && !one_in(5)) ||
       (u.weapon.curammo->m1 != WOOD && !one_in(15)))) {
    int lastx = trajectory[trajectory.size() - 1].x;
    int lasty = trajectory[trajectory.size() - 1].y;
    if (m.move_cost(lastx, lasty) == 0) {
     lastx = trajectory[trajectory.size() - 2].x;
     lasty = trajectory[trajectory.size() - 2].y;
    }
    m.add_item(lastx, lasty, ammotmp);
  }
 }

 if (p.weapon.charges == 0)
  p.weapon.curammo = NULL;
}


void game::throw_item(player &p, int tarx, int tary, item &thrown,
                      std::vector<point> &trajectory)
{
 int deviation = 0;
 int trange = 1.5 * trig_dist(p.posx, p.posy, tarx, tary);

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
  tarx += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
  tary += rng(0 - int(sqrt(missed_by)), int(sqrt(missed_by)));
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
 int dam = (thrown.weight() + thrown.type->melee_dam + p.str_cur) /
            double(2 + double(thrown.volume() / 6));
 int i, tx, ty;
 for (i = 0; i < trajectory.size() && dam > 0; i++) {
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
    if (thrown.type->melee_cut > z[mon_at(tx, ty)].armor())
     dam += (thrown.type->melee_cut - z[mon_at(tx, ty)].armor());
   }
   if (thrown.made_of(GLASS) && !thrown.active && // active = molotov, etc.
       rng(0, thrown.volume() + 8) - rng(0, p.str_cur) < thrown.volume()) {
    if (u_see(tx, ty, tart))
    add_msg("The %s shatters!", thrown.tname().c_str());
    for (int i = 0; i < thrown.contents.size(); i++)
     m.add_item(tx, ty, thrown.contents[i]);
    sound(tx, ty, 16, "glass breaking!");
    int glassdam = rng(0, thrown.volume() * 2);
    if (glassdam > z[mon_at(tx, ty)].armor())
     dam += (glassdam - z[mon_at(tx, ty)].armor());
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
    add_msg("%s hits the %s for %d damage.",
            z[mon_at(tx, ty)].name().c_str(), dam);
   if (z[mon_at(tx, ty)].hurt(dam))
    kill_mon(mon_at(tx, ty));
   return;
  } else // No monster hit, but the terrain might be.
   m.shoot(this, tx, ty, dam, false);
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
    dist = trig_dist(t[i].posx, t[i].posy, u.posx, u.posy);
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
 if (relevent == &u.weapon && relevent->is_gun())
  mvwprintz(w_target, 1, 1, c_red, "Firing %s - %s (%d)",
            u.weapon.tname().c_str(), u.weapon.curammo->name.c_str(),
            u.weapon.charges);
 else
  mvwprintz(w_target, 1, 1, c_red, "Throwing %s", relevent->tname().c_str());
 mvwprintz(w_target, 2, 1, c_white,
           "Move cursor to target with directional keys.");
 mvwprintz(w_target, 3, 1, c_white,
           "'<' '>' Cycle targets; 'f' or '.' to fire.");
 wrefresh(w_target);
 char ch;
// The main loop.
 do {
// Clear the target window.
  for (int i = 4; i < 12; i++) {
   for (int j = 1; j < 46; j++)
    mvwputch(w_target, i, j, c_white, ' ');
  }
  if (x != u.posx || y != u.posy) {
// Calculate the return vector (and draw it too)
   for (int i = 0; i < ret.size(); i++)
    m.drawsq(w_terrain, u, ret[i].x, ret[i].y, false, true);
// Draw the player
   mvwputch(w_terrain, SEEX, SEEY, u.color(), '@');
// Draw the Monsters
   for (int i = 0; i < z.size(); i++) {
    if (u_see(&(z[i]), tart) && z[i].posx >= lowx && z[i].posy >= lowy &&
                                z[i].posx <=  hix && z[i].posy <=  hiy)
     z[i].draw(w_terrain, u.posx, u.posy, false);
   }
// Draw the NPCs
   for (int i = 0; i < active_npc.size(); i++) {
    if (u_see(active_npc[i].posx, active_npc[i].posy, tart))
     active_npc[i].draw(w_terrain, u.posx, u.posy, false);
   }
   if (m.sees(u.posx, u.posy, x, y, -1, tart)) {// Selects a valid line-of-sight
    ret = line_to(u.posx, u.posy, x, y, tart); // Sets the vector to that LOS
    for (int i = 0; i < ret.size(); i++) {
     if (abs(ret[i].x - u.posx) <= sight_dist &&
         abs(ret[i].y - u.posy) <= sight_dist   ) {
      if (mon_at(ret[i].x, ret[i].y) != -1 &&
          u_see(&(z[mon_at(ret[i].x, ret[i].y)]), tart))
       z[mon_at(ret[i].x, ret[i].y)].draw(w_terrain, u.posx, u.posy, true);
      else if (npc_at(ret[i].x, ret[i].y) != -1)
       active_npc[npc_at(ret[i].x, ret[i].y)].draw(w_terrain, u.posx, u.posy,
                                                   true);
      else
       m.drawsq(w_terrain, u, ret[i].x, ret[i].y, true, true);
     }
    }
   }
   if (mon_at(x, y) == -1) {
    mvwputch(w_terrain, y + SEEY - u.posy, x + SEEX - u.posx, c_red, '*');
    mvwprintw(w_status, 0, 9, "                             ");
   } else if (u_see(&(z[mon_at(x, y)]), tart))
    z[mon_at(x, y)].print_info(this, w_target);
   wrefresh(w_target);
  }
  wrefresh(w_terrain);
  wrefresh(w_status);
  refresh();
  ch = input();
  get_direction(tarx, tary, ch);
  if (tarx != -2 && tary != -2 && ch != '.') {	// Direction character pressed
   if (m.sees(u.posx, u.posy, x, y, -1, junk))
    m.drawsq(w_terrain, u, x, y, false, true);
   else
    mvwputch(w_terrain, y + SEEY - u.posy, x + SEEX - u.posx, c_black, 'X');
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
  } else if (ch == '.' || ch == 'f' || ch == 'F') {
   for (int i = 0; i < t.size(); i++) {
    if (t[i].posx == x && t[i].posy == y)
     target = i;
   }
   return ret;
  } else if (ch == KEY_ESCAPE || ch == 'q') { // return empty vector (cancel)
   ret.clear();
   return ret;
  }
 } while (true);
}
