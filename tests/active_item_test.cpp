#include <list>
#include <new>
#include <optional>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "itype.h"
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

    here.add_item( player_character.pos_bub(), active_item );
    // TODO: spawn a vehicle and stash a chainsaw in there too.

    // Call item processing entry points.
    here.process_items();
    player_character.process_items();

    // Each chainsaw was processed and turned off from lack of fuel
    CHECK( inventory_item->typeId().str() == "chainsaw_off" );
    CHECK( player_character.get_wielded_item()->typeId().str() == "chainsaw_off" );
    CHECK( here.i_at( player_character.pos_bub() ).only_item().typeId().str() == "chainsaw_off" );
}

TEST_CASE( "tool_power_consumption_rate", "[active_item]" )
{
    // Give the flashlight a fully charged battery, 56 kJ
    item test_battery( "medium_battery_cell" );
    test_battery.ammo_set( test_battery.ammo_default(), 56 );
    REQUIRE( test_battery.energy_remaining() == 56_kJ );

    item tool( "flashlight_on" );
    tool.put_in( test_battery, pocket_type::MAGAZINE_WELL );
    REQUIRE( tool.energy_remaining() == 56_kJ );
    tool.active = true;

    // Now process the tool until it runs out of battery power, which should be about 10h.
    int seconds_of_discharge = 0;
    map &here = get_map();
    // Capture now because after the loop the tool will be an inactive tool with no power draw.
    units::energy minimum_energy = tool.type->tool->power_draw * 1_seconds;
    do {
        tool.process( here, nullptr, tripoint_zero );
        seconds_of_discharge++;
    } while( tool.active );
    REQUIRE( tool.energy_remaining() < minimum_energy );
    // Just a loose check, 9 - 10 hours runtime, based on the product page.
    CHECK( seconds_of_discharge > to_seconds<int>( 9_hours + 30_minutes ) );
    CHECK( seconds_of_discharge < to_seconds<int>( 10_hours ) );
}
