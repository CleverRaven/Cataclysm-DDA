#include <cstdio>
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
    test_vehicle_drag( "bicycle_electric", 0.609525, 0.027581, 69.420833, 2753, 3268 );
    test_vehicle_drag( "motorcycle", 0.609525, 0.569952, 254.820312, 7296, 8687 );
    test_vehicle_drag( "motorcycle_sidecart", 0.880425, 0.859065, 455.206250, 6423, 7657 );
    test_vehicle_drag( "quad_bike", 0.537285, 1.112797, 710.745536, 7457, 8918 );
    test_vehicle_drag( "scooter", 0.609525, 0.172681, 130.389583, 4273, 5082 );
    test_vehicle_drag( "scooter_electric", 0.609525, 0.183133, 138.281250, 4825, 5001 );
    test_vehicle_drag( "superbike", 0.609525, 0.846042, 378.257812, 9912, 11797 );
    test_vehicle_drag( "tandem", 0.609525, 0.021592, 40.759375, 2353, 3076 );
    test_vehicle_drag( "unicycle", 0.690795, 0.002493, 25.100000, 2266, 2958 );
    test_vehicle_drag( "beetle", 0.785610, 1.644624, 1164.220312, 8995, 10735 );
    test_vehicle_drag( "bubble_car", 0.823988, 1.373576, 926.044643, 9365, 9711 );
    test_vehicle_drag( "car", 0.294604, 2.317723, 1093.802083, 11973, 14404 );
    test_vehicle_drag( "car_mini", 0.294604, 1.660254, 1175.284375, 12215, 14636 );
    test_vehicle_drag( "car_sports", 0.294604, 2.391878, 1354.557500, 20898, 24953 );
    test_vehicle_drag( "car_sports_atomic", 0.294604, 3.554633, 1677.536458, 23767, 24664 );
    test_vehicle_drag( "car_sports_electric", 0.294604, 3.241243, 1835.566250, 23863, 24760 );
    test_vehicle_drag( "electric_car", 0.304763, 2.329794, 1099.498958, 16259, 16876 );
    test_vehicle_drag( "rara_x", 0.880425, 1.902452, 990.701509, 8408, 8724 );
    test_vehicle_drag( "suv", 0.294604, 2.758440, 1115.819643, 14042, 16878 );
    test_vehicle_drag( "suv_electric", 0.304763, 2.511535, 1015.943750, 16200, 16817 );
    test_vehicle_drag( "golf_cart", 0.943313, 1.472114, 926.312500, 7039, 7303 );
    test_vehicle_drag( "golf_cart_4seat", 0.943313, 1.439476, 905.775000, 7044, 7308 );
    test_vehicle_drag( "hearse", 0.355556, 2.827377, 1143.705357, 11164, 13453 );
    test_vehicle_drag( "pickup_technical", 0.838097, 2.724950, 1102.272321, 10210, 12205 );
    test_vehicle_drag( "ambulance", 1.049323, 2.222427, 1824.051042, 11321, 13485 );
    test_vehicle_drag( "car_fbi", 0.457144, 2.585904, 1220.364583, 14661, 17525 );
    test_vehicle_drag( "fire_engine", 2.305875, 3.454497, 2268.215179, 8702, 10369 );
    test_vehicle_drag( "fire_truck", 1.092446, 7.902344, 5238.551803, 10573, 12740 );
    test_vehicle_drag( "policecar", 0.629843, 2.587007, 1220.885417, 13262, 15834 );
    test_vehicle_drag( "policesuv", 0.629843, 2.918245, 1180.462500, 13204, 15779 );
    test_vehicle_drag( "truck_swat", 0.808830, 7.563401, 6207.639583, 9935, 11688 );
    test_vehicle_drag( "oldtractor", 0.537285, 0.893482, 1319.981250, 12446, 14408 );
    test_vehicle_drag( "autotractor", 1.425450, 2.012592, 1982.195000, 7203, 7471 );
    test_vehicle_drag( "tractor_plow", 0.609525, 0.918444, 1739.562500, 11941, 13822 );
    test_vehicle_drag( "tractor_reaper", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "tractor_seed", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "aapc-mg", 1.625400, 8.660271, 4378.969494, 8038, 9427 );
    test_vehicle_drag( "apc", 1.625400, 8.537083, 4316.680804, 8047, 9436 );
    test_vehicle_drag( "humvee", 0.616297, 7.288223, 4913.611607, 12935, 15175 );
    test_vehicle_drag( "military_cargo_truck", 0.840757, 9.507160, 4387.005556, 11554, 13581 );
    test_vehicle_drag( "flatbed_truck", 0.883327, 4.376577, 1959.307386, 9818, 11790 );
    test_vehicle_drag( "pickup", 0.589208, 2.992958, 1210.684821, 11340, 13589 );
    test_vehicle_drag( "semi_truck", 0.781317, 9.931077, 5738.235833, 11737, 13816 );
    test_vehicle_drag( "truck_trailer", 1.176534, 12.890242, 5713.000000, 0, 0 );
    test_vehicle_drag( "tatra_truck", 0.440213, 7.636835, 4081.430233, 17812, 20868 );
    test_vehicle_drag( "animalctrl", 0.345398, 2.668016, 1079.241964, 13420, 16107 );
    test_vehicle_drag( "autosweeper", 0.986850, 1.844396, 1305.637500, 6884, 7144 );
    test_vehicle_drag( "excavator", 0.659728, 1.793523, 1269.625000, 13204, 15305 );
    test_vehicle_drag( "road_roller", 1.823738, 2.729505, 9068.464583, 9433, 10931 );
    test_vehicle_drag( "forklift", 0.565988, 1.510686, 712.937500, 8258, 8572 );
    test_vehicle_drag( "trencher", 0.659728, 1.166350, 1325.462019, 7944, 8240 );
    test_vehicle_drag( "armored_car", 0.896872, 6.982755, 4707.669643, 11645, 13619 );
    test_vehicle_drag( "cube_van", 0.518580, 2.594634, 2129.538542, 11876, 14220 );
    test_vehicle_drag( "cube_van_cheap", 0.512775, 2.530795, 1823.832622, 10085, 12105 );
    test_vehicle_drag( "hippie_van", 0.386033, 2.730920, 1104.687500, 10926, 13152 );
    test_vehicle_drag( "icecream_truck", 0.681673, 3.723092, 1994.475000, 10719, 12867 );
    test_vehicle_drag( "lux_rv", 1.609183, 3.459129, 1952.483443, 8479, 9850 );
    test_vehicle_drag( "meth_lab", 0.518580, 2.994264, 2049.687500, 11790, 14138 );
    test_vehicle_drag( "rv", 0.541800, 2.972401, 2034.721277, 11639, 13952 );
    test_vehicle_drag( "schoolbus", 0.411187, 2.794265, 1250.937500, 13069, 15234 );
    test_vehicle_drag( "security_van", 0.541800, 7.617575, 6252.103125, 11074, 13079 );
    test_vehicle_drag( "wienermobile", 1.063697, 2.273655, 1866.095833, 11266, 13420 );
    test_vehicle_drag( "canoe", 0.609525, 7.741047, 2.191938, 298, 628 );
    test_vehicle_drag( "kayak", 0.609525, 4.036067, 1.523792, 544, 1067 );
    test_vehicle_drag( "kayak_racing", 0.609525, 3.704980, 1.398792, 586, 1133 );
    test_vehicle_drag( "DUKW", 0.776902, 3.610873, 78.099941, 10319, 12373 );
    test_vehicle_drag( "raft", 0.997815, 9.743243, 5.517750, 239, 508 );
    test_vehicle_drag( "inflatable_boat", 0.469560, 3.616690, 2.048188, 602, 1173 );
}
