#include <cstdio>
#include <memory>
#include <string>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_helpers.h"
#include "point.h"
#include "player_helpers.h"
#include "string_formatter.h"
#include "test_statistics.h"
#include "type_id.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

using efficiency_stat = statistics<long>;

const efftype_id effect_blind( "blind" );

static void clear_game_drag( const ter_id &terrain )
{
    // Set to turn 0 to prevent solars from producing power
    calendar::turn = 0;
    clear_creatures();
    clear_npcs();
    clear_avatar();

    // Move player somewhere safe
    CHECK( !g->u.in_vehicle );
    g->u.setpos( tripoint_zero );
    // Blind the player to avoid needless drawing-related overhead
    g->u.add_effect( effect_blind, 1_turns, num_bp, true );
    // Make sure the ST is 8 so that muscle powered results are consistent
    g->u.str_cur = 8;

    clear_vehicles();
    build_test_map( terrain );

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
        cata_printf( "    test_vehicle_drag( \"%s\", %f, %f, %f, %d, %d );\n",
                     veh_id.c_str(), c_air, c_rolling, c_water, safe_v, max_v );
    }
    return valid;
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
        test_drag( vproto_id( veh ) );
    }
}

// format is vehicle, coeff_air_drag, coeff_rolling_drag, coeff_water_drag, safe speed, max speed
// coeffs are dimensionless, speeds are 100ths of mph, so 6101 is 61.01 mph
TEST_CASE( "vehicle_drag", "[vehicle] [engine]" )
{
    test_vehicle_drag( "bicycle", 0.609525, 0.017205, 43.304167, 2355, 3078 );
    test_vehicle_drag( "bicycle_electric", 0.609525, 0.027581, 69.420833, 2753, 3268 );
    test_vehicle_drag( "motorcycle", 0.609525, 0.569952, 254.820312, 7296, 8687 );
    test_vehicle_drag( "motorcycle_sidecart", 0.880425, 0.859065, 455.206250, 6423, 7657 );
    test_vehicle_drag( "quad_bike", 0.537285, 1.112797, 710.745536, 7457, 8918 );
    test_vehicle_drag( "scooter", 0.609525, 0.172681, 130.389583, 4273, 5082 );
    test_vehicle_drag( "scooter_electric", 0.609525, 0.183133, 138.281250, 4825, 5001 );
    test_vehicle_drag( "superbike", 0.609525, 0.846042, 378.257812, 9912, 11797 );
    test_vehicle_drag( "tandem", 0.609525, 0.021592, 40.759375, 2353, 3076 );
    test_vehicle_drag( "unicycle", 0.690795, 0.002493, 25.100000, 2266, 2958 );
    test_vehicle_drag( "beetle", 0.785610, 1.800385, 1274.482813, 8969, 10711 );
    test_vehicle_drag( "bubble_car", 0.823988, 1.918740, 1293.586310, 9280, 9627 );
    test_vehicle_drag( "car", 0.294604, 2.473484, 1167.310417, 11916, 14350 );
    test_vehicle_drag( "car_mini", 0.294604, 1.816015, 1285.546875, 12157, 14580 );
    test_vehicle_drag( "car_sports", 0.294604, 2.547639, 1442.767500, 20848, 24904 );
    test_vehicle_drag( "car_sports_atomic", 0.294604, 3.788275, 1787.798958, 23696, 24593 );
    test_vehicle_drag( "car_sports_electric", 0.294604, 3.397004, 1923.776250, 23815, 24712 );
    test_vehicle_drag( "electric_car", 0.304763, 2.485556, 1173.007292, 16208, 16826 );
    test_vehicle_drag( "rara_x", 0.880425, 2.267517, 1180.809267, 8353, 8670 );
    test_vehicle_drag( "suv", 0.294604, 2.914201, 1178.826786, 13988, 16827 );
    test_vehicle_drag( "suv_electric", 0.304763, 2.667296, 1078.950893, 16149, 16767 );
    test_vehicle_drag( "golf_cart", 0.943313, 1.472114, 926.312500, 7039, 7303 );
    test_vehicle_drag( "golf_cart_4seat", 0.943313, 1.439476, 905.775000, 7044, 7308 );
    test_vehicle_drag( "hearse", 0.355556, 3.216780, 1301.223214, 11046, 13340 );
    test_vehicle_drag( "pickup_technical", 0.838097, 2.958591, 1196.783036, 10176, 12173 );
    test_vehicle_drag( "ambulance", 1.049323, 2.334381, 1915.936458, 11308, 13473 );
    test_vehicle_drag( "car_fbi", 0.457144, 2.741665, 1293.872917, 14625, 17491 );
    test_vehicle_drag( "fire_engine", 2.305875, 3.574448, 2346.974107, 8695, 10362 );
    test_vehicle_drag( "fire_truck", 1.092446, 8.014297, 5312.766947, 10561, 12729 );
    test_vehicle_drag( "policecar", 0.629843, 2.742769, 1294.393750, 13235, 15808 );
    test_vehicle_drag( "policesuv", 0.629843, 3.074006, 1243.469643, 13177, 15753 );
    test_vehicle_drag( "truck_swat", 0.808830, 7.563401, 6207.639583, 9935, 11688 );
    test_vehicle_drag( "oldtractor", 0.537285, 0.893482, 1319.981250, 12446, 14408 );
    test_vehicle_drag( "autotractor", 1.425450, 2.317193, 2282.195000, 7172, 7441 );
    test_vehicle_drag( "tractor_plow", 0.609525, 0.918444, 1739.562500, 11941, 13822 );
    test_vehicle_drag( "tractor_reaper", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "tractor_seed", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "aapc-mg", 1.625400, 8.660271, 4378.969494, 8038, 9427 );
    test_vehicle_drag( "apc", 1.625400, 8.537083, 4316.680804, 8047, 9436 );
    test_vehicle_drag( "humvee", 0.616298, 7.288223, 4913.611607, 12935, 15175 );
    test_vehicle_drag( "military_cargo_truck", 0.840758, 9.507160, 4387.005556, 11554, 13581 );
    test_vehicle_drag( "flatbed_truck", 0.883328, 7.754008, 3471.316477, 9376, 11371 );
    test_vehicle_drag( "pickup", 0.589208, 3.756340, 1519.481250, 11194, 13450 );
    test_vehicle_drag( "semi_truck", 0.781317, 10.083741, 5826.445833, 11718, 13797 );
    test_vehicle_drag( "truck_trailer", 1.176534, 18.305362, 8113.000000, 0, 0 );
    test_vehicle_drag( "tatra_truck", 0.440213, 8.807828, 4707.255814, 17575, 20635 );
    test_vehicle_drag( "animalctrl", 0.345398, 2.823777, 1142.249107, 13373, 16062 );
    test_vehicle_drag( "autosweeper", 0.986850, 1.844396, 1305.637500, 6884, 7144 );
    test_vehicle_drag( "excavator", 0.659728, 1.793523, 1269.625000, 13204, 15305 );
    test_vehicle_drag( "road_roller", 1.823737, 2.768224, 9197.104167, 9430, 10928 );
    test_vehicle_drag( "forklift", 0.565988, 1.510686, 712.937500, 8258, 8572 );
    test_vehicle_drag( "trencher", 0.659728, 1.166350, 1325.462019, 7944, 8240 );
    test_vehicle_drag( "armored_car", 0.896873, 6.982755, 4707.669643, 11645, 13619 );
    test_vehicle_drag( "cube_van", 0.518580, 3.620389, 2971.423958, 11657, 14011 );
    test_vehicle_drag( "cube_van_cheap", 0.512775, 3.556550, 2563.049085, 9853, 11885 );
    test_vehicle_drag( "hippie_van", 0.386032, 2.886681, 1167.694643, 10881, 13109 );
    test_vehicle_drag( "icecream_truck", 0.681673, 3.956734, 2119.637838, 10680, 12830 );
    test_vehicle_drag( "lux_rv", 1.609183, 3.770999, 2128.516557, 8453, 9826 );
    test_vehicle_drag( "meth_lab", 0.518580, 3.114214, 2131.797872, 11765, 14113 );
    test_vehicle_drag( "rv", 0.541800, 3.092351, 2116.831649, 11614, 13928 );
    test_vehicle_drag( "schoolbus", 0.411187, 3.331642, 1491.510227, 12930, 15101 );
    test_vehicle_drag( "security_van", 0.541800, 7.617575, 6252.103125, 11074, 13079 );
    test_vehicle_drag( "wienermobile", 1.063697, 2.385608, 1957.981250, 11253, 13409 );
    test_vehicle_drag( "canoe", 0.609525, 7.741047, 2.191938, 298, 628 );
    test_vehicle_drag( "kayak", 0.609525, 4.036067, 1.523792, 544, 1067 );
    test_vehicle_drag( "kayak_racing", 0.609525, 3.704980, 1.398792, 586, 1133 );
    test_vehicle_drag( "DUKW", 0.776903, 5.808919, 125.641706, 9993, 12063 );
    test_vehicle_drag( "raft", 0.997815, 9.743243, 5.517750, 239, 508 );
    test_vehicle_drag( "inflatable_boat", 0.469560, 3.616690, 2.048188, 602, 1173 );
}
