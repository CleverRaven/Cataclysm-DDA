#include "catch/catch.hpp"

#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "vehicle.h"
#include "veh_type.h"
#include "itype.h"
#include "player.h"
#include "cata_utility.h"
#include "options.h"
#include "test_statistics.h"

const efftype_id effect_blind( "blind" );

void clear_game( const ter_id &terrain )
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
}

void set_vehicle_fuel( vehicle &v )
{
    // First we need to find the fuels to set
    // That is, fuels actually used by some engine
    std::set<itype_id> actually_used;
    std::set<itype_id> liquid_fuel;
    for( size_t p = 0; p < v.parts.size(); p++ ) {
        auto &pt = v.parts[ p ];
        if( pt.is_engine() ) {
            if( item::find_type( pt.info().fuel_type )->phase == LIQUID ) {
                liquid_fuel.insert( pt.info().fuel_type );
            }
            actually_used.insert( pt.info().fuel_type );
            pt.enabled = true;
        } else {
            // Disable all parts that use up power or electric cars become non-deterministic
            pt.enabled = false;
        }
    }

    // Need at least one fuel.
    REQUIRE( !actually_used.empty() );

    // Set fuel to a given percentage
    // Batteries are special cased because they aren't liquid fuel
    for( size_t p = 0; p < v.parts.size(); p++ ) {
        auto &pt = v.parts[ p ];

        if( pt.is_battery() ) {
            pt.ammo_set( "battery", pt.ammo_capacity() );
        } else if( pt.is_tank() && !liquid_fuel.empty() ) {
            auto fuel = liquid_fuel.begin();
            pt.ammo_set( *fuel, long( -1 >> 1 ) );
            liquid_fuel.erase( fuel );
        } else {
            pt.ammo_unset();
        }
    }
}

float fuel_liters_used( vehicle &v )
{
    float used = 0.0f;
    for( size_t p = 0; p < v.parts.size(); p++ ) {
        auto &pt = v.parts[ p ];

        if( ( pt.is_tank() || pt.is_battery() ) && pt.ammo_current() != "null" ) {
            used += pt.ammo_capacity() - pt.ammo_remaining();
        }
    }
    return used;
}

const int cycle_limit = 100;

// Algorithm goes as follows:
// Clear map
// Spawn a vehicle
// Give it fuel - remember exact fuel counts that were set here
// Drive it for a while, always moving it back to start point every turn to avoid it going off the bubble
// When moving back, record the sum of the tiles moved so far
// Repeat that for a set number of turns or until all fuel is drained
// Compare distance to fuel usage to get and return mileage.
long test_efficiency( const vproto_id &veh_id, const ter_id &terrain,
                      int reset_velocity_turn, long target_distance,
                      bool smooth_stops = false )
{
    long min_dist = target_distance * 0.9;
    long max_dist = target_distance * 1.1;
    clear_game( terrain );

    const tripoint map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = g->m.add_vehicle( veh_id, map_starting_point, -90, 100, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return 0;
    }

    vehicle &veh = *veh_ptr;

    // Remove all items from cargo to normalize weight.
    for( size_t p = 0; p < veh.parts.size(); p++ ) {
        auto &pt = veh.parts[ p ];
        while( veh.remove_item( p, 0 ) );
    }
    set_vehicle_fuel( veh );

    const tripoint starting_point = veh.global_pos3();
    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;

    // Don't go above ~60mph, the drag disrupts the results.
    veh.cruise_velocity = std::min( 27.0f, veh.safe_velocity() );
    // If we aren't testing repeated cold starts, start the vehicle at cruising velocity.
    if( reset_velocity_turn == -1 ) {
        veh.velocity = veh.cruise_velocity;
    }
    int reset_counter = 0;
    long tiles_travelled = 0;
    int turn_count = 0;
    int cycles_left = cycle_limit;
    bool accelerating = true;
    CHECK( veh.safe_velocity() > 0 );
    while( veh.engine_on && veh.safe_velocity() > 0 && cycles_left > 0 ) {
        cycles_left--;
        g->m.vehmove();
        veh.idle( true );
        // If the vehicle starts skidding, the effects become random and test is RUINED
        REQUIRE( !veh.skidding );
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
                veh.cruise_velocity = accelerating ? veh.safe_velocity() : 0;
            } else {
                veh.stop();
            }
            reset_counter = 0;
        }
    }

    float fuel_used = fuel_liters_used( veh );
    \
    long adjusted_tiles_travelled = 1000.0 * tiles_travelled / fuel_used;
    CHECK( adjusted_tiles_travelled >= min_dist );
    CHECK( adjusted_tiles_travelled <= max_dist );
    //    printf( "Mass: %d, Power: %d, Meters: %d, mL: %d\n", int(veh.total_mass().value() / 1000), (int)veh.total_power(), (int)tiles_travelled, (int)fuel_used );

    return adjusted_tiles_travelled;
}

