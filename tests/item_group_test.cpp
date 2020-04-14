#include "catch/catch.hpp"
#include "item.h"
#include "item_group.h"

TEST_CASE( "spawn with default charges and with ammo", "[item_group]" )
{
    Item_modifier default_charges;
    default_charges.with_ammo = 100;
    SECTION( "tools without ammo" ) {
        item matches( "matches" );
        REQUIRE( matches.ammo_default() == "NULL" );
        default_charges.modify( matches );
        CHECK( matches.ammo_remaining() == matches.ammo_capacity() );
    }

    SECTION( "gun with ammo type" ) {
        item glock( "glock_19" );
        REQUIRE( glock.ammo_default() != "NULL" );
        default_charges.modify( glock );
        CHECK( glock.ammo_remaining() == glock.ammo_capacity() );
    }
}

TEST_CASE( "Item_modifier damages item", "[item_group]" )
{
    Item_modifier damaged;
    damaged.damage.first = 1;
    damaged.damage.second = 1;
    SECTION( "except when it's an ammunition" ) {
        item rock( "rock" );
        REQUIRE( rock.damage() == 0 );
        REQUIRE( rock.max_damage() == 0 );
        damaged.modify( rock );
        CHECK( rock.damage() == 0 );
    }
    SECTION( "when it can be damaged" ) {
        item glock( "glock_19" );
        REQUIRE( glock.damage() == 0 );
        REQUIRE( glock.max_damage() > 0 );
        damaged.modify( glock );
        CHECK( glock.damage() == 1 );
    }
}

TEST_CASE( "Item_modifier gun fouling", "[item_group]" )
{
    Item_modifier fouled;
    fouled.dirt.first = 1;
    SECTION( "guns can be fouled" ) {
        item glock( "glock_19" );
        REQUIRE( !glock.has_flag( "PRIMITIVE_RANGED_WEAPON" ) );
        REQUIRE( !glock.has_var( "dirt" ) );
        fouled.modify( glock );
        CHECK( glock.get_var( "dirt", 0.0 ) > 0.0 );
    }
    SECTION( "bows can't be fouled" ) {
        item bow( "longbow" );
        REQUIRE( !bow.has_var( "dirt" ) );
        REQUIRE( bow.has_flag( "PRIMITIVE_RANGED_WEAPON" ) );
        fouled.modify( bow );
        CHECK( !bow.has_var( "dirt" ) );
    }
}
