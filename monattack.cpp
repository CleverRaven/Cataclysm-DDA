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
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 12 ||
     !g->sees_u(z->posx, z->posy, junk))
  return;	// Out of range
 z->moves = -300;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 g->sound(z->posx, z->posy, 4, "a spitting noise.");
 int hitx = g->u.posx + rng(-2, 2), hity = g->u.posy + rng(-2, 2);
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
 if (!g->sees_u(z->posx, z->posy, t))
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
 std::vector<point> corpses;
 int junk;
// Find all corposes that we can see within 4 tiles.
 for (int x = z->posx - 4; x <= z->posx + 4; x++) {
  for (int y = z->posy - 4; y <= z->posy + 4; y++) {
   if (g->is_empty(x, y) && g->m.sees(z->posx, z->posy, x, y, -1, junk)) {
    for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
     if (g->m.i_at(x, y)[i].type->id == itm_corpse)
      corpses.push_back(point(x, y));
    }
   }
  }
 }
 if (corpses.size() == 0)	// No nearby corpses
  return;
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

void mattack::leap(game *g, monster *z)
{
 int linet;
 if (!g->sees_u(z->posx, z->posy, linet))
  return;	// Only leap if we can see you!

 std::vector<point> options;
 int dx = 0, dy = 0, best = 0;
 if (g->u.posx > z->posx)
  dx = 1;
 else if (g->u.posx < z->posx)
  dx = -1;
 if (g->u.posy > z->posy)
  dy = 1;
 else if (g->u.posy < z->posy)
  dy = -1;

 if (z->is_fleeing(g->u)) { // Leaping away from the player
  dx *= -1;
  dy *= -1;
 }

 for (int x = z->posx + dx * 3; x != z->posx - dx * 4; x -= dx) {
  for (int y = z->posy + dy * 3; y != z->posy - dy * 4; y -= dy) {
   if (g->is_empty(x, y) && rl_dist(z->posx, z->posy, x, y) >= best &&
       g->m.sees(z->posx, z->posy, x, y, g->light_level(), linet)       ) {
    options.push_back( point(x, y) );
    best = rl_dist(z->posx, z->posy, x, y);
   }
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

void mattack::plant(game *g, monster *z)
{
// Spores taking seed and growing into a fungaloid
 int j;
 if (g->m.has_flag(diggable, z->posx, z->posy)) {
  if (g->u_see(z->posx, z->posy, j))
   g->add_msg("The %s takes seed and becomes a fungaloid!", z->name().c_str());
  z->poly(g->mtypes[mon_fungaloid]);
  z->moves = -1000;			// It takes a while
  z->sp_timeout = z->type->sp_freq;	// Reset timer
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
  g->add_msg("The %s stares at you...", z->name().c_str());
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

void mattack::tazer(game *g, monster *z)
{
 int j;
 if (abs(g->u.posx - z->posx) > 2 || abs(g->u.posy - z->posy) > 2 ||
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
 int t, j;
 if (trig_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 12 ||
     !g->sees_u(z->posx, z->posy, t))
  return;
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -150;			// It takes a while

 if (g->u_see(z->posx, z->posy, j))
  g->add_msg("The %s fires its smg!", z->name().c_str());
 player tmp;
 tmp.name = "The " + z->name();
 tmp.sklevel[sk_smg] = 1;
 tmp.sklevel[sk_gun] = 0;
 tmp.recoil = 0;
 tmp.posx = z->posx;
 tmp.posy = z->posy;
 tmp.str_cur = 16;
 tmp.dex_cur =  5;
 tmp.per_cur =  7;
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
