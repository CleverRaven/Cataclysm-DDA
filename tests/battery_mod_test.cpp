#include <functional>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "debug.h"
#include "item.h"
#include "item_contents.h"
#include "item_pocket.h"
#include "itype.h"
#include "iuse.h"
#include "iuse_actor.h"
#include "make_static.h"
#include "player.h"
#include "player_helpers.h"
#include "ret_val.h"
#include "type_id.h"
#include "value_ptr.h"

// Includes functions:
// item::magazine_compatible
// item::magazine_default
// item::magazine_integral
// item::is_reloadable
// item::is_reloadable_with
// item::toolmods
//
// item::ammo_type
// item::ammo_types
// item::ammo_default
// item::ammo_remaining
// item::ammo_capacity
//
// Attributes:
// item.type->mod->acceptable_ammo
// item.type->mod->magazine_adaptor
// item.type->tool->ammo_id
//
// Related JSON fields:
// "ammo_type"
// "acceptable_ammo"
// "ammo_restriction"
// "magazine_adaptor"
//
TEST_CASE( "battery tool mod test", "[battery][mod]" )
{
    item med_mod( "magazine_battery_medium_mod" );

    SECTION( "battery mod properties" ) {
        // Is a toolmod, and nothing else
        CHECK( med_mod.is_toolmod() );
        CHECK_FALSE( med_mod.is_gun() );
        CHECK_FALSE( med_mod.is_tool() );
        CHECK_FALSE( med_mod.is_magazine() );

        // Mod can be installed on items using battery ammotype
        CHECK( med_mod.type->mod->acceptable_ammo.count( ammotype( "battery" ) ) == 1 );
        // The battery mod does not use ammo_modifier (since it gives explicit battery ids)
        CHECK( med_mod.type->mod->ammo_modifier.empty() );

        // And has some magazine adaptors
        CHECK_FALSE( med_mod.type->mod->magazine_adaptor.empty() );
        // Mod itself has no ammo types
        CHECK( med_mod.ammo_types().empty() );

        // No tool mods
        CHECK( med_mod.toolmods().empty() );

        // Mods are not directly compatible with magazines, nor reloadable
        CHECK( med_mod.magazine_compatible().empty() );
        CHECK_FALSE( med_mod.is_reloadable() );
        CHECK_FALSE( med_mod.is_reloadable_with( itype_id( "battery" ) ) );

        // Mod magazine is not integral
        CHECK_FALSE( med_mod.magazine_integral() );
    }

    GIVEN( "tool compatible with light batteries" ) {
        item flashlight( "flashlight" );
        REQUIRE( flashlight.is_reloadable() );
        REQUIRE( flashlight.is_reloadable_with( itype_id( "light_battery_cell" ) ) );

        // Flashlight must be free of battery or existing mods
        REQUIRE_FALSE( flashlight.magazine_current() );
        REQUIRE( flashlight.toolmods().empty() );
        // Needs a MOD pocket to allow modding
        REQUIRE( flashlight.has_pocket_type( item_pocket::pocket_type::MOD ) );

        WHEN( "medium battery mod is installed" ) {
            med_mod.set_flag( STATIC( flag_id( "IRREMOVABLE" ) ) );
            flashlight.put_in( med_mod, item_pocket::pocket_type::MOD );

            THEN( "tool modification is successful" ) {
                CHECK_FALSE( flashlight.toolmods().empty() );
                CHECK_FALSE( flashlight.magazine_compatible().empty() );

                CHECK( flashlight.tname() == "flashlight (off)+1" );
            }

            THEN( "medium batteries can be installed" ) {
                CHECK( flashlight.is_reloadable() );
                CHECK( flashlight.is_reloadable_with( itype_id( "medium_battery_cell" ) ) );
                const std::set<itype_id> mag_compats = flashlight.magazine_compatible();
                CHECK( mag_compats.count( itype_id( "medium_battery_cell" ) ) == 1 );
                CHECK( mag_compats.count( itype_id( "medium_plus_battery_cell" ) ) == 1 );
                CHECK( mag_compats.count( itype_id( "medium_atomic_battery_cell" ) ) == 1 );
                CHECK( mag_compats.count( itype_id( "medium_disposable_cell" ) ) == 1 );
                CHECK( flashlight.has_pocket_type( item_pocket::pocket_type::MAGAZINE_WELL ) );
            }

            THEN( "medium battery is now the default" ) {
                itype_id mag_default = flashlight.magazine_default( false );
                CHECK_FALSE( mag_default.is_null() );
                CHECK( mag_default.str() == "medium_battery_cell" );
            }

            THEN( "light batteries no longer fit" ) {
                CHECK_FALSE( flashlight.is_reloadable_with( itype_id( "light_battery_cell" ) ) );
                CHECK_FALSE( flashlight.magazine_compatible().count( itype_id( "light_battery_cell" ) ) );
            }

            WHEN( "medium battery is installed" ) {
                item med_battery( "medium_battery_cell" );
                ret_val<bool> result = flashlight.put_in( med_battery, item_pocket::pocket_type::MAGAZINE_WELL );

                THEN( "battery installation succeeds" ) {
                    CHECK( result.success() );
                }

                THEN( "the flashlight has a battery" ) {
                    CHECK( flashlight.magazine_current() );
                }
            }

            WHEN( "charged medium battery is installed" ) {
                item med_battery( "medium_battery_cell" );

                const int bat_charges = med_battery.ammo_capacity( ammotype( "battery" ) );
                med_battery.ammo_set( med_battery.ammo_default(), bat_charges );
                REQUIRE( med_battery.ammo_remaining() == bat_charges );
                flashlight.put_in( med_battery, item_pocket::pocket_type::MAGAZINE_WELL );

                THEN( "the flashlight has charges" ) {
                    CHECK( flashlight.ammo_remaining() == bat_charges );
                }

                AND_WHEN( "flashlight is activated" ) {
                    const use_function *use = flashlight.type->get_use( "transform" );
                    CHECK( use != nullptr );
                    const iuse_transform *actor = dynamic_cast<const iuse_transform *>( use->get_actor_ptr() );

                    player &dummy = get_avatar();
                    clear_avatar();
                    actor->use( dummy, flashlight, false, dummy.pos() );

                    // Regression tests for #42764 / #42854
                    THEN( "mod remains installed" ) {
                        CHECK_FALSE( flashlight.toolmods().empty() );
                        CHECK_FALSE( flashlight.magazine_compatible().empty() );
                    }
                    THEN( "battery remains installed" ) {
                        CHECK( flashlight.magazine_current() );
                    }
                    THEN( "medium battery remains the default" ) {
                        itype_id mag_default = flashlight.magazine_default( false );
                        CHECK_FALSE( mag_default.is_null() );
                        CHECK( mag_default.str() == "medium_battery_cell" );
                    }
                }
            }
        }
    }
}

