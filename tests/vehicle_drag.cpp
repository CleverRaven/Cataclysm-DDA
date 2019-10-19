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
    test_vehicle_drag( "bicycle", 0.609525, 0.017205, 43.304167, 2355, 3078 );
    test_vehicle_drag( "bicycle_electric", 0.609525, 0.031865, 80.202083, 2722, 3243 );
    test_vehicle_drag( "motorcycle", 0.609525, 0.569952, 254.820312, 7296, 8687 );
    test_vehicle_drag( "motorcycle_sidecart", 0.880425, 0.859065, 455.206250, 6423, 7657 );
    test_vehicle_drag( "quad_bike", 0.537285, 1.112797, 710.745536, 7457, 8918 );
    test_vehicle_drag( "scooter", 0.609525, 0.172681, 130.389583, 4273, 5082 );
    test_vehicle_drag( "scooter_electric", 0.609525, 0.190030, 143.489583, 4919, 5098 );
    test_vehicle_drag( "superbike", 0.609525, 0.846042, 378.257812, 9912, 11797 );
    test_vehicle_drag( "tandem", 0.609525, 0.021592, 40.759375, 2353, 3076 );
    test_vehicle_drag( "unicycle", 0.690795, 0.002493, 25.100000, 2266, 2958 );
    test_vehicle_drag( "beetle", 0.785610, 1.737929, 1230.270313, 8979, 10720 );
    test_vehicle_drag( "bubble_car", 0.823988, 1.714711, 1156.032738, 9427, 9778 );
    test_vehicle_drag( "car", 0.294604, 2.411028, 1137.835417, 11939, 14372 );
    test_vehicle_drag( "car_mini", 0.294604, 1.753559, 1241.334375, 12180, 14603 );
    test_vehicle_drag( "car_sports", 0.294604, 2.485183, 1407.397500, 20868, 24924 );
    test_vehicle_drag( "car_sports_atomic", 0.294604, 3.613363, 1705.253125, 24646, 25575 );
    test_vehicle_drag( "car_sports_electric", 0.294604, 3.253321, 1842.406250, 24755, 25684 );
    test_vehicle_drag( "electric_car", 0.304763, 2.407649, 1136.240625, 16203, 16819 );
    test_vehicle_drag( "rara_x", 0.880425, 2.130240, 1109.322198, 8479, 8799 );
    test_vehicle_drag( "suv", 0.294604, 2.851745, 1153.562500, 14010, 16847 );
    test_vehicle_drag( "suv_electric", 0.304763, 2.589389, 1047.436607, 16144, 16760 );
    test_vehicle_drag( "golf_cart", 0.943313, 1.479398, 930.895833, 7126, 7393 );
    test_vehicle_drag( "golf_cart_4seat", 0.943313, 1.446760, 910.358333, 7131, 7398 );
    test_vehicle_drag( "hearse", 0.355556, 3.060639, 1238.062500, 11093, 13385 );
    test_vehicle_drag( "pickup_technical", 0.838097, 2.860051, 1156.922321, 10190, 12186 );
    test_vehicle_drag( "ambulance", 1.049323, 2.289490, 1879.092708, 11313, 13478 );
    test_vehicle_drag( "car_fbi", 0.457144, 2.679209, 1264.397917, 14640, 17504 );
    test_vehicle_drag( "fire_engine", 2.305875, 3.526351, 2315.393750, 8698, 10365 );
    test_vehicle_drag( "fire_truck", 1.092446, 7.969406, 5283.008534, 10566, 12733 );
    test_vehicle_drag( "policecar", 0.629843, 2.680312, 1264.918750, 13246, 15818 );
    test_vehicle_drag( "policesuv", 0.629843, 3.011550, 1218.205357, 13188, 15763 );
    test_vehicle_drag( "truck_swat", 0.808830, 7.563401, 6207.639583, 9935, 11688 );
    test_vehicle_drag( "oldtractor", 0.537285, 0.893482, 1319.981250, 12446, 14408 );
    test_vehicle_drag( "autotractor", 1.425450, 2.018176, 1987.695000, 7291, 7563 );
    test_vehicle_drag( "tractor_plow", 0.609525, 0.918444, 1739.562500, 11941, 13822 );
    test_vehicle_drag( "tractor_reaper", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "tractor_seed", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "aapc-mg", 1.625400, 8.660271, 4378.969494, 8038, 9427 );
    test_vehicle_drag( "apc", 1.625400, 8.537083, 4316.680804, 8047, 9436 );
    test_vehicle_drag( "humvee", 0.616297, 7.282793, 4909.950893, 12936, 15176 );
    test_vehicle_drag( "military_cargo_truck", 0.840757, 9.507160, 4387.005556, 11554, 13581 );
    test_vehicle_drag( "flatbed_truck", 0.883327, 4.483878, 2007.343750, 9803, 11777 );
    test_vehicle_drag( "pickup", 0.589208, 3.132916, 1267.299107, 11313, 13563 );
    test_vehicle_drag( "semi_truck", 0.781317, 10.022526, 5791.075833, 11725, 13805 );
    test_vehicle_drag( "truck_trailer", 1.176534, 12.890242, 5713.000000, 0, 0 );
    test_vehicle_drag( "tatra_truck", 0.440213, 7.751800, 4142.872093, 17788, 20845 );
    test_vehicle_drag( "animalctrl", 0.345398, 2.761321, 1116.984821, 13392, 16080 );
    test_vehicle_drag( "autosweeper", 0.986850, 1.849252, 1309.075000, 6970, 7233 );
    test_vehicle_drag( "excavator", 0.659728, 1.793523, 1269.625000, 13204, 15305 );
    test_vehicle_drag( "road_roller", 1.823738, 2.752699, 9145.522917, 9431, 10929 );
    test_vehicle_drag( "forklift", 0.565988, 1.517970, 716.375000, 8361, 8678 );
    test_vehicle_drag( "trencher", 0.659728, 1.169142, 1328.635096, 8042, 8342 );
    test_vehicle_drag( "armored_car", 0.896872, 6.980292, 4706.008929, 11645, 13619 );
    test_vehicle_drag( "cube_van", 0.518580, 2.661697, 2184.580208, 11862, 14206 );
    test_vehicle_drag( "cube_van_cheap", 0.512775, 2.597858, 1872.161890, 10070, 12091 );
    test_vehicle_drag( "hippie_van", 0.386033, 2.824225, 1142.430357, 10899, 13126 );
    test_vehicle_drag( "icecream_truck", 0.681673, 3.863050, 2069.450676, 10696, 12845 );
    test_vehicle_drag( "lux_rv", 1.609183, 3.645947, 2057.931689, 8463, 9836 );
    test_vehicle_drag( "meth_lab", 0.518580, 3.066117, 2098.873670, 11775, 14123 );
    test_vehicle_drag( "rv", 0.541800, 3.044254, 2083.907447, 11624, 13937 );
    test_vehicle_drag( "schoolbus", 0.411187, 3.116167, 1395.046591, 12986, 15154 );
    test_vehicle_drag( "security_van", 0.541800, 7.617575, 6252.103125, 11074, 13079 );
    test_vehicle_drag( "wienermobile", 1.063697, 2.340718, 1921.137500, 11258, 13413 );
    test_vehicle_drag( "canoe", 0.609525, 6.948203, 1.967437, 331, 691 );
    test_vehicle_drag( "kayak", 0.609525, 3.243223, 1.224458, 655, 1236 );
    test_vehicle_drag( "kayak_racing", 0.609525, 2.912135, 1.099458, 715, 1320 );
    test_vehicle_drag( "DUKW", 0.776902, 3.754579, 81.208176, 10298, 12353 );
    test_vehicle_drag( "raft", 0.997815, 8.950399, 5.068750, 259, 548 );
    test_vehicle_drag( "inflatable_boat", 0.469560, 2.823845, 1.599187, 741, 1382 );

}
