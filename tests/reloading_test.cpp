#include <functional>
#include <list>
#include <memory>
#include <set>
#include <vector>

#include "avatar.h"
#include "calendar.h"
#include "cata_catch.h"
#include "game.h"
#include "item.h"
#include "item_contents.h"
#include "item_location.h"
#include "item_pocket.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "player.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"

TEST_CASE( "reload_gun_with_integral_magazine", "[reload],[gun]" )
{
    player &dummy = get_avatar();

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", calendar::turn_zero ) );

    item &ammo = dummy.i_add( item( "40sw", calendar::turn_zero, item::default_charges_tag{} ) );
    item &gun = dummy.i_add( item( "sw_610", calendar::turn_zero, item::default_charges_tag{} ) );

    REQUIRE( dummy.has_item( ammo ) );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.magazine_integral() );

    bool success = gun.reload( dummy, item_location( dummy, &ammo ), ammo.charges );

    REQUIRE( success );
    REQUIRE( gun.remaining_ammo_capacity() == 0 );
}

TEST_CASE( "reload_gun_with_integral_magazine_using_speedloader", "[reload],[gun]" )
{
    player &dummy = get_avatar();

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", calendar::turn_zero ) );

    item &ammo = dummy.i_add( item( "38_special", calendar::turn_zero,
                                    item::default_charges_tag{} ) );
    item &speedloader = dummy.i_add( item( "38_speedloader", calendar::turn_zero, false ) );
    item &gun = dummy.i_add( item( "sw_619", calendar::turn_zero, false ) );

    REQUIRE( dummy.has_item( ammo ) );
    REQUIRE( gun.ammo_remaining() == 0 );
    REQUIRE( gun.magazine_integral() );
    REQUIRE( dummy.has_item( speedloader ) );
    REQUIRE( speedloader.ammo_remaining() == 0 );
    REQUIRE( speedloader.has_flag( flag_id( "SPEEDLOADER" ) ) );

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
    player &dummy = get_avatar();

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", calendar::turn_zero ) );

    item &ammo = dummy.i_add( item( "9mm", calendar::turn_zero, item::default_charges_tag{} ) );
    const cata::value_ptr<islot_ammo> &ammo_type = ammo.type->ammo;
    REQUIRE( ammo_type );

    const item mag( "glockmag", calendar::turn_zero, 0 );
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
    REQUIRE( dummy.unload( glock_loc ) );
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
        CHECK( !dummy.weapon.empty() );
        CHECK( dummy.weapon.ammo_current() == ammo.type->get_id() );
    }
}

