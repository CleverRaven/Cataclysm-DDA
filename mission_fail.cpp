#include "mission.h"
#include "game.h"

void mission_fail::kill_npc(game *g, mission *miss)
{
 for (int i = 0; i < g->active_npc.size(); i++) {
  if (g->active_npc[i].id == miss->npc_id) {
   g->active_npc[i].die(g, false);
   g->active_npc.erase(g->active_npc.begin() + i);
   return;
  }
 }
 for (int i = 0; i < g->cur_om.npcs.size(); i++) {
  if (g->cur_om.npcs[i].id == miss->npc_id) {
   g->cur_om.npcs[i].marked_for_death = true;
   return;
  }
 }
}
