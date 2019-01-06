#include "mission.h" // IWYU pragma: associated

#include "debug.h"
#include "game.h"
#include "messages.h"
#include "npc.h"
#include "output.h"
#include "rng.h"
#include "translations.h"

const efftype_id effect_infection( "infection" );

void mission_end::heal_infection( mission *miss )
{
    npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->get_npc_id() );
        return;
    }
    p->remove_effect( effect_infection );
}

void mission_end::leave( mission *miss )
{
    npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->get_npc_id() );
        return;
    }
    p->set_attitude( NPCATT_NULL );
}

void mission_end::thankful( mission *miss )
{
    npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->get_npc_id() );
        return;
    }
    if( p->get_attitude() == NPCATT_MUG || p->get_attitude() == NPCATT_WAIT_FOR_LEAVE ||
        p->get_attitude() == NPCATT_FLEE || p->get_attitude() == NPCATT_KILL ) {
        p->set_attitude( NPCATT_NULL );
    }
    if( p->chatbin.first_topic != "TALK_FRIEND" ) {
        p->chatbin.first_topic = "TALK_STRANGER_FRIENDLY";
    }
    p->personality.aggression -= 1;
}

void mission_end::deposit_box( mission *miss )
{
    npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->get_npc_id() );
        return;
    }
    p->set_attitude( NPCATT_NULL );//npc leaves your party
    std::string itemName = "deagle_44";
    if( one_in( 4 ) ) {
        itemName = "katana";
    } else if( one_in( 3 ) ) {
        itemName = "m4a1";
    }
    g->u.i_add( item( itemName, 0 ) );
    add_msg( m_good, _( "%s gave you an item from the deposit box." ), p->name.c_str() );
}
