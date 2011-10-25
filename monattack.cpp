#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"

void mattack::antqueen(game *g, monster *z)
{
 std::vector<point> egg_points;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
// Count up all adjacent tiles the contain at least one egg.
 for (int x = z->posx - 1; x <= z->posx + 1; x++) {
  for (int y = z->posy - 1; y <= z->posy + 1; y++) {
   for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
// is_empty() because we can't hatch an ant under the player, a monster, etc.
    if (g->m.i_at(x, y)[i].type->id == itm_ant_egg && g->is_empty(x, y)) {
     egg_points.push_back(point(x, y));
     i = g->m.i_at(x, y).size();	// Done looking at this tile
    }
   }
  }
 }
 if (egg_points.size() == 0) {	// There's no eggs nearby.  Let's lay one.
  int junk;
  if (g->u_see(z->posx, z->posy, junk))
   g->add_msg("The %s lays an egg!", z->name().c_str());
  g->m.add_item(z->posx, z->posy, g->itypes[itm_ant_egg], g->turn);
 } else {			// There are eggs nearby.  Let's hatch some.
  int junk;
  if (g->u_see(z->posx, z->posy, junk))
   g->add_msg("The %s tends nearby eggs, and they hatch!", z->name().c_str());
  for (int i = 0; i < egg_points.size(); i++) {
   int x = egg_points[i].x, y = egg_points[i].y;
   for (int j = 0; j < g->m.i_at(x, y).size(); j++) {
    if (g->m.i_at(x, y)[j].type->id == itm_ant_egg) {
     g->m.i_rem(x, y, j);
     j = g->m.i_at(x, y).size();	// Max one hatch per tile.
     monster tmp(g->mtypes[mon_ant_larva], x, y);
     g->z.push_back(tmp);
    }
   }
  }
 }
}

void mattack::shriek(game *g, monster *z)
{
 int j;
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 4 ||
     !g->sees_u(z->posx, z->posy, j))
  return;	// Out of range
 z->moves = -240;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 g->sound(z->posx, z->posy, 50, "a terrible shriek!");
}

void mattack::acid(game *g, monster *z)
{
 int junk;
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 10 ||
     !g->sees_u(z->posx, z->posy, junk))
  return;	// Out of range
 z->moves = -300;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 g->sound(z->posx, z->posy, 4, "a spitting noise.");
 int hitx = g->u.posx + rng(-2, 2), hity = g->u.posy + rng(-2, 2);
 std::vector<point> line = line_to(z->posx, z->posy, hitx, hity, junk);
 for (int i = 0; i < line.size(); i++) {
  if (g->m.hit_with_acid(g, line[i].x, line[i].y)) {
   if (g->u_see(line[i].x, line[i].y, junk))
    g->add_msg("A glob of acid hits the %s!",
               g->m.tername(line[i].x, line[i].y).c_str());
   return;
  }
 }
 for (int i = -3; i <= 3; i++) {
  for (int j = -3; j <= 3; j++) {
   if (g->m.move_cost(hitx + i, hity +j) > 0 &&
       g->m.sees(hitx + i, hity + j, hitx, hity, 6, junk) &&
       ((one_in(abs(j)) && one_in(abs(i))) || (i == 0 && j == 0))) {
    if (g->m.field_at(hitx + i, hity + j).type == fd_acid &&
        g->m.field_at(hitx + i, hity + j).density < 3)
     g->m.field_at(hitx + i, hity + j).density++;
    else
     g->m.add_field(g, hitx + i, hity + j, fd_acid, 2);
   }
  }
 }
}

void mattack::shockstorm(game *g, monster *z)
{
 int t;
 if (!g->sees_u(z->posx, z->posy, t) ||
     rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 12)
  return;	// Can't see you, no attack
 z->moves = -50;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 g->add_msg("A bolt of electricity arcs towards you!");
 int tarx = g->u.posx + rng(-1, 1) + rng(-1, 1),// 3 in 9 chance of direct hit,
     tary = g->u.posy + rng(-1, 1) + rng(-1, 1);// 4 in 9 chance of near hit
 if (!g->m.sees(z->posx, z->posy, tarx, tary, -1, t))
  t = 0;
 std::vector<point> bolt = line_to(z->posx, z->posy, tarx, tary, t);
 for (int i = 0; i < bolt.size(); i++) { // Fill the LOS with electricity
  if (!one_in(4))
   g->m.add_field(g, bolt[i].x, bolt[i].y, fd_electricity, rng(1, 3));
 }
// 3x3 cloud of electricity at the square hit
 for (int i = tarx - 1; i <= tarx + 1; i++) {
  for (int j = tary - 1; j <= tary + 1; j++) {
   if (!one_in(6))
    g->m.add_field(g, i, j, fd_electricity, rng(1, 3));
  }
 }
}

