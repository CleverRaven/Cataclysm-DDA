#include "../src/temp_crafting_inventory.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "type_id.h"

static const itype_id itype_test_gum( "test_gum" );

static const quality_id qual_AXE( "AXE" );
static const quality_id qual_DIG( "DIG" );
static const quality_id qual_HAMMER( "HAMMER" );
static const quality_id qual_PRY( "PRY" );

TEST_CASE( "temp_crafting_inv_test_amount", "[crafting][inventory]" )
{
    temp_crafting_inventory inv;
    CHECK( inv.size() == 0 );

    item gum( "test_gum", calendar::turn_zero, item::default_charges_tag{} );

    inv.add_item_ref( gum );
    CHECK( inv.size() == 1 );

    CHECK( inv.amount_of( itype_test_gum ) == 1 );
    CHECK( inv.has_amount( itype_test_gum, 1 ) );
    CHECK( inv.has_charges( itype_test_gum, 10 ) );
    CHECK_FALSE( inv.has_charges( itype_test_gum, 11 ) );

    inv.clear();
    CHECK( inv.size() == 0 );
}

TEST_CASE( "temp_crafting_inv_test_quality", "[crafting][inventory]" )
{
    temp_crafting_inventory inv;
    inv.add_item_copy( item( "test_halligan" ) );

    CHECK( inv.has_quality( qual_HAMMER, 1 ) );
    CHECK( inv.has_quality( qual_HAMMER, 2 ) );
    CHECK_FALSE( inv.has_quality( qual_HAMMER, 3 ) );
    CHECK( inv.has_quality( qual_DIG, 1 ) );
    CHECK_FALSE( inv.has_quality( qual_AXE ) );

    inv.add_item_copy( item( "test_fire_ax" ) );
    CHECK( inv.has_quality( qual_AXE ) );

    CHECK( inv.max_quality( qual_PRY ) == 4 );
}
