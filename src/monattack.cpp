#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"
#include "material.h"
#include "json.h"
#include "monstergenerator.h"
#include "speech.h"
#include <algorithm>

//Used for e^(x) functions
#include <stdio.h>
#include <math.h>

// for loading monster dialogue:
#include <iostream>
#include <fstream>

#include <limits>  // std::numeric_limits
#define SKIPLINE(stream) stream.ignore(std::numeric_limits<std::streamsize>::max(), '\n')

void mattack::antqueen(monster *z)
{
 std::vector<point> egg_points;
 std::vector<int> ants;
 z->sp_timeout = z->type->sp_freq; // Reset timer
// Count up all adjacent tiles the contain at least one egg.
 for (int x = z->posx() - 2; x <= z->posx() + 2; x++) {
  for (int y = z->posy() - 2; y <= z->posy() + 2; y++) {
   for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
// is_empty() because we can't hatch an ant under the player, a monster, etc.
    if (g->m.i_at(x, y)[i].type->id == "ant_egg" && g->is_empty(x, y)) {
     egg_points.push_back(point(x, y));
     i = g->m.i_at(x, y).size(); // Done looking at this tile
    }
    int mondex = g->mon_at(x, y);
    if (mondex != -1 && (g->zombie(mondex).type->id == "mon_ant_larva" ||
                         g->zombie(mondex).type->id == "mon_ant"         ))
     ants.push_back(mondex);
   }
  }
 }

 if (ants.size() > 0) {
  z->moves -= 100; // It takes a while
  int mondex = ants[ rng(0, ants.size() - 1) ];
  monster *ant = &(g->zombie(mondex));
  if (g->u_see(z->posx(), z->posy()) && g->u_see(ant->posx(), ant->posy()))
   g->add_msg(_("The %s feeds an %s and it grows!"), z->name().c_str(),
              ant->name().c_str());
  if (ant->type->id == "mon_ant_larva")
   ant->poly(GetMType("mon_ant"));
  else
   ant->poly(GetMType("mon_ant_soldier"));
 } else if (egg_points.size() == 0) { // There's no eggs nearby--lay one.
  if (g->u_see(z->posx(), z->posy()))
   g->add_msg(_("The %s lays an egg!"), z->name().c_str());
  g->m.spawn_item(z->posx(), z->posy(), "ant_egg", 1, 0, g->turn);
 } else { // There are eggs nearby.  Let's hatch some.
  z->moves -= 20 * egg_points.size(); // It takes a while
  if (g->u_see(z->posx(), z->posy()))
   g->add_msg(_("The %s tends nearby eggs, and they hatch!"), z->name().c_str());
  for (int i = 0; i < egg_points.size(); i++) {
   int x = egg_points[i].x, y = egg_points[i].y;
   for (int j = 0; j < g->m.i_at(x, y).size(); j++) {
    if (g->m.i_at(x, y)[j].type->id == "ant_egg") {
     g->m.i_rem(x, y, j);
     j = g->m.i_at(x, y).size(); // Max one hatch per tile.
     monster tmp(GetMType("mon_ant_larva"), x, y);
     g->add_zombie(tmp);
    }
   }
  }
 }
}

void mattack::shriek(monster *z)
{
 int j;
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 4 ||
     !g->sees_u(z->posx(), z->posy(), j))
  return; // Out of range
 z->moves -= 240;   // It takes a while
 z->sp_timeout = z->type->sp_freq; // Reset timer
 g->sound(z->posx(), z->posy(), 50, _("a terrible shriek!"));
}

void mattack::rattle(monster *z)
{
 int j;
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 4 ||
     !g->sees_u(z->posx(), z->posy(), j))
  return; // Out of range
 z->moves -= 20;   // It takes a very short while
 z->sp_timeout = z->type->sp_freq; // Reset timer
 g->sound(z->posx(), z->posy(), 10, _("a sibilant rattling sound!"));
}

void mattack::acid(monster *z)
{
    int t;
    int junk = 0;
    if (!g->sees_u(z->posx(), z->posy(), t) ||
       !g->m.clear_path(z->posx(), z->posy(), g->u.posx, g->u.posy, 10, 1, 100, junk)) {
        return; // Can't see/reach you, no attack
    }
    z->moves -= 300;   // It takes a while
    z->sp_timeout = z->type->sp_freq; // Reset timer
    g->sound(z->posx(), z->posy(), 4, _("a spitting noise."));
    int hitx = g->u.posx + rng(-2, 2), hity = g->u.posy + rng(-2, 2);
    std::vector<point> line = line_to(z->posx(), z->posy(), hitx, hity, junk);
    for (int i = 0; i < line.size(); i++) {
        if (g->m.hit_with_acid(g, line[i].x, line[i].y)) {
            if (g->u_see(line[i].x, line[i].y)) {
                g->add_msg(_("A glob of acid hits the %s!"),
                              g->m.tername(line[i].x, line[i].y).c_str());
            }
            return;
        }
    }
    for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <= 3; j++) {
            if (g->m.move_cost(hitx + i, hity +j) > 0 &&
                 g->m.sees(hitx + i, hity + j, hitx, hity, 6, junk) &&
                 ((one_in(abs(j)) && one_in(abs(i))) || (i == 0 && j == 0))) {
                g->m.add_field(g, hitx + i, hity + j, fd_acid, 2);
            }
        }
    }
}

void mattack::shockstorm(monster *z)
{
    int t;
    int junk = 0;
    if (!g->sees_u(z->posx(), z->posy(), t) ||
       !g->m.clear_path(z->posx(), z->posy(), g->u.posx, g->u.posy, 12, 1, 100, junk)) {
        return; // Can't see/reach you, no attack
    }
    z->moves -= 50;   // It takes a while
    z->sp_timeout = z->type->sp_freq; // Reset timer
    g->add_msg(_("A bolt of electricity arcs towards you!"));
    int tarx = g->u.posx + rng(-1, 1) + rng(-1, 1);// 3 in 9 chance of direct hit,
    int tary = g->u.posy + rng(-1, 1) + rng(-1, 1);// 4 in 9 chance of near hit
    if (!g->m.sees(z->posx(), z->posy(), tarx, tary, -1, t)) {
        t = 0;
    }
    std::vector<point> bolt = line_to(z->posx(), z->posy(), tarx, tary, t);
    for (int i = 0; i < bolt.size(); i++) { // Fill the LOS with electricity
        if (!one_in(4)) {
            g->m.add_field(g, bolt[i].x, bolt[i].y, fd_electricity, rng(1, 3));
        }
    }
    // 5x5 cloud of electricity at the square hit
    for (int i = tarx - 2; i <= tarx + 2; i++) {
        for (int j = tary - 2; j <= tary + 2; j++) {
            if (!one_in(4) || (i == 0 && j == 0)) {
                g->m.add_field(g, i, j, fd_electricity, rng(1, 3));
            }
        }
    }
}


void mattack::smokecloud(monster *z)
{
    z->sp_timeout = z->type->sp_freq; // Reset timer
    const int monx = z->posx();
    const int mony = z->posy();
    int junk = 0;
    for (int i = -3; i <= 3; i++) {
        for (int j = -3; j <=3; j++) {
            if( g->m.move_cost( monx + i, mony + j ) != 0 &&
                g->m.clear_path(monx, mony, monx + i, mony + j, 3, 1, 100, junk) ) {
                g->m.add_field(g, monx + i, mony + j, fd_smoke, 2);
            }
        }
    }
    //Round it out a bit
    for (int i = -2; i <= 2; i++){
        if( g->m.move_cost( monx + i, mony + 4 ) != 0 &&
            g->m.clear_path(monx, mony, monx + i, mony + 4, 3, 1, 100, junk) ) {
            g->m.add_field(g, monx + i, mony + 4, fd_smoke, 2);
        }
        if( g->m.move_cost( monx + i, mony - 4 ) != 0 &&
            g->m.clear_path(monx, mony, monx + i, mony - 4, 3, 1, 100, junk) ) {
            g->m.add_field(g, monx + i, mony - 4, fd_smoke, 2);
        }
        if( g->m.move_cost( monx + 4, mony + i ) != 0 &&
            g->m.clear_path(monx, mony, monx + 4, mony + i, 3, 1, 100, junk) ) {
            g->m.add_field(g, monx + 4, mony + i, fd_smoke, 2);
        }
        if( g->m.move_cost( monx - 4, mony + i ) != 0 &&
            g->m.clear_path(monx, mony, monx - 4, mony + i, 3, 1, 100, junk) ) {
            g->m.add_field(g, monx - 4, mony + i, fd_smoke, 2);
        }
    }
}

