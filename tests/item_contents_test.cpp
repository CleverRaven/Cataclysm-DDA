#include <functional>

#include "cata_catch.h"
#include "item.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"

static const itype_id itype_crowbar_pocket_test( "crowbar_pocket_test" );
static const itype_id itype_jar_glass_sealed( "jar_glass_sealed" );
static const itype_id itype_log( "log" );
static const itype_id itype_pickle( "pickle" );
static const itype_id itype_purse( "purse" );

TEST_CASE( "item_contents" )
{
    clear_map();
    item tool_belt( "test_tool_belt" );

    const units::volume tool_belt_vol = tool_belt.volume();
    const units::mass tool_belt_weight = tool_belt.weight();

    //check empty weight is consistent
    CHECK( tool_belt.weight( true ) == tool_belt.type->weight );
    CHECK( tool_belt.weight( false ) == tool_belt.type->weight );

    item hammer( "hammer_pocket_test" );
    item tongs( "tongs_pocket_test" );
    item wrench( "wrench_pocket_test" );
    item crowbar( "crowbar_pocket_test" );

    ret_val<void> i1 = tool_belt.put_in( hammer, item_pocket::pocket_type::CONTAINER );
    ret_val<void> i2 = tool_belt.put_in( tongs, item_pocket::pocket_type::CONTAINER );
    ret_val<void> i3 = tool_belt.put_in( wrench, item_pocket::pocket_type::CONTAINER );
    ret_val<void> i4 = tool_belt.put_in( crowbar, item_pocket::pocket_type::CONTAINER );

    {
        CAPTURE( i1.str() );
        CHECK( i1.success() );
    }
    {
        CAPTURE( i2.str() );
        CHECK( i2.success() );
    }
    {
        CAPTURE( i3.str() );
        CHECK( i3.success() );
    }
    {
        CAPTURE( i4.str() );
        CHECK( i4.success() );
    }

    // check the items actually got added to the tool belt
    REQUIRE( tool_belt.num_item_stacks() == 4 );
    // tool belts are non-rigid
    CHECK( tool_belt.volume() == tool_belt_vol +
           hammer.volume() + tongs.volume() + wrench.volume() + crowbar.volume() );
    // check that the tool belt's weight adds up all its contents properly
    CHECK( tool_belt.weight() == tool_belt_weight +
           hammer.weight() + tongs.weight() + wrench.weight() + crowbar.weight() );
    // check that individual (not including contained items) weight is correct
    CHECK( tool_belt.weight( false ) == tool_belt.type->weight );
    // check that the tool belt is "full"
    CHECK( !tool_belt.can_contain( crowbar ).success() );

    tool_belt.force_insert_item( crowbar, item_pocket::pocket_type::CONTAINER );
    CHECK( tool_belt.num_item_stacks() == 5 );
    tool_belt.force_insert_item( crowbar, item_pocket::pocket_type::CONTAINER );
    tool_belt.overflow( tripoint_zero );
    CHECK( tool_belt.num_item_stacks() == 4 );
    tool_belt.overflow( tripoint_zero );
    // overflow should only spill items if they can't fit
    CHECK( tool_belt.num_item_stacks() == 4 );

    tool_belt.remove_items_with( []( const item & it ) {
        return it.typeId() == itype_crowbar_pocket_test;
    } );
    // check to see that removing an item works
    CHECK( tool_belt.num_item_stacks() == 3 );
    tool_belt.spill_contents( tripoint_zero );
    CHECK( tool_belt.empty() );
}

TEST_CASE( "overflow_on_combine", "[item]" )
{
    clear_map();
    tripoint origin{ 60, 60, 0 };
    item purse( itype_purse );
    item log( itype_log );
    item_contents overfull_contents( purse.type->pockets );
    overfull_contents.force_insert_item( log, item_pocket::pocket_type::CONTAINER );
    capture_debugmsg_during( [&purse, &overfull_contents]() {
        purse.combine( overfull_contents );
    } );
    map &here = get_map();
    here.i_clear( origin );
    purse.overflow( origin );
    CHECK( here.i_at( origin ).size() == 1 );
}

TEST_CASE( "overflow_test", "[item]" )
{
    clear_map();
    tripoint origin{ 60, 60, 0 };
    item purse( itype_purse );
    item log( itype_log );
    purse.force_insert_item( log, item_pocket::pocket_type::MIGRATION );
    map &here = get_map();
    purse.overflow( origin );
    CHECK( here.i_at( origin ).size() == 1 );
}

TEST_CASE( "overflow_test_into_parent_item", "[item]" )
{
    clear_map();
    tripoint origin{ 60, 60, 0 };
    item jar( itype_jar_glass_sealed );
    item pickle( itype_pickle );
    pickle.force_insert_item( pickle, item_pocket::pocket_type::MIGRATION );
    jar.put_in( pickle, item_pocket::pocket_type::CONTAINER );
    int contents_pre = 0;
    for( item *it : jar.all_items_top() ) {
        contents_pre += it->count();
    }
    REQUIRE( contents_pre == 1 );

    item_location jar_loc( map_cursor( origin ), &jar );
    jar_loc.overflow();
    map &here = get_map();
    CHECK( here.i_at( origin ).empty() );

    int contents_count = 0;
    for( item *it : jar.all_items_top() ) {
        contents_count += it->count();
    }
    CHECK( contents_count == 2 );
}
