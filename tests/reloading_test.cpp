#include <limits.h>
#include <list>
#include <memory>
#include <set>
#include <string>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "player.h"
#include "player_helpers.h"
#include "calendar.h"
#include "inventory.h"
#include "optional.h"
#include "player_activity.h"
#include "type_id.h"

TEST_CASE( "reload_integral_magazine_of gun", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &ammo = dummy.i_add( item( "762_54R", 0, item::default_charges_tag{} ) );
    item &gun = dummy.i_add( item( "mosin91_30", 0, item::default_charges_tag{} ) );
    int ammo_pos = dummy.inv.position_by_item( &ammo );

    REQUIRE( ammo_pos != INT_MIN );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( !gun.integral_magazines().empty() );
    REQUIRE( gun.magazine_current()->is_reloadable_with( ammo.type->get_id() ) );

    bool success = gun.magazine_current()->reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( success );
    REQUIRE( gun.ammo_remaining() == gun.ammo_capacity() );
}

TEST_CASE( "reload_gun_with_integral_magazine", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &ammo = dummy.i_add( item( "762_54R", 0, item::default_charges_tag{} ) );
    item &gun = dummy.i_add( item( "mosin91_30", 0, item::default_charges_tag{} ) );
    int ammo_pos = dummy.inv.position_by_item( &ammo );

    REQUIRE( ammo_pos != INT_MIN );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( !gun.integral_magazines().empty() );
    REQUIRE( !gun.is_reloadable_with( ammo.type->get_id() ) );

    bool success = gun.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    // The gun item itself should not be reloadable
    REQUIRE( !success );
    REQUIRE( gun.ammo_remaining() == 0 );
}

TEST_CASE( "reload_integral_magazine_of_gun_using_speedloader", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &ammo = dummy.i_add( item( "762_54R", 0, item::default_charges_tag{} ) );
    item &speedloader = dummy.i_add( item( "762R_clip", 0, false ) );
    int loader_pos = dummy.inv.position_by_item( &speedloader );
    item &gun = dummy.i_add( item( "mosin91_30", 0, false ) );
    int ammo_pos = dummy.inv.position_by_item( &ammo );

    REQUIRE( ammo_pos != INT_MIN );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( !gun.integral_magazines().empty() );
    REQUIRE( loader_pos != INT_MIN );
    REQUIRE( speedloader.ammo_remaining() == 0 );
    REQUIRE( speedloader.has_flag( "SPEEDLOADER" ) );
    REQUIRE( speedloader.is_reloadable_with( ammo.type->get_id() ) );

    bool speedloader_success = speedloader.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( speedloader_success );
    REQUIRE( speedloader.ammo_remaining() == speedloader.ammo_capacity() );
    REQUIRE( gun.magazine_current()->is_reloadable_with( speedloader.type->get_id() ) );

    bool success = gun.magazine_current()->reload( dummy, item_location( dummy, &speedloader ),
                   speedloader.ammo_remaining() );

    REQUIRE( success );
    REQUIRE( gun.ammo_remaining() == gun.ammo_capacity() );
    // Speedloader should be empty and still in inventory.
    REQUIRE( dummy.inv.position_by_item( &speedloader ) != INT_MIN );
    REQUIRE( speedloader.ammo_remaining() == 0 );
}

TEST_CASE( "reload_gun_with_integral_magazine_using_speedloader", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &ammo = dummy.i_add( item( "762_54R", 0, item::default_charges_tag{} ) );
    item &speedloader = dummy.i_add( item( "762R_clip", 0, false ) );
    int loader_pos = dummy.inv.position_by_item( &speedloader );
    item &gun = dummy.i_add( item( "mosin91_30", 0, false ) );
    int ammo_pos = dummy.inv.position_by_item( &ammo );

    REQUIRE( ammo_pos != INT_MIN );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( !gun.integral_magazines().empty() );
    REQUIRE( loader_pos != INT_MIN );
    REQUIRE( speedloader.ammo_remaining() == 0 );
    REQUIRE( speedloader.has_flag( "SPEEDLOADER" ) );
    REQUIRE( speedloader.is_reloadable_with( ammo.type->get_id() ) );

    bool speedloader_success = speedloader.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( speedloader_success );
    REQUIRE( speedloader.ammo_remaining() == speedloader.ammo_capacity() );
    REQUIRE( !gun.is_reloadable_with( speedloader.type->get_id() ) );

    bool success = gun.reload( dummy, item_location( dummy, &speedloader ),
                               speedloader.ammo_remaining() );

    // The gun item itself should not be reloadable
    REQUIRE( !success );
    REQUIRE( gun.ammo_remaining() == 0 );
    // Speedloader should be loaded and still in inventory.
    REQUIRE( dummy.inv.position_by_item( &speedloader ) != INT_MIN );
}