void mattack::boomer(game *g, monster *z)
{
 int j;
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 3 ||
     !g->sees_u(z->posx, z->posy, j))
  return;	// Out of range
 std::vector<point> line = line_to(z->posx, z->posy, g->u.posx, g->u.posy, j);
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -250;			// It takes a while
 bool u_see = g->u_see(z->posx, z->posy, j);
 if (u_see)
  g->add_msg("The %s spews bile!", z->name().c_str());
 for (int i = 0; i < line.size(); i++) {
  if (g->m.field_at(line[i].x, line[i].y).type == fd_blood) {
   g->m.field_at(line[i].x, line[i].y).type = fd_bile;
   g->m.field_at(line[i].x, line[i].y).density = 1;
  } else if (g->m.field_at(line[i].x, line[i].y).type == fd_bile &&
             g->m.field_at(line[i].x, line[i].y).density < 3)
   g->m.field_at(line[i].x, line[i].y).density++;
  else
   g->m.add_field(g, line[i].x, line[i].y, fd_bile, 1);
// If bile hit a solid tile, return.
  if (g->m.move_cost(line[i].x, line[i].y) == 0) {
   g->m.add_field(g, line[i].x, line[i].y, fd_bile, 3);
   if (g->u_see(line[i].x, line[i].y, j))
    g->add_msg("Bile splatters on the %s!",
               g->m.tername(line[i].x, line[i].y).c_str());
   return;
  }
 }
 if (rng(0, 10) > g->u.dodge() || one_in(g->u.dodge()))
  g->u.infect(DI_BOOMERED, bp_eyes, 3, 12, g);
 else if (u_see)
  g->add_msg("You dodge it!");
}

void mattack::resurrect(game *g, monster *z)
{
 if (z->speed < z->type->speed / 2)
  return;	// We can only resurrect so many times!
 std::vector<point> corpses;
 int junk;
// Find all corposes that we can see within 4 tiles.
 for (int x = z->posx - 4; x <= z->posx + 4; x++) {
  for (int y = z->posy - 4; y <= z->posy + 4; y++) {
   if (g->is_empty(x, y) && g->m.sees(z->posx, z->posy, x, y, -1, junk)) {
    for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
     if (g->m.i_at(x, y)[i].type->id == itm_corpse &&
         g->m.i_at(x, y)[i].corpse->sym == 'Z')
      corpses.push_back(point(x, y));
    }
   }
  }
 }
 if (corpses.size() == 0)	// No nearby corpses
  return;
 z->speed = (z->speed - rng(0, 10)) * .8;
 bool sees_necromancer = (g->u_see(z, junk));
 if (sees_necromancer)
  g->add_msg("The %s throws its arms wide...", z->name().c_str());
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -500;			// It takes a while
 int raised = 0;
 for (int i = 0; i < corpses.size(); i++) {
  int x = corpses[i].x, y = corpses[i].y;
  for (int n = 0; n < g->m.i_at(x, y).size(); n++) {
   if (g->m.i_at(x, y)[n].type->id == itm_corpse && one_in(2)) {
    if (g->u_see(x, y, junk))
     raised++;
    monster mon(g->m.i_at(x, y)[n].corpse, x, y);
    mon.speed = int(mon.speed * .8);	// Raised corpses are slower
    mon.hp    = int(mon.hp    * .7);	// Raised corpses are weaker
    g->m.i_rem(x, y, n);
    n = g->m.i_at(x, y).size();	// Only one body raised per tile
    g->z.push_back(mon);
   }
  }
 }
 if (raised > 0) {
  if (raised == 1)
   g->add_msg("A nearby corpse rises from the dead!");
  else if (raised < 4)
   g->add_msg("A few corpses rise from the dead!");
  else
   g->add_msg("Several corpses rise from the dead!");
 } else if (sees_necromancer)
  g->add_msg("...but nothing seems to happen.");
}

void mattack::science(game *g, monster *z)	// I said SCIENCE again!
{
 int t, dist = rl_dist(z->posx, z->posy, g->u.posx, g->u.posy);
 if (dist > 5 || !g->sees_u(z->posx, z->posy, t))
  return;	// Out of range
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 std::vector<point> line = line_to(z->posx, z->posy, g->u.posx, g->u.posy, t);
 std::vector<point> free;
 for (int x = z->posx - 1; x <= z->posx + 1; x++) {
  for (int y = z->posy - 1; y <= z->posy + 1; y++) {
   if (g->is_empty(x, y))
    free.push_back(point(x, y));
  }
 }
 std::vector<int> valid;// List of available attacks
 int index;
 monster tmp(g->mtypes[mon_manhack]);
 if (dist == 1)
  valid.push_back(1);	// Shock
 if (dist <= 2)
  valid.push_back(2);	// Radiation
 if (!free.empty()) {
  valid.push_back(3);	// Manhack
  valid.push_back(4);	// Acid pool
 }
 valid.push_back(5);	// Flavor text
 switch (valid[rng(0, valid.size() - 1)]) {	// What kind of attack?
 case 1:	// Shock the player
  g->add_msg("The %s shocks you!", z->name().c_str());
  z->moves -= 150;
  g->u.hurtall(rng(1, 2));
  if (one_in(6) && !one_in(30 - g->u.str_cur)) {
   g->add_msg("You're paralyzed!");
   g->u.moves -= 300;
  }
  break;
 case 2:	// Radioactive beam
  g->add_msg("The %s opens it's mouth and a beam shoots towards you!",
             z->name().c_str());
  z->moves -= 400;
  if (g->u.dodge() > rng(1, 16))
   g->add_msg("You dodge the beam!");
  else if (one_in(6))
   g->u.mutate(g);
  else {
   g->add_msg("You get pins and needles all over.");
   g->u.radiation += rng(20, 50);
  }
  break;
 case 3:	// Spawn a manhack
  g->add_msg("The %s opens its coat, and a manhack flies out!",
             z->name().c_str());
  z->moves -= 200;
  index = rng(0, valid.size() - 1);
  tmp.spawn(free[index].x, free[index].y);
  g->z.push_back(tmp);
  break;
 case 4:	// Acid pool
  g->add_msg("The %s drops a flask of acid!", z->name().c_str());
  z->moves -= 100;
  for (int i = 0; i < free.size(); i++)
   g->m.add_field(g, free[i].x, free[i].y, fd_acid, 3);
  break;
 case 5:	// Flavor text
  switch (rng(1, 4)) {
  case 1:
   g->add_msg("The %s gesticulates wildly!", z->name().c_str());
   break;
  case 2:
   g->add_msg("The %s coughs up a strange dust.", z->name().c_str());
   break;
  case 3:
   g->add_msg("The %s moans softly.", z->name().c_str());
   break;
  case 4:
   g->add_msg("The %s's skin crackles with electricity.", z->name().c_str());
   z->moves -= 80;
   break;
  }
  break;
 }
}