void test_power( const vproto_id &veh_id, const ter_id &terrain, double target_power,
                 double target_acceleration, double target_max_vel, double target_safe_vel )
{
    double min_power = target_power * 0.9, min_acceleration = target_acceleration * 0.9;
    double max_power = target_power * 1.1, max_acceleration = target_acceleration * 1.1;
    double min_safe_vel = target_safe_vel * 0.9, min_max_vel = target_max_vel * 0.9;
    double max_safe_vel = target_safe_vel * 1.1, max_max_vel = target_max_vel * 1.1;
    clear_game( terrain );

    const tripoint map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = g->m.add_vehicle( veh_id, map_starting_point, -90, 100, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return;
    }

    vehicle &veh = *veh_ptr;
    set_vehicle_fuel( veh );
    // Remove all items from cargo to normalize weight.
    for( size_t p = 0; p < veh.parts.size(); p++ ) {
        auto &pt = veh.parts[ p ];
        while( veh.remove_item( p, 0 ) );
    }
    set_vehicle_fuel( veh );

    const tripoint starting_point = veh.global_pos3();
    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;

    // More than 13hp per ton.
    CHECK( veh.total_power( false ) / ( veh.total_mass().value() / ( 1000.0 * 1000.0 ) ) >= 10 );
    // Power
    CHECK( veh.total_power( false ) >= min_power );
    CHECK( veh.total_power( false ) <= max_power );
    // Acceleration
    CHECK( veh.acceleration( false ) >= min_acceleration );
    CHECK( veh.acceleration( false ) <= max_acceleration );
    // Maximum Velocity
    CHECK( veh.max_velocity( false ) >= min_max_vel );
    CHECK( veh.max_velocity( false ) <= max_max_vel );
    // Safe Velocity
    CHECK( veh.safe_velocity( false ) >= min_safe_vel );
    CHECK( veh.safe_velocity( false ) <= max_safe_vel );
}

void test_traction( const vproto_id &veh_id, const ter_id &terrain, double target_traction, double target_friction )
{
    double min_traction = target_traction * 0.9;
    double max_traction = target_traction * 1.1;
    double min_friction = target_friction * 0.9;
    double max_friction = target_friction * 1.1;
    clear_game( terrain );
    
    const tripoint map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = g->m.add_vehicle( veh_id, map_starting_point, -90, 100, 0 );
    
    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return;
    }
    
    vehicle &veh = *veh_ptr;
    // Remove all items from cargo to normalize weight.
    for( size_t p = 0; p < veh.parts.size(); p++ ) {
        auto &pt = veh.parts[ p ];
        while( veh.remove_item( p, 0 ) );
    }
    set_vehicle_fuel( veh );
    
    const tripoint starting_point = veh.global_pos3();
    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;
    
    CHECK( min_traction <= veh.k_traction() * 100 );
    CHECK( max_traction >= veh.k_traction() * 100 );
    CHECK( min_friction <= veh.k_friction() * 100 );
    CHECK( max_friction >= veh.k_friction() * 100 );
}

