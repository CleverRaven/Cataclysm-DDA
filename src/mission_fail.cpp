#include "mission.h"
#include "game.h"
#include "overmapbuffer.h"

void mission_fail::kill_npc(mission *miss)
{
    for( auto &elem : g->active_npc ) {
        if( elem->getID() == miss->npc_id ) {
            elem->die( nullptr );
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