void mattack::boomer(monster *z)
{
 int j;
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 3 ||
     !g->sees_u(z->posx(), z->posy(), j))
  return; // Out of range
 std::vector<point> line = line_to(z->posx(), z->posy(), g->u.posx, g->u.posy, j);
 z->sp_timeout = z->type->sp_freq; // Reset timer
 z->moves -= 250;   // It takes a while
 bool u_see = g->u_see(z->posx(), z->posy());
 if (u_see)
  g->add_msg(_("The %s spews bile!"), z->name().c_str());
 for (int i = 0; i < line.size(); i++) {
   g->m.add_field(g, line[i].x, line[i].y, fd_bile, 1);
// If bile hit a solid tile, return.
  if (g->m.move_cost(line[i].x, line[i].y) == 0) {
   g->m.add_field(g, line[i].x, line[i].y, fd_bile, 3);
   if (g->u_see(line[i].x, line[i].y))
    g->add_msg(_("Bile splatters on the %s!"),
               g->m.tername(line[i].x, line[i].y).c_str());
   return;
  }
 }
 if (!g->u.uncanny_dodge()) {
  if (rng(0, 10) > g->u.dodge(g) || one_in(g->u.dodge(g)))
   g->u.infect("boomered", bp_eyes, 3, 12, false, 1, 1);
  else if (u_see)
   g->add_msg(_("You dodge it!"));
  g->u.practice(g->turn, "dodge", 10);
 }
}

void mattack::resurrect(monster *z)
{
 if (z->speed < z->type->speed / 2)
  return; // We can only resurrect so many times!
 std::vector<point> corpses;
 int junk;
// Find all corposes that we can see within 4 tiles.
 for (int x = z->posx() - 4; x <= z->posx() + 4; x++) {
  for (int y = z->posy() - 4; y <= z->posy() + 4; y++) {
   if (g->is_empty(x, y) && g->m.sees(z->posx(), z->posy(), x, y, -1, junk)) {
    for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
     if (g->m.i_at(x, y)[i].type->id == "corpse" &&
         g->m.i_at(x, y)[i].corpse->has_flag(MF_REVIVES) &&
         g->m.i_at(x, y)[i].corpse->in_species("ZOMBIE")) {
      corpses.push_back(point(x, y));
      i = g->m.i_at(x, y).size();
     }
    }
   }
  }
 }
 if (corpses.size() == 0) // No nearby corpses
  return;
 z->speed = (z->speed - rng(0, 10)) * .8;
 bool sees_necromancer = (g->u_see(z));
 if (sees_necromancer)
  g->add_msg(_("The %s throws its arms wide..."), z->name().c_str());
 z->sp_timeout = z->type->sp_freq; // Reset timer
 z->moves -= 500;   // It takes a while
 int raised = 0;
 for (int i = 0; i < corpses.size(); i++) {
  int x = corpses[i].x, y = corpses[i].y;
  for (int n = 0; n < g->m.i_at(x, y).size(); n++) {
   if (g->m.i_at(x, y)[n].type->id == "corpse" && one_in(2)) {
    if (g->u_see(x, y))
     raised++;
    g->revive_corpse(x, y, n);
    n = g->m.i_at(x, y).size(); // Only one body raised per tile
   }
  }
 }
 if (raised > 0) {
  if (raised == 1)
   g->add_msg(_("A nearby corpse rises from the dead!"));
  else if (raised < 4)
   g->add_msg(_("A few corpses rise from the dead!"));
  else
   g->add_msg(_("Several corpses rise from the dead!"));
 } else if (sees_necromancer)
  g->add_msg(_("...but nothing seems to happen."));
}

void mattack::science(monster *z) // I said SCIENCE again!
{
 int t, dist = rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy);
 if (dist > 5 || !g->sees_u(z->posx(), z->posy(), t))
  return; // Out of range
 z->sp_timeout = z->type->sp_freq; // Reset timer
 std::vector<point> free;
 for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
  for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
   if (g->is_empty(x, y))
    free.push_back(point(x, y));
  }
 }
 std::vector<int> valid;// List of available attacks
 int index;
 monster tmp(GetMType("mon_manhack"));
 if (dist == 1)
  valid.push_back(1); // Shock
 if (dist <= 2)
  valid.push_back(2); // Radiation
 if (!free.empty()) {
  valid.push_back(3); // Manhack
  valid.push_back(4); // Acid pool
 }
 valid.push_back(5); // Flavor text
 switch (valid[rng(0, valid.size() - 1)]) { // What kind of attack?
 case 1: // Shock the player
  if (!g->u.uncanny_dodge()) {
   g->add_msg(_("The %s shocks you!"), z->name().c_str());
   z->moves -= 150;
   g->u.hurtall(rng(1, 2));
   if (one_in(6) && !one_in(30 - g->u.str_cur)) {
    g->add_msg(_("You're paralyzed!"));
    g->u.moves -= 300;
   }
  }
  break;
 case 2: // Radioactive beam
  g->add_msg(_("The %s opens it's mouth and a beam shoots towards you!"),
             z->name().c_str());
  z->moves -= 400;
  if (!g->u.uncanny_dodge()) {
   if (g->u.dodge(g) > rng(0, 16) && !one_in(g->u.dodge(g)))
    g->add_msg(_("You dodge the beam!"));
   else if (one_in(6))
    g->u.mutate(g);
   else {
    g->add_msg(_("You get pins and needles all over."));
    g->u.radiation += rng(20, 50);
   }
  }
  break;
 case 3: // Spawn a manhack
  g->add_msg(_("The %s opens its coat, and a manhack flies out!"),
             z->name().c_str());
  z->moves -= 200;
  index = rng(0, valid.size() - 1);
  tmp.spawn(free[index].x, free[index].y);
  g->add_zombie(tmp);
  break;
 case 4: // Acid pool
  g->add_msg(_("The %s drops a flask of acid!"), z->name().c_str());
  z->moves -= 100;
  for (int i = 0; i < free.size(); i++)
   g->m.add_field(g, free[i].x, free[i].y, fd_acid, 3);
  break;
 case 5: // Flavor text
  switch (rng(1, 4)) {
  case 1:
   g->add_msg(_("The %s gesticulates wildly!"), z->name().c_str());
   break;
  case 2:
   g->add_msg(_("The %s coughs up a strange dust."), z->name().c_str());
   break;
  case 3:
   g->add_msg(_("The %s moans softly."), z->name().c_str());
   break;
  case 4:
   g->add_msg(_("The %s's skin crackles with electricity."), z->name().c_str());
   z->moves -= 80;
   break;
  }
  break;
 }
}

void mattack::growplants(monster *z)
{
 for (int i = -3; i <= 3; i++) {
  for (int j = -3; j <= 3; j++) {
   if (i == 0 && j == 0)
    j++;
   if (!g->m.has_flag("DIGGABLE", z->posx() + i, z->posy() + j) && one_in(4))
    g->m.ter_set(z->posx() + i, z->posy() + j, t_dirt);
   else if (one_in(3) && g->m.is_destructable(z->posx() + i, z->posy() + j))
    g->m.ter_set(z->posx() + i, z->posy() + j, t_dirtmound); // Destroy walls, &c
   else {
    if (one_in(4)) { // 1 in 4 chance to grow a tree
     int mondex = g->mon_at(z->posx() + i, z->posy() + j);
     if (mondex != -1) {
      if (g->u_see(z->posx() + i, z->posy() + j))
       g->add_msg(_("A tree bursts forth from the earth and pierces the %s!"),
                  g->zombie(mondex).name().c_str());
      int rn = rng(10, 30);
      rn -= g->zombie(mondex).armor_cut();
      if (rn < 0)
       rn = 0;
      if (g->zombie(mondex).hurt(rn))
       g->kill_mon(mondex, (z->friendly != 0));
     } else if (g->u.posx == z->posx() + i && g->u.posy == z->posy() + j) {
// Player is hit by a growing tree
  if (!g->u.uncanny_dodge()) {
       body_part hit = bp_legs;
       int side = random_side(hit);
       if (one_in(4))
        hit = bp_torso;
       else if (one_in(2))
        hit = bp_feet;
       g->add_msg(_("A tree bursts forth from the earth and pierces your %s!"),
                  body_part_name(hit, side).c_str());
       g->u.hit(g, hit, side, 0, rng(10, 30));
      }
     } else {
      int npcdex = g->npc_at(z->posx() + i, z->posy() + j);
      if (npcdex != -1) { // An NPC got hit
       body_part hit = bp_legs;
       int side = random_side(hit);
       if (one_in(4))
        hit = bp_torso;
       else if (one_in(2))
        hit = bp_feet;
       if (g->u_see(z->posx() + i, z->posy() + j))
        g->add_msg(_("A tree bursts forth from the earth and pierces %s's %s!"),
                   g->active_npc[npcdex]->name.c_str(),
                   body_part_name(hit, side).c_str());
       g->active_npc[npcdex]->hit(g, hit, side, 0, rng(10, 30));
      }
     }
     g->m.ter_set(z->posx() + i, z->posy() + j, t_tree_young);
    } else if (one_in(3)) // If no tree, perhaps underbrush
     g->m.ter_set(z->posx() + i, z->posy() + j, t_underbrush);
   }
  }
 }

 if (one_in(5)) { // 1 in 5 chance of making exisiting vegetation grow larger
  for (int i = -5; i <= 5; i++) {
   for (int j = -5; j <= 5; j++) {
    if (i != 0 || j != 0) {
     if (g->m.ter(z->posx() + i, z->posy() + j) == t_tree_young)
      g->m.ter_set(z->posx() + i, z->posy() + j, t_tree); // Young tree => tree
     else if (g->m.ter(z->posx() + i, z->posy() + j) == t_underbrush) {
// Underbrush => young tree
      int mondex = g->mon_at(z->posx() + i, z->posy() + j);
      if (mondex != -1) {
       if (g->u_see(z->posx() + i, z->posy() + j))
        g->add_msg(_("Underbrush forms into a tree, and it pierces the %s!"),
                   g->zombie(mondex).name().c_str());
       int rn = rng(10, 30);
       rn -= g->zombie(mondex).armor_cut();
       if (rn < 0)
        rn = 0;
       if (g->zombie(mondex).hurt(rn))
        g->kill_mon(mondex, (z->friendly != 0));
      } else if (g->u.posx == z->posx() + i && g->u.posy == z->posy() + j) {
  if (!g->u.uncanny_dodge()) {
        body_part hit = bp_legs;
        int side = random_side(hit);
        if (one_in(4))
         hit = bp_torso;
        else if (one_in(2))
         hit = bp_feet;
        g->add_msg(_("The underbrush beneath your feet grows and pierces your %s!"),
                   body_part_name(hit, side).c_str());
        g->u.hit(g, hit, side, 0, rng(10, 30));
       }
      } else {
       int npcdex = g->npc_at(z->posx() + i, z->posy() + j);
       if (npcdex != -1) {
        body_part hit = bp_legs;
        int side = random_side(hit);
        if (one_in(4))
         hit = bp_torso;
        else if (one_in(2))
         hit = bp_feet;
        if (g->u_see(z->posx() + i, z->posy() + j))
         g->add_msg(_("Underbrush grows into a tree, and it pierces %s's %s!"),
                    g->active_npc[npcdex]->name.c_str(),
                    body_part_name(hit, side).c_str());
        g->active_npc[npcdex]->hit(g, hit, side, 0, rng(10, 30));
       }
      }
     }
    }
   }
  }
 }
}

