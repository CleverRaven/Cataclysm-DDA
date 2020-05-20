#include "catch/catch.hpp"

#include "item.h"
#include "item_contents.h"

#include <sstream>

TEST_CASE( "item_contents" )
{
    item tool_belt( "test_tool_belt" );

    const units::volume tool_belt_vol = tool_belt.volume();
    const units::mass tool_belt_weight = tool_belt.weight();

    item hammer( "hammer" );
    item tongs( "tongs" );
    item wrench( "wrench" );
    item crowbar( "crowbar" );

    tool_belt.put_in( hammer, item_pocket::pocket_type::CONTAINER );
    tool_belt.put_in( tongs, item_pocket::pocket_type::CONTAINER );
    tool_belt.put_in( wrench, item_pocket::pocket_type::CONTAINER );
    tool_belt.put_in( crowbar, item_pocket::pocket_type::CONTAINER );

    // check the items actually got added to the tool belt
    REQUIRE( tool_belt.contents.num_item_stacks() == 4 );
    // tool belts are non-rigid
    CHECK( tool_belt.volume() == tool_belt_vol +
           hammer.volume() + tongs.volume() + wrench.volume() + crowbar.volume() );
    // check that the tool belt's weight adds up all its contents properly
    CHECK( tool_belt.weight() == tool_belt_weight +
           hammer.weight() + tongs.weight() + wrench.weight() + crowbar.weight() );
    // check that the tool belt is "full"
    CHECK( !tool_belt.contents.can_contain( hammer ).success() );

    tool_belt.contents.force_insert_item( hammer, item_pocket::pocket_type::CONTAINER );
    CHECK( tool_belt.contents.num_item_stacks() == 5 );
    tool_belt.contents.overflow( tripoint_zero );
    CHECK( tool_belt.contents.num_item_stacks() == 4 );
    tool_belt.contents.overflow( tripoint_zero );
    // overflow should only spill items if they can't fit
    CHECK( tool_belt.contents.num_item_stacks() == 4 );

    tool_belt.contents.remove_items_if( []( item & it ) {
        return it.typeId() == itype_id( "hammer" );
    } );
    // check to see that removing an item works
    CHECK( tool_belt.contents.num_item_stacks() == 3 );
    tool_belt.spill_contents( tripoint_zero );
    CHECK( tool_belt.contents.empty() );
}
