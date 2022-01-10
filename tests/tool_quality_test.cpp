#include "cata_catch.h"
#include "itype.h"
#include "player_helpers.h"
#include "type_id.h"

static const quality_id qual_BOIL( "BOIL" );
static const quality_id qual_DRILL( "DRILL" );
static const quality_id qual_SCREW( "SCREW" );

// Tools may have a list of "qualities" with things they can do
TEST_CASE( "check-tool_qualities", "[tool][quality]" )
{
    item mess_kit = tool_with_ammo( "mess_kit", 20 );
    item surv_mess_kit = tool_with_ammo( "survivor_mess_kit", 20 );

    // Tools that can BOIL and CONTAIN will have BOIL quality only when empty
    REQUIRE( mess_kit.empty_container() );
    REQUIRE( surv_mess_kit.empty_container() );
    CHECK( mess_kit.get_quality( qual_BOIL ) == 2 );
    CHECK( surv_mess_kit.get_quality( qual_BOIL ) == 2 );
    // TODO: When they contain something, they lose their BOIL quality

    // Without charges, the cordless drill cannot drill
    CHECK( tool_with_ammo( "test_cordless_drill", 0 ).get_quality( qual_DRILL ) == 0 );
    // But with charges, it can drill
    CHECK( tool_with_ammo( "test_cordless_drill", 20 ).get_quality( qual_DRILL ) == 3 );
}

// Tools that run on battery power (or are otherwise "charged") may have "charged_qualities"
// that are only available when the item is charged with at least "charges_per_use" charges.
TEST_CASE( "battery-powered tool qualities", "[tool][battery][quality]" )
{
    item drill( "test_cordless_drill" );
    item battery( "medium_battery_cell" );

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
        REQUIRE( battery.ammo_remaining() == 0 );
        // Install the battery in the drill
        drill.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );
        REQUIRE( drill.magazine_current() );
        REQUIRE( drill.ammo_remaining() == 0 );

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
        REQUIRE( battery.ammo_remaining() == bat_charges );
        // Install the battery in the drill
        drill.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );
        REQUIRE( drill.magazine_current() );
        REQUIRE( drill.ammo_remaining() == bat_charges );

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
        REQUIRE( battery.ammo_remaining() == bat_charges );
        // Install the battery in the drill
        drill.put_in( battery, item_pocket::pocket_type::MAGAZINE_WELL );
        REQUIRE( drill.magazine_current() );
        REQUIRE( drill.ammo_remaining() == bat_charges );

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
}

