#include "mondeath.h"
#include "monster.h"
#include "game.h"
#include "rng.h"
#include "line.h"
#include <sstream>

void mdeath::normal(game *g, monster *z)
{
 int junk;
 if (g->u_see(z, junk))
  g->add_msg("It dies!");
 if (z->made_of(FLESH) && z->has_flag(MF_WARM)) {
  if (g->m.field_at(z->posx, z->posy).type == fd_blood &&
      g->m.field_at(z->posx, z->posy).density < 3)
   g->m.field_at(z->posx, z->posy).density++;
  else
   g->m.add_field(g, z->posx, z->posy, fd_blood, 1);
 }
// Drop a dang ol' corpse
// If their hp is less than -50, we destroyed them so badly no corpse was left
 if ((z->hp >= -50 || z->hp >= 0 - 2 * z->type->hp) &&
     (z->made_of(FLESH) || z->made_of(VEGGY))) {
  item tmp;
  tmp.make_corpse(g->itypes[itm_corpse], z->type, g->turn);
  g->m.add_item(z->posx, z->posy, tmp);
 }
}

void mdeath::acid(game *g, monster *z)
{
 int tmp;
 if (g->u_see(z, tmp))
  g->add_msg("The %s's corpse melts into a pool of acid.", z->name().c_str());
 g->m.add_field(g, z->posx, z->posy, fd_acid, 3);
}

void mdeath::boomer(game *g, monster *z)
{
 std::string tmp;
 g->sound(z->posx, z->posy, 24, "a boomer explode!");
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   g->m.bash(z->posx + i, z->posy + j, 10, tmp);
   if (g->m.field_at(z->posx + i, z->posy + j).type == fd_bile &&
       g->m.field_at(z->posx + i, z->posy + j).density < 3)
    g->m.field_at(z->posx + i, z->posy + j).density++;
   else
    g->m.add_field(g, z->posx + i, z->posy + j, fd_bile, 1);
   int mondex = g->mon_at(z->posx + i, z->posy +j);
   if (mondex != -1) {
    g->z[mondex].stumble(g, false);
    g->z[mondex].moves -= 250;
   }
  }
 }
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) == 1)
  g->u.infect(DI_BOOMERED, bp_eyes, 2, 24, g);
}

void mdeath::kill_vines(game *g, monster *z)
{
 std::vector<int> vines;
 std::vector<int> hubs;
 for (int i = 0; i < g->z.size(); i++) {
  if (g->z[i].type->id == mon_creeper_hub &&
      (g->z[i].posx != z->posx || g->z[i].posy != z->posy))
   hubs.push_back(i);
  if (g->z[i].type->id == mon_creeper_vine)
   vines.push_back(i);
 }

 for (int i = 0; i < vines.size(); i++) {
  monster *vine = &(g->z[ vines[i] ]);
  int dist = rl_dist(vine->posx, vine->posy, z->posx, z->posy);
  bool closer_hub = false;
  for (int j = 0; j < hubs.size() && !closer_hub; j++) {
   if (rl_dist(vine->posx, vine->posy,
               g->z[ hubs[j] ].posx, g->z[ hubs[j] ].posy) < dist)
    closer_hub = true;
  }
  if (!closer_hub)
   vine->hp = 0;
 }
}

void mdeath::vine_cut(game *g, monster *z)
{
 std::vector<int> vines;
 for (int x = z->posx - 1; x <= z->posx + 1; x++) {
  for (int y = z->posy - 1; y <= z->posy + 1; y++) {
   if (x == z->posx && y == z->posy)
    y++; // Skip ourselves
   int mondex = g->mon_at(x, y);
   if (mondex != -1 && g->z[mondex].type->id == mon_creeper_vine)
    vines.push_back(mondex);
  }
 }

 for (int i = 0; i < vines.size(); i++) {
  bool found_neighbor = false;
  monster *vine = &(g->z[ vines[i] ]);
  for (int x = vine->posx - 1; x <= vine->posx + 1 && !found_neighbor; x++) {
   for (int y = vine->posy - 1; y <= vine->posy + 1 && !found_neighbor; y++) {
    if (x != z->posx || y != z->posy) { // Not the dying vine
     int mondex = g->mon_at(x, y);
     if (mondex != -1 && (g->z[mondex].type->id == mon_creeper_hub ||
                          g->z[mondex].type->id == mon_creeper_vine  ))
      found_neighbor = true;
    }
   }
  }
  if (!found_neighbor)
   vine->hp = 0;
 }
}

void mdeath::triffid_heart(game *g, monster *z)
{
 g->add_msg("The root walls begin to crumble around you.");
 g->add_event(EVENT_ROOTS_DIE, int(g->turn) + 100);
}

void mdeath::fungus(game *g, monster *z)
{
 monster spore(g->mtypes[mon_spore]);
 int sporex, sporey;
 g->sound(z->posx, z->posy, 10, "Pouf!");
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   sporex = z->posx + i;
   sporey = z->posy + j;
   if (g->m.move_cost(sporex, sporey) > 0 && one_in(5)) {
    if (g->mon_at(sporex, sporey) >= 0) {	// Spores hit a monster
     if (g->u_see(sporex, sporey, j))
      g->add_msg("The %s is covered in tiny spores!",
                 g->z[g->mon_at(sporex, sporey)].name().c_str());
     if (!g->z[g->mon_at(sporex, sporey)].make_fungus(g))
      g->kill_mon(g->mon_at(sporex, sporey), (z->friendly != 0));
    } else if (g->u.posx == sporex && g->u.posy == sporey)
     g->u.infect(DI_SPORES, bp_mouth, 4, 30, g);
    else {
     spore.spawn(sporex, sporey);
     g->z.push_back(spore);
    }
   }
  }
 }
}

