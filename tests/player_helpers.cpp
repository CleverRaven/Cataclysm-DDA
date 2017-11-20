#include "player_helpers.h"

#include "enums.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "player.h"

#include <list>

void clear_player()
{
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );
    for( trait_id tr : dummy.get_mutations() ) {
        dummy.unset_mutation( tr );
    }
    // Prevent spilling, but don't cause encumbrance
    if( !dummy.has_trait( trait_id( "DEBUG_STORAGE" ) ) ) {
        dummy.set_mutation( trait_id( "DEBUG_STORAGE" ) );
    }

    // Make stats nominal.
    dummy.str_cur = 8;
    dummy.dex_cur = 8;
    dummy.int_cur = 8;
    dummy.per_cur = 8;

    const tripoint spot( 60, 60, 0 );
    dummy.setpos( spot );
}
