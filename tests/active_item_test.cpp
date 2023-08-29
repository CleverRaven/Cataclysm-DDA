#include <list>
#include <new>
#include <optional>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"

TEST_CASE( "active_items_processed_regularly", "[active_item]" )
{
    clear_avatar();
    clear_map();
    avatar &player_character = get_avatar();
    map &here = get_map();
    // An arbitrary active item that ticks every turn.
    item active_item( "chainsaw_on" );
    active_item.active = true;

    item storage( "debug_backpack", calendar::turn_zero );
    std::optional<std::list<item>::iterator> wear_success = player_character.wear_item( storage );
    REQUIRE( wear_success );

    item_location inventory_item = player_character.try_add( active_item );
    REQUIRE( inventory_item != item_location::nowhere );

    bool wield_success = player_character.wield( active_item );
    REQUIRE( wield_success );

    here.add_item( player_character.pos(), active_item );
    // TODO: spawn a vehicle and stash a chainsaw in there too.

    // Call item processing entry points.
    here.process_items();
    player_character.process_items();

    // Each chainsaw was processed and turned off from lack of fuel
    CHECK( inventory_item->typeId().str() == "chainsaw_off" );
    CHECK( player_character.get_wielded_item()->typeId().str() == "chainsaw_off" );
    CHECK( here.i_at( player_character.pos() ).only_item().typeId().str() == "chainsaw_off" );
}