void mdeath::fungusawake(game *g, monster *z)
{
 monster newfung(g->mtypes[mon_fungaloid]);
 newfung.spawn(z->posx, z->posy);
 g->z.push_back(newfung);
}

void mdeath::disintegrate(game *g, monster *z)
{
 int junk;
 if (g->u_see(z, junk))
  g->add_msg("It disintegrates!");
}

void mdeath::worm(game *g, monster *z)
{
 int j;
 if (g->u_see(z, j))
  g->add_msg("The %s splits in two!", z->name().c_str());

 std::vector <point> wormspots;
 int wormx, wormy;
 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   wormx = z->posx + i;
   wormy = z->posy + i;
   if (g->m.has_flag(diggable, wormx, wormy) && g->mon_at(wormx, wormy) == -1 &&
       !(g->u.posx == wormx && g->u.posy == wormy)) {
    wormspots.push_back(point(wormx, wormy));
   }
  }
 }
 int rn;
 monster worm(g->mtypes[mon_halfworm]);
 for (int worms = 0; worms < 2 && wormspots.size() > 0; worms++) {
  rn = rng(0, wormspots.size() - 1);
  worm.spawn(wormspots[rn].x, wormspots[rn].y);
  g->z.push_back(worm);
  wormspots.erase(wormspots.begin() + rn);
 }
}

void mdeath::disappear(game *g, monster *z)
{
 g->add_msg("The %s disappears!  Was it in your head?", z->name().c_str());
}

void mdeath::guilt(game *g, monster *z)
{
 if (g->u.has_trait(PF_HEARTLESS))
  return;	// We don't give a shit!
 if (rl_dist(z->posx, z->posy, g->u.posx, g->u.posy) > 1)
  return;	// Too far away, we can deal with it
 if (z->hp >= 0)
  return;	// It probably didn't die from damage
 g->add_msg("You feel terrible for killing %s!", z->name().c_str());
 g->u.add_morale(MORALE_KILLED_MONSTER, -50, -250);
}

void mdeath::blobsplit(game *g, monster *z)
{
 int j;
 int speed = z->speed - rng(30, 50);
 if (speed <= 0) {
  if (g->u_see(z, j))
   g->add_msg("The %s splatters into tiny, dead pieces.", z->name().c_str());
  return;
 }
 monster blob(g->mtypes[(speed < 50 ? mon_blob_small : mon_blob)]);
 blob.speed = speed;
 blob.friendly = z->friendly; // If we're tame, our kids are too
 if (g->u_see(z, j))
  g->add_msg("The %s splits!", z->name().c_str());
 blob.hp = blob.speed;
 std::vector <point> valid;

 for (int i = -1; i <= 1; i++) {
  for (int j = -1; j <= 1; j++) {
   if (g->m.move_cost(z->posx+i, z->posy+j) > 0 &&
       g->mon_at(z->posx+i, z->posy+j) == -1 &&
       (g->u.posx != z->posx+i || g->u.posy != z->posy + j))
    valid.push_back(point(z->posx+i, z->posy+j));
  }
 }
 
 int rn;
 for (int s = 0; s < 2 && valid.size() > 0; s++) {
  rn = rng(0, valid.size() - 1);
  blob.spawn(valid[rn].x, valid[rn].y);
  g->z.push_back(blob);
  valid.erase(valid.begin() + rn);
 }
}

void mdeath::melt(game *g, monster *z)
{
 int j;
 if (g->u_see(z, j))
  g->add_msg("The %s melts away!", z->name().c_str());
}

void mdeath::amigara(game *g, monster *z)
{
 if (g->u.has_disease(DI_AMIGARA)) {
  int count = 0;
  for (int i = 0; i < g->z.size(); i++) {
   if (g->z[i].type->id == mon_amigara_horror)
    count++;
  }
  if (count <= 1) { // We're the last!
   g->u.rem_disease(DI_AMIGARA);
   g->add_msg("Your obsession with the fault fades away...");
   item art(g->new_artifact(), g->turn);
   g->m.add_item(z->posx, z->posy, art);
  }
 }
 normal(g, z);
}

void mdeath::thing(game *g, monster *z)
{
 monster thing(g->mtypes[mon_thing]);
 thing.spawn(z->posx, z->posy);
 g->z.push_back(thing);
}

void mdeath::explode(game *g, monster *z)
{
 int size;
 switch (z->type->size) {
  case MS_TINY:   size =  4; break;
  case MS_SMALL:  size =  8; break;
  case MS_MEDIUM: size = 14; break;
  case MS_LARGE:  size = 20; break;
  case MS_HUGE:   size = 26; break;
 }
 g->explosion(z->posx, z->posy, size, 0, false);
}

void mdeath::ratking(game *g, monster *z)
{
 g->u.rem_disease(DI_RAT);
}

void mdeath::gameover(game *g, monster *z)
{
 g->add_msg("Your %s is destroyed!  GAME OVER!", z->name().c_str());
 g->u.hp_cur[hp_torso] = 0;
}

void mdeath::kill_breathers(game *g, monster *z)
{
 for (int i = 0; i < g->z.size(); i++) {
  if (g->z[i].type->id == mon_breather_hub || g->z[i].type->id == mon_breather)
   g->z[i].dead = true;
 }
}
