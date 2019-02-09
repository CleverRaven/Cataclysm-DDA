#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "player.h"
#include "player_helpers.h"
#include "requirements.h"
#include "veh_type.h"
#include "vehicle.h"

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

    const tripoint vehicle_origin = test_origin + tripoint( 1, 1, 0 );
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
    bool can_repair = vp.repair_requirements().can_make_with_inventory( g->u.crafting_inventory() );
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
