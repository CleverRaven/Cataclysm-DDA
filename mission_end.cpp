#include "mission.h"
#include "game.h"

void mission_end::heal_infection(game *g, mission *miss)
{
 bool found_npc = false;
 for (int i = 0; !found_npc && i < g->cur_om->npcs.size(); i++) {
  if (g->cur_om->npcs[i]->getID() == miss->npc_id) {
   g->cur_om->npcs[i]->rem_disease(DI_INFECTION);
   found_npc = true;
  }
 }
}

void mission_end::leave(game *g, mission *miss)
{
 npc *p = g->find_npc(miss->npc_id);
 p->attitude = NPCATT_NULL;
}
