#include <array>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "activity_type.h"
#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "damage.h"
#include "game.h"
#include "inventory.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "optional.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "recipe.h"
#include "requirements.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const activity_id ACT_CRAFT( "ACT_CRAFT" );

static const ammotype ammo_flammable( "flammable" );
static const ammotype ammo_water( "water" );

static const itype_id itype_fridge_test( "fridge_test" );
static const itype_id itype_metal_tank_test( "metal_tank_test" );
static const itype_id itype_oatmeal( "oatmeal" );
static const itype_id itype_water_clean( "water_clean" );

static const recipe_id recipe_oatmeal_cooked( "oatmeal_cooked" );

static const vpart_id vpart_ap_fridge_test( "ap_fridge_test" );
static const vpart_id vpart_halfboard_horizontal( "halfboard_horizontal" );
static const vpart_id vpart_tank_test( "tank_test" );

static const vproto_id vehicle_prototype_test_rv( "test_rv" );

static time_point midnight = calendar::turn_zero;
static time_point midday = midnight + 12_hours;

TEST_CASE( "verify_copy_from_gets_damage_reduction", "[vehicle]" )
{
    // Picking halfboard_horizontal as a vpart which is likely to remain
    // defined via copy-from, and which should have non-zero damage reduction.
    const vpart_info &vp = vpart_halfboard_horizontal.obj();
    CHECK( vp.damage_reduction[static_cast<int>( damage_type::BASH )] != 0 );
}

TEST_CASE( "vehicle_parts_seats_and_beds_have_beltable_flags", "[vehicle][vehicle_parts]" )
{
    // this checks all seats and beds either BELTABLE or NONBELTABLE but not both

    for( const auto &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;

        if( !vp.has_flag( "BED" ) && !vp.has_flag( "SEAT" ) ) {
            continue;
        }
        CAPTURE( vp.get_id().c_str() );
        CHECK( ( vp.has_flag( "BELTABLE" ) ^ vp.has_flag( "NONBELTABLE" ) ) );
    }
}

TEST_CASE( "vehicle_parts_boardable_openable_parts_have_door_flag", "[vehicle][vehicle_parts]" )
{
    // this checks all BOARDABLE and OPENABLE parts have DOOR flag

    for( const auto &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;

        if( !vp.has_flag( "BOARDABLE" ) || !vp.has_flag( "OPENABLE" ) ) {
            continue;
        }
        CAPTURE( vp.get_id().c_str() );
        CHECK( vp.has_flag( "DOOR" ) );
    }
}

TEST_CASE( "vehicle_parts_have_at_least_one_category", "[vehicle][vehicle_parts]" )
{
    // check parts have at least one category
    const std::vector<vpart_category> categories = vpart_category::all();
    std::set<std::string> all_cat_ids;
    for( const vpart_category &cat : categories ) {
        all_cat_ids.insert( cat.get_id() );
    }

    for( const auto &e : vpart_info::all() ) {
        const vpart_info &vp = e.second;
        CAPTURE( vp.get_id().c_str() );

        bool part_has_category = false;
        for( const vpart_category &cat : categories ) {
            if( vp.has_category( cat.get_id() ) ) {
                part_has_category = true;
                break;
            }
        }
        for( const std::string &cat : vp.get_categories() ) {
            const bool no_unknown_categories = all_cat_ids.find( cat ) == all_cat_ids.cend();
            CHECK_FALSE( no_unknown_categories );
        }
        CHECK( part_has_category );
    }
}

static void test_craft_via_rig( const std::vector<item> &items, int give_battery,
                                int expect_battery, int give_water, int expect_water, const recipe &recipe, bool expect_success )
{
    clear_avatar();
    clear_map();
    clear_vehicles();
    set_time( midday );

    const tripoint test_origin( 60, 60, 0 );
    Character &character = get_player_character();
    const item backpack( "backpack" );
    character.wear_item( backpack );
    for( const item &i : items ) {
        character.i_add( i );
    }
    character.set_skill_level( recipe.skill_used, recipe.difficulty );
    for( const std::pair<skill_id, int> req : recipe.required_skills ) {
        character.set_skill_level( req.first, req.second );
    }
    for( const recipe_proficiency &prof : recipe.proficiencies ) {
        character.add_proficiency( prof.id );
    }
    character.learn_recipe( &recipe );

    get_map().add_vehicle( vehicle_prototype_test_rv, test_origin, -90_degrees, 0, 0 );
    const optional_vpart_position &ovp = get_map().veh_at( test_origin );
    vehicle &veh = ovp->vehicle();
    REQUIRE( ovp.has_value() );

    REQUIRE( veh.fuel_left( itype_water_clean, true ) == 0 );
    for( const vpart_reference &tank : veh.get_avail_parts( vpart_bitflags::VPFLAG_FLUIDTANK ) ) {
        tank.part().ammo_set( itype_water_clean, give_water );
        break;
    }
    for( const vpart_reference &p : ovp->vehicle().get_all_parts() ) {
        if( p.has_feature( "DOOR" ) ) {
            veh.open( p.part_index() );
        }
        // seems it's not needed but just in case
        if( p.has_feature( "SOLAR_PANEL" ) ) {
            veh.set_hp( p.part(), 0, true );
        }
    }
    get_map().board_vehicle( test_origin, &character );

    veh.discharge_battery( 500000 );
    veh.charge_battery( give_battery );

    // Bust cache on crafting_inventory()
    character.mod_moves( 1 );

    const inventory &crafting_inv = character.crafting_inventory();
    bool can_craft = recipe
                     .deduped_requirements()
                     .can_make_with_inventory( crafting_inv, recipe.get_component_filter() );

    if( expect_success ) {
        REQUIRE( can_craft );

        REQUIRE_FALSE( character.is_armed() );
        character.make_craft( recipe.ident(), 1 );
        REQUIRE( character.activity );
        REQUIRE( character.activity.id() == ACT_CRAFT );
        while( character.activity.id() == ACT_CRAFT ) {
            character.moves = 100;
            character.activity.do_turn( character );
        }

        REQUIRE( character.get_wielded_item().type->get_id() == recipe.result() );
    } else {
        REQUIRE_FALSE( can_craft );
    }

    CHECK( veh.battery_power_level().first == expect_battery );
    CHECK( veh.fuel_left( itype_water_clean, true ) == expect_water );

    veh.unboard_all();
}

