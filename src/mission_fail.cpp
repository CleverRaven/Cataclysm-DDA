#include "mission.h"
#include "game.h"
#include "overmapbuffer.h"

void mission_fail::kill_npc(mission *miss)
{
    for (size_t i = 0; i < g->active_npc.size(); i++) {
        if (g->active_npc[i]->getID() == miss->npc_id) {
            g->active_npc[i]->die( nullptr );
            // Actuall removoal of the npc is done in game::cleanup_dead
            break;
        }
    }
    npc *n = overmap_buffer.find_npc( miss->npc_id );
    if( n != nullptr && !n->is_dead() ) {
        // in case the npc was not inside the reality bubble, mark it as dead anyway.
        n->marked_for_death = true;
    }
}
