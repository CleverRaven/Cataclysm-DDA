#include "catch/catch.hpp"

#include "item.h"
#include "item_contents.h"

#include <sstream>

TEST_CASE( "item_contents" )
{
    item tool_belt( "tool_belt" );

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

    INFO( "test_save_load" );
    std::ostringstream os;
    JsonOut jsout( os );
    jsout.write( tool_belt );
    std::istringstream is( os.str() );
    JsonIn jsin( is );
    item read_val;
    jsin.read( read_val );
    // a tool belt has 4 pockets. check to see we've deserialized it properly
    CHECK( read_val.contents.size() == 4 );
    // check to see that there are 4 items in the pockets
    CHECK( read_val.contents.num_item_stacks() == 4 );
    // check to see if the items are in the same pockets as they were
    CHECK( tool_belt.contents.same_contents( read_val.contents ) );

    tool_belt.contents.force_insert_item( crowbar, item_pocket::pocket_type::CONTAINER );
    CHECK( tool_belt.contents.num_item_stacks() == 5 );
    tool_belt.contents.overflow( tripoint_zero );
    CHECK( tool_belt.contents.num_item_stacks() == 4 );
    // the item that dropped should have been the crowbar
    CHECK( tool_belt.contents.same_contents( read_val.contents ) );
    tool_belt.contents.overflow( tripoint_zero );
    // overflow should only spill items if they can't fit
    CHECK( tool_belt.contents.num_item_stacks() == 4 );

    tool_belt.contents.remove_items_if( []( item & it ) {
        return it.typeId() == "hammer";
    } );
    // check to see that removing an item works
    CHECK( tool_belt.contents.num_item_stacks() == 3 );
    tool_belt.spill_contents( tripoint_zero );
    CHECK( tool_belt.contents.empty() );
}
