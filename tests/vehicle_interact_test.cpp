#include <list>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "requirements.h"
#include "veh_type.h"
#include "vehicle.h"
#include "calendar.h"
#include "inventory.h"
#include "item.h"
#include "type_id.h"
#include "point.h"

static void test_repair( const std::vector<item> &tools, bool expect_craftable )
{
    clear_player();
    clear_map();

    const tripoint test_origin( 60, 60, 0 );
    g->u.setpos( test_origin );
    const item backpack( "backpack" );
    g->u.wear( g->u.i_add( backpack ), false );
    for( const item gear : tools ) {
        g->u.i_add( gear );
    }

    const tripoint vehicle_origin = test_origin + tripoint_south_east;
    vehicle *veh_ptr = g->m.add_vehicle( vproto_id( "bicycle" ), vehicle_origin, -90, 0, 0 );
    REQUIRE( veh_ptr != nullptr );
    // Find the frame at the origin.
    vehicle_part *origin_frame = nullptr;
    for( vehicle_part *part : veh_ptr->get_parts_at( vehicle_origin, "", part_status_flag::any ) ) {
        if( part->info().location == "structure" ) {
            origin_frame = part;
            break;
        }
    }
    REQUIRE( origin_frame != nullptr );
    REQUIRE( origin_frame->hp() == origin_frame->info().durability );
    veh_ptr->mod_hp( *origin_frame, -100 );
    REQUIRE( origin_frame->hp() < origin_frame->info().durability );

    const vpart_info &vp = origin_frame->info();
    // Assertions about frame part?

    requirement_data reqs = vp.repair_requirements();
    // Bust cache on crafting_inventory()
    g->u.mod_moves( 1 );
    inventory crafting_inv = g->u.crafting_inventory();
    bool can_repair = vp.repair_requirements().can_make_with_inventory( g->u.crafting_inventory(),
                      is_crafting_component );
    CHECK( can_repair == expect_craftable );
}

TEST_CASE( "repair_vehicle_part" )
{
    SECTION( "welder" ) {
        std::vector<item> tools;
        tools.emplace_back( "welder", -1, 500 );
        tools.emplace_back( "goggles_welding" );
        test_repair( tools, true );
    }
    SECTION( "UPS_modded_welder" ) {
        std::vector<item> tools;
        item welder( "welder", -1, 0 );
        welder.contents.emplace_back( "battery_ups" );
        tools.push_back( welder );
        tools.emplace_back( "UPS_off", -1, 500 );
        tools.emplace_back( "goggles_welding" );
        test_repair( tools, true );
    }
    SECTION( "welder_missing_goggles" ) {
        std::vector<item> tools;
        tools.emplace_back( "welder", -1, 500 );
        test_repair( tools, false );
    }
    SECTION( "welder_missing_charge" ) {
        std::vector<item> tools;
        tools.emplace_back( "welder", -1, 5 );
        tools.emplace_back( "goggles_welding" );
        test_repair( tools, false );
    }
    SECTION( "UPS_modded_welder_missing_charges" ) {
        std::vector<item> tools;
        item welder( "welder", -1, 0 );
        welder.contents.emplace_back( "battery_ups" );
        tools.push_back( welder );
        tools.emplace_back( "UPS_off", -1, 5 );
        tools.emplace_back( "goggles_welding" );
        test_repair( tools, false );
    }
}

