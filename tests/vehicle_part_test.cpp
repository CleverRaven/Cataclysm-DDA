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

static const ammotype ammo_test_liquid( "test_liquid" );
static const ammotype ammo_water( "water" );

static const damage_type_id damage_bash( "bash" );

static const itype_id itype_backpack( "backpack" );
static const itype_id itype_fridge_test( "fridge_test" );
static const itype_id itype_gasoline( "gasoline" );
static const itype_id itype_metal_tank_test( "metal_tank_test" );
static const itype_id itype_oatmeal( "oatmeal" );
static const itype_id itype_test_multimag_direct_battery( "test_multimag_direct_battery" );
static const itype_id itype_test_multimag_mixed_battery( "test_multimag_mixed_battery" );
static const itype_id itype_test_multimag_two_battery( "test_multimag_two_battery" );
static const itype_id itype_test_multimag_two_fluid( "test_multimag_two_fluid" );
static const itype_id itype_test_multimag_vehicle_combo( "test_multimag_vehicle_combo" );
static const itype_id itype_test_multimag_well_fluid( "test_multimag_well_fluid" );
static const itype_id itype_water_clean( "water_clean" );
static const itype_id itype_water_faucet( "water_faucet" );
static const itype_id itype_water_purifier( "water_purifier" );

static const recipe_id recipe_oatmeal_cooked( "oatmeal_cooked" );

static const trait_id trait_DEBUG_CNF( "DEBUG_CNF" );

static const vpart_id vpart_ap_fridge_test( "ap_fridge_test" );
static const vpart_id vpart_frame( "frame" );
static const vpart_id vpart_halfboard( "halfboard" );
static const vpart_id vpart_small_storage_battery( "small_storage_battery" );
static const vpart_id vpart_tank_test( "tank_test" );