void mattack::growplants(game *g, monster *z)
{
 int junk;
 for (int i = -3; i <= 3; i++) {
  for (int j = -3; j <= 3; j++) {
   if ((i != 0 || j != 0) && g->m.has_flag(diggable, z->posx + i, z->posy + j)){
    if (one_in(4)) {	// 1 in 4 chance to grow a tree
     int mondex = g->mon_at(z->posx + i, z->posy + j);
     if (mondex != -1) {
      if (g->u_see(z->posx + i, z->posy + j, junk))
       g->add_msg("A tree bursts forth from the earth and pierces the %s!",
                  g->z[mondex].name().c_str());
      int rn = rng(10, 30);
      rn -= g->z[mondex].armor();
      if (rn < 0)
       rn = 0;
      if (g->z[mondex].hurt(rn))
       g->kill_mon(mondex);
     } else if (g->u.posx == z->posx + i && g->u.posy == z->posy + j) {
// Player is hit by a growing tree
      body_part hit = bp_legs;
      int side = rng(1, 2);
      if (one_in(4))
       hit = bp_torso;
      else if (one_in(2))
       hit = bp_feet;
      g->add_msg("A tree bursts forth from the earth and pierces your %s!",
                 body_part_name(hit, side).c_str());
      g->u.hit(g, hit, side, 0, rng(10, 30));
     } else {
      int npcdex = g->npc_at(z->posx + i, z->posy + j);
      if (npcdex != -1) {	// An NPC got hit
       body_part hit = bp_legs;
       int side = rng(1, 2);
       if (one_in(4))
        hit = bp_torso;
       else if (one_in(2))
        hit = bp_feet;
       if (g->u_see(z->posx + i, z->posy + j, junk))
        g->add_msg("A tree bursts forth from the earth and pierces %s's %s!",
                   g->active_npc[npcdex].name.c_str(),
                   body_part_name(hit, side).c_str());
       g->active_npc[npcdex].hit(g, hit, side, 0, rng(10, 30));
      }
     }
     g->m.ter(z->posx + i, z->posy + j) = t_tree_young;
    } else if (one_in(3)) // If no tree, perhaps underbrush
     g->m.ter(z->posx + i, z->posy + j) = t_underbrush;
   } else if (one_in(3) && g->m.is_destructable(z->posx + i, z->posy + j))
    g->m.ter(z->posx + i, z->posy + j) = t_dirtmound; // Destroy walls, &c
  }
 }

 if (one_in(5)) { // 1 in 5 chance of making exisiting vegetation grow larger
  for (int i = -5; i <= 5; i++) {
   for (int j = -5; j <= 5; j++) {
    if (i != 0 || j != 0) {
     if (g->m.ter(z->posx + i, z->posy + j) == t_tree_young)
      g->m.ter(z->posx + i, z->posy + j) = t_tree; // Young tree => tree
     else if (g->m.ter(z->posx + i, z->posy + j) == t_underbrush) {
// Underbrush => young tree
      int mondex = g->mon_at(z->posx + i, z->posy + j);
      if (mondex != -1) {
       if (g->u_see(z->posx + i, z->posy + j, junk))
        g->add_msg("Underbrush forms into a tree, and it pierces the %s!",
                   g->z[mondex].name().c_str());
       int rn = rng(10, 30);
       rn -= g->z[mondex].armor();
       if (rn < 0)
        rn = 0;
       if (g->z[mondex].hurt(rn))
        g->kill_mon(mondex);
      } else if (g->u.posx == z->posx + i && g->u.posy == z->posy + j) {
       body_part hit = bp_legs;
       int side = rng(1, 2);
       if (one_in(4))
        hit = bp_torso;
       else if (one_in(2))
        hit = bp_feet;
       g->add_msg("The underbrush beneath your feet grows and pierces your %s!",
                  body_part_name(hit, side).c_str());
       g->u.hit(g, hit, side, 0, rng(10, 30));
      } else {
       int npcdex = g->npc_at(z->posx + i, z->posy + j);
       if (npcdex != -1) {
        body_part hit = bp_legs;
        int side = rng(1, 2);
        if (one_in(4))
         hit = bp_torso;
        else if (one_in(2))
         hit = bp_feet;
        if (g->u_see(z->posx + i, z->posy + j, junk))
         g->add_msg("Underbrush grows into a tree, and it pierces %s's %s!",
                    g->active_npc[npcdex].name.c_str(),
                    body_part_name(hit, side).c_str());
        g->active_npc[npcdex].hit(g, hit, side, 0, rng(10, 30));
       }
      }
     }
    }
   }
  }
 }
}

