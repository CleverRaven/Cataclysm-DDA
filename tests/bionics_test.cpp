#include <functional>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "bionics.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "coordinates.h"
#include "debug.h"
#include "enums.h"
#include "game.h"
#include "item.h"
#include "item_location.h"
#include "map_helpers.h"
#include "options_helpers.h"
#include "pimpl.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "weather_type.h"

static const bionic_id bio_batteries( "bio_batteries" );
// Change to some other weapon CBM if bio_blade is ever removed
static const bionic_id bio_blade( "bio_blade" );
static const bionic_id bio_cable( "bio_cable" );
static const bionic_id bio_earplugs( "bio_earplugs" );
static const bionic_id bio_ears( "bio_ears" );
static const bionic_id bio_fuel_cell_gasoline( "bio_fuel_cell_gasoline" );
static const bionic_id bio_fuel_wood( "bio_fuel_wood" );
static const bionic_id bio_power_storage( "bio_power_storage" );
// Change to some other weapon CBM if bio_surgical_razor is ever removed
static const bionic_id bio_surgical_razor( "bio_surgical_razor" );
// Any item that can be wielded

static const flag_id json_flag_PSEUDO( "PSEUDO" );

static const itype_id fuel_type_gasoline( "gasoline" );
static const itype_id itype_UPS_ON( "UPS_ON" );
static const itype_id itype_backpack( "backpack" );
static const itype_id itype_jumper_cable( "jumper_cable" );
static const itype_id itype_light_battery_cell( "light_battery_cell" );
static const itype_id itype_pants_cargo( "pants_cargo" );
static const itype_id itype_solarpack_on( "solarpack_on" );
static const itype_id itype_splinter( "splinter" );
static const itype_id itype_test_backpack( "test_backpack" );

TEST_CASE( "Bionic_power_capacity", "[bionics] [power]" )
{
    avatar &dummy = get_avatar();

    GIVEN( "character starts without bionics and no bionic power" ) {
        clear_avatar();
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
                std::optional<bionic *> bio = dummy.find_bionic_by_type( bio_power_storage );
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
                std::optional<bionic *> bio = dummy.find_bionic_by_type( bio_power_storage );
                REQUIRE( bio );
                dummy.remove_bionic( **bio );
                THEN( "current power is reduced to fit the new capacity" ) {
                    CHECK( dummy.get_power_level() == dummy.get_max_power_level() );
                }
            }
        }
    }
}

