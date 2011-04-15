#include "mission.h"
#include "game.h"

/* These functions are responsible for making changes to the game at the moment
 * the mission is accepted by the player.  They are also responsible for
 * updating *miss with the target and any other important information.
 */

void mis_start::standard(game *g, mission *miss)
{
}

void mis_start::get_jelly(game *g, mission *miss)
{
// Find the closest hive, and reveal it on our map.
 int dist = 0;
 point hive = g->cur_om.find_closest( point(g->levx, g->levy), ot_hive_center,
                                      1, dist, false);
// Reveal the hive (and surrounding terrain) on the player's map
 for (int x = hive.x - 2; x <= hive.x + 2; x++) {
  for (int y = hive.y - 2; y <= hive.y + 2; y++)
   g->cur_om.seen(x, y) = true;
 }
 miss->target = hive;
}

void mis_start::get_jelly_ignt(game *g, mission *miss)
{
// Same as above, but without revealing the hive.
 int dist = 0;
 point hive = g->cur_om.find_closest(
                point(g->levx, g->levy), ot_hive_center, 1, dist, false);
 miss->target = hive;
}

void mis_start::lost_npc(game *g, mission *miss)
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
}

void mis_start::kidnap_victim(game *g, mission *miss)
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
}