void mattack::fungus(game *g, monster *z)
{
// TODO: Infect NPCs?
 z->moves = -200;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 monster spore(g->mtypes[mon_spore]);
 int sporex, sporey;
 int moncount = 0, mondex;
 int j;
 g->sound(z->posx, z->posy, 10, "Pouf!");
 if (g->u_see(z->posx, z->posy, j))
  g->add_msg("Spores are released from the %s!", z->name().c_str());
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   if (i == 0 && j == 0)
    j++;	// No need to check 0, 0
   sporex = z->posx + i;
   sporey = z->posy + j;
   mondex = g->mon_at(sporex, sporey);
   if (g->m.move_cost(sporex, sporey) > 0 && one_in(5)) {
    if (mondex != -1) {	// Spores hit a monster
     if (g->u_see(sporex, sporey, j))
      g->add_msg("The %s is covered in tiny spores!",
                 g->z[mondex].name().c_str());
     if (!g->z[mondex].make_fungus(g))
      g->kill_mon(mondex);
    } else if (g->u.posx == sporex && g->u.posy == sporey)
     g->u.infect(DI_SPORES, bp_mouth, 4, 30, g); // Spores hit the player
    else { // Spawn a spore
     spore.spawn(sporex, sporey);
     g->z.push_back(spore);
    }
   }
  }
 }
 if (moncount >= 7)	// If we're surrounded by monsters, go dormant
  z->poly(g->mtypes[mon_fungaloid_dormant]);
}

void mattack::fungus_sprout(game *g, monster *z)
{
 for (int x = z->posx - 1; x <= z->posx + 1; x++) {
  for (int y = z->posy - 1; y <= z->posy + 1; y++) {
   if (g->u.posx == x && g->u.posy == y) {
    g->add_msg("You're shoved away as a fungal wall grows!");
    g->teleport();
   }
   if (g->is_empty(x, y)) {
    monster wall(g->mtypes[mon_fungal_wall]);
    wall.spawn(x, y);
    g->z.push_back(wall);
   }
  }
 }
}

void mattack::leap(game *g, monster *z)
{
 int linet;
 if (!g->sees_u(z->posx, z->posy, linet))
  return;	// Only leap if we can see you!

 std::vector<point> options;
 int best = 0;
 bool fleeing = z->is_fleeing(g->u);

 for (int x = z->posx - 3; x <= z->posx + 3; x++) {
  for (int y = z->posy - 3; y <= z->posy + 3; y++) {
/* If we're fleeing, we want to pick those tiles with the greatest distance
 * from the player; otherwise, those tiles with the least distance from the
 * player.
 */
   if (g->is_empty(x, y) &&
       g->m.sees(z->posx, z->posy, x, y, g->light_level(), linet) &&
       (( fleeing && rl_dist(g->u.posx, g->u.posy, x, y) >= best) ||
        (!fleeing && rl_dist(g->u.posx, g->u.posy, x, y) <= best)   )) {
    options.push_back( point(x, y) );
    best = rl_dist(g->u.posx, g->u.posy, x, y);
   }
  }
 }

// Go back and remove all options that aren't tied for best
 for (int i = 0; i < options.size() && options.size() > 1; i++) {
  point p = options[i];
  if (rl_dist(g->u.posx, g->u.posy, options[i].x, options[i].y) != best) {
   options.erase(options.begin() + i);
   i--;
  }
 }

 if (options.size() == 0)
  return; // Nowhere to leap!

 z->moves -= 150;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 point chosen = options[rng(0, options.size() - 1)];
 bool seen = g->u_see(z, linet); // We can see them jump...
 z->posx = chosen.x;
 z->posy = chosen.y;
 seen |= g->u_see(z, linet); // ... or we can see them land
 if (seen)
  g->add_msg("The %s leaps!", z->name().c_str());
}

void mattack::dermatik(game *g, monster *z)
{
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 1 ||
     g->u.has_disease(DI_DERMATIK))
  return; // Too far to implant, or the player's already incubating bugs

 z->sp_timeout = z->type->sp_freq;	// Reset timer

// Can we dodge the attack?
 int attack_roll = dice(z->type->melee_skill, 10);
 int player_dodge = g->u.dodge_roll();
 if (player_dodge > attack_roll) {
  g->add_msg("The %s tries to land on you, but you dodge.", z->name().c_str());
  z->stumble(g, false);
  return;
 }