TEST_CASE( "automatic_reloading_action", "[reload],[gun]" )
{
    player &dummy = get_avatar();

    clear_avatar();
    // Make sure the player doesn't drop anything :P
    dummy.wear_item( item( "backpack", calendar::turn_zero ) );

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
        item &ammo = dummy.i_add( item( "40sw", calendar::turn_zero, 100 ) );
        REQUIRE( ammo.is_ammo() );

        dummy.weapon = item( "sw_610", calendar::turn_zero, 0 );
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
            item &gun2 = dummy.i_add( item( "sw_610", calendar::turn_zero, 0 ) );
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
        dummy.worn.emplace_back( "backpack" );
        item &ammo = dummy.i_add( item( "9mm", calendar::turn_zero, 50 ) );
        const cata::value_ptr<islot_ammo> &ammo_type = ammo.type->ammo;
        REQUIRE( ammo_type );

        item &mag = dummy.i_add( item( "glockmag", calendar::turn_zero, 0 ) );
        const cata::value_ptr<islot_magazine> &magazine_type = mag.type->magazine;
        REQUIRE( magazine_type );
        REQUIRE( magazine_type->type.count( ammo_type->type ) != 0 );
        REQUIRE( mag.ammo_remaining() == 0 );

        dummy.weapon = item( "glock_19", calendar::turn_zero, 0 );
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
                REQUIRE( !mags.front()->empty() );
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
            item &mag2 = dummy.i_add( item( "glockbigmag", calendar::turn_zero, 0 ) );
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
                    REQUIRE( !mags.front()->empty() );
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
                            REQUIRE( !mags.front()->empty() );
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

// TODO: nested containers and frozen liquids.
TEST_CASE( "reload_liquid_container", "[reload],[liquid]" )
{
    player &dummy = get_avatar();
    clear_avatar();
    clear_map();
    item backpack( item( "bigback" ) );
    dummy.wear_item( backpack );
    item canteen( item( "2lcanteen" ) );
    REQUIRE( dummy.wield( canteen ) ) ;

    item &ammo_jug = dummy.i_add( item( "jug_plastic" ) );
    ammo_jug.put_in( item( "water_clean", calendar::turn_zero, 2 ),
                     item_pocket::pocket_type::CONTAINER );
    units::volume ammo_volume = ammo_jug.contents.total_contained_volume();

    SECTION( "reload liquid into empty container" ) {
        g->reload_wielded();
        REQUIRE( dummy.activity );
        process_activity( dummy );
        CHECK( dummy.weapon.contents.total_contained_volume() == ammo_volume );
        CHECK( ammo_jug.contents.total_contained_volume() == units::volume() );
    }

    SECTION( "reload liquid into partially filled container with same type liquid" ) {
        item water_one( "water_clean", calendar::turn_zero, 1 );
        units::volume initial_volume = water_one.volume();
        dummy.weapon.put_in( water_one, item_pocket::pocket_type::CONTAINER );
        g->reload_wielded();
        REQUIRE( dummy.activity );
        process_activity( dummy );
        CHECK( dummy.weapon.contents.total_contained_volume() == ammo_volume + initial_volume );
        CHECK( ammo_jug.contents.total_contained_volume() == units::volume() );
    }

    SECTION( "reload liquid into partially filled container with different type liquid" ) {
        item milk_one( "milk", calendar::turn_zero, 1 );
        units::volume initial_volume = milk_one.volume();
        dummy.weapon.put_in( milk_one, item_pocket::pocket_type::CONTAINER );
        g->reload_wielded();
        if( !!dummy.activity ) {
            process_activity( dummy );
        }
        CHECK( dummy.weapon.contents.total_contained_volume() == initial_volume );
        CHECK( ammo_jug.contents.total_contained_volume() == ammo_volume );
    }

    SECTION( "reload liquid into container containing a non-liquid" ) {
        item pebble( "pebble", calendar::turn_zero, 1 );
        units::volume initial_volume = pebble.volume();
        dummy.weapon.put_in( pebble, item_pocket::pocket_type::CONTAINER );
        g->reload_wielded();
        if( !!dummy.activity ) {
            process_activity( dummy );
        }
        CHECK( dummy.weapon.contents.total_contained_volume() == initial_volume );
        CHECK( ammo_jug.contents.total_contained_volume() == ammo_volume );
    }

    SECTION( "reload liquid container with more liquid than it can hold" ) {
        ammo_jug.fill_with( item( "water_clean", calendar::turn_zero, 1 ) );
        ammo_volume = ammo_jug.contents.total_contained_volume();
        g->reload_wielded();
        REQUIRE( dummy.activity );
        process_activity( dummy );
        CHECK( dummy.weapon.contents.remaining_container_capacity() == units::volume() );
        CHECK( ammo_jug.contents.total_contained_volume() +
               dummy.weapon.contents.total_contained_volume() == ammo_volume );
    }

    SECTION( "liquid reload from map" ) {
        const tripoint test_origin( 60, 60, 0 );
        map &here = get_map();
        dummy.setpos( test_origin );
        const tripoint near_point = test_origin + tripoint_east;

        SECTION( "liquid in container on floor" ) {
            ammo_jug = here.add_item( near_point, item( "bottle_plastic" ) );
            ammo_jug.fill_with( item( "water_clean" ) );
            ammo_volume = ammo_jug.contents.total_contained_volume();
            g->reload_wielded();
            REQUIRE( dummy.activity );
            process_activity( dummy );
            CHECK( dummy.weapon.contents.total_contained_volume() == ammo_volume );
            CHECK( ammo_jug.contents.total_contained_volume() == units::volume() );
        }

        SECTION( "liquid spill on floor" ) {
            REQUIRE( ammo_jug.spill_contents( near_point ) );
            g->reload_wielded();
            if( !!dummy.activity ) {
                process_activity( dummy );
            }
            CHECK( ammo_jug.contents.total_contained_volume() == units::volume() );
            CHECK( dummy.weapon.contents.total_contained_volume() == units::volume() );
        }
    }
}
