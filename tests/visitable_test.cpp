#include "cata_catch.h"

#include "calendar.h"
#include "inventory.h"
#include "item.h"
#include "item_pocket.h"
#include "ret_val.h"
#include "type_id.h"

TEST_CASE( "visitable_summation" )
{
    inventory test_inv;

    item bottle_of_water( "bottle_plastic", calendar::turn );
    item water_in_bottle( "water", calendar::turn );
    water_in_bottle.charges = bottle_of_water.get_remaining_capacity_for_liquid( water_in_bottle );
    bottle_of_water.put_in( water_in_bottle, item_pocket::pocket_type::CONTAINER );
    test_inv.add_item( bottle_of_water );

    const item unlimited_water( "water", calendar::turn_zero, item::INFINITE_CHARGES );
    test_inv.add_item( unlimited_water );

    CHECK( test_inv.charges_of( itype_id( "water" ), item::INFINITE_CHARGES ) > 1 );
}
