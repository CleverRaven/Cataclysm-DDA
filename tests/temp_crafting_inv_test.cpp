#include "../src/temp_crafting_inventory.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "type_id.h"

TEST_CASE( "temp_crafting_inv_test_amount", "[crafting][inventory]" )
{
    temp_crafting_inventory inv;
    CHECK( inv.size() == 0 );

    item gum( "test_gum", calendar::turn_zero, item::default_charges_tag{} );

    inv.add_item_ref( gum );
    CHECK( inv.size() == 1 );

    CHECK( inv.amount_of( itype_id( "test_gum" ) ) == 1 );
    CHECK( inv.has_amount( itype_id( "test_gum" ), 1 ) );
    CHECK( inv.has_charges( itype_id( "test_gum" ), 10 ) );
    CHECK_FALSE( inv.has_charges( itype_id( "test_gum" ), 11 ) );

    inv.clear();
    CHECK( inv.size() == 0 );
}

TEST_CASE( "temp_crafting_inv_test_quality", "[crafting][inventory]" )
{
    temp_crafting_inventory inv;
    inv.add_item_copy( item( "test_halligan" ) );

    CHECK( inv.has_quality( quality_id( "HAMMER" ), 1 ) );
    CHECK( inv.has_quality( quality_id( "HAMMER" ), 2 ) );
    CHECK_FALSE( inv.has_quality( quality_id( "HAMMER" ), 3 ) );
    CHECK( inv.has_quality( quality_id( "DIG" ), 1 ) );
    CHECK_FALSE( inv.has_quality( quality_id( "AXE" ) ) );

    inv.add_item_copy( item( "test_fire_ax" ) );
    CHECK( inv.has_quality( quality_id( "AXE" ) ) );

    CHECK( inv.max_quality( quality_id( "PRY" ) ) == 4 );
}
