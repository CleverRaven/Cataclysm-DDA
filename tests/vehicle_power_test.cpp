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

static void remove_all_vehicles()
{
    VehicleList vehs = g->m.get_vehicles();
    vehicle *veh_ptr;
    for( auto &vehs_v : vehs ) {
        veh_ptr = vehs_v.v;
        g->m.destroy_vehicle( veh_ptr );
    }
}

TEST_CASE( "vehicle power with reactor and solar panels", "[vehicle][power]" )
{
    // Set up a sandbox world
    for( const tripoint &p : g->m.points_in_rectangle( tripoint_zero,
            tripoint( MAPSIZE * SEEX, MAPSIZE * SEEY, 0 ) ) ) {
        g->m.furn_set( p, furn_id( "f_null" ) );
        g->m.ter_set( p, ter_id( "t_pavement" ) );
        g->m.trap_set( p, trap_id( "tr_null" ) );
        g->m.i_clear( p );
    }

    g->m.invalidate_map_cache( 0 );
    g->m.build_map_cache( 0, true );

    REQUIRE( !g->u.in_vehicle );

    const tripoint player_origin( 15, 15, 0 );
    vehicle *veh_ptr;

    g->u.setpos( player_origin );
    g->refresh_all();

    remove_all_vehicles();
    REQUIRE( g->m.get_vehicles().empty() );

    SECTION( "vehicle with reactor" ) {
        const tripoint reactor_origin = tripoint( 10, 10, 0 );
        veh_ptr = g->m.add_vehicle( vproto_id( "reactor_test" ), reactor_origin, 0, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        g->refresh_all();

        REQUIRE( !veh_ptr->reactors.empty() );
        vehicle_part &reactor = veh_ptr->parts[ veh_ptr->reactors.front() ];

        GIVEN( "the reactor is empty" ) {
            reactor.ammo_unset();
            veh_ptr->discharge_battery( veh_ptr->fuel_left( fuel_type_battery ) );
            REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );

            WHEN( "the reactor is loaded with plutonium fuel" ) {
                reactor.ammo_set( fuel_type_plut_cell, 1 );
                REQUIRE( reactor.ammo_remaining() == 1 );

                AND_WHEN( "the reactor is used to power and charge the battery" ) {
                    veh_ptr->power_parts();
                    THEN( "the reactor should be empty, and the battery should be charged" ) {
                        CHECK( reactor.ammo_remaining() == 0 );
                        CHECK( veh_ptr->fuel_left( fuel_type_battery ) == 100 );
                    }
                }
            }
        }
    }

    SECTION( "vehicle with solar panels" ) {
        const tripoint solar_origin = tripoint( 5, 5, 0 );
        veh_ptr = g->m.add_vehicle( vproto_id( "solar_panel_test" ), solar_origin, 0, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        g->refresh_all();

        GIVEN( "it is 3 hours after sunrise, with sunny weather" ) {
            calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days;
            const time_point start_time = sunrise( calendar::turn ) + 3_hours;
            veh_ptr->update_time( start_time );
            g->weather.weather_override = WEATHER_SUNNY;

            AND_GIVEN( "the battery has no charge" ) {
                veh_ptr->discharge_battery( veh_ptr->fuel_left( fuel_type_battery ) );
                REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );

                WHEN( "30 minutes elapse" ) {
                    veh_ptr->update_time( start_time + 30_minutes );

                    THEN( "the battery should be partially charged" ) {
                        int charge = veh_ptr->fuel_left( fuel_type_battery ) / 100;
                        CHECK( 10 <= charge );
                        CHECK( charge <= 15 );

                        AND_WHEN( "another 30 minutes elapse" ) {
                            veh_ptr->update_time( start_time + 2 * 30_minutes );

                            THEN( "the battery should be further charged" ) {
                                charge = veh_ptr->fuel_left( fuel_type_battery ) / 100;
                                CHECK( 20 <= charge );
                                CHECK( charge <= 30 );
                            }
                        }
                    }
                }
            }
        }

        GIVEN( "it is 3 hours after sunset, with clear weather" ) {
            const time_point at_night = sunset( calendar::turn ) + 3_hours;
            g->weather.weather_override = WEATHER_CLEAR;
            veh_ptr->update_time( at_night );

            AND_GIVEN( "the battery has no charge" ) {
                veh_ptr->discharge_battery( veh_ptr->fuel_left( fuel_type_battery ) );
                REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 0 );

                WHEN( "60 minutes elapse" ) {
                    veh_ptr->update_time( at_night + 2 * 30_minutes );

                    THEN( "the battery should still have no charge" ) {
                        CHECK( veh_ptr->fuel_left( fuel_type_battery ) == 0 );
                    }
                }
            }
        }
    }
}
