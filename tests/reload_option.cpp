#include "catch/catch.hpp"

#include "item.h"
#include "item_location.h"
#include "player.h"

TEST_CASE( "revolver_reload_option", "[reload],[reload_option],[gun]" )
{
    player dummy;

    item &gun = dummy.i_add( item( "sw_619", 0, 0 ) );
    item &ammo = dummy.i_add( item( "38_special", 0, gun.ammo_capacity() ) );
    item_location ammo_location( dummy, &ammo );
    REQUIRE( gun.has_flag( "RELOAD_ONE" ) );
    REQUIRE( gun.ammo_remaining() == 0 );

    item::reload_option gun_option( &dummy, &gun, &gun, std::move( ammo_location ) );
    REQUIRE( gun_option.qty() == 1 );

    ammo_location = item_location( dummy, &ammo );
    item &speedloader = dummy.i_add( item( "38_speedloader", 0, 0 ) );
    REQUIRE( speedloader.ammo_remaining() == 0 );

    item::reload_option speedloader_option( &dummy, &speedloader, &speedloader,
                                            std::move( ammo_location ) );
    CHECK( speedloader_option.qty() == speedloader.ammo_capacity() );

    speedloader.contents.push_back( ammo );
    item_location speedloader_location( dummy, &speedloader );
    item::reload_option gun_speedloader_option( &dummy, &gun, &gun, std::move( speedloader_location ) );
    CHECK( gun_speedloader_option.qty() == speedloader.ammo_capacity() );
}

TEST_CASE( "magazine_reload_option", "[reload],[reload_option],[gun]" )
{
    player dummy;

    item &magazine = dummy.i_add( item( "glockmag", 0, 0 ) );
    item &ammo = dummy.i_add( item( "9mm", 0, magazine.ammo_capacity() ) );
    item_location ammo_location( dummy, &ammo );

    item::reload_option magazine_option( &dummy, &magazine, &magazine, std::move( ammo_location ) );
    CHECK( magazine_option.qty() == magazine.ammo_capacity() );

    magazine.contents.push_back( ammo );
    item_location magazine_location( dummy, &magazine );
    item &gun = dummy.i_add( item( "glock_19", 0, 0 ) );
    item::reload_option gun_option( &dummy, &gun, &gun, std::move( magazine_location ) );
    CHECK( gun_option.qty() == 1 );
}

TEST_CASE( "belt_reload_option", "[reload],[reload_option],[gun]" )
{
    player dummy;

    item &belt = dummy.i_add( item( "belt308", 0, 0 ) );
    item &ammo = dummy.i_add( item( "308", 0, belt.ammo_capacity() ) );
    item &links = dummy.i_add( item( "ammolink308", 0, belt.ammo_capacity() ) );
    item_location ammo_location( dummy, &ammo );
    // Belt is populated with "charges" rounds by the item constructor.
    belt.ammo_unset();

    REQUIRE( belt.ammo_remaining() == 0 );
    item::reload_option belt_option( &dummy, &belt, &belt, std::move( ammo_location ) );
    CHECK( belt_option.qty() == belt.ammo_capacity() );

    belt.contents.push_back( ammo );
    item_location belt_location( dummy, &ammo );
    item &gun = dummy.i_add( item( "m134", 0, 0 ) );

    item::reload_option gun_option( &dummy, &gun, &gun, std::move( belt_location ) );

    CHECK( gun_option.qty() == 1 );
}

TEST_CASE( "canteen_reload_option", "[reload],[reload_option],[liquid]" )
{
    player dummy;

    item &water = dummy.i_add( item( "water_clean", 0, 2 ) );
    item &bottle = dummy.i_add( item( "bottle_plastic" ) );
    item_location water_location( dummy, &water );

    item::reload_option bottle_option( &dummy, &bottle, &bottle, std::move( water_location ) );
    CHECK( bottle_option.qty() == bottle.get_remaining_capacity_for_liquid( water, true ) );

    // Add water to bottle?
    bottle.fill_with( water, 2 );
    item &canteen = dummy.i_add( item( "2lcanteen" ) );
    item_location bottle_location( dummy, &bottle );

    item::reload_option canteen_option( &dummy, &canteen, &canteen, std::move( bottle_location ) );

    CHECK( canteen_option.qty() == 2 );
}