// Can we swat the bug away?
 int dodge_roll = z->dodge_roll();
 int swat_skill = (g->u.sklevel[sk_melee] + g->u.sklevel[sk_unarmed] * 2) / 3;
 int player_swat = dice(swat_skill, 10);
 if (player_swat > dodge_roll) {
  g->add_msg("The %s lands on you, but you swat it off.", z->name().c_str());
  if (z->hp >= z->type->hp / 2)
   z->hurt(1);
  if (player_swat > dodge_roll * 1.5)
   z->stumble(g, false);
  return;
 }

// Can the bug penetrate our armor?
 body_part targeted = bp_head;
 if (!one_in(4))
  targeted = bp_torso;
 else if (one_in(2))
  targeted = bp_legs;
 else if (one_in(5))
  targeted = bp_hands;
 else if (one_in(5))
  targeted = bp_feet;
 if (g->u.armor_cut(targeted) >= 2) {
  g->add_msg("The %s lands on your %s, but can't penetrate your armor.",
             z->name().c_str(), body_part_name(targeted, rng(0, 1)).c_str());
  z->moves -= 150; // Attemped laying takes a while
  return;
 }

// Success!
 z->moves -= 500; // Successful laying takes a long time
 g->add_msg("The %s sinks its ovipositor into you!", z->name().c_str());
 g->u.add_disease(DI_DERMATIK, -1, g); // -1 = infinite
}

void mattack::plant(game *g, monster *z)
{
// Spores taking seed and growing into a fungaloid
 int j;
 if (g->m.has_flag(diggable, z->posx, z->posy)) {
  if (g->u_see(z->posx, z->posy, j))
   g->add_msg("The %s takes seed and becomes a young fungaloid!",
              z->name().c_str());
  z->poly(g->mtypes[mon_fungaloid_young]);
  z->moves = -1000;	// It takes a while
 }
}

void mattack::disappear(game *g, monster *z)
{
 z->hp = 0;
}

void mattack::formblob(game *g, monster *z)
{
 bool didit = false;
 int thatmon = -1;
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   thatmon = g->mon_at(z->posx + i, z->posy + j);
   if (g->u.posx == z->posx + i && g->u.posy == z->posy + i) {
// If we hit the player, cover them with slime
    didit = true;
    g->u.add_disease(DI_SLIMED, rng(0, z->hp), g);
   } else if (thatmon != -1) {
// Hit a monster.  If it's a blob, give it our speed.  Otherwise, blobify it?
    if (z->speed > 20 && g->z[thatmon].type->id == mon_blob &&
        g->z[thatmon].speed < 85) {
     didit = true;
     g->z[thatmon].speed += 5;
     z->speed -= 5;
    } else if (z->speed > 20 && g->z[thatmon].type->id == mon_blob_small) {
     didit = true;
     z->speed -= 5;
     g->z[thatmon].speed += 5;
     if (g->z[thatmon].speed >= 60)
      g->z[thatmon].poly(g->mtypes[mon_blob]);
    } else if ((g->z[thatmon].made_of(FLESH) || g->z[thatmon].made_of(VEGGY)) &&
               rng(0, z->hp) > rng(0, g->z[thatmon].hp)) {	// Blobify!
     didit = true;
     g->z[thatmon].poly(g->mtypes[mon_blob]);
     g->z[thatmon].speed = z->speed - rng(5, 25);
     g->z[thatmon].hp = g->z[thatmon].speed;
    }
   } else if (z->speed >= 85 && rng(0, 250) < z->speed) {
// If we're big enough, spawn a baby blob.
    didit = true;
    z->speed -= 15;
    monster blob(g->mtypes[mon_blob_small]);
    blob.spawn(z->posx + i, z->posy + j);
    blob.speed = z->speed - rng(30, 60);
    blob.hp = blob.speed;
    g->z.push_back(blob);
   }
  }
  if (didit) {	// We did SOMEthing.
   if (z->type->id == mon_blob && z->speed <= 50)	// We shrank!
    z->poly(g->mtypes[mon_blob]);
   z->moves = -500;
   z->sp_timeout = z->type->sp_freq;	// Reset timer
   return;
  }
 }
}

void mattack::dogthing(game *g, monster *z)
{
 int t;
 if (!one_in(3) || !g->u_see(z, t))
  return;

 g->add_msg("The %s's head explodes in a mass of roiling tentacles!",
            z->name().c_str());

 for (int x = z->posx - 2; x <= z->posx + 2; x++) {
  for (int y = z->posy - 2; y <= z->posy + 2; y++) {
   if (rng(0, 2) >= rl_dist(z->posx, z->posy, x, y))
    g->m.add_field(g, x, y, fd_blood, 2);
  }
 }

 z->friendly = 0;
 z->poly(g->mtypes[mon_headless_dog_thing]);
}