void mattack::grow_vine(monster *z)
{
 z->sp_timeout = z->type->sp_freq;
 z->moves -= 100;
 monster vine(GetMType("mon_creeper_vine"));
 int xshift = rng(0, 2), yshift = rng(0, 2);
 for (int x = 0; x < 3; x++) {
  for (int y = 0; y < 3; y++) {
   int xvine = z->posx() + (x + xshift) % 3 - 1,
       yvine = z->posy() + (y + yshift) % 3 - 1;
   if (g->is_empty(xvine, yvine)) {
    monster vine(GetMType("mon_creeper_vine"));
    vine.sp_timeout = 5;
    vine.spawn(xvine, yvine);
    g->add_zombie(vine);
   }
  }
 }
}

void mattack::vine(monster *z)
{
 std::vector<point> grow;
 int vine_neighbors = 0;
 z->sp_timeout = z->type->sp_freq;
 z->moves -= 100;
 for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
  for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
   if (g->u.posx == x && g->u.posy == y) {
    if ( g->u.uncanny_dodge() ) {
       return;
    }
    else {
     body_part bphit = random_body_part();
     int side = random_side(bphit);
     g->add_msg(_("The %s lashes your %s!"), z->name().c_str(),
                body_part_name(bphit, side).c_str());
     g->u.hit(g, bphit, side, 4, 4);
     z->sp_timeout = z->type->sp_freq;
     z->moves -= 100;
     return;
    }
   } else if (g->is_empty(x, y))
    grow.push_back(point(x, y));
   else {
    const int zid = g->mon_at(x, y);
    if (zid > -1 && g->zombie(zid).type->id == "mon_creeper_vine") {
     vine_neighbors++;
    }
   }
  }
 }
// Calculate distance from nearest hub
 int dist_from_hub = 999;
 for (int i = 0; i < g->num_zombies(); i++) {
  if (g->zombie(i).type->id == "mon_creeper_hub") {
   int dist = rl_dist(z->posx(), z->posy(), g->zombie(i).posx(), g->zombie(i).posy());
   if (dist < dist_from_hub)
    dist_from_hub = dist;
  }
 }
 if (grow.empty() || vine_neighbors > 5 || one_in(7 - vine_neighbors) ||
     !one_in(dist_from_hub))
  return;
 int index = rng(0, grow.size() - 1);
 monster vine(GetMType("mon_creeper_vine"));
 vine.sp_timeout = 5;
 vine.spawn(grow[index].x, grow[index].y);
 g->add_zombie(vine);
}

void mattack::spit_sap(monster *z)
{
// TODO: Friendly biollantes?
 int t = 0;
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 12 ||
     !g->sees_u(z->posx(), z->posy(), t))
  return;

 z->moves -= 150;
 z->sp_timeout = z->type->sp_freq;

 int dist = rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy);
 int deviation = rng(1, 10);
 double missed_by = (.0325 * deviation * dist);
 std::set<std::string> no_effects;

 if (missed_by > 1.) {
  if (g->u_see(z->posx(), z->posy()))
   g->add_msg(_("The %s spits sap, but misses you."), z->name().c_str());

  int hitx = g->u.posx + rng(0 - int(missed_by), int(missed_by)),
      hity = g->u.posy + rng(0 - int(missed_by), int(missed_by));
  std::vector<point> line = line_to(z->posx(), z->posy(), hitx, hity, 0);
  int dam = 5;
  for (int i = 0; i < line.size() && dam > 0; i++) {
   g->m.shoot(g, line[i].x, line[i].y, dam, false, no_effects);
   if (dam == 0 && g->u_see(line[i].x, line[i].y)) {
    g->add_msg(_("A glob of sap hits the %s!"),
               g->m.tername(line[i].x, line[i].y).c_str());
    return;
   }
  }
  g->m.add_field(g, hitx, hity, fd_sap, (dam >= 4 ? 3 : 2));
  return;
 }

 if (g->u_see(z->posx(), z->posy()))
  g->add_msg(_("The %s spits sap!"), z->name().c_str());
 g->m.sees(g->u.posx, g->u.posy, z->posx(), z->posy(), 60, t);
 std::vector<point> line = line_to(z->posx(), z->posy(), g->u.posx, g->u.posy, t);
 int dam = 5;
 for (int i = 0; i < line.size() && dam > 0; i++) {
  g->m.shoot(g, line[i].x, line[i].y, dam, false, no_effects);
  if (dam == 0 && g->u_see(line[i].x, line[i].y)) {
   g->add_msg(_("A glob of sap hits the %s!"),
              g->m.tername(line[i].x, line[i].y).c_str());
   return;
  }
 }
 if (dam <= 0)
  return;
 if (g->u.uncanny_dodge() ) { return; }
 g->add_msg(_("A glob of sap hits you!"));
 g->u.hit(g, bp_torso, -1, dam, 0);
 g->u.add_disease("sap", dam);
}

void mattack::triffid_heartbeat(monster *z)
{
 g->sound(z->posx(), z->posy(), 14, _("thu-THUMP."));
 z->moves -= 300;
 z->sp_timeout = z->type->sp_freq;
 if ((z->posx() < 0 || z->posx() >= SEEX * MAPSIZE) &&
     (z->posy() < 0 || z->posy() >= SEEY * MAPSIZE)   )
  return;
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 5 &&
     !g->m.route(g->u.posx, g->u.posy, z->posx(), z->posy()).empty()) {
  g->add_msg(_("The root walls creak around you."));
  for (int x = g->u.posx; x <= z->posx() - 3; x++) {
   for (int y = g->u.posy; y <= z->posy() - 3; y++) {
    if (g->is_empty(x, y) && one_in(4))
     g->m.ter_set(x, y, t_root_wall);
    else if (g->m.ter(x, y) == t_root_wall && one_in(10))
     g->m.ter_set(x, y, t_dirt);
   }
  }
// Open blank tiles as long as there's no possible route
  int tries = 0;
  while (g->m.route(g->u.posx, g->u.posy, z->posx(), z->posy()).empty() &&
         tries < 20) {
   int x = rng(g->u.posx, z->posx() - 3), y = rng(g->u.posy, z->posy() - 3);
   tries++;
   g->m.ter_set(x, y, t_dirt);
   if (rl_dist(x, y, g->u.posx, g->u.posy > 3 && g->num_zombies() < 30 &&
       g->mon_at(x, y) == -1 && one_in(20))) { // Spawn an extra monster
    std::string montype = "mon_triffid";
    if (one_in(4))
     montype = "mon_creeper_hub";
    else if (one_in(3))
     montype = "mon_biollante";
    monster plant(GetMType(montype));
    plant.spawn(x, y);
    g->add_zombie(plant);
   }
  }

 } else { // The player is close enough for a fight!

  monster triffid(GetMType("mon_triffid"));
  for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
   for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
    if (g->is_empty(x, y) && one_in(2)) {
     triffid.spawn(x, y);
     g->add_zombie(triffid);
    }
   }
  }

 }
}

