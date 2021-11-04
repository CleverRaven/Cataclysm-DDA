#include "mission.h" // IWYU pragma: associated

#include <memory>

#include "character.h"
#include "debug.h"
#include "game.h"
#include "messages.h"
#include "npc.h"
#include "rng.h"
#include "translations.h"
#include "item.h"

void mission_end::deposit_box( mission *miss )
{
    npc *p = g->find_npc( miss->get_npc_id() );
    if( p == nullptr ) {
        debugmsg( "could not find mission NPC %d", miss->get_npc_id().get_value() );
        return;
    }
    // Npc leaves your party
    p->set_attitude( NPCATT_NULL );
    std::string itemName = "deagle_44";
    if( one_in( 4 ) ) {
        itemName = "katana";
    } else if( one_in( 3 ) ) {
        itemName = "m4_carbine";
    }
    get_player_character().i_add( item( itemName, calendar::turn_zero ) );
    add_msg( m_good, _( "%s gave you an item from the deposit box." ), p->get_name() );
}
