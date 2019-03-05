#include <sstream>

#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "vehicle.h"
#include "veh_type.h"
#include "vpart_range.h"
#include "vpart_reference.h"
#include "itype.h"
#include "player.h"
#include "cata_utility.h"
#include "options.h"
#include "test_statistics.h"

typedef statistics<long> efficiency_stat;

const efftype_id effect_blind( "blind" );

void clear_game_drag( const ter_id &terrain )
{
    // Set to turn 0 to prevent solars from producing power
    calendar::turn = 0;
    for( monster &critter : g->all_monsters() ) {
        g->remove_zombie( critter );
    }

    g->unload_npcs();

    // Move player somewhere safe
    g->u.setpos( tripoint( 0, 0, 0 ) );
    // Blind the player to avoid needless drawing-related overhead
    g->u.add_effect( effect_blind, 1_turns, num_bp, true );
    // Make sure the ST is 8 so that muscle powered results are consistent
    g->u.str_cur = 8;

    for( const tripoint &p : g->m.points_in_rectangle( tripoint( 0, 0, 0 ),
            tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        g->m.furn_set( p, furn_id( "f_null" ) );
        g->m.ter_set( p, terrain );
        g->m.trap_set( p, trap_id( "tr_null" ) );
        g->m.i_clear( p );
    }

    for( wrapped_vehicle &veh : g->m.get_vehicles( tripoint( 0, 0, 0 ), tripoint( MAPSIZE * SEEX,
            MAPSIZE * SEEY, 0 ) ) ) {
        g->m.destroy_vehicle( veh.v );
    }

    g->m.build_map_cache( 0, true );
    // hard force a rebuild of caches
    g->m.shift( 0, 1 );
    g->m.shift( 0, -1 );
}


vehicle *setup_drag_test( const vproto_id &veh_id )
{
    clear_game_drag( ter_id( "t_pavement" ) );

    const tripoint map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = g->m.add_vehicle( veh_id, map_starting_point, -90, 0, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return nullptr;
    }

    // Remove all items from cargo to normalize weight.
    // turn everything on
    for( const vpart_reference vp : veh_ptr->get_all_parts() ) {
        while( veh_ptr->remove_item( vp.part_index(), 0 ) );
        veh_ptr->toggle_specific_part( vp.part_index(), true );
    }
    // close the doors
    const auto doors = veh_ptr->get_avail_parts( "OPENABLE" );
    for( const vpart_reference vp :  doors ) {
        const size_t door = vp.part_index();
        veh_ptr->close( door );
    }

    veh_ptr->refresh_insides();
    return veh_ptr;
}

// Algorithm goes as follows:
// Clear map
// Spawn a vehicle
// calculate c_air_drag and c_rolling_resistance
// return whether they're within 5% of expected values
bool test_drag( const vproto_id &veh_id,
                const double expected_c_air = 0, const double expected_c_rr = 0,
                const double expected_c_water = 0,
                const int expected_safe = 0, const int expected_max = 0,
                const bool test_results = false )
{
    vehicle *veh_ptr = setup_drag_test( veh_id );
    if( veh_ptr == nullptr ) {
        return false;
    }

    const double c_air = veh_ptr->coeff_air_drag();
    const double c_rolling = veh_ptr->coeff_rolling_drag();
    const double c_water = veh_ptr->coeff_water_drag();
    const int safe_v = veh_ptr->safe_ground_velocity( false );
    const int max_v = veh_ptr->max_ground_velocity( false );

    const auto d_in_bounds = [&]( const double expected, double value ) {
        double expected_high = expected * 1.05;
        double expected_low = expected * 0.95;
        CHECK( value >= expected_low );
        CHECK( value <= expected_high );
        return ( value >= expected_low ) && ( value <= expected_high );
    };
    const auto i_in_bounds = [&]( const int expected, int value ) {
        int expected_high = expected * 1.05;
        int expected_low = expected * 0.95;
        CHECK( value >= expected_low );
        CHECK( value <= expected_high );
        return ( value >= expected_low ) && ( value <= expected_high );
    };
    bool valid = test_results;
    if( test_results ) {
        valid &= d_in_bounds( expected_c_air, c_air );
        valid &= d_in_bounds( expected_c_rr, c_rolling );
        valid &= d_in_bounds( expected_c_water, c_water );
        valid &= i_in_bounds( expected_safe, safe_v );
        valid &= i_in_bounds( expected_max, max_v );
    }
    if( !valid ) {
        printf( "    test_vehicle_drag( \"%s\", %f, %f, %f, %d, %d );\n",
                veh_id.c_str(), c_air, c_rolling, c_water, safe_v, max_v );
        fflush( stdout );
    }
    return valid;
}

void print_drag_test_strings( const std::string &type )
{
    test_drag( vproto_id( type ) );
    fflush( stdout );
}

void test_vehicle_drag( std::string type,
                        const double expected_c_air, const double expected_c_rr,
                        const double expected_c_water,
                        const int expected_safe, const int expected_max )
{
    SECTION( type ) {
        test_drag( vproto_id( type ), expected_c_air, expected_c_rr, expected_c_water,
                   expected_safe, expected_max, true );
    }
}

std::vector<std::string> vehs_to_test_drag = {
    {
        "bicycle",
        "bicycle_electric",
        "motorcycle",
        "motorcycle_sidecart",
        "quad_bike",
        "scooter",
        "scooter_electric",
        "superbike",
        "tandem",
        "unicycle",
        "beetle",
        "bubble_car",
        "car",
        "car_mini",
        "car_sports",
        "car_sports_atomic",
        "car_sports_electric",
        "electric_car",
        "rara_x",
        "suv",
        "suv_electric",
        "golf_cart",
        "golf_cart_4seat",
        "hearse",
        "pickup_technical",
        "ambulance",
        "car_fbi",
        "fire_engine",
        "fire_truck",
        "policecar",
        "policesuv",
        "truck_swat",
        "oldtractor",
        "autotractor",
        "tractor_plow",
        "tractor_reaper",
        "tractor_seed",
        "aapc-mg",
        "apc",
        "humvee",
        "military_cargo_truck",
        "flatbed_truck",
        "pickup",
        "semi_truck",
        "truck_trailer",
        "tatra_truck",
        "animalctrl",
        "autosweeper",
        "excavator",
        "road_roller",
        "forklift",
        "trencher",
        "armored_car",
        "cube_van",
        "cube_van_cheap",
        "hippie_van",
        "icecream_truck",
        "lux_rv",
        "meth_lab",
        "rv",
        "schoolbus",
        "security_van",
        "wienermobile",
    }
};

/** This is even less of a test. It generates C++ lines for the actual test below */
TEST_CASE( "vehicle_drag_calc_baseline", "[.]" )
{
    for( const std::string &veh : vehs_to_test_drag ) {
        print_drag_test_strings( veh );
    }
}

// format is vehicle, coeff_air_drag, coeff_rolling_drag, safe speed, max speed
// coeffs are dimensionless, speeds are 100ths of mph, so 6101 is 61.01 mph
TEST_CASE( "vehicle_drag", "[vehicle] [engine]" )
{
    test_vehicle_drag( "bicycle", 0.609525, 0.016957, 42.679167, 2356, 3078 );
    test_vehicle_drag( "bicycle_electric", 0.609525, 0.031616, 79.577083, 2672, 3208 );
    test_vehicle_drag( "motorcycle", 0.609525, 0.568903, 254.351562, 7212, 8628 );
    test_vehicle_drag( "motorcycle_sidecart", 0.880425, 0.857295, 340.701562, 6349, 7604 );
    test_vehicle_drag( "quad_bike", 1.151325, 1.110281, 551.552083, 5786, 6937 );
    test_vehicle_drag( "scooter", 0.609525, 0.171854, 129.764583, 4012, 4902 );
    test_vehicle_drag( "scooter_electric", 0.609525, 0.178166, 134.531250, 4922, 5102 );
    test_vehicle_drag( "superbike", 0.609525, 0.844993, 377.789062, 9867, 11765 );
    test_vehicle_drag( "tandem", 0.609525, 0.021344, 40.290625, 2353, 3076 );
    test_vehicle_drag( "unicycle", 0.690795, 0.002400, 24.162500, 2266, 2958 );
    test_vehicle_drag( "beetle", 1.320638, 1.693784, 1199.020312, 7542, 9024 );
    test_vehicle_drag( "bubble_car", 0.846562, 1.661737, 941.067500, 9354, 9702 );
    test_vehicle_drag( "car", 0.507938, 2.366883, 1117.002083, 10054, 12108 );
    test_vehicle_drag( "car_mini", 0.507938, 1.709414, 1210.084375, 10209, 12254 );
    test_vehicle_drag( "car_sports", 0.507938, 2.424461, 1373.010000, 17561, 20950 );
    test_vehicle_drag( "car_sports_atomic", 0.507938, 3.442279, 1624.513542, 20837, 21611 );
    test_vehicle_drag( "car_sports_electric", 0.507938, 2.751150, 1558.018750, 20968, 21741 );
    test_vehicle_drag( "electric_car", 0.507938, 2.142779, 1011.240625, 13889, 14407 );
    test_vehicle_drag( "rara_x", 0.903000, 1.995736, 861.116964, 8432, 8748 );
    test_vehicle_drag( "suv", 0.507938, 2.807600, 1135.705357, 11834, 14217 );
    test_vehicle_drag( "suv_electric", 0.541800, 2.324519, 940.293750, 13572, 14080 );
    test_vehicle_drag( "golf_cart", 0.943313, 1.311868, 825.479167, 7152, 7418 );
    test_vehicle_drag( "golf_cart_4seat", 0.943313, 1.279229, 804.941667, 7157, 7423 );
    test_vehicle_drag( "hearse", 0.677250, 3.016494, 1220.205357, 9086, 10957 );
    test_vehicle_drag( "pickup_technical", 2.336513, 2.820320, 1140.850893, 7321, 8747 );
    test_vehicle_drag( "ambulance", 2.275560, 2.264056, 1393.663281, 8763, 10446 );
    test_vehicle_drag( "car_fbi", 1.036193, 2.635064, 1243.564583, 11259, 13453 );
    test_vehicle_drag( "fire_engine", 2.305875, 3.281582, 2154.679464, 8666, 10345 );
    test_vehicle_drag( "fire_truck", 2.275560, 7.944023, 3912.020625, 8433, 10139 );
    test_vehicle_drag( "policecar", 1.036193, 2.636167, 1244.085417, 11259, 13453 );
    test_vehicle_drag( "policesuv", 1.036193, 2.967405, 1200.348214, 11221, 13417 );
    test_vehicle_drag( "truck_swat", 1.787940, 7.538017, 4640.104687, 7831, 9188 );
    test_vehicle_drag( "oldtractor", 1.151325, 0.868099, 712.489583, 9631, 11170 );
    test_vehicle_drag( "autotractor", 1.567995, 1.738959, 1712.695000, 7099, 7361 );
    test_vehicle_drag( "tractor_plow", 1.693125, 0.893061, 879.572500, 8482, 9835 );
    test_vehicle_drag( "tractor_reaper", 1.693125, 0.778836, 767.072500, 8492, 9844 );
    test_vehicle_drag( "tractor_seed", 1.693125, 0.778836, 767.072500, 8492, 9844 );
    test_vehicle_drag( "aapc-mg", 4.740750, 8.587756, 4052.815972, 5776, 6754 );
    test_vehicle_drag( "apc", 4.740750, 8.464568, 3994.679861, 5780, 6757 );
    test_vehicle_drag( "humvee", 1.668938, 7.235435, 4878.022321, 9594, 11208 );
    test_vehicle_drag( "military_cargo_truck", 1.523813, 9.416864, 4345.338889, 9695, 11367 );
    test_vehicle_drag( "flatbed_truck", 1.378688, 4.458495, 1995.980114, 8484, 10205 );
    test_vehicle_drag( "pickup", 0.914288, 3.088771, 1249.441964, 9798, 11764 );
    test_vehicle_drag( "semi_truck", 1.712475, 9.950414, 4312.056875, 9329, 10937 );
    test_vehicle_drag( "truck_trailer", 1.196475, 12.833835, 5688.000000, 0, 0 );
    test_vehicle_drag( "tatra_truck", 0.846563, 7.711005, 3937.911111, 14668, 17134 );
    test_vehicle_drag( "animalctrl", 0.677250, 2.717176, 1099.127679, 10815, 12987 );
    test_vehicle_drag( "autosweeper", 1.567995, 1.584382, 1121.575000, 6047, 6272 );
    test_vehicle_drag( "excavator", 1.425450, 1.749378, 1238.375000, 10243, 11880 );
    test_vehicle_drag( "road_roller", 2.201063, 2.745174, 9120.522917, 8829, 10247 );
    test_vehicle_drag( "forklift", 0.943313, 1.350440, 637.312500, 7146, 7412 );
    test_vehicle_drag( "trencher", 1.425450, 1.080300, 1063.983750, 6290, 6522 );
    test_vehicle_drag( "armored_car", 1.668938, 6.952104, 4687.005357, 9615, 11228 );
    test_vehicle_drag( "cube_van", 1.173900, 2.636314, 1622.810156, 9138, 10934 );
    test_vehicle_drag( "cube_van_cheap", 1.111012, 2.572474, 1583.513281, 7863, 9442 );
    test_vehicle_drag( "hippie_van", 0.711113, 2.780080, 1124.573214, 8995, 10832 );
    test_vehicle_drag( "icecream_truck", 2.113020, 3.708542, 1500.146429, 7479, 8965 );
    test_vehicle_drag( "lux_rv", 3.286920, 3.503599, 1463.926136, 6694, 7786 );
    test_vehicle_drag( "meth_lab", 1.173900, 2.957331, 1510.270833, 9076, 10882 );
    test_vehicle_drag( "rv", 1.173900, 2.932748, 1497.716667, 9078, 10885 );
    test_vehicle_drag( "schoolbus", 0.846563, 3.060324, 1370.046591, 10335, 12057 );
    test_vehicle_drag( "security_van", 1.173900, 7.592192, 4673.452344, 8855, 10421 );
    test_vehicle_drag( "wienermobile", 3.286920, 2.315334, 1425.228125, 7770, 9258 );
};
