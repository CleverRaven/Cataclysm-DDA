#include "catch/catch.hpp"

#include "game.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "player.h"

TEST_CASE( "reload_gun_with_integral_magazine", "[reload],[gun]" )
{
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );

    dummy.remove_weapon();

    // TODO: inline the gun and ammo definitions so this test doesn't rely on json.
    item &ammo = dummy.i_add( item( "40sw", 0, item::default_charges_tag{} ) );
    item &gun = dummy.i_add( item( "sw_610", 0, false ) );
    int ammo_pos = dummy.inv.position_by_item( &ammo );

    REQUIRE( ammo_pos != INT_MIN );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.magazine_integral() );

    bool success = gun.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( success );
    REQUIRE( gun.ammo_remaining() == gun.ammo_capacity() );
}

TEST_CASE( "reload_gun_with_swappable_magazine", "[reload],[gun]" )
{
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    dummy.remove_weapon();

    item &ammo = dummy.i_add( item( "9mm", 0, item::default_charges_tag{} ) );
    const islot_ammo *ammo_type = ammo.type->ammo.get();
    REQUIRE( ammo_type != nullptr );

    item mag( "glockmag", 0, 0 );
    const islot_magazine *magazine_type = mag.type->magazine.get();
    REQUIRE( magazine_type != nullptr );
    REQUIRE( ammo_type->type.count( magazine_type->type ) != 0 );

    item &gun = dummy.i_add( item( "glock_19", 0, false ) );
    REQUIRE( ammo_type->type.count( gun.ammo_type() ) != 0 );

    gun.put_in( mag );

    int gun_pos = dummy.inv.position_by_type( "glock_19" );
    REQUIRE( gun_pos != INT_MIN );
    // We're expecting the magazine to end up in the inventory.
    g->unload( gun_pos );
    int magazine_pos = dummy.inv.position_by_type( "glockmag" );
    REQUIRE( magazine_pos != INT_MIN );
    item &magazine = dummy.inv.find_item( magazine_pos );
    REQUIRE( magazine.ammo_remaining() == 0 );

    int ammo_pos = dummy.inv.position_by_item( &ammo );
    REQUIRE( ammo_pos != INT_MIN );

    bool magazine_success = magazine.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( magazine_success );
    REQUIRE( magazine.ammo_remaining() == magazine.ammo_capacity() );

    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.magazine_integral() == false );

    bool gun_success = gun.reload( dummy, item_location( dummy, &magazine ), 1 );

    REQUIRE( gun.ammo_remaining() == gun.ammo_capacity() );
}
