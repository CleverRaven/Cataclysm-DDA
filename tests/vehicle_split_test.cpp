#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "player.h"
#include "veh_type.h"
#include "vehicle.h"

TEST_CASE( "vehicle_split_section" )
{
    for( int dir = 0; dir < 360; dir += 15 ) {
        const tripoint test_origin( 15, 15, 0 );
        g->u.setpos( test_origin );
        tripoint vehicle_origin = tripoint( 10, 10, 0 );
        VehicleList vehs = g->m.get_vehicles();
        vehicle *veh_ptr;
        for( auto &vehs_v : vehs ) {
            veh_ptr = vehs_v.v;
            g->m.destroy_vehicle( veh_ptr );
        }
        g->refresh_all();
        REQUIRE( g->m.get_vehicles().empty() );
        veh_ptr = g->m.add_vehicle( vproto_id( "cross_split_test" ), vehicle_origin, dir, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        std::set<tripoint> original_points = veh_ptr->get_points( true );

        g->m.destroy( vehicle_origin );
        veh_ptr->part_removal_cleanup();
        REQUIRE( veh_ptr->get_parts_at( vehicle_origin, "", part_status_flag::available ).empty() );
        vehs = g->m.get_vehicles();
        // destroying the center frame results in 4 new vehicles
        CHECK( vehs.size() == 4 );
        if( vehs.size() == 4 ) {
            // correct number of parts
            CHECK( vehs[ 0 ].v->parts.size() == 10 );
            CHECK( vehs[ 1 ].v->parts.size() == 10 );
            CHECK( vehs[ 2 ].v->parts.size() == 2 );
            CHECK( vehs[ 3 ].v->parts.size() == 3 );
            std::vector<std::set<tripoint>> all_points;
            for( int i = 0; i < 4; i++ ) {
                std::set<tripoint> &veh_points = vehs[ i ].v->get_points( true );
                all_points.push_back( veh_points );
            }
            for( int i = 0; i < 4; i++ ) {
                std::set<tripoint> &veh_points = all_points[ i ];
                // every point in the new vehicle was in the old vehicle
                for( const tripoint &vpos : veh_points ) {
                    CHECK( original_points.find( vpos ) != original_points.end() );
                }
                // no point in any new vehicle is in any other new vehicle
                for( int j = i + 1; j < 4; j++ ) {
                    std::set<tripoint> &other_points = all_points[ j ];
                    for( const tripoint &vpos : veh_points ) {
                        CHECK( other_points.find( vpos ) == other_points.end() );
                    }
                }
            }
            g->m.destroy_vehicle( vehs[ 3 ].v );
            g->m.destroy_vehicle( vehs[ 2 ].v );
            g->m.destroy_vehicle( vehs[ 1 ].v );
            g->m.destroy_vehicle( vehs[ 0 ].v );
        }
        g->refresh_all();
        REQUIRE( g->m.get_vehicles().empty() );
        vehicle_origin = tripoint( 20, 20, 0 );
        veh_ptr = g->m.add_vehicle( vproto_id( "circle_split_test" ), vehicle_origin, dir, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        g->m.destroy( vehicle_origin );
        veh_ptr->part_removal_cleanup();
        REQUIRE( veh_ptr->get_parts_at( vehicle_origin, "", part_status_flag::available ).empty() );
        vehs = g->m.get_vehicles();
        CHECK( vehs.size() == 1 );
        if( vehs.size() == 1 ) {
            CHECK( vehs[ 0 ].v->parts.size() == 38 );
        }
    }
}
