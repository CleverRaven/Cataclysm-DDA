#include <stdio.h>
#include <stdlib.h>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "map_iterator.h"
#include "test_statistics.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_range.h"
#include "bodypart.h"
#include "calendar.h"
#include "enums.h"
#include "game_constants.h"
#include "item.h"
#include "line.h"
#include "mapdata.h"
#include "units.h"
#include "type_id.h"
#include "point.h"
#include "vpart_position.h"

using efficiency_stat = statistics<int>;

const efftype_id effect_blind( "blind" );

static void clear_game( const ter_id &terrain )
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
}

// Returns how much fuel did it provide
// But contains only fuels actually used by engines
static std::map<itype_id, int> set_vehicle_fuel( vehicle &v, const float veh_fuel_mult )
{
    // First we need to find the fuels to set
    // That is, fuels actually used by some engine
    std::set<itype_id> actually_used;
    for( const vpart_reference vp : v.get_all_parts() ) {
        vehicle_part &pt = vp.part();
        if( pt.is_engine() ) {
            actually_used.insert( pt.info().fuel_type );
            pt.enabled = true;
        } else {
            // Disable all parts that use up power or electric cars become non-deterministic
            pt.enabled = false;
        }
    }

    // We ignore battery when setting fuel because it uses designated "tanks"
    actually_used.erase( "battery" );

    // Currently only one liquid fuel supported
    REQUIRE( actually_used.size() <= 1 );
    itype_id liquid_fuel = "null";
    for( const auto &ft : actually_used ) {
        if( item::find_type( ft )->phase == LIQUID ) {
            liquid_fuel = ft;
            break;
        }
    }

    // Set fuel to a given percentage
    // Batteries are special cased because they aren't liquid fuel
    std::map<itype_id, int> ret;
    for( const vpart_reference vp : v.get_all_parts() ) {
        vehicle_part &pt = vp.part();

        if( pt.is_battery() ) {
            pt.ammo_set( "battery", pt.ammo_capacity() * veh_fuel_mult );
            ret[ "battery" ] += pt.ammo_capacity() * veh_fuel_mult;
        } else if( pt.is_tank() && liquid_fuel != "null" ) {
            float qty = pt.ammo_capacity() * veh_fuel_mult;
            qty *= std::max( item::find_type( liquid_fuel )->stack_size, 1 );
            qty /= to_milliliter( units::legacy_volume_factor );
            pt.ammo_set( liquid_fuel, qty );
            ret[ liquid_fuel ] += qty;
        } else {
            pt.ammo_unset();
        }
    }

    // We re-add battery because we want it accounted for, just not in the section above
    actually_used.insert( "battery" );
    for( auto iter = ret.begin(); iter != ret.end(); ) {
        if( iter->second <= 0 || actually_used.count( iter->first ) == 0 ) {
            iter = ret.erase( iter );
        } else {
            ++iter;
        }
    }
    return ret;
}

// Returns the lowest percentage of fuel left
// ie. 1 means no fuel was used, 0 means at least one dry tank
static float fuel_percentage_left( vehicle &v, const std::map<itype_id, int> &started_with )
{
    std::map<itype_id, int> fuel_amount;
    std::set<itype_id> consumed_fuels;
    for( const vpart_reference vp : v.get_all_parts() ) {
        vehicle_part &pt = vp.part();

        if( ( pt.is_battery() || pt.is_reactor() || pt.is_tank() ) &&
            pt.ammo_current() != "null" ) {
            fuel_amount[ pt.ammo_current() ] += pt.ammo_remaining();
        }

        if( pt.is_engine() && pt.info().fuel_type != "null" ) {
            consumed_fuels.insert( pt.info().fuel_type );
        }
    }

    float left = 1.0f;
    for( const auto &type : consumed_fuels ) {
        const auto iter = started_with.find( type );
        // Weird - we started without this fuel
        float fuel_amt_at_start = iter != started_with.end() ? iter->second : 0.0f;
        REQUIRE( fuel_amt_at_start != 0.0f );
        left = std::min( left, static_cast<float>( fuel_amount[type] ) / fuel_amt_at_start );
    }

    return left;
}

