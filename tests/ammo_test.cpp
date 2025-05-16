#include <set>
#include <string>

#include "cata_catch.h"
#include "coordinates.h"
#include "damage.h"
#include "item.h"
#include "pocket_type.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"

static const ammotype ammo_762( "762" );
static const ammotype ammo_9mm( "9mm" );
static const ammotype ammo_battery( "battery" );

static const itype_id itype_38_special( "38_special" );
static const itype_id itype_44army( "44army" );
static const itype_id itype_44magnum( "44magnum" );
static const itype_id itype_9mm( "9mm" );
static const itype_id itype_ak47( "ak47" );
static const itype_id itype_ak74_semi( "ak74_semi" );
static const itype_id itype_akdrum75( "akdrum75" );
static const itype_id itype_akmag10( "akmag10" );
static const itype_id itype_barrel_glock_short( "barrel_glock_short" );
static const itype_id itype_battery( "battery" );
static const itype_id itype_battery_car( "battery_car" );
static const itype_id itype_battery_motorbike( "battery_motorbike" );
static const itype_id itype_belt308( "belt308" );
static const itype_id itype_belt40mm( "belt40mm" );
static const itype_id itype_bp_9mm( "bp_9mm" );
static const itype_id itype_colt_army( "colt_army" );
static const itype_id itype_compositebow( "compositebow" );
static const itype_id itype_cordless_drill( "cordless_drill" );
static const itype_id itype_fish_bait( "fish_bait" );
static const itype_id itype_fish_trap( "fish_trap" );
static const itype_id itype_flashlight( "flashlight" );
static const itype_id itype_forge( "forge" );
static const itype_id itype_glockmag( "glockmag" );
static const itype_id itype_hand_crossbow( "hand_crossbow" );
static const itype_id itype_heavy_battery_cell( "heavy_battery_cell" );
static const itype_id itype_heavy_plus_battery_cell( "heavy_plus_battery_cell" );
static const itype_id itype_hotplate( "hotplate" );
static const itype_id itype_large_storage_battery( "large_storage_battery" );
static const itype_id itype_light_battery_cell( "light_battery_cell" );
static const itype_id itype_m1911( "m1911" );
static const itype_id itype_magazine_battery_medium_mod( "magazine_battery_medium_mod" );
static const itype_id itype_medium_battery_cell( "medium_battery_cell" );
static const itype_id itype_nail( "nail" );
static const itype_id itype_nailgun( "nailgun" );
static const itype_id itype_needle_bone( "needle_bone" );
static const itype_id itype_needle_wood( "needle_wood" );
static const itype_id itype_pebble( "pebble" );
static const itype_id itype_reloaded_308( "reloaded_308" );
static const itype_id itype_rock( "rock" );
static const itype_id itype_sewing_kit( "sewing_kit" );
static const itype_id itype_sinew( "sinew" );
static const itype_id itype_sling( "sling" );
static const itype_id itype_slingshot( "slingshot" );
static const itype_id itype_solder_wire( "solder_wire" );
static const itype_id itype_tailors_kit( "tailors_kit" );
static const itype_id itype_test_100mm_ammo( "test_100mm_ammo" );
static const itype_id itype_test_100mm_ammo_relative( "test_100mm_ammo_relative" );
static const itype_id itype_test_glock_super_long( "test_glock_super_long" );
static const itype_id itype_thread( "thread" );
static const itype_id itype_tommygun( "tommygun" );
static const itype_id itype_usp_9mm( "usp_9mm" );
static const itype_id itype_vac_sealer( "vac_sealer" );

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

