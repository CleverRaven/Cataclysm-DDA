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

/*Build a map with a single square of terrain, and place the player character on it.
Args: terrain: The terrain type to use.*/
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

/*The test setup is a vehicle with a cargo area and a fuel tank.
The vehicle is placed on the map, facing north, at the center of the map.
The vehicle is then filled with fuel and all cargo is removed.
The vehicle is then turned on, and all cargo is removed.
The vehicle is then closed, and all cargo is removed.*/
static vehicle *setup_drag_test( const vproto_id &veh_id )
{
    // Clearing all vehicles from the map.    
    clear_vehicles();
  
    const tripoint map_starting_point( 60, 60, 0 );
   
    vehicle *veh_ptr = get_map().add_vehicle( veh_id, map_starting_point, -90_degrees, 0, 0 );

    // Ensure we have placed the vehicle   
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
 Clear map
 Spawn a vehicle
 Calculate c_air_drag and c_rolling_resistance
 Return whether they're within 5% of expected values
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
    test_vehicle_drag( "motorcycle", 0.609525, 0.568204, 254.039062, 7297, 8687 );
    test_vehicle_drag( "motorcycle_sidecart", 0.880425, 0.857099, 454.164583, 6424, 7657 );
    test_vehicle_drag( "quad_bike", 0.537285, 1.110700, 709.406250, 7457, 8919 );
    test_vehicle_drag( "scooter", 0.609525, 0.152965, 115.502083, 4279, 5088 );
    test_vehicle_drag( "scooter_electric", 0.609525, 0.108235, 81.727083, 4831, 5007 );
    test_vehicle_drag( "superbike", 0.609525, 0.844294, 377.476562, 9912, 11797 );
    test_vehicle_drag( "tandem", 0.609525, 0.010590, 19.990625, 1430, 1870 );
    test_vehicle_drag( "unicycle", 0.690795, 0.002493, 25.100000, 1377, 1798 );
    test_vehicle_drag( "beetle", 0.785610, 1.584313, 1121.526562, 9005, 10744 );
    test_vehicle_drag( "bubble_car", 0.823988, 1.002279, 675.721726, 9424, 9769 );
    test_vehicle_drag( "car", 0.294604, 2.255646, 1064.506250, 11995, 14426 );
    test_vehicle_drag( "car_mini", 0.294604, 1.599943, 1132.590625, 12238, 14657 );
    test_vehicle_drag( "car_sports", 0.294604, 2.331567, 1320.402500, 20918, 24972 );
    test_vehicle_drag( "car_sports_atomic", 0.294604, 3.185611, 1503.384375, 23795, 24693 );
    test_vehicle_drag( "car_sports_electric", 0.294604, 1.798178, 1018.336250, 23968, 24864 );
    test_vehicle_drag( "electric_car", 0.304762, 1.541638, 727.544791, 16337, 16954 );
    test_vehicle_drag( "rara_x", 0.679185, 1.285418, 718.965277, 9191, 9535 );
    test_vehicle_drag( "suv", 0.294604, 2.696363, 1090.708929, 14063, 16899 );
    test_vehicle_drag( "suv_electric", 0.304762, 1.692274, 684.543750, 16288, 16904 );
    test_vehicle_drag( "golf_cart", 0.943313, 1.058255, 665.895833, 7039, 7303 );
    test_vehicle_drag( "golf_cart_4seat", 0.943313, 1.025616, 645.358333, 7044, 7308 );
    test_vehicle_drag( "hearse", 0.355556, 2.672185, 1080.928571, 11212, 13499 );
    test_vehicle_drag( "pickup_technical", 0.838097, 2.638015, 1067.106250, 10222, 12217 );
    test_vehicle_drag( "ambulance", 1.049323, 2.191681, 1798.815625, 11324, 13488 );
    test_vehicle_drag( "car_fbi", 0.457144, 2.525593, 1191.902083, 14675, 17538 );
    test_vehicle_drag( "fire_engine", 2.305875, 2.866842, 1882.362500, 8706, 10373 );
    test_vehicle_drag( "fire_truck", 1.056510, 7.862720, 5212.284976, 10681, 12872 );
    test_vehicle_drag( "policecar", 0.629843, 2.526697, 1192.422917, 13273, 15844 );
    test_vehicle_drag( "policesuv", 0.629843, 2.878958, 1164.570536, 13211, 15785 );
    test_vehicle_drag( "truck_swat", 0.808830, 8.226852, 6752.165625, 9846, 11602 );
    test_vehicle_drag( "oldtractor", 0.537285, 0.893482, 1319.981250, 12446, 14408 );
    test_vehicle_drag( "autotractor", 1.425450, 1.426719, 1405.171250, 7778, 8066 );
    test_vehicle_drag( "tractor_plow", 0.609525, 0.918444, 1739.562500, 11941, 13822 );
    test_vehicle_drag( "tractor_reaper", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "tractor_seed", 0.609525, 0.804219, 1523.216346, 11963, 13843 );
    test_vehicle_drag( "aapc-mg", 1.625400, 8.808073, 4453.703869, 8027, 9416 );
    test_vehicle_drag( "apc", 1.625400, 8.681954, 4389.933036, 8036, 9425 );
    test_vehicle_drag( "humvee", 0.616298, 7.936433, 5350.625000, 12830, 15073 );
    test_vehicle_drag( "military_cargo_truck", 0.840758, 10.714119, 4943.947222, 11407, 13439 );
    test_vehicle_drag( "flatbed_truck", 0.776903, 4.448263, 1991.399432, 10194, 12254 );
    test_vehicle_drag( "pickup", 0.589208, 2.852032, 1153.678571, 11367, 13615 );
    test_vehicle_drag( "semi_truck", 0.778423, 7.637768, 5148.673056, 12051, 14123 );
    test_vehicle_drag( "truck_trailer", 1.128750, 10.893304, 4827.950000, 0, 0 );
    test_vehicle_drag( "tatra_truck", 0.968468, 8.157085, 4359.472384, 14080, 16428 );
    test_vehicle_drag( "animalctrl", 0.396191, 2.611512, 1056.385714, 12891, 15457 );
    test_vehicle_drag( "autosweeper", 0.986850, 1.298589, 919.264062, 6884, 7144 );
    test_vehicle_drag( "excavator", 0.659728, 2.439720, 1727.064062, 13095, 15200 );
    test_vehicle_drag( "road_roller", 1.823738, 2.714826, 9019.696875, 9434, 10932 );
    test_vehicle_drag( "forklift", 0.565988, 1.096826, 517.625000, 8258, 8572 );
    test_vehicle_drag( "trencher", 0.520838, 1.386881, 1576.078846, 8466, 8788 );
    test_vehicle_drag( "armored_car", 0.844628, 7.687341, 5182.691071, 11764, 13780 );
    test_vehicle_drag( "cube_van", 0.518580, 2.591620, 2127.064583, 11877, 14220 );
    test_vehicle_drag( "cube_van_cheap", 0.518580, 2.528161, 2074.981250, 10052, 12064 );
    test_vehicle_drag( "hippie_van", 0.375874, 2.480786, 1003.505357, 11086, 13328 );
    test_vehicle_drag( "icecream_truck", 0.681673, 2.005141, 1868.108445, 10986, 13120 );
    test_vehicle_drag( "lux_rv", 1.630929, 3.431263, 1936.754934, 8426, 9792 );
    test_vehicle_drag( "meth_lab", 0.499230, 3.014333, 2063.425930, 11811, 14195 );
    test_vehicle_drag( "rv", 0.541800, 2.749458, 1882.109175, 11645, 13958 );
    test_vehicle_drag( "schoolbus", 0.445050, 3.028119, 1709.202741, 12702, 14813 );
    test_vehicle_drag( "security_van", 0.541800, 8.623353, 7077.592708, 10891, 12901 );
    test_vehicle_drag( "wienermobile", 1.281891, 2.237757, 1836.632292, 10612, 12636 );
    test_vehicle_drag( "canoe", 0.609525, 7.741047, 2.191938, 298, 628 );
    test_vehicle_drag( "kayak", 0.609525, 2.935863, 1.108417, 710, 1314 );
    test_vehicle_drag( "kayak_racing", 0.609525, 2.604776, 0.983417, 779, 1406 );
    test_vehicle_drag( "DUKW", 0.776903, 3.602238, 77.913176, 10321, 12374 );
    test_vehicle_drag( "raft", 0.997815, 9.743243, 5.517750, 239, 508 );
    test_vehicle_drag( "inflatable_boat", 0.469560, 3.616690, 2.048188, 602, 1173 );
}