const float fuel_level = 0.1f;
const int cycle_limit = 100;

// Algorithm goes as follows:
// Clear map
// Spawn a vehicle
// Set its fuel up to some percentage - remember exact fuel counts that were set here
// Drive it for a while, always moving it back to start point every turn to avoid it going off the bubble
// When moving back, record the sum of the tiles moved so far
// Repeat that for a set number of turns or until all fuel is drained
// Compare saved percentage (set before) to current percentage
// Rescale the recorded number of tiles based on fuel percentage left
// (ie. 0% fuel left means no scaling, 50% fuel left means double the effective distance)
// Return the rescaled number
static int test_efficiency( const vproto_id &veh_id, int &expected_mass,
                            const ter_id &terrain,
                            const int reset_velocity_turn, const int target_distance,
                            const bool smooth_stops = false, const bool test_mass = true )
{
    int min_dist = target_distance * 0.99;
    int max_dist = target_distance * 1.01;
    clear_game( terrain );

    const tripoint map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = g->m.add_vehicle( veh_id, map_starting_point, -90, 0, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return 0;
    }

    vehicle &veh = *veh_ptr;

    // Remove all items from cargo to normalize weight.
    for( const vpart_reference vp : veh.get_all_parts() ) {
        veh_ptr->get_items( vp.part_index() ).clear();
        vp.part().ammo_consume( vp.part().ammo_remaining(), vp.pos() );
    }
    for( const vpart_reference vp : veh.get_avail_parts( "OPENABLE" ) ) {
        veh.close( vp.part_index() );
    }

    veh.refresh_insides();

    if( test_mass ) {
        CHECK( to_gram( veh.total_mass() ) == expected_mass );
    }
    expected_mass = to_gram( veh.total_mass() );
    veh.check_falling_or_floating();
    REQUIRE( !veh.is_in_water() );
    const auto &starting_fuel = set_vehicle_fuel( veh, fuel_level );
    // This is ugly, but improves accuracy: compare the result of fuel approx function
    // rather than the amount of fuel we actually requested
    const float starting_fuel_per = fuel_percentage_left( veh, starting_fuel );
    REQUIRE( std::abs( starting_fuel_per - 1.0f ) < 0.001f );

    const tripoint starting_point = veh.global_pos3();
    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;

    const int target_velocity = std::min( 70 * 100, veh.safe_ground_velocity( false ) );
    veh.cruise_velocity = target_velocity;
    // If we aren't testing repeated cold starts, start the vehicle at cruising velocity.
    // Otherwise changing the amount of fuel in the tank perturbs the test results.
    if( reset_velocity_turn == -1 ) {
        veh.velocity = target_velocity;
    }
    int reset_counter = 0;
    int tiles_travelled = 0;
    int cycles_left = cycle_limit;
    bool accelerating = true;
    CHECK( veh.safe_velocity() > 0 );
    while( veh.engine_on && veh.safe_velocity() > 0 && cycles_left > 0 ) {
        cycles_left--;
        g->m.vehmove();
        veh.idle( true );
        // If the vehicle starts skidding, the effects become random and test is RUINED
        REQUIRE( !veh.skidding );
        for( const tripoint &pos : veh.get_points() ) {
            REQUIRE( g->m.ter( pos ) );
        }
        // How much it moved
        tiles_travelled += square_dist( starting_point, veh.global_pos3() );
        // Bring it back to starting point to prevent it from leaving the map
        const tripoint displacement = starting_point - veh.global_pos3();
        tripoint veh_pos = veh.global_pos3();
        g->m.displace_vehicle( veh_pos, displacement );
        if( reset_velocity_turn < 0 ) {
            continue;
        }

        reset_counter++;
        if( reset_counter > reset_velocity_turn ) {
            if( smooth_stops ) {
                accelerating = !accelerating;
                veh.cruise_velocity = accelerating ? target_velocity : 0;
            } else {
                veh.velocity = 0;
                veh.last_turn = 0;
                veh.of_turn_carry = 0;
            }
            reset_counter = 0;
        }
    }

    float fuel_left = fuel_percentage_left( veh, starting_fuel );
    REQUIRE( starting_fuel_per - fuel_left > 0.0001f );
    const float fuel_percentage_used = fuel_level * ( starting_fuel_per - fuel_left );
    int adjusted_tiles_travelled = tiles_travelled / fuel_percentage_used;
    if( target_distance >= 0 ) {
        CHECK( adjusted_tiles_travelled >= min_dist );
        CHECK( adjusted_tiles_travelled <= max_dist );
    }

    return adjusted_tiles_travelled;
}

