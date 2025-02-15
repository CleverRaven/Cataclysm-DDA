#include <algorithm>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "player_activity.h"
#include "player_helpers.h"
#include "point.h"
#include "recipe.h"
#include "requirements.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "veh_utils.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const activity_id ACT_CRAFT( "ACT_CRAFT" );

static const ammotype ammo_flammable( "flammable" );
static const ammotype ammo_water( "water" );

static const damage_type_id damage_bash( "bash" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_fridge_test( "fridge_test" );
static const itype_id itype_metal_tank_test( "metal_tank_test" );
static const itype_id itype_oatmeal( "oatmeal" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_water_faucet( "water_faucet" );

static const recipe_id recipe_oatmeal_cooked( "oatmeal_cooked" );

static const trait_id trait_DEBUG_CNF( "DEBUG_CNF" );

static const vpart_id vpart_ap_fridge_test( "ap_fridge_test" );
static const vpart_id vpart_halfboard( "halfboard" );
static const vpart_id vpart_tank_test( "tank_test" );

static const vproto_id vehicle_prototype_test_rv( "test_rv" );

static time_point midnight = calendar::turn_zero;
static time_point midday = midnight + 12_hours;

TEST_CASE( "verify_copy_from_gets_damage_reduction", "[vehicle]" )
{
    // Picking halfboard_horizontal as a vpart which is likely to remain
    // defined via copy-from, and which should have non-zero damage reduction.
    CHECK( vpart_halfboard->damage_reduction.at( damage_bash ) != 0.f );
}

TEST_CASE( "vehicle_parts_seats_and_beds_have_beltable_flags", "[vehicle][vehicle_parts]" )
{
    // this checks all seats and beds either BELTABLE or NONBELTABLE but not both

    for( const vpart_info &vpi : vehicles::parts::get_all() ) {
        if( !vpi.has_flag( "BED" ) && !vpi.has_flag( "SEAT" ) ) {
            continue;
        }
        CAPTURE( vpi.id.str() );
        CHECK( ( vpi.has_flag( "BELTABLE" ) ^ vpi.has_flag( "NONBELTABLE" ) ) );
    }
}

TEST_CASE( "vehicle_parts_boardable_openable_parts_have_door_flag", "[vehicle][vehicle_parts]" )
{
    // this checks all BOARDABLE and OPENABLE parts have DOOR flag

    for( const vpart_info &vpi : vehicles::parts::get_all() ) {
        if( !vpi.has_flag( "BOARDABLE" ) || !vpi.has_flag( "OPENABLE" ) ) {
            continue;
        }
        CAPTURE( vpi.id.str() );
        CHECK( vpi.has_flag( "DOOR" ) );
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

    for( const vpart_info &vpi : vehicles::parts::get_all() ) {
        CAPTURE( vpi.id.str() );

        bool part_has_category = false;
        for( const vpart_category &cat : categories ) {
            if( vpi.has_category( cat.get_id() ) ) {
                part_has_category = true;
                break;
            }
        }
        for( const std::string &cat : vpi.get_categories() ) {
            const bool no_unknown_categories = all_cat_ids.find( cat ) == all_cat_ids.cend();
            CHECK_FALSE( no_unknown_categories );
        }
        CHECK( part_has_category );
    }
}

static void test_craft_via_rig( const std::vector<item> &items, int give_battery,
                                int expect_battery, int give_water, int expect_water, const recipe &recipe, bool expect_success )
{
    map &here = get_map();
    clear_avatar();
    clear_map();
    clear_vehicles();
    set_time_to_day();

    const tripoint_bub_ms test_origin( 60, 60, 0 );
    Character &character = get_player_character();
    character.toggle_trait( trait_DEBUG_CNF );
    const item backpack( itype_backpack );
    character.wear_item( backpack );
    for( const item &i : items ) {
        character.i_add( i );
    }
    // Shift skill levels by one to ensure successful crafting
    // after the change in https://github.com/CleverRaven/Cataclysm-DDA/pull/61985
    character.set_skill_level( recipe.skill_used, recipe.difficulty + 1 );
    for( const std::pair<const skill_id, int> &req : recipe.required_skills ) {
        character.set_skill_level( req.first, req.second + 1 );
    }
    for( const recipe_proficiency &prof : recipe.proficiencies ) {
        character.add_proficiency( prof.id );
    }
    character.learn_recipe( &recipe );

    here.add_vehicle( vehicle_prototype_test_rv, test_origin, -90_degrees, 0, 0 );
    const optional_vpart_position ovp = here.veh_at( test_origin );
    REQUIRE( ovp.has_value() );
    vehicle &veh = ovp->vehicle();

    REQUIRE( veh.fuel_left( here, itype_water_clean ) == 0 );
    for( const vpart_reference &tank : veh.get_avail_parts( vpart_bitflags::VPFLAG_FLUIDTANK ) ) {
        tank.part().ammo_set( itype_water_clean, give_water );
        break;
    }
    for( const vpart_reference &p : ovp->vehicle().get_all_parts() ) {
        if( p.has_feature( "DOOR" ) ) {
            veh.open( here, p.part_index() );
        }
        // seems it's not needed but just in case
        if( p.has_feature( "SOLAR_PANEL" ) ) {
            veh.set_hp( p.part(), 0, true );
        }
    }
    here.board_vehicle( test_origin, &character );

    veh.discharge_battery( here, 500000 );
    veh.charge_battery( here, give_battery );

    character.invalidate_crafting_inventory();
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
            character.set_moves( 100 );
            character.activity.do_turn( character );
        }

        REQUIRE( character.get_wielded_item()->type->get_id() == recipe.result() );
    } else {
        REQUIRE_FALSE( can_craft );
    }

    CHECK( veh.battery_power_level( ).first == expect_battery );
    CHECK( veh.fuel_left( here, itype_water_clean ) == expect_water );

    veh.unboard_all( here );
}

TEST_CASE( "faucet_offers_cold_water", "[vehicle][vehicle_parts]" )
{
    clear_avatar();
    clear_map();
    clear_vehicles();
    set_time( midday );
    map &here = get_map();

    const tripoint_bub_ms test_origin( 60, 60, 0 );
    const int water_charges = 8;
    Character &character = get_player_character();
    const item backpack( itype_backpack );
    character.wear_item( backpack );
    here.add_vehicle( vehicle_prototype_test_rv, test_origin, -90_degrees, 0, 0 );
    const optional_vpart_position ovp = here.veh_at( test_origin );
    REQUIRE( ovp.has_value() );
    vehicle &veh = ovp->vehicle();

    REQUIRE( veh.fuel_left( here, itype_water_clean ) == 0 );
    item *tank_it = nullptr;
    for( const vpart_reference &tank : veh.get_avail_parts( vpart_bitflags::VPFLAG_FLUIDTANK ) ) {
        tank.part().ammo_set( itype_water_clean, water_charges );
        tank_it = const_cast<item *>( &tank.part().get_base() );
        tank_it->only_item().cold_up();
        break;
    }
    REQUIRE( tank_it != nullptr );
    REQUIRE( veh.fuel_left( here, itype_water_clean ) == static_cast<int64_t>( water_charges ) );

    std::optional<vpart_reference> faucet;
    for( const vpart_reference &vpr : veh.get_all_parts() ) {
        faucet = vpr.part_with_tool( here, itype_water_faucet );
        if( faucet.has_value() ) {
            break;
        }
    }
    REQUIRE( faucet.has_value() );
    here.board_vehicle( faucet->pos_bub( here ) + tripoint::east, &character );
    veh_menu menu( veh, "TEST" );
    for( int i = 0; i < water_charges; i++ ) {
        CAPTURE( i, veh.fuel_left( here, itype_water_clean ) );
        menu.reset();
        veh.build_interact_menu( menu, &here, faucet->pos_bub( here ), false );
        const std::vector<veh_menu_item> items = menu.get_items();
        const bool stomach_should_be_full = i == water_charges - 1;
        const auto drink_item_it = std::find_if( items.begin(), items.end(),
        []( const veh_menu_item & it ) {
            return it._text == "Have a drink";
        } );
        REQUIRE( veh.fuel_left( here, itype_water_clean ) == ( water_charges - i ) );
        REQUIRE( drink_item_it != items.end() );
        REQUIRE( drink_item_it->_enabled == !stomach_should_be_full ); // stomach should be full
        REQUIRE( character.get_morale_level() == ( i != 0 ? 1 : 0 ) ); // bonus morale from cold water
        if( stomach_should_be_full ) {
            clear_avatar(); // clear stomach
        }
        drink_item_it->_on_submit();
        character.clear_morale();
        process_activity( character );
        REQUIRE( character.get_morale_level() == 1 );
    }
    REQUIRE( veh.fuel_left( here, itype_water_clean ) == 0 );
    REQUIRE( tank_it->empty_container() );
    here.destroy_vehicle( &veh );
}

TEST_CASE( "craft_available_via_vehicle_rig", "[vehicle][vehicle_craft]" )
{
    SECTION( "cook oatmeal without oatmeal" ) {
        std::vector<item> items;

        test_craft_via_rig( items, 105, 105, 1, 1, recipe_oatmeal_cooked.obj(), false );
    }
    SECTION( "cook oatmeal without battery" ) {
        std::vector<item> items;
        items.emplace_back( itype_oatmeal );

        test_craft_via_rig( items, 0, 0, 1, 1, recipe_oatmeal_cooked.obj(), false );
    }
    SECTION( "cook oatmeal without water" ) {
        std::vector<item> items;
        items.emplace_back( itype_oatmeal );

        test_craft_via_rig( items, 105, 105, 0, 0, recipe_oatmeal_cooked.obj(), false );
    }
    SECTION( "cook oatmeal successfully" ) {
        std::vector<item> items;
        items.emplace_back( itype_oatmeal );

        test_craft_via_rig( items, 105, 0, 1, 0, recipe_oatmeal_cooked.obj(), true );
    }
}

static void check_part_ammo_capacity( vpart_id part_type, itype_id item_type, ammotype ammo_type,
                                      int expected_count )
{
    CAPTURE( part_type );
    CAPTURE( item_type );
    CAPTURE( ammo_type );
    vehicle_part test_part( part_type, item( item_type ) );
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
        vehicle_part vp( vpart_tank_test, item( itype_metal_tank_test ) );
        REQUIRE( vp.ammo_remaining( ) == 0 );
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
        vehicle_part vp( vpart_tank_test, item( itype_metal_tank_test ) );
        item tank( itype_metal_tank_test );
        REQUIRE( tank.fill_with( item( itype_water_clean ), 100 ) == 100 );
        vp.set_base( std::move( tank ) );
        REQUIRE( vp.ammo_remaining( ) == 100 );

        THEN( "ammo_current is not null" ) {
            CHECK( vp.ammo_current() == itype_water_clean );
            CHECK( !!item::find_type( vp.ammo_current() )->ammo );
            CHECK( vp.ammo_capacity( item::find_type( vp.ammo_current() )->ammo->type ) == 240 );
        }
    }
}