TEST_CASE( "bionic_UIDs", "[bionics]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();

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

TEST_CASE( "bionic_weapons", "[bionics] [weapon] [item]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();
    dummy.add_bionic( bio_power_storage );
    dummy.add_bionic( bio_power_storage );
    dummy.set_power_level( dummy.get_max_power_level() );
    REQUIRE( dummy.get_power_level() == 200_kJ );
    REQUIRE( dummy.is_max_power() );

    GIVEN( "character has two weapon CBM" ) {
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
                    std::optional<int> cbm_index = dummy.active_bionic_weapon_index();
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
                std::optional<item> removed_weapon = customizable_bionic.uninstall_weapon();

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

TEST_CASE( "included_bionics", "[bionics]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();

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

TEST_CASE( "fueled_bionics", "[bionics] [item]" )
{
    avatar &dummy = get_avatar();
    clear_avatar();

    // 500kJ ought to be enough for everybody, make sure not to add any bionics in the
    // tests after taking reference to a bionic, if bionics vector is too small and gets
    // reallocated the references become invalid
    dummy.add_bionic( bio_power_storage );
    dummy.add_bionic( bio_power_storage );
    dummy.add_bionic( bio_power_storage );
    dummy.add_bionic( bio_power_storage );
    dummy.add_bionic( bio_power_storage );

    REQUIRE( !dummy.has_power() );
    REQUIRE( dummy.has_max_power() );

    SECTION( "bio_fuel_cell_gasoline" ) {
        bionic &bio = *dummy.find_bionic_by_uid( dummy.add_bionic( bio_fuel_cell_gasoline ) ).value();
        item_location gasoline_tank = dummy.top_items_loc().front();

        // There should be no fuel available, can't turn bionic on and no power is produced
        CHECK( dummy.get_bionic_fuels( bio.id ).empty() );
        CHECK( dummy.get_cable_ups().empty() );
        CHECK( dummy.get_cable_solar().empty() );
        CHECK( dummy.get_cable_vehicle().empty() );
        CHECK_FALSE( dummy.activate_bionic( bio ) );
        dummy.suffer();
        REQUIRE( !dummy.has_power() );

        // Add fuel. Now it turns on and generates power.
        item gasoline = item( fuel_type_gasoline );
        gasoline.charges = 2;
        CHECK( gasoline_tank->can_reload_with( gasoline, true ) );
        gasoline_tank->put_in( gasoline, pocket_type::CONTAINER );
        REQUIRE( gasoline_tank->only_item().charges == 2 );
        CHECK( dummy.activate_bionic( bio ) );
        CHECK_FALSE( dummy.get_bionic_fuels( bio.id ).empty() );
        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 8550 );
        CHECK( gasoline_tank->only_item().charges == 1 );

        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 17100 );
        CHECK( gasoline_tank->empty_container() );

        // Run out of fuel
        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 17100 );
    }

    SECTION( "bio_batteries" ) {
        bionic &bio = *dummy.find_bionic_by_uid( dummy.add_bionic( bio_batteries ) ).value();
        item_location bat_compartment = dummy.top_items_loc().front();

        // There should be no fuel available, can't turn bionic on and no power is produced
        REQUIRE( bat_compartment->ammo_remaining( ) == 0 );
        CHECK( dummy.get_bionic_fuels( bio.id ).empty() );
        CHECK( dummy.get_cable_ups().empty() );
        CHECK( dummy.get_cable_solar().empty() );
        CHECK( dummy.get_cable_vehicle().empty() );
        CHECK_FALSE( dummy.activate_bionic( bio ) );
        dummy.suffer();
        REQUIRE( !dummy.has_power() );

        // Add empty battery. Still won't work
        item battery = item( itype_light_battery_cell );
        CHECK( bat_compartment->can_reload_with( battery, true ) );
        bat_compartment->put_in( battery, pocket_type::MAGAZINE_WELL );
        REQUIRE( bat_compartment->ammo_remaining( ) == 0 );
        CHECK( dummy.get_bionic_fuels( bio.id ).empty() );
        CHECK( dummy.get_cable_ups().empty() );
        CHECK( dummy.get_cable_solar().empty() );
        CHECK( dummy.get_cable_vehicle().empty() );
        CHECK_FALSE( dummy.activate_bionic( bio ) );
        dummy.suffer();
        REQUIRE( !dummy.has_power() );

        // Add fuel. Now it turns on and generates power.
        bat_compartment->magazine_current()->ammo_set( battery.ammo_default(), 2 );
        REQUIRE( bat_compartment->ammo_remaining( ) == 2 );
        CHECK( dummy.activate_bionic( bio ) );
        CHECK_FALSE( dummy.get_bionic_fuels( bio.id ).empty() );
        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 1000 );
        CHECK( bat_compartment->ammo_remaining( ) == 1 );

        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 2000 );
        CHECK( bat_compartment->ammo_remaining( ) == 0 );

        // Run out of ammo
        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 2000 );
    }

    SECTION( "bio_cable ups" ) {
        bionic &bio = *dummy.find_bionic_by_uid( dummy.add_bionic( bio_cable ) ).value();

        // There should be no fuel available, can't turn bionic on and no power is produced
        CHECK( dummy.get_bionic_fuels( bio.id ).empty() );
        CHECK( dummy.get_cable_ups().empty() );
        CHECK( dummy.get_cable_solar().empty() );
        CHECK( dummy.get_cable_vehicle().empty() );
        CHECK_FALSE( dummy.activate_bionic( bio ) );
        dummy.suffer();
        REQUIRE( !dummy.has_power() );

        // Connect to empty ups. Bionic shouldn't work
        dummy.worn.wear_item( dummy, item( itype_backpack ), false, false );
        item_location ups = dummy.i_add( item( itype_UPS_ON ) );
        item_location cable = dummy.i_add( item( itype_jumper_cable ) );
        cable->link().source = link_state::ups;
        cable->link().target = link_state::bio_cable;
        ups->set_var( "cable", "plugged_in" );
        cable->active = true;

        REQUIRE( ups->ammo_remaining( ) == 0 );
        CHECK( dummy.get_bionic_fuels( bio.id ).empty() );
        CHECK( dummy.get_cable_ups().empty() );
        CHECK( dummy.get_cable_solar().empty() );
        CHECK( dummy.get_cable_vehicle().empty() );
        CHECK_FALSE( dummy.activate_bionic( bio ) );
        dummy.suffer();
        REQUIRE( !dummy.has_power() );

        // Put empty battery into ups. Still does not work.
        item ups_mag( ups->magazine_default() );
        ups->put_in( ups_mag, pocket_type::MAGAZINE_WELL );
        REQUIRE( ups->ammo_remaining( ) == 0 );
        CHECK( dummy.get_bionic_fuels( bio.id ).empty() );
        CHECK_FALSE( dummy.activate_bionic( bio ) );
        dummy.suffer();
        REQUIRE( !dummy.has_power() );

        // Fill the battery. Works now.
        ups->magazine_current()->ammo_set( ups_mag.ammo_default(), 2 );
        REQUIRE( ups->ammo_remaining( ) == 2 );
        CHECK( dummy.activate_bionic( bio ) );
        CHECK_FALSE( dummy.get_cable_ups().empty() );
        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 1000 );
        CHECK( ups->ammo_remaining( ) == 1 );

        dummy.suffer();
        CHECK( ups->ammo_remaining( ) == 0 );
        CHECK( units::to_joule( dummy.get_power_level() ) == 2000 );

        // Run out of fuel
        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 2000 );
    }

    SECTION( "bio_cable solar" ) {
        bionic &bio = *dummy.find_bionic_by_uid( dummy.add_bionic( bio_cable ) ).value();

        // Midday for solar test
        clear_map();
        g->reset_light_level();
        scoped_weather_override weather_clear( WEATHER_CLEAR );
        calendar::turn = calendar::turn_zero + 12_hours;
        REQUIRE( g->is_in_sunlight( dummy.pos_bub() ) );

        // Connect solar backpack
        dummy.worn.wear_item( dummy, item( itype_pants_cargo ), false, false );
        dummy.worn.wear_item( dummy, item( itype_solarpack_on ), false, false );
        // Unsafe way to get the worn solar backpack
        item_location solar_pack = dummy.top_items_loc()[1];
        REQUIRE( solar_pack->typeId() == itype_solarpack_on );
        item_location cable = dummy.i_add( item( itype_jumper_cable ) );
        cable->link().source = link_state::solarpack;
        cable->link().target = link_state::bio_cable;
        solar_pack->set_var( "cable", "plugged_in" );
        cable->active = true;

        CHECK( dummy.get_bionic_fuels( bio.id ).empty() );
        CHECK( dummy.get_cable_ups().empty() );
        CHECK_FALSE( dummy.get_cable_solar().empty() );
        CHECK( dummy.get_cable_vehicle().empty() );
        CHECK( dummy.activate_bionic( bio ) );
        dummy.suffer();
        CHECK( units::to_millijoule( dummy.get_power_level() ) == 37525 );
    }

    SECTION( "bio_wood_burner" ) {
        bionic &bio = *dummy.find_bionic_by_uid( dummy.add_bionic( bio_fuel_wood ) ).value();
        item_location woodshed = dummy.top_items_loc().front();

        // Turn safe fuel off since log produces too much energy
        bio.set_safe_fuel_thresh( -1.0f );

        // There should be no fuel available, can't turn bionic on and no power is produced
        CHECK( dummy.get_bionic_fuels( bio.id ).empty() );
        CHECK_FALSE( dummy.activate_bionic( bio ) );
        dummy.suffer();
        REQUIRE( !dummy.has_power() );

        // Add two splints. Now it turns on and generates power.
        item wood = item( itype_splinter );
        item wood_2 = item( itype_splinter );
        REQUIRE_FALSE( wood.count_by_charges() );
        woodshed->put_in( wood, pocket_type::CONTAINER );
        woodshed->put_in( wood_2, pocket_type::CONTAINER );
        REQUIRE( woodshed->all_items_ptr().size() == 2 );
        CHECK( dummy.activate_bionic( bio ) );
        CHECK_FALSE( dummy.get_bionic_fuels( bio.id ).empty() );
        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 62500 );
        CHECK( woodshed->all_items_ptr().size() == 1 );

        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 125000 );
        CHECK( woodshed->empty_container() );

        // Run out of fuel
        dummy.suffer();
        CHECK( units::to_joule( dummy.get_power_level() ) == 125000 );
    }
}
