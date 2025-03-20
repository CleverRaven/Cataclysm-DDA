#include <algorithm>
#include <cmath>
#include <cstdio>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "calendar.h"
#include "cata_catch.h"
#include "character.h"
#include "coordinates.h"
#include "enums.h"
#include "item.h"
#include "itype.h"
#include "map.h"
#include "map_helpers.h"
#include "point.h"
#include "test_data.h"
#include "test_statistics.h"
#include "type_id.h"
#include "units.h"
#include "value_ptr.h"
#include "veh_type.h"
#include "vehicle.h"
#include "vpart_position.h"
#include "vpart_range.h"

using efficiency_stat = statistics<int>;

static const ammotype ammo_battery( "battery" );

static const efftype_id effect_blind( "blind" );

static const itype_id itype_battery( "battery" );

static void clear_game( const ter_id &terrain )
{
    map &here = get_map();
    // Set to turn 0 to prevent solars from producing power
    calendar::turn = calendar::turn_zero;
    clear_creatures();
    clear_npcs();
    clear_vehicles();

    Character &player_character = get_player_character();
    // Move player somewhere safe
    REQUIRE_FALSE( player_character.in_vehicle );
    player_character.setpos( here, tripoint_bub_ms::zero );
    // Blind the player to avoid needless drawing-related overhead
    player_character.add_effect( effect_blind, 1_turns, true );

    build_test_map( terrain );
}

