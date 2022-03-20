#include <vector>

#include "avatar.h"
#include "cata_catch.h"
#include "character.h"
#include "damage.h"
#include "enums.h"
#include "item.h"
#include "map.h"
#include "map_helpers.h"
#include "optional.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"

static const vproto_id vehicle_prototype_bicycle( "bicycle" );

TEST_CASE( "detaching_vehicle_unboards_passengers" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin;
    map &here = get_map();
    Character &player_character = get_player_character();
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_bicycle, vehicle_origin, -90_degrees, 0,
                                         0 );
    here.board_vehicle( test_origin, &player_character );
    REQUIRE( player_character.in_vehicle );
    here.detach_vehicle( veh_ptr );
    REQUIRE( !player_character.in_vehicle );
}

TEST_CASE( "destroy_grabbed_vehicle_section" )
{
    GIVEN( "A vehicle grabbed by the player" ) {
        map &here = get_map();
        const tripoint test_origin( 60, 60, 0 );
        avatar &player_character = get_avatar();
        player_character.setpos( test_origin );
        const tripoint vehicle_origin = test_origin + tripoint_south_east;
        vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_bicycle, vehicle_origin, -90_degrees,
                                             0, 0 );
        REQUIRE( veh_ptr != nullptr );
        tripoint grab_point = test_origin + tripoint_east;
        player_character.grab( object_type::VEHICLE, tripoint_east );
        REQUIRE( player_character.get_grab_type() == object_type::VEHICLE );
        REQUIRE( player_character.grab_point == tripoint_east );
        WHEN( "The vehicle section grabbed by the player is destroyed" ) {
            here.destroy( grab_point );
            REQUIRE( veh_ptr->get_parts_at( grab_point, "", part_status_flag::available ).empty() );
            THEN( "The player's grab is released" ) {
                CHECK( player_character.get_grab_type() == object_type::NONE );
                CHECK( player_character.grab_point == tripoint_zero );
            }
        }
    }
}

TEST_CASE( "add_item_to_broken_vehicle_part" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin;
    vehicle *veh_ptr = get_map().add_vehicle( vehicle_prototype_bicycle, vehicle_origin, 0_degrees,
                       0, 0 );
    REQUIRE( veh_ptr != nullptr );

    const tripoint pos = vehicle_origin + tripoint_west;
    auto cargo_parts = veh_ptr->get_parts_at( pos, "CARGO", part_status_flag::any );
    REQUIRE( !cargo_parts.empty( ) );
    vehicle_part *cargo_part = cargo_parts.front();
    REQUIRE( cargo_part != nullptr );
    //Must not be broken yet
    REQUIRE( !cargo_part->is_broken() );
    //For some reason (0 - cargo_part->hp()) is just not enough to destroy a part
    REQUIRE( veh_ptr->mod_hp( *cargo_part, -( 1 + cargo_part->hp() ), damage_type::BASH ) );
    //Now it must be broken
    REQUIRE( cargo_part->is_broken() );
    //Now part is really broken, adding an item should fail
    const item itm2 = item( "jeans" );
    REQUIRE( !veh_ptr->add_item( *cargo_part, itm2 ) );
}

TEST_CASE( "starting_bicycle_damaged_pedal" )
{
    clear_map();
    const tripoint test_origin( 60, 60, 0 );
    const tripoint vehicle_origin = test_origin;
    map &here = get_map();
    Character &player_character = get_player_character();
    vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_bicycle, vehicle_origin, -90_degrees, 0,
                                         0 );
    here.board_vehicle( test_origin, &player_character );
    REQUIRE( player_character.in_vehicle );
    REQUIRE( veh_ptr->engines.size() == 1 );

    vehicle_part &pedel = veh_ptr->part( veh_ptr->engines[ 0 ] );

    SECTION( "when the pedal has 1/4 hp" ) {
        veh_ptr->set_hp( pedel, pedel.hp() * 0.25, true );
        // Try starting the engine 100 time because it is random that a combustion engine does fails
        for( int i = 0; i < 100 ; i++ ) {
            CHECK( veh_ptr->start_engine( 0 ) );
        }
    }

    SECTION( "when the pedal has 0 hp" ) {
        veh_ptr->set_hp( pedel, 0, true );

        CHECK_FALSE( veh_ptr->start_engine( 0 ) );
    }

    here.detach_vehicle( veh_ptr );
}
