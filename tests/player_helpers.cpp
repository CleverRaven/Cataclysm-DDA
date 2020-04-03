#include "player_helpers.h"

#include <list>
#include <memory>
#include <vector>

#include "avatar.h"
#include "bionics.h"
#include "character.h"
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

void clear_character( player &dummy, bool debug_storage )
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
    if( debug_storage && !dummy.has_trait( trait_id( "DEBUG_STORAGE" ) ) ) {
        dummy.set_mutation( trait_id( "DEBUG_STORAGE" ) );
    }

    dummy.empty_skills();
    dummy.clear_morale();
    dummy.clear_bionics();
    dummy.activity.set_to_null();

    // Restore all stamina and go to walk mode
    dummy.set_stamina( dummy.get_stamina_max() );
    dummy.set_movement_mode( CMM_WALK );

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

void give_and_activate_bionic( player &p, bionic_id const &bioid )
{
    INFO( "bionic " + bioid.str() + " is valid" );
    REQUIRE( bioid.is_valid() );

    p.add_bionic( bioid );
    INFO( "dummy has gotten " + bioid.str() + " bionic " );
    REQUIRE( p.has_bionic( bioid ) );

    // get bionic's index - might not be "last added" due to "integrated" ones
    int bioindex = -1;
    for( size_t i = 0; i < p.my_bionics->size(); i++ ) {
        const auto &bio = ( *p.my_bionics )[ i ];
        if( bio.id == bioid ) {
            bioindex = i;
        }
    }
    REQUIRE( bioindex != -1 );

    const bionic &bio = p.bionic_at_index( bioindex );
    REQUIRE( bio.id == bioid );

    // turn on if possible
    if( bio.id->toggled && !bio.powered ) {
        const std::vector<itype_id> fuel_opts = bio.info().fuel_opts;
        if( !fuel_opts.empty() ) {
            p.set_value( fuel_opts.front(), "2" );
        }
        p.activate_bionic( bioindex );
        INFO( "bionic " + bio.id.str() + " with index " + std::to_string( bioindex ) + " is active " );
        REQUIRE( p.has_active_bionic( bioid ) );
        if( !fuel_opts.empty() ) {
            p.remove_value( fuel_opts.front() );
        }
    }
}

