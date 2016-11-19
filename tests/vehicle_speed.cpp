#include "catch/catch.hpp"

#include "game.h"
#include "map.h"
#include "vehicle.h"
#include "veh_type.h"
#include "itype.h"
#include "player.h"
#include "cata_utility.h"

/** check max velocity for vehicle between @ref min and @ref max tiles per turn */
static void test_max_velocity( const vproto_id &id, double min, double max ) {
    INFO( "max velocity (tiles/turn)" );
    REQUIRE( id.is_valid() );
    vehicle veh( id, 100, 0 );
    auto base = veh.part_base( veh.index_of_part( &veh.current_engine() ) );
    REQUIRE( base );
    int mass = veh.total_mass() + ( player().get_weight() / 1000.0 );
    const double v = ms_to_tt( base->type->engine->velocity_max( mass, veh.k_dynamics() ) );
    REQUIRE( v >= min );
    REQUIRE( v <= max );
}

/** check safe velocity for vehicle between @ref min and @ref max tiles per turn */
static void test_safe_velocity( const vproto_id &id, double min, double max ) {
    INFO( "safe velocity (tiles/turn)" );
    REQUIRE( id.is_valid() );
    vehicle veh( id, 100, 0 );
    auto base = veh.part_base( veh.index_of_part( &veh.current_engine() ) );
    REQUIRE( base );
    int mass = veh.total_mass() + ( player().get_weight() / 1000.0 );
    const double v = ms_to_tt( base->type->engine->velocity_safe( mass, veh.k_dynamics() ) );
    REQUIRE( v >= min );
    REQUIRE( v <= max );
}

/** check optimal velocity for vehicle between @ref min and @ref max tiles per turn */
static void test_optimal_velocity( const vproto_id &id, double min, double max ) {
    INFO( "optimal velocity (tiles/turn)" );
    REQUIRE( id.is_valid() );
    vehicle veh( id, 100, 0 );
    auto base = veh.part_base( veh.index_of_part( &veh.current_engine() ) );
    REQUIRE( base );
    int mass = veh.total_mass() + ( player().get_weight() / 1000.0 );
    const double v = ms_to_tt( base->type->engine->velocity_optimal( mass, veh.k_dynamics() ) );
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
        test_max_velocity    ( vproto_id( "motorcycle" ), 7.5, 8.5 );
        test_safe_velocity   ( vproto_id( "motorcycle" ), 6.0, 7.0 );
        test_optimal_velocity( vproto_id( "motorcycle" ), 3.0, 4.0 );
    }

    SECTION( "humvee" ) {
        // very heavy vehicle is slower than car but with a relatively efficient diesel engine
        test_max_velocity    ( vproto_id( "humvee" ), 2.5, 3.5 );
        test_safe_velocity   ( vproto_id( "humvee" ), 2.5, 3.5 );
        test_optimal_velocity( vproto_id( "humvee" ), 2.0, 3.0 );
    }

    SECTION( "road_roller" ) {
        // huge vehicle with poor dynamics but a powerful traction engine that is slow but efficient
        test_max_velocity    ( vproto_id( "road_roller" ), 1.0, 2.0 );
        test_safe_velocity   ( vproto_id( "road_roller" ), 1.0, 2.0 );
        test_optimal_velocity( vproto_id( "road_roller" ), 1.0, 2.0 );
    }

    SECTION( "car_sports" ) {
        // twice as fast as a regular car but not especially efficient at the higher speeds
        test_max_velocity    ( vproto_id( "car_sports" ), 10.0, 12.0 );
        test_safe_velocity   ( vproto_id( "car_sports" ), 10.0, 12.0 );
        test_optimal_velocity( vproto_id( "car_sports" ), 6.0, 7.0 );
    }

    SECTION( "car_sports_electric" ) {
        // electric cars have similar speeds to gasoline but are equally efficient at all speeds
        test_max_velocity    ( vproto_id( "car_sports_electric" ), 10.0, 12.0 );
        test_safe_velocity   ( vproto_id( "car_sports_electric" ), 10.0, 12.0 );
        test_optimal_velocity( vproto_id( "car_sports_electric" ), 10.0, 12.0 );
    }

    SECTION( "scooter" ) {
        // light with fixed gearing high speeds possible only at inefficient and dangerous high rpm
        test_max_velocity    ( vproto_id( "scooter" ), 4.0, 5.0 );
        test_safe_velocity   ( vproto_id( "scooter" ), 1.5, 2.5 );
        test_optimal_velocity( vproto_id( "scooter" ), 0.5, 1.5 );
    }

    SECTION( "scooter_electric" ) {
        // More efficient than the gasoline alternative but similarly struggles when carrying cargo
        test_max_velocity    ( vproto_id( "scooter_electric" ), 4.0, 5.0 );
        test_safe_velocity   ( vproto_id( "scooter_electric" ), 4.0, 5.0 );
        test_optimal_velocity( vproto_id( "scooter_electric" ), 4.0, 5.0 );
    }
}
