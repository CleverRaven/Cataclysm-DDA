#include "catch/catch.hpp"
#include "calendar.h"
#include "inventory.h"
#include "item.h"

TEST_CASE( "visitable_summation" )
{
    inventory test_inv;

    item bottle_of_water( "bottle_plastic", calendar::turn );
    item water_in_bottle( "water", calendar::turn );
    water_in_bottle.charges = bottle_of_water.get_remaining_capacity_for_liquid( water_in_bottle );
    bottle_of_water.put_in( water_in_bottle );
    test_inv.add_item( bottle_of_water );

    const item unlimited_water( "water", 0, item::INFINITE_CHARGES );
    test_inv.add_item( unlimited_water );

    CHECK( test_inv.charges_of( "water", item::INFINITE_CHARGES ) > 1 );
}
