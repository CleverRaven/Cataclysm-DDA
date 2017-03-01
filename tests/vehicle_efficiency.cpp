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

void clear_game( const ter_id &terrain )
{
    while( g->num_zombies() > 0 ) {
        g->remove_zombie( 0 );
    }

    g->unload_npcs();

    // Move player somewhere safe
    g->u.setpos( tripoint( 0, 0, 0 ) );

    for( const tripoint &p : g->m.points_in_rectangle( tripoint( 0, 0, 0 ),
                                                       tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        g->m.furn_set( p, furn_id( "f_null" ) );
        g->m.ter_set( p, terrain );
        g->m.trap_set( p, trap_id( "tr_null" ) );
        g->m.i_clear( p );
    }

    for( wrapped_vehicle &veh : g->m.get_vehicles( tripoint( 0, 0, 0 ), tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        g->m.destroy_vehicle( veh.v );
    }

    g->m.build_map_cache( 0, true );
}

void set_vehicle_fuel( vehicle *v, float veh_fuel_mult ) {
    for( size_t p = 0; p < v->parts.size(); p++ ) {
        auto &pt = v->parts[ p ];

       if( pt.is_battery() ) {
            pt.ammo_set( "battery", pt.ammo_capacity() * veh_fuel_mult );
        }

        if( pt.is_tank() && v->type->parts[p].fuel != "null" ) {
            float qty = pt.ammo_capacity() * veh_fuel_mult;
            qty *= std::max( item::find_type( v->type->parts[p].fuel )->stack_size, 1 );
            qty /= to_milliliter( units::legacy_volume_factor );
            pt.ammo_set( v->type->parts[ p ].fuel, qty );
        }

        if( pt.is_engine() ) {
            pt.enabled = true;
        }
    }
}

const float fuel_level = 0.001;

long test_efficiency( const vproto_id &veh_id, const ter_id &terrain, int reset_velocity_turn, long target_distance )
{
    long min_dist = target_distance * 0.99;
    long max_dist = target_distance * 1.01;
    clear_game( terrain );

    const tripoint map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = g->m.add_vehicle( veh_id, map_starting_point, -90, 100, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return 0;
    }

    // Remove all items from cargo to normalize weight.
    for( size_t p = 0; p < veh_ptr->parts.size(); p++ ) {
        auto &pt = veh_ptr->parts[ p ];
        while( veh_ptr->remove_item( p, 0 ) );
    }
    set_vehicle_fuel( veh_ptr, fuel_level );
    vehicle &veh = *veh_ptr;
    const tripoint starting_point = veh.global_pos3();
    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;

    veh.cruise_velocity = veh.safe_velocity();
    // If we aren't testing repeated cold starts, start the vehicle at cruising velocity.
    // Otherwise changing the amount of fuel in the tank perturbs the test results.
    if( reset_velocity_turn == -1 ) {
        veh.velocity = veh.cruise_velocity;
    }
    int reset_counter = 0;
    long tiles_travelled = 0;
    int turn_count = 0;
    while( veh.engine_on && veh.safe_velocity() > 0 ) {
        g->m.vehmove();
        veh.idle( true );
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
            veh.velocity = 0;
            veh.last_turn = 0;
            veh.of_turn_carry = 0;
            reset_counter = 0;
        }
    }

    long adjusted_tiles_travelled = tiles_travelled / fuel_level;
    if( target_distance >= 0 ) {
        CHECK( adjusted_tiles_travelled >= min_dist );
        CHECK( adjusted_tiles_travelled <= max_dist );
    }

    return adjusted_tiles_travelled;
}

int find_inner( std::string type, std::string terrain, int delay, bool print = true ) {
    statistics efficiency;
    for( int i = 0; i < 10; i++) {
        efficiency.add( test_efficiency( vproto_id( type ), ter_id( terrain ), delay, -1 ) );
    }
    if( print ) {
        printf( "Testing %s on %s with %s: Min %d, Max %d, Midpoint %f.\n",
                type.c_str(), terrain.c_str(), (delay < 0) ? "no resets" : "resets every 5 turns",
                efficiency.min(), efficiency.max(), ( efficiency.min() + efficiency.max() ) / 2.0 );
    }
    return ( efficiency.min() + efficiency.max() ) / 2.0;
}

void find_efficiency( std::string type ) {
    SECTION( "finding efficiency of " + type ) {
        find_inner( type, "t_pavement", -1 );
        find_inner( type, "t_dirt", -1 );
        find_inner( type, "t_pavement", 5 );
        find_inner( type, "t_dirt", 5 );
    }
}

int less_ugly_integer( int ugly_integer )
{
    if( ugly_integer % 10 == 9 ) {
        return ugly_integer + 1;
    }
    return ugly_integer;
}

// Behold: power of laziness
void print_test_strings( std::string type ) {
    std::ostringstream ss;
    ss << "test_vehicle( \"" << type << "\", ";
    ss << less_ugly_integer( find_inner( type, "t_pavement", -1, false ) ) << ", ";
    ss << less_ugly_integer( find_inner( type, "t_dirt", -1, false ) ) << ", ";
    ss << less_ugly_integer( find_inner( type, "t_pavement", 5, false ) ) << ", ";
    ss << less_ugly_integer( find_inner( type, "t_dirt", 5, false ) ) << " );" << std::endl;
    printf( "%s", ss.str().c_str() );
}

void test_vehicle( std::string type, long pavement_target, long dirt_target, long pavement_target_w_stops, long dirt_target_w_stops ) {
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
}};

/** This isn't a test per se, it executes this code to
 * determine the current state of vehicle efficiency.
 **/
TEST_CASE( "vehicle_find_efficiency", "[.]" ) {
    for( const std::string &veh : vehs_to_test ) {
        find_efficiency( veh );
    }
}

/** This is even less of a test. It generates C++ lines for the actual test below */
TEST_CASE( "vehicle_make_efficiency_case", "[.]" ) {
    for( const std::string &veh : vehs_to_test ) {
        print_test_strings( veh );
    }
}

// TODO:
// Amount of fuel needed to reach safe speed.
// Amount of cruising range for a fixed amount of fuel.
// Fix test for electric vehicles
TEST_CASE( "vehicle_efficiency", "[vehicle] [engine]" ) {
    test_vehicle( "beetle", 124000, 124000, 12000, 11000 );
    test_vehicle( "car", 134000, 108000, 13000, 8000 );
    test_vehicle( "car_sports", 320000, 224000, 5000, 2000 );
    test_vehicle( "electric_car", 95000, 79000, 0, 0 );
    test_vehicle( "suv", 333000, 260000, 24000, 14000 );
    test_vehicle( "motorcycle", 14000, 14000, 2000, 1000 );
    test_vehicle( "quad_bike", 10000, 10000, 1000, 1000 );
    test_vehicle( "scooter", 10000, 10000, 1000, 1000 );
    test_vehicle( "superbike", 27000, 27000, 1000, 0 );
    test_vehicle( "ambulance", 373000, 348000, 26000, 23000 );
    test_vehicle( "fire_engine", 371000, 385000, 29000, 28000 );
    test_vehicle( "fire_truck", 313000, 125000, 23000, 8000 );
    test_vehicle( "truck_swat", 253000, 80000, 23000, 7000 );
    test_vehicle( "tractor_plow", 187000, 187000, 14000, 14000 );
    test_vehicle( "apc", 896000, 896000, 70000, 65000 );
    test_vehicle( "humvee", 378000, 279000, 26000, 15000 );
}
