#include "player_helpers.h"

#include <list>

#include "enums.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "player.h"

int get_remaining_charges( const std::string &tool_id )
{
    const inventory crafting_inv = g->u.crafting_inventory();
    std::vector<const item *> items =
    crafting_inv.items_with( [tool_id]( const item & i ) {
        return i.typeId() == tool_id;
    } );
    int remaining_charges = 0;
    for( const item *instance : items ) {
        remaining_charges += instance->charges;
    }
    return remaining_charges;
}

bool player_has_item_of_type( const std::string &type )
{
    std::vector<item *> matching_items = g->u.inv.items_with(
    [&]( const item & i ) {
        return i.type->get_id() == type;
    } );

    return !matching_items.empty();
}

void clear_player()
{
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );
    dummy.inv.clear();
    dummy.remove_weapon();
    for( const trait_id &tr : dummy.get_mutations() ) {
        dummy.unset_mutation( tr );
    }
    // Prevent spilling, but don't cause encumbrance
    if( !dummy.has_trait( trait_id( "DEBUG_STORAGE" ) ) ) {
        dummy.set_mutation( trait_id( "DEBUG_STORAGE" ) );
    }

    dummy.clear_morale();

    dummy.clear_bionics();

    // Make stats nominal.
    dummy.str_cur = 8;
    dummy.dex_cur = 8;
    dummy.int_cur = 8;
    dummy.per_cur = 8;

    const tripoint spot( 60, 60, 0 );
    dummy.setpos( spot );
}