// Returns how much fuel did it provide
// But contains only fuels actually used by engines
static std::map<itype_id, int> set_vehicle_fuel( vehicle &v, const float veh_fuel_mult )
{
    // First we need to find the fuels to set
    // That is, fuels actually used by some engine
    std::set<itype_id> actually_used;
    for( const vpart_reference &vp : v.get_all_parts() ) {
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
    actually_used.erase( itype_battery );

    // Currently only one liquid fuel supported
    REQUIRE( actually_used.size() <= 1 );
    itype_id liquid_fuel = itype_id::NULL_ID();
    for( const auto &ft : actually_used ) {
        if( item::find_type( ft )->phase == phase_id::LIQUID ) {
            liquid_fuel = ft;
            break;
        }
    }

    // Set fuel to a given percentage
    // Batteries are special cased because they aren't liquid fuel
    std::map<itype_id, int> ret;
    for( const vpart_reference &vp : v.get_all_parts() ) {
        vehicle_part &pt = vp.part();

        if( pt.is_battery() ) {
            pt.ammo_set( itype_battery, pt.ammo_capacity( ammo_battery ) * veh_fuel_mult );
            ret[itype_battery] += pt.ammo_capacity( ammo_battery ) * veh_fuel_mult;
        } else if( pt.is_tank() && !liquid_fuel.is_null() ) {
            float qty = pt.ammo_capacity( item::find_type( liquid_fuel )->ammo->type ) * veh_fuel_mult;
            qty *= std::max( item::find_type( liquid_fuel )->stack_size, 1 );
            qty /= to_milliliter( 250_ml );
            pt.ammo_set( liquid_fuel, qty );
            ret[ liquid_fuel ] += qty;
        } else {
            pt.ammo_unset();
        }
    }

    // We re-add battery because we want it accounted for, just not in the section above
    actually_used.insert( itype_battery );
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
// i.e. 1 means no fuel was used, 0 means at least one dry tank
static float fuel_percentage_left( vehicle &v, const std::map<itype_id, int> &started_with )
{
    std::map<itype_id, int> fuel_amount;
    std::set<itype_id> consumed_fuels;
    for( const vpart_reference &vp : v.get_all_parts() ) {
        vehicle_part &pt = vp.part();

        if( ( pt.is_battery() || pt.is_reactor() || pt.is_tank() ) &&
            !pt.ammo_current().is_null() ) {
            fuel_amount[ pt.ammo_current() ] += pt.ammo_remaining( );
        }

        if( pt.is_engine() && !pt.info().fuel_type.is_null() ) {
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

static const float fuel_level = 0.1f;
static const int cycle_limit = 100;

// Algorithm goes as follows:
// Clear map
// Spawn a vehicle
// Set its fuel up to some percentage - remember exact fuel counts that were set here
// Drive it for a while, always moving it back to start point every turn to avoid it going off the bubble
// When moving back, record the sum of the tiles moved so far
// Repeat that for a set number of turns or until all fuel is drained
// Compare saved percentage (set before) to current percentage
// Rescale the recorded number of tiles based on fuel percentage left
// (i.e. 0% fuel left means no scaling, 50% fuel left means double the effective distance)
// Return the rescaled number
static int test_efficiency( const vproto_id &veh_id, int &expected_mass,
                            const ter_id &terrain,
                            const int reset_velocity_turn, const int target_distance,
                            const bool smooth_stops = false, const bool test_mass = true,
                            const bool in_reverse = false )
{
    int min_dist = target_distance * 0.99;
    int max_dist = target_distance * 1.01;
    clear_game( terrain );

    const tripoint_bub_ms map_starting_point( 60, 60, 0 );
    map &here = get_map();
    vehicle *veh_ptr = here.add_vehicle( veh_id, map_starting_point, -90_degrees, 0, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return 0;
    }

    vehicle &veh = *veh_ptr;

    // Remove all items from cargo to normalize weight.
    for( const vpart_reference &vp : veh.get_all_parts() ) {
        veh_ptr->get_items( vp.part() ).clear();
        vp.part().ammo_consume( vp.part().ammo_remaining( ), &here, vp.pos_bub( here ) );
    }
    for( const vpart_reference &vp : veh.get_avail_parts( "OPENABLE" ) ) {
        veh.close( here, vp.part_index() );
    }

    veh.refresh_insides();

    if( test_mass ) {
        CHECK( to_gram( veh.total_mass( here ) ) == expected_mass );
    }
    expected_mass = to_gram( veh.total_mass( here ) );
    veh.check_falling_or_floating();
    REQUIRE( !veh.is_in_water() );
    const auto &starting_fuel = set_vehicle_fuel( veh, fuel_level );
    // This is ugly, but improves accuracy: compare the result of fuel approx function
    // rather than the amount of fuel we actually requested
    const float starting_fuel_per = fuel_percentage_left( veh, starting_fuel );
    REQUIRE( std::abs( starting_fuel_per - 1.0f ) < 0.001f );

    const tripoint_bub_ms starting_point = veh.pos_bub( here );
    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;

    const int sign = in_reverse ? -1 : 1;
    const int target_velocity = sign * std::min( 50 * 100, veh.safe_ground_velocity( here, false ) );
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
    CHECK( veh.safe_velocity( here ) > 0 );
    while( veh.engine_on && veh.safe_velocity( here ) > 0 && cycles_left > 0 ) {
        cycles_left--;
        here.vehmove();
        veh.idle( here, true );
        // If the vehicle starts skidding, the effects become random and test is RUINED
        REQUIRE( !veh.skidding );
        for( const tripoint_abs_ms &pos : veh.get_points() ) {
            REQUIRE( here.ter( here.get_bub( pos ) ) );
        }
        // How much it moved
        tiles_travelled += square_dist( starting_point, veh.pos_bub( here ) );
        // Bring it back to starting point to prevent it from leaving the map
        const tripoint_rel_ms displacement = starting_point - veh.pos_bub( here );
        here.displace_vehicle( veh, displacement );
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
                veh.last_turn = 0_degrees;
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
        INFO( "Target distance: " << target_distance )
        INFO( "Travelled distance: " << adjusted_tiles_travelled )
        CHECK( adjusted_tiles_travelled >= min_dist );
        CHECK( adjusted_tiles_travelled <= max_dist );
    }

    return adjusted_tiles_travelled;
}

static efficiency_stat find_inner(
    const vproto_id &type, int &expected_mass, const std::string &terrain, const int delay,
    const bool smooth, const bool test_mass = false, const bool in_reverse = false )
{
    efficiency_stat efficiency;
    for( int i = 0; i < 10; i++ ) {
        efficiency.add( test_efficiency( type, expected_mass, ter_id( terrain ),
                                         delay, -1, smooth, test_mass, in_reverse ) );
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
    const vproto_id &type, int expected_mass, const std::string &terrain, const int delay,
    const bool smooth )
{
    printf( "Testing %s on %s with %s: ",
            type.c_str(), terrain.c_str(), ( delay < 0 ) ? "no resets" : "resets every 5 turns" );
    print_stats( find_inner( type, expected_mass, terrain, delay, smooth ) );
}

static void find_efficiency( const vproto_id &type )
{
    SECTION( "finding efficiency of " + type.str() ) {
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
static void print_test_strings( const vproto_id &type )
{
    // should look like this
    // "beetle": { "forward": [ 707777, 399680, 358994, 110952, 91402 ], "reverse": [ 707777, 58800, 58800, 45900, 44560 ] },

    const float acceptable = 1.25;

    std::ostringstream ss;
    int expected_mass = 0;
    ss << R"(      ")" << type << R"(": { "forward": [ )";
    const int d_forward = average_from_stat( find_inner( type, expected_mass, "t_pavement", -1,
                          false ) );
    ss << expected_mass << ", " << d_forward << ", ";
    ss << average_from_stat( find_inner( type, expected_mass, "t_dirt", -1, false ) ) << ", ";
    ss << average_from_stat( find_inner( type, expected_mass, "t_pavement", 5, false ) ) << ", ";
    ss << average_from_stat( find_inner( type, expected_mass, "t_dirt", 5, false ) );
    //ss << average_from_stat( find_inner( type, "t_pavement", 5, true ) ) << ", ";
    //ss << average_from_stat( find_inner( type, "t_dirt", 5, true ) );
    ss << R"( ], "reverse": [ )";
    ss << expected_mass << ", ";
    const int d_reverse = average_from_stat( find_inner( type, expected_mass, "t_pavement", -1, false,
                          false,
                          true ) );
    CHECK( d_reverse < ( acceptable * d_forward ) );
    ss << d_reverse << ", ";
    ss << average_from_stat( find_inner( type, expected_mass, "t_dirt", -1, false, false,
                                         true ) ) << ", ";
    ss << average_from_stat( find_inner( type, expected_mass, "t_pavement", 5, false, false,
                                         true ) ) << ", ";
    ss << average_from_stat( find_inner( type, expected_mass, "t_dirt", 5, false, false, true ) );
    ss << " ] }," << std::endl;
    printf( "%s", ss.str().c_str() );
    static_cast<void>( fflush( stdout ) );
}

static void test_vehicle(
    const vproto_id &type, int expected_mass,
    const int pavement_target, const int dirt_target,
    const int pavement_target_w_stops, const int dirt_target_w_stops,
    const int pavement_target_smooth_stops = 0, const int dirt_target_smooth_stops = 0,
    const bool in_reverse = false )
{
    std::string name = type.str() + ( in_reverse ? " reverse" : " " );

    SECTION( name + " on pavement" ) {
        test_efficiency( type, expected_mass, ter_id( "t_pavement" ), -1,
                         pavement_target, false, true, in_reverse );
    }
    SECTION( name + " on dirt" ) {
        test_efficiency( type, expected_mass, ter_id( "t_dirt" ), -1,
                         dirt_target, false, true, in_reverse );
    }
    SECTION( name + " on pavement, full stop every 5 turns" ) {
        test_efficiency( type, expected_mass, ter_id( "t_pavement" ), 5,
                         pavement_target_w_stops, false, true, in_reverse );
    }
    SECTION( name + " on dirt, full stop every 5 turns" ) {
        test_efficiency( type, expected_mass, ter_id( "t_dirt" ), 5,
                         dirt_target_w_stops, false, true, in_reverse );
    }
    if( pavement_target_smooth_stops > 0 ) {
        SECTION( name +
                 " on pavement, alternating 5 turns of acceleration and 5 turns of decceleration" ) {
            test_efficiency( type, expected_mass, ter_id( "t_pavement" ), 5,
                             pavement_target_smooth_stops, true, true, in_reverse );
        }
    }
    if( dirt_target_smooth_stops > 0 ) {
        SECTION( name +
                 " on dirt, alternating 5 turns of acceleration and 5 turns of decceleration" ) {
            test_efficiency( type, expected_mass, ter_id( "t_dirt" ), 5,
                             dirt_target_smooth_stops, true, true, in_reverse );
        }
    }
}

/** This isn't a test per se, it executes this code to
 * determine the current state of vehicle efficiency.
 **/
TEST_CASE( "vehicle_find_efficiency", "[.]" )
{
    for( const auto &veh : test_data::eff_data ) {
        find_efficiency( veh.first );
    }
}

/** This is even less of a test. It generates C++ lines for the actual test below */
TEST_CASE( "make_vehicle_efficiency_case", "[.]" )
{
    for( const auto &veh : test_data::eff_data ) {
        print_test_strings( veh.first );
    }
}

// TODO:
// Amount of fuel needed to reach safe speed.
// Amount of cruising range for a fixed amount of fuel.
// Fix test for electric vehicles
TEST_CASE( "vehicle_efficiency", "[vehicle] [engine]" )
{
    REQUIRE_FALSE( test_data::eff_data.empty() );

    for( auto &vehicle : test_data::eff_data ) {
        test_vehicle( vehicle.first, vehicle.second.forward[0], vehicle.second.forward[1],
                      vehicle.second.forward[2], vehicle.second.forward[3], vehicle.second.forward[4] );
        test_vehicle( vehicle.first, vehicle.second.reverse[0], vehicle.second.reverse[1],
                      vehicle.second.reverse[2], vehicle.second.reverse[3], vehicle.second.reverse[4], 0, 0, true );
    }
}