TEST_CASE( "reload_gun_with_empty_detachable_magazine", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &mag = dummy.i_add( item( "glockmag", 0, false ) );
    item &gun = dummy.i_add( item( "glock_19", 0, false ) );
    int mag_pos = dummy.inv.position_by_item( &mag );
    int gun_pos = dummy.inv.position_by_item( &gun );
    REQUIRE( gun.magazine_compatible().count( mag.type->get_id() ) != 0 );
    REQUIRE( mag_pos != INT_MIN );
    REQUIRE( gun_pos != INT_MIN );

    REQUIRE( mag.ammo_remaining() == 0 );
    REQUIRE( gun.is_reloadable_with( mag.type->get_id() ) );

    bool success = gun.reload( dummy, item_location( dummy, &mag ), 1 );

    // The gun should be loaded with an empty magazine
    REQUIRE( success );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.contents.front().type->get_id() == mag.type->get_id() );

    // The magazine should no longer be in the player's inventory
    int magazine_pos = dummy.inv.position_by_type( "glockmag" );
    REQUIRE( magazine_pos == INT_MIN );
}

TEST_CASE( "reload_gun_with_loaded_detachable_magazine", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &mag = dummy.i_add( item( "glockmag", 0, false ) );
    item &gun = dummy.i_add( item( "glock_19", 0, false ) );
    item &ammo = dummy.i_add( item( "9mm", 0, item::default_charges_tag{} ) );
    int mag_pos = dummy.inv.position_by_item( &mag );
    int gun_pos = dummy.inv.position_by_item( &gun );
    int ammo_pos = dummy.inv.position_by_item( &ammo );
    REQUIRE( mag.type->magazine->type.count( ammo.type->ammo->type ) != 0 );
    REQUIRE( gun.type->gun->ammo.count( ammo.type->ammo->type ) != 0 );
    REQUIRE( gun.magazine_compatible().count( mag.type->get_id() ) != 0 );
    REQUIRE( mag_pos != INT_MIN );
    REQUIRE( gun_pos != INT_MIN );
    REQUIRE( ammo_pos != INT_MIN );

    REQUIRE( mag.ammo_remaining() == 0 );
    REQUIRE( mag.is_reloadable_with( ammo.type->get_id() ) );

    bool mag_success = mag.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    // The magazine should be loaded to full capacity
    REQUIRE( mag_success );
    REQUIRE( mag.ammo_remaining() == mag.ammo_capacity() );
    REQUIRE( mag.contents.front().type->get_id() == ammo.type->get_id() );

    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.is_reloadable_with( mag.type->get_id() ) );

    bool gun_success = gun.reload( dummy, item_location( dummy, &mag ), 1 );

    // The gun should be loaded with a full magazine
    REQUIRE( gun_success );
    REQUIRE( gun.ammo_remaining() == gun.ammo_capacity() );
    REQUIRE( gun.contents.front().type->get_id() == mag.type->get_id() );

    // The magazine should no longer be in the player's inventory
    mag_pos = dummy.inv.position_by_type( "glockmag" );
    REQUIRE( mag_pos == INT_MIN );
}

static void reload_a_revolver( player &dummy, item &gun, item &ammo )
{
    while( gun.ammo_remaining() < gun.ammo_capacity() ) {
        g->reload_weapon( false );
        REQUIRE( dummy.activity );
        process_activity( dummy );
        CHECK( gun.ammo_remaining() > 0 );
        CHECK( gun.ammo_current() == ammo.type->get_id() );
    }
}

