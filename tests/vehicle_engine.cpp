#include "catch/catch.hpp"

#include "game.h"
#include "map.h"
#include "vehicle.h"
#include "veh_type.h"

TEST_CASE( "vehicle_engine", "[vehicle] [engine]" ) {
    SECTION( "check vehicle definitions" ) {
        for( auto e : vehicle_prototype::get_all() ) {
            // prevent created vehicles colliding with terrain and producing wrecks
            for( int x = 0; x <= 100; ++x ) {
                for( int y = 0; y <= 100; ++y ) {
                    g->m.ter_set( x, y, ter_id( "t_grass" ) );
                    g->m.furn_set( x, y, furn_id( "f_null" ) );
                }
            }

            INFO( e->name );
            vehicle &veh = *g->m.add_vehicle( e, 50, 50, 270, 100, 0 );

            for( const auto &pt : veh.parts ) {
                INFO( pt.name() );

                // newly created vehicle should not have any damaged parts
                REQUIRE( ( pt.hp() == pt.info().durability ) );

                if( !pt.is_engine() ) {
                    continue;
                }

                // base part must be a valid engine item
                auto base = veh.part_base( veh.index_of_part( &pt ) );
                REQUIRE( ( base && base->is_engine() ) );

                REQUIRE( veh.optimal_velocity( pt ) <= veh.safe_velocity( pt ) );
                REQUIRE( veh.optimal_velocity( pt ) <= veh.max_velocity( pt ) );
                REQUIRE( veh.safe_velocity( pt ) <= veh.max_velocity( pt ) );

                // sufficient power available to start the engine at 10°C
                REQUIRE( base->engine_start_energy( 10 ) <= veh.fuel_left( "battery" ) );
            }

            g->m.destroy_vehicle( &veh );
        }
    }
}
