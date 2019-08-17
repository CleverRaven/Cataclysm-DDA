#include <memory>
#include <vector>

#include "avatar.h"
#include "catch/catch.hpp"
#include "game.h"
#include "map.h"
#include "map_iterator.h"
#include "vehicle.h"
#include "calendar.h"
#include "weather.h"
#include "game_constants.h"
#include "mapdata.h"
#include "type_id.h"
#include "point.h"

static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_plut_cell( "plut_cell" );

TEST_CASE( "vehicle_power" )
{
    GIVEN( "Reactor and solar panels" ) {
        for( const tripoint &p : g->m.points_in_rectangle( tripoint_zero,
                tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
            g->m.furn_set( p, furn_id( "f_null" ) );
            g->m.ter_set( p, ter_id( "t_pavement" ) );
            g->m.trap_set( p, trap_id( "tr_null" ) );
            g->m.i_clear( p );
        }

        g->m.invalidate_map_cache( 0 );
        g->m.build_map_cache( 0, true );

        CHECK( !g->u.in_vehicle );
        const tripoint test_origin( 15, 15, 0 );
        g->u.setpos( test_origin );
        const tripoint vehicle_origin = tripoint( 10, 10, 0 );
        VehicleList vehs = g->m.get_vehicles();
        vehicle *veh_ptr;
        for( auto &vehs_v : vehs ) {
            veh_ptr = vehs_v.v;
            g->m.destroy_vehicle( veh_ptr );
        }
        g->refresh_all();
        REQUIRE( g->m.get_vehicles().empty() );
        veh_ptr = g->m.add_vehicle( vproto_id( "reactor_test" ), vehicle_origin, 0, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        g->refresh_all();
        REQUIRE( !veh_ptr->reactors.empty() );
        vehicle_part &reactor = veh_ptr->parts[ veh_ptr->reactors.front() ];
        reactor.ammo_unset();
        veh_ptr->discharge_battery( veh_ptr->fuel_left( fuel_type_battery ) );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        reactor.ammo_set( fuel_type_plut_cell, 1 );
        REQUIRE( reactor.ammo_remaining() == 1 );
        veh_ptr->power_parts();
        CHECK( reactor.ammo_remaining() == 0 );
        CHECK( veh_ptr->fuel_left( fuel_type_battery ) == 100 );
        g->m.destroy_vehicle( veh_ptr );
        g->refresh_all();
        REQUIRE( g->m.get_vehicles().empty() );
        const tripoint solar_origin = tripoint( 5, 5, 0 );
        veh_ptr = g->m.add_vehicle( vproto_id( "solar_panel_test" ), solar_origin, 0, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        g->refresh_all();
        calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days;
        const time_point start_time = sunrise( calendar::turn ) + 3_hours;
        veh_ptr->update_time( start_time );
        veh_ptr->discharge_battery( veh_ptr->fuel_left( fuel_type_battery ) );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
        g->weather.weather_override = WEATHER_SUNNY;
        veh_ptr->update_time( start_time + 30_minutes );
        int approx_battery1 = veh_ptr->fuel_left( fuel_type_battery ) / 100;
        const int exp_min = 10;
        const int exp_max = 15;
        CHECK( approx_battery1 >= exp_min );
        CHECK( approx_battery1 <= exp_max );
        veh_ptr->update_time( start_time + 2 * 30_minutes );
        int approx_battery2 = veh_ptr->fuel_left( fuel_type_battery ) / 100;
        CHECK( approx_battery2 >= approx_battery1 + exp_min );
        CHECK( approx_battery2 <= approx_battery1 + exp_max );
    }
}
