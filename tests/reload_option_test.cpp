#include <list>
#include <memory>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"

static const flag_id json_flag_RELOAD_ONE( "RELOAD_ONE" );

TEST_CASE( "revolver_reload_option", "[reload],[reload_option],[gun]" )
{
    avatar dummy;
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    item_location gun = dummy.i_add( item( "sw_619", calendar::turn_zero, 0 ) );
    const ammotype &gun_ammo_type = item::find_type( gun->ammo_default() )->ammo->type;
    item_location ammo = dummy.i_add( item( "38_special", calendar::turn_zero,
                                            gun->ammo_capacity( gun_ammo_type ) ) );
    REQUIRE( gun->has_flag( json_flag_RELOAD_ONE ) );
    REQUIRE( gun->ammo_remaining() == 0 );

    const item::reload_option gun_option( &dummy, gun, ammo );
    REQUIRE( gun_option.qty() == 1 );

    item_location speedloader = dummy.i_add( item( "38_speedloader", calendar::turn_zero, 0 ) );
    REQUIRE( speedloader->ammo_remaining() == 0 );

    const item::reload_option speedloader_option( &dummy, speedloader, ammo );
    CHECK( speedloader_option.qty() == speedloader->ammo_capacity( gun_ammo_type ) );

    speedloader->put_in( *ammo, pocket_type::MAGAZINE );
    const item::reload_option gun_speedloader_option( &dummy, gun, speedloader );
    CHECK( gun_speedloader_option.qty() == speedloader->ammo_capacity( gun_ammo_type ) );
}

TEST_CASE( "magazine_reload_option", "[reload],[reload_option],[gun]" )
{
    avatar dummy;
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    item_location magazine = dummy.i_add( item( "glockmag", calendar::turn_zero, 0 ) );
    const ammotype &mag_ammo_type = item::find_type( magazine->ammo_default() )->ammo->type;
    item_location ammo = dummy.i_add( item( "9mm", calendar::turn_zero,
                                            magazine->ammo_capacity( mag_ammo_type ) ) );

    const item::reload_option magazine_option( &dummy, magazine, ammo );
    CHECK( magazine_option.qty() == magazine->ammo_capacity( mag_ammo_type ) );

    magazine->put_in( *ammo, pocket_type::MAGAZINE );
    item_location gun = dummy.i_add( item( "glock_19", calendar::turn_zero, 0 ) );
    const item::reload_option gun_option( &dummy, gun, magazine );
    CHECK( gun_option.qty() == 1 );
}

TEST_CASE( "belt_reload_option", "[reload],[reload_option],[gun]" )
{
    avatar dummy;
    dummy.set_body();
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    item_location belt = dummy.i_add( item( "belt308", calendar::turn_zero, 0 ) );
    const ammotype &belt_ammo_type = item::find_type( belt->ammo_default() )->ammo->type;
    item_location ammo = dummy.i_add( item( "308", calendar::turn_zero,
                                            belt->ammo_capacity( belt_ammo_type ) ) );
    dummy.i_add( item( "ammolink308", calendar::turn_zero, belt->ammo_capacity( belt_ammo_type ) ) );
    // Belt is populated with "charges" rounds by the item constructor.
    belt->ammo_unset();

    REQUIRE( belt->ammo_remaining() == 0 );
    const item::reload_option belt_option( &dummy, belt, ammo );
    CHECK( belt_option.qty() == belt->ammo_capacity( belt_ammo_type ) );

    belt->put_in( *ammo, pocket_type::MAGAZINE );
    item_location gun = dummy.i_add( item( "m134", calendar::turn_zero, 0 ) );

    const item::reload_option gun_option( &dummy, gun, belt );

    CHECK( gun_option.qty() == 1 );
}

TEST_CASE( "canteen_reload_option", "[reload],[reload_option],[liquid]" )
{
    avatar dummy;
    dummy.worn.wear_item( dummy, item( "backpack" ), false, false );

    item plastic_bottle( "bottle_plastic" );
    item_location bottle = dummy.i_add( plastic_bottle );
    item water( "water_clean", calendar::turn_zero, 2 );
    // add an extra bottle with water
    item_location water_bottle = dummy.i_add( plastic_bottle );
    water_bottle->put_in( water, pocket_type::CONTAINER );

    const item::reload_option bottle_option( &dummy, bottle, water_bottle );
    CHECK( bottle_option.qty() == bottle->get_remaining_capacity_for_liquid( water, true ) );

    item_location canteen = dummy.i_add( item( "2lcanteen" ) );

    const item::reload_option canteen_option( &dummy, canteen, water_bottle );

    CHECK( canteen_option.qty() == 2 );
}

