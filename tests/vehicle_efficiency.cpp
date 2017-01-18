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
    }
}

const float fuel_level = 0.01;

long test_efficiency( const vproto_id &veh_id, const ter_id &terrain, int reset_velocity_turn, long min_dist, long max_dist )
{
    clear_game( terrain );

    const tripoint map_starting_point( 60, 60, 0 );
    vehicle *veh_ptr = g->m.add_vehicle( veh_id, map_starting_point, -90, 0, 0 );

    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return 0;
    }

    set_vehicle_fuel( veh_ptr, fuel_level );
    vehicle &veh = *veh_ptr;
    const tripoint starting_point = veh.global_pos3();
    veh.tags.insert( "IN_CONTROL_OVERRIDE" );
    veh.engine_on = true;
    REQUIRE( veh.safe_velocity() > 0 );
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
    CHECK( adjusted_tiles_travelled >= min_dist );
    CHECK( adjusted_tiles_travelled <= max_dist );
    return adjusted_tiles_travelled;
}

void find_inner( std::string type, std::string terrain, int delay ) {
    statistics efficiency;
    for( int i = 0; i < 10; i++) {
        efficiency.add( test_efficiency( vproto_id( type ), ter_id( terrain ), delay, 0, 0 ) );
    }
    printf( "Testing %s on %s with %s: Min %d, Max %d, Midpoint %f.\n",
	    type.c_str(), terrain.c_str(), (delay < 0) ? "no resets" : "resets every 5 turns",
	    efficiency.min(), efficiency.max(), ( efficiency.min() + efficiency.max() ) / 2.0 );
}

void find_efficiency( std::string type ) {
    SECTION( "finding efficiency of " + type ) {
        find_inner( type, "t_pavement", -1 );
        find_inner( type, "t_dirt", -1 );
        find_inner( type, "t_pavement", 5 );
        find_inner( type, "t_dirt", 5 );
    }
}

void test_vehicle( std::string type, long pavement_target, long dirt_target, long pavement_target_w_stops, long dirt_target_w_stops ) {
    SECTION( type + " on pavement" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_pavement" ), -1, pavement_target * 0.8, pavement_target * 1.2 );
    }
    SECTION( type + " on dirt" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_dirt" ), -1, dirt_target * 0.8, dirt_target * 1.2 );
    }
    SECTION( type + " on pavement, full stop every 5 turns" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_pavement" ), 5, pavement_target_w_stops * 0.8, pavement_target_w_stops * 1.2 );
    }
    SECTION( type + " on dirt, full stop every 5 turns" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_dirt" ), 5, dirt_target_w_stops * 0.8, dirt_target_w_stops * 1.2 );
    }
}

/** This isn't a test per se, it executes this code to
 * determine the current state of vehicle efficiency.
 **/
TEST_CASE( "vehicle_find_efficiency", "[.]" ) {
  find_efficiency( "beetle" );
  find_efficiency( "car" );
  find_efficiency( "car_sports" );
  // Electric car seems to spawn with no charge.
  //find_efficiency( "electric_car", 300 );
  find_efficiency( "suv" );
  find_efficiency( "motorcycle" );
  find_efficiency( "quad_bike" );
  find_efficiency( "scooter" );
  find_efficiency( "superbike" );
  find_efficiency( "ambulance" );
  find_efficiency( "fire_engine" );
  find_efficiency( "fire_truck" );
  find_efficiency( "truck_swat" );
  find_efficiency( "tractor_plow" );
  find_efficiency( "aapc-mg" );
  find_efficiency( "apc" );
  find_efficiency( "humvee" );
}

// TODO:
// Amount of fuel needed to reach safe speed.
// Amount of cruising range for a fixed amount of fuel.
// Fix test for electric vehicles
TEST_CASE( "vehicle_efficiency", "[vehicle] [engine]" ) {
  test_vehicle( "beetle", 937800, 886750, 47400, 43300 );
  test_vehicle( "car", 907050, 595600, 47650, 31500 );
  test_vehicle( "car_sports", 858200, 429150, 48500, 28850 );
  // Electric car seems to spawn with no charge.
  //test_vehicle( "electric_car", 300 );
  test_vehicle( "suv", 1953850, 1078100, 98800, 54300 );
  test_vehicle( "motorcycle", 133850, 68450, 11350, 6450 );
  test_vehicle( "quad_bike", 113800, 78600, 9700, 6450 );
  test_vehicle( "scooter", 130550, 121900, 11150, 10400 );
  test_vehicle( "superbike", 121950, 40700, 11650, 5700 );
  test_vehicle( "ambulance", 1825150, 1647700, 72850, 63950 );
  test_vehicle( "fire_engine", 1859050, 1752900, 82250, 77350 );
  test_vehicle( "fire_truck", 1445700, 277700, 64350, 15550 );
  test_vehicle( "truck_swat", 1553350, 206350, 69250, 15050 );
  test_vehicle( "tractor_plow", 1045350, 1091050, 47000, 47000 );
  test_vehicle( "aapc-mg", 6372750, 5765650, 236900, 203050 );
  test_vehicle( "apc", 6379250, 5894700, 236550, 219450 );
  test_vehicle( "humvee", 1873600, 962600, 79300, 42400 );
}
