#include "computer.h"
#include "game.h"
#include "monster.h"
#include "overmap.h"
#include <fstream>

void computerk::spawn_manhacks(game *g)
{
 int x = g->u.posx, y = g->u.posy;
 g->sound(x, y, 40, "An alarm sounds!");
 g->add_msg("Manhacks drop from compartments in the ceiling.");
 monster robot(g->mtypes[mon_manhack]);
 int mx, my, num_robots = rng(4, 8);
 for (int i = 0; i < num_robots; i++) {
  int tries = 0;
  do {
   mx = rng(x - 3, x + 3);
   my = rng(y - 3, y + 3);
   tries++;
  } while(!g->is_empty(mx, my) && tries < 10);
  if (tries != 10) {
   robot.spawn(mx, my);
   g->z.push_back(robot);
  }
 }
}

void computerk::spawn_secubots(game *g)
{
 int x = g->u.posx, y = g->u.posy;
 g->sound(x, y, 40, "An alarm sounds!");
 g->add_msg("Secubots emerge from compartments in the floor.");
 monster robot(g->mtypes[mon_secubot]);
 int mx, my, num_robots = rng(4, 8);
 for (int i = 0; i < num_robots; i++) {
  int tries = 0;
  do {
   mx = rng(x - 3, x + 3);
   my = rng(y - 3, y + 3);
   tries++;
  } while(!g->is_empty(mx, my) && tries < 10);
  if (tries != 10) {
   robot.spawn(mx, my);
   g->z.push_back(robot);
  }
 }
}

// USE FUNCTIONS
// Note that these return false if the computer used is to be broken

bool computerk::release(game *g, int success)
{
 int x = g->u.posx, y = g->u.posy;
 g->sound(x - 3, y - 3, 40, "an alarm sounding!");
 if (success <= 0) {
  g->add_msg("ERROR: Improper variable entered.  Please try again.");
  return true;
 }
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEY * 3; j++) {
   if (g->m.ter(i, j) == t_reinforced_glass_h ||
       g->m.ter(i, j) == t_reinforced_glass_v)
    g->m.ter(i, j) = t_floor;
  }
 }
 return true;
}

bool computerk::terminate(game *g, int success)
{
 if (success <= 1) {
  g->add_msg("ERROR: Improper variable entered.  Please try again.");
  return true;
 }
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEY * 3; j++) {
   int mondex = g->mon_at(i, j);
   if (mondex != -1 &&
       ((g->m.ter(i, j - 1) == t_reinforced_glass_h &&
         g->m.ter(i, j + 1) == t_wall_h) ||
        (g->m.ter(i, j + 1) == t_reinforced_glass_h &&
         g->m.ter(i, j - 1) == t_wall_h)))
    g->kill_mon(mondex);
  }
 }
 return true;
}

bool computerk::portal(game *g, int success)
{
 if (success < 0) {
  g->add_msg("Sequence incomplete.  Please try again.");
  return true;
 } else if (success < 2) {
  popup("ERROR!  Phase interference override!");
  g->resonance_cascade(g->u.posx, g->u.posy);
  return false;
 }
 for (int i = 0; i < SEEX * 3; i++) {
  for (int j = 0; j < SEEY * 3; j++) {
   int numtowers = 0;
   for (int xt = i - 2; xt <= i + 2; xt++) {
    for (int yt = j - 2; yt <= j + 2; yt++) {
     if (g->m.ter(xt, yt) == t_radio_tower)
      numtowers++;
    }
   }
   if (numtowers == 4) {
    if (g->m.tr_at(i, j) == tr_portal)
     g->m.tr_at(i, j) = tr_null;
    else
     g->m.add_trap(i, j, tr_portal);
   }
  }
 }
 return true;
}
 
bool computerk::cascade(game *g, int success)
{
 int x = g->u.posx, y = g->u.posy;
 if (success < 2) {
  g->add_msg("Sequence incomplete; please try again.");
  return true;
 }
 if (!query_yn("WARNING: Resonance Cascade carries severe risk!  Continue?"))
  return true;
 if (success > 5) {
  std::vector<point> cascade_points;
  for (int i = x - 10; i <= x + 10; i++) {
   for (int j = y - 10; j <= y + 10; j++) {
    if (g->m.ter(i, j) == t_radio_tower)
     cascade_points.push_back(point(i, j));
   }
  }
  if (cascade_points.size() == 0)
   g->resonance_cascade(x, y);
  else {
   point p = cascade_points[rng(0, cascade_points.size() - 1)];
   g->resonance_cascade(p.x, p.y);
  }
 } else
  g->resonance_cascade(x, y);
 return false;
}
 
bool computerk::research(game *g, int success)
{
 if (success >= 1) {
  int lines = 0, notes = 0;
  std::string log, tmp;
  int ch;
  std::ifstream fin;
  fin.open("data/LAB_NOTES");
  if (!fin.is_open()) {
   debugmsg("Couldn't open LAB_NOTES for reading");
   return false;
  }
  while (fin.good()) {
   ch = fin.get();
   if (ch == '%')
    notes++;
  }
   
  while (lines < 23 && lines < success * 5) {
   fin.clear();
   fin.seekg(0, std::ios::beg);
   fin.clear();
   int choice = rng(1, notes);
   while (choice > 0) {
    getline(fin, tmp);
    if (tmp.find_first_of('%') == 0)
     choice--;
   }
   bool get_okay;
   getline(fin, tmp);
   do {
    lines++;
    if (lines < 23 && lines < success * 5 &&
        tmp.find_first_of('%') != 0) {
     log.append(tmp);
     log.append("\n");
    }
   } while(tmp.find_first_of('%') != 0 && getline(fin, tmp));
  }
  full_screen_popup(log.c_str());
 } else if (one_in(4)) {
  g->add_msg("ERROR - OS CORRUPTED!");
  return false;
 } else
  g->add_msg("Access denied.");
 return true;
}
 
bool computerk::maps(game *g, int success)
{
 if (success >= 0) {
  int minx = int(g->levx / 2) - 20 - success * 15;
  int maxx = int(g->levx / 2) + 20 + success * 15;
  int miny = int(g->levy / 2) - 20 - success * 15;
  int maxy = int(g->levy / 2) + 20 + success * 15;
  if (minx < 0)	     minx = 0;
  if (maxx >= OMAPX) maxx = OMAPX - 1;
  if (miny < 0)	     miny = 0;
  if (maxy >= OMAPY) maxy = OMAPY - 1;
  overmap tmp(g, g->cur_om.posx, g->cur_om.posy, 0);
  for (int i = minx; i <= maxx; i++) {
   for (int j = miny; j <= maxy; j++)
    tmp.seen(i, j) = true;
  }
  tmp.save(g->u.name, g->cur_om.posx, g->cur_om.posy, 0);
  g->add_msg("Surface map data downloaded.");
 } else {
  g->add_msg("Surface map data corrupted.");
  if (one_in(4)) {
   g->add_msg("The computer breaks down!");
   return true;
  }
 }
 return true;
}
 
bool computerk::launch(game *g, int success)
{
 return true;
}
bool computerk::disarm(game *g, int success)
{
 return true;
}
