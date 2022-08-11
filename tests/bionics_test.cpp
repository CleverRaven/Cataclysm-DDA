#include <climits>
#include <iosfwd>
#include <list>
#include <memory>
#include <string>

#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_catch.h"
#include "item.h"
#include "item_pocket.h"
#include "npc.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"

static const bionic_id bio_batteries( "bio_batteries" );
// Change to some other weapon CBM if bio_blade is ever removed
static const bionic_id bio_blade( "bio_blade" );
static const bionic_id bio_earplugs( "bio_earplugs" );
static const bionic_id bio_ears( "bio_ears" );
static const bionic_id bio_fuel_cell_gasoline( "bio_fuel_cell_gasoline" );
static const bionic_id bio_power_storage( "bio_power_storage" );
// Change to some other weapon CBM if bio_surgical_razor is ever removed
static const bionic_id bio_surgical_razor( "bio_surgical_razor" );
// Any item that can be wielded
static const flag_id json_flag_PSEUDO( "PSEUDO" );
static const itype_id itype_test_backpack( "test_backpack" );

static void clear_bionics( Character &you )
{
    you.my_bionics->clear();
    you.update_last_bionic_uid();
    you.update_bionic_power_capacity();
    you.set_power_level( 0_kJ );
    you.set_max_power_level_modifier( 0_kJ );
}

TEST_CASE( "Bionic power capacity", "[bionics] [power]" )
{
    avatar &dummy = get_avatar();

    GIVEN( "character starts without bionics and no bionic power" ) {
        clear_avatar();
        clear_bionics( dummy );
        REQUIRE( !dummy.has_max_power() );

        WHEN( "a Power Storage CBM is installed" ) {
            dummy.add_bionic( bio_power_storage );

            THEN( "their total bionic power capacity increases by the Power Storage capacity" ) {
                CHECK( dummy.get_max_power_level() == bio_power_storage->capacity );
            }
        }
    }

    GIVEN( "character starts with 3 power storage bionics" ) {
        clear_avatar();
        clear_bionics( dummy );
        dummy.add_bionic( bio_power_storage );
        dummy.add_bionic( bio_power_storage );
        dummy.add_bionic( bio_power_storage );
        REQUIRE( dummy.has_max_power() );
        units::energy current_max_power = dummy.get_max_power_level();
        REQUIRE( !dummy.has_power() );

        AND_GIVEN( "power level is twice the capacity of a power storage bionic (not maxed)" ) {
            dummy.set_power_level( bio_power_storage->capacity * 2 );
            REQUIRE( dummy.get_power_level() == bio_power_storage->capacity * 2 );

            WHEN( "a Power Storage CBM is uninstalled" ) {
                cata::optional<bionic *> bio = dummy.find_bionic_by_type( bio_power_storage );
                REQUIRE( bio );
                dummy.remove_bionic( **bio );
                THEN( "maximum power decreases by the Power Storage capacity without changing current power level" ) {
                    CHECK( dummy.get_max_power_level() == current_max_power - bio_power_storage->capacity );
                    CHECK( dummy.get_power_level() == bio_power_storage->capacity * 2 );
                }
            }
        }

        AND_GIVEN( "power level is maxed" ) {
            dummy.set_power_level( dummy.get_max_power_level() );
            REQUIRE( dummy.is_max_power() );

            WHEN( "a Power Storage CBM is uninstalled" ) {
                cata::optional<bionic *> bio = dummy.find_bionic_by_type( bio_power_storage );
                REQUIRE( bio );
                dummy.remove_bionic( **bio );
                THEN( "current power is reduced to fit the new capacity" ) {
                    CHECK( dummy.get_power_level() == dummy.get_max_power_level() );
                }
            }
        }
    }
}

