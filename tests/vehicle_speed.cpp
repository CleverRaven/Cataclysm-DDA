#include "catch/catch.hpp"

#include "game.h"
#include "map.h"
#include "vehicle.h"
#include "veh_type.h"
#include "cata_utility.h"

/** check max velocity for vehicle between @ref min and @ref max tiles per turn */
static void test_max_velocity( const vproto_id &id, double min, double max ) {
    REQUIRE( id.is_valid() );
    const vehicle veh( id, 100, 0 );
    const double v = ms_to_mph( veh.max_velocity( veh.current_engine() ) ) / 10.0;
    REQUIRE( v >= min );
    REQUIRE( v <= max );
}

/** check safe velocity for vehicle between @ref min and @ref max tiles per turn */
static void test_safe_velocity( const vproto_id &id, double min, double max ) {
    REQUIRE( id.is_valid() );
    const vehicle veh( id, 100, 0 );
    const double v = ms_to_mph( veh.safe_velocity( veh.current_engine() ) ) / 10.0;
    REQUIRE( v >= min );
    REQUIRE( v <= max );
}

/** check optimal velocity for vehicle between @ref min and @ref max tiles per turn */
static void test_optimal_velocity( const vproto_id &id, double min, double max ) {
    REQUIRE( id.is_valid() );
    const vehicle veh( id, 100, 0 );
    const double v = ms_to_mph( veh.optimal_velocity( veh.current_engine() ) ) / 10.0;
    REQUIRE( v >= min );
    REQUIRE( v <= max );
}

TEST_CASE( "vehicle_speed", "[vehicle] [engine]" ) {
    SECTION( "car" ) {
        // cars travel at top speed without engine damage but with slightly increased fuel usage
        test_max_velocity    ( vproto_id( "car" ), 5.0, 6.0 );
        test_safe_velocity   ( vproto_id( "car" ), 5.0, 6.0 );
        test_optimal_velocity( vproto_id( "car" ), 4.0, 5.0 );
    }

    SECTION( "electric_car" ) {
        // electric cars have similar speeds to gasoline but are equally efficient at all speeds
        test_max_velocity    ( vproto_id( "electric_car" ), 5.0, 6.0 );
        test_safe_velocity   ( vproto_id( "electric_car" ), 5.0, 6.0 );
        test_optimal_velocity( vproto_id( "electric_car" ), 5.0, 6.0 );
    }

    SECTION( "motorcycle" ) {
        // less efficient than car but can be driven faster (with increasing risk of engine damage)
        test_max_velocity    ( vproto_id( "motorcycle" ), 9.0, 10.0 );
        test_safe_velocity   ( vproto_id( "motorcycle" ), 6.0,  7.0 );
        test_optimal_velocity( vproto_id( "motorcycle" ), 3.0,  4.0 );
    }

    SECTION( "humvee" ) {
        // very heavy vehicle is slower than car but with a relatively efficient diesel engine
        test_max_velocity    ( vproto_id( "humvee" ), 2.5, 3.5 );
        test_safe_velocity   ( vproto_id( "humvee" ), 2.5, 3.5 );
        test_optimal_velocity( vproto_id( "humvee" ), 2.0, 3.0 );
    }
}