TEST_CASE( "ammo_types", "[ammo][ammo_types]" )
{
    // Only a few kinds of item have ammo_types:
    // - Items with type=MAGAZINE (including batteries as well as gun magazines)
    // - Tools/weapons with magazine_integral (pocket_data has a MAGAZINE rather than MAGAZINE_WELL)

    SECTION( "items with MAGAZINE pockets have ammo_types" ) {
        // Batteries are magazines
        REQUIRE( item( itype_light_battery_cell ).is_magazine() );
        REQUIRE( item( itype_battery_car ).is_magazine() );

        // Tool batteries
        CHECK( has_ammo_types( item( itype_light_battery_cell ) ) );
        CHECK( has_ammo_types( item( itype_medium_battery_cell ) ) );
        CHECK( has_ammo_types( item( itype_heavy_battery_cell ) ) );
        CHECK( has_ammo_types( item( itype_heavy_plus_battery_cell ) ) );
        // Vehicle batteries
        CHECK( has_ammo_types( item( itype_battery_car ) ) );
        CHECK( has_ammo_types( item( itype_battery_motorbike ) ) );
        CHECK( has_ammo_types( item( itype_large_storage_battery ) ) );

        SECTION( "battery magazines include 'battery' ammo type" ) {
            CHECK( item( itype_light_battery_cell ).ammo_types().count( ammo_battery ) == 1 );
            CHECK( item( itype_battery_car ).ammo_types().count( ammo_battery ) == 1 );
        }

        // Gun magazines
        REQUIRE( item( itype_belt40mm ).is_magazine() );
        REQUIRE( item( itype_akmag10 ).is_magazine() );

        CHECK( has_ammo_types( item( itype_belt40mm ) ) );
        CHECK( has_ammo_types( item( itype_belt308 ) ) );
        CHECK( has_ammo_types( item( itype_akmag10 ) ) );
        CHECK( has_ammo_types( item( itype_akdrum75 ) ) );
        CHECK( has_ammo_types( item( itype_glockmag ) ) );

        SECTION( "gun magazines include ammo type for that magazine" ) {
            CHECK( item( itype_glockmag ).ammo_types().count( ammo_9mm ) == 1 );
            CHECK( item( itype_akmag10 ).ammo_types().count( ammo_762 ) == 1 );
        }
    }

    SECTION( "GUN items with integral MAGAZINE pockets have ammo_types" ) {
        REQUIRE( item( itype_nailgun ).magazine_integral() );
        REQUIRE( item( itype_colt_army ).magazine_integral() );

        CHECK( has_ammo_types( item( itype_nailgun ) ) );
        CHECK( has_ammo_types( item( itype_colt_army ) ) );
        CHECK( has_ammo_types( item( itype_hand_crossbow ) ) );
        CHECK( has_ammo_types( item( itype_compositebow ) ) );
        CHECK( has_ammo_types( item( itype_sling ) ) );
        CHECK( has_ammo_types( item( itype_slingshot ) ) );
    }

    SECTION( "TOOL items with integral MAGAZINE pockets have ammo_types" ) {
        REQUIRE( item( itype_sewing_kit ).magazine_integral() );

        CHECK( has_ammo_types( item( itype_needle_bone ) ) );
        CHECK( has_ammo_types( item( itype_needle_wood ) ) );
        CHECK( has_ammo_types( item( itype_sewing_kit ) ) );
        CHECK( has_ammo_types( item( itype_tailors_kit ) ) );
    }

    SECTION( "TOOL items with NO pockets have ammo_types" ) {
        // NOTE: Fish trap is a TOOL with "ammo", but no "pocket_data", so an implicit MAGAZINE
        // pocket is added by Item_factory::check_and_create_magazine_pockets on JSON load.
        // This item would be considered needing data migration to an explicit MAGAZINE pocket.
        REQUIRE( item( itype_fish_trap ).magazine_integral() );

        CHECK( has_ammo_types( item( itype_fish_trap ) ) );
    }

    // These items have NO ammo_types:

    SECTION( "GUN items with MAGAZINE_WELL pockets also have ammo_types" ) {
        REQUIRE_FALSE( item( itype_m1911 ).magazine_integral() );

        CHECK( has_ammo_types( item( itype_m1911 ) ) );
        CHECK( has_ammo_types( item( itype_usp_9mm ) ) );
        CHECK( has_ammo_types( item( itype_tommygun ) ) );
        CHECK( has_ammo_types( item( itype_ak74_semi ) ) );
        CHECK( has_ammo_types( item( itype_ak47 ) ) );
    }

    SECTION( "TOOL items with MAGAZINE_WELL pockets do NOT have ammo_types" ) {
        REQUIRE( item( itype_flashlight ).is_tool() );

        CHECK_FALSE( has_ammo_types( item( itype_flashlight ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_hotplate ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_vac_sealer ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_forge ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_cordless_drill ) ) );
    }

    SECTION( "AMMO items themselves do NOT have ammo_types" ) {
        REQUIRE( item( itype_38_special ).is_ammo() );
        REQUIRE( item( itype_sinew ).is_ammo() );

        // Ammo for guns
        CHECK_FALSE( has_ammo_types( item( itype_38_special ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_reloaded_308 ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_bp_9mm ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_44magnum ) ) );
        // Not for guns but classified as ammo
        CHECK_FALSE( has_ammo_types( item( itype_sinew ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_nail ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_rock ) ) );
        CHECK_FALSE( has_ammo_types( item( itype_solder_wire ) ) );
    }

    SECTION( "TOOLMOD items do NOT have ammo_types" ) {
        item med_mod( itype_magazine_battery_medium_mod );
        REQUIRE( med_mod.is_toolmod() );

        CHECK_FALSE( has_ammo_types( med_mod ) );
    }
}

