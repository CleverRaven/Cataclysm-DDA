#include "catch/catch.hpp"

#include <vector>

#include "activity_handlers.h"
#include "game.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "npc.h"
#include "overmapbuffer.h"
#include "point.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

static const zone_type_id zone_type_VEHICLE_DECONSTRUCT( "VEHICLE_DECONSTRUCT" );
static const time_point midday = calendar::turn_zero + 12_hours;

static void test_dismantle_vehicle( std::string vehicle_type, int expected_parts )
{
    INFO( vehicle_type )
    clear_map_and_put_player_underground();
    set_time( midday ); // Ensure light for crafting
    tripoint target_location( 60, 60, 0 );
    // Radius is currently limited by the need to place a crane outside the zone.
    int zone_radius = 3;
    // Place a bicycle
    map &here = get_map();
    vehicle *v_ptr = here.add_vehicle( vproto_id( vehicle_type ), target_location, 0_degrees, 0, 0 );
    REQUIRE( v_ptr );
    // Remove all items from cargo to normalize disassembly results.
    for( const vpart_reference &vp : v_ptr->get_all_parts() ) {
        v_ptr->get_items( vp.part_index() ).clear();
        vp.part().ammo_consume( vp.part().ammo_remaining(), vp.pos() );
    }
    // Give the NPC what they need to dismantle the vehicle.
    const item backpack( "backpack" );
    std::vector<item> tools;
    tools.emplace_back( "hacksaw" );
    tools.emplace_back( "wrench" );
    tools.emplace_back( "screwdriver" );
    // Just somewhere well outsize the zone to avoid landing on a vehicle.
    const tripoint npc_pos = target_location + tripoint_west * ( zone_radius + 3 );
    shared_ptr_fast<npc> guy = make_shared_fast<npc>();
    guy->normalize();
    guy->set_skill_level( skill_id( "mechanics" ), 2 );
    guy->wear_item( backpack );
    for( const item &gear : tools ) {
        guy->i_add( gear );
    }
    // Place several cranes for full coverage.
    // Especially, place cranes on the subcardinals because the ones on cardinals
    // are not detected from the adjacent square?
    for( const tripoint &direction : eight_horizontal_neighbors ) {
        tripoint crane_location = target_location + direction * ( zone_radius + 1 );
        CAPTURE( crane_location );
        vehicle *crane = here.add_vehicle( vproto_id( "engine_crane" ), crane_location, 0_degrees );
        REQUIRE( crane );
    }
    // Then make sure the NPC will always be able to use at least one.
    for( tripoint &loc : closest_points_first( target_location, zone_radius ) ) {
        CAPTURE( loc );
        REQUIRE( guy->best_nearby_lifting_assist( loc ) >= 3500000_gram );
    }
    guy->spawn_at_precise( here.get_abs_sub().xy(), npc_pos );
    guy->place_on_map();
    // Set a zone around the vehicle.
    zone_manager &zman = zone_manager::get_manager();
    zman.add( "disassemble Stephanie!", zone_type_VEHICLE_DECONSTRUCT,
              faction_id( "your_followers" ), false, true,
              target_location + tripoint_north_west * zone_radius,
              target_location + tripoint_south_east * zone_radius );
    guy->assign_activity( activity_id( "ACT_VEHICLE_DECONSTRUCTION" ) );
    REQUIRE( generic_multi_activity_handler( guy->activity, *guy, true ) );
    do {
        do {
            guy->mod_moves( guy->get_speed() );
            guy->do_player_activity();
        } while( guy->activity );
        if( guy->has_destination() ) {
            const tripoint destination = here.getlocal( *guy->destination_point );
            if( rl_dist( target_location, destination ) <= zone_radius ) {
                guy->setpos( destination );
                guy->start_destination_activity();
            }
        } else if( !guy->backlog.empty() ) {
            guy->activity = guy->backlog.front();
            guy->backlog.pop_front();
        }
    } while( generic_multi_activity_handler( guy->activity, *guy, true ) );

    // Assert pile of parts in the deconstruction zone.
    int nearby_items = 0;
    for( tripoint &loc : closest_points_first( target_location, zone_radius + 1 ) ) {
        nearby_items += here.i_at( loc ).size();
    }
    CHECK( nearby_items == expected_parts );
}

TEST_CASE( "NPCs dismantling a vehicle" )
{
    test_dismantle_vehicle( "bicycle", 4 );
    test_dismantle_vehicle( "quad_bike", 11 );
    test_dismantle_vehicle( "beetle", 34 );
    test_dismantle_vehicle( "car_mini", 37 );
    test_dismantle_vehicle( "car_hatch", 47 );
}
