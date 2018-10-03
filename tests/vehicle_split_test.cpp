#include "catch/catch.hpp"

#include "game.h"
#include "map.h"
#include "vehicle.h"
#include "veh_type.h"
#include "player.h"

TEST_CASE( "vehicle_split_section" )
{
    GIVEN( "Some vehicles to split" ) {
        tripoint test_origin( 15, 15, 0 );
        g->u.setpos( test_origin );
        tripoint vehicle_origin = tripoint( 10, 10, 0 );
        VehicleList vehs;
        vehs = g->m.get_vehicles();
        vehicle *veh_ptr;
        for( auto &vehs_v : vehs ) {
            veh_ptr = vehs_v.v;
            g->m.destroy_vehicle( veh_ptr );
        }
        g->refresh_all();
        REQUIRE( g->m.get_vehicles().empty() );
        veh_ptr = g->m.add_vehicle( vproto_id( "cross_split_test" ), vehicle_origin, 0 );
        REQUIRE( veh_ptr != nullptr );
        g->m.destroy( vehicle_origin );
        vehs = g->m.get_vehicles();
        // destroying the center frame results in 4 new vehicles
        CHECK( vehs.size() == 4 );
        if( vehs.size() == 4 ) {
            CHECK( vehs[ 0 ].v->parts.size() == 8 );
            CHECK( vehs[ 1 ].v->parts.size() == 8 );
            CHECK( vehs[ 2 ].v->parts.size() == 2 );
            CHECK( vehs[ 3 ].v->parts.size() == 2 );
            g->m.destroy_vehicle( vehs[ 3 ].v );
            g->m.destroy_vehicle( vehs[ 2 ].v );
            g->m.destroy_vehicle( vehs[ 1 ].v );
            g->m.destroy_vehicle( vehs[ 0 ].v );
        }
        g->refresh_all();
        REQUIRE( g->m.get_vehicles().empty() );
        vehicle_origin = tripoint( 20, 20, 0 );
        veh_ptr = g->m.add_vehicle( vproto_id( "circle_split_test" ), vehicle_origin, 0 );
        REQUIRE( veh_ptr != nullptr );
        g->m.destroy( vehicle_origin );
        vehs = g->m.get_vehicles();
        CHECK( vehs.size() == 1 );
        if( vehs.size() == 1 ) {
            CHECK( vehs[ 0 ].v->parts.size() == 39 );
        }
    }
}
