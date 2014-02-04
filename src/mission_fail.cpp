#include "mission.h"
#include "game.h"
#include "overmapbuffer.h"

void mission_fail::kill_npc(mission *miss)
{
    for (int i = 0; i < g->active_npc.size(); i++) {
        if (g->active_npc[i]->getID() == miss->npc_id) {
            if(!g->active_npc[i]->dead) {
                g->active_npc[i]->die(false);
            }
            g->active_npc.erase(g->active_npc.begin() + i);
            break;
        }
    }
    // This deletes the npc object
    overmap_buffer.remove_npc(miss->npc_id);
}
