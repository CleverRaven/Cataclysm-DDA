#include <climits>
#include <memory>
#include <set>
#include <string>

#include "avatar.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "itype.h"
#include "player.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "type_id.h"
#include "value_ptr.h"

TEST_CASE( "reload_gun_with_integral_magazine", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &ammo = dummy.i_add( item( "40sw", 0, item::default_charges_tag{} ) );
    item &gun = dummy.i_add( item( "sw_610", 0, item::default_charges_tag{} ) );

    REQUIRE( dummy.has_item( ammo ) );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.magazine_integral() );

    bool success = gun.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( success );
    REQUIRE( gun.remaining_ammo_capacity() == 0 );
}

TEST_CASE( "reload_gun_with_integral_magazine_using_speedloader", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &ammo = dummy.i_add( item( "38_special", 0, item::default_charges_tag{} ) );
    item &speedloader = dummy.i_add( item( "38_speedloader", 0, false ) );
    item &gun = dummy.i_add( item( "sw_619", 0, false ) );

    REQUIRE( dummy.has_item( ammo ) );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.magazine_integral() );
    REQUIRE( dummy.has_item( speedloader ) );
    REQUIRE( speedloader.ammo_remaining() == 0 );
    REQUIRE( speedloader.has_flag( "SPEEDLOADER" ) );

    bool speedloader_success = speedloader.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( speedloader_success );
    REQUIRE( speedloader.remaining_ammo_capacity() == 0 );

    bool success = gun.reload( dummy, item_location( dummy, &speedloader ),
                               speedloader.ammo_remaining() );

    REQUIRE( success );
    REQUIRE( gun.remaining_ammo_capacity() == 0 );
    // Speedloader is still in inventory.
    REQUIRE( dummy.has_item( speedloader ) );
}

TEST_CASE( "reload_gun_with_swappable_magazine", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", 0 ) );

    item &ammo = dummy.i_add( item( "9mm", 0, item::default_charges_tag{} ) );
    const cata::value_ptr<islot_ammo> &ammo_type = ammo.type->ammo;
    REQUIRE( ammo_type );

    const item mag( "glockmag", 0, 0 );
    const cata::value_ptr<islot_magazine> &magazine_type = mag.type->magazine;
    REQUIRE( magazine_type );
    REQUIRE( magazine_type->type.count( ammo_type->type ) != 0 );

    item gun( "glock_19" );
    gun.put_in( mag, item_pocket::pocket_type::MAGAZINE_WELL );
    REQUIRE( gun.magazine_current() != nullptr );
    REQUIRE( gun.magazine_current()->ammo_types().count( ammo_type->type ) != 0 );
    dummy.i_add( gun );

    const std::vector<item *> guns = dummy.items_with( []( const item & it ) {
        return it.typeId() == itype_id( "glock_19" );
    } );
    REQUIRE( guns.size() == 1 );
    item &glock = *guns.front();
    REQUIRE( glock.magazine_current() != nullptr );
    // We're expecting the magazine to end up in the inventory.
    item_location glock_loc( dummy, &glock );
    REQUIRE( g->unload( glock_loc ) );
    const std::vector<item *> glock_mags = dummy.items_with( []( const item & it ) {
        return it.typeId() == itype_id( "glockmag" );
    } );
    REQUIRE( glock_mags.size() == 1 );
    item &magazine = *glock_mags.front();
    REQUIRE( magazine.ammo_remaining() == 0 );

    REQUIRE( dummy.has_item( ammo ) );

    bool magazine_success = magazine.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( magazine_success );
    REQUIRE( magazine.remaining_ammo_capacity() == 0 );

    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.magazine_integral() == false );

    bool gun_success = gun.reload( dummy, item_location( dummy, &magazine ), 1 );

    CHECK( gun_success );
    REQUIRE( gun.remaining_ammo_capacity() == 0 );
}

static void reload_a_revolver( player &dummy, item &gun, item &ammo )
{
    if( !dummy.is_wielding( gun ) ) {
        if( dummy.has_weapon() ) {
            // to avoid dispose_option in player::unwield()
            dummy.i_add( dummy.weapon );
            dummy.remove_weapon();
        }
        dummy.wield( gun );
    }
    while( dummy.weapon.remaining_ammo_capacity() > 0 ) {
        g->reload_weapon( false );
        REQUIRE( dummy.activity );
        process_activity( dummy );
        CAPTURE( dummy.weapon.typeId() );
        CAPTURE( ammo.typeId() );
        CHECK( !dummy.weapon.contents.empty() );
        CHECK( dummy.weapon.ammo_current() == ammo.type->get_id() );
    }
}

TEST_CASE( "automatic_reloading_action", "[reload],[gun]" )
{
    player &dummy = g->u;

    clear_avatar();
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
        item &ammo = dummy.i_add( item( "40sw", 0, 100 ) );
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
        dummy.worn.clear();
        dummy.worn.push_back( item( "backpack" ) );
        item &ammo = dummy.i_add( item( "9mm", 0, 50 ) );
        const cata::value_ptr<islot_ammo> &ammo_type = ammo.type->ammo;
        REQUIRE( ammo_type );

        item &mag = dummy.i_add( item( "glockmag", 0, 0 ) );
        const cata::value_ptr<islot_magazine> &magazine_type = mag.type->magazine;
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
                const std::vector<item *> mags = dummy.items_with( []( const item & it ) {
                    return it.typeId() == itype_id( "glockmag" );
                } );
                REQUIRE( mags.size() == 1 );
                REQUIRE( !mags.front()->contents.empty() );
                CHECK( mags.front()->contents.first_ammo().type == ammo.type );
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
            const cata::value_ptr<islot_magazine> &magazine_type2 = mag2.type->magazine;
            REQUIRE( magazine_type2 );
            REQUIRE( magazine_type2->type.count( ammo_type->type ) != 0 );
            REQUIRE( mag2.ammo_remaining() == 0 );

            WHEN( "the player triggers auto reload" ) {
                g->reload_weapon( false );
                REQUIRE( dummy.activity );
                process_activity( dummy );

                THEN( "the associated magazine is reloaded" ) {
                    const std::vector<item *> mags = dummy.items_with( []( const item & it ) {
                        return it.typeId() == itype_id( "glockmag" );
                    } );
                    REQUIRE( mags.size() == 1 );
                    REQUIRE( !mags.front()->contents.empty() );
                    CHECK( mags.front()->contents.first_ammo().type == ammo.type );
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
                            const std::vector<item *> mags = dummy.items_with( []( const item & it ) {
                                return it.typeId() == itype_id( "glockbigmag" );
                            } );
                            REQUIRE( mags.size() == 1 );
                            REQUIRE( !mags.front()->contents.empty() );
                            CHECK( mags.front()->contents.first_ammo().type == ammo.type );
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