void mattack::fungus(monster *z)
{
    // TODO: Infect NPCs?
    z->moves -= 200;   // It takes a while
    z->sp_timeout = z->type->sp_freq; // Reset timer
    monster spore(GetMType("mon_spore"));
    int sporex, sporey;
    int mondex;
    //~ the sound of a fungus releasing spores
    g->sound(z->posx(), z->posy(), 10, _("Pouf!"));
    if (g->u_see(z->posx(), z->posy())) {
        g->add_msg(_("Spores are released from the %s!"), z->name().c_str());
    }
    for (int i = -1; i <= 1; i++) {
        for (int j = -1; j <= 1; j++) {
            if (i == 0 && j == 0) {
                continue;
            }
            sporex = z->posx() + i;
            sporey = z->posy() + j;
            mondex = g->mon_at(sporex, sporey);
            if (g->m.move_cost(sporex, sporey) > 0) {
                if (mondex != -1) { // Spores hit a monster
                    if (g->u_see(sporex, sporey) &&
                            !g->zombie(mondex).type->in_species("FUNGUS")) {
                        g->add_msg(_("The %s is covered in tiny spores!"),
                                        g->zombie(mondex).name().c_str());
                    }
                    if (!g->zombie(mondex).make_fungus()) {
                        g->kill_mon(mondex, (z->friendly != 0));
                    }
                } else if (g->u.posx == sporex && g->u.posy == sporey) {
                    // Spores hit the player
                    bool hit = false;
                    if (one_in(4) && g->u.infect("spores", bp_head, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(2) && g->u.infect("spores", bp_torso, 3, 90, false, 1, 3, 120, 1, true)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_arms, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 1)) {
                        hit = true;
                    }
                    if (one_in(4) && g->u.infect("spores", bp_legs, 3, 90, false, 1, 3, 120, 1, true, 0)) {
                        hit = true;
                    }
                    if (hit) {
                        g->add_msg(_("You're covered in tiny spores!"));
                    }
                } else if (one_in(4) && g->num_zombies() <= 1000) { // Spawn a spore
                    spore.spawn(sporex, sporey);
                    g->add_zombie(spore);
                }
            }
        }
    }
}

void mattack::fungus_growth(monster *z)
{
    // Young fungaloid growing into an adult
    if (g->u_see(z->posx(), z->posy())) {
        g->add_msg(_("The %s young fungaloid grows into an adult!"),
                      z->name().c_str());
    }
    z->poly(GetMType("mon_fungaloid"));
}

void mattack::fungus_sprout(monster *z)
{
    z->sp_timeout = z->type->sp_freq; // Reset timer
    for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
        for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
            if (g->u.posx == x && g->u.posy == y) {
                g->add_msg(_("You're shoved away as a fungal wall grows!"));
                g->fling_player_or_monster(&g->u, 0, g->m.coord_to_angle(z->posx(), z->posy(), g->u.posx, g->u.posy), rng(10, 50));
            }
            if (g->is_empty(x, y)) {
                monster wall(GetMType("mon_fungal_wall"));
                wall.spawn(x, y);
                g->add_zombie(wall);
            }
        }
    }
}

void mattack::leap(monster *z)
{
    int linet = 0;
    std::vector<point> options;
    point target = z->move_target();
    int best = rl_dist(z->posx(), z->posy(), target.x, target.y);

    for (int x = z->posx() - 3; x <= z->posx() + 3; x++)
    {
        for (int y = z->posy() - 3; y <= z->posy() + 3; y++)
        {
            if (x == z->posx() && y == z->posy()) {
                continue;
            }
            bool blocked_path = false;
            // check if monster has a clear path to the proposed point
            if (g->m.sees(z->posx(), z->posy(), x, y, z->vision_range(x, y), linet)) {
                std::vector<point> line = line_to(z->posx(), z->posy(), x, y, linet);
                for (int i = 0; i < line.size(); i++)
                {
                    if (g->m.move_cost(line[i].x, line[i].y) == 0)
                    {
                        blocked_path = true;
                    }
                }
            }
            if (!blocked_path && g->is_empty(x, y) &&
                  g->m.sees(z->posx(), z->posy(), x, y, g->light_level(), linet) &&
                  rl_dist(target.x, target.y, x, y) <= best) {
                options.push_back( point(x, y) );
                best = rl_dist(g->u.posx, g->u.posy, x, y);
            }

        }
    }

    // Go back and remove all options that aren't tied for best
    for (int i = 0; i < options.size() && options.size() > 1; i++)
    {
        point p = options[i];
        if (rl_dist(target.x, target.y, options[i].x, options[i].y) != best)
        {
            options.erase(options.begin() + i);
            i--;
        }
    }

    if (options.size() == 0)
        return; // Nowhere to leap!

    z->moves -= 150;
    z->sp_timeout = z->type->sp_freq; // Reset timer
    point chosen = options[rng(0, options.size() - 1)];
    bool seen = g->u_see(z); // We can see them jump...
    z->setpos(chosen);
    seen |= g->u_see(z); // ... or we can see them land
    if (seen) {
        g->add_msg(_("The %s leaps!"), z->name().c_str());
    }
}

void mattack::dermatik(monster *z)
{
    if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 1) {
        return; // Too far to implant
    }

    z->sp_timeout = z->type->sp_freq; // Reset timer

    if (g->u.uncanny_dodge()) { return; }
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    int dodge_check = std::max(g->u.dodge(g) - rng(0, z->type->melee_skill), 0L);
    if (rng(0, 10000) < 10000 / (1 + (99 * exp(-.6 * dodge_check))))
    {
        g->add_msg(_("The %s tries to land on you, but you dodge."), z->name().c_str());
        z->stumble(g, false);
        g->u.practice(g->turn, "dodge", z->type->melee_skill * 2);
        return;
    }

    // Can we swat the bug away?
    int dodge_roll = z->dodge_roll();
    int swat_skill = (g->u.skillLevel("melee") + g->u.skillLevel("unarmed") * 2) / 3;
    int player_swat = dice(swat_skill, 10);
    if (player_swat > dodge_roll) {
        g->add_msg(_("The %s lands on you, but you swat it off."), z->name().c_str());
        if (z->hp >= z->type->hp / 2) {
            z->hurt(1);
        }
        if (player_swat > dodge_roll * 1.5) {
            z->stumble(g, false);
        }
        return;
    }

    // Can the bug penetrate our armor?
    body_part targeted = random_body_part();
    int side = random_side(targeted);
    if (4 < g->u.armor_cut(targeted) / 3) {
        g->add_msg(_("The %s lands on your %s, but can't penetrate your armor."),
                     z->name().c_str(), body_part_name(targeted, side).c_str());
        z->moves -= 150; // Attemped laying takes a while
        return;
    }

    // Success!
    z->moves -= 500; // Successful laying takes a long time
    g->add_msg(_("The %s sinks its ovipositor into your %s!"), z->name().c_str(),
                 body_part_name(targeted, side).c_str());
    g->u.add_disease("dermatik", 14401, false, 1, 1, 0, 0, targeted, side, true);
    g->u.add_memorial_log(_("Injected with dermatik eggs."));
}

void mattack::dermatik_growth(monster *z)
{
    // Dermatik larva growing into an adult
    if (g->u_see(z->posx(), z->posy())) {
        g->add_msg(_("The %s dermatik larva grows into an adult!"),
                      z->name().c_str());
    }
    z->poly(GetMType("mon_dermatik"));
}

void mattack::plant(monster *z)
{
    // Spores taking seed and growing into a fungaloid
    if (!g->spread_fungus(z->posx(), z->posy()) && one_in(20)) {
        if (g->u_see(z->posx(), z->posy())) {
            g->add_msg(_("The %s takes seed and becomes a young fungaloid!"),
                          z->name().c_str());
        }
        z->poly(GetMType("mon_fungaloid_young"));
        z->moves -= 1000; // It takes a while
    } else {
        if (g->u_see(z->posx(), z->posy())) {
        g->add_msg(_("The %s falls to the ground and bursts!"),
                       z->name().c_str());
        }
        z->hp = 0;
    }
}

void mattack::disappear(monster *z)
{
    (void)g; //unused
    z->hp = 0;
}

