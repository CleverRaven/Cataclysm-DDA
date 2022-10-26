#include <cstdio>
#include <iosfwd>
#include <string>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
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
    test_vehicle_drag( "bicycle_electric", 0.609525, 0.017178, 43.235417, 2339, 2565 );
    test_vehicle_drag( "motorcycle", 0.609525, 0.606647, 271.226563, 7297, 8688 );
    test_vehicle_drag( "motorcycle_sidecart", 0.880425, 0.900347, 477.081250, 6424, 7658 );
    test_vehicle_drag( "quad_bike", 0.537285, 1.106507, 706.727679, 7459, 8920 );
    test_vehicle_drag( "scooter", 0.609525, 0.166761, 125.918750, 4280, 5089 );
    test_vehicle_drag( "scooter_electric", 0.609525, 0.108235, 81.727083, 4848, 5023 );
    test_vehicle_drag( "superbike", 0.609525, 0.851284, 380.601562, 9911, 11796 );
    test_vehicle_drag( "tandem", 0.609525, 0.010590, 19.990625, 1430, 1870 );
    test_vehicle_drag( "unicycle", 0.690795, 0.002493, 25.100000, 1377, 1798 );
    test_vehicle_drag( "beetle", 0.785610, 1.588728, 1124.651562, 9004, 10743 );
    test_vehicle_drag( "bubble_car", 0.823988, 0.958134, 645.959821, 9431, 9775 );
    test_vehicle_drag( "car", 0.294604, 2.260061, 1066.589583, 11994, 14424 );
    test_vehicle_drag( "car_mini", 0.294604, 1.604357, 1135.715625, 12236, 14656 );
    test_vehicle_drag( "car_sports", 0.294604, 2.203546, 1247.902500, 20916, 24970 );
    test_vehicle_drag( "car_sports_atomic", 0.294604, 3.185611, 1503.384375, 23880, 24777 );
    test_vehicle_drag( "car_sports_electric", 0.294604, 1.798178, 1018.336250, 24314, 25208 );
    test_vehicle_drag( "electric_car", 0.304763, 1.541638, 727.544792, 16521, 17136 );
    test_vehicle_drag( "rara_x", 0.679185, 1.164526, 651.347222, 9263, 9606 );
    test_vehicle_drag( "suv", 0.294604, 2.700778, 1092.494643, 14062, 16897 );
    test_vehicle_drag( "suv_electric", 0.304763, 1.692274, 684.543750, 16471, 17086 );
    test_vehicle_drag( "golf_cart", 0.943313, 1.058255, 665.895833, 7103, 7366 );
    test_vehicle_drag( "golf_cart_4seat", 0.943313, 1.025616, 645.358333, 7109, 7371 );
    test_vehicle_drag( "hearse", 0.355556, 2.676984, 1082.869643, 11210, 13498 );
    test_vehicle_drag( "pickup_technical", 0.838097, 2.639781, 1067.820536, 10222, 12216 );
    test_vehicle_drag( "ambulance", 1.049323, 2.194219, 1800.898958, 11324, 13488 );
    test_vehicle_drag( "car_fbi", 0.457144, 2.530007, 1193.985417, 14674, 17537 );
    test_vehicle_drag( "fire_engine", 2.305875, 2.866843, 1882.362500, 8736, 10401 );
    test_vehicle_drag( "fire_truck", 1.056510, 7.865258, 5213.967668, 10681, 12872 );
    test_vehicle_drag( "policecar", 0.629843, 2.531111, 1194.506250, 13272, 15843 );
    test_vehicle_drag( "policesuv", 0.629843, 2.883373, 1166.356250, 13210, 15785 );
    test_vehicle_drag( "truck_swat", 0.808830, 8.229391, 6754.248958, 9845, 11601 );
    test_vehicle_drag( "oldtractor", 0.537285, 0.896020, 1323.731250, 12446, 14407 );
    test_vehicle_drag( "autotractor", 1.425450, 1.426720, 1405.171250, 7840, 8128 );
    test_vehicle_drag( "tractor_plow", 0.609525, 0.920982, 1744.370192, 11941, 13821 );
    test_vehicle_drag( "tractor_reaper", 0.609525, 0.806757, 1528.024038, 11963, 13842 );
    test_vehicle_drag( "tractor_seed", 0.609525, 0.806757, 1528.024038, 11963, 13842 );
    test_vehicle_drag( "aapc-mg", 1.625400, 8.820575, 4460.025298, 8026, 9415 );
    test_vehicle_drag( "apc", 1.625400, 8.684897, 4391.421131, 8036, 9425 );
    test_vehicle_drag( "humvee", 0.616298, 7.937201, 5351.142857, 12830, 15073 );
    test_vehicle_drag( "military_cargo_truck", 0.840758, 10.717129, 4945.336111, 11407, 13438 );
    test_vehicle_drag( "flatbed_truck", 0.776903, 4.450801, 1992.535795, 10193, 12254 );
    test_vehicle_drag( "pickup", 0.589208, 2.856446, 1155.464286, 11366, 13614 );
    test_vehicle_drag( "semi_truck", 0.778423, 7.640653, 5150.617500, 12050, 14123 );
    test_vehicle_drag( "truck_trailer", 1.128750, 10.893304, 4827.950000, 0, 0 );
    test_vehicle_drag( "tatra_truck", 0.968468, 8.160908, 4361.515988, 14079, 16428 );
    test_vehicle_drag( "animalctrl", 0.396191, 2.615927, 1058.171429, 12890, 15455 );
    test_vehicle_drag( "autosweeper", 0.986850, 1.298589, 919.264062, 6965, 7224 );
    test_vehicle_drag( "excavator", 0.659728, 3.315998, 2347.376563, 13094, 15199 );
    test_vehicle_drag( "road_roller", 1.823737, 2.715453, 9021.780208, 9434, 10932 );
    test_vehicle_drag( "forklift", 0.565988, 6.504838, 3683.790625, 7182, 7507 );
    test_vehicle_drag( "trencher", 0.520838, 1.386882, 1576.078846, 8506, 8828 );
    test_vehicle_drag( "armored_car", 0.844627, 7.691982, 5185.819643, 11763, 13780 );
    test_vehicle_drag( "cube_van", 0.518580, 2.594158, 2129.147917, 11876, 14220 );
    test_vehicle_drag( "cube_van_cheap", 0.518580, 2.530700, 2077.064583, 10051, 12063 );
    test_vehicle_drag( "hippie_van", 0.375874, 2.485200, 1005.291071, 11085, 13327 );
    test_vehicle_drag( "icecream_truck", 0.681673, 2.005142, 1868.108446, 11014, 13145 );
    test_vehicle_drag( "lux_rv", 1.630929, 3.431264, 1936.754934, 8445, 9810 );
    test_vehicle_drag( "meth_lab", 0.499230, 3.014333, 2063.425931, 11923, 14301 );
    test_vehicle_drag( "rv", 0.541800, 2.749459, 1882.109176, 11685, 13995 );
    test_vehicle_drag( "schoolbus", 0.445050, 3.030839, 1710.737829, 12701, 14812 );
    test_vehicle_drag( "security_van", 0.541800, 8.372058, 6871.342708, 10936, 12945 );
    test_vehicle_drag( "wienermobile", 1.281891, 2.240295, 1838.715625, 10612, 12636 );
    test_vehicle_drag( "canoe", 0.609525, 7.741047, 2.191937, 298, 628 );
    test_vehicle_drag( "kayak", 0.609525, 2.935863, 1.108417, 710, 1314 );
    test_vehicle_drag( "kayak_racing", 0.609525, 2.604776, 0.983417, 779, 1406 );
    test_vehicle_drag( "DUKW", 0.776903, 3.604958, 77.972000, 10320, 12374 );
    test_vehicle_drag( "raft", 0.997815, 9.743243, 5.517750, 239, 508 );
    test_vehicle_drag( "inflatable_boat", 0.469560, 3.619890, 2.050000, 601, 1173 );
}