// The same items with no ammo_types, also have no ammo_default.
TEST_CASE( "ammo_default", "[ammo][ammo_default]" )
{
    // TOOLMOD type, and TOOL type items with MAGAZINE_WELL pockets have no ammo_default
    SECTION( "items without ammo_default" ) {
        item flashlight( itype_flashlight );
        item med_mod( itype_magazine_battery_medium_mod );

        CHECK( flashlight.ammo_default().is_null() );
        CHECK( med_mod.ammo_default().is_null() );
    }

    // MAGAZINE type, and TOOL/GUN items with integral MAGAZINE pockets do have ammo_default
    SECTION( "items with ammo_default" ) {
        // MAGAZINE type items
        item battery( itype_light_battery_cell );
        item glockmag( itype_glockmag );
        CHECK( battery.ammo_default() == itype_battery );
        CHECK( glockmag.ammo_default() == itype_9mm );

        // TOOL type items with integral magazines
        item sewing_kit( itype_sewing_kit );
        item needle( itype_needle_bone );
        item fishtrap( itype_fish_trap );
        CHECK( sewing_kit.ammo_default() == itype_thread );
        CHECK( needle.ammo_default() == itype_thread );
        CHECK( fishtrap.ammo_default() == itype_fish_bait );

        // GUN type items with integral magazine
        item slingshot( itype_slingshot );
        item colt( itype_colt_army );
        CHECK( slingshot.ammo_default() == itype_pebble );
        // Revolver ammo is "44paper" but default ammunition type is "44army"
        CHECK( colt.ammo_default() == itype_44army );
    }
}

TEST_CASE( "barrel_test", "[ammo][weapon]" )
{
    SECTION( "basic ammo and barrel length test" ) {
        item base_gun( itype_test_glock_super_long );
        CHECK( base_gun.gun_damage( itype_test_100mm_ammo ).total_damage() == 65 );
    }

    SECTION( "basic ammo and mod length test" ) {
        item base_gun( itype_test_glock_super_long );
        item gun_mod( itype_barrel_glock_short );
        REQUIRE( base_gun.put_in( gun_mod, pocket_type::MOD ).success() );
        REQUIRE( base_gun.barrel_length().value() == 100 );
        CHECK( base_gun.gun_damage( itype_test_100mm_ammo ).total_damage() == 60 );
    }

    SECTION( "inherited ammo and barrel length test" ) {
        item base_gun( itype_test_glock_super_long );
        CHECK( base_gun.gun_damage( itype_test_100mm_ammo_relative ).total_damage() == 66 );
    }
}

TEST_CASE( "battery_energy_test", "[ammo][energy][item]" )
{
    item test_battery( itype_medium_battery_cell );
    test_battery.ammo_set( test_battery.ammo_default(), 56 );

    SECTION( "Integer drain from battery" ) {
        REQUIRE( test_battery.energy_remaining( nullptr ) == 56_kJ );
        units::energy consumed = test_battery.energy_consume( 40_kJ, tripoint_bub_ms::zero, nullptr );
        CHECK( test_battery.energy_remaining( nullptr ) == 16_kJ );
        CHECK( consumed == 40_kJ );
    }

    SECTION( "Integer over-drain from battery" ) {
        REQUIRE( test_battery.energy_remaining( nullptr ) == 56_kJ );
        units::energy consumed = test_battery.energy_consume( 400_kJ, tripoint_bub_ms::zero, nullptr );
        CHECK( test_battery.energy_remaining( nullptr ) == 0_kJ );
        CHECK( consumed == 56_kJ );
    }

    SECTION( "Non-integer drain from battery" ) {
        // Battery charge is in mJ now, so check for precise numbers.
        // 4.5 kJ drain is 4.5 kJ drain
        REQUIRE( test_battery.energy_remaining( nullptr ) == 56_kJ );
        units::energy consumed = test_battery.energy_consume( 4500_J, tripoint_bub_ms::zero, nullptr );
        CHECK( test_battery.energy_remaining( nullptr ) == 51.5_kJ );
        CHECK( consumed == 4500_J );
    }

    SECTION( "Tiny Non-integer drain from battery" ) {
        // Make sure lots of tiny discharges sum up as expected.
        REQUIRE( test_battery.energy_remaining( nullptr ) == 56_kJ );
        for( int i = 0; i < 133; ++i ) {
            units::energy consumed = test_battery.energy_consume( 2_J, tripoint_bub_ms::zero, nullptr );
            CHECK( consumed == 2_J );
        }
        CHECK( test_battery.energy_remaining( nullptr ) == 55734_J );
    }

    SECTION( "Non-integer over-drain from battery" ) {
        REQUIRE( test_battery.energy_remaining( nullptr ) == 56_kJ );
        units::energy consumed = test_battery.energy_consume( 500500_J, tripoint_bub_ms::zero, nullptr );
        CHECK( test_battery.energy_remaining( nullptr ) == 0_kJ );
        CHECK( consumed == 56_kJ );
    }

    SECTION( "zero drain from battery" ) {
        REQUIRE( test_battery.energy_remaining( nullptr ) == 56_kJ );
        units::energy consumed = test_battery.energy_consume( 0_J, tripoint_bub_ms::zero, nullptr );
        CHECK( test_battery.energy_remaining( nullptr ) == 56_kJ );
        CHECK( consumed == 0_kJ );
    }

}

