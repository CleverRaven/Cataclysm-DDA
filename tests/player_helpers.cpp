#include "player_helpers.h"

#include <list>
#include <memory>
#include <vector>

#include "avatar.h"
#include "game.h"
#include "item.h"
#include "itype.h"
#include "player.h"
#include "inventory.h"
#include "map.h"
#include "npc.h"
#include "player_activity.h"
#include "type_id.h"
#include "point.h"

#include "catch/catch.hpp"

int get_remaining_charges( const std::string &tool_id )
{
    const inventory crafting_inv = g->u.crafting_inventory();
    std::vector<const item *> items =
    crafting_inv.items_with( [tool_id]( const item & i ) {
        return i.typeId() == tool_id;
    } );
    int remaining_charges = 0;
    for( const item *instance : items ) {
        remaining_charges += instance->ammo_remaining();
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

void clear_character( player &dummy )
{
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

    dummy.activity.set_to_null();

    // Make stats nominal.
    dummy.str_cur = 8;
    dummy.dex_cur = 8;
    dummy.int_cur = 8;
    dummy.per_cur = 8;

    const tripoint spot( 60, 60, 0 );
    g->place_player( spot );
}

void clear_avatar()
{
    clear_character( g->u );
}

void process_activity( player &dummy )
{
    do {
        dummy.moves += dummy.get_speed();
        while( dummy.moves > 0 && dummy.activity ) {
            dummy.activity.do_turn( dummy );
        }
    } while( dummy.activity );
}

npc &spawn_npc( const point &p, const std::string &npc_class )
{
    const string_id<npc_template> test_guy( npc_class );
    const character_id model_id = g->m.place_npc( p, test_guy, true );
    g->load_npcs();

    npc *guy = g->find_npc( model_id );
    REQUIRE( guy != nullptr );
    CHECK( !guy->in_vehicle );
    return *guy;
}