void test_drag( const vproto_id &veh_id, const ter_id &terrain, double target_drag )
{
    double min_drag = target_drag * 0.9;
    double max_drag = target_drag * 1.1;
    clear_game( terrain );

    const tripoint map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = g->m.add_vehicle( veh_id, map_starting_point, -90, 100, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return;
    }

    vehicle &veh = *veh_ptr;
    
    CHECK( min_drag <= veh.k_aerodynamics() * 100 );
    CHECK( max_drag >= veh.k_aerodynamics() * 100 );
}

statistics find_inner( std::string type, std::string terrain, int delay, bool smooth )
{
    statistics efficiency;
    for( int i = 0; i < 10; i++ ) {
        efficiency.add( test_efficiency( vproto_id( type ), ter_id( terrain ), delay, -1, smooth ) );
    }
    return efficiency;
}

void print_stats( const statistics &st )
{
    if( st.min() == st.max() ) {
        printf( "All results %d.\n", st.min() );
    } else {
        printf( "Min %d, Max %d, Midpoint %f.\n", st.min(), st.max(), ( st.min() + st.max() ) / 2.0 );
    }
}

void print_efficiency( const std::string &type, const std::string &terrain, int delay, bool smooth )
{
    printf( "Testing %s on %s with %s: ",
            type.c_str(), terrain.c_str(), ( delay < 0 ) ? "no resets" : "resets every 5 turns" );
    print_stats( find_inner( type, terrain, delay, smooth ) );
}

void find_efficiency( std::string type )
{
    SECTION( "finding efficiency of " + type ) {
        print_efficiency( type, "t_pavement", -1, false );
        print_efficiency( type, "t_dirt", -1, false );
        print_efficiency( type, "t_pavement", 5, false );
        print_efficiency( type, "t_dirt", 5, false );
    }
}

int average_from_stat( const statistics &st )
{
    int ugly_integer = ( st.min() + st.max() ) / 2.0;
    // Round to 4 most significant places
    int magnitude = std::max<int>( 0, std::floor( std::log10( ugly_integer ) ) );
    int precision = std::max<int>( 1, std::round( std::pow( 10.0, magnitude - 3 ) ) );
    return ugly_integer - ugly_integer % precision;
}

// Behold: power of laziness
void print_test_strings( std::string type )
{
    std::ostringstream ss;
    ss << "test_vehicle( \"" << type << "\", ";
    ss << average_from_stat( find_inner( type, "t_pavement", -1, false ) ) << ", ";
    ss << average_from_stat( find_inner( type, "t_dirt", -1, false ) ) << ", ";
    ss << average_from_stat( find_inner( type, "t_pavement", 5, false ) ) << ", ";
    ss << average_from_stat( find_inner( type, "t_dirt", 5, false ) );
    ss << " );" << std::endl;
    printf( "%s", ss.str().c_str() );
    fflush( stdout );
}

