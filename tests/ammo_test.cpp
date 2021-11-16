#include "cata_catch.h"

#include <set>

#include "item.h"
#include "type_id.h"

// Functions:
// - item::ammo_types
//
// TODO:
// - ammo_type
// - common_ammo_default
// - ammo_sort_name

// item::ammo_types returns the ammo types an item may *contain*, which is distinct from the ammo
// types an item may *use*.
//
// Only magazines have ammo_types().
//
// Tools, tool mods, and (all other item types?) have empty ammo_types()

// Return true if the given item has non-empty ammo_types
static bool has_ammo_types( const item &it )
{
    return !it.ammo_types().empty();
}

TEST_CASE( "ammo types", "[ammo][ammo_types]" )
{
    // Only a few kinds of item have ammo_types:
    // - Items with type=MAGAZINE (including batteries as well as gun magazines)
    // - Tools/weapons with magazine_integral (pocket_data has a MAGAZINE rather than MAGAZINE_WELL)

    SECTION( "items with MAGAZINE pockets have ammo_types" ) {
        // Batteries are magazines
        REQUIRE( item( "light_battery_cell" ).is_magazine() );
        REQUIRE( item( "battery_car" ).is_magazine() );

        // Tool batteries
        CHECK( has_ammo_types( item( "light_battery_cell" ) ) );
        CHECK( has_ammo_types( item( "medium_battery_cell" ) ) );
        CHECK( has_ammo_types( item( "heavy_battery_cell" ) ) );
        CHECK( has_ammo_types( item( "light_disposable_cell" ) ) );
        CHECK( has_ammo_types( item( "medium_disposable_cell" ) ) );
        CHECK( has_ammo_types( item( "heavy_disposable_cell" ) ) );
        // Vehicle batteries
        CHECK( has_ammo_types( item( "battery_car" ) ) );
        CHECK( has_ammo_types( item( "battery_motorbike" ) ) );
        CHECK( has_ammo_types( item( "large_storage_battery" ) ) );

        SECTION( "battery magazines include 'battery' ammo type" ) {
            CHECK( item( "light_battery_cell" ).ammo_types().count( ammotype( "battery" ) ) == 1 );
            CHECK( item( "battery_car" ).ammo_types().count( ammotype( "battery" ) ) == 1 );
        }

        // Gun magazines
        REQUIRE( item( "belt40mm" ).is_magazine() );
        REQUIRE( item( "akmag10" ).is_magazine() );

        CHECK( has_ammo_types( item( "belt40mm" ) ) );
        CHECK( has_ammo_types( item( "belt308" ) ) );
        CHECK( has_ammo_types( item( "akmag10" ) ) );
        CHECK( has_ammo_types( item( "akdrum75" ) ) );
        CHECK( has_ammo_types( item( "glockmag" ) ) );

        SECTION( "gun magazines include ammo type for that magazine" ) {
            CHECK( item( "glockmag" ).ammo_types().count( ammotype( "9mm" ) ) == 1 );
            CHECK( item( "akmag10" ).ammo_types().count( ammotype( "762" ) ) == 1 );
        }
    }

    SECTION( "GUN items with integral MAGAZINE pockets have ammo_types" ) {
        REQUIRE( item( "nailgun" ).magazine_integral() );
        REQUIRE( item( "colt_army" ).magazine_integral() );

        CHECK( has_ammo_types( item( "nailgun" ) ) );
        CHECK( has_ammo_types( item( "colt_army" ) ) );
        CHECK( has_ammo_types( item( "hand_crossbow" ) ) );
        CHECK( has_ammo_types( item( "compositebow" ) ) );
        CHECK( has_ammo_types( item( "sling" ) ) );
        CHECK( has_ammo_types( item( "slingshot" ) ) );
    }

    SECTION( "TOOL items with integral MAGAZINE pockets have ammo_types" ) {
        REQUIRE( item( "sewing_kit" ).magazine_integral() );

        CHECK( has_ammo_types( item( "needle_bone" ) ) );
        CHECK( has_ammo_types( item( "needle_wood" ) ) );
        CHECK( has_ammo_types( item( "sewing_kit" ) ) );
        CHECK( has_ammo_types( item( "tailors_kit" ) ) );
    }

    SECTION( "TOOL items with NO pockets have ammo_types" ) {
        // NOTE: Fish trap is a TOOL with "ammo", but no "pocket_data", so an implicit MAGAZINE
        // pocket is added by Item_factory::check_and_create_magazine_pockets on JSON load.
        // This item would be considered needing data migration to an explicit MAGAZINE pocket.
        REQUIRE( item( "fish_trap" ).magazine_integral() );

        CHECK( has_ammo_types( item( "fish_trap" ) ) );
    }

    // These items have NO ammo_types:

    SECTION( "GUN items with MAGAZINE_WELL pockets also have ammo_types" ) {
        REQUIRE_FALSE( item( "m1911" ).magazine_integral() );

        CHECK( has_ammo_types( item( "m1911" ) ) );
        CHECK( has_ammo_types( item( "usp_9mm" ) ) );
        CHECK( has_ammo_types( item( "tommygun" ) ) );
        CHECK( has_ammo_types( item( "ak74" ) ) );
        CHECK( has_ammo_types( item( "ak47" ) ) );
    }

    SECTION( "TOOL items with MAGAZINE_WELL pockets do NOT have ammo_types" ) {
        REQUIRE( item( "flashlight" ).is_tool() );

        CHECK_FALSE( has_ammo_types( item( "flashlight" ) ) );
        CHECK_FALSE( has_ammo_types( item( "hotplate" ) ) );
        CHECK_FALSE( has_ammo_types( item( "vac_sealer" ) ) );
        CHECK_FALSE( has_ammo_types( item( "forge" ) ) );
        CHECK_FALSE( has_ammo_types( item( "cordless_drill" ) ) );
    }

    SECTION( "AMMO items themselves do NOT have ammo_types" ) {
        REQUIRE( item( "38_special" ).is_ammo() );
        REQUIRE( item( "sinew" ).is_ammo() );

        // Ammo for guns
        CHECK_FALSE( has_ammo_types( item( "38_special" ) ) );
        CHECK_FALSE( has_ammo_types( item( "reloaded_308" ) ) );
        CHECK_FALSE( has_ammo_types( item( "bp_9mm" ) ) );
        CHECK_FALSE( has_ammo_types( item( "44magnum" ) ) );
        // Not for guns but classified as ammo
        CHECK_FALSE( has_ammo_types( item( "sinew" ) ) );
        CHECK_FALSE( has_ammo_types( item( "nail" ) ) );
        CHECK_FALSE( has_ammo_types( item( "rock" ) ) );
        CHECK_FALSE( has_ammo_types( item( "solder_wire" ) ) );
    }

    SECTION( "TOOLMOD items do NOT have ammo_types" ) {
        item med_mod( "magazine_battery_medium_mod" );
        REQUIRE( med_mod.is_toolmod() );

        CHECK_FALSE( has_ammo_types( med_mod ) );
    }
}