TEST_CASE( "bionic UIDs", "[bionics]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_bionics( dummy );

    GIVEN( "character doesn't have any CBMs installed" ) {
        REQUIRE( dummy.get_bionics().empty() );

        WHEN( "a new CBM is installed" ) {
            dummy.add_bionic( bio_power_storage );

            THEN( "its UID is set" ) {
                CHECK( dummy.my_bionics->back().get_uid() );
            }
        }
    }

    GIVEN( "character has multiple CBMs installed" ) {
        dummy.my_bionics->emplace_back( bio_power_storage, get_free_invlet( dummy ), 1000 );
        dummy.my_bionics->emplace_back( bio_power_storage, get_free_invlet( dummy ), 1050 );
        dummy.my_bionics->emplace_back( bio_power_storage, get_free_invlet( dummy ), 2000 );
        dummy.update_last_bionic_uid();
        bionic_uid expected_bionic_uid = dummy.generate_bionic_uid() + 1;
        REQUIRE( dummy.get_bionics().size() == 3 );

        WHEN( "a new CBM is installed" ) {
            dummy.add_bionic( bio_power_storage );

            THEN( "its UID is set to the next available UID" ) {
                CHECK( dummy.my_bionics->back().get_uid() == expected_bionic_uid );
            }
        }
    }
}

