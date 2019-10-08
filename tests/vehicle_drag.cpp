#include <stdio.h>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "vehicle.h"
#include "vpart_range.h"
#include "test_statistics.h"
#include "bodypart.h"
#include "calendar.h"
#include "game_constants.h"
#include "type_id.h"
#include "point.h"
#include "vpart_position.h"

using efficiency_stat = statistics<long>;

const efftype_id effect_blind( "blind" );

static void clear_game_drag( const ter_id &terrain )
{
    // Set to turn 0 to prevent solars from producing power
    calendar::turn = 0;
    clear_creatures();
    clear_npcs();

    // Move player somewhere safe
    CHECK( !g->u.in_vehicle );
    g->u.setpos( tripoint_zero );
    // Blind the player to avoid needless drawing-related overhead
    g->u.add_effect( effect_blind, 1_turns, num_bp, true );
    // Make sure the ST is 8 so that muscle powered results are consistent
    g->u.str_cur = 8;

    for( const tripoint &p : g->m.points_in_rectangle( tripoint_zero,
            tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        g->m.furn_set( p, furn_id( "f_null" ) );
        g->m.ter_set( p, terrain );
        g->m.trap_set( p, trap_id( "tr_null" ) );
        g->m.i_clear( p );
    }

    for( wrapped_vehicle &veh : g->m.get_vehicles( tripoint_zero, tripoint( MAPSIZE * SEEX,
            MAPSIZE * SEEY, 0 ) ) ) {
        g->m.destroy_vehicle( veh.v );
    }

    g->m.invalidate_map_cache( 0 );
    g->m.build_map_cache( 0, true );
    // hard force a rebuild of caches
    g->m.shift( point_south );
    g->m.shift( point_north );
}


static vehicle *setup_drag_test( const vproto_id &veh_id )
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
        veh_ptr->get_items( vp.part_index() ).clear();
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
static bool test_drag(
    const vproto_id &veh_id,
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

static void print_drag_test_strings( const std::string &type )
{
    test_drag( vproto_id( type ) );
    fflush( stdout );
}

static void test_vehicle_drag(
    std::string type, const double expected_c_air, const double expected_c_rr,
    const double expected_c_water, const int expected_safe, const int expected_max )
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
        "canoe",
        "kayak",
        "kayak_racing",
        "DUKW",
        "raft",
        "inflatable_boat",
    }
};

/** This is even less of a test. It generates C++ lines for the actual test below */
TEST_CASE( "vehicle_drag_calc_baseline", "[.]" )
{
    for( const std::string &veh : vehs_to_test_drag ) {
        print_drag_test_strings( veh );
    }
}