static const vproto_id vehicle_prototype_none( "none" );
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
    const std::vector<vpart_category> &categories = vpart_category::all();
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
    clear_map_without_vision();
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
    for( const recipe_proficiency &prof : recipe.get_proficiencies() ) {
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

TEST_CASE( "vehicle_prepare_tool_multimag", "[vehicle][multimag]" )
{
    clear_avatar();
    clear_map_without_vision();
    clear_vehicles();
    map &here = get_map();

    const tripoint_bub_ms test_origin( 60, 60, 0 );
    REQUIRE_FALSE( here.veh_at( test_origin ).has_value() );
    vehicle *veh = here.add_vehicle( vehicle_prototype_none, test_origin, 0_degrees, 0, 0 );
    REQUIRE( veh != nullptr );
    REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
    REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_small_storage_battery ) != -1 );
    REQUIRE( veh->install_part( here, point_rel_ms( 1, 0 ), vpart_frame ) != -1 );
    const int tank_idx = veh->install_part( here, point_rel_ms( 1, 0 ), vpart_tank_test );
    REQUIRE( tank_idx != -1 );
    veh->refresh();
    here.add_vehicle_to_cache( veh );

    veh->charge_battery( here, 1000 );
    veh->part( tank_idx ).ammo_set( itype_gasoline, 50 );

    SECTION( "prepare_tool populates battery + tank pockets" ) {
        item welder( itype_test_multimag_vehicle_combo );
        veh->prepare_tool( here, welder );
        CHECK( welder.ammo_remaining_in_pocket( "power" ) > 0 );
        CHECK( welder.ammo_remaining_in_pocket( "fluid" ) > 0 );
    }

    SECTION( "MAGAZINE_WELL non-battery pocket synthesizes magazine from vehicle tank fluid" ) {
        // Vehicle tank holds gasoline; tool's fuel pocket is MAGAZINE_WELL
        // accepting a fluid magazine. Prep must synthesize the magazine and
        // fill it from the tank, recording a TANK binding so drain_back
        // bills the same vpart.
        item tool( itype_test_multimag_well_fluid );
        const std::map<std::string, multimag_pocket_state> bindings =
            vehicle::prepare_multimag_pockets( *veh, here, tool );
        REQUIRE( bindings.count( "fuel" ) );
        CHECK( bindings.at( "fuel" ).kind == multimag_pocket_state::source_kind::TANK );
        CHECK( bindings.at( "fuel" ).vpart_index == tank_idx );
        CHECK( bindings.at( "fuel" ).initial_qty > 0 );

        const int tank_before = veh->part( tank_idx ).ammo_remaining();
        REQUIRE( tool.consume_tool_uses( 1, here, test_origin, nullptr ) == 1 );
        vehicle::drain_back_multimag( *veh, here, tool, bindings );
        // per_use=3 -> tank drained by exactly 3.
        CHECK( veh->part( tank_idx ).ammo_remaining() == tank_before - 3 );
    }

    SECTION( "two same-fuel tanks bind each pocket to its own vpart" ) {
        // Install a second gasoline tank, partially fill both, give the tool
        // two same-fuel tank pockets. Each pocket must bind to one specific
        // tank: prep claims per-vpart, drain-back bills per-vpart.
        REQUIRE( veh->install_part( here, point_rel_ms( 1, 1 ), vpart_frame ) != -1 );
        const int tank2_idx = veh->install_part( here, point_rel_ms( 1, 1 ), vpart_tank_test );
        REQUIRE( tank2_idx != -1 );
        veh->refresh();
        veh->part( tank_idx ).ammo_set( itype_gasoline, 6 );
        veh->part( tank2_idx ).ammo_set( itype_gasoline, 6 );

        item tool( itype_test_multimag_two_fluid );
        const std::map<std::string, multimag_pocket_state> bindings =
            vehicle::prepare_multimag_pockets( *veh, here, tool );
        REQUIRE( bindings.count( "left" ) );
        REQUIRE( bindings.count( "right" ) );
        // Each pocket must be backed by a distinct vpart_index.
        CHECK( bindings.at( "left" ).vpart_index != bindings.at( "right" ).vpart_index );
        // Each pocket gets the per-tank total, not double the global pool.
        CHECK( bindings.at( "left" ).initial_qty == 6 );
        CHECK( bindings.at( "right" ).initial_qty == 6 );

        const int tank_a_before = veh->part( tank_idx ).ammo_remaining();
        const int tank_b_before = veh->part( tank2_idx ).ammo_remaining();
        REQUIRE( tool.consume_tool_uses( 1, here, test_origin, nullptr ) == 1 );
        vehicle::drain_back_multimag( *veh, here, tool, bindings );
        // Drain bills each pocket's bound tank exactly per_use=2.
        CHECK( veh->part( tank_idx ).ammo_remaining() == tank_a_before - 2 );
        CHECK( veh->part( tank2_idx ).ammo_remaining() == tank_b_before - 2 );
    }

    SECTION( "two battery pockets share one network without double-claim" ) {
        // Two battery wells share one vehicle battery network. Per-use cost
        // sums to 10, vehicle has 8: load must split, not double-claim, so
        // feasibility is 0 and pocket totals stay within the network budget.
        item tool( itype_test_multimag_two_battery );
        veh->discharge_battery( here, 100000 );
        veh->charge_battery( here, 8 );
        REQUIRE( static_cast<int>( veh->battery_left( here ) ) == 8 );

        const std::map<std::string, multimag_pocket_state> bindings =
            vehicle::prepare_multimag_pockets( *veh, here, tool );
        const int left = bindings.count( "left" ) ? bindings.at( "left" ).initial_qty : 0;
        const int right = bindings.count( "right" ) ? bindings.at( "right" ).initial_qty : 0;
        CHECK( left + right <= 8 );
        CHECK( tool.feasible_tool_uses( /*external_pool=*/0 ) == 0 );
    }

    SECTION( "direct MAGAZINE battery pocket caps at pocket capacity, not vehicle pool" ) {
        // Vehicle pool (1000) >> pocket capacity (40). Prep must cap at the
        // pocket's own ammo_restriction limit; an uncapped insert would fail.
        item tool( itype_test_multimag_direct_battery );
        veh->discharge_battery( here, 100000 );
        veh->charge_battery( here, 1000 );
        REQUIRE( static_cast<int>( veh->battery_left( here ) ) == 1000 );

        const std::map<std::string, multimag_pocket_state> bindings =
            vehicle::prepare_multimag_pockets( *veh, here, tool );
        REQUIRE( bindings.count( "core" ) );
        CHECK( bindings.at( "core" ).initial_qty == 40 );
        CHECK( tool.ammo_remaining_in_pocket( "core" ) == 40 );

        const int batt_after_prep = static_cast<int>( veh->battery_left( here ) );
        CHECK( batt_after_prep == 1000 );

        REQUIRE( tool.consume_tool_uses( 1, here, test_origin, nullptr ) == 1 );
        vehicle::drain_back_multimag( *veh, here, tool, bindings );
        CHECK( static_cast<int>( veh->battery_left( here ) ) == 1000 - 5 );
    }

    SECTION( "mixed MAGAZINE_WELL + direct MAGAZINE battery pockets account claims correctly" ) {
        // Vehicle has 350 battery. Well caps at heavy_battery_cell capacity
        // (259); direct caps at its own 40 ammo_restriction. Sum (299) fits;
        // direct must NOT be underfilled by a double-subtract bug.
        item tool( itype_test_multimag_mixed_battery );
        veh->discharge_battery( here, 100000 );
        veh->charge_battery( here, 350 );
        REQUIRE( static_cast<int>( veh->battery_left( here ) ) == 350 );

        const std::map<std::string, multimag_pocket_state> bindings =
            vehicle::prepare_multimag_pockets( *veh, here, tool );
        REQUIRE( bindings.count( "well" ) );
        REQUIRE( bindings.count( "direct" ) );
        const int well_qty = bindings.at( "well" ).initial_qty;
        const int direct_qty = bindings.at( "direct" ).initial_qty;
        CHECK( well_qty == 259 );
        CHECK( direct_qty == 40 );
    }

    SECTION( "use_vehicle_tool precheck rejects multimag with no feasible use" ) {
        // Empty every source: feasible_tool_uses must report 0 and
        // use_vehicle_tool must refuse without invoking the tool.
        veh->discharge_battery( here, 100000 );
        for( const vpart_reference &tvp :
             veh->get_avail_parts( vpart_bitflags::VPFLAG_FLUIDTANK ) ) {
            tvp.part().ammo_unset();
        }
        REQUIRE( veh->battery_left( here ) == 0 );

        const bool ok = vehicle::use_vehicle_tool( *veh, &here, test_origin,
                        itype_test_multimag_vehicle_combo, /*no_invoke=*/true );
        CHECK_FALSE( ok );
    }

    SECTION( "drain_back_multimag drains battery + tank per pocket" ) {
        item welder( itype_test_multimag_vehicle_combo );
        const std::map<std::string, multimag_pocket_state> bindings =
            vehicle::prepare_multimag_pockets( *veh, here, welder );
        REQUIRE( bindings.at( "power" ).initial_qty > 0 );
        REQUIRE( bindings.at( "fluid" ).initial_qty > 0 );

        const int batt_before = static_cast<int>( veh->battery_left( here ) );
        const int gas_before = static_cast<int>( veh->fuel_left( here, itype_gasoline ) );
        REQUIRE( batt_before > 0 );
        REQUIRE( gas_before > 0 );

        // No carrier and no cable, so consume_tool_uses pulls from local pockets only.
        REQUIRE( welder.consume_tool_uses( 1, here, test_origin, nullptr ) == 1 );
        vehicle::drain_back_multimag( *veh, here, welder, bindings );

        // Per-use cost: power=5, fluid=2.
        CHECK( static_cast<int>( veh->battery_left( here ) ) == batt_before - 5 );
        CHECK( static_cast<int>( veh->fuel_left( here, itype_gasoline ) ) == gas_before - 2 );
    }
}