TEST_CASE("water faucet offers comestible liquids", "[water_faucet]") {
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin + tripoint_north;
    int battery_charge = 10000;

    clear_player();
    clear_map();

    g->place_player( test_origin );

    vehicle *veh_ptr = g->m.add_vehicle( vproto_id( "none" ), vehicle_origin, 0, 0, 0 );
    REQUIRE( veh_ptr != nullptr );

    REQUIRE( veh_ptr->install_part( point_zero, vpart_id( "storage_battery" ), true ) >= 0 );
    veh_ptr->charge_battery( battery_charge );

    REQUIRE( veh_ptr->install_part( point_zero, vpart_id( "tank_medium" ), true ) >= 0 );
    REQUIRE( veh_ptr->install_part( point_zero + point_north, vpart_id( "tank_medium" ), true ) >= 0 );
    REQUIRE( veh_ptr->install_part( point_zero + point_east, vpart_id( "tank_medium" ), true ) >= 0 );
    REQUIRE( veh_ptr->install_part( point_zero + point_west, vpart_id( "tank_medium" ), true ) >= 0 );

    std::vector<vehicle_part*> parts;

    vehicle_part* battery_ptr = nullptr;

    for( auto &part : veh_ptr->parts ) {
      if (part.is_tank()) {
        parts.push_back(std::addressof(part));
      }
      if (part.is_battery()) {
        battery_ptr = std::addressof(part);
      }
    }

    REQUIRE( battery_ptr != nullptr );
    REQUIRE( parts.size() == 4 );

    std::vector<item_count_tuple> liquids = veh_ptr->get_comestible_liquids();

    REQUIRE(liquids.empty());

    item item_water(itype_id("water"), calendar::turn, 4);
    item item_water_clean(itype_id("water_clean"), calendar::turn, 4);
    item item_pine_tea(itype_id("pine_tea"), calendar::turn, 4);

    parts[0]->ammo_set(item_water.typeId(), item_water.charges);
    parts[1]->ammo_set(item_water_clean.typeId(), item_water_clean.charges);
    parts[2]->ammo_set(item_pine_tea.typeId(), item_pine_tea.charges);

    liquids = veh_ptr->get_comestible_liquids();
    REQUIRE(liquids.size() == 2);

    // clean water
    REQUIRE(item_water_clean.charges == 4);

    veh_ptr->use_faucet(item_count_tuple(std::addressof(item_water_clean), item_water_clean.charges--), false, false);

    REQUIRE(parts[1]->ammo_remaining() == item_water_clean.charges);
    REQUIRE(battery_ptr->ammo_remaining() == battery_charge);

    REQUIRE(item_water_clean.charges == 3);

    veh_ptr->use_faucet(item_count_tuple(std::addressof(item_water_clean), item_water_clean.charges--), true, true);
    battery_charge--;

    REQUIRE(parts[1]->ammo_remaining() == item_water_clean.charges);
    REQUIRE(battery_ptr->ammo_remaining() == battery_charge);

    REQUIRE(item_water_clean.charges == 2);

    veh_ptr->use_faucet(item_count_tuple(std::addressof(item_water_clean), item_water_clean.charges--), true, false);

    REQUIRE(parts[1]->ammo_remaining() == item_water_clean.charges);
    REQUIRE(battery_ptr->ammo_remaining() == battery_charge);

    REQUIRE(item_water_clean.charges == 1);

    veh_ptr->use_faucet(item_count_tuple(std::addressof(item_water_clean), item_water_clean.charges--), false, true);
    battery_charge--;

    REQUIRE(parts[1]->ammo_remaining() == item_water_clean.charges);
    REQUIRE(battery_ptr->ammo_remaining() == battery_charge);

    REQUIRE(item_water_clean.charges == 0);

    liquids = veh_ptr->get_comestible_liquids();
    REQUIRE(liquids.size() == 1);

    // pine needle tea
    REQUIRE(item_pine_tea.charges == 4);

    veh_ptr->use_faucet(item_count_tuple(std::addressof(item_pine_tea), item_pine_tea.charges--), false, false);

    REQUIRE(parts[2]->ammo_remaining() == item_pine_tea.charges);
    REQUIRE(battery_ptr->ammo_remaining() == battery_charge);

    REQUIRE(item_pine_tea.charges == 3);

    veh_ptr->use_faucet(item_count_tuple(std::addressof(item_pine_tea), item_pine_tea.charges--), true, true);
    battery_charge--;

    REQUIRE(parts[2]->ammo_remaining() == item_pine_tea.charges);
    REQUIRE(battery_ptr->ammo_remaining() == battery_charge);

    REQUIRE(item_pine_tea.charges == 2);

    veh_ptr->use_faucet(item_count_tuple(std::addressof(item_pine_tea), item_pine_tea.charges--), true, false);
    battery_charge--;

    REQUIRE(parts[2]->ammo_remaining() == item_pine_tea.charges);
    REQUIRE(battery_ptr->ammo_remaining() == battery_charge);

    REQUIRE(item_pine_tea.charges == 1);

    veh_ptr->use_faucet(item_count_tuple(std::addressof(item_pine_tea), item_pine_tea.charges--), false, true);

    REQUIRE(parts[2]->ammo_remaining() == item_pine_tea.charges);
    REQUIRE(battery_ptr->ammo_remaining() == battery_charge);

    REQUIRE(item_pine_tea.charges == 0);

    liquids = veh_ptr->get_comestible_liquids();
    REQUIRE(liquids.size() == 0);
}