// This test exercises several item functions related to battery-powered tools.
//
// Tool battery compartments and battery charge use terms derived from firearms, thus:
//
// - Batteries are "magazines"
//   - Have "ammo types" compatible with them
//   - Charge left in the battery is "ammo remaining"
//
// - Tools have a "magazine well" (battery compartment)
//   - Can be reloaded with a compatible "magazine" (battery)
//   - Charge left in the tool's battery is "ammo remaining"
//
TEST_CASE( "battery and tool properties", "[battery][tool][properties]" )
{
    const item bat_cell( "light_battery_cell" );
    const item flashlight( "flashlight" );

    // In JSON, "battery" is both an "ammunition_type" (ammo_types.json) and an "AMMO" (ammo.json)
    const ammotype bat_ammotype( "battery" );
    const itype_id bat_ammo( "battery" );

    SECTION( "battery cell" ) {
        SECTION( "is a magazine" ) {
            CHECK( bat_cell.is_magazine() );
        }

        SECTION( "is not a tool" ) {
            CHECK_FALSE( bat_cell.is_tool() );
        }

        SECTION( "is not ammo" ) {
            CHECK_FALSE( bat_cell.is_ammo() );
            // Non-ammo itself does not have an ammo type
            CHECK( bat_cell.ammo_type() == ammotype::NULL_ID() );
        }

        SECTION( "has compatible ammo types" ) {
            const std::set<ammotype> bat_ammos = bat_cell.ammo_types();
            CHECK_FALSE( bat_ammos.empty() );
            CHECK( bat_ammos.count( bat_ammotype ) );
        }

        SECTION( "has capacity to hold battery ammo type" ) {
            CHECK( bat_cell.ammo_capacity( bat_ammotype ) > 0 );
        }

        SECTION( "has battery ammo as default" ) {
            CHECK( bat_cell.ammo_default() == bat_ammo );
        }

        SECTION( "is not counted by charges" ) {
            CHECK_FALSE( bat_cell.count_by_charges() );
        }
    }

    SECTION( "flashlight" ) {
        SECTION( "is a tool" ) {
            CHECK( flashlight.is_tool() );
        }

        SECTION( "is not a magazine" ) {
            CHECK_FALSE( flashlight.is_magazine() );
            CHECK_FALSE( flashlight.magazine_integral() );
        }

        SECTION( "is not ammo" ) {
            CHECK_FALSE( flashlight.is_ammo() );
            // Non-ammo itself does not have an ammo type
            CHECK( flashlight.ammo_type() == ammotype::NULL_ID() );
        }

        SECTION( "is reloadable with a magazine" ) {
            CHECK( flashlight.is_reloadable() );
            CHECK( flashlight.is_reloadable_with( itype_id( "light_battery_cell" ) ) );
            CHECK( flashlight.is_reloadable_with( itype_id( "light_disposable_cell" ) ) );
        }

        SECTION( "has compatible magazines" ) {
            const std::set<itype_id> mag_compats = flashlight.magazine_compatible();
            CHECK_FALSE( mag_compats.empty() );
            CHECK( mag_compats.count( itype_id( "light_battery_cell" ) ) == 1 );
            CHECK( mag_compats.count( itype_id( "light_disposable_cell" ) ) == 1 );
            CHECK( mag_compats.count( itype_id( "light_plus_battery_cell" ) ) == 1 );
            CHECK( mag_compats.count( itype_id( "light_atomic_battery_cell" ) ) == 1 );
        }

        SECTION( "has a default magazine" ) {
            itype_id mag_default = flashlight.magazine_default( false );
            CHECK_FALSE( mag_default.is_null() );
            CHECK( mag_default.str() == "light_disposable_cell" );
        }

        SECTION( "can use battery ammo" ) {
            // Since flashlights get their "ammo" from a "magazine", their ammo_types is empty
            CHECK( flashlight.ammo_types().empty() );
            CHECK( flashlight.ammo_default().is_null() );

            // The ammo a flashlight can *use* is given by type->tool->ammo_id
            CHECK_FALSE( flashlight.type->tool->ammo_id.empty() );
            CHECK( flashlight.type->tool->ammo_id.count( ammotype( "battery" ) ) == 1 );
        }

        SECTION( "requires some ammo (charge) to use" ) {
            CHECK( flashlight.ammo_required() > 0 );
        }

        SECTION( "is not counted by charges" ) {
            CHECK_FALSE( flashlight.count_by_charges() );
        }
    }
}