void test_vehicle( std::string type,
                   long pavement_target, long dirt_target,
                   long pavement_target_w_stops, long dirt_target_w_stops, double target_drag, double target_power,
                   double target_acceleration, double target_max_vel, double target_safe_vel, double target_traction, double target_friction, double target_traction_dirt, double target_friction_dirt,
                   long pavement_target_smooth_stops = 0, long dirt_target_smooth_stops = 0 )
{
    SECTION( type + " on pavement" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_pavement" ), -1, pavement_target );
    }
    SECTION( type + " on dirt" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_dirt" ), -1, dirt_target );
    }
    SECTION( type + " on pavement, full stop every 5 turns" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_pavement" ), 5, pavement_target_w_stops );
    }
    SECTION( type + " on dirt, full stop every 5 turns" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_dirt" ), 5, dirt_target_w_stops );
    }
    SECTION( type + " power test" ) {
        test_power( vproto_id( type ), ter_id( "t_pavement" ), target_power, target_acceleration,
                    target_max_vel, target_safe_vel );
    }
    SECTION( type + " drag test" ) {
        test_drag( vproto_id( type ), ter_id( "t_pavement" ), target_drag );
    }
    SECTION( type + " pavement traction test" ) {
        test_traction( vproto_id( type ), ter_id( "t_pavement" ), target_traction, target_friction );
    }
    SECTION( type + " dirt traction test" ) {
        test_traction( vproto_id( type ), ter_id( "t_dirt" ), target_traction_dirt, target_friction_dirt );
    }
    if( pavement_target_smooth_stops > 0 ) {
        SECTION( type + " on pavement, alternating 5 turns of acceleration and 5 turns of decceleration" ) {
            test_efficiency( vproto_id( type ), ter_id( "t_pavement" ), 5, pavement_target_smooth_stops, true );
        }
    }
    if( dirt_target_smooth_stops > 0 ) {
        SECTION( type + " on dirt, alternating 5 turns of acceleration and 5 turns of decceleration" ) {
            test_efficiency( vproto_id( type ), ter_id( "t_dirt" ), 5, dirt_target_smooth_stops, true );
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
    // PaveCruise, DirtCruise, PaveAccel, DirtAccel, Drag, Power, Accel, MaxVel, MinVel, PaveTraction, PaveFriction, DirtTraction, DirtFriction.
    test_vehicle( "beetle", 10000, 8000, 3000, 2700, 30, 56, 8, 67, 33, 100, 0.6, 60, 4 );
    test_vehicle( "car", 10000, 8000, 3000, 2400, 30, 93, 9, 69, 35, 100, 0.7, 60, 5.2 );
    test_vehicle( "car_sports", 9000, 8000, 3400, 2800, 30, 287, 9, 71, 36, 100, 0.75, 60, 5.5 );
    //    test_vehicle( "electric_car", 62800, 45280, 3590, 2519 );
    test_vehicle( "suv", 10000, 8000, 3000, 2400, 30, 93, 9, 73, 37, 100, 0.8, 60, 5.5 );
    test_vehicle( "motorcycle", 13000, 12500, 7000, 6000, 20, 22, 7, 81, 40, 75, 0.6, 50, 4.5 );
    test_vehicle( "quad_bike", 11000, 9000, 5000, 4000, 20, 22, 7, 55, 27, 75, 0.8, 50, 6 );
    test_vehicle( "scooter", 13000, 11600, 8000, 6000, 20, 12, 7, 60, 30, 75, 0.5, 46, 3.6 );
    test_vehicle( "superbike", 13000, 13000, 7500, 6000, 20, 58, 7, 85, 42, 75, 0.7, 50, 5 );
    test_vehicle( "ambulance", 3500, 3000, 690, 620, 40, 152, 6, 50, 25, 80, 0.5, 53, 3.5 );
    test_vehicle( "fire_engine", 3400, 2500, 360, 330, 40, 152, 4, 54, 27, 80, 0.6, 53, 4 );
    test_vehicle( "fire_truck", 4000, 2500, 400, 320, 50, 152, 6, 73, 37, 80, 1, 53, 6.5 );
    test_vehicle( "truck_swat", 4000, 3000, 630, 530, 55, 140, 5, 58, 29, 80, 1, 53, 6 );
    test_vehicle( "tractor_plow", 8600, 8300, 1600, 1500, 20, 140, 5, 72, 36, 80, 0.3, 53, 2.2 );
    test_vehicle( "apc", 2500, 1500, 220, 170, 50, 140, 1.8, 37, 18, 100, 0.6, 66, 4 );
    test_vehicle( "humvee", 5000, 3700, 530, 480, 50, 231, 6, 75, 38, 100, 0.5, 66, 4 );
}