void mattack::formblob(monster *z)
{
 bool didit = false;
 int thatmon = -1;
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   thatmon = g->mon_at(z->posx() + i, z->posy() + j);
   if (g->u.posx == z->posx() + i && g->u.posy == z->posy() + i) {
// If we hit the player, cover them with slime
    didit = true;
    g->u.add_disease("slimed", rng(0, z->hp));
   } else if (thatmon != -1) {
// Hit a monster.  If it's a blob, give it our speed.  Otherwise, blobify it?
    if (z->speed > 20 && g->zombie(thatmon).type->id == "mon_blob" &&
        g->zombie(thatmon).speed < 85) {
     didit = true;
     g->zombie(thatmon).speed += 5;
     z->speed -= 5;
    } else if (z->speed > 20 && g->zombie(thatmon).type->id == "mon_blob_small") {
     didit = true;
     z->speed -= 5;
     g->zombie(thatmon).speed += 5;
     if (g->zombie(thatmon).speed >= 60)
      g->zombie(thatmon).poly(GetMType("mon_blob"));
    } else if ((g->zombie(thatmon).made_of("flesh") || g->zombie(thatmon).made_of("veggy")) &&
               rng(0, z->hp) > rng(0, g->zombie(thatmon).hp)) { // Blobify!
     didit = true;
     g->zombie(thatmon).poly(GetMType("mon_blob"));
     g->zombie(thatmon).speed = z->speed - rng(5, 25);
     g->zombie(thatmon).hp = g->zombie(thatmon).speed;
    }
   } else if (z->speed >= 85 && rng(0, 250) < z->speed) {
// If we're big enough, spawn a baby blob.
    didit = true;
    z->speed -= 15;
    monster blob(GetMType("mon_blob_small"));
    blob.spawn(z->posx() + i, z->posy() + j);
    blob.speed = z->speed - rng(30, 60);
    blob.hp = blob.speed;
    g->add_zombie(blob);
   }
  }
  if (didit) { // We did SOMEthing.
   if (z->type->id == "mon_blob" && z->speed <= 50) // We shrank!
    z->poly(GetMType("mon_blob"));
   z->moves -= 500;
   z->sp_timeout = z->type->sp_freq; // Reset timer
   return;
  }
 }
}

void mattack::dogthing(monster *z)
{
 if (!one_in(3) || !g->u_see(z))
  return;

 g->add_msg(_("The %s's head explodes in a mass of roiling tentacles!"),
            z->name().c_str());

 for (int x = z->posx() - 2; x <= z->posx() + 2; x++) {
  for (int y = z->posy() - 2; y <= z->posy() + 2; y++) {
   if (rng(0, 2) >= rl_dist(z->posx(), z->posy(), x, y))
    g->m.add_field(g, x, y, fd_blood, 2);
  }
 }

 z->friendly = 0;
 z->poly(GetMType("mon_headless_dog_thing"));
}

void mattack::tentacle(monster *z)
{
    int t;
    if (!g->sees_u(z->posx(), z->posy(), t))
    {
        return;
    }
    g->add_msg(_("The %s lashes its tentacle at you!"), z->name().c_str());
    z->moves -= 100;
    z->sp_timeout = z->type->sp_freq; // Reset timer

    std::vector<point> line = line_to(z->posx(), z->posy(), g->u.posx, g->u.posy, t);
    std::set<std::string> no_effects;
    for (int i = 0; i < line.size(); i++)
    {
        int tmpdam = 20;
        g->m.shoot(g, line[i].x, line[i].y, tmpdam, true, no_effects);
    }

    if (g->u.uncanny_dodge()) { return; }
    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    int dodge_check = std::max(g->u.dodge(g) - rng(0, z->type->melee_skill), 0L);
    if (rng(0, 10000) < 10000 / (1 + (99 * exp(-.6 * dodge_check))))
    {
        g->add_msg(_("You dodge it!"));
        g->u.practice(g->turn, "dodge", z->type->melee_skill*2);
        return;
    }

    body_part hit = random_body_part();
    int dam = rng(10, 20), side = random_side(hit);
    g->add_msg(_("Your %s is hit for %d damage!"), body_part_name(hit, side).c_str(), dam);
    g->u.hit(g, hit, side, dam, 0);
    g->u.practice(g->turn, "dodge", z->type->melee_skill);
}

void mattack::vortex(monster *z)
{
// Make sure that the player's butchering is interrupted!
 if (g->u.activity.type == ACT_BUTCHER &&
     rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) <= 2) {
  g->add_msg(_("The buffeting winds interrupt your butchering!"));
  g->u.activity.type = ACT_NULL;
 }
// Moves are NOT used up by this attack, as it is "passive"
 z->sp_timeout = z->type->sp_freq;
// Before anything else, smash terrain!
 for (int x = z->posx() - 2; x <= z->posx() + 2; x++) {
  for (int y = z->posx() - 2; y <= z->posy() + 2; y++) {
   if (x == z->posx() && y == z->posy()) // Don't throw us!
    y++;
   std::string sound;
   g->m.bash(x, y, 14, sound);
   g->sound(x, y, 8, sound);
  }
 }
 std::set<std::string> no_effects;

 for (int x = z->posx() - 2; x <= z->posx() + 2; x++) {
  for (int y = z->posx() - 2; y <= z->posy() + 2; y++) {
   if (x == z->posx() && y == z->posy()) // Don't throw us!
    y++;
   std::vector<point> from_monster = line_to(z->posx(), z->posy(), x, y, 0);
   while (!g->m.i_at(x, y).empty()) {
    item thrown = g->m.i_at(x, y)[0];
    g->m.i_rem(x, y, 0);
    int distance = 5 - (thrown.weight() / 1700);
    if (distance > 0) {
     int dam = (thrown.weight() / 113) / double(3 + double(thrown.volume() / 6));
     std::vector<point> traj = continue_line(from_monster, distance);
     for (int i = 0; i < traj.size() && dam > 0; i++) {
      g->m.shoot(g, traj[i].x, traj[i].y, dam, false, no_effects);
      int mondex = g->mon_at(traj[i].x, traj[i].y);
      if (mondex != -1) {
       if (g->zombie(mondex).hurt(dam))
        g->kill_mon(mondex, (z->friendly != 0));
       dam = 0;
      }
      if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
       dam = 0;
       i--;
      } else if (traj[i].x == g->u.posx && traj[i].y == g->u.posy) {
      if (! g->u.uncanny_dodge()) {
        body_part hit = random_body_part();
        int side = random_side(hit);
        g->add_msg(_("A %s hits your %s for %d damage!"), thrown.tname().c_str(),
                   body_part_name(hit, side).c_str(), dam);
        g->u.hit(g, hit, side, dam, 0);
        dam = 0;
       }
      }
// TODO: Hit NPCs
      if (dam == 0 || i == traj.size() - 1) {
       if (thrown.made_of("glass")) {
        if (g->u_see(traj[i].x, traj[i].y))
         g->add_msg(_("The %s shatters!"), thrown.tname().c_str());
        for (int n = 0; n < thrown.contents.size(); n++)
         g->m.add_item_or_charges(traj[i].x, traj[i].y, thrown.contents[n]);
        g->sound(traj[i].x, traj[i].y, 16, _("glass breaking!"));
       } else
        g->m.add_item_or_charges(traj[i].x, traj[i].y, thrown);
      }
     }
    } // Done throwing item
   } // Done getting items
// Throw monsters
   int mondex = g->mon_at(x, y);
   if (mondex != -1) {
    int distance = 0, damage = 0;
    monster *thrown = &(g->zombie(mondex));
    switch (thrown->type->size) {
     case MS_TINY:   distance = 10; break;
     case MS_SMALL:  distance = 6; break;
     case MS_MEDIUM: distance = 4; break;
     case MS_LARGE:  distance = 2; break;
     case MS_HUGE:   distance = 0; break;
    }
    damage = distance * 3;
    // subtract 1 unit of distance for every 10 units of density
    // subtract 5 units of damage for every 10 units of density
    material_type* mon_mat = material_type::find_material(thrown->type->mat);
    distance -= mon_mat->density() / 10;
    damage -= mon_mat->density() / 5;

    if (distance > 0) {
     if (g->u_see(thrown))
      g->add_msg(_("The %s is thrown by winds!"), thrown->name().c_str());
     std::vector<point> traj = continue_line(from_monster, distance);
     bool hit_wall = false;
     for (int i = 0; i < traj.size() && !hit_wall; i++) {
      int monhit = g->mon_at(traj[i].x, traj[i].y);
      if (i > 0 && monhit != -1 && !g->zombie(monhit).digging()) {
       if (g->u_see(traj[i].x, traj[i].y))
        g->add_msg(_("The %s hits a %s!"), thrown->name().c_str(),
                   g->zombie(monhit).name().c_str());
       if (g->zombie(monhit).hurt(damage))
        g->kill_mon(monhit, (z->friendly != 0));
       hit_wall = true;
       thrown->setpos(traj[i - 1]);
      } else if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
       hit_wall = true;
       thrown->setpos(traj[i - 1]);
      }
      int damage_copy = damage;
      g->m.shoot(g, traj[i].x, traj[i].y, damage_copy, false, no_effects);
      if (damage_copy < damage)
       thrown->hurt(damage - damage_copy);
     }
     if (hit_wall)
      damage *= 2;
     else {
      thrown->setpos(traj[traj.size() - 1]);
     }
     if (thrown->hurt(damage))
      g->kill_mon(g->mon_at(thrown->posx(), thrown->posy()), (z->friendly != 0));
    } // if (distance > 0)
   } // if (mondex != -1)

   if (g->u.posx == x && g->u.posy == y) { // Throw... the player?! D:
    if (!g->u.uncanny_dodge()) {
     std::vector<point> traj = continue_line(from_monster, rng(2, 3));
     g->add_msg(_("You're thrown by winds!"));
     bool hit_wall = false;
     int damage = rng(5, 10);
     for (int i = 0; i < traj.size() && !hit_wall; i++) {
      int monhit = g->mon_at(traj[i].x, traj[i].y);
      if (i > 0 && monhit != -1 && !g->zombie(monhit).digging()) {
       if (g->u_see(traj[i].x, traj[i].y))
        g->add_msg(_("You hit a %s!"), g->zombie(monhit).name().c_str());
       if (g->zombie(monhit).hurt(damage))
        g->kill_mon(monhit, true); // We get the kill :)
       hit_wall = true;
       g->u.posx = traj[i - 1].x;
       g->u.posy = traj[i - 1].y;
      } else if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
       g->add_msg(_("You slam into a %s"),
                  g->m.tername(traj[i].x, traj[i].y).c_str());
       hit_wall = true;
       g->u.posx = traj[i - 1].x;
       g->u.posy = traj[i - 1].y;
      }
      int damage_copy = damage;
      g->m.shoot(g, traj[i].x, traj[i].y, damage_copy, false, no_effects);
      if (damage_copy < damage)
       g->u.hit(g, bp_torso, -1, damage - damage_copy, 0);
     }
     if (hit_wall)
      damage *= 2;
     else {
      g->u.posx = traj[traj.size() - 1].x;
      g->u.posy = traj[traj.size() - 1].y;
     }
     g->u.hit(g, bp_torso, -1, damage, 0);
     g->update_map(g->u.posx, g->u.posy);
    } // Done with checking for player
   }
  }
 } // Done with loop!
}

