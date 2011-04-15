#include "mission.h"
#include "game.h"

#define MAX_HIVE_DIST 30

bool mis_place::danger(game *g, int posx, int posy)
{
 return (g->cur_om.monsters_at(posx, posy).size() > 0);
}

bool mis_place::get_jelly(game *g, int posx, int posy)
{
// First, make sure there's fungaloids here to cause the infection.
 std::vector<mongroup> monsters = g->cur_om.monsters_at(posx, posy);
 bool found_fungus = false;
 for (int i = 0; i < monsters.size(); i++) {
  if (monsters[i].type == mcat_fungi) {
   found_fungus = true;
   i = monsters.size();
  }
 }
 if (!found_fungus)
  return false;
// Now, make sure there's a bee hive close enough.
 int dist = MAX_HIVE_DIST;
 point hive = g->cur_om.find_closest(point(posx, posy), ot_hive_center, 1, dist,
                                     false);
 if (hive.x == -1)
  return false;
 return true;
}

bool mis_place::lost_npc(game *g, int posx, int posy)
{
// Basically, just don't do it in the middle of a field
 for (int x = posx - 5; x <= posx + 5; x++) {
  for (int y = posy - 5; y <= posy + 5; y++) {
   if (g->cur_om.ter(x, y) != ot_field && g->cur_om.ter(x, y) != ot_null)
    return false;
  }
 }
 return true;
}

bool mis_place::kidnap_victim(game *g, int posx, int posy)
{
// We need to have at least one faction, to do the kidnapping
// The victim may or may not belong to a faction.
 return (g->factions_at(posx, posy).size() > 0);
}