void mattack::tentacle(game *g, monster *z)
{
 int t;
 if (!g->sees_u(z->posx, z->posy, t))
  return;

 g->add_msg("The %s lashes its tentacle at you!", z->name().c_str());
 z->moves -= 100;
 z->sp_timeout = z->type->sp_freq;	// Reset timer

 std::vector<point> line = line_to(z->posx, z->posy, g->u.posx, g->u.posy, t);
 for (int i = 0; i < line.size(); i++) {
  int tmpdam = 20;
  g->m.shoot(g, line[i].x, line[i].y, tmpdam, true, 0);
 }

 if (rng(0, 20) > g->u.dodge() || one_in(g->u.dodge())) {
  g->add_msg("You dodge it!");
  return;
 }
 body_part hit = random_body_part();
 int dam = rng(10, 20), side = rng(0, 1);
 g->add_msg("Your %s is hit for %d damage!", body_part_name(hit, side).c_str(),
            dam);
 g->u.hit(g, hit, side, dam, 0);
}

void mattack::vortex(game *g, monster *z)
{
 int t;
// Moves are NOT used up by this attack, as it is "passive"
 z->sp_timeout = z->type->sp_freq;
// Before anything else, smash terrain!
 for (int x = z->posx - 2; x <= z->posx + 2; x++) {
  for (int y = z->posx - 2; y <= z->posy + 2; y++) {
   if (x == z->posx && y == z->posy) // Don't throw us!
    y++;
   std::string sound;
   g->m.bash(x, y, 14, sound);
   g->sound(x, y, 8, sound);
  }
 }
 for (int x = z->posx - 2; x <= z->posx + 2; x++) {
  for (int y = z->posx - 2; y <= z->posy + 2; y++) {
   if (x == z->posx && y == z->posy) // Don't throw us!
    y++;
   std::vector<point> from_monster = line_to(z->posx, z->posy, x, y, 0);
   while (!g->m.i_at(x, y).empty()) {
    item thrown = g->m.i_at(x, y)[0];
    g->m.i_rem(x, y, 0);
    int distance = 5 - (thrown.weight() / 15);
    if (distance > 0) {
     int dam = thrown.weight() / double(3 + double(thrown.volume() / 6));
     std::vector<point> traj = continue_line(from_monster, distance);
     for (int i = 0; i < traj.size() && dam > 0; i++) {
      g->m.shoot(g, traj[i].x, traj[i].y, dam, false, 0);
      int mondex = g->mon_at(traj[i].x, traj[i].y);
      if (mondex != -1) {
       if (g->z[mondex].hurt(dam))
        g->kill_mon(mondex);
       dam = 0;
      }
      if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
       dam = 0;
       i--;
      } else if (traj[i].x == g->u.posx && traj[i].y == g->u.posy) {
       body_part hit = random_body_part();
       int side = rng(0, 1);
       g->add_msg("A %s hits your %s for %d damage!", thrown.tname().c_str(),
                  body_part_name(hit, side).c_str(), dam);
       g->u.hit(g, hit, side, dam, 0);
       dam = 0;
      }
// TODO: Hit NPCs
      if (dam == 0 || i == traj.size() - 1) {
       if (thrown.made_of(GLASS)) {
        if (g->u_see(traj[i].x, traj[i].y, t))
         g->add_msg("The %s shatters!", thrown.tname().c_str());
        for (int n = 0; n < thrown.contents.size(); n++)
         g->m.add_item(traj[i].x, traj[i].y, thrown.contents[n]);
        g->sound(traj[i].x, traj[i].y, 16, "glass breaking!");
       } else
        g->m.add_item(traj[i].x, traj[i].y, thrown);
      }
     }
    } // Done throwing item
   } // Done getting items
// Throw monsters
   int mondex = g->mon_at(x, y);
   if (mondex != -1) {
    int distance = 0, damage = 0;
    monster *thrown = &(g->z[mondex]);
    switch (thrown->type->size) {
     case MS_TINY:   distance = 5; break;
     case MS_SMALL:  distance = 3; break;
     case MS_MEDIUM: distance = 2; break;
     case MS_LARGE:  distance = 1; break;
     case MS_HUGE:   distance = 0; break;
    }
    damage = distance * 4;
    switch (thrown->type->mat) {
     case LIQUID:  distance += 3; damage -= 10; break;
     case VEGGY:   distance += 1; damage -=  5; break;
     case POWDER:  distance += 4; damage -= 30; break;
     case COTTON:
     case WOOL:    distance += 5; damage -= 40; break;
     case LEATHER: distance -= 1; damage +=  5; break;
     case KEVLAR:  distance -= 3; damage -= 20; break;
     case STONE:   distance -= 3; damage +=  5; break;
     case PAPER:   distance += 6; damage -= 10; break;
     case WOOD:    distance += 1; damage +=  5; break;
     case PLASTIC: distance += 1; damage +=  5; break;
     case GLASS:   distance += 2; damage += 20; break;
     case IRON:    distance -= 1; // fall through
     case STEEL:
     case SILVER:  distance -= 3; damage -= 10; break;
    }
    if (distance > 0) {
     if (g->u_see(thrown, t))
      g->add_msg("The %s is thrown by winds!", thrown->name().c_str());
     std::vector<point> traj = continue_line(from_monster, distance);
     bool hit_wall = false;
     for (int i = 0; i < traj.size() && !hit_wall; i++) {
      int monhit = g->mon_at(traj[i].x, traj[i].y);
      if (i > 0 && monhit != -1 && !g->z[monhit].has_flag(MF_DIGS)) {
       if (g->u_see(traj[i].x, traj[i].y, t))
        g->add_msg("The %s hits a %s!", thrown->name().c_str(),
                   g->z[monhit].name().c_str());
       if (g->z[monhit].hurt(damage))
        g->kill_mon(monhit);
       hit_wall = true;
       thrown->posx = traj[i - 1].x;
       thrown->posy = traj[i - 1].y;
      } else if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
       hit_wall = true;
       thrown->posx = traj[i - 1].x;
       thrown->posy = traj[i - 1].y;
      }
      int damage_copy = damage;
      g->m.shoot(g, traj[i].x, traj[i].y, damage_copy, false, 0);
      if (damage_copy < damage)
       thrown->hurt(damage - damage_copy);
     }
     if (hit_wall)
      damage *= 2;
     else {
      thrown->posx = traj[traj.size() - 1].x;
      thrown->posy = traj[traj.size() - 1].y;
     }
     if (thrown->hurt(damage))
      g->kill_mon(g->mon_at(thrown->posx, thrown->posy));
    } // if (distance > 0)
   } // if (mondex != -1)

   if (g->u.posx == x && g->u.posy == y) { // Throw... the player?! D:
    std::vector<point> traj = continue_line(from_monster, rng(2, 3));
    g->add_msg("You're thrown by winds!");
    bool hit_wall = false;
    int damage = rng(5, 10);
    for (int i = 0; i < traj.size() && !hit_wall; i++) {
     int monhit = g->mon_at(traj[i].x, traj[i].y);
     if (i > 0 && monhit != -1 && !g->z[monhit].has_flag(MF_DIGS)) {
      if (g->u_see(traj[i].x, traj[i].y, t))
       g->add_msg("You hit a %s!", g->z[monhit].name().c_str());
      if (g->z[monhit].hurt(damage))
       g->kill_mon(monhit);
      hit_wall = true;
      g->u.posx = traj[i - 1].x;
      g->u.posy = traj[i - 1].y;
     } else if (g->m.move_cost(traj[i].x, traj[i].y) == 0) {
      g->add_msg("You slam into a %s",
                 g->m.tername(traj[i].x, traj[i].y).c_str());
      hit_wall = true;
      g->u.posx = traj[i - 1].x;
      g->u.posy = traj[i - 1].y;
     }
     int damage_copy = damage;
     g->m.shoot(g, traj[i].x, traj[i].y, damage_copy, false, 0);
     if (damage_copy < damage)
      g->u.hit(g, bp_torso, 0, damage - damage_copy, 0);
    }
    if (hit_wall)
     damage *= 2;
    else {
     g->u.posx = traj[traj.size() - 1].x;
     g->u.posy = traj[traj.size() - 1].y;
    }
    g->u.hit(g, bp_torso, 0, damage, 0);
    g->update_map(g->u.posx, g->u.posy);
   } // Done with checking for player
  }
 } // Done with loop!
}

