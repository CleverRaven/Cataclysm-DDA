#include <functional>
#include <iosfwd>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "inventory.h"
#include "item.h"
#include "item_pocket.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "point.h"
#include "requirements.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "veh_type.h"
#include "vehicle.h"

static void test_repair( const std::vector<item> &tools, bool expect_craftable )
{
    clear_avatar();
    clear_map();

    const tripoint test_origin( 60, 60, 0 );
    Character &player_character = get_player_character();
    player_character.setpos( test_origin );
    const item backpack( "backpack" );
    player_character.wear_item( backpack );
    for( const item &gear : tools ) {
        player_character.i_add( gear );
    }

    const tripoint vehicle_origin = test_origin + tripoint_south_east;
    vehicle *veh_ptr = get_map().add_vehicle( vproto_id( "bicycle" ), vehicle_origin, -90_degrees,
                       0, 0 );
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
    player_character.mod_moves( 1 );
    inventory crafting_inv = player_character.crafting_inventory();
    bool can_repair = vp.repair_requirements().can_make_with_inventory(
                          player_character.crafting_inventory(),
                          is_crafting_component );
    CHECK( can_repair == expect_craftable );
}

TEST_CASE( "repair_vehicle_part" )
{
    SECTION( "welder" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "welder", 500 ) );
        tools.emplace_back( "goggles_welding" );
        test_repair( tools, true );
    }
    SECTION( "UPS_modded_welder" ) {
        std::vector<item> tools;
        item welder( "welder", calendar::turn_zero, 0 );
        welder.put_in( item( "battery_ups" ), item_pocket::pocket_type::MOD );
        tools.push_back( welder );
        tools.emplace_back( "UPS_off", calendar::turn_zero, 500 );
        tools.emplace_back( "goggles_welding" );
        test_repair( tools, true );
    }
    SECTION( "welder_missing_goggles" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "welder", 500 ) );
        test_repair( tools, false );
    }
    SECTION( "welder_missing_charge" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "welder", 5 ) );
        tools.emplace_back( "goggles_welding" );
        test_repair( tools, false );
    }
    SECTION( "UPS_modded_welder_missing_charges" ) {
        std::vector<item> tools;
        item welder( "welder", calendar::turn_zero, 0 );
        welder.put_in( item( "battery_ups" ), item_pocket::pocket_type::MOD );
        tools.push_back( welder );
        tools.emplace_back( "UPS_off", calendar::turn_zero, 5 );
        tools.emplace_back( "goggles_welding" );
        test_repair( tools, false );
    }
}
