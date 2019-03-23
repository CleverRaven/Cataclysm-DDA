#include "mission.h" // IWYU pragma: associated

#include "game.h"
#include "npc.h"
#include "overmapbuffer.h"

void mission_fail::kill_npc( mission *miss )
{
    if( npc *const elem = g->critter_by_id<npc>( miss->get_npc_id() ) ) {
        elem->die( nullptr );
        // Actual removal of the npc is done in game::cleanup_dead
    }
    std::shared_ptr<npc> n = overmap_buffer.find_npc( miss->get_npc_id() );
    if( n != nullptr && !n->is_dead() ) {
        // in case the npc was not inside the reality bubble, mark it as dead anyway.
        n->marked_for_death = true;
    }
}