TEST_CASE( "installing battery in tool", "[battery][tool][install]" )
{
    item bat_cell( "light_battery_cell" );
    item flashlight( "flashlight" );

    const int bat_charges = bat_cell.ammo_capacity( ammotype( "battery" ) );
    REQUIRE( bat_charges > 0 );

    SECTION( "flashlight with no battery installed" ) {
        REQUIRE( !flashlight.magazine_current() );

        CHECK( flashlight.ammo_remaining() == 0 );
        CHECK( flashlight.ammo_capacity( ammotype( "battery" ) ) == 0 );
        CHECK( flashlight.remaining_ammo_capacity() == 0 );
    }

    SECTION( "dead battery installed in flashlight" ) {
        // Ensure battery is dead
        bat_cell.ammo_set( bat_cell.ammo_default(), 0 );
        REQUIRE( bat_cell.ammo_remaining() == 0 );

        // Put battery in flashlight
        REQUIRE( flashlight.has_pocket_type( item_pocket::pocket_type::MAGAZINE_WELL ) );
        ret_val<bool> result = flashlight.put_in( bat_cell, item_pocket::pocket_type::MAGAZINE_WELL );
        CHECK( result.success() );
        CHECK( flashlight.magazine_current() );

        // No remaining ammo
        CHECK( flashlight.ammo_remaining() == 0 );
    }

    SECTION( "charged battery installed in flashlight" ) {
        // Charge the battery
        bat_cell.ammo_set( bat_cell.ammo_default(), bat_charges );
        REQUIRE( bat_cell.ammo_remaining() == bat_charges );

        // Put battery in flashlight
        REQUIRE( flashlight.has_pocket_type( item_pocket::pocket_type::MAGAZINE_WELL ) );
        ret_val<bool> result = flashlight.put_in( bat_cell, item_pocket::pocket_type::MAGAZINE_WELL );
        CHECK( result.success() );
        CHECK( flashlight.magazine_current() );

        // Flashlight has a full charge
        CHECK( flashlight.ammo_remaining() == bat_charges );
    }

    SECTION( "wrong size battery for flashlight" ) {
        item med_bat_cell( "medium_battery_cell" );

        // Should fail to install the magazine
        REQUIRE( flashlight.has_pocket_type( item_pocket::pocket_type::MAGAZINE_WELL ) );
        std::string dmsg = capture_debugmsg_during( [&flashlight, &med_bat_cell]() {
            ret_val<bool> result = flashlight.put_in( med_bat_cell, item_pocket::pocket_type::MAGAZINE_WELL );
            CHECK_FALSE( result.success() );
        } );
        CHECK_THAT( dmsg, Catch::EndsWith( "holster does not accept this item type" ) );
        CHECK_FALSE( flashlight.magazine_current() );
    }
}