TEST_CASE( "automatic_reloading_action", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    GIVEN( "an unarmed player" ) {
        REQUIRE( !dummy.is_armed() );
        WHEN( "the player triggers auto reload" ) {
            g->reload_weapon( false );
            THEN( "No activity is generated" ) {
                CHECK( !dummy.activity );
            }
        }
    }

    GIVEN( "a player armed with a revolver and ammo for it" ) {
        item &ammo = dummy.i_add( item( "40sw", 0, item::default_charges_tag{} ) );
        REQUIRE( ammo.is_ammo() );

        dummy.weapon = item( "sw_610", 0, 0 );
        REQUIRE( dummy.weapon.ammo_remaining() == 0 );
        REQUIRE( dummy.weapon.magazine_current()->can_reload_with( ammo.type->get_id() ) );

        WHEN( "the player triggers auto reload until the revolver is full" ) {
            reload_a_revolver( dummy, dummy.weapon, ammo );
            WHEN( "the player triggers auto reload again" ) {
                g->reload_weapon( false );
                THEN( "no activity is generated" ) {
                    CHECK( !dummy.activity );
                }
            }
        }
        GIVEN( "the player has another gun with ammo" ) {
            item &gun2 = dummy.i_add( item( "sw_610", 0, 0 ) );
            REQUIRE( gun2.ammo_remaining() == 0 );
            REQUIRE( gun2.magazine_current()->can_reload_with( ammo.type->get_id() ) );
            WHEN( "the player triggers auto reload until the first revolver is full" ) {
                reload_a_revolver( dummy, dummy.weapon, ammo );
                WHEN( "the player triggers auto reload until the second revolver is full" ) {
                    reload_a_revolver( dummy, gun2, ammo );
                    WHEN( "the player triggers auto reload again" ) {
                        g->reload_weapon( false );
                        THEN( "no activity is generated" ) {
                            CHECK( !dummy.activity );
                        }
                    }
                }
            }
        }
    }

    GIVEN( "a player wielding an unloaded gun, carrying an unloaded magazine, and carrying ammo for the magazine" ) {
        item &ammo = dummy.i_add( item( "9mm", 0, 50 ) );
        const cata::optional<islot_ammo> &ammo_type = ammo.type->ammo;
        REQUIRE( ammo_type );

        item &mag = dummy.i_add( item( "glockmag", 0, 0 ) );
        const cata::optional<islot_magazine> &magazine_type = mag.type->magazine;
        REQUIRE( magazine_type );
        REQUIRE( magazine_type->type.count( ammo_type->type ) != 0 );
        REQUIRE( mag.ammo_remaining() == 0 );

        dummy.weapon = item( "glock_19", 0, 0 );
        REQUIRE( dummy.weapon.ammo_remaining() == 0 );

        WHEN( "the player triggers auto reload" ) {
            g->reload_weapon( false );
            REQUIRE( dummy.activity );
            process_activity( dummy );

            THEN( "the associated magazine is reloaded" ) {
                CHECK( mag.ammo_remaining() > 0 );
                CHECK( mag.contents.front().type == ammo.type );
            }
            WHEN( "the player triggers auto reload again" ) {
                g->reload_weapon( false );
                REQUIRE( dummy.activity );
                process_activity( dummy );

                THEN( "The magazine is loaded into the gun" ) {
                    CHECK( dummy.weapon.ammo_remaining() > 0 );
                }
                WHEN( "the player triggers auto reload again" ) {
                    g->reload_weapon( false );
                    THEN( "No activity is generated" ) {
                        CHECK( !dummy.activity );
                    }
                }
            }
        }
        GIVEN( "the player also has an extended magazine" ) {
            item &mag2 = dummy.i_add( item( "glockbigmag", 0, 0 ) );
            const cata::optional<islot_magazine> &magazine_type2 = mag2.type->magazine;
            REQUIRE( magazine_type2 );
            REQUIRE( magazine_type2->type.count( ammo_type->type ) != 0 );
            REQUIRE( mag2.ammo_remaining() == 0 );

            WHEN( "the player triggers auto reload" ) {
                g->reload_weapon( false );
                REQUIRE( dummy.activity );
                process_activity( dummy );

                THEN( "the associated magazine is reloaded" ) {
                    CHECK( mag.ammo_remaining() > 0 );
                    CHECK( mag.contents.front().type == ammo.type );
                }
                WHEN( "the player triggers auto reload again" ) {
                    g->reload_weapon( false );
                    REQUIRE( dummy.activity );
                    process_activity( dummy );

                    THEN( "The magazine is loaded into the gun" ) {
                        CHECK( dummy.weapon.ammo_remaining() > 0 );
                    }
                    WHEN( "the player triggers auto reload again" ) {
                        g->reload_weapon( false );
                        REQUIRE( dummy.activity );
                        process_activity( dummy );

                        THEN( "the second associated magazine is reloaded" ) {
                            CHECK( mag2.ammo_remaining() > 0 );
                            CHECK( mag2.contents.front().type == ammo.type );
                        }
                        WHEN( "the player triggers auto reload again" ) {
                            g->reload_weapon( false );
                            THEN( "No activity is generated" ) {
                                CHECK( !dummy.activity );
                            }
                        }
                    }
                }
            }
        }
    }
}
