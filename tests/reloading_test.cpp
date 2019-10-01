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

TEST_CASE( "reload_gun_with_integral_magazine", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

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

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

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

    clear_player();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &ammo = dummy.i_add( item( "9mm", 0, item::default_charges_tag{} ) );
    const cata::optional<islot_ammo> &ammo_type = ammo.type->ammo;
    REQUIRE( ammo_type );

    const item mag( "glockmag", 0, 0 );
    const cata::optional<islot_magazine> &magazine_type = mag.type->magazine;
    REQUIRE( magazine_type );
    REQUIRE( magazine_type->type.count( ammo_type->type ) != 0 );

    item &gun = dummy.i_add( item( "glock_19", 0, item::default_charges_tag{} ) );
    REQUIRE( gun.ammo_types().count( ammo_type->type ) != 0 );

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

    CHECK( gun_success );
    REQUIRE( gun.ammo_remaining() == gun.ammo_capacity() );
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
        REQUIRE( dummy.weapon.can_reload_with( ammo.type->get_id() ) );

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
            REQUIRE( gun2.can_reload_with( ammo.type->get_id() ) );
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
