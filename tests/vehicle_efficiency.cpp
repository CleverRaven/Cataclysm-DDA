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
    g->u.add_effect( effect_blind, 1, num_bp, true );

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
            pt.ammo_set( *fuel, long(-1 >> 1) );
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

    veh.cruise_velocity = veh.safe_velocity();
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
        printf( "Used: %d, Distance: %ld, Velocity: %.2f\n",
               int(fuel_liters_used( veh )),
               tiles_travelled, veh.velocity);
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
    
    float fuel_used = fuel_liters_used( veh );\
    long adjusted_tiles_travelled = 1000.0 * tiles_travelled / fuel_used;
    CHECK( adjusted_tiles_travelled >= min_dist );
    CHECK( adjusted_tiles_travelled <= max_dist );
    printf( "Mass: %d, Power: %d, Meters: %d, mL: %d\n", int(veh.total_mass().value() / 1000), (int)veh.total_power(), (int)tiles_travelled, (int)fuel_used );

    return adjusted_tiles_travelled;
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
                   long pavement_target_w_stops, long dirt_target_w_stops,
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
    // Max/Min kmpL at cruise, Max/Min mpL while accelerating.
//    test_vehicle( "beetle", 117600, 113600, 12580, 11550 );
    test_vehicle( "car", 13000, 10000, 2600, 2400 );
//    test_vehicle( "car_sports", 243500, 164300, 15780, 9458 );
//    test_vehicle( "electric_car", 62800, 45280, 3590, 2519 );
//    test_vehicle( "suv", 294300, 221000, 27040, 15720 );
//    test_vehicle( "motorcycle", 15370, 13050, 2304, 1322 );
//    test_vehicle( "quad_bike", 11400, 10650, 1963, 1302 );
//    test_vehicle( "scooter", 9692, 9692, 1723, 1723 );
//    test_vehicle( "superbike", 32350, 24760, 3322, 1576 );
//    test_vehicle( "ambulance", 253600, 233000, 22480, 19650 );
//    test_vehicle( "fire_engine", 296300, 286300, 24780, 23800 );
//    test_vehicle( "fire_truck", 220100, 66910, 18740, 4813 );
//    test_vehicle( "truck_swat", 198100, 69170, 21020, 4691 );
//    test_vehicle( "tractor_plow", 145500, 145500, 14120, 14120 );
//    test_vehicle( "apc", 623700, 593300, 65960, 60720 );
//    test_vehicle( "humvee", 2500, 2200, 600, 550 );
}
