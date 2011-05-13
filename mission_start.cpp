#include "mission.h"
#include "game.h"
#include <sstream>

/* These functions are responsible for making changes to the game at the moment
 * the mission is accepted by the player.  They are also responsible for
 * updating *miss with the target and any other important information.
 */

void mission_start::standard(game *g, mission *miss)
{
}

void mission_start::danger(game *g, mission *miss)
{
// Set the destination to the closest tile with no monsters
 bool done = false;
 point target;
 int omx = (g->levx + 1) / 2, omy = (g->levy + 1) / 2;
 int current = g->cur_om.monsters_at(omx, omy).size(), best = 999;
 for (int dist = 1; dist < 50 && !done; dist++) {
  int startx = omx - dist, endx = omx + dist,
      starty = omy - dist, endy = omy + dist;
  if (startx < 0)
   startx = 0;
  if (starty < 0)
   starty = 0;
  if (endx >= OMAPX)
   endx = OMAPX - 1;
  if (endy >= OMAPY)
   endy = OMAPY - 1;
  for (int x = startx; x <= endx && !done; x++) {
   for (int y = starty; y <= endy && !done; y++) {
    int count = g->cur_om.monsters_at(x, y).size();
    if (count < current) {
     target = point(x, y);
     done = true;
    } else if (count < best) {
     best = count;
     target = point(x, y);
    }
   }
  }
 }
 miss->description = "\
Reach the target point; there you will be relatively safe from monsters.";
 miss->target = target;
 g->cur_om.seen(target.x, target.y) = true;
}

void mission_start::find_family(game *g, mission *miss)
{
 miss->description = "\
You need to know if your brother is alive.  Check his house.";
// Find a house, at least 12 tiles away
 int origx = (g->levx + 1) / 2, origy = (g->levy + 1) / 2;
 bool done = false;
 for (int dist = 12; dist < 30 && !done; dist++) {
  int startx = origx - dist, endx = origx + dist,
      starty = origy - dist, endy = origy + dist;

  if (startx < 0)	startx = 0;
  if (starty < 0)	starty = 0;
  if (endx >= OMAPX)	endx = OMAPX - 1;
  if (endy >= OMAPY)	endy = OMAPY - 1;

  std::vector<point> possible;
  for (int x = startx; x <= endx; x++) {
   if (g->cur_om.ter(x, starty) >= ot_house_north && 
       g->cur_om.ter(x, starty) <= ot_house_west    ) {
    possible.push_back(point(x, starty));
    x = endx + 1;
   }
   if (g->cur_om.ter(x, endy) >= ot_house_north &&
       g->cur_om.ter(x, endy) <= ot_house_west    ) {
    possible.push_back(point(x, endy));
    x = endx + 1;
   }
  }
  for (int y = starty; y <= endy; y++) {
   if (g->cur_om.ter(startx, y) >= ot_house_north &&
       g->cur_om.ter(startx, y) <= ot_house_west    ) {
    possible.push_back(point(startx, y));
    y = endy + 1;
   }
   if (g->cur_om.ter(endx, y) >= ot_house_north &&
       g->cur_om.ter(endx, y) <= ot_house_west    ) {
    possible.push_back(point(endx, y));
    y = endy + 1;
   }
  }
  if (possible.size() > 0) {
   int index = rng(0, possible.size() - 1);
   miss->target = possible[index];
   done = true;
  }
 }
 if (!done) {
  debugmsg("Find Your Brother mission couldn't find a valid house!");
  return;
 }
/*
 if (!one_in(3)) { // 66% chance: brother joined a gang
  map home(&(g->itypes), &(g->mapitems), &(g->traps));
  home.load(g, miss->target.x, miss->target.y);
  for (int x = 0; x < SEEX * 2; x++) {
   for (int y = 0; y < SEEY * 2; y++) {
    if (home.ter(x, y) == t_bed) {
    */
}

void mission_start::get_jelly(game *g, mission *miss)
{
// Find the closest hive, and reveal it on our map.
 int dist = 0;
 point omp = point((g->levx + 1) / 2, (g->levy + 1) / 2);
 point hive = g->cur_om.find_closest( omp, ot_hive_center, 1, dist, false);
// Reveal the hive (and surrounding terrain) on the player's map
 for (int x = hive.x - 2; x <= hive.x + 2; x++) {
  for (int y = hive.y - 2; y <= hive.y + 2; y++)
   g->cur_om.seen(x, y) = true;
 }
 miss->target = hive;
 miss->description = "\
Travel to the nearby bee hive to collect royal jelly, then return the cure to\n\
the fungus-infected humans.";
}

void mission_start::get_jelly_ignt(game *g, mission *miss)
{
// Same as above, but without revealing the hive.
 int dist = 0;
 point omp = point((g->levx + 1) / 2, (g->levy + 1) / 2);
 miss->target = omp;
 miss->description = "\
Find a cure for the fungus-infected humans and return with it.";
}

void mission_start::lost_npc(game *g, mission *miss)
{
 npc missing;
 missing.randomize(g);
 missing.attitude = NPCATT_MISSING;
 missing.mission  = NPC_MISSION_MISSING;
 
 int xdif = rng(20, 30) * (one_in(2) ? 1 : -1),
     ydif = rng(20, 30) * (one_in(2) ? 1 : -1);
 missing.mapx = g->levx + xdif;
 missing.mapy = g->levy + ydif;
 if (missing.mapx < 0)
  missing.mapx = 0;
 if (missing.mapx >= OMAPX)
  missing.mapx = OMAPX - 1;
 if (missing.mapy < 0)
  missing.mapy = 0;
 if (missing.mapy >= OMAPY)
  missing.mapy = OMAPY - 1;
 miss->target = point(missing.mapx, missing.mapy);
 miss->npc_id = missing.id;
 std::stringstream desc;
 desc << missing.name << " is missing.  Find " <<
         (missing.male ? "him" : "her") << " and return to " <<
         (missing.male ? "his" : "her") << " friends.";
 miss->description = desc.str();
}

void mission_start::kidnap_victim(game *g, mission *miss)
{
 npc missing;
 missing.randomize(g);
 missing.attitude = NPCATT_KIDNAPPED;
 missing.mission  = NPC_MISSION_KIDNAPPED;

 int xdif = rng(20, 30) * (one_in(2) ? 1 : -1),
     ydif = rng(20, 30) * (one_in(2) ? 1 : -1);
 missing.mapx = g->levx + xdif;
 missing.mapy = g->levy + ydif;
 if (missing.mapx < 0)
  missing.mapx = 0;
 if (missing.mapx >= OMAPX)
  missing.mapx = OMAPX - 1;
 if (missing.mapy < 0)
  missing.mapy = 0;
 if (missing.mapy >= OMAPY)
  missing.mapy = OMAPY - 1;
 miss->target = point(missing.mapx, missing.mapy);
 miss->npc_id = missing.id;
 std::stringstream desc;
 desc << missing.name << " has been kidnapped.  Rescue " <<
         (missing.male ? "him" : "her") << " and return to " <<
         (missing.male ? "his" : "her") << " friends.";
 miss->description = desc.str();
}