void mattack::gene_sting(game *g, monster *z)
{
 int j;
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 7 ||
     !g->sees_u(z->posx, z->posy, j))
  return;	// Not within range and/or sight

 z->moves -= 150;
 z->sp_timeout = z->type->sp_freq;
 g->add_msg("The %s shoots a dart into you!", z->name().c_str());
 g->u.mutate(g);
}

void mattack::stare(game *g, monster *z)
{
 z->moves -= 200;
 z->sp_timeout = z->type->sp_freq;
 int j;
 if (g->sees_u(z->posx, z->posy, j)) {
  g->add_msg("The %s stares at you, and you shudder.", z->name().c_str());
  g->u.add_disease(DI_TELEGLOW, 800, g);
 } else {
  g->add_msg("A piercing beam of light bursts forth!");
  std::vector<point> sight = line_to(z->posx, z->posy, g->u.posx, g->u.posy, 0);
  for (int i = 0; i < sight.size(); i++) {
   if (g->m.ter(sight[i].x, sight[i].y) == t_reinforced_glass_h ||
       g->m.ter(sight[i].x, sight[i].y) == t_reinforced_glass_v)
    i = sight.size();
   else if (g->m.is_destructable(sight[i].x, sight[i].y))
    g->m.ter(sight[i].x, sight[i].y) = t_rubble;
  }
 }
}

void mattack::fear_paralyze(game *g, monster *z)
{
 int t;
 if (g->u_see(z->posx, z->posy, t)) {
  z->sp_timeout = z->type->sp_freq;	// Reset timer
  if (rng(1, 20) > g->u.int_cur) {
   g->add_msg("The terrifying visage of the %s paralyzes you.",
              z->name().c_str());
   g->u.moves -= 100;
  } else
   g->add_msg("You manage to avoid staring at the horrdenous %s.",
              z->name().c_str());
 }
}

void mattack::photograph(game *g, monster *z)
{
 int t;
 if (z->faction_id == -1 ||
     rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 6 ||
     !g->sees_u(z->posx, z->posy, t))
  return;
 z->sp_timeout = z->type->sp_freq;
 z->moves -= 150;
 g->add_msg("The %s takes your picture!", z->name().c_str());
// TODO: Make the player known to the faction
 g->add_event(EVENT_ROBOT_ATTACK, int(g->turn) + rng(15, 30), z->faction_id,
              g->levx, g->levy);
}