void mattack::gene_sting(monster *z)
{
 int j;
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 7 ||
     !g->sees_u(z->posx(), z->posy(), j))
  return; // Not within range and/or sight
 if (g->u.uncanny_dodge()) { return; }
 z->moves -= 150;
 z->sp_timeout = z->type->sp_freq;
 g->add_msg(_("The %s shoots a dart into you!"), z->name().c_str());
 g->u.mutate(g);
}

void mattack::para_sting(monster *z)
{
    int j;
    if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 4 ||
         !g->sees_u(z->posx(), z->posy(), j))
        return; // Not within range and/or sight
    if (g->u.uncanny_dodge()) {
        return;
    }
    z->moves -= 150;
    z->sp_timeout = z->type->sp_freq;
    g->add_msg(_("The %s shoots a dart into you!"), z->name().c_str());
    g->add_msg(_("You feel poison enter your body!"));
    g->u.add_disease("paralyzepoison", 50, false, 1, 20, 100);
}

void mattack::triffid_growth(monster *z)
{
    // Young triffid growing into an adult
    if (g->u_see(z->posx(), z->posy())) {
        g->add_msg(_("The %s young triffid grows into an adult!"),
                      z->name().c_str());
    }
    z->poly(GetMType("mon_triffid"));
}

void mattack::stare(monster *z)
{
 z->moves -= 200;
 z->sp_timeout = z->type->sp_freq;
 int j;
 if (g->sees_u(z->posx(), z->posy(), j)) {
  g->add_msg(_("The %s stares at you, and you shudder."), z->name().c_str());
  g->u.add_disease("teleglow", 800);
 } else {
  g->add_msg(_("A piercing beam of light bursts forth!"));
  std::vector<point> sight = line_to(z->posx(), z->posy(), g->u.posx, g->u.posy, 0);
  for (int i = 0; i < sight.size(); i++) {
   if (g->m.ter(sight[i].x, sight[i].y) == t_reinforced_glass_h ||
       g->m.ter(sight[i].x, sight[i].y) == t_reinforced_glass_v)
    i = sight.size();
   else if (g->m.is_destructable(sight[i].x, sight[i].y))
    g->m.ter_set(sight[i].x, sight[i].y, t_rubble);
  }
 }
}

void mattack::fear_paralyze(monster *z)
{
 if (g->u_see(z->posx(), z->posy())) {
  z->sp_timeout = z->type->sp_freq; // Reset timer
  if (g->u.has_artifact_with(AEP_PSYSHIELD)) {
   g->add_msg(_("The %s probes your mind, but is rebuffed!"), z->name().c_str());
  } else if (rng(1, 20) > g->u.int_cur) {
   g->add_msg(_("The terrifying visage of the %s paralyzes you."),
              z->name().c_str());
   g->u.moves -= 100;
  } else
   g->add_msg(_("You manage to avoid staring at the horrendous %s."),
              z->name().c_str());
 }
}

void mattack::photograph(monster *z)
{
 int t;
 if (z->faction_id == -1 ||
     rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 6 ||
     !g->sees_u(z->posx(), z->posy(), t))
  return;
 z->sp_timeout = z->type->sp_freq;
 z->moves -= 150;
 g->add_msg(_("The %s takes your picture!"), z->name().c_str());
// TODO: Make the player known to the faction
 g->add_event(EVENT_ROBOT_ATTACK, int(g->turn) + rng(15, 30), z->faction_id,
              g->levx, g->levy);
}

void mattack::tazer(monster *z)
{
 int j;
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 2 ||
     !g->sees_u(z->posx(), z->posy(), j))
  return; // Out of range
 if (g->u.uncanny_dodge()) { return; }
 z->sp_timeout = z->type->sp_freq; // Reset timer
 z->moves -= 200;   // It takes a while
 g->add_msg(_("The %s shocks you!"), z->name().c_str());
 int shock = rng(1, 5);
 g->u.hurt(g, bp_torso, -1, shock * rng(1, 3));
 g->u.moves -= shock * 20;
}

void mattack::smg(monster *z)
{
 // Make sure our ammo isn't weird.
 if (z->ammo > 1000) {
   z->ammo = 1000;
   debugmsg("Generated too much ammo (%d) for %s in mattack::smg", z->ammo, z->type->name.c_str());
 }
 int t, fire_t = 0;
 if (z->friendly != 0) {   // Attacking monsters, not the player!
  monster* target = NULL;
  const int iff_dist=24;   // iff check triggers at this distance
  int iff_hangle=15; // iff safety margin (degrees). less accuracy, more paranoia
  int closest = 19;
  int c=0;
  int u_angle = 0;         // player angle relative to turret
  int boo_hoo = 0;         // how many targets were passed due to IFF. Tragically.
  bool iff_trig = false;   // player seen and within range of stray shots
  int pldist=rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy);
  if ( pldist < iff_dist &&
      g->sees_u(z->posx(), z->posy(), t) ) {
      iff_trig = true;
      if ( pldist < 3 ) iff_hangle=( pldist == 2 ? 30 : 60 ); // granularity increases with proximity
      u_angle = g->m.coord_to_angle (z->posx(), z->posy(), g->u.posx, g->u.posy );
  }
  for (int i = 0; i < g->num_zombies(); i++) {
    if ( g->m.sees(z->posx(), z->posy(), g->zombie(i).posx(), g->zombie(i).posy(), 18, t) ) {
      int dist = rl_dist(z->posx(), z->posy(), g->zombie(i).posx(), g->zombie(i).posy());
      if (g->zombie(i).friendly == 0 ) {
        bool safe_target=true;
        if ( iff_trig ) {
          int tangle = g->m.coord_to_angle (z->posx(), z->posy(), g->zombie(i).posx(), g->zombie(i).posy() );
          int diff = abs ( u_angle - tangle );
          if ( diff + iff_hangle > 360 || diff < iff_hangle ) {
              safe_target=false;
              boo_hoo++;
          }
        }
        if (dist < closest && safe_target && !g->zombie(i).is_hallucination()) {
          target = &(g->zombie(i));
          closest = dist;
          fire_t = t;
        }
      } // else if ( advanced_software_upgrade ) {
        // todo; make friendly && unfriendly lists, then select closest non-friendly fire target
    }
    c++;
  }
  z->sp_timeout = z->type->sp_freq; // Reset timer
  if (target == NULL) {// Couldn't find any targets!
    if(boo_hoo > 0 && g->u_see(z->posx(), z->posy()) ) { // because that stupid oaf was in the way!
       if(boo_hoo > 1) {
         g->add_msg(_("Pointed in your direction, the %s emits %d annoyed sounding beeps."),
           z->name().c_str(),boo_hoo);
       } else {
         g->add_msg(_("Pointed in your direction, the %s emits an IFF warning beep."),
           z->name().c_str());
       }
    }
    return;
  }
  z->moves -= 150;   // It takes a while
  if (z->ammo > 0) {
    if (g->u_see(z->posx(), z->posy()))
     g->add_msg(_("The %s fires its smg!"), z->name().c_str());
    npc tmp;
    tmp.name = _("The ") + z->name();

    tmp.skillLevel("smg").level(8);
    tmp.skillLevel("gun").level(4);

    tmp.recoil = 0;
    tmp.posx = z->posx();
    tmp.posy = z->posy();
    tmp.str_cur = 16;
    tmp.dex_cur = 8;
    tmp.per_cur = 12;
    tmp.weapon = item(itypes["smg_9mm"], 0);
    tmp.weapon.curammo = dynamic_cast<it_ammo*>(itypes["9mm"]);
    tmp.weapon.charges = 10;
    std::vector<point> traj = line_to(z->posx(), z->posy(),
                                      target->posx(), target->posy(), fire_t);
    g->fire(tmp, target->posx(), target->posy(), traj, true);
    z->ammo -= 1;
  }
  else {
    if (one_in(3)) {
      g->sound(z->posx(), z->posy(), 2, _("a chk!"));
    }
    else if (one_in(4)) {
      g->sound(z->posx(), z->posy(), 6, _("boop-boop!"));
    }
  }
  return;
 }

