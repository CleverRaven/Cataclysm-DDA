#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include "bodypart.h"

void mattack::antqueen(game *g, monster *z)
{
 std::vector<point> egg_points;
 z->moves = -200;			// It takes a while
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 for (int x = z->posx - 1; x <= z->posx + 1; x++) {
  for (int y = z->posy - 1; y <= z->posy + 1; y++) {
   for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
    if (g->m.i_at(x, y)[i].type->id == itm_ant_egg)
     egg_points.push_back(point(x, y));
   }
  }
 }
 if (egg_points.size() == 0) {
  int junk;
  if (g->u_see(z->posx, z->posy, junk))
   g->add_msg("The %s lays an egg!", z->name().c_str());
  g->m.add_item(z->posx, z->posy, g->itypes[itm_ant_egg], g->turn);
 } else {
  int junk;
  if (g->u_see(z->posx, z->posy, junk))
   g->add_msg("The %s tends nearby eggs, and they hatch!", z->name().c_str());
  for (int i = 0; i < egg_points.size(); i++) {
   int x = egg_points[i].x, y = egg_points[i].y;
   for (int j = 0; j < g->m.i_at(x, y).size(); j++) {
    if (g->m.i_at(x, y)[j].type->id == itm_ant_egg) {
     g->m.i_rem(x, y, j);
     j = g->m.i_at(x, y).size();
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
 g->sound(z->posx, z->posy, 100, "a terrible shriek!");
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
       (one_in(abs(j)) && one_in(abs(i)) || (i == 0 && j == 0))) {
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
 int tarx = g->u.posx + rng(-3, 3), tary = g->u.posy + rng(-3, 3);
 if (!g->m.sees(z->posx, z->posy, tarx, tary, -1, t))
  t = 0;
 std::vector<point> bolt = line_to(z->posx, z->posy, tarx, tary, t);
 for (int i = 0; i < bolt.size(); i++) {
  if (!one_in(4))
   g->m.add_field(g, bolt[i].x, bolt[i].y, fd_electricity, rng(1, 3));
 }
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
 if (abs(g->u.posx - z->posx) > 3 || abs(g->u.posy - z->posy) > 3 ||
     !g->sees_u(z->posx, z->posy, j))
  return;	// Out of range
 std::vector<point> line = line_to(z->posx, z->posy, g->u.posx, g->u.posy, j);
 z->sp_timeout = z->type->sp_freq;	// Reset timer
 z->moves = -250;			// It takes a while
 bool u_see = g->u_see(z->posx, z->posy, j);
 for (int i = 0; i < line.size(); i++) {
  if (g->m.field_at(line[i].x, line[i].y).type == fd_blood) {
   g->m.field_at(line[i].x, line[i].y).type = fd_bile;
   g->m.field_at(line[i].x, line[i].y).density = 1;
  } else if (g->m.field_at(line[i].x, line[i].y).type == fd_bile &&
             g->m.field_at(line[i].x, line[i].y).density < 3)
   g->m.field_at(line[i].x, line[i].y).density++;
  else
   g->m.add_field(g, line[i].x, line[i].y, fd_bile, 1);
 }
 if (u_see)
  g->add_msg("The %s spews bile!", z->name().c_str());
 if (rng(0, 10) > g->u.dodge() || one_in(g->u.dodge()))
  g->u.infect(DI_BOOMERED, bp_eyes, 3, 12, g);
 else if (u_see)
  g->add_msg("You dodge it!");
}

void mattack::resurrect(game *g, monster *z)
{
 std::vector<point> corpses;
 for (int x = z->posx - 6; x <= z->posx + 6; x++) {
  for (int y = z->posy - 6; y <= z->posy + 6; y++) {
   if (g->mon_at(x, y) == -1) {
    for (int i = 0; i < g->m.i_at(x, y).size(); i++) {
     if (g->m.i_at(x, y)[i].type->id == itm_corpse)
      corpses.push_back(point(x, y));
    }
   }
  }
 }
 if (corpses.size() == 0)	// No nearby corpses
  return;
 int junk;
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
    g->m.i_rem(x, y, n);
    n = g->m.i_at(x, y).size();
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

void mattack::growplants(game *g, monster *z)
{
 int junk;
 for (int i = -3; i <= 3; i++) {
  for (int j = -3; j <= 3; j++) {
   if ((i != 0 || j != 0) && g->m.has_flag(diggable, z->posx + i, z->posy + j)){
    if (one_in(4)) {
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
      if (npcdex != -1) {
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
      } else
       g->m.ter(z->posx + i, z->posy + j) = t_tree_young;
     }
    } else if (one_in(3))
     g->m.ter(z->posx + i, z->posy + j) = t_underbrush;
   } else if (one_in(3) && g->m.is_destructable(z->posx + i, z->posy + j))
    g->m.ter(z->posx + i, z->posy + j) = t_dirtmound;
  }
 }

 if (one_in(5)) {
  for (int i = -5; i <= 5; i++) {
   for (int j = -5; j <= 5; j++) {
    if (i != 0 || j != 0) {
     if (g->m.ter(z->posx + i, z->posy + j) == t_tree_young)
      g->m.ter(z->posx + i, z->posy + j) = t_tree;
     else if (g->m.ter(z->posx + i, z->posy + j) == t_underbrush) {
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
   if (i == 0 && j == 0) j++;	// No need to check 0, 0
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
     g->u.infect(DI_SPORES, bp_mouth, 4, 30, g);
    else {
     spore.spawn(sporex, sporey);
     g->z.push_back(spore);
    }
   }
  }
 }
 if (moncount >= 7)
  z->poly(g->mtypes[mon_fungaloid_dormant]);
}

void mattack::plant(game *g, monster *z)
{
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
 bool didit = false;;
 int thatmon = -1;
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   thatmon = g->mon_at(z->posx + i, z->posy + j);
   if (g->u.posx == z->posx + i && g->u.posy == z->posy + i) {
    didit = true;
    g->u.add_disease(DI_SLIMED, rng(0, z->hp), g);
   } else if (thatmon != -1) {
    didit = true;
    if (g->z[thatmon].type->id == mon_blob) {
     if (g->z[thatmon].speed < 85)
      g->z[thatmon].speed += 5;
    } else if (g->z[thatmon].type->id == mon_blob_small) {
     g->z[thatmon].speed += 5;
     if (g->z[thatmon].speed >= 60)
      g->z[thatmon].poly(g->mtypes[mon_blob]);
    } else if ((g->z[thatmon].made_of(FLESH) || g->z[thatmon].made_of(VEGGY)) &&
               rng(0, z->hp) > rng(0, g->z[thatmon].hp)) {
     g->z[thatmon].poly(g->mtypes[mon_blob]);
     g->z[thatmon].speed = z->speed - rng(5, 25);
     g->z[thatmon].hp = g->z[thatmon].speed;
    }
   } else if (z->speed >= 85 && rng(0, 250) < z->speed) {
    didit = true;
    z->speed -= 15;
    monster blob(g->mtypes[mon_blob_small]);
    blob.spawn(z->posx + i, z->posy + j);
    blob.speed = z->speed - rng(30, 60);
    blob.hp = blob.speed;
    g->z.push_back(blob);
   }
  }
  if (didit) {
   z->moves = -500;
   z->sp_timeout = z->type->sp_freq;	// Reset timer
   return;
  }
 }
}

void mattack::gene_sting(game *g, monster *z)
{
 int j;
 if (abs(g->u.posx - z->posx) > 7 || abs(g->u.posy - z->posy) > 7 ||
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
 g->u.moves -= shock * 50;
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
 tmp.sklevel[sk_smg] = 2;
 tmp.sklevel[sk_gun] = 2;
 tmp.recoil = 0;
 tmp.posx = z->posx;
 tmp.posy = z->posy;
 tmp.str_cur = 16;
 tmp.dex_cur = 12;
 tmp.per_cur = 10;
 tmp.weapon = item(g->itypes[itm_hk_mp7], 0);
 tmp.weapon.curammo = dynamic_cast<it_ammo*>(g->itypes[itm_46mm]);
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
 if (abs(g->u.posx - z->posx) <= 2 && abs(g->u.posy - z->posy) <= 2 &&one_in(2))
  mode = 1;
 else if (abs(g->u.posx - z->posx) <= 5 && abs(g->u.posy - z->posy) <= 5)
  mode = 2;
 else if (abs(g->u.posx - z->posx) <= 12 && abs(g->u.posy - z->posy) <= 12)
  mode = 3;

 if (mode == 0)
  return;	// No attacks were valid!

 mattack tmp;
 switch (mode) {
  case 1: tmp.tazer(g, z); break;
  case 2: tmp.flamethrower(g, z); break;
  case 3: tmp.smg(g, z); break;
 }
}
