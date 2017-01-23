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

    for( auto &veh : g->m.get_vehicles( tripoint( 0, 0, 0 ), tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        g->m.destroy_vehicle( veh.v );
    }

    g->m.build_map_cache( 0, true );
}

void test_efficiency( const vproto_id &veh_id, const ter_id &terrain, int reset_velocity_turn, long min_dist, long max_dist )
{
    clear_game( terrain );

    const tripoint starting_point( 60, 60, 0 );
    vehicle *veh_ptr = g->m.add_vehicle( veh_id, starting_point, -90, 5, 0 );
    REQUIRE( veh_ptr != nullptr );
    if( veh_ptr == nullptr ) {
        return;
    }

    vehicle &veh = *veh_ptr;
    veh.engine_on = true;
    REQUIRE( (bool)veh.current_engine() );
    REQUIRE( veh.safe_velocity( veh.current_engine() ) > 0 );
    veh.cruise_velocity = veh.safe_velocity( veh.current_engine() ) * 2.237 * 100;
    int reset_counter = 0;
    long tiles_travelled = 0;
    int turn_count = 0;
    while( veh.engine_on && veh.safe_velocity( veh.current_engine() ) > 0 ) {
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

    CHECK( tiles_travelled >= min_dist );
    CHECK( tiles_travelled <= max_dist );
}

void test_vehicle( std::string type, long target ) {
    SECTION( type + " on pavement" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_pavement" ), -1, target * 0.9, target * 1.1 );
    }
    SECTION( type + " on dirt" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_dirt" ), -1, target * 0.45, target * 1.1 );
    }
    SECTION( type + " on pavement, full stop every 5 turns" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_pavement" ), 5, target * 0.045, target * 0.1 );
    }
    SECTION( type + " on dirt, full stop every 5 turns" ) {
        test_efficiency( vproto_id( type ), ter_id( "t_dirt" ), 5, target * 0.025, target * 0.1 );
    }
}

TEST_CASE( "vehicle_efficiency", "[vehicle] [engine]" ) {
    options_manager &options = get_options();
    options.get_option( "VEHICLES_CONTROLLED" ).setValue( "true" );
    REQUIRE( get_option<bool>( "VEHICLES_CONTROLLED" ) );

    test_vehicle( "beetle", 48000 );
    test_vehicle( "car", 48000 );
    test_vehicle( "car_sports", 51400 );
    //test_vehicle( "electric_car", 300 );
    test_vehicle( "suv", 99000 );
    test_vehicle( "motorcycle", 7400 );
    test_vehicle( "quad_bike", 7100 );
    test_vehicle( "scooter", 6400 );
    test_vehicle( "superbike", 8334 );
}
