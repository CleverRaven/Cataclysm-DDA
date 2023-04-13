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

static const vproto_id vehicle_prototype_bicycle( "bicycle" );

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
    vehicle *veh_ptr = get_map().add_vehicle( vehicle_prototype_bicycle, vehicle_origin, -90_degrees,
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

TEST_CASE( "repair_vehicle_part", "[vehicle]" )
{
    SECTION( "welder" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "welder", 500 ) );
        tools.emplace_back( "goggles_welding" );
        tools.emplace_back( "hammer" );
        tools.insert( tools.end(), 100, item( "scrap" ) );
        tools.insert( tools.end(), 10, item( "material_aluminium_ingot" ) );
        tools.insert( tools.end(), 50, item( "welding_wire_steel" ) );
        tools.insert( tools.end(), 50, item( "welding_wire_alloy" ) );
        test_repair( tools, true );
    }
    SECTION( "UPS_modded_welder" ) {
        std::vector<item> tools;
        item welder( "welder", calendar::turn_zero, 0 );
        welder.put_in( item( "battery_ups" ), item_pocket::pocket_type::MOD );
        tools.push_back( welder );

        item ups( "UPS_off" );
        item ups_mag( ups.magazine_default() );
        ups_mag.ammo_set( ups_mag.ammo_default(), 500 );
        ups.put_in( ups_mag, item_pocket::pocket_type::MAGAZINE_WELL );
        tools.push_back( ups );

        tools.emplace_back( "goggles_welding" );
        tools.emplace_back( "hammer" );
        tools.insert( tools.end(), 100, item( "scrap" ) );
        tools.insert( tools.end(), 10, item( "material_aluminium_ingot" ) );
        tools.insert( tools.end(), 50, item( "welding_wire_steel" ) );
        tools.insert( tools.end(), 50, item( "welding_wire_alloy" ) );
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

        item ups( "UPS_off" );
        item ups_mag( ups.magazine_default() );
        ups_mag.ammo_set( ups_mag.ammo_default(), 5 );
        ups.put_in( ups_mag, item_pocket::pocket_type::MAGAZINE_WELL );
        tools.push_back( ups );

        tools.emplace_back( "goggles_welding" );
        test_repair( tools, false );
    }
    SECTION( "welder_missing_consumables" ) {
        std::vector<item> tools;
        tools.push_back( tool_with_ammo( "welder", 500 ) );
        tools.emplace_back( "goggles_welding" );
        test_repair( tools, false );
    }
}
