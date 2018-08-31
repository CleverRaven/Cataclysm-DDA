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

    item &ammo = dummy.i_add( item( "40sw", 0, item::default_charges_tag{} ) );
    item &gun = dummy.i_add( item( "sw_610", 0, item::default_charges_tag{} ) );
    int ammo_pos = dummy.inv.position_by_item( &ammo );

    REQUIRE( ammo_pos != INT_MIN );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.magazine_integral() );

    bool success = gun.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( success );
    REQUIRE( gun.ammo_remaining() == gun.ammo_capacity() );
}

TEST_CASE( "reload_gun_with_integral_magazine_using_speedloader", "[reload],[gun]" )
{
    player &dummy = g->u;

    // Remove first worn item until there are none left.
    std::list<item> temp;
    while( dummy.takeoff( dummy.i_at( -2 ), &temp ) );

    dummy.remove_weapon();

    item &ammo = dummy.i_add( item( "38_special", 0, item::default_charges_tag{} ) );
    item &speedloader = dummy.i_add( item( "38_speedloader", 0, false ) );
    int loader_pos = dummy.inv.position_by_item( &speedloader );
    item &gun = dummy.i_add( item( "sw_619", 0, false ) );
    int ammo_pos = dummy.inv.position_by_item( &ammo );

    REQUIRE( ammo_pos != INT_MIN );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.magazine_integral() );
    REQUIRE( loader_pos != INT_MIN );
    REQUIRE( speedloader.ammo_remaining() == 0 );
    REQUIRE( speedloader.has_flag( "SPEEDLOADER" ) );

    bool speedloader_success = speedloader.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( speedloader_success );
    REQUIRE( speedloader.ammo_remaining() == speedloader.ammo_capacity() );

    bool success = gun.reload( dummy, item_location( dummy, &speedloader ),
                               speedloader.ammo_remaining() );

    REQUIRE( success );
    REQUIRE( gun.ammo_remaining() == gun.ammo_capacity() );
    // Speedloader is still in inventory.
    REQUIRE( dummy.inv.position_by_item( &speedloader ) != INT_MIN );
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
    const cata::optional<islot_ammo> &ammo_type = ammo.type->ammo;
    REQUIRE( ammo_type );

    item mag( "glockmag", 0, 0 );
    const cata::optional<islot_magazine> &magazine_type = mag.type->magazine;
    REQUIRE( magazine_type );
    REQUIRE( ammo_type->type.count( magazine_type->type ) != 0 );

    item &gun = dummy.i_add( item( "glock_19", 0, item::default_charges_tag{} ) );
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
