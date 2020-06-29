#include <cstdlib>
#include <memory>
#include <vector>

#include "avatar.h"
#include "bodypart.h"
#include "calendar.h"
#include "catch/catch.hpp"
#include "map.h"
#include "map_helpers.h"
#include "point.h"
#include "type_id.h"
#include "vehicle.h"
#include "weather.h"

static const itype_id fuel_type_battery( "battery" );
static const itype_id fuel_type_plut_cell( "plut_cell" );
static const efftype_id effect_blind( "blind" );

static void reset_player()
{
    avatar &player_character = get_avatar();
    // Move player somewhere safe
    REQUIRE( !player_character.in_vehicle );
    player_character.setpos( tripoint_zero );
    // Blind the player to avoid needless drawing-related overhead
    player_character.add_effect( effect_blind, 1_turns, num_bp, true );
}

TEST_CASE( "vehicle power with reactor and solar panels", "[vehicle][power]" )
{
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    clear_vehicles();
    map &here = get_map();

    SECTION( "vehicle with reactor" ) {
        const tripoint reactor_origin = tripoint( 10, 10, 0 );
        vehicle *veh_ptr = here.add_vehicle( vproto_id( "reactor_test" ), reactor_origin, 0, 0, 0 );
        REQUIRE( veh_ptr != nullptr );

        REQUIRE( !veh_ptr->reactors.empty() );
        vehicle_part &reactor = veh_ptr->part( veh_ptr->reactors.front() );

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
        vehicle *veh_ptr = here.add_vehicle( vproto_id( "solar_panel_test" ), solar_origin, 0, 0, 0 );
        REQUIRE( veh_ptr != nullptr );

        GIVEN( "it is 3 hours after sunrise, with sunny weather" ) {
            calendar::turn = calendar::turn_zero + calendar::season_length() + 1_days;
            const time_point start_time = sunrise( calendar::turn ) + 3_hours;
            veh_ptr->update_time( start_time );
            get_weather().weather_override = WEATHER_SUNNY;

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
            get_weather().weather_override = WEATHER_CLEAR;
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

TEST_CASE( "maximum reverse velocity", "[vehicle][power][reverse]" )
{
    reset_player();
    build_test_map( ter_id( "t_pavement" ) );
    clear_vehicles();
    map &here = get_map();

    GIVEN( "a scooter with combustion engine and charged battery" ) {
        const tripoint origin = tripoint( 10, 0, 0 );
        vehicle *veh_ptr = here.add_vehicle( vproto_id( "scooter_test" ), origin, 0, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        veh_ptr->charge_battery( 500 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 500 );

        WHEN( "the engine is started" ) {
            veh_ptr->start_engines();

            THEN( "it can go in both forward and reverse" ) {
                int max_fwd = veh_ptr->max_velocity( false );
                int max_rev = veh_ptr->max_reverse_velocity( false );

                CHECK( max_rev < 0 );
                CHECK( max_fwd > 0 );

                AND_THEN( "its maximum reverse velocity is 1/4 of the maximum forward velocity" ) {
                    CHECK( std::abs( max_fwd / max_rev ) == 4 );
                }
            }

        }
    }

    GIVEN( "a scooter with an electric motor and charged battery" ) {
        const tripoint origin = tripoint( 15, 0, 0 );
        vehicle *veh_ptr = here.add_vehicle( vproto_id( "scooter_electric_test" ), origin, 0, 0, 0 );
        REQUIRE( veh_ptr != nullptr );
        veh_ptr->charge_battery( 5000 );
        REQUIRE( veh_ptr->fuel_left( fuel_type_battery ) == 5000 );

        WHEN( "the engine is started" ) {
            veh_ptr->start_engines();

            THEN( "it can go in both forward and reverse" ) {
                int max_fwd = veh_ptr->max_velocity( false );
                int max_rev = veh_ptr->max_reverse_velocity( false );

                CHECK( max_rev < 0 );
                CHECK( max_fwd > 0 );

                AND_THEN( "its maximum reverse velocity is equal to maximum forward velocity" ) {
                    CHECK( std::abs( max_rev ) == std::abs( max_fwd ) );
                }
            }
        }
    }
}