// Not friendly; hence, firing at the player
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 24 ||
     !g->sees_u(z->posx(), z->posy(), t))
  return;
 z->sp_timeout = z->type->sp_freq; // Reset timer

 if (!z->has_effect(ME_TARGETED)) {
  g->sound(z->posx(), z->posy(), 6, _("beep-beep-beep!"));
  z->add_effect(ME_TARGETED, 8);
  z->moves -= 100;
  return;
 }
 z->moves -= 150;   // It takes a while

  if (z->ammo > 0) {
    if (g->u_see(z->posx(), z->posy()))
      g->add_msg(_("The %s fires its smg!"), z->name().c_str());
      // Set up a temporary player to fire this gun
      npc tmp;
      tmp.name = _("The ") + z->name();

      tmp.skillLevel("smg").level(8);
      tmp.skillLevel("gun").level(4);

      tmp.recoil = 0;
      tmp.posx = z->posx();
      tmp.posy = z->posy();
      tmp.str_cur = 16;
      tmp.dex_cur = 8;
      tmp.per_cur = 12;
      tmp.weapon = item(itypes["smg_9mm"], 0);
      tmp.weapon.curammo = dynamic_cast<it_ammo*>(itypes["9mm"]);
      tmp.weapon.charges = 10;
      std::vector<point> traj = line_to(z->posx(), z->posy(), g->u.posx, g->u.posy, t);
      g->fire(tmp, g->u.posx, g->u.posy, traj, true);
      z->ammo -= 1;
      z->add_effect(ME_TARGETED, 3);
    }
    else {
      if (one_in(3)) {
        g->sound(z->posx(), z->posy(), 2, _("a chk!"));
      }
      else if (one_in(4)) {
        g->sound(z->posx(), z->posy(), 6, _("boop-boop!"));
      }
    }
}

void mattack::laser(monster *z)
{
 bool sunlight = g->is_in_sunlight(z->posx(), z->posy());
 int t, fire_t = 0;
 if (z->friendly != 0) {   // Attacking monsters, not the player!
  monster* target = NULL;
  const int iff_dist=24;   // iff check triggers at this distance
  int iff_hangle=15; // iff safety margin (degrees). less accuracy, more paranoia
  int closest = 19;
  int c=0;
  int u_angle = 0;         // player angle relative to turret
  int boo_hoo = 0;         // how many targets were passed due to IFF. Tragically.
  bool iff_trig = false;   // player seen and within range of stray shots
  int pldist=rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy);
  if ( pldist < iff_dist &&
      g->sees_u(z->posx(), z->posy(), t) ) {
      iff_trig = true;
      if ( pldist < 3 ) iff_hangle=( pldist == 2 ? 30 : 60 ); // granularity increases with proximity
      u_angle = g->m.coord_to_angle (z->posx(), z->posy(), g->u.posx, g->u.posy );
  }
  for (int i = 0; i < g->num_zombies(); i++) {
    if ( g->m.sees(z->posx(), z->posy(), g->zombie(i).posx(), g->zombie(i).posy(), 18, t) ) {
      int dist = rl_dist(z->posx(), z->posy(), g->zombie(i).posx(), g->zombie(i).posy());
      if (g->zombie(i).friendly == 0 ) {
        bool safe_target=true;
        if ( iff_trig ) {
          int tangle = g->m.coord_to_angle (z->posx(), z->posy(), g->zombie(i).posx(), g->zombie(i).posy() );
          int diff = abs ( u_angle - tangle );
          if ( diff + iff_hangle > 360 || diff < iff_hangle ) {
              safe_target=false;
              boo_hoo++;
          }
        }
        if (dist < closest && safe_target && !g->zombie(i).is_hallucination()) {
          target = &(g->zombie(i));
          closest = dist;
          fire_t = t;
        }
      } // else if ( advanced_software_upgrade ) {
        // todo; make friendly && unfriendly lists, then select closest non-friendly fire target
    }
    c++;
  }
  z->sp_timeout = z->type->sp_freq; // Reset timer
  if (target == NULL) {// Couldn't find any targets!
    if(boo_hoo > 0 && g->u_see(z->posx(), z->posy()) ) { // because that stupid oaf was in the way!
       if(boo_hoo > 1) {
         g->add_msg(_("Pointed in your direction, the %s emits %d annoyed sounding beeps."),
           z->name().c_str(),boo_hoo);
       } else {
         g->add_msg(_("Pointed in your direction, the %s emits an IFF warning beep."),
           z->name().c_str());
       }
    }
    return;
  }
  z->moves -= 150;   // It takes a while
  if (sunlight) {
      if (g->u_see(z->posx(), z->posy()))
       g->add_msg(_("The %s's barrel spins and fires!"), z->name().c_str());
      npc tmp;
      tmp.name = _("The ") + z->name();

      tmp.skillLevel("rifle").level(8);
      tmp.skillLevel("gun").level(4);

      tmp.recoil = 0;
      tmp.posx = z->posx();
      tmp.posy = z->posy();
      tmp.str_cur = 16;
      tmp.dex_cur = 8;
      tmp.per_cur = 12;
      tmp.weapon = item(itypes["cerberus_laser"], 0);
      tmp.weapon.curammo = dynamic_cast<it_ammo*>(itypes["laser_capacitor"]);
      tmp.weapon.charges = 100;
      std::vector<point> traj = line_to(z->posx(), z->posy(),
                                        target->posx(), target->posy(), fire_t);
      g->fire(tmp, target->posx(), target->posy(), traj, true);
    return;
  }
  else {
    if (one_in(3)) {
      if (g->u_see(z->posx(), z->posy()))
         g->add_msg(_("The %s's barrel spins but nothing happens!"), z->name().c_str());
    }
    else if (one_in(4)) {
      g->sound(z->posx(), z->posy(), 6, _("boop-boop!"));
    }
  }
 }

// Not friendly; hence, firing at the player
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 24 ||
     !g->sees_u(z->posx(), z->posy(), t))
  return;
 z->sp_timeout = z->type->sp_freq; // Reset timer

 if (!z->has_effect(ME_TARGETED)) {
  g->sound(z->posx(), z->posy(), 6, _("beep-beep-beep!"));
  z->add_effect(ME_TARGETED, 8);
  z->moves -= 100;
  return;
 }
 z->moves -= 150;   // It takes a while
  if (sunlight) {
    if (g->u_see(z->posx(), z->posy()))
        g->add_msg(_("The %s's barrel spins and fires!"), z->name().c_str());
      // Set up a temporary player to fire this gun
      npc tmp;
      tmp.name = _("The ") + z->name();

      tmp.skillLevel("rifle").level(8);
      tmp.skillLevel("gun").level(4);

      tmp.recoil = 0;
      tmp.posx = z->posx();
      tmp.posy = z->posy();
      tmp.str_cur = 16;
      tmp.dex_cur = 8;
      tmp.per_cur = 12;
      tmp.weapon = item(itypes["cerberus_laser"], 0);
      tmp.weapon.curammo = dynamic_cast<it_ammo*>(itypes["laser_capacitor"]);
      tmp.weapon.charges = 100;
      std::vector<point> traj = line_to(z->posx(), z->posy(), g->u.posx, g->u.posy, t);
      g->fire(tmp, g->u.posx, g->u.posy, traj, true);
      z->add_effect(ME_TARGETED, 3);
  }
  else {
    if (one_in(3)) {
      if (g->u_see(z->posx(), z->posy()))
         g->add_msg(_("The %s's barrel spins but nothing happens!"), z->name().c_str());
    }
    else if (one_in(4)) {
      g->sound(z->posx(), z->posy(), 6, _("boop-boop!"));
    }
  }
}

void mattack::flamethrower(monster *z)
{
    int t;
    if (abs(g->u.posx - z->posx()) > 5 || abs(g->u.posy - z->posy()) > 5 ||
     !g->sees_u(z->posx(), z->posy(), t))
    return; // Out of range
    z->sp_timeout = z->type->sp_freq; // Reset timer
    z->moves -= 500;   // It takes a while
    std::vector<point> traj = line_to(z->posx(), z->posy(), g->u.posx, g->u.posy, t);

    for (int i = 0; i < traj.size(); i++)
    {
        // break out of attack if flame hits a wall
        if (g->m.hit_with_fire(g, traj[i].x, traj[i].y))
        {
            if (g->u_see(traj[i].x, traj[i].y))
                g->add_msg(_("The tongue of flame hits the %s!"),
            g->m.tername(traj[i].x, traj[i].y).c_str());
            return;
        }
        g->m.add_field(g, traj[i].x, traj[i].y, fd_fire, 1);
    }
    if (!g->u.uncanny_dodge()) { g->u.add_disease("onfire", 8); }
}

