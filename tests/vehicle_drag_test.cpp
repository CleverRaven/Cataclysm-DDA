#include <cstdio>
#include <iosfwd>
#include <string>
#include <vector>

#include "calendar.h"
#include "catch/catch.hpp"
#include "character.h"
#include "map.h"
#include "map_helpers.h"
#include "point.h"
#include "test_statistics.h"
#include "type_id.h"
#include "units.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

using efficiency_stat = statistics<long>;

static const efftype_id effect_blind( "blind" );

static void clear_game_drag( const ter_id &terrain )
{
    // Set to turn 0 to prevent solars from producing power
    calendar::turn = calendar::turn_zero;
    clear_creatures();
    clear_npcs();

    Character &player_character = get_player_character();
    // Move player somewhere safe
    CHECK( !player_character.in_vehicle );
    player_character.setpos( tripoint_zero );
    // Blind the player to avoid needless drawing-related overhead
    player_character.add_effect( effect_blind, 1_turns, true );
    // Make sure the ST is 8 so that muscle powered results are consistent
    player_character.str_cur = 8;

    build_test_map( terrain );

    map &here = get_map();
    // hard force a rebuild of caches
    here.shift( point_south );
    here.shift( point_north );
}

static vehicle *setup_drag_test( const vproto_id &veh_id )
{
    clear_vehicles();
    const tripoint map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = get_map().add_vehicle( veh_id, map_starting_point, -90_degrees, 0, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return nullptr;
    }

    // Remove all items from cargo to normalize weight.
    // turn everything on
    for( const vpart_reference &vp : veh_ptr->get_all_parts() ) {
        veh_ptr->get_items( vp.part_index() ).clear();
        veh_ptr->toggle_specific_part( vp.part_index(), true );
    }
    // close the doors
    const auto doors = veh_ptr->get_avail_parts( "OPENABLE" );
    for( const vpart_reference &vp :  doors ) {
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
    const std::string &type, const double expected_c_air, const double expected_c_rr,
    const double expected_c_water, const int expected_safe, const int expected_max )
{
    test_drag( vproto_id( type ), expected_c_air, expected_c_rr, expected_c_water,
               expected_safe, expected_max, true );
}

static std::vector<std::string> vehs_to_test_drag = {
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
    clear_game_drag( ter_id( "t_pavement" ) );

    for( const std::string &veh : vehs_to_test_drag ) {
        print_drag_test_strings( veh );
    }
}

// format is vehicle, coeff_air_drag, coeff_rolling_drag, coeff_water_drag, safe speed, max speed
// coeffs are dimensionless, speeds are 100ths of mph, so 6101 is 61.01 mph
TEST_CASE( "vehicle_drag", "[vehicle] [engine]" )
{
    clear_game_drag( ter_id( "t_pavement" ) );

    test_vehicle_drag( "bicycle", 0.609525, 0.008953, 22.535417, 1431, 1871 );
    test_vehicle_drag( "bicycle_electric", 0.609525, 0.018005, 45.318750, 2338, 2564 );
    test_vehicle_drag( "motorcycle", 0.609525, 0.569952, 254.820312, 7296, 8687 );
    test_vehicle_drag( "motorcycle_sidecart", 0.880425, 0.859065, 455.206250, 6423, 7657 );
    test_vehicle_drag( "quad_bike", 0.537285, 1.112797, 710.745536, 7457, 8918 );
    test_vehicle_drag( "scooter", 0.609525, 0.154345, 116.543750, 4279, 5088 );
    test_vehicle_drag( "scooter_electric", 0.609525, 0.164796, 124.435417, 4831, 5006 );
    test_vehicle_drag( "superbike", 0.609525, 0.846042, 378.257812, 9912, 11797 );
    test_vehicle_drag( "tandem", 0.609525, 0.010590, 19.990625, 1430, 1870 );
    test_vehicle_drag( "unicycle", 0.690795, 0.002493, 25.100000, 1377, 1798 );
    test_vehicle_drag( "beetle", 0.785610, 1.802151, 1275.732812, 8969, 10710 );
    test_vehicle_drag( "bubble_car", 0.823988, 1.764712, 1189.742560, 9304, 9651 );
    test_vehicle_drag( "car", 0.294604, 2.473484, 1167.310417, 11916, 14350 );
    test_vehicle_drag( "car_mini", 0.294604, 1.817781, 1286.796875, 12157, 14580 );
    test_vehicle_drag( "car_sports", 0.294604, 2.549405, 1443.767500, 20848, 24904 );
    test_vehicle_drag( "car_sports_atomic", 0.294604, 3.788275, 1787.798958, 23696, 24593 );
    test_vehicle_drag( "car_sports_electric", 0.294604, 3.119641, 1766.701250, 23901, 24797 );
    test_vehicle_drag( "electric_car", 0.304763, 2.311289, 1090.765625, 16266, 16882 );
    test_vehicle_drag( "rara_x", 0.679185, 1.952527, 1092.094907, 9114, 9459 );
    test_vehicle_drag( "suv", 0.294604, 2.914201, 1178.826786, 13988, 16827 );
    test_vehicle_drag( "suv_electric", 0.304763, 2.461925, 995.875893, 16216, 16833 );
    test_vehicle_drag( "golf_cart", 0.943313, 1.472114, 926.312500, 7039, 7303 );
    test_vehicle_drag( "golf_cart_4seat", 0.943313, 1.439476, 905.775000, 7044, 7308 );
    test_vehicle_drag( "hearse", 0.355556, 3.216780, 1301.223214, 11046, 13340 );
    test_vehicle_drag( "pickup_technical", 0.838097, 2.958591, 1196.783036, 10176, 12173 );
    test_vehicle_drag( "ambulance", 1.049323, 2.348252, 1927.320833, 11306, 13472 );
    test_vehicle_drag( "car_fbi", 0.457144, 2.743431, 1294.706250, 14625, 17490 );
    test_vehicle_drag( "fire_engine", 2.305875, 3.544531, 2327.331250, 8697, 10364 );
    test_vehicle_drag( "fire_truck", 1.056510, 8.175862, 5419.870313, 10648, 12841 );
    test_vehicle_drag( "policecar", 0.629843, 2.744534, 1295.227083, 13235, 15807 );
    test_vehicle_drag( "policesuv", 0.629843, 3.096796, 1252.688393, 13173, 15749 );
    test_vehicle_drag( "truck_swat", 0.808830, 7.607580, 6243.900000, 9929, 11682 );
    test_vehicle_drag( "oldtractor", 0.537285, 0.893482, 1319.981250, 12446, 14408 );
    test_vehicle_drag( "autotractor", 1.425450, 2.044321, 2013.445000, 7779, 8068 );
    test_vehicle_drag( "tractor_plow", 0.609525, 0.918444, 1739.562500, 11941, 13822 );
    test_vehicle_drag( "tractor_reaper", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "tractor_seed", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "aapc-mg", 1.625400, 8.660271, 4378.969494, 8038, 9427 );
    test_vehicle_drag( "apc", 1.625400, 8.538354, 4317.323661, 8047, 9436 );
    test_vehicle_drag( "humvee", 0.616297, 7.288356, 4913.700893, 12935, 15175 );
    test_vehicle_drag( "military_cargo_truck", 0.840757, 9.507160, 4387.005556, 11554, 13581 );
    test_vehicle_drag( "flatbed_truck", 0.776902, 4.556718, 2039.952841, 10177, 12239 );
    test_vehicle_drag( "pickup", 0.589208, 3.264179, 1320.396429, 11288, 13539 );
    test_vehicle_drag( "semi_truck", 0.792020, 10.185646, 5885.327500, 11661, 13732 );
    test_vehicle_drag( "truck_trailer", 1.196475, 13.127154, 5818.000000, 0, 0 );
    test_vehicle_drag( "tatra_truck", 0.968467, 8.434611, 4507.793605, 14050, 16400 );
    test_vehicle_drag( "animalctrl", 0.396191, 2.829350, 1144.503571, 12832, 15400 );
    test_vehicle_drag( "autosweeper", 0.986850, 1.844396, 1305.637500, 6884, 7144 );
    test_vehicle_drag( "excavator", 0.659728, 1.793523, 1269.625000, 13204, 15305 );
    test_vehicle_drag( "road_roller", 1.823738, 2.768224, 9197.104167, 9430, 10928 );
    test_vehicle_drag( "forklift", 0.565988, 1.510686, 712.937500, 8258, 8572 );
    test_vehicle_drag( "trencher", 0.520838, 1.173965, 1334.115865, 8559, 8881 );
    test_vehicle_drag( "armored_car", 0.844628, 7.034173, 4742.334821, 11846, 13860 );
    test_vehicle_drag( "cube_van", 0.518580, 2.707603, 2222.257292, 11852, 14196 );
    test_vehicle_drag( "cube_van_cheap", 0.512775, 2.643764, 1905.244207, 10060, 12081 );
    test_vehicle_drag( "hippie_van", 0.375874, 2.698624, 1091.623214, 11022, 13267 );
    test_vehicle_drag( "icecream_truck", 0.681673, 3.271602, 2225.536486, 10796, 12939 );
    test_vehicle_drag( "lux_rv", 1.630929, 4.101402, 2315.010526, 8390, 9759 );
    test_vehicle_drag( "meth_lab", 0.499230, 4.185080, 2538.355452, 11668, 14057 );
    test_vehicle_drag( "rv", 0.541800, 3.107252, 2127.031915, 11611, 13925 );
    test_vehicle_drag( "schoolbus", 0.445050, 4.727095, 2140.748136, 12301, 14426 );
    test_vehicle_drag( "security_van", 0.541800, 8.004081, 6569.327083, 11003, 13010 );
    test_vehicle_drag( "wienermobile", 1.281891, 2.613527, 2145.044792, 10576, 12602 );
    test_vehicle_drag( "canoe", 0.609525, 7.741047, 2.191938, 298, 628 );
    test_vehicle_drag( "kayak", 0.609525, 2.935863, 1.108417, 710, 1314 );
    test_vehicle_drag( "kayak_racing", 0.609525, 2.604776, 0.983417, 779, 1406 );
    test_vehicle_drag( "DUKW", 0.776902, 3.850773, 83.288765, 10283, 12339 );
    test_vehicle_drag( "raft", 0.997815, 9.743243, 5.517750, 239, 508 );
    test_vehicle_drag( "inflatable_boat", 0.469560, 3.616690, 2.048188, 602, 1173 );
}