void mattack::tazer(game *g, monster *z)
{
 int j;
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 2 ||
     !g->sees_u(z->posx, z->posy, j))
  return;	// Out of range
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -200;			// It takes a while
 g->add_msg("The %s shocks you!", z->name().c_str());
 int shock = rng(1, 5);
 g->u.hurt(g, bp_torso, 0, shock * rng(1, 3));
 g->u.moves -= shock * 20;
}

void mattack::smg(game *g, monster *z)
{
 int t, j, fire_t;
 if (z->friendly != 0) { // Attacking monsters, not the player!
  monster* target = NULL;
  int closest = 13;
  for (int i = 0; i < g->z.size(); i++) {
   int dist = rl_dist(z->posx, z->posy, g->z[i].posx, g->z[i].posy);
   if (g->z[i].friendly == 0 && dist < closest && 
       g->m.sees(z->posx, z->posy, g->z[i].posx, g->z[i].posy, 12, t)) {
    target = &(g->z[i]);
    closest = dist;
    fire_t = t;
   }
  }
  if (target == NULL) // Couldn't find any targets!
   return;
  z->sp_timeout = z->type->sp_freq;	// Reset timer
  z->moves = -150;			// It takes a while
  if (g->u_see(z->posx, z->posy, t))
   g->add_msg("The %s fires its smg!", z->name().c_str());
  player tmp;
  tmp.name = "The " + z->name();
  tmp.sklevel[sk_smg] = 1;
  tmp.sklevel[sk_gun] = 0;
  tmp.recoil = 0;
  tmp.posx = z->posx;
  tmp.posy = z->posy;
  tmp.str_cur = 16;
  tmp.dex_cur =  6;
  tmp.per_cur =  8;
  tmp.weapon = item(g->itypes[itm_smg_9mm], 0);
  tmp.weapon.curammo = dynamic_cast<it_ammo*>(g->itypes[itm_9mm]);
  tmp.weapon.charges = 10;
  std::vector<point> traj = line_to(z->posx, z->posy,
                                    target->posx, target->posy, fire_t);
  g->fire(tmp, target->posx, target->posy, traj, true);

  return;
 }
 
// Not friendly; hence, firing at the player
 if (trig_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 12 ||
     !g->sees_u(z->posx, z->posy, t))
  return;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -150;			// It takes a while

 if (g->u_see(z->posx, z->posy, j))
  g->add_msg("The %s fires its smg!", z->name().c_str());
// Set up a temporary player to fire this gun
 player tmp;
 tmp.name = "The " + z->name();
 tmp.sklevel[sk_smg] = 1;
 tmp.sklevel[sk_gun] = 0;
 tmp.recoil = 0;
 tmp.posx = z->posx;
 tmp.posy = z->posy;
 tmp.str_cur = 16;
 tmp.dex_cur =  6;
 tmp.per_cur =  8;
 tmp.weapon = item(g->itypes[itm_smg_9mm], 0);
 tmp.weapon.curammo = dynamic_cast<it_ammo*>(g->itypes[itm_9mm]);
 tmp.weapon.charges = 10;
 std::vector<point> traj = line_to(z->posx, z->posy, g->u.posx, g->u.posy, t);
 g->fire(tmp, g->u.posx, g->u.posy, traj, true);
}

void mattack::flamethrower(game *g, monster *z)
{
 int t;
 if (abs(g->u.posx - z->posx) > 5 || abs(g->u.posy - z->posy) > 5 ||
     !g->sees_u(z->posx, z->posy, t))
  return;	// Out of range
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -500;			// It takes a while
 std::vector<point> traj = line_to(z->posx, z->posy, g->u.posx, g->u.posy, t);
 for (int i = 0; i < traj.size(); i++)
  g->m.add_field(g, traj[i].x, traj[i].y, fd_fire, 1);
 g->u.add_disease(DI_ONFIRE, 8, g);
}

void mattack::copbot(game *g, monster *z)
{
 int t, mode = 0;
 bool sees_u = g->sees_u(z->posx, z->posy, t);
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 2 || !sees_u) {
  if (one_in(3)) {
   if (sees_u)
    g->sound(z->posx, z->posy, 18, "a robotic voice boom, \"Citizen, Halt!\"");
   else
    g->sound(z->posx, z->posy, 18,
             "a robotic voice boom, \"Come out with your hands up!\"");
  } else
   g->sound(z->posx, z->posy, 18, "a police siren, whoop WHOOP");
  return;
 }
 mattack tmp;
 tmp.tazer(g, z);
}

void mattack::multi_robot(game *g, monster *z)
{
 int t, mode = 0;
 if (!g->sees_u(z->posx, z->posy, t))
  return;	// Can't see you!
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) == 1 && one_in(2))
  mode = 1;
 else if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) <= 5)
  mode = 2;
 else if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) <= 12)
  mode = 3;

 if (mode == 0)
  return;	// No attacks were valid!

 mattack tmp;
 switch (mode) {
  case 1: tmp.tazer(g, z);        break;
  case 2: tmp.flamethrower(g, z); break;
  case 3: tmp.smg(g, z);          break;
 }
}