void mattack::copbot(monster *z)
{
 int t;
 bool sees_u = g->sees_u(z->posx(), z->posy(), t);
 z->sp_timeout = z->type->sp_freq; // Reset timer
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 2 || !sees_u) {
  if (one_in(3)) {
   if (sees_u) {
    if (g->u.unarmed_attack())
     g->sound(z->posx(), z->posy(), 18, _("a robotic voice boom, \"Citizen, Halt!\""));
    else
     g->sound(z->posx(), z->posy(), 18, _("a robotic voice boom, \"\
Please put down your weapon.\""));
   } else
    g->sound(z->posx(), z->posy(), 18,
             _("a robotic voice boom, \"Come out with your hands up!\""));
  } else
   g->sound(z->posx(), z->posy(), 18, _("a police siren, whoop WHOOP"));
  return;
 }
 mattack tmp;
 tmp.tazer(z);
}

void mattack::multi_robot(monster *z)
{
 int t, mode = 0;
 if (!g->sees_u(z->posx(), z->posy(), t))
  return; // Can't see you!
 if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) == 1 && one_in(2))
  mode = 1;
 else if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) <= 5)
  mode = 2;
 else if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) <= 12)
  mode = 3;

 if (mode == 0)
  return; // No attacks were valid!

 switch (mode) {
  case 1: this->tazer(z);        break;
  case 2: this->flamethrower(z); break;
  case 3: this->smg(z);          break;
 }
}

void mattack::ratking(monster *z)
{
    if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 50) {
        return;
    }
    z->sp_timeout = z->type->sp_freq;    // Reset timer

    switch (rng(1, 5)) { // What do we say?
        case 1: g->add_msg(_("\"YOU... ARE FILTH...\"")); break;
        case 2: g->add_msg(_("\"VERMIN... YOU ARE VERMIN...\"")); break;
        case 3: g->add_msg(_("\"LEAVE NOW...\"")); break;
        case 4: g->add_msg(_("\"WE... WILL FEAST... UPON YOU...\"")); break;
        case 5: g->add_msg(_("\"FOUL INTERLOPER...\"")); break;
    }
    if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) <= 10) {
        g->u.add_disease("rat", 30);
    }
}

void mattack::generator(monster *z)
{
 g->sound(z->posx(), z->posy(), 100, "");
 if (int(g->turn) % 10 == 0 && z->hp < z->type->hp)
  z->hp++;
}

void mattack::upgrade(monster *z)
{
 std::vector<int> targets;
 for (int i = 0; i < g->num_zombies(); i++) {
  if (g->zombie(i).type->id == "mon_zombie" &&
      rl_dist(z->posx(), z->posy(), g->zombie(i).posx(), g->zombie(i).posy()) <= 5)
   targets.push_back(i);
 }
 if (targets.empty())
  return;
 z->sp_timeout = z->type->sp_freq; // Reset timer
 z->moves -= 150;   // It takes a while

 monster *target = &( g->zombie( targets[ rng(0, targets.size()-1) ] ) );

 std::string newtype = "mon_zombie";

 switch( rng(1, 10) ) {
  case  1: newtype = "mon_zombie_shrieker";
           break;
  case  2:
  case  3: newtype = "mon_zombie_spitter";
           break;
  case  4:
  case  5: newtype = "mon_zombie_electric";
           break;
  case  6:
  case  7:
  case  8: newtype = "mon_zombie_hunter";
           break;
  case  9: newtype = "mon_zombie_brute";
           break;
  case 10: newtype = "mon_boomer";
           break;
 }

 target->poly(GetMType(newtype));
 if (g->u_see(z->posx(), z->posy()))
  g->add_msg(_("The black mist around the %s grows..."), z->name().c_str());
 if (g->u_see(target->posx(), target->posy()))
  g->add_msg(_("...a zombie becomes a %s!"), target->name().c_str());
}

void mattack::breathe(monster *z)
{
 z->sp_timeout = z->type->sp_freq; // Reset timer
 z->moves -= 100;   // It takes a while

 bool able = (z->type->id == "mon_breather_hub");
 if (!able) {
  for (int x = z->posx() - 3; x <= z->posx() + 3 && !able; x++) {
   for (int y = z->posy() - 3; y <= z->posy() + 3 && !able; y++) {
    int mondex = g->mon_at(x, y);
    if (mondex != -1 && g->zombie(mondex).type->id == "mon_breather_hub")
     able = true;
   }
  }
 }
 if (!able)
  return;

 std::vector<point> valid;
 for (int x = z->posx() - 1; x <= z->posx() + 1; x++) {
  for (int y = z->posy() - 1; y <= z->posy() + 1; y++) {
   if (g->is_empty(x, y))
    valid.push_back( point(x, y) );
  }
 }

 if (!valid.empty()) {
  point place = valid[ rng(0, valid.size() - 1) ];
  monster spawned(GetMType("mon_breather"));
  spawned.sp_timeout = 12;
  spawned.spawn(place.x, place.y);
  g->add_zombie(spawned);
 }
}

void mattack::bite(monster *z) {
    if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 1) {
        return;
    }

    z->sp_timeout = z->type->sp_freq; // Reset timer
    g->add_msg(_("The %s lunges forward!"), z->name().c_str());
    z->moves -= 100;

    if (g->u.uncanny_dodge()) { return; }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    int dodge_check = std::max(g->u.dodge(g) - rng(0, z->type->melee_skill), 0L);
    if (rng(0, 10000) < 10000 / (1 + (99 * exp(-.6 * dodge_check)))) {
        g->add_msg(_("You dodge it!"));
        g->u.practice(g->turn, "dodge", z->type->melee_skill*2);
        return;
    }

    body_part hit = random_body_part();
    int dam = rng(5, 10), side = random_side(hit);
    dam = g->u.hit(g, hit, side, dam, 0);

    if (dam > 0) {
        g->add_msg(_("The %s bites your %s!"), z->name().c_str(), body_part_name(hit, side).c_str());

        if(one_in(14 - dam)) {
            if (g->u.has_disease("bite", hit, side)) {
                g->u.add_disease("bite", 400, false, 1, 1, 0, -1, hit, side, true);
            } else if (g->u.has_disease("infected", hit, side)) {
                g->u.add_disease("infected", 250, false, 1, 1, 0, -1, hit, side, true);
            } else {
                g->u.add_disease("bite", 3601, false, 1, 1, 0, 0, hit, side, true); //6 hours + 1 "tick"
            }
        }
    } else {
        g->add_msg(_("The %s bites your %s, but your armor protects you."), z->name().c_str(), body_part_name(hit, side).c_str());
    }

    g->u.practice(g->turn, "dodge", z->type->melee_skill);
}

void mattack::brandish(monster *z){
    int linet;
    if (!g->sees_u(z->posx(), z->posy(), linet)){
        return; // Only brandish if we can see you!
    }
    z->sp_timeout = 10000; // Reset timer
    g->add_msg("He's brandishing a knife!");
    g->add_msg("Quiet, quiet");
}

void mattack::flesh_golem(monster *z)
{
    if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 1) {
        if (one_in(12)) {
            int j;
            if (rl_dist(z->posx(), z->posy(), g->u.posx, g->u.posy) > 20 ||
                    !g->sees_u(z->posx(), z->posy(), j)) {
                return; // Out of range
            }
            z->moves -= 200;
            z->sp_timeout = z->type->sp_freq; // Reset timer
            g->sound(z->posx(), z->posy(), 80, _("a terrifying roar that nearly deafens you!"));
        }
        return;
    }
    z->sp_timeout = z->type->sp_freq; // Reset timer
    g->add_msg(_("The %s swings a massive claw at you!"), z->name().c_str());
    z->moves -= 100;

    if (g->u.uncanny_dodge()) { return; }

    // Can we dodge the attack? Uses player dodge function % chance (melee.cpp)
    int dodge_check = std::max(g->u.dodge(g) - rng(0, z->type->melee_skill), 0L);
    if (rng(0, 10000) < 10000 / (1 + (99 * exp(-.6 * dodge_check)))) {
        g->add_msg(_("You dodge it!"));
        g->u.practice(g->turn, "dodge", z->type->melee_skill*2);
        return;
    }
    body_part hit = random_body_part();
    int dam = rng(5, 10), side = random_side(hit);
    g->add_msg(_("Your %s is battered for %d damage!"), body_part_name(hit, side).c_str(), dam);
    g->u.hit(g, hit, side, dam, 0);
    if (one_in(6)) {
        g->u.add_disease("downed", 30);
    }
    g->u.practice(g->turn, "dodge", z->type->melee_skill);
}

void mattack::parrot(monster *z)
{
    if (one_in(20)) {
        z->moves -= 100;  // It takes a while
        z->sp_timeout = z->type->sp_freq;  // Reset timer
        const SpeechBubble speech = get_speech( "migo" );
        g->sound(z->posx(), z->posy(), speech.volume, speech.text);
    }
}