static efficiency_stat find_inner(
    const std::string &type, int &expected_mass, const std::string &terrain, const int delay,
    const bool smooth, const bool test_mass = false )
{
    efficiency_stat efficiency;
    for( int i = 0; i < 10; i++ ) {
        efficiency.add( test_efficiency( vproto_id( type ), expected_mass, ter_id( terrain ),
                                         delay, -1, smooth, test_mass ) );
    }
    return efficiency;
}

static void print_stats( const efficiency_stat &st )
{
    if( st.min() == st.max() ) {
        printf( "All results %d.\n", st.min() );
    } else {
        printf( "Min %d, Max %d, Midpoint %f.\n", st.min(), st.max(),
                ( st.min() + st.max() ) / 2.0 );
    }
}

static void print_efficiency(
    const std::string &type, int expected_mass, const std::string &terrain, const int delay,
    const bool smooth )
{
    printf( "Testing %s on %s with %s: ",
            type.c_str(), terrain.c_str(), ( delay < 0 ) ? "no resets" : "resets every 5 turns" );
    print_stats( find_inner( type, expected_mass, terrain, delay, smooth ) );
}

static void find_efficiency( const std::string &type )
{
    SECTION( "finding efficiency of " + type ) {
        print_efficiency( type, 0,  "t_pavement", -1, false );
        print_efficiency( type, 0, "t_dirt", -1, false );
        print_efficiency( type, 0, "t_pavement", 5, false );
        print_efficiency( type, 0, "t_dirt", 5, false );
    }
}

static int average_from_stat( const efficiency_stat &st )
{
    const int ugly_integer = ( st.min() + st.max() ) / 2.0;
    // Round to 4 most significant places
    const int magnitude = std::max<int>( 0, std::floor( std::log10( ugly_integer ) ) );
    const int precision = std::max<int>( 1, std::round( std::pow( 10.0, magnitude - 3 ) ) );
    return ugly_integer - ugly_integer % precision;
}

// Behold: power of laziness
static void print_test_strings( const std::string &type )
{
    std::ostringstream ss;
    int expected_mass = 0;
    ss << "    test_vehicle( \"" << type << "\", ";
    const int d_pave = average_from_stat( find_inner( type, expected_mass, "t_pavement", -1,
                                          false, false ) );
    ss << expected_mass << ", " << d_pave << ", ";
    ss << average_from_stat( find_inner( type, expected_mass, "t_dirt", -1,
                                         false, false ) ) << ", ";
    ss << average_from_stat( find_inner( type, expected_mass, "t_pavement", 5,
                                         false, false ) ) << ", ";
    ss << average_from_stat( find_inner( type, expected_mass, "t_dirt", 5, false, false ) );
    //ss << average_from_stat( find_inner( type, "t_pavement", 5, true ) ) << ", ";
    //ss << average_from_stat( find_inner( type, "t_dirt", 5, true ) );
    ss << " );" << std::endl;
    printf( "%s", ss.str().c_str() );
    fflush( stdout );
}

