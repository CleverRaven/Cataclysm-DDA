#include "cata_catch.h"

#include "avatar.h"
#include "calendar.h"
#include "character.h"
#include "color.h"
#include "item.h"
#include "item_location.h"
#include "player_helpers.h"
#include "type_id.h"

static const ammotype ammo_38( "38" );
static const ammotype ammo_9mm( "9mm" );

static const itype_id itype_38_special( "38_special" );
static const itype_id itype_9mm( "9mm" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_glock_19( "glock_19" );
static const itype_id itype_glockmag( "glockmag" );
static const itype_id itype_sw_619( "sw_619" );

static void prepare_avatar( avatar &u )
{
    clear_avatar();
    u.clear_worn();
    u.inv->clear();
    u.remove_weapon();
    u.wear_item( item( itype_backpack ) );
}

TEST_CASE( "full_gun_with_spare_mag_is_green", "[item][color][gun][inventory][reload]" )
{
    avatar &u = get_avatar();
    prepare_avatar( u );

    // Put a fully-loaded gun in the inventory.
    item_location gun = u.i_add( item( itype_glock_19 ) );
    item_location mag_for_gun = u.i_add( item( itype_glockmag ) );
    const int mag_cap = mag_for_gun->ammo_capacity( ammo_9mm );
    item_location ammo_for_gun = u.i_add( item( itype_9mm, calendar::turn, mag_cap ) );
    REQUIRE( mag_for_gun->reload( u, ammo_for_gun, mag_cap ) );
    REQUIRE( gun->reload( u, mag_for_gun, mag_cap ) );
    REQUIRE( gun->remaining_ammo_capacity() == 0 );

    // Player also carries a second loaded magazine.
    item_location spare_mag = u.i_add( item( itype_glockmag ) );
    item_location ammo_for_spare = u.i_add( item( itype_9mm, calendar::turn, mag_cap ) );
    REQUIRE( spare_mag->reload( u, ammo_for_spare, mag_cap ) );

    // Gun is full, but the player still possesses a loaded magazine for it,
    // so the display color should signal "equipped" (green), not "missing
    // something" (light red).
    CHECK( gun->color_in_inventory( &u ) == c_green );
}

TEST_CASE( "full_revolver_with_spare_ammo_is_green", "[item][color][gun][inventory][reload]" )
{
    avatar &u = get_avatar();
    prepare_avatar( u );

    item_location gun = u.i_add( item( itype_sw_619 ) );
    const int cylinder_cap = gun->ammo_capacity( ammo_38 );
    REQUIRE( cylinder_cap > 0 );

    // Two cylinders' worth of rounds: one to load the revolver, one to keep.
    item_location ammo =
        u.i_add( item( itype_38_special, calendar::turn, cylinder_cap * 2 ) );
    REQUIRE( gun->reload( u, ammo, cylinder_cap ) );
    REQUIRE( gun->remaining_ammo_capacity() == 0 );

    // Integral-magazine gun is full but the player still has loose rounds.
    CHECK( gun->color_in_inventory( &u ) == c_green );
}

TEST_CASE( "full_magazine_with_gun_and_spare_ammo_is_green",
           "[item][color][magazine][inventory][reload]" )
{
    avatar &u = get_avatar();
    prepare_avatar( u );

    u.i_add( item( itype_glock_19 ) );
    item_location mag = u.i_add( item( itype_glockmag ) );
    const int mag_cap = mag->ammo_capacity( ammo_9mm );
    item_location ammo = u.i_add( item( itype_9mm, calendar::turn, mag_cap * 2 ) );
    REQUIRE( mag->reload( u, ammo, mag_cap ) );
    REQUIRE( mag->remaining_ammo_capacity() == 0 );

    // Magazine is full, but the player still has a compatible gun and spare
    // loose rounds, so it should color green.
    CHECK( mag->color_in_inventory( &u ) == c_green );
}

TEST_CASE( "empty_gun_alone_is_uncolored", "[item][color][gun][inventory]" )
{
    avatar &u = get_avatar();
    prepare_avatar( u );

    item_location gun = u.i_add( item( itype_glock_19 ) );
    // No ammo, no magazine in inventory: neither green nor light red (will be default).
    const nc_color c = gun->color_in_inventory( &u );
    CHECK( c != c_green );
    CHECK( c != c_light_red );
}

TEST_CASE( "gun_with_ammo_but_no_mag_is_light_red", "[item][color][gun][inventory]" )
{
    avatar &u = get_avatar();
    prepare_avatar( u );

    item_location gun = u.i_add( item( itype_glock_19 ) );
    u.i_add( item( itype_9mm, calendar::turn, 10 ) );
    // Compatible loose ammo but no magazine -> light red.
    CHECK( gun->color_in_inventory( &u ) == c_light_red );
}
