#include <cstdio>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "map.h"
#include "map_helpers.h"
#include "point.h"
#include "test_data.h"
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
    map &here = get_map();
    // Set to turn 0 to prevent solars from producing power
    calendar::turn = calendar::turn_zero;
    clear_creatures();
    clear_npcs();

    Character &player_character = get_player_character();
    // Move player somewhere safe
    CHECK( !player_character.in_vehicle );
    player_character.setpos( here, tripoint_bub_ms::zero );
    // Blind the player to avoid needless drawing-related overhead
    player_character.add_effect( effect_blind, 1_turns, true );
    // Make sure the ST is 8 so that muscle powered results are consistent
    player_character.str_cur = 8;

    build_test_map( terrain );

    // hard force a rebuild of caches
    here.shift( point_rel_sm::south );
    here.shift( point_rel_sm::north );
}

static vehicle *setup_drag_test( const vproto_id &veh_id )
{
    map &here = get_map();
    clear_vehicles();
    const tripoint_bub_ms map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = here.add_vehicle( veh_id, map_starting_point, -90_degrees, 0, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return nullptr;
    }

    // Remove all items from cargo to normalize weight.
    // turn everything on
    for( const vpart_reference &vp : veh_ptr->get_all_parts() ) {
        veh_ptr->get_items( vp.part() ).clear();
        vp.part().enabled = true;
    }
    // close the doors
    const auto doors = veh_ptr->get_avail_parts( "OPENABLE" );
    for( const vpart_reference &vp :  doors ) {
        const size_t door = vp.part_index();
        veh_ptr->close( here, door );
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
    map &here = get_map();

    vehicle *veh_ptr = setup_drag_test( veh_id );
    if( veh_ptr == nullptr ) {
        return false;
    }

    const double c_air = veh_ptr->coeff_air_drag();
    const double c_rolling = veh_ptr->coeff_rolling_drag( here );
    const double c_water = veh_ptr->coeff_water_drag( here );
    const int safe_v = veh_ptr->safe_ground_velocity( here, false );
    const int max_v = veh_ptr->max_ground_velocity( here, false );

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
        printf( "    { \"%s\": [ %f, %f, %f, %d, %d ] },\n",
                veh_id.c_str(), c_air, c_rolling, c_water, safe_v, max_v );
        static_cast<void>( fflush( stdout ) );
    }
    return valid;
}

static void print_drag_test_strings( const vproto_id &veh )
{
    test_drag( veh );
    static_cast<void>( fflush( stdout ) );
}

static void test_vehicle_drag( std::pair<const vproto_id, std::vector<double>> &veh_drag )
{
    test_drag( veh_drag.first, veh_drag.second[0], veh_drag.second[1], veh_drag.second[2],
               veh_drag.second[3], veh_drag.second[4], true );
}

/** This is even less of a test. It generates C++ lines for the actual test below */
TEST_CASE( "vehicle_drag_calc_baseline", "[.]" )
{
    clear_game_drag( ter_id( "t_pavement" ) );

    for( std::pair<const vproto_id, std::vector<double>> &veh_drag : test_data::drag_data ) {
        print_drag_test_strings( veh_drag.first );
    }
}

// format is vehicle, coeff_air_drag, coeff_rolling_drag, coeff_water_drag, safe speed, max speed
// coeffs are dimensionless, speeds are 100ths of mph, so 6101 is 61.01 mph
TEST_CASE( "vehicle_drag", "[vehicle] [engine]" )
{
    clear_game_drag( ter_id( "t_pavement" ) );

    REQUIRE_FALSE( test_data::drag_data.empty() );

    INFO( "Vehicle drag values changed.  Update values in data/mods/TEST_DATA/vehicle_drag_test.json with values printed below." );

    for( std::pair<const vproto_id, std::vector<double>> &veh_drag : test_data::drag_data ) {
        test_vehicle_drag( veh_drag );
    }
}
