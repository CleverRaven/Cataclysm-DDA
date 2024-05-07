#include <set>
#include <vector>

#include "cata_catch.h"
#include "character.h"
#include "map.h"
#include "map_helpers.h"
#include "point.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"

static const vproto_id vehicle_prototype_car( "car" );
static const vproto_id vehicle_prototype_circle_split_test( "circle_split_test" );
static const vproto_id vehicle_prototype_cross_split_test( "cross_split_test" );

TEST_CASE( "vehicle_split_section", "[vehicle]" )
{
    map &here = get_map();
    Character &player_character = get_player_character();
    for( units::angle dir = 0_degrees; dir < 360_degrees; dir += vehicles::steer_increment ) {
        CHECK( !player_character.in_vehicle );
        const tripoint test_origin( 15, 15, 0 );
        player_character.setpos( test_origin );
        tripoint vehicle_origin = tripoint( 10, 10, 0 );
        VehicleList vehs = here.get_vehicles();
        clear_vehicles();
        REQUIRE( here.get_vehicles().empty() );
        vehicle *veh_ptr = here.add_vehicle( vehicle_prototype_cross_split_test,
                                             vehicle_origin, dir, 0, 0 );
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
        veh_ptr = here.add_vehicle( vehicle_prototype_circle_split_test, vehicle_origin, dir, 0, 0 );
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

TEST_CASE( "conjoined_vehicles", "[vehicle]" )
{
    map &here = get_map();
    here.add_vehicle( vehicle_prototype_car, { 40, 40, here.get_abs_sub().z() }, 0_degrees );
    here.add_vehicle( vehicle_prototype_car, { 42, 42, here.get_abs_sub().z() }, 0_degrees );
    here.add_vehicle( vehicle_prototype_car, { 44, 44, here.get_abs_sub().z() }, 45_degrees );
    here.add_vehicle( vehicle_prototype_car, { 48, 44, here.get_abs_sub().z() }, 45_degrees );
}

TEST_CASE( "crater_crash", "[vehicle]" )
{
    tinymap here;
    tripoint_abs_omt map_location( 30, 30, 0 );
    here.load( map_location, true );
    here.add_vehicle( vehicle_prototype_car, { 14, 11, here.get_abs_sub().z() }, 45_degrees );
    const tripoint end{ 20, 20, 0 };
    for( tripoint cursor = { 14, 4, 0 }; cursor.y < end.y; cursor.y++ ) {
        for( cursor.x = 4; cursor.x < end.x; cursor.x++ ) {
            here.destroy( cursor, true );
        }
    }
}

TEST_CASE( "split_vehicle_during_mapgen", "[vehicle]" )
{
    clear_map();
    map &here = get_map();
    clear_vehicles();
    REQUIRE( here.get_vehicles().empty() );

    tinymap tm;
    // Wherever the main map is, create this tinymap outside its bounds.
    tm.load( project_to<coords::omt>( here.get_abs_sub() ) + tripoint_rel_omt( 10, 10, 0 ), false );
    wipe_map_terrain( tm.cast_to_map() );
    REQUIRE( tm.get_vehicles().empty() );
    tripoint vehicle_origin{ 14, 14, 0 };
    vehicle *veh_ptr = tm.add_vehicle( vehicle_prototype_cross_split_test,
                                       vehicle_origin, 0_degrees, 0, 0 );
    REQUIRE( veh_ptr != nullptr );
    REQUIRE( !tm.get_vehicles().empty() );
    tm.destroy( vehicle_origin );
    CHECK( tm.get_vehicles().size() == 4 );
    for( wrapped_vehicle &veh : here.get_vehicles() ) {
        WARN( veh.v->sm_pos );
    }
    CHECK( here.get_vehicles().empty() );
    clear_vehicles();
}
