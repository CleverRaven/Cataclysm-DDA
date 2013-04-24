#include "mission.h"
#include "game.h"

void mission_fail::kill_npc(game *g, mission *miss)
{
    for (int i = 0; i < g->active_npc.size(); i++)
    {
        if (g->active_npc[i]->getID() == miss->npc_id)
        {
            npc * tmp = g->active_npc[i];
            g->active_npc.erase(g->active_npc.begin() + i);
            tmp->die(g, false);
            i = g->active_npc.size();
        }
    }
    for(int i = 0; i < g->cur_om.npcs.size(); i++) //now remove the npc from the overmap list.
    {
        if(g->cur_om.npcs[i]->getID() == miss->npc_id)
        {
            if(!g->cur_om.npcs[i]->dead)
               g->cur_om.npcs[i]->die(g, false);
            return;
        }
    }
}
