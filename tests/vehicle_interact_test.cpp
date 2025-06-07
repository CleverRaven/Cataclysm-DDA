#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "enums.h"
#include "inventory.h"
#include "item.h"
#include "item_location.h"
#include "map.h"
#include "map_helpers.h"
#include "player_helpers.h"
#include "pocket_type.h"
#include "point.h"
#include "requirements.h"
#include "ret_val.h"
#include "type_id.h"
#include "units.h"
#include "veh_appliance.h"
#include "veh_type.h"
#include "vehicle.h"

static const itype_id itype_UPS_ON( "UPS_ON" );
static const itype_id itype_battery_ups( "battery_ups" );
static const itype_id itype_debug_backpack( "debug_backpack" );
static const itype_id itype_goggles_welding( "goggles_welding" );
static const itype_id itype_hammer( "hammer" );
static const itype_id itype_lc_steel_chunk( "lc_steel_chunk" );
static const itype_id itype_test_storage_battery( "test_storage_battery" );
static const itype_id itype_welder( "welder" );
static const itype_id itype_welding_wire_steel( "welding_wire_steel" );

static const skill_id skill_mechanics( "mechanics" );

static const vpart_id vpart_ap_test_storage_battery( "ap_test_storage_battery" );

static const vproto_id vehicle_prototype_car( "car" );

static void test_repair( const std::vector<item> &tools, bool plug_in_tools, bool expect_craftable )
{
    map &here = get_map();
    clear_avatar();
    clear_map();

    const tripoint_bub_ms test_origin( 60, 60, 0 );
    Character &player_character = get_player_character();
    player_character.setpos( here, test_origin );
    const item debug_backpack( itype_debug_backpack );
    player_character.wear_item( debug_backpack );

    const tripoint_bub_ms battery_pos = test_origin + tripoint::north_west;
    std::optional<item> battery_item( itype_test_storage_battery );
    place_appliance( here, battery_pos, vpart_ap_test_storage_battery, player_character, battery_item );

    for( const item &gear : tools ) {
        item_location added_tool = player_character.i_add( gear );
        if( plug_in_tools && added_tool->can_link_up() ) {
            added_tool->link_to( here.veh_at( player_character.pos_bub() + tripoint::north_west ),
                                 link_state::automatic );
            REQUIRE( added_tool->link().t_veh );
        }
    }
    player_character.set_skill_level( skill_mechanics, 10 );

    const tripoint_bub_ms vehicle_origin = test_origin + tripoint::south_east;
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_car, vehicle_origin, -90_degrees, 0,
                                         0 );

    REQUIRE( veh_ptr != nullptr );
    // Find the frame at the origin.
    vehicle_part *origin_frame = nullptr;
    for( vehicle_part *part : veh_ptr->get_parts_at( &here, vehicle_origin, "",
            part_status_flag::any ) ) {
        if( part->info().location == "structure" ) {
            origin_frame = part;
            break;
        }
    }
    REQUIRE( origin_frame != nullptr );
    REQUIRE( origin_frame->hp() == origin_frame->info().durability );
    veh_ptr->mod_hp( *origin_frame, -50 );
    REQUIRE( origin_frame->hp() < origin_frame->info().durability );
    // for a steel frame, one quadrant of damage takes 1000 kJ, 5 chunks of steel and 50 welding wires/rods to fix. (it has 400 max hp)

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

        item welder( itype_welder );
        tools.push_back( welder );

        tools.emplace_back( itype_goggles_welding );
        tools.emplace_back( itype_hammer );
        tools.insert( tools.end(), 20, item( itype_lc_steel_chunk ) );
        tools.insert( tools.end(), 200, item( itype_welding_wire_steel ) );
        test_repair( tools, true, true );
    }
    SECTION( "UPS_modded_welder" ) {
        std::vector<item> tools;
        item welder( itype_welder, calendar::turn_zero, 0 );
        welder.put_in( item( itype_battery_ups ), pocket_type::MOD );
        tools.push_back( welder );

        item ups( itype_UPS_ON );
        item ups_mag( ups.magazine_default() );
        ups_mag.ammo_set( ups_mag.ammo_default(), 1000 );
        ups.put_in( ups_mag, pocket_type::MAGAZINE_WELL );
        tools.push_back( ups );

        tools.emplace_back( itype_goggles_welding );
        tools.emplace_back( itype_hammer );
        tools.insert( tools.end(), 5, item( itype_lc_steel_chunk ) );
        tools.insert( tools.end(), 50, item( itype_welding_wire_steel ) );
        test_repair( tools, false, false );
    }
    SECTION( "welder_missing_goggles" ) {
        std::vector<item> tools;

        item welder( itype_welder );
        tools.push_back( welder );

        tools.emplace_back( itype_hammer );
        tools.insert( tools.end(), 5, item( itype_lc_steel_chunk ) );
        tools.insert( tools.end(), 50, item( itype_welding_wire_steel ) );
        test_repair( tools, true, false );
    }
    SECTION( "welder_missing_charge" ) {
        std::vector<item> tools;

        item welder( itype_welder );
        tools.push_back( welder );

        tools.emplace_back( itype_goggles_welding );
        tools.emplace_back( itype_hammer );
        tools.insert( tools.end(), 5, item( itype_lc_steel_chunk ) );
        tools.insert( tools.end(), 50, item( itype_welding_wire_steel ) );
        test_repair( tools, false, false );
    }
    SECTION( "UPS_modded_welder_missing_charges" ) {
        std::vector<item> tools;
        item welder( itype_welder, calendar::turn_zero, 0 );
        welder.put_in( item( itype_battery_ups ), pocket_type::MOD );
        tools.push_back( welder );

        item ups( itype_UPS_ON );
        item ups_mag( ups.magazine_default() );
        ups_mag.ammo_set( ups_mag.ammo_default(), 500 );
        ups.put_in( ups_mag, pocket_type::MAGAZINE_WELL );
        tools.push_back( ups );

        tools.emplace_back( itype_goggles_welding );
        tools.insert( tools.end(), 5, item( itype_lc_steel_chunk ) );
        tools.insert( tools.end(), 50, item( itype_welding_wire_steel ) );
        test_repair( tools, false, false );
    }
    SECTION( "welder_missing_consumables" ) {
        std::vector<item> tools;

        item welder( itype_welder );
        tools.push_back( welder );

        tools.emplace_back( itype_goggles_welding );
        test_repair( tools, true, false );
    }
}