TEST_CASE( "vehicle_run_legacy_charge_tool_uses_water_purifier",
           "[vehicle][purifier]" )
{
    clear_avatar();
    clear_map_without_vision();
    clear_vehicles();
    map &here = get_map();

    const tripoint_bub_ms test_origin( 60, 60, 0 );
    REQUIRE_FALSE( here.veh_at( test_origin ).has_value() );
    vehicle *veh = here.add_vehicle( vehicle_prototype_none, test_origin, 0_degrees, 0, 0 );
    REQUIRE( veh != nullptr );
    REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_frame ) != -1 );
    REQUIRE( veh->install_part( here, point_rel_ms::zero, vpart_small_storage_battery ) != -1 );
    veh->refresh();
    here.add_vehicle_to_cache( veh );

    const int per_use = itype_water_purifier->charges_to_use();
    REQUIRE( per_use > 0 );

    SECTION( "drains battery exactly per_use * uses on success" ) {
        veh->discharge_battery( here, 100000 );
        const int budget = per_use * 3;
        veh->charge_battery( here, budget );
        REQUIRE( static_cast<int>( veh->battery_left( here ) ) == budget );

        const int got = veh->run_legacy_charge_tool_uses( here, itype_water_purifier, 3 );
        CHECK( got == 3 );
        CHECK( static_cast<int>( veh->battery_left( here ) ) == 0 );
    }

    SECTION( "caps uses by available battery" ) {
        veh->discharge_battery( here, 100000 );
        veh->charge_battery( here, per_use * 2 );
        const int got = veh->run_legacy_charge_tool_uses( here, itype_water_purifier, 5 );
        CHECK( got == 2 );
        CHECK( veh->battery_left( here ) == 0 );
    }

    SECTION( "zero battery returns zero uses, drains nothing" ) {
        veh->discharge_battery( here, 100000 );
        REQUIRE( veh->battery_left( here ) == 0 );
        const int got = veh->run_legacy_charge_tool_uses( here, itype_water_purifier, 5 );
        CHECK( got == 0 );
    }

    SECTION( "menu purify pre-check refuses partial budget without burning power" ) {
        veh->discharge_battery( here, 100000 );
        veh->charge_battery( here, per_use * 2 );
        const int batt_before = static_cast<int>( veh->battery_left( here ) );
        const int requested = 5;
        REQUIRE_FALSE( batt_before >= requested * per_use );
        CHECK( static_cast<int>( veh->battery_left( here ) ) == batt_before );
    }
}

TEST_CASE( "faucet_offers_cold_water", "[vehicle][vehicle_parts]" )
{
    clear_avatar();
    clear_map_without_vision();
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
    check_part_ammo_capacity( vpart_ap_fridge_test, itype_fridge_test, ammo_test_liquid, 444444 );
    check_part_ammo_capacity( vpart_tank_test, itype_metal_tank_test, ammo_water, 240 );
    check_part_ammo_capacity( vpart_tank_test, itype_metal_tank_test, ammo_test_liquid, 60000 );
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
            CHECK( vp.ammo_capacity( ammo_test_liquid ) == 60000 );
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