TEST_CASE( "bionic weapons", "[bionics] [weapon] [item]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_bionics( dummy );
    dummy.set_max_power_level( units::from_kilojoule( 200 ) );
    dummy.set_power_level( dummy.get_max_power_level() );
    REQUIRE( dummy.is_max_power() );

    GIVEN( "character has two weapon CBM" ) {
        dummy.add_bionic( bio_power_storage );
        dummy.add_bionic( bio_power_storage );
        dummy.add_bionic( bio_surgical_razor );
        bionic &bio2 = dummy.bionic_at_index( dummy.get_bionics().size() - 1 );
        dummy.add_bionic( bio_power_storage );
        dummy.add_bionic( bio_blade );
        bionic &bio = dummy.bionic_at_index( dummy.get_bionics().size() - 1 );
        REQUIRE_FALSE( dummy.get_bionics().empty() );
        REQUIRE_FALSE( dummy.has_weapon() );

        AND_GIVEN( "the weapon CBM is activated" ) {
            REQUIRE( dummy.activate_bionic( bio ) );
            REQUIRE( dummy.has_weapon() );

            THEN( "current CBM UID is stored and fake weapon is wielded by the character" ) {
                REQUIRE( dummy.is_using_bionic_weapon() );
                CHECK( dummy.get_weapon_bionic_uid() == bio.get_uid() );
                CHECK( dummy.get_wielded_item()->typeId() == bio.id->fake_weapon );
            }

            WHEN( "the weapon CBM is deactivated" ) {
                REQUIRE( dummy.deactivate_bionic( bio ) );

                THEN( "character doesn't have a weapon equipped anymore" ) {
                    CHECK( !dummy.get_wielded_item() );
                    CHECK_FALSE( dummy.is_using_bionic_weapon() );
                }
            }

            WHEN( "the second weapon CBM is activated next" ) {
                REQUIRE( dummy.get_wielded_item()->typeId() == bio.id->fake_weapon );
                REQUIRE( dummy.activate_bionic( bio2 ) );

                THEN( "current weapon bionic UID is stored and wielded weapon is replaced" ) {
                    REQUIRE( dummy.is_using_bionic_weapon() );
                    CHECK( dummy.get_weapon_bionic_uid() == bio2.get_uid() );
                    CHECK( dummy.get_wielded_item()->typeId() == bio2.id->fake_weapon );
                }
            }
        }

        // Test can't be executed because dispose_item creates a query window
        /*
        AND_GIVEN("character is wielding a regular item")
        {
            item real_item(itype_real_item);
            REQUIRE(dummy.can_wield(real_item).success());
            dummy.wield(real_item);
            REQUIRE(dummy.get_wielded_item().typeId() == itype_real_item);

            WHEN("the weapon CBM is activated")
            {
                REQUIRE(dummy.activate_bionic(installed_index));

                THEN("current CBM UID is stored and fake weapon is wielded by the character")
                {
                    cata::optional<int> cbm_index = dummy.active_bionic_weapon_index();
                    REQUIRE(cbm_index);
                    CHECK(*cbm_index == installed_index);
                    CHECK(dummy.get_wielded_item().typeId() == weapon_bionic->fake_weapon);
                }
            }
        }*/
    }

    GIVEN( "character has a customizable weapon CBM" ) {
        bionic_id customizable_weapon_bionic_id( "bio_blade" );
        dummy.add_bionic( customizable_weapon_bionic_id );
        bionic &customizable_bionic = dummy.bionic_at_index( dummy.my_bionics->size() - 1 );
        REQUIRE_FALSE( dummy.get_bionics().empty() );
        REQUIRE_FALSE( dummy.has_weapon() );
        item::FlagsSetType *allowed_flags = const_cast<item::FlagsSetType *>
                                            ( &customizable_weapon_bionic_id->installable_weapon_flags );
        allowed_flags->insert( json_flag_PSEUDO );

        GIVEN( "weapon bionic allows installation of new weapons" ) {
            REQUIRE( customizable_weapon_bionic_id->installable_weapon_flags.find(
                         json_flag_PSEUDO ) != customizable_weapon_bionic_id->installable_weapon_flags.end() );

            WHEN( "character tries uninstalls weapon installed in the customizable bionic" ) {
                cata::optional<item> removed_weapon = customizable_bionic.uninstall_weapon();

                THEN( "weapon is uninstalled and retrieved as an item" ) {
                    REQUIRE( removed_weapon );
                    CHECK( removed_weapon->typeId() == customizable_bionic.id->fake_weapon );
                }
            }
        }

        AND_GIVEN( "character is wielding a regular item" ) {
            item real_item( itype_test_backpack );
            REQUIRE( dummy.can_wield( real_item ).success() );
            dummy.wield( real_item );
            item_location wielded_item = dummy.get_wielded_item();
            REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_backpack );

            AND_GIVEN( "weapon bionic doesn't allow installation of new weapons" ) {
                allowed_flags->clear();
                customizable_bionic.set_weapon( item() );
                REQUIRE_FALSE( customizable_bionic.can_install_weapon() );
                REQUIRE_FALSE( customizable_bionic.has_weapon() );

                THEN( "character fails to install a new weapon on bionic" ) {
                    capture_debugmsg_during( [&customizable_bionic, &wielded_item]() {
                        CHECK_FALSE( customizable_bionic.install_weapon( *wielded_item ) );
                    } );
                }
            }

            AND_GIVEN( "weapon bionic allows installation of new weapons" ) {
                REQUIRE( customizable_weapon_bionic_id->installable_weapon_flags.find(
                             json_flag_PSEUDO ) != customizable_weapon_bionic_id->installable_weapon_flags.end() );

                AND_GIVEN( "a weapon is already installed on bionic" ) {
                    REQUIRE( customizable_bionic.has_weapon() );

                    THEN( "character fails to install a new weapon on bionic" ) {
                        capture_debugmsg_during( [&customizable_bionic, &wielded_item]() {
                            CHECK_FALSE( customizable_bionic.install_weapon( *wielded_item ) );
                        } );
                    }
                }

                AND_GIVEN( "bionic is powered" ) {
                    customizable_bionic.powered = true;

                    WHEN( "character tries to install a new weapon on bionic" ) {
                        THEN( "installation fails" ) {
                            capture_debugmsg_during( [&customizable_bionic, &wielded_item]() {
                                CHECK_FALSE( customizable_bionic.install_weapon( *wielded_item ) );
                            } );
                        }
                    }
                }

                AND_GIVEN( "bionic has no weapon installed" ) {
                    customizable_bionic.set_weapon( item() );
                    REQUIRE_FALSE( customizable_bionic.has_weapon() );

                    AND_GIVEN( "item doesn't have valid flags for bionic installation" ) {
                        wielded_item->unset_flag( json_flag_PSEUDO );
                        REQUIRE_FALSE( wielded_item->has_flag( json_flag_PSEUDO ) );

                        THEN( "character fails to install a new weapon on bionic" ) {
                            capture_debugmsg_during( [&customizable_bionic, &wielded_item]() {
                                CHECK_FALSE( customizable_bionic.install_weapon( *wielded_item ) );
                            } );
                        }
                    }

                    AND_GIVEN( "item has valid flags" ) {
                        wielded_item->set_flag( json_flag_PSEUDO );
                        REQUIRE( wielded_item->has_flag( json_flag_PSEUDO ) );
                        REQUIRE( wielded_item->has_any_flag( customizable_weapon_bionic_id->installable_weapon_flags ) );

                        WHEN( "character tries to install a new weapon on bionic" ) {
                            THEN( "installation succeeds" ) {
                                CHECK( customizable_bionic.install_weapon( *wielded_item ) );
                            }
                        }
                    }
                }
            }
        }
    }

    GIVEN( "character has an activated weapon CBM with a breakable weapon" ) {
        dummy.add_bionic( bio_surgical_razor );
        bionic &bio = dummy.bionic_at_index( dummy.get_bionics().size() - 1 );
        bio.set_weapon( item( itype_test_backpack ) );
        REQUIRE_FALSE( dummy.get_bionics().empty() );
        REQUIRE_FALSE( dummy.has_weapon() );
        REQUIRE( dummy.activate_bionic( bio ) );
        REQUIRE( dummy.is_using_bionic_weapon() );
        REQUIRE( dummy.get_weapon_bionic_uid() == bio.get_uid() );
        REQUIRE( dummy.get_wielded_item()->typeId() == itype_test_backpack );

        WHEN( "weapon breaks" ) {
            item_location weapon = dummy.get_wielded_item();
            weapon->set_damage( weapon->max_damage() );
            REQUIRE( dummy.handle_melee_wear( weapon, 100000 ) );
            REQUIRE_FALSE( dummy.has_weapon() );

            THEN( "weapon bionic is deactivated and its weapon is gone" ) {
                CHECK_FALSE( dummy.is_using_bionic_weapon() );
                CHECK_FALSE( bio.has_weapon() );
                CHECK_FALSE( bio.powered );
            }
        }
    }
}