static void test_vehicle(
    std::string type, int expected_mass,
    const int pavement_target, const int dirt_target,
    const int pavement_target_w_stops, const int dirt_target_w_stops,
    const int pavement_target_smooth_stops = 0, const int dirt_target_smooth_stops = 0 )
{
    SECTION( type + " on pavement" ) {
        test_efficiency( vproto_id( type ), expected_mass, ter_id( "t_pavement" ), -1,
                         pavement_target );
    }
    SECTION( type + " on dirt" ) {
        test_efficiency( vproto_id( type ), expected_mass, ter_id( "t_dirt" ), -1, dirt_target );
    }
    SECTION( type + " on pavement, full stop every 5 turns" ) {
        test_efficiency( vproto_id( type ), expected_mass, ter_id( "t_pavement" ), 5,
                         pavement_target_w_stops );
    }
    SECTION( type + " on dirt, full stop every 5 turns" ) {
        test_efficiency( vproto_id( type ), expected_mass, ter_id( "t_dirt" ), 5,
                         dirt_target_w_stops );
    }
    if( pavement_target_smooth_stops > 0 ) {
        SECTION( type + " on pavement, alternating 5 turns of acceleration and 5 turns of decceleration" ) {
            test_efficiency( vproto_id( type ), expected_mass, ter_id( "t_pavement" ), 5,
                             pavement_target_smooth_stops, true );
        }
    }
    if( dirt_target_smooth_stops > 0 ) {
        SECTION( type + " on dirt, alternating 5 turns of acceleration and 5 turns of decceleration" ) {
            test_efficiency( vproto_id( type ), expected_mass, ter_id( "t_dirt" ), 5,
                             dirt_target_smooth_stops, true );
        }
    }
}

std::vector<std::string> vehs_to_test = {{
        "beetle",
        "car",
        "car_sports",
        "electric_car",
        "suv",
        "motorcycle",
        "quad_bike",
        "scooter",
        "superbike",
        "ambulance",
        "fire_engine",
        "fire_truck",
        "truck_swat",
        "tractor_plow",
        "apc",
        "humvee",
        "road_roller",
        "golf_cart"
    }
};

/** This isn't a test per se, it executes this code to
 * determine the current state of vehicle efficiency.
 **/
TEST_CASE( "vehicle_find_efficiency", "[.]" )
{
    for( const std::string &veh : vehs_to_test ) {
        find_efficiency( veh );
    }
}

/** This is even less of a test. It generates C++ lines for the actual test below */
TEST_CASE( "vehicle_make_efficiency_case", "[.]" )
{
    for( const std::string &veh : vehs_to_test ) {
        print_test_strings( veh );
    }
}

// TODO:
// Amount of fuel needed to reach safe speed.
// Amount of cruising range for a fixed amount of fuel.
// Fix test for electric vehicles
TEST_CASE( "vehicle_efficiency", "[vehicle] [engine]" )
{
    test_vehicle( "beetle", 787373, 292300, 228600, 75730, 56370 );
    test_vehicle( "car", 1092322, 520800, 302500, 47060, 26470 );
    test_vehicle( "car_sports", 1125918, 456200, 312000, 38140, 22390 );
    test_vehicle( "electric_car", 1097791, 215469, 120379, 17211, 8898 );
    test_vehicle( "suv", 1291990, 987100, 483000, 69560, 33260 );
    test_vehicle( "motorcycle", 163085, 77300, 63120, 47530, 39030 );
    test_vehicle( "quad_bike", 265345, 75950, 75950, 35190, 35190 );
    test_vehicle( "scooter", 62587, 266900, 258300, 195200, 184400 );
    test_vehicle( "superbike", 242085, 72120, 46820, 34580, 21610 );
    test_vehicle( "ambulance", 1803929, 432000, 345500, 64010, 47220 );
    test_vehicle( "fire_engine", 2593241, 1169000, 960700, 244700, 215300 );
    test_vehicle( "fire_truck", 6279233, 308500, 199100, 19950, 4747 );
    test_vehicle( "truck_swat", 5959334, 505800, 348000, 30060, 7719 );
    test_vehicle( "tractor_plow", 723658, 528000, 528000, 117000, 117000 );
    test_vehicle( "apc", 5801619, 1100000, 948300, 132800, 86880 );
    test_vehicle( "humvee", 5499145, 607600, 341100, 25880, 9263 );
    test_vehicle( "road_roller", 8779702, 369800, 412000, 22990, 6996 );
    test_vehicle( "golf_cart", 444630, 52800, 105591, 27250, 14207 );
}
