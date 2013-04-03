#include "mission.h"
#include "game.h"

void mission_end::heal_infection(game *g, mission *miss)
{
 bool found_npc = false;
 for (int i = 0; i < g->active_npc.size() && !found_npc; i++) {
  if (g->active_npc[i].id == miss->npc_id) {
   g->active_npc[i].rem_disease(DI_INFECTION);
   found_npc = true;
  }
 }

 for (int i = 0; !found_npc && i < g->cur_om.npcs.size(); i++) {
  if (g->cur_om.npcs[i].id == miss->npc_id) {
   g->cur_om.npcs[i].rem_disease(DI_INFECTION);
   found_npc = true;
  }
 }
}