TEST_CASE( "included bionics", "[bionics]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    clear_bionics( dummy );

    GIVEN( "character doesn't have any CBMs installed" ) {
        REQUIRE( dummy.get_bionics().empty() );

        WHEN( "a CBM with included bionics is installed" ) {
            dummy.add_bionic( bio_ears );
            bionic &parent_bio = dummy.bionic_at_index( dummy.num_bionics() - 2 );
            bionic &included_bio = dummy.bionic_at_index( dummy.num_bionics() - 1 );

            THEN( "the bionic and its included bionics are installed" ) {
                REQUIRE( dummy.num_bionics() > 1 );
                REQUIRE( parent_bio.id == bio_ears );
                REQUIRE( included_bio.id == bio_earplugs );
                CHECK( included_bio.get_parent_uid() == parent_bio.get_uid() );
            }

            WHEN( "the parent bionic is uninstalled" ) {
                REQUIRE( dummy.num_bionics() > 1 );
                dummy.remove_bionic( parent_bio );

                THEN( "the bionic is removed along with the included bionic" ) {
                    CHECK( dummy.num_bionics() == 0 );
                }
            }
        }
    }
}

TEST_CASE( "bionics", "[bionics] [item]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();

    // one section failing shouldn't affect the rest
    clear_bionics( dummy );

    // Could be a SECTION, but prerequisite for many tests.
    INFO( "no power capacity at first" );
    CHECK( !dummy.has_max_power() );

    dummy.add_bionic( bio_power_storage );

    INFO( "adding Power Storage CBM only increases capacity" );
    CHECK( !dummy.has_power() );

    REQUIRE( dummy.has_max_power() );

    SECTION( "bio_fuel_cell_gasoline" ) {
        dummy.add_bionic( bio_fuel_cell_gasoline );

        item gasoline = item( "gasoline" );
        REQUIRE( gasoline.charges != 0 );
        CHECK( dummy.can_fuel_bionic_with( gasoline ) );

        // Bottle with gasoline does not work
        item bottle = item( "bottle_plastic" );
        bottle.put_in( gasoline, item_pocket::pocket_type::CONTAINER );
        CHECK( !dummy.can_fuel_bionic_with( bottle ) );

        // Armor has no reason to work.
        item armor = item( "rm13_armor" );
        CHECK( !dummy.can_fuel_bionic_with( armor ) );
    }

    SECTION( "bio_batteries" ) {
        dummy.add_bionic( bio_batteries );

        item battery = item( "light_battery_cell" );

        // Empty battery won't work
        battery.ammo_set( battery.ammo_default(), 0 );
        CHECK_FALSE( dummy.can_fuel_bionic_with( battery ) );

        // Full battery works
        battery.ammo_set( battery.ammo_default(), 50 );
        CHECK( dummy.can_fuel_bionic_with( battery ) );

        // Tool with battery won't work
        item flashlight = item( "flashlight" );
        flashlight.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );
        REQUIRE( flashlight.ammo_remaining() == 50 );
        CHECK_FALSE( dummy.can_fuel_bionic_with( flashlight ) );

    }

    clear_bionics( dummy );
    // TODO: bio_cable bio_reactor
    // TODO: (pick from stuff with power_source)
}
