#include <list>
#include <memory>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"

TEST_CASE( "revolver_reload_option", "[reload],[reload_option],[gun]" )
{
    avatar dummy;
    dummy.worn.emplace_back( "backpack" );

    item &gun = dummy.i_add( item( "sw_619", calendar::turn_zero, 0 ) );
    const ammotype &gun_ammo_type = item::find_type( gun.ammo_default() )->ammo->type;
    item &ammo = dummy.i_add( item( "38_special", calendar::turn_zero,
                                    gun.ammo_capacity( gun_ammo_type ) ) );
    item_location ammo_location( dummy, &ammo );
    REQUIRE( gun.has_flag( flag_id( "RELOAD_ONE" ) ) );
    REQUIRE( gun.ammo_remaining() == 0 );

    const item::reload_option gun_option( &dummy, &gun, &gun, ammo_location );
    REQUIRE( gun_option.qty() == 1 );

    ammo_location = item_location( dummy, &ammo );
    item &speedloader = dummy.i_add( item( "38_speedloader", calendar::turn_zero, 0 ) );
    REQUIRE( speedloader.ammo_remaining() == 0 );

    const item::reload_option speedloader_option( &dummy, &speedloader, &speedloader,
            ammo_location );
    CHECK( speedloader_option.qty() == speedloader.ammo_capacity( gun_ammo_type ) );

    speedloader.put_in( ammo, item_pocket::pocket_type::MAGAZINE );
    item_location speedloader_location( dummy, &speedloader );
    const item::reload_option gun_speedloader_option( &dummy, &gun, &gun,
            speedloader_location );
    CHECK( gun_speedloader_option.qty() == speedloader.ammo_capacity( gun_ammo_type ) );
}

TEST_CASE( "magazine_reload_option", "[reload],[reload_option],[gun]" )
{
    avatar dummy;
    dummy.worn.emplace_back( "backpack" );

    item &magazine = dummy.i_add( item( "glockmag", calendar::turn_zero, 0 ) );
    const ammotype &mag_ammo_type = item::find_type( magazine.ammo_default() )->ammo->type;
    item &ammo = dummy.i_add( item( "9mm", calendar::turn_zero,
                                    magazine.ammo_capacity( mag_ammo_type ) ) );
    item_location ammo_location( dummy, &ammo );

    const item::reload_option magazine_option( &dummy, &magazine, &magazine,
            ammo_location );
    CHECK( magazine_option.qty() == magazine.ammo_capacity( mag_ammo_type ) );

    magazine.put_in( ammo, item_pocket::pocket_type::MAGAZINE );
    item_location magazine_location( dummy, &magazine );
    item &gun = dummy.i_add( item( "glock_19", calendar::turn_zero, 0 ) );
    const item::reload_option gun_option( &dummy, &gun, &gun, magazine_location );
    CHECK( gun_option.qty() == 1 );
}

TEST_CASE( "belt_reload_option", "[reload],[reload_option],[gun]" )
{
    avatar dummy;
    dummy.set_body();
    dummy.worn.emplace_back( "backpack" );

    item &belt = dummy.i_add( item( "belt308", calendar::turn_zero, 0 ) );
    const ammotype &belt_ammo_type = item::find_type( belt.ammo_default() )->ammo->type;
    item &ammo = dummy.i_add( item( "308", calendar::turn_zero,
                                    belt.ammo_capacity( belt_ammo_type ) ) );
    dummy.i_add( item( "ammolink308", calendar::turn_zero, belt.ammo_capacity( belt_ammo_type ) ) );
    item_location ammo_location( dummy, &ammo );
    // Belt is populated with "charges" rounds by the item constructor.
    belt.ammo_unset();

    REQUIRE( belt.ammo_remaining() == 0 );
    const item::reload_option belt_option( &dummy, &belt, &belt, ammo_location );
    CHECK( belt_option.qty() == belt.ammo_capacity( belt_ammo_type ) );

    belt.put_in( ammo, item_pocket::pocket_type::MAGAZINE );
    item_location belt_location( dummy, &ammo );
    item &gun = dummy.i_add( item( "m134", calendar::turn_zero, 0 ) );

    const item::reload_option gun_option( &dummy, &gun, &gun, belt_location );

    CHECK( gun_option.qty() == 1 );
}

TEST_CASE( "canteen_reload_option", "[reload],[reload_option],[liquid]" )
{
    avatar dummy;
    dummy.worn.emplace_back( "backpack" );

    item &bottle = dummy.i_add( item( "bottle_plastic" ) );
    item water( "water_clean", calendar::turn_zero, 2 );
    // add an extra bottle with water
    item_location water_bottle( dummy, &dummy.i_add( bottle ) );
    water_bottle->put_in( water, item_pocket::pocket_type::CONTAINER );

    const item::reload_option bottle_option( &dummy, &bottle, &bottle, water_bottle );
    CHECK( bottle_option.qty() == bottle.get_remaining_capacity_for_liquid( water, true ) );

    item &canteen = dummy.i_add( item( "2lcanteen" ) );

    const item::reload_option canteen_option( &dummy, &canteen, &canteen,
            water_bottle );

    CHECK( canteen_option.qty() == 2 );
}

