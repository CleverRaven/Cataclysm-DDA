#include "avatar.h"
#include "catch/catch.hpp"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"

TEST_CASE( "active_items_processed_regularly", "[item]" )
{
    clear_avatar();
    clear_map();
    avatar &player_character = get_avatar();
    map &here = get_map();
    // An arbitrary active item that ticks every turn.
    item active_item( "firecracker_act", 0, 5 );
    active_item.activate();
    const int active_item_ticks = active_item.charges;
    item storage( "backpack", 0 );
    cata::optional<std::list<item>::iterator> wear_success = player_character.wear_item( storage );
    REQUIRE( wear_success );

    item *inventory_item = player_character.try_add( active_item );
    REQUIRE( inventory_item != nullptr );
    REQUIRE( inventory_item->charges == active_item_ticks );

    bool wield_success = player_character.wield( active_item );
    REQUIRE( wield_success );
    REQUIRE( player_character.weapon.charges == active_item_ticks );

    here.add_item( player_character.pos(), active_item );
    REQUIRE( here.i_at( player_character.pos() ).only_item().charges == active_item_ticks );
    // TODO: spawn a vehicle and stash a firecracker in there too.

    // Call item processing entry points.
    here.process_items();
    player_character.process_items();

    const int expected_ticks = active_item_ticks - 1;
    CHECK( inventory_item->charges == expected_ticks );
    CHECK( player_character.weapon.charges == expected_ticks );
    CHECK( here.i_at( player_character.pos() ).only_item().charges == expected_ticks );
}
