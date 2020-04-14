#include "catch/catch.hpp"
#include "item.h"
#include "item_group.h"

TEST_CASE( "spawn tool with default charges and ammo setting" )
{
    Item_modifier default_charges;
    default_charges.with_ammo = 100;
    item matches( "matches" );
    matches.charges = 0;
    default_charges.modify( matches );
}
