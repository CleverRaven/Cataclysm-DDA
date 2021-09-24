#include <set>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "map.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"

TEST_CASE( "vehicle_split_section" )
{
    map &here = get_map();
    Character &player_character = get_player_character();
    for( units::angle dir = 0_degrees; dir < 360_degrees; dir += 15_degrees ) {
        CHECK( !player_character.in_vehicle );
        const tripoint test_origin( 15, 15, 0 );
        player_character.setpos( test_origin );
        tripoint vehicle_origin = tripoint( 10, 10, 0 );
        VehicleList vehs = here.get_vehicles();
        vehicle *veh_ptr;
        for( auto &vehs_v : vehs ) {
            veh_ptr = vehs_v.v;
            here.destroy_vehicle( veh_ptr );
        }
        REQUIRE( here.get_vehicles().empty() );
        veh_ptr = here.add_vehicle( vproto_id( "cross_split_test" ), vehicle_origin, dir, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        std::set<tripoint> original_points = veh_ptr->get_points( true );

        here.destroy( vehicle_origin );
        veh_ptr->part_removal_cleanup();
        REQUIRE( veh_ptr->get_parts_at( vehicle_origin, "", part_status_flag::available ).empty() );
        vehs = here.get_vehicles();
        // destroying the center frame results in 4 new vehicles
        CHECK( vehs.size() == 4 );
        if( vehs.size() == 4 ) {
            // correct number of parts
            CHECK( vehs[ 0 ].v->part_count() == 12 );
            CHECK( vehs[ 1 ].v->part_count() == 12 );
            CHECK( vehs[ 2 ].v->part_count() == 2 );
            CHECK( vehs[ 3 ].v->part_count() == 3 );
            std::vector<std::set<tripoint>> all_points;
            for( int i = 0; i < 4; i++ ) {
                const std::set<tripoint> &veh_points = vehs[ i ].v->get_points( true );
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
            here.destroy_vehicle( vehs[ 3 ].v );
            here.destroy_vehicle( vehs[ 2 ].v );
            here.destroy_vehicle( vehs[ 1 ].v );
            here.destroy_vehicle( vehs[ 0 ].v );
        }
        REQUIRE( here.get_vehicles().empty() );
        vehicle_origin = tripoint( 20, 20, 0 );
        veh_ptr = here.add_vehicle( vproto_id( "circle_split_test" ), vehicle_origin, dir, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        here.destroy( vehicle_origin );
        veh_ptr->part_removal_cleanup();
        REQUIRE( veh_ptr->get_parts_at( vehicle_origin, "", part_status_flag::available ).empty() );
        vehs = here.get_vehicles();
        CHECK( vehs.size() == 1 );
        if( vehs.size() == 1 ) {
            CHECK( vehs[ 0 ].v->part_count() == 38 );
        }
    }
}