// format is vehicle, coeff_air_drag, coeff_rolling_drag, coeff_water_drag, safe speed, max speed
// coeffs are dimensionless, speeds are 100ths of mph, so 6101 is 61.01 mph
TEST_CASE( "vehicle_drag", "[vehicle] [engine]" )
{
    test_vehicle_drag( "bicycle", 0.609525, 0.016957, 42.679167, 2356, 3078 );
    test_vehicle_drag( "bicycle_electric", 0.609525, 0.031616, 79.577083, 2672, 3208 );
    test_vehicle_drag( "motorcycle", 0.609525, 0.568903, 254.351562, 7212, 8628 );
    test_vehicle_drag( "motorcycle_sidecart", 0.880425, 0.857295, 454.268750, 6349, 7604 );
    test_vehicle_drag( "quad_bike", 0.537285, 1.110281, 709.138393, 7369, 8856 );
    test_vehicle_drag( "scooter", 0.609525, 0.171854, 129.764583, 4012, 4902 );
    test_vehicle_drag( "scooter_electric", 0.609525, 0.189203, 142.864583, 4919, 5098 );
    test_vehicle_drag( "superbike", 0.609525, 0.844993, 377.789062, 9867, 11765 );
    test_vehicle_drag( "tandem", 0.609525, 0.021344, 40.290625, 2353, 3076 );
    test_vehicle_drag( "unicycle", 0.690795, 0.002400, 24.162500, 2266, 2958 );
    test_vehicle_drag( "beetle", 0.785610, 1.693784, 1199.020312, 8902, 10668 );
    test_vehicle_drag( "bubble_car", 0.823988, 1.670566, 1126.270833, 9434, 9785 );
    test_vehicle_drag( "car", 0.294604, 2.366883, 1117.002083, 11837, 14304 );
    test_vehicle_drag( "car_mini", 0.294604, 1.709414, 1210.084375, 12080, 14536 );
    test_vehicle_drag( "car_sports", 0.294604, 2.424461, 1373.010000, 20847, 24914 );
    test_vehicle_drag( "car_sports_atomic", 0.294604, 3.552642, 1676.596875, 24664, 25593 );
    test_vehicle_drag( "car_sports_electric", 0.294604, 3.192600, 1808.018750, 24774, 25702 );
    test_vehicle_drag( "electric_car", 0.304763, 2.363504, 1115.407292, 16218, 16834 );
    test_vehicle_drag( "rara_x", 0.880425, 2.099200, 1093.158405, 8484, 8803 );
    test_vehicle_drag( "suv", 0.294604, 2.807600, 1135.705357, 13939, 16801 );
    test_vehicle_drag( "suv_electric", 0.304763, 2.545244, 1029.579464, 16158, 16775 );
    test_vehicle_drag( "golf_cart", 0.943312, 1.477412, 929.645833, 7126, 7393 );
    test_vehicle_drag( "golf_cart_4seat", 0.943312, 1.444773, 909.108333, 7131, 7398 );
    test_vehicle_drag( "hearse", 0.355556, 3.016494, 1220.205357, 10995, 13320 );
    test_vehicle_drag( "pickup_technical", 0.838097, 2.820320, 1140.850893, 10136, 12149 );
    test_vehicle_drag( "ambulance", 1.049323, 2.264056, 1858.217708, 11256, 13438 );
    test_vehicle_drag( "car_fbi", 0.457144, 2.635064, 1243.564583, 14570, 17458 );
    test_vehicle_drag( "fire_engine", 2.305875, 3.485556, 2288.608036, 8654, 10334 );
    test_vehicle_drag( "fire_truck", 1.092446, 7.944023, 5266.181611, 10508, 12693 );
    test_vehicle_drag( "policecar", 0.629842, 2.636167, 1244.085417, 13182, 15775 );
    test_vehicle_drag( "policesuv", 0.629842, 2.967405, 1200.348214, 13124, 15720 );
    test_vehicle_drag( "truck_swat", 0.808830, 7.538017, 6186.806250, 9850, 11625 );
    test_vehicle_drag( "oldtractor", 0.537285, 0.868099, 1282.481250, 12353, 14339 );
    test_vehicle_drag( "autotractor", 1.425450, 1.992793, 1962.695000, 7294, 7565 );
    test_vehicle_drag( "tractor_plow", 0.609525, 0.893061, 1691.485577, 11852, 13756 );
    test_vehicle_drag( "tractor_reaper", 0.609525, 0.778836, 1475.139423, 11874, 13777 );
    test_vehicle_drag( "tractor_seed", 0.609525, 0.778836, 1475.139423, 11874, 13777 );
    test_vehicle_drag( "aapc-mg", 1.625400, 8.606709, 4351.886161, 7972, 9379 );
    test_vehicle_drag( "apc", 1.625400, 8.483520, 4289.597470, 7982, 9388 );
    test_vehicle_drag( "humvee", 0.616297, 7.253658, 4890.308036, 12871, 15129 );
    test_vehicle_drag( "military_cargo_truck", 0.840757, 9.416864, 4345.338889, 11502, 13545 );
    test_vehicle_drag( "flatbed_truck", 0.883328, 4.458495, 1995.980114, 9717, 11717 );
    test_vehicle_drag( "pickup", 0.589208, 3.088771, 1249.441964, 11219, 13499 );
    test_vehicle_drag( "semi_truck", 0.781317, 9.950414, 5749.409167, 11670, 13766 );
    test_vehicle_drag( "truck_trailer", 1.176534, 12.833835, 5688.000000, 0, 0 );
    test_vehicle_drag( "tatra_truck", 0.440213, 7.711005, 4121.069767, 17745, 20815 );
    test_vehicle_drag( "animalctrl", 0.345398, 2.717176, 1099.127679, 13283, 16007 );
    test_vehicle_drag( "autosweeper", 0.986850, 1.805107, 1277.825000, 6976, 7240 );
    test_vehicle_drag( "excavator", 0.659728, 1.749378, 1238.375000, 13141, 15260 );
    test_vehicle_drag( "road_roller", 1.823738, 2.745174, 9120.522917, 9382, 10893 );
    test_vehicle_drag( "forklift", 0.565988, 1.515984, 715.437500, 8361, 8679 );
    test_vehicle_drag( "trencher", 0.659728, 1.143758, 1299.788942, 8047, 8347 );
    test_vehicle_drag( "armored_car", 0.896872, 6.953143, 4687.705357, 11588, 13577 );
    test_vehicle_drag( "cube_van", 0.518580, 2.636314, 2163.746875, 11796, 14161 );
    test_vehicle_drag( "cube_van_cheap", 0.512775, 2.572474, 1853.869207, 9978, 12027 );
    test_vehicle_drag( "hippie_van", 0.386033, 2.780080, 1124.573214, 10804, 13062 );
    test_vehicle_drag( "icecream_truck", 0.681673, 3.818905, 2045.802027, 10606, 12783 );
    test_vehicle_drag( "lux_rv", 1.609183, 3.571591, 2015.961623, 8400, 9790 );
    test_vehicle_drag( "meth_lab", 0.518580, 3.025322, 2070.948138, 11677, 14056 );
    test_vehicle_drag( "rv", 0.541800, 3.000739, 2054.120213, 11528, 13872 );
    test_vehicle_drag( "schoolbus", 0.411188, 3.060324, 1370.046591, 12891, 15087 );
    test_vehicle_drag( "security_van", 0.541800, 7.592192, 6231.269792, 10977, 13009 );
    test_vehicle_drag( "wienermobile", 1.063697, 2.315334, 1900.304167, 11201, 13374 );
    test_vehicle_drag( "canoe", 0.609525, 6.948203, 1.967437, 331, 691 );
    test_vehicle_drag( "kayak", 0.609525, 3.243223, 1.224458, 655, 1236 );
    test_vehicle_drag( "kayak_racing", 0.609525, 2.912135, 1.099458, 715, 1320 );
    test_vehicle_drag( "DUKW", 0.776902, 3.713785, 80.325824, 10210, 12293 );
    test_vehicle_drag( "raft", 0.997815, 8.950399, 5.068750, 259, 548 );
    test_vehicle_drag( "inflatable_boat", 0.469560, 2.823845, 1.599187, 741, 1382 );

}