// The same items with no ammo_types, also have no ammo_default.
TEST_CASE( "ammo default", "[ammo][ammo_default]" )
{
    // TOOLMOD type, and TOOL type items with MAGAZINE_WELL pockets have no ammo_default
    SECTION( "items without ammo_default" ) {
        item flashlight( "flashlight" );
        item med_mod( "magazine_battery_medium_mod" );

        CHECK( flashlight.ammo_default().is_null() );
        CHECK( med_mod.ammo_default().is_null() );
    }

    // MAGAZINE type, and TOOL/GUN items with integral MAGAZINE pockets do have ammo_default
    SECTION( "items with ammo_default" ) {
        // MAGAZINE type items
        item battery( "light_battery_cell" );
        item glockmag( "glockmag" );
        CHECK( battery.ammo_default() == itype_id( "battery" ) );
        CHECK( glockmag.ammo_default() == itype_id( "9mm" ) );

        // TOOL type items with integral magazines
        item sewing_kit( "sewing_kit" );
        item needle( "needle_bone" );
        item fishtrap( "fish_trap" );
        CHECK( sewing_kit.ammo_default() == itype_id( "thread" ) );
        CHECK( needle.ammo_default() == itype_id( "thread" ) );
        CHECK( fishtrap.ammo_default() == itype_id( "fish_bait" ) );

        // GUN type items with integral magazine
        item slingshot( "slingshot" );
        item colt( "colt_army" );
        item lemat( "lemat_revolver" );
        CHECK( slingshot.ammo_default() == itype_id( "pebble" ) );
        // Revolver ammo is "44paper" but default ammunition type is "44army"
        CHECK( colt.ammo_default() == itype_id( "44army" ) );
        CHECK( lemat.ammo_default() == itype_id( "44army" ) );
    }
}