TEST_CASE( "craft_available_via_vehicle_rig", "[vehicle][vehicle_craft]" )
{
    SECTION( "cook oatmeal without oatmeal" ) {
        std::vector<item> items;

        test_craft_via_rig( items, 2, 2, 1, 1, recipe_oatmeal_cooked.obj(), false );
    }
    SECTION( "cook oatmeal without battery" ) {
        std::vector<item> items;
        items.emplace_back( itype_oatmeal );

        test_craft_via_rig( items, 0, 0, 1, 1, recipe_oatmeal_cooked.obj(), false );
    }
    SECTION( "cook oatmeal without water" ) {
        std::vector<item> items;
        items.emplace_back( itype_oatmeal );

        test_craft_via_rig( items, 2, 2, 0, 0, recipe_oatmeal_cooked.obj(), false );
    }
    SECTION( "cook oatmeal successfully" ) {
        std::vector<item> items;
        items.emplace_back( itype_oatmeal );

        test_craft_via_rig( items, 2, 0, 1, 0, recipe_oatmeal_cooked.obj(), true );
    }
}

static void check_part_ammo_capacity( vpart_id part_type, itype_id item_type, ammotype ammo_type,
                                      int expected_count )
{
    CAPTURE( part_type );
    CAPTURE( item_type );
    CAPTURE( ammo_type );
    vehicle_part test_part( part_type, "", point_zero, item( item_type ) );
    CHECK( expected_count == test_part.ammo_capacity( ammo_type ) );
}

TEST_CASE( "verify_vehicle_tank_refill", "[vehicle]" )
{
    check_part_ammo_capacity( vpart_ap_fridge_test, itype_fridge_test, ammo_water, 1600 );
    check_part_ammo_capacity( vpart_ap_fridge_test, itype_fridge_test, ammo_flammable, 444444 );
    check_part_ammo_capacity( vpart_tank_test, itype_metal_tank_test, ammo_water, 240 );
    check_part_ammo_capacity( vpart_tank_test, itype_metal_tank_test, ammo_flammable, 60000 );
}

TEST_CASE( "check_capacity_fueltype_handling", "[vehicle]" )
{
    GIVEN( "tank is empty" ) {
        vehicle_part vp( vpart_tank_test, "", point_zero, item( itype_metal_tank_test ) );
        REQUIRE( vp.ammo_remaining() == 0 );
        THEN( "ammo_current ammotype is always null" ) {
            CHECK( vp.ammo_current().is_null() );
            CHECK( !item::find_type( vp.ammo_current() )->ammo );
            // Segmentation fault:
            //vp.ammo_capacity( item::find_type( vp.ammo_current() )->ammo->type );
        }
        THEN( "using explicit ammotype for ammo_capacity returns expected value" ) {
            CHECK( vp.ammo_capacity( ammo_flammable ) == 60000 );
        }
    }

    GIVEN( "tank is not empty" ) {
        vehicle_part vp( vpart_tank_test, "", point_zero, item( itype_metal_tank_test ) );
        item tank( itype_metal_tank_test );
        REQUIRE( tank.fill_with( item( itype_water_clean ), 100 ) == 100 );
        vp.set_base( tank );
        REQUIRE( vp.ammo_remaining() == 100 );

        THEN( "ammo_current is not null" ) {
            CHECK( vp.ammo_current() == itype_water_clean );
            CHECK( !!item::find_type( vp.ammo_current() )->ammo );
            CHECK( vp.ammo_capacity( item::find_type( vp.ammo_current() )->ammo->type ) == 240 );
        }
    }
}
