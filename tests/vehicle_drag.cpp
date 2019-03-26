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
    test_vehicle_drag( "scooter_electric", 0.609525, 0.178166, 134.531250, 4922, 5102 );
    test_vehicle_drag( "superbike", 0.609525, 0.844993, 377.789062, 9867, 11765 );
    test_vehicle_drag( "tandem", 0.609525, 0.021344, 40.290625, 2353, 3076 );
    test_vehicle_drag( "unicycle", 0.690795, 0.002400, 24.162500, 2266, 2958 );
    test_vehicle_drag( "beetle", 0.785610, 1.693784, 1199.020312, 8902, 10668 );
    test_vehicle_drag( "bubble_car", 0.823988, 1.661737, 1120.318452, 9435, 9786 );
    test_vehicle_drag( "car", 0.294604, 2.366883, 1117.002083, 11837, 14304 );
    test_vehicle_drag( "car_mini", 0.294604, 1.709414, 1210.084375, 12080, 14536 );
    test_vehicle_drag( "car_sports", 0.294604, 2.424461, 1373.010000, 20847, 24914 );
    test_vehicle_drag( "car_sports_atomic", 0.294604, 3.442279, 1624.513542, 24698, 25626 );
    test_vehicle_drag( "car_sports_electric", 0.294604, 2.751150, 1558.018750, 24910, 25837 );
    test_vehicle_drag( "electric_car", 0.304762, 2.142779, 1011.240625, 16290, 16906 );
    test_vehicle_drag( "rara_x", 0.880425, 1.995736, 1039.279095, 8499, 8819 );
    test_vehicle_drag( "suv", 0.294604, 2.807600, 1135.705357, 13939, 16801 );
    test_vehicle_drag( "suv_electric", 0.304762, 2.324519, 940.293750, 16230, 16846 );
    test_vehicle_drag( "golf_cart", 0.943313, 1.311868, 825.479167, 7152, 7418 );
    test_vehicle_drag( "golf_cart_4seat", 0.943313, 1.279229, 804.941667, 7157, 7423 );
    test_vehicle_drag( "hearse", 0.355556, 3.016494, 1220.205357, 10995, 13320 );
    test_vehicle_drag( "pickup_technical", 0.838097, 2.820320, 1140.850893, 10136, 12149 );
    test_vehicle_drag( "ambulance", 1.049323, 2.264056, 1858.217708, 11256, 13438 );
    test_vehicle_drag( "car_fbi", 0.457144, 2.635064, 1243.564583, 14570, 17458 );
    test_vehicle_drag( "fire_engine", 2.305875, 3.281582, 2154.679464, 8666, 10345 );
    test_vehicle_drag( "fire_truck", 1.092446, 7.944023, 5266.181611, 10508, 12693 );
    test_vehicle_drag( "policecar", 0.629843, 2.636167, 1244.085417, 13182, 15775 );
    test_vehicle_drag( "policesuv", 0.629843, 2.967405, 1200.348214, 13124, 15720 );
    test_vehicle_drag( "truck_swat", 0.808830, 7.538017, 6186.806250, 9850, 11625 );
    test_vehicle_drag( "oldtractor", 0.537285, 0.868099, 1282.481250, 12353, 14339 );
    test_vehicle_drag( "autotractor", 1.425450, 1.738959, 1712.695000, 7320, 7591 );
    test_vehicle_drag( "tractor_plow", 0.609525, 0.893061, 1691.485577, 11852, 13756 );
    test_vehicle_drag( "tractor_reaper", 0.609525, 0.778836, 1475.139423, 11874, 13777 );
    test_vehicle_drag( "tractor_seed", 0.609525, 0.778836, 1475.139423, 11874, 13777 );
    test_vehicle_drag( "aapc-mg", 1.625400, 8.606709, 4351.886161, 7972, 9379 );
    test_vehicle_drag( "apc", 1.625400, 8.483520, 4289.597470, 7982, 9388 );
    test_vehicle_drag( "humvee", 0.616298, 7.232786, 4876.236607, 12875, 15132 );
    test_vehicle_drag( "military_cargo_truck", 0.840758, 9.416864, 4345.338889, 11502, 13545 );
    test_vehicle_drag( "flatbed_truck", 0.883328, 4.458495, 1995.980114, 9717, 11717 );
    test_vehicle_drag( "pickup", 0.589208, 3.088771, 1249.441964, 11219, 13499 );
    test_vehicle_drag( "semi_truck", 0.781317, 9.950414, 5749.409167, 11670, 13766 );
    test_vehicle_drag( "truck_trailer", 1.176534, 12.833835, 5688.000000, 0, 0 );
    test_vehicle_drag( "tatra_truck", 0.440213, 7.711005, 4121.069767, 17745, 20815 );
    test_vehicle_drag( "animalctrl", 0.345398, 2.717176, 1099.127679, 13283, 16007 );
    test_vehicle_drag( "autosweeper", 0.986850, 1.584382, 1121.575000, 7009, 7272 );
    test_vehicle_drag( "excavator", 0.659728, 1.749378, 1238.375000, 13141, 15260 );
    test_vehicle_drag( "road_roller", 1.823738, 2.745174, 9120.522917, 9382, 10893 );
    test_vehicle_drag( "forklift", 0.565988, 1.350440, 637.312500, 8400, 8717 );
    test_vehicle_drag( "trencher", 0.659728, 1.080300, 1227.673558, 8060, 8360 );
    test_vehicle_drag( "armored_car", 0.896872, 6.951678, 4686.717857, 11588, 13577 );
    test_vehicle_drag( "cube_van", 0.518580, 2.636314, 2163.746875, 11796, 14161 );
    test_vehicle_drag( "cube_van_cheap", 0.512775, 2.572474, 1853.869207, 9978, 12027 );
    test_vehicle_drag( "hippie_van", 0.386033, 2.780080, 1124.573214, 10804, 13062 );
    test_vehicle_drag( "icecream_truck", 0.681673, 3.708542, 1986.680405, 10624, 12800 );
    test_vehicle_drag( "lux_rv", 1.609183, 3.503599, 1977.584430, 8406, 9795 );
    test_vehicle_drag( "meth_lab", 0.518580, 2.957331, 2024.405585, 11692, 14070 );
    test_vehicle_drag( "rv", 0.541800, 2.932748, 2007.577660, 11542, 13885 );
    test_vehicle_drag( "schoolbus", 0.411188, 3.060324, 1370.046591, 12891, 15087 );
    test_vehicle_drag( "security_van", 0.541800, 7.592192, 6231.269792, 10977, 13009 );
    test_vehicle_drag( "wienermobile", 1.063697, 2.315334, 1900.304167, 11201, 13374 );
}
