#include "catch/catch.hpp"

#include "game.h"
#include "map.h"
#include "vehicle.h"
#include "veh_type.h"
#include "player.h"

TEST_CASE( "destroy_grabbed_vehicle_section" ) {
    GIVEN( "A vehicle grabbed by the player" ) {
        tripoint test_origin( 60, 60, 0 );
        g->u.setpos( test_origin );
        tripoint vehicle_origin = test_origin + tripoint( 1, 1, 0 );
        vehicle *veh_ptr = g->m.add_vehicle( vproto_id( "bicycle" ), vehicle_origin, -90 );
        REQUIRE( veh_ptr != nullptr );
        tripoint grab_point = test_origin + tripoint( 1, 0, 0 );
        g->u.grab_type = OBJECT_VEHICLE;
        g->u.grab_point = grab_point;
        WHEN( "The vehicle section grabbed by the player is destroyed" ) {
            g->m.destroy( grab_point );
            THEN( "The player's grab is released" ) {
                CHECK( g->u.grab_type == OBJECT_NONE );
                CHECK( g->u.grab_point == tripoint_zero );
            }
        }
    }
}
