#include <climits>
#include <map>
#include <string>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "character_attire.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"

static const itype_id itype_UPS_ON( "UPS_ON" );
static const itype_id itype_battery_ups( "battery_ups" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_heavy_battery_cell( "heavy_battery_cell" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );
static const itype_id itype_test_can_food( "test_can_food" );
static const itype_id itype_test_cordless_drill( "test_cordless_drill" );
static const itype_id itype_test_liquid( "test_liquid" );
static const itype_id itype_test_sonic_screwdriver( "test_sonic_screwdriver" );

static const quality_id qual_BOIL( "BOIL" );
static const quality_id qual_DRILL( "DRILL" );
static const quality_id qual_LOCKPICK( "LOCKPICK" );
static const quality_id qual_PRY( "PRY" );
static const quality_id qual_SCREW( "SCREW" );
static const quality_id qual_SCREW_FINE( "SCREW_FINE" );
static const quality_id qual_WRENCH( "WRENCH" );

// item::get_quality
//
// Returns the best quality level provided by:
//
// - inherent qualities from the "qualities" field
// - charged qualities from the "charged_qualities" field (only if item has sufficient charge)
// - inherent or charged qualities of any items contained within this item
//
// The BOIL quality requires the item (container) to be empty, unless strict_boiling = false.
//
TEST_CASE( "get_quality", "[tool][quality]" )
{
    SECTION( "get_quality returns numeric level of given quality" ) {
        item sonic( itype_test_sonic_screwdriver );
        CHECK( sonic.get_quality( qual_LOCKPICK ) == 30 );
        CHECK( sonic.get_quality( qual_PRY ) == 2 );
        CHECK( sonic.get_quality( qual_SCREW ) == 2 );
        CHECK( sonic.get_quality( qual_SCREW_FINE ) == 1 );
        CHECK( sonic.get_quality( qual_WRENCH ) == 1 );
    }

    SECTION( "charged_qualities" ) {
        // Without charges, the cordless drill cannot drill, but with charges, it can drill
        SECTION( "with 0 charge, get_quality returns 0" ) {
            CHECK( tool_with_ammo( itype_test_cordless_drill, 0 ).get_quality( qual_DRILL ) == 0 );
        }
        SECTION( "with sufficient charge, get_quality returns quality level" ) {
            CHECK( tool_with_ammo( itype_test_cordless_drill, 20 ).get_quality( qual_DRILL ) == 3 );
        }
    }

    SECTION( "BOIL quality" ) {
        // Tools that can BOIL and have a CONTAINER pocket have BOIL quality only when empty
        item tin_can( itype_test_can_food );
        SECTION( "get_quality returns BOIL quality if container is empty" ) {
            REQUIRE( tin_can.empty_container() );

            CHECK( tin_can.get_quality( qual_BOIL ) == 1 );
        }
        SECTION( "get_quality returns INT_MIN for BOIL quality if container is not empty" ) {
            item broth( itype_test_liquid );
            tin_can.put_in( broth, pocket_type::CONTAINER );
            REQUIRE_FALSE( tin_can.empty_container() );

            CHECK( tin_can.get_quality( qual_BOIL ) == INT_MIN );
            // Passing strict_boiling = false to get_quality ignores the emptiness rule
            CHECK( tin_can.get_quality( qual_BOIL, false ) == 1 );
        }
    }
}

// Tools that run on battery power (or are otherwise "charged") may have "charged_qualities"
// that are only available when the item is charged with at least "charges_per_use" charges.
TEST_CASE( "battery-powered_tool_qualities", "[tool][battery][quality]" )
{
    item drill( itype_test_cordless_drill );
    item battery( itype_medium_battery_cell );

    // This cordless drill is a battery-powered tool needing some charges for each use
    REQUIRE( drill.is_tool() );
    REQUIRE( drill.type->charges_to_use() > 0 );

    // It has one "inherent" quality, SCREW 1, that always works regardless of charges
    REQUIRE( drill.type->qualities.size() == 1 );
    // and it has one "charged" quality, DRILL 3, that only works with enough charges
    REQUIRE( drill.type->charged_qualities.size() == 1 );
    // the idea being that the screwdriver bit can be used to awkwardly turn screws, even without
    // powered assistance, while drilling holes with an unpowered drill bit is nigh impossible.

    // TODO: Eventually, battery power should also give a significant advantage to the speed or
    // effort of screw-turning, compared with manual wrist-twisting.

    WHEN( "tool has no battery" ) {
        REQUIRE_FALSE( drill.magazine_current() );

        // Screwing should work, but drilling should not
        THEN( "inherent qualities can be used" ) {
            CHECK( drill.has_quality( qual_SCREW, 1, 1 ) );
        }
        THEN( "charged tool qualities cannot be used" ) {
            CHECK_FALSE( drill.has_quality( qual_DRILL, 3, 1 ) );
        }
    }

    WHEN( "tool has a battery with zero charge" ) {
        // Get a dead battery
        battery.ammo_set( battery.ammo_default(), 0 );
        REQUIRE( battery.ammo_remaining( ) == 0 );
        // Install the battery in the drill
        drill.put_in( battery, pocket_type::MAGAZINE_WELL );
        REQUIRE( drill.magazine_current() );
        REQUIRE( drill.ammo_remaining( ) == 0 );

        // Screwing should work, but drilling should not
        THEN( "inherent qualities can be used" ) {
            CHECK( drill.has_quality( qual_SCREW, 1, 1 ) );
        }
        THEN( "charged tool qualities cannot be used" ) {
            CHECK_FALSE( drill.has_quality( qual_DRILL, 3, 1 ) );
        }
    }

    WHEN( "tool has a battery with enough charge for one use, equal to charges_per_use" ) {
        // Get a battery with exactly enough charge for one use
        int bat_charges = drill.type->charges_to_use();
        battery.ammo_set( battery.ammo_default(), bat_charges );
        REQUIRE( battery.ammo_remaining( ) == bat_charges );
        // Install the battery in the drill
        drill.put_in( battery, pocket_type::MAGAZINE_WELL );
        REQUIRE( drill.magazine_current() );
        REQUIRE( drill.ammo_remaining( ) == bat_charges );

        // Ensure there is enough charge
        REQUIRE( drill.type->charges_to_use() <= bat_charges );

        // Both screwing and drilling should work
        THEN( "both inherent and charged tool qualities can be used" ) {
            CHECK( drill.has_quality( qual_SCREW, 1, 1 ) );
            CHECK( drill.has_quality( qual_DRILL, 3, 1 ) );
        }
    }

    WHEN( "tool has a battery with insufficient charge, less than charges_per_use" ) {
        // Get a battery with too few charges by 1
        int bat_charges = drill.type->charges_to_use() - 1;
        battery.ammo_set( battery.ammo_default(), bat_charges );
        REQUIRE( battery.ammo_remaining( ) == bat_charges );
        // Install the battery in the drill
        drill.put_in( battery, pocket_type::MAGAZINE_WELL );
        REQUIRE( drill.magazine_current() );
        REQUIRE( drill.ammo_remaining( ) == bat_charges );

        // Ensure there is not enough charge
        REQUIRE( drill.type->charges_to_use() > bat_charges );

        // Screwing should still work, but drilling should not
        THEN( "inherent qualities can be used" ) {
            CHECK( drill.has_quality( qual_SCREW, 1, 1 ) );
        }
        THEN( "charged tool qualities cannot be used" ) {
            CHECK_FALSE( drill.has_quality( qual_DRILL, 3, 1 ) );
        }
    }

    SECTION( "charged tool qualities with UPS conversion mod" ) {
        // Need avatar as "carrier" for the UPS
        Character &they = get_player_character();
        clear_character( they );
        they.worn.wear_item( they, item( itype_debug_backpack ), false, false );

        // Use i_add to place everything in the avatar's inventory (backpack)
        // so the UPS power will be available to the cordless drill after modding
        item_location drill = they.i_add( item( itype_test_cordless_drill ) );
        item_location bat_cell = they.i_add( item( itype_heavy_battery_cell ) );
        item_location ups_mod = they.i_add( item( itype_battery_ups ) );
        item_location ups = they.i_add( item( itype_UPS_ON ) );

        GIVEN( "UPS has battery with enough charge, equal to drill's charges_per_use" ) {
            // Charge the battery
            int bat_charges = drill->type->charges_to_use();
            REQUIRE( bat_charges > 0 );
            bat_cell->ammo_set( bat_cell->ammo_default(), bat_charges );
            REQUIRE( bat_cell->ammo_remaining( ) == bat_charges );
            // Install heavy battery into UPS
            REQUIRE( ups->put_in( *bat_cell, pocket_type::MAGAZINE_WELL ).success() );
            REQUIRE( ups->ammo_remaining( &they ) == bat_charges );

            WHEN( "UPS battery mod is installed into the drill" ) {
                // Ensure drill currently has no mods
                REQUIRE( drill->toolmods().empty() );
                REQUIRE( drill->tname() == "test cordless drill" );
                // Install the UPS mod and ensure it worked
                drill->put_in( *ups_mod, pocket_type::MOD );
                REQUIRE_FALSE( drill->toolmods().empty() );
                REQUIRE( drill->tname() == "test cordless drill+1 (UPS)" );
                // Ensure avatar actually has the drill and UPS in possession
                CHECK( they.has_item( *drill ) );
                CHECK( they.has_item( *ups ) );

                THEN( "the drill has the same charge as the UPS" ) {
                    CHECK( drill->ammo_remaining( &they ) == bat_charges );
                }
                THEN( "inherent qualities of the drill can be used" ) {
                    CHECK( drill->has_quality( qual_SCREW, 1, 1 ) );
                }
                // Regression test for #54471
                THEN( "charged qualities of the drill can be used" ) {
                    CHECK( drill->has_quality( qual_DRILL, 3, 1 ) );
                }
            }
        }
    }
}

